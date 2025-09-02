#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include <ArduinoJson.h>

namespace FileSystem {

bool init();

// File operations using LVGL fs
bool exists(const String& path);
String readFile(const String& path);
bool writeFile(const String& path, const String& content);
bool deleteFile(const String& path);

// Story-specific operations
bool saveStory(const String& filename, const String& content);
String loadStory(const String& filename);
bool deleteStory(const String& filename);

// Image cache operations
String getCachedImagePath(const String& url);
bool isImageCached(const String& url);
bool cacheImage(const String& url, const uint8_t* data, size_t size);

// Index management for stories
bool loadIndex(JsonDocument& doc);
bool saveIndex(const JsonDocument& doc);
bool indexContains(const String& file);
bool addToIndex(const String& file, const String& name, const String& lang);
bool removeFromIndex(const String& file);

// HTTP operations
bool httpGet(const String& url, String& response);
bool downloadFile(const String& url, const String& localPath);

// Cache management
void clearCache();
void clearStories();
void clearAll();

// Utility functions
std::vector<String> listFiles(const String& directory = "/");
size_t getFreeSpace();
size_t getTotalSpace();

}
