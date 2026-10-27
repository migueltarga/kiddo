/**
 * @file story_progress.cpp
 * @brief Saved reading position for each story
 *
 * Positions live in their own NVS namespace, one key per story. NVS keys are
 * limited to 15 characters, so the key is a hash of the story ID and the
 * value repeats the full ID to rule out collisions.
 */

#include "story_progress.h"
#include <Preferences.h>
#include <ArduinoJson.h>

namespace story_progress
{

/** @brief NVS namespace for saved positions */
static const char *NAMESPACE = "progress";

/**
 * @brief Build the NVS key for a story
 * @param story_id Story ID
 * @return Short key derived from the ID
 */
static String keyFor(const String &story_id)
{
    uint32_t hash = 5381;
    for (size_t i = 0; i < story_id.length(); i++)
        hash = hash * 33 + (uint8_t)story_id[i];
    return "s" + String(hash, HEX);
}

bool load(const String &story_id, Saved &out)
{
    Preferences prefs;
    // Read-write: read-only fails (and logs) until the namespace exists
    if (!prefs.begin(NAMESPACE, false))
        return false;
    String key = keyFor(story_id);
    String value = prefs.isKey(key.c_str()) ? prefs.getString(key.c_str()) : "";
    prefs.end();
    if (value.length() == 0)
        return false;

    JsonDocument doc;
    if (deserializeJson(doc, value) || story_id != (doc["id"] | ""))
        return false;

    out.node = doc["node"] | "";
    out.items.clear();
    for (const char *item : doc["items"].as<JsonArrayConst>())
        out.items.push_back(item);
    return out.node.length() > 0;
}

void save(const String &story_id, const String &node, const InventoryManager::Inventory &inventory)
{
    JsonDocument doc;
    doc["id"] = story_id;
    doc["node"] = node;
    JsonArray items = doc["items"].to<JsonArray>();
    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        const InventoryItem_t &item = inventory.getItem(i);
        if (!item.isEmpty())
            items.add(item.id);
    }
    String value;
    serializeJson(doc, value);

    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false))
        return;
    String key = keyFor(story_id);
    // Skip identical writes to spare the flash
    if (!prefs.isKey(key.c_str()) || prefs.getString(key.c_str()) != value)
        prefs.putString(key.c_str(), value);
    prefs.end();
}

void clear(const String &story_id)
{
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false))
        return;
    String key = keyFor(story_id);
    if (prefs.isKey(key.c_str()))
        prefs.remove(key.c_str());
    prefs.end();
}

void clearAll()
{
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false))
        return;
    prefs.clear();
    prefs.end();
}

}
