#include "image_loader.h"
#include "file_system.h"
#include <SPIFFS.h>
#include <JPEGDecoder.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

namespace ImageLoader {

static QueueHandle_t load_queue = nullptr;
static QueueHandle_t result_queue = nullptr;
static TaskHandle_t loader_task = nullptr;

struct LoadResult {
    lv_obj_t* img_obj;
    bool success;
    char cached_path[256];
    char error_message[64];
    
    LoadResult() : img_obj(nullptr), success(false) {
        cached_path[0] = '\0';
        error_message[0] = '\0';
    }
};

static void img_delete_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_DELETE) {
        lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
        uint16_t* buf = (uint16_t*)lv_obj_get_user_data(obj);
        if (buf) {
            Serial.printf("[IMAGE_LOADER] Auto-cleanup: Freeing buffer %p\n", buf);
            free(buf);
            lv_obj_set_user_data(obj, nullptr);
        }
    }
}

static void create_loading_placeholder(lv_obj_t* img_obj) {
    lv_obj_t* placeholder = lv_obj_create(lv_obj_get_parent(img_obj));
    lv_obj_set_size(placeholder, 220, 140);
    lv_obj_set_style_bg_color(placeholder, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_bg_opa(placeholder, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(placeholder, 2, 0);
    lv_obj_set_style_border_color(placeholder, lv_color_hex(0x1976D2), 0);
    lv_obj_set_style_border_opa(placeholder, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(placeholder, 8, 0);
    lv_obj_set_style_pad_all(placeholder, 8, 0);
    lv_obj_set_style_pad_bottom(placeholder, 12, 0);
    lv_obj_set_style_pad_top(placeholder, 6, 0);
    
    lv_obj_t* label = lv_label_create(placeholder);
    lv_label_set_text(label, "Loading\nImage...");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
    
    lv_obj_set_user_data(placeholder, (void*)"loading_placeholder");
}

static bool load_jpeg_with_decoder(const String& filepath, lv_obj_t* img_obj) {
    uint16_t* old_buf = (uint16_t*)lv_obj_get_user_data(img_obj);
    if (old_buf) {
        free(old_buf);
        lv_obj_set_user_data(img_obj, nullptr);
    }
    
    lv_obj_clean(img_obj);
    
    File jpegFile = SPIFFS.open(filepath, "r");
    if (!jpegFile) {
        Serial.println("[IMAGE_LOADER] ERROR: Could not open file: " + filepath);
        return false;
    }
    
    size_t fileSize = jpegFile.size();
    Serial.printf("[IMAGE_LOADER] File size: %d bytes\n", fileSize);
    jpegFile.close();
    
    size_t freeHeap = ESP.getFreeHeap();
    
    if (freeHeap < 80000) {
        Serial.printf("[IMAGE_LOADER] ERROR: Insufficient heap memory: %d bytes\n", freeHeap);
        return false;
    }

    if (!JpegDec.decodeFsFile(filepath)) {
        Serial.println("[IMAGE_LOADER] JPEGDecoder: decode failed");
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
        Serial.println("[IMAGE_LOADER] ERROR: Failed to allocate image buffer");
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
                
                // Simple pixel placement
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
        Serial.println("[IMAGE_LOADER] ERROR: Failed to create canvas");
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

static void loader_task_func(void* parameter) {
    LoadRequest request;
    
    while (true) {
        if (xQueueReceive(load_queue, &request, portMAX_DELAY) == pdTRUE) {
            LoadResult result;
            result.img_obj = request.img_obj;
            result.success = false;
            
            String url_str = String(request.url);
            
            // Check if image is already cached
            if (FileSystem::isImageCached(url_str)) {
                String cached_path = FileSystem::getCachedImagePath(url_str);
                strncpy(result.cached_path, cached_path.c_str(), sizeof(result.cached_path) - 1);
                result.cached_path[sizeof(result.cached_path) - 1] = '\0';
                result.success = true;
            } else {
                // Download and cache the image
                String cached_path = FileSystem::getCachedImagePath(url_str);
                if (FileSystem::downloadFile(url_str, cached_path)) {
                    strncpy(result.cached_path, cached_path.c_str(), sizeof(result.cached_path) - 1);
                    result.cached_path[sizeof(result.cached_path) - 1] = '\0';
                    result.success = true;
                } else {
                    strncpy(result.error_message, "Download Failed", sizeof(result.error_message) - 1);
                    result.error_message[sizeof(result.error_message) - 1] = '\0';
                }
            }
            
            xQueueSend(result_queue, &result, 0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void init() {
    if (load_queue == nullptr) {
        load_queue = xQueueCreate(10, sizeof(LoadRequest));
        result_queue = xQueueCreate(10, sizeof(LoadResult));
        
        xTaskCreate(
            loader_task_func,
            "ImageLoader",
            8192,
            nullptr,
            1,
            &loader_task
        );
    }
}

void loadImageAsync(const String& url, lv_obj_t* img_obj, bool show_loading_placeholder) {
    if (!load_queue || !img_obj) return;
    
    if (show_loading_placeholder) {
        create_loading_placeholder(img_obj);
    }
    
    LoadRequest request(url, img_obj, show_loading_placeholder);
    if (xQueueSend(load_queue, &request, 0) != pdTRUE) {
        Serial.println("[IMAGE_LOADER] WARNING: Load queue is full, request dropped");
    }
}

void process() {
    if (!result_queue) return;
    
    LoadResult result;
    while (xQueueReceive(result_queue, &result, 0) == pdTRUE) {
        lv_obj_t* parent = lv_obj_get_parent(result.img_obj);
        lv_obj_t* placeholder = nullptr;
        
        uint32_t child_count = lv_obj_get_child_cnt(parent);
        for (uint32_t i = 0; i < child_count; i++) {
            lv_obj_t* child = lv_obj_get_child(parent, i);
            if (lv_obj_get_user_data(child) && 
                strcmp((char*)lv_obj_get_user_data(child), "loading_placeholder") == 0) {
                placeholder = child;
                break;
            }
        }
        
        if (result.success && strlen(result.cached_path) > 0) {
            String cached_path_str = String(result.cached_path);
            if (load_jpeg_with_decoder(cached_path_str, result.img_obj)) {
                if (placeholder) {
                    lv_obj_del(placeholder);
                }
            } else {
                lv_obj_set_style_bg_color(result.img_obj, lv_color_hex(0xFF6600), 0);
                lv_obj_set_style_bg_opa(result.img_obj, LV_OPA_COVER, 0);
                
                if (placeholder) {
                    lv_obj_del(placeholder);
                }
                
                lv_obj_t* error_label = lv_label_create(result.img_obj);
                lv_label_set_text(error_label, "Decode\nFailed");
                lv_obj_set_style_text_color(error_label, lv_color_white(), 0);
                lv_obj_center(error_label);
            }
        } else {
            if (placeholder) {
                lv_obj_set_style_bg_color(placeholder, lv_color_hex(0xFF0000), 0);
                lv_obj_t* label = lv_obj_get_child(placeholder, 0);
                if (label) {
                    if (strlen(result.error_message) > 0) {
                        lv_label_set_text(label, result.error_message);
                    } else {
                        lv_label_set_text(label, "Load\nFailed");
                    }
                }
            }
        }
    }
}

int getPendingCount() {
    if (!load_queue) return 0;
    return uxQueueMessagesWaiting(load_queue);
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
