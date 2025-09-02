#include "story_utils.h"
#include "file_system.h"
#include "i18n.h"
#include <WiFi.h>
#include <ArduinoJson.h>

extern Language current_language;

namespace story_utils 
{
    bool matchesLanguage(Language currentLang, const String& lang) 
    {
        return (currentLang == LANG_PT && lang == "pt-br") || 
               (currentLang == LANG_EN && lang == "en") || 
               currentLang == LANG_EN;
    }
    
    bool shouldShowContent(const String& contentLang)
    {
        // Show content if it matches current language OR if current language is English (fallback)
        return story_utils::matchesLanguage(current_language, contentLang) || current_language == LANG_EN;
    }
    
    String currentLanguageToString()
    {
        if (current_language == LANG_PT) return "pt-br";
        if (current_language == LANG_EN) return "en";
        return "en"; // Default fallback
    }
    
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