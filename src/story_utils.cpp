/**
 * @file story_utils.cpp
 * @brief Utility functions for story management and language handling
 *
 * This module provides utility functions for story management including
 * language matching, content filtering, and index file operations.
 */

#include "story_utils.h"
#include "file_system.h"
#include "i18n.h"
#include <WiFi.h>
#include <ArduinoJson.h>

extern Language current_language;

namespace story_utils
{

/**
 * @brief Check if language matches current language setting
 * @param currentLang Current language setting
 * @param lang Language string to check
 * @return True if languages match
 */
bool matchesLanguage(Language currentLang, const String& lang)
{
    return (currentLang == LANG_PT && lang == "pt-br") ||
           (currentLang == LANG_EN && lang == "en");
}

/**
 * @brief Check if content should be shown based on current language
 * @param contentLang Language of the content
 * @return True if content should be shown
 */
bool shouldShowContent(const String& contentLang)
{
    // Show content if it matches current language (includes English fallback)
    return story_utils::matchesLanguage(current_language, contentLang);
}

/**
 * @brief Convert current language to language string
 * @return Current language as string
 */
String currentLanguageToString()
{
    if (current_language == LANG_PT) return "pt-br";
    if (current_language == LANG_EN) return "en";
    return "en"; // Default fallback
}

/**
 * @brief Load story index from file
 * @param doc JsonDocument to fill with index data
 * @return True if loading was successful
 */
bool loadIndex(JsonDocument& doc) 
    {
        return FileSystem::loadIndex(doc);
    }
    
    bool saveIndex(const JsonDocument& doc) 
    {
        return FileSystem::saveIndex(doc);
    }
    
    bool indexContains(const String& file) 
    {
        return FileSystem::indexContains(file);
    }
    
    bool addToIndex(const String& file, const String& name, const String& lang) 
    {
        return FileSystem::addToIndex(file, name, lang);
    }
    
    bool removeFromIndex(const String& file) 
    {
        return FileSystem::removeFromIndex(file);
    }
    
    std::vector<String> getIndexedFiles() 
    {
        std::vector<String> files;
        JsonDocument doc;
        if (!loadIndex(doc)) return files;
        
        JsonArray stories = doc["stories"].as<JsonArray>();
        for (JsonObject story : stories) {
            const char* f = story["file"] | "";
            if (*f) files.push_back(String(f));
        }
        return files;
    }
    
    bool isWiFiConnected() 
    {
        bool connected = WiFi.status() == WL_CONNECTED;
        return connected;
    }
    
    bool parseJsonSafely(const String& json, JsonDocument& doc) 
    {
        return deserializeJson(doc, json) == DeserializationError::Ok;
    }
    
    bool hasStoriesArray(const JsonDocument& doc) 
    {
        return doc["stories"].is<JsonArray>();
    }
    
    JsonArray getStoriesArray(JsonDocument& doc, bool createIfMissing) 
    {
        if (!hasStoriesArray(doc) && createIfMissing) {
            doc["stories"].to<JsonArray>();
        }
        return doc["stories"].as<JsonArray>();
    }
    
    String getCachedImagePath(const String& url) 
    {
        return FileSystem::getCachedImagePath(url);
    }
    
    bool isImageCached(const String& url) 
    {
        return FileSystem::isImageCached(url);
    }
    
    bool downloadImage(const String& url, const String& localPath) 
    {
        return FileSystem::downloadFile(url, localPath);
    }
    
    void cleanImageCache() 
    {
        FileSystem::clearCache();
    }
}