#include "story_utils.h"
#include "storage.h"
#include "i18n.h"
#include <WiFi.h>
#include <SPIFFS.h>

extern Language current_language;

namespace story_utils 
{
    bool matchesCurrentLanguage(const String& lang) 
    {
        if (current_language == LANG_PT) return lang == "pt-br";
        if (current_language == LANG_EN) return lang == "en";
        return true;
    }
    
    bool loadIndex(JsonDocument& doc) 
    {
        String idx = storage::readFileToString("/index.json");
        if (idx.length() == 0) {
            // Create empty index structure
            doc.clear();
            doc["stories"].to<JsonArray>();
            return true;
        }
        return deserializeJson(doc, idx) == DeserializationError::Ok;
    }
    
    bool saveIndex(const JsonDocument& doc) 
    {
        String output;
        serializeJson(doc, output);
        return storage::writeStringToFile("/index.json", output);
    }
    
    bool indexContains(const String& file) 
    {
        JsonDocument doc;
        if (!loadIndex(doc)) return false;
        
        JsonArray stories = doc["stories"].as<JsonArray>();
        for (JsonObject story : stories) {
            const char* f = story["file"] | "";
            if (file == f) return true;
        }
        return false;
    }
    
    bool addToIndex(const String& file, const String& name, const String& lang) 
    {
        JsonDocument doc;
        if (!loadIndex(doc)) return false;
        
        // Check if already exists
        if (indexContains(file)) return true;
        
        JsonArray stories = getStoriesArray(doc, true);
        JsonObject newStory = stories.add<JsonObject>();
        newStory["file"] = file;
        if (name.length() > 0) newStory["name"] = name;
        if (lang.length() > 0) newStory["lang"] = lang;
        
        return saveIndex(doc);
    }
    
    bool removeFromIndex(const String& file) 
    {
        JsonDocument doc;
        if (!loadIndex(doc)) return false;
        
        JsonArray stories = doc["stories"].as<JsonArray>();
        JsonDocument newDoc;
        JsonArray newStories = newDoc["stories"].to<JsonArray>();
        
        bool found = false;
        for (JsonObject story : stories) {
            const char* f = story["file"] | "";
            if (file != f) {
                JsonObject newStory = newStories.add<JsonObject>();
                newStory["file"] = f;
                if (story["name"].is<const char*>()) {
                    newStory["name"] = story["name"].as<const char*>();
                }
                if (story["lang"].is<const char*>()) {
                    newStory["lang"] = story["lang"].as<const char*>();
                }
            } else {
                found = true;
            }
        }
        
        if (!found) return false;
        return saveIndex(newDoc);
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
        return WiFi.status() == WL_CONNECTED;
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
}