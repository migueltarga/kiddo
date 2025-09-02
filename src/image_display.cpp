#include "image_display.h"
#include <SPIFFS.h>
#include <JPEGDecoder.h>

namespace ImageDisplay {

static void img_delete_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_DELETE) {
        lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
        uint16_t* buf = (uint16_t*)lv_obj_get_user_data(obj);
        if (buf) {
            free(buf);
            lv_obj_set_user_data(obj, nullptr);
        }
    }
}

void createLoadingPlaceholder(lv_obj_t* img_obj) {
    lv_obj_t* placeholder = lv_obj_create(lv_obj_get_parent(img_obj));
    lv_obj_set_size(placeholder, 220, 140);
    lv_obj_set_style_bg_opa(placeholder, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(placeholder, 0, 0);
    lv_obj_set_style_pad_all(placeholder, 0, 0);
    
    lv_obj_t* spinner = lv_spinner_create(placeholder);
    lv_obj_set_size(spinner, 32, 32);
    lv_obj_center(spinner);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
    
    lv_obj_set_user_data(placeholder, (void*)"loading_placeholder");
}

bool displayJpegFromFile(const String& filepath, lv_obj_t* img_obj) {
    uint16_t* old_buf = (uint16_t*)lv_obj_get_user_data(img_obj);
    if (old_buf) {
        free(old_buf);
        lv_obj_set_user_data(img_obj, nullptr);
    }
    
    lv_obj_clean(img_obj);
    
    lv_fs_file_t file;
    String lvgl_path = "S:" + filepath;
    if (lv_fs_open(&file, lvgl_path.c_str(), LV_FS_MODE_RD) != LV_FS_RES_OK) {
        Serial.println("[IMAGE_DISPLAY] ERROR: Could not open file via LVGL FS: " + filepath);
        return false;
    }
    
    uint32_t fileSize;
    lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    lv_fs_tell(&file, &fileSize);
    lv_fs_close(&file);
    
    size_t freeHeap = ESP.getFreeHeap();
    
    if (freeHeap < 80000) {
        Serial.printf("[IMAGE_DISPLAY] ERROR: Insufficient heap memory: %d bytes\n", freeHeap);
        return false;
    }

    // Use the SPIFFS file directly for JPEGDecoder (it doesn't support LVGL FS)
    if (!JpegDec.decodeFsFile(filepath)) {
        Serial.println("[IMAGE_DISPLAY] JPEGDecoder: decode failed");
        return false;
    }
    
    float scale_x = 220.0f / JpegDec.width;
    float scale_y = 140.0f / JpegDec.height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    if (scale > 1.0f) {
        scale = 1.0f;
    }
    
    int scaled_width = (int)(JpegDec.width * scale);
    int scaled_height = (int)(JpegDec.height * scale);
    
    size_t img_size = scaled_width * scaled_height * 2;
    uint16_t* img_buf = (uint16_t*)malloc(img_size);
    if (!img_buf) {
        Serial.println("[IMAGE_DISPLAY] ERROR: Failed to allocate image buffer");
        JpegDec.abort();
        return false;
    }
    
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
    
    lv_obj_t* canvas = lv_canvas_create(img_obj);
    if (!canvas) {
        Serial.println("[IMAGE_DISPLAY] ERROR: Failed to create canvas");
        free(img_buf);
        return false;
    }
    
    lv_canvas_set_buffer(canvas, img_buf, scaled_width, scaled_height, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_size(canvas, scaled_width, scaled_height);
    lv_obj_center(canvas);
    
    lv_obj_set_user_data(img_obj, img_buf);
    lv_obj_add_event_cb(img_obj, img_delete_event_cb, LV_EVENT_DELETE, nullptr);
    
    return true;
}

void cleanupImageResources(lv_obj_t* img_obj) {
    if (!img_obj) return;
    
    uint16_t* img_buf = (uint16_t*)lv_obj_get_user_data(img_obj);
    if (img_buf) {
        free(img_buf);
        lv_obj_set_user_data(img_obj, nullptr);
    }
}

}
