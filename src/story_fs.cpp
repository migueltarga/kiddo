/**
 * @file story_fs.cpp
 * @brief Story filesystem operations
 *
 * This module handles loading stories from the filesystem,
 * managing the story index, and coordinating with the
 * story engine for story discovery and loading.
 */

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

    /**
     * @brief Read the summary fields of a story in the current language
     *
     * Deserializes straight from the file with a filter, so the node text
     * is never held in memory.
     *
     * @param path SPIFFS path of the story file
     * @param out Summary to fill
     * @return True if the file is a story in the current language
     */
    static bool readSummary(const String &path, Story_t &out)
    {
        File file = SPIFFS.open(path, "r");
        if (!file) {
            return false;
        }

        JsonDocument filter;
        filter["id"] = true;
        filter["title"] = true;
        filter["start"] = true;
        filter["lang"] = true;
        filter["has_inventory"] = true;

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, file, DeserializationOption::Filter(filter));
        file.close();
        if (err) {
            return false;
        }

        if (!story_utils::matchesLanguage(current_language, doc["lang"] | "")) {
            return false;
        }

        out.id = doc["id"] | "";
        out.title = doc["title"] | "";
        out.start = doc["start"] | "";
        out.has_inventory = doc["has_inventory"] | false;
        out.file = path;
        return out.id.length() > 0 && out.start.length() > 0;
    }

    /**
     * @brief Load all stories from filesystem
     * Scans the stories directory and loads all indexed story files
     */
    void loadFromFS()
    {
        g_stories.clear();

        std::vector<String> indexedFiles;
        JsonDocument indexDoc;
        if (FileSystem::loadIndex(indexDoc)) {
            JsonArray stories = indexDoc["stories"].as<JsonArray>();
            for (JsonObject story : stories) {
                const char* f = story["file"] | "";
                if (*f) {
                    indexedFiles.push_back(String(f));
                }
            }
        }

        for (const String& file : indexedFiles) {
            Story_t st;
            if (readSummary(file, st)) {
                g_stories.push_back(st);
            }
        }

        // Also pick up story files that are on disk but missing from the index
        File root = SPIFFS.open("/");
        if (root && root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                if (!file.isDirectory()) {
                    String filepath = file.path();
                    String normalizedPath = filepath.startsWith("/") ? filepath : ("/" + filepath);
                    if (normalizedPath.endsWith(".json") && normalizedPath != "/index.json") {
                        bool alreadyLoaded = false;
                        for (const String& indexedFile : indexedFiles) {
                            if (indexedFile == normalizedPath) {
                                alreadyLoaded = true;
                                break;
                            }
                        }

                        Story_t st;
                        if (!alreadyLoaded && readSummary(normalizedPath, st)) {
                            g_stories.push_back(st);
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
