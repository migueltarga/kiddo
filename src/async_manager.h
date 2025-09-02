#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <functional>

namespace AsyncManager {

typedef std::function<void(bool success, const String& cachedPath)> ImageCallback;
typedef std::function<void(bool success, const String& storyId)> StoryCallback;
typedef std::function<void(bool success)> CatalogCallback;

void init();

void loadImage(const String& url, lv_obj_t* imgWidget, ImageCallback callback);

void downloadStory(const String& filename, StoryCallback callback);

void fetchCatalog(CatalogCallback callback);

void process();

}