#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

namespace ImageLoader {

struct LoadRequest {
    char url[256];
    lv_obj_t* img_obj;
    bool show_loading_placeholder;
    
    LoadRequest() : img_obj(nullptr), show_loading_placeholder(true) {
        url[0] = '\0';
    }
    
    LoadRequest(const String& u, lv_obj_t* obj, bool placeholder = true) 
        : img_obj(obj), show_loading_placeholder(placeholder) {
        strncpy(url, u.c_str(), sizeof(url) - 1);
        url[sizeof(url) - 1] = '\0';
    }
};

void init();

void loadImageAsync(const String& url, lv_obj_t* img_obj, bool show_loading_placeholder = true);

void process();

int getPendingCount();

void cleanupImageResources(lv_obj_t* img_obj);

}
