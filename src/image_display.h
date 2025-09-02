#pragma once

#include <Arduino.h>
#include <lvgl.h>

namespace ImageDisplay {

bool displayJpegFromFile(const String& filepath, lv_obj_t* img_obj);

void createLoadingPlaceholder(lv_obj_t* img_obj);

void cleanupImageResources(lv_obj_t* img_obj);

}
