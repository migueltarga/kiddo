/**
 * @file remote_catalog.cpp
 * @brief Remote story catalog management
 *
 * This module handles fetching and managing remote story catalogs,
 * including downloading stories, reconciling with local content,
 * and managing catalog URLs.
 */

#include "remote_catalog.h"
#include "config.h"
#include "file_system.h"
#include "story_engine.h"
#include <ArduinoJson.h>
#include "i18n.h"
#include "story_utils.h"
#include <Preferences.h>
#include <WiFi.h>

extern Preferences prefs;

namespace remote_catalog
{
    /** @brief Cached catalog entries */
    static std::vector<Entry> g_entries;
    /** @brief Last fetch timestamp */
    static uint32_t g_last_fetch_ms = 0;
    /** @brief Language of last fetch */
    static Language g_last_fetch_lang;
    /** @brief Last fetch success flag */
    static bool g_last_ok = false;

    /**
     * @brief Check if index contains a specific file
     * @param doc JsonDocument containing index
     * @param file Filename to check
     * @return True if file is in index
     */
    static bool indexContainsFile(const JsonDocument& doc, const String& file) {
        if (!doc["stories"].is<JsonArray>()) return false;

        JsonArrayConst stories = doc["stories"];
        for (JsonObjectConst story : stories) {
            const char* f = story["file"] | "";
            if (file == f) return true;
        }
        return false;
    }

    /**
     * @brief Get the current catalog URL
     * @return Current catalog URL string
     */
    String getCatalogUrl()
    {
        String url = prefs.getString(PK_CATALOG_URL, REMOTE_CATALOG_URL);
        if (url.length() == 0) {
            url = REMOTE_CATALOG_URL;
        }
        return url;
    }
    
    void setCatalogUrl(const String& url)
    {
        prefs.putString(PK_CATALOG_URL, url);
        invalidate();
    }
    
    static String basePathFromCatalog()
    {
        String url = getCatalogUrl();
        int slash = url.lastIndexOf('/');
        if (slash > 0)
            return url.substring(0, slash + 1);
        return url;
    }

    bool fetch()
    {
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }
        
        if (current_language == g_last_fetch_lang && 
            millis() - g_last_fetch_ms < 30000 && 
            !g_entries.empty()) {
            g_last_ok = true;
            return true;
        }
        
        String url = getCatalogUrl();
        String payload;
        if (!FileSystem::httpGet(url, payload)) {
            return false;
        }
        
        JsonDocument doc;
        if (deserializeJson(doc, payload) != DeserializationError::Ok) {
            g_last_ok = false;
            return false;
        }
        
        JsonArray stories = doc["stories"].as<JsonArray>();
        if (stories.size() == 0) {
            g_last_ok = false;
            return false;
        }
        
        g_entries.clear();
        int total = 0, filtered = 0;
        
        for (JsonObject o : stories) {
            const char *f = o["file"] | "";
            if (!*f) continue;
            
            const char *n = o["name"] | "";
            const char *lang = o["lang"] | "";
            
            Entry e;
            e.file = f;
            e.name = n;
            e.lang = lang;
            
            if (e.name.length() == 0) {
                e.name = e.file;
            }
            
            ++total;
            
            if (story_utils::shouldShowContent(e.lang)) {
                g_entries.push_back(e);
                ++filtered;
            }
        }
        
        g_last_fetch_ms = millis();
        g_last_fetch_lang = current_language;
        g_last_ok = true;
        return true;
    }

    const std::vector<Entry> &entries() { return g_entries; }
    bool last_ok() { return g_last_ok; }
    void invalidate()
    {
        g_entries.clear();
        g_last_fetch_ms = 0;
        g_last_ok = false;
    }

    bool ensureDownloadedOrIndexed(const String &file, String *outStoryId)
    {
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }
        
        // Find the entry for this file to get name and lang
        Entry foundEntry;
        bool entryFound = false;
        for (const auto& entry : g_entries) {
            if (entry.file == file) {
                foundEntry = entry;
                entryFound = true;
                break;
            }
        }
        
        String localPath = "/" + file;
        
        // Check if file exists AND has content
        bool fileExistsWithContent = false;
        if (FileSystem::exists(localPath)) {
            String existingContent = FileSystem::readFile(localPath);
            if (existingContent.length() > 0) {
                fileExistsWithContent = true;
            }
        }
        
        if (fileExistsWithContent) {
            JsonDocument doc;
            if (!FileSystem::loadIndex(doc) || !indexContainsFile(doc, localPath)) {
                String name = entryFound ? foundEntry.name : "";
                String lang = entryFound ? foundEntry.lang : "";
                FileSystem::addToIndex(localPath, name, lang);
            }
            story::loadFromFS();
            return true;
        }
        
        String base = basePathFromCatalog();
        String url = base + file;
        
        String payload;
        if (!FileSystem::httpGet(url, payload)) {
            return false;
        }
        
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            if (!doc["lang"].is<const char*>() || strlen(doc["lang"].as<const char*>()) == 0) {
                String lang = story_utils::currentLanguageToString();
                doc["lang"] = lang;
                String outPayload;
                serializeJson(doc, outPayload);
                payload = outPayload;
            }
        }
        
        if (!FileSystem::writeFile(localPath, payload)) {
            return false;
        }
        
        if (outStoryId) {
            JsonDocument doc;
            if (deserializeJson(doc, payload) == DeserializationError::Ok) {
                *outStoryId = doc["id"].as<const char *>();
            }
        }
        
        String name = entryFound ? foundEntry.name : "";
        String lang = entryFound ? foundEntry.lang : "";
        if (!FileSystem::addToIndex(localPath, name, lang)) {
            return false;
        }
        
        story::loadFromFS();
        return true;
    }

    int reconcileExisting()
    {
        int added = 0;
        bool any = false;
        
        for (const auto &ent : g_entries) {
            String localPath = "/" + ent.file;
            if (FileSystem::exists(localPath) && !FileSystem::indexContains(localPath)) {
                if (FileSystem::addToIndex(localPath, ent.name, ent.lang)) {
                    ++added;
                    any = true;
                }
            }
        }
        
        if (any) {
            story::loadFromFS();
        }
        
        return added;
    }

    int clearDownloads()
    {
        FileSystem::clearStories();
        story::loadFromFS();
        return 1; // Return success indicator
    }
}
