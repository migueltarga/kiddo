#include "story_engine.h"
#include "story_utils.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "storage.h"
#include "i18n.h"
#include <Arduino.h>

namespace story
{
    extern std::vector<Story_t> g_stories;
    void loadFromFS()
    {
        g_stories.clear();
        
        std::vector<String> indexedFiles = story_utils::getIndexedFiles();
        
        if (!indexedFiles.empty()) {
            for (const String& file : indexedFiles) {
                if (!SPIFFS.exists(file)) continue;
                
                String payload = storage::readFileToString(file.c_str());
                if (payload.length() == 0) continue;
                
                JsonDocument storyDoc;
                if (!story_utils::parseJsonSafely(payload, storyDoc)) continue;
                
                String lang = storyDoc["lang"].as<String>();
                if (!story_utils::matchesCurrentLanguage(lang)) continue;
                
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
                            String content = storage::readFileToString(file.path());
                            
                            JsonDocument storyDoc;
                            if (story_utils::parseJsonSafely(content, storyDoc)) {
                                String lang = storyDoc["lang"].as<String>();
                                if (story_utils::matchesCurrentLanguage(lang)) {
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
