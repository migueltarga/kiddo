#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

namespace story_utils 
{
    // Language filtering
    bool matchesCurrentLanguage(const String& lang);
    
    // Index.json management
    bool loadIndex(JsonDocument& doc);
    bool saveIndex(const JsonDocument& doc);
    bool indexContains(const String& file);
    bool addToIndex(const String& file, const String& name = "", const String& lang = "");
    bool removeFromIndex(const String& file);
    std::vector<String> getIndexedFiles();
    
    // Network utilities
    bool isWiFiConnected();
    
    // JSON utilities
    bool parseJsonSafely(const String& json, JsonDocument& doc);
    bool hasStoriesArray(const JsonDocument& doc);
    JsonArray getStoriesArray(JsonDocument& doc, bool createIfMissing = false);
}