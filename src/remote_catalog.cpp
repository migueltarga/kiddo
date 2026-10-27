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

extern Preferences prefs;

namespace remote_catalog
{
    /** @brief Cached catalog entries for the current language (main thread only) */
    static std::vector<Entry> g_entries;
    /** @brief Last fetch timestamp */
    static uint32_t g_last_fetch_ms = 0;
    /** @brief Language of last fetch */
    static Language g_last_fetch_lang;
    /** @brief Last fetch success flag */
    static bool g_last_ok = false;

    /** @brief SPIFFS object names hold at most 31 characters, including the leading '/' */
    static const size_t MAX_FILE_NAME_LEN = 30;
    /** @brief Saved catalogs besides the default; the least recent is dropped */
    static const size_t MAX_SAVED_CATALOGS = 6;

    /**
     * @brief Read the saved catalog list, [{"u": url, "n": name}, ...]
     */
    static JsonDocument loadSaved()
    {
        JsonDocument doc;
        String json = prefs.isKey(PK_CATALOGS) ? prefs.getString(PK_CATALOGS) : "";
        if (json.length() == 0 || deserializeJson(doc, json) != DeserializationError::Ok || !doc.is<JsonArray>()) {
            doc.to<JsonArray>();
        }
        return doc;
    }

    /**
     * @brief Put a catalog first in the saved list
     * @param url Catalog URL; the default is never stored
     * @param name Name to store, or empty to keep the known one
     */
    static void remember(const String& url, const String& name)
    {
        if (url.length() == 0 || isDefaultCatalog(url)) {
            return;
        }
        JsonDocument old = loadSaved();
        String keep = name;
        for (JsonObjectConst o : old.as<JsonArrayConst>()) {
            if (keep.length() == 0 && url == (o["u"] | "")) {
                keep = o["n"] | "";
            }
        }

        JsonDocument doc;
        JsonArray list = doc.to<JsonArray>();
        JsonObject first = list.add<JsonObject>();
        first["u"] = url;
        first["n"] = keep;
        for (JsonObjectConst o : old.as<JsonArrayConst>()) {
            if (list.size() >= MAX_SAVED_CATALOGS) {
                break;
            }
            if (url != (o["u"] | "")) {
                list.add(o);
            }
        }

        String json;
        serializeJson(doc, json);
        // Every fetch remembers its name; skip the flash write when nothing changed
        if (!prefs.isKey(PK_CATALOGS) || prefs.getString(PK_CATALOGS) != json) {
            prefs.putString(PK_CATALOGS, json);
        }
    }

    bool isDefaultCatalog(const String& url)
    {
        return url == REMOTE_CATALOG_URL;
    }

    std::vector<SavedCatalog> savedCatalogs()
    {
        // A URL set before the list existed joins it here
        String current = getCatalogUrl();
        bool listed = isDefaultCatalog(current);
        JsonDocument doc = loadSaved();
        for (JsonObjectConst o : doc.as<JsonArrayConst>()) {
            listed = listed || current == (o["u"] | "");
        }
        if (!listed) {
            remember(current, "");
            doc = loadSaved();
        }

        std::vector<SavedCatalog> out;
        out.push_back({REMOTE_CATALOG_URL, ""});
        for (JsonObjectConst o : doc.as<JsonArrayConst>()) {
            out.push_back({o["u"] | "", o["n"] | ""});
        }
        return out;
    }

    String getCatalogUrl()
    {
        // isKey() first: getString() logs an error for a missing key
        String url = prefs.isKey(PK_CATALOG_URL) ? prefs.getString(PK_CATALOG_URL) : "";
        if (url.length() == 0) {
            url = REMOTE_CATALOG_URL;
        }
        return url;
    }

    void setCatalogUrl(const String& url)
    {
        prefs.putString(PK_CATALOG_URL, url);
        remember(url, "");
        invalidate();
    }

    String storyUrl(const String& file)
    {
        String url = getCatalogUrl();
        int slash = url.lastIndexOf('/');
        if (slash > 0)
            return url.substring(0, slash + 1) + file;
        return url + file;
    }

    bool isValidFileName(const String& file)
    {
        return file.length() > 0 &&
               file.length() <= MAX_FILE_NAME_LEN &&
               file.endsWith(".json") &&
               file != "index.json" &&
               file.indexOf('/') < 0 &&
               file.indexOf('\\') < 0 &&
               !file.startsWith(".");
    }

    bool fetchEntries(const String& url, Catalog& out)
    {
        String payload;
        if (!FileSystem::httpGet(url, payload)) {
            return false;
        }

        JsonDocument doc;
        if (deserializeJson(doc, payload) != DeserializationError::Ok) {
            return false;
        }

        JsonArray stories = doc["stories"].as<JsonArray>();
        if (stories.size() == 0) {
            return false;
        }

        out.name = doc["name"] | "";
        out.entries.clear();
        for (JsonObject o : stories) {
            Entry e;
            e.file = o["file"] | "";
            e.name = o["name"] | "";
            e.lang = o["lang"] | "";

            if (!isValidFileName(e.file)) {
                Serial.println("[CATALOG] Skipping invalid file name: " + e.file);
                continue;
            }
            if (e.name.length() == 0) {
                e.name = e.file;
            }
            out.entries.push_back(e);
        }
        return true;
    }

    bool isFresh()
    {
        return g_last_ok &&
               current_language == g_last_fetch_lang &&
               millis() - g_last_fetch_ms < 30000 &&
               !g_entries.empty();
    }

    void applyFetched(bool ok, const Catalog& fetched)
    {
        if (!ok) {
            g_last_ok = false;
            return;
        }

        g_entries.clear();
        for (const Entry& e : fetched.entries) {
            if (story_utils::shouldShowContent(e.lang)) {
                g_entries.push_back(e);
            }
        }

        g_last_fetch_ms = millis();
        g_last_fetch_lang = current_language;
        g_last_ok = true;

        // Learn the catalog's name for the picker in Settings
        if (fetched.name.length() > 0) {
            remember(getCatalogUrl(), fetched.name);
        }
    }

    const std::vector<Entry> &entries() { return g_entries; }
    bool last_ok() { return g_last_ok; }
    void invalidate()
    {
        g_entries.clear();
        g_last_fetch_ms = 0;
        g_last_ok = false;
    }

    bool downloadStory(const String& url, const String& localPath, const String& lang, String& outStoryId)
    {
        // Reuse a copy that is already on disk
        bool cached = FileSystem::fileSize(localPath) > 0;

        String payload;
        if (cached) {
            payload = FileSystem::readFile(localPath);
        } else if (!FileSystem::httpGet(url, payload)) {
            return false;
        }

        JsonDocument doc;
        if (deserializeJson(doc, payload) != DeserializationError::Ok) {
            Serial.println("[CATALOG] Not a valid story file: " + localPath);
            return false;
        }

        const char* id = doc["id"] | "";
        if (!*id) {
            Serial.println("[CATALOG] Story has no id: " + localPath);
            return false;
        }
        outStoryId = id;

        if (cached) {
            return true;
        }

        if (!doc["lang"].is<const char*>() || strlen(doc["lang"].as<const char*>()) == 0) {
            doc["lang"] = lang;
            payload = "";
            serializeJson(doc, payload);
        }

        return FileSystem::writeFile(localPath, payload);
    }

    bool indexDownloaded(const String& file)
    {
        String name, lang;
        for (const auto& entry : g_entries) {
            if (entry.file == file) {
                name = entry.name;
                lang = entry.lang;
                break;
            }
        }
        return FileSystem::addToIndex("/" + file, name, lang);
    }

    int reconcileExisting()
    {
        int added = 0;

        for (const auto &ent : g_entries) {
            String localPath = "/" + ent.file;
            if (FileSystem::exists(localPath) && !FileSystem::indexContains(localPath)) {
                if (FileSystem::addToIndex(localPath, ent.name, ent.lang)) {
                    ++added;
                }
            }
        }

        if (added > 0) {
            story::loadFromFS();
        }

        return added;
    }
}
