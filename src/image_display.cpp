/**
 * @file image_display.cpp
 * @brief Image display and JPEG decoding functionality
 *
 * This module handles JPEG image decoding and display using the
 * JPEGDecoder library and LVGL image objects. It provides
 * functionality to load and display images from SPIFFS.
 */

#include "styles.h"
#include "image_display.h"
#include <SPIFFS.h>
#include <JPEGDecoder.h>
#include <esp_heap_caps.h>

namespace ImageDisplay {

/**
 * @brief Marks loading placeholders through user data
 *
 * Compared by address, so siblings whose user data holds something else
 * (such as an image buffer) are never misread as a string.
 */
static const char kPlaceholderTag[] = "loading_placeholder";

/** @brief RAM left free for the network worker when decoding (an HTTPS handshake peaks around this) */
static const size_t kHeapReserve = 24 * 1024;

/**
 * @brief Stop showing and free the picture an image object owns
 *
 * The picture is one heap block, an lv_image_dsc_t followed by its pixels,
 * kept in the object's user data. The image object shows it directly: an
 * lv_canvas would work too, but LVGL 9.6's canvas destructor hands the image
 * cache a bad pointer and logs "invalid magic" every time a page closes.
 *
 * @param img_obj Image object
 */
static void free_picture(lv_obj_t* img_obj) {
    void* block = lv_obj_get_user_data(img_obj);
    if (!block) return;
    lv_image_set_src(img_obj, nullptr);
    free(block);
    lv_obj_set_user_data(img_obj, nullptr);
}

/**
 * @brief LVGL object delete event callback
 * Frees the picture when the image object is deleted
 * @param e LVGL event
 */
static void img_delete_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_DELETE) {
        free_picture((lv_obj_t*)lv_event_get_target(e));
    }
}

/**
 * @brief Show a spinner while the image loads
 *
 * The spinner goes inside the image object, centered in the space the
 * picture will take; displaying the image clears it.
 *
 * @param img_obj Image object to create placeholder for
 */
void createLoadingPlaceholder(lv_obj_t* img_obj) {
    lv_obj_t* spinner = lv_spinner_create(img_obj);
    lv_obj_set_size(spinner, 32, 32);
    lv_obj_center(spinner);
    lv_obj_set_style_arc_color(spinner, palette::parchment(), LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, palette::primary(), LV_PART_INDICATOR);
    
    lv_obj_set_user_data(spinner, (void*)kPlaceholderTag);
}

Result displayJpegFromFile(const String& filepath, lv_obj_t* img_obj) {
    uint32_t started = millis();
    if (!JpegDec.decodeFsFile(filepath)) {
        Serial.println("[IMAGE_DISPLAY] JPEGDecoder: decode failed");
        return Result::DECODE_FAILED;
    }
    
    float scale_x = 220.0f / JpegDec.width;
    float scale_y = 140.0f / JpegDec.height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    if (scale > 1.0f) {
        scale = 1.0f;
    }
    
    int scaled_width = (int)(JpegDec.width * scale);
    int scaled_height = (int)(JpegDec.height * scale);
    
    // The background worker may be in the middle of an HTTPS download, which
    // needs a lot of RAM for a moment; leave it room and let the caller retry
    // later rather than starving the download or failing the image
    size_t img_size = scaled_width * scaled_height * 2;
    size_t block_size = sizeof(lv_image_dsc_t) + img_size;
    size_t free_heap = ESP.getFreeHeap();
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    uint8_t* block = nullptr;
    if (free_heap >= block_size + kHeapReserve && largest >= block_size) {
        block = (uint8_t*)malloc(block_size);
    }
    if (!block) {
        Serial.printf("[IMAGE_DISPLAY] Waiting for memory: need %u, free %u, largest block %u\n",
                      img_size, free_heap, largest);
        JpegDec.abort();
        return Result::NO_MEMORY;
    }
    
    // Only now replace what the widget shows (old image, spinner)
    free_picture(img_obj);
    lv_obj_clean(img_obj);
    
    lv_image_dsc_t* dsc = (lv_image_dsc_t*)block;
    uint16_t* img_buf = (uint16_t*)(block + sizeof(lv_image_dsc_t));
    memset(img_buf, 0, img_size);
    
    uint16_t* pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;
    
    while (JpegDec.read()) {
        pImg = JpegDec.pImage;
        
        for (int mcu_y = 0; mcu_y < mcu_h; mcu_y++) {
            int y = JpegDec.MCUy * mcu_h + mcu_y;
            if (y >= max_y) break;
            
            for (int mcu_x = 0; mcu_x < mcu_w; mcu_x++) {
                int x = JpegDec.MCUx * mcu_w + mcu_x;
                if (x >= max_x) break;
                
                int scaled_x = (int)((float)x * scale);
                int scaled_y = (int)((float)y * scale);
                
                if (scaled_x >= scaled_width || scaled_y >= scaled_height) continue;
                if (scaled_x < 0 || scaled_y < 0) continue;
                
                uint16_t pixel = pImg[mcu_y * mcu_w + mcu_x];
                
                int idx = scaled_y * scaled_width + scaled_x;
                if (idx >= 0 && idx < (scaled_width * scaled_height)) {
                    img_buf[idx] = pixel;
                }
            }
        }
    }
    
    JpegDec.abort();
    Serial.printf("[IMAGE_DISPLAY] Decoded %ux%u in %lu ms\n", JpegDec.width, JpegDec.height, millis() - started);
    
    memset(dsc, 0, sizeof(lv_image_dsc_t));
    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    dsc->header.w = scaled_width;
    dsc->header.h = scaled_height;
    dsc->header.stride = scaled_width * 2;
    dsc->data_size = img_size;
    dsc->data = (const uint8_t*)img_buf;
    
    // Centered in the image object's fixed area
    lv_obj_set_user_data(img_obj, block);
    lv_image_set_src(img_obj, dsc);
    // Register once even if the widget is reused for another image
    lv_obj_remove_event_cb(img_obj, img_delete_event_cb);
    lv_obj_add_event_cb(img_obj, img_delete_event_cb, LV_EVENT_DELETE, nullptr);
    
    return Result::OK;
}

void removeLoadingPlaceholder(lv_obj_t* img_obj) {
    uint32_t child_count = lv_obj_get_child_cnt(img_obj);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t* child = lv_obj_get_child(img_obj, i);
        if (lv_obj_get_user_data(child) == kPlaceholderTag) {
            lv_obj_delete(child);
            return;
        }
    }
}

void showError(lv_obj_t* img_obj, const char* message) {
    lv_obj_t* label = lv_label_create(img_obj);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_color(label, palette::ink_soft(), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label);
}

void cleanupImageResources(lv_obj_t* img_obj) {
    if (!img_obj) return;
    free_picture(img_obj);
}

}
