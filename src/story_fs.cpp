#include "story_engine.h"
#include "file_system.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "i18n.h"
#include "story_utils.h"
#include <Arduino.h>

namespace story
{
    extern std::vector<Story_t> g_stories;
    void loadFromFS()
    {
        g_stories.clear();
        
        std::vector<String> indexedFiles;
        JsonDocument indexDoc;
        if (FileSystem::loadIndex(indexDoc)) {
            JsonArray stories = indexDoc["stories"].as<JsonArray>();
            for (JsonObject story : stories) {
                const char* f = story["file"] | "";
                if (*f) indexedFiles.push_back(String(f));
            }
        }
        
        if (!indexedFiles.empty()) {
            for (const String& file : indexedFiles) {
                if (!FileSystem::exists(file)) continue;
                
                String payload = FileSystem::readFile(file);
                if (payload.length() == 0) continue;
                
                JsonDocument storyDoc;
                if (deserializeJson(storyDoc, payload) != DeserializationError::Ok) continue;
                
                String lang = storyDoc["lang"].as<String>();
                if (!story_utils::matchesLanguage(current_language, lang)) continue;
                
                Story_t st;
                bool ok = parseStoryJson(payload, st);
                if (ok) {
                    g_stories.push_back(st);
                }
            }
        }
        
        File root = SPIFFS.open("/");
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                if (!file.isDirectory()) {
                    String filename = file.name();
                    String filepath = file.path();
                    if (filename.endsWith(".json") && filename != "/index.json" && filepath != "/index.json") {
                        String normalizedFilename = filename.startsWith("/") ? filename : ("/" + filename);
                        String normalizedPath = filepath.startsWith("/") ? filepath : ("/" + filepath);
                        
                        bool alreadyLoaded = false;
                        for (const String& indexedFile : indexedFiles) {
                            if (indexedFile == normalizedFilename || indexedFile == normalizedPath) {
                                alreadyLoaded = true;
                                break;
                            }
                        }
                        
                        if (!alreadyLoaded) {
                            String content = FileSystem::readFile(file.path());
                            
                            JsonDocument storyDoc;
                            if (deserializeJson(storyDoc, content) == DeserializationError::Ok) {
                                String lang = storyDoc["lang"].as<String>();
                                if (story_utils::matchesLanguage(current_language, lang)) {
                                    Story_t story;
                                    if (parseStoryJson(content, story)) {
                                        g_stories.push_back(story);
                                    }
                                }
                            }
                        }
                    }
                }
                file.close();
                file = root.openNextFile();
            }
            root.close();
        }
    }
}
