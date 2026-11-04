/**
 * @file main.cpp
 * @brief Main application entry point for Kiddo story reader
 *
 * This file contains the main setup and loop functions for the ESP32-based
 * story reader application. It initializes all subsystems including LVGL,
 * touchscreen, WiFi, filesystem, and the UI components.
 */

#include <Arduino.h>

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#ifdef KIDDO_3248S035C
#include <Wire.h>
#endif
#include <Preferences.h>
#include "config.h"
#include "i18n.h"
#include "file_system.h"
#include "async_manager.h"
#include "story_engine.h"
#include "inventory_manager.h"
#include "ui/screens/ui_screens.h"
#include "ui/app_ui.h"
#include "ui/fonts.h"
#include "ui/router.h"
#include "audio.h"
#include "devtools.h"
#include "led_effects.h"
#include "display_comfort.h"

#if __has_include(<WiFi.h>)
#include <WiFi.h>
#endif

/** @brief Touchscreen SPI instance */
SPIClass touchscreenSPI = SPIClass(VSPI);
#ifndef KIDDO_3248S035C
/** @brief Touchscreen driver instance */
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
#else
static uint8_t gt911_address = 0;
static bool gt911_read(uint16_t reg, uint8_t *bytes, size_t count) {
    Wire.beginTransmission(gt911_address);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xff);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(gt911_address, count) != count) return false;
    for (size_t i = 0; i < count; ++i) bytes[i] = Wire.read();
    return true;
}
static void gt911_clear() {
    Wire.beginTransmission(gt911_address);
    Wire.write(0x81);
    Wire.write(0x4e);
    Wire.write(0);
    Wire.endTransmission();
}
static void gt911_init() {
    // GPIO21 (INT) is tied low on some C revisions, so poll the controller.
    pinMode(25, OUTPUT);
    digitalWrite(25, LOW);
    delay(10);
    digitalWrite(25, HIGH);
    delay(60);
    Wire.begin(33, 32);
    for (uint8_t address : {uint8_t(0x5d), uint8_t(0x14)}) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            gt911_address = address;
            Serial.printf("[TOUCH] GT911 at 0x%02x\n", address);
            return;
        }
    }
    Serial.println("[TOUCH] GT911 not found");
}
#endif
/** @brief LVGL display buffer */
uint32_t draw_buf[DRAW_BUF_SIZE / 4];
/** @brief TFT display driver */
TFT_eSPI tft;
/** @brief Preferences storage instance */
Preferences prefs;
/** @brief Current backlight brightness level */
uint8_t brightness = 200;
/** @brief Current story font scale setting */
uint8_t story_font_scale = 1;
/** @brief Online mode enabled flag */
bool online_mode = false;
/** @brief WiFi connection status */
bool wifi_connected = false;

/**
 * @brief LVGL touchscreen read callback
 * @param indev LVGL input device
 * @param data Input data structure to fill
 */
static void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (devtools::read_touch(data))
        return;
#ifdef KIDDO_3248S035C
    data->state = LV_INDEV_STATE_RELEASED;
    if (!gt911_address) return;
    uint8_t status = 0;
    if (!gt911_read(0x814e, &status, 1) || !(status & 0x80)) return;
    if ((status & 0x0f) > 0) {
        uint8_t point[7];
        if (gt911_read(0x814f, point, sizeof(point))) {
            data->point.x = constrain((int)(point[1] | (point[2] << 8)), 0, SCREEN_WIDTH - 1);
            data->point.y = constrain((int)(point[3] | (point[4] << 8)), 0, SCREEN_HEIGHT - 1);
            data->state = LV_INDEV_STATE_PRESSED;
        }
    }
    gt911_clear();
#else
    if (touchscreen.tirqTouched() && touchscreen.touched())
    {
        TS_Point p = touchscreen.getPoint();
        int16_t rx = p.x;
        int16_t ry = p.y;
#ifdef TOUCH_SWAP_XY
        int16_t tmp = rx;
        rx = ry;
        ry = tmp;
#endif
        int16_t px = map(rx, TOUCH_X_MIN, TOUCH_X_MAX, 1, SCREEN_WIDTH);
        int16_t py = map(ry, TOUCH_Y_MIN, TOUCH_Y_MAX, 1, SCREEN_HEIGHT);
#ifdef TOUCH_INVERT_X
        px = (SCREEN_WIDTH + 1) - px;
#endif
#ifdef TOUCH_INVERT_Y
        py = (SCREEN_HEIGHT + 1) - py;
#endif
        if (px < 1)
            px = 1;
        if (px > SCREEN_WIDTH)
            px = SCREEN_WIDTH;
        if (py < 1)
            py = 1;
        if (py > SCREEN_HEIGHT)
            py = SCREEN_HEIGHT;
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = px;
        data->point.y = py;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
#endif
}

void setup()
{
    Serial.begin(115200);
    delay(50);
    
    tft.init();
    tft.setRotation(0);
    backlight_init();
    lv_init();
    // Read real time, so blocking work (JPEG decode, HTTP) doesn't slow LVGL's clock
    lv_tick_set_cb([]() -> uint32_t { return millis(); });
    
    FileSystem::init();
    
#ifdef KIDDO_3248S035C
    gt911_init();
#else
    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(TOUCH_ROTATION);
    pinMode(XPT2046_IRQ, INPUT_PULLUP);
#endif
    lv_display_t *disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_default(disp);
    devtools::init(disp);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev, disp);
    lv_indev_set_read_cb(indev, touchscreen_read);

    audio::init();
    led_effects::init();
    bool prefs_ok = prefs.begin(PNS, false);
    brightness = prefs.getUChar(PK_BRIGHTNESS, 200);
    audio::set_volume(prefs.getUChar(PK_VOLUME, DEFAULT_VOLUME));
    current_language = (Language)prefs.getUInt(PK_LANG, LANG_EN);
    story_font_scale = prefs.getUChar(PK_STORY_FONT, 1);
    online_mode = prefs.getBool(PK_ONLINE_MODE, false);
    Serial.printf("[BOOT] Settings %s: language %u, online %d, brightness %u\n",
                  prefs_ok ? "loaded" : "UNAVAILABLE", (unsigned)current_language, online_mode, brightness);
    display_comfort::init(brightness,
                          prefs.getBool(PK_AUTO_BRIGHTNESS, false),
                          prefs.getBool(PK_BEDTIME, false));

    ui_router::show_splash(S()->loading);
    for (int i = 0; i < 10; i++) {
        lv_timer_handler();
        delay(10);
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(); 
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 6) {
        delay(500);
        attempts++;
        for (int i = 0; i < 10; i++) {
            lv_timer_handler();
            delay(10);
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
    } else {
        wifi_connected = false;
        WiFi.mode(WIFI_OFF);
    }

    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(10);
    }
    
    AsyncManager::init();
    
    InventoryManager::initialize();
    
    story::loadFromFS();
    // Load the catalog while the reader is still on the home screen
    if (wifi_connected)
        ui_library_prefetch_catalog();
    ui_story_set_home_cb([]() { ui_router::show_home(); });
    ui_router::show_home();
    Serial.printf("[BOOT] Home shown after %lu ms\n", millis());
}

void loop()
{
    lv_timer_handler();
    
    AsyncManager::process();
    led_effects::update();
    display_comfort::update();
    devtools::poll();
    
    delay(5);
}
