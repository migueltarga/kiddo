/**
 * @file devtools.cpp
 * @brief Serial commands for driving and inspecting the UI from a computer
 *
 * Screenshots reuse the normal rendering path: the whole screen is
 * invalidated and redrawn, and every flushed strip is also written to serial
 * as base64 lines ("@S A x1 y1 x2 y2", then "@S D <data>"), framed by
 * "@S BEGIN w h" and "@S END". Pixels are little-endian RGB565. Each line is
 * written in one call so logs from other tasks can't split it.
 */

#ifdef KIDDO_DEVTOOLS

#include "devtools.h"
#include <Arduino.h>
#include "src/display/lv_display_private.h"
#include "config.h"
#include "file_system.h"
#include "remote_catalog.h"
#include "ui/screens/ui_screens.h"
#include <esp_heap_caps.h>

namespace devtools {

static lv_display_t* g_disp = nullptr;
static lv_display_flush_cb_t g_original_flush = nullptr;
static bool g_capturing = false;

/** @brief Injected gesture: press at from, move to to over the duration, release */
static struct {
    bool active = false;
    bool release_pending = false;
    lv_point_t from, to;
    uint32_t start_ms, duration_ms;
} g_touch;

static String g_line;

/**
 * @brief Write a line in a single call
 * @param line Line without the trailing newline
 */
static void write_line(const String& line) {
    String out = line + "\n";
    Serial.write((const uint8_t*)out.c_str(), out.length());
}

/**
 * @brief Base64-encode bytes
 * @param data Bytes
 * @param len Number of bytes
 * @return Encoded text
 */
static String base64(const uint8_t* data, size_t len) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String out;
    out.reserve((len + 2) / 3 * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = (uint32_t)data[i] << 16;
        if (i + 1 < len) n |= (uint32_t)data[i + 1] << 8;
        if (i + 2 < len) n |= data[i + 2];
        out += table[(n >> 18) & 63];
        out += table[(n >> 12) & 63];
        out += i + 1 < len ? table[(n >> 6) & 63] : '=';
        out += i + 2 < len ? table[n & 63] : '=';
    }
    return out;
}

static void capture_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    if (g_capturing) {
        write_line("@S A " + String(area->x1) + " " + String(area->y1) + " " +
                   String(area->x2) + " " + String(area->y2));
        size_t total = (size_t)lv_area_get_width(area) * lv_area_get_height(area) * 2;
        const size_t CHUNK = 768; // multiple of 3, so chunks encode without padding
        for (size_t off = 0; off < total; off += CHUNK) {
            size_t n = total - off < CHUNK ? total - off : CHUNK;
            write_line("@S D " + base64(px_map + off, n));
        }
    }
    g_original_flush(disp, area, px_map);
}

void init(lv_display_t* disp) {
    g_disp = disp;
    g_original_flush = disp->flush_cb;
    lv_display_set_flush_cb(disp, capture_flush);
}

/**
 * @brief Redraw the whole screen while copying it to serial
 */
static void screenshot() {
    write_line("@S BEGIN " + String(SCREEN_WIDTH) + " " + String(SCREEN_HEIGHT));
    g_capturing = true;
    lv_obj_invalidate(lv_display_get_screen_active(g_disp));
    lv_refr_now(g_disp);
    g_capturing = false;
    write_line("@S END");
}

/**
 * @brief Start an injected press/move/release
 */
static void start_gesture(int x1, int y1, int x2, int y2, uint32_t duration_ms) {
    g_touch.from = {(int32_t)x1, (int32_t)y1};
    g_touch.to = {(int32_t)x2, (int32_t)y2};
    g_touch.start_ms = millis();
    g_touch.duration_ms = duration_ms;
    g_touch.active = true;
    g_touch.release_pending = false;
}

static void run_command(const String& line) {
    int a, b, c, d;
    if (line == "shot") {
        screenshot();
    } else if (sscanf(line.c_str(), "tap %d %d", &a, &b) == 2) {
        start_gesture(a, b, a, b, 120);
        write_line("@OK tap");
    } else if (sscanf(line.c_str(), "drag %d %d %d %d", &a, &b, &c, &d) == 4) {
        start_gesture(a, b, c, d, 400);
        write_line("@OK drag");
    } else if (line == "heap") {
        write_line("@HEAP free " + String(ESP.getFreeHeap()) + " largest " +
                   String(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
    } else if (line == "clearimg") {
        // Image cache only; downloaded stories and progress stay
        FileSystem::clearCache();
        write_line("@OK clearimg");
    } else if (line.startsWith("rm /")) {
        // E.g. a downloaded story, so the library fetches the new version
        String path = line.substring(3);
        write_line(FileSystem::deleteFile(path) ? "@OK rm" : "@ERR rm " + path);
    } else if (line.startsWith("catalog ")) {
        // Long URLs are painful on the on-screen keyboard; same as saving it in Settings
        String url = line.substring(8);
        url.trim();
        remote_catalog::setCatalogUrl(url);
        ui_library_catalog_changed();
        write_line("@CATALOG " + remote_catalog::getCatalogUrl());
    } else if (line.length() > 0) {
        write_line("@ERR unknown command: " + line);
    }
}

void poll() {
    while (Serial.available()) {
        char ch = (char)Serial.read();
        if (ch == '\n' || ch == '\r') {
            String line = g_line;
            g_line = "";
            line.trim();
            run_command(line);
        } else if (g_line.length() < 300) {
            g_line += ch;
        }
    }
}

bool read_touch(lv_indev_data_t* data) {
    if (g_touch.release_pending) {
        // One released report at the end point finishes the gesture
        g_touch.release_pending = false;
        data->point = g_touch.to;
        data->state = LV_INDEV_STATE_RELEASED;
        return true;
    }
    if (!g_touch.active) {
        return false;
    }
    uint32_t elapsed = millis() - g_touch.start_ms;
    if (elapsed >= g_touch.duration_ms) {
        g_touch.active = false;
        g_touch.release_pending = true;
        data->point = g_touch.to;
    } else {
        data->point.x = g_touch.from.x + (g_touch.to.x - g_touch.from.x) * (int32_t)elapsed / (int32_t)g_touch.duration_ms;
        data->point.y = g_touch.from.y + (g_touch.to.y - g_touch.from.y) * (int32_t)elapsed / (int32_t)g_touch.duration_ms;
    }
    data->state = LV_INDEV_STATE_PRESSED;
    return true;
}

}

#endif
