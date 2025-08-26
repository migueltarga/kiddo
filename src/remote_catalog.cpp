#include "remote_catalog.h"
#include "config.h"
#include "net.h"
#include "storage.h"
#include "story_engine.h"
#include "story_utils.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include "i18n.h"
#include <Preferences.h>

extern Preferences prefs;

namespace remote_catalog
{
    static std::vector<Entry> g_entries;
    static uint32_t g_last_fetch_ms = 0;
    static Language g_last_fetch_lang;
    static bool g_last_ok = false;
    
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
        if (!story_utils::isWiFiConnected()) {
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
        if (!net::httpGet(url, payload)) {
            return false;
        }
        
        JsonDocument doc;
        if (!story_utils::parseJsonSafely(payload, doc)) {
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
            
            if (story_utils::matchesCurrentLanguage(e.lang)) {
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
        if (!story_utils::isWiFiConnected()) {
            return false;
        }
        
        String localPath = "/" + file;
        
        if (SPIFFS.exists(localPath)) {
            if (!story_utils::indexContains(localPath)) {
                story_utils::addToIndex(localPath);
            }
            story::loadFromFS();
            return true;
        }
        
        String base = basePathFromCatalog();
        String url = base + file;
        
        String payload;
        if (!net::httpGet(url, payload)) {
            return false;
        }
        
        JsonDocument doc;
        if (story_utils::parseJsonSafely(payload, doc)) {
            if (!doc["lang"].is<const char*>() || strlen(doc["lang"].as<const char*>()) == 0) {
                String lang;
                if (current_language == LANG_PT) lang = "pt-br";
                else if (current_language == LANG_EN) lang = "en";
                else lang = "en";
                doc["lang"] = lang;
                String outPayload;
                serializeJson(doc, outPayload);
                payload = outPayload;
            }
        }
        
        if (!storage::writeStringToFile(localPath.c_str(), payload)) {
            return false;
        }
        
        if (outStoryId) {
            JsonDocument doc;
            if (story_utils::parseJsonSafely(payload, doc)) {
                *outStoryId = doc["id"].as<const char *>();
            }
        }
        
        if (!story_utils::addToIndex(localPath)) {
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
            if (SPIFFS.exists(localPath) && !story_utils::indexContains(localPath)) {
                if (story_utils::addToIndex(localPath, ent.name, ent.lang)) {
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
        std::vector<String> paths;
        paths.reserve(g_entries.size());
        for (const auto &ent : g_entries) {
            paths.push_back(String("/") + ent.file);
        }
        
        int removed = 0;
        for (const auto &path : paths) {
            if (SPIFFS.exists(path)) {
                if (SPIFFS.remove(path)) {
                    ++removed;
                }
                story_utils::removeFromIndex(path);
            }
        }
        
        story::loadFromFS();
        return removed;
    }
}
