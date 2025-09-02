#include "story_engine.h"
#include "models.h"
#include <ArduinoJson.h>

namespace story
{
    bool parseStoryJson(const String &json, Story_t &out)
    {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, json);
        if (err)
            return false;
        out.id = doc["id"].as<const char *>();
        out.title = doc["title"].as<const char *>();
        out.start = doc["start"].as<const char *>();
        if (out.id.length() == 0 || out.start.length() == 0)
            return false;
            
        // Parse inventory settings
        out.has_inventory = doc["has_inventory"].is<bool>() ? (bool)doc["has_inventory"] : false;
        out.initial_inventory.clear();
        
        if (out.has_inventory && doc["initial_inventory"].is<JsonArray>()) {
            for (JsonObject item : doc["initial_inventory"].as<JsonArray>()) {
                InventoryItem_t inv_item;
                inv_item.id = item["id"].as<const char *>();
                inv_item.name = item["name"].as<const char *>();
                inv_item.icon = item["icon"].is<const char *>() ? item["icon"].as<const char *>() : "";
                if (inv_item.id.length() > 0 && inv_item.name.length() > 0) {
                    out.initial_inventory.push_back(inv_item);
                }
            }
        }
        
        JsonObject nodes = doc["nodes"].as<JsonObject>();
        for (JsonPair kv : nodes)
        {
            String key = kv.key().c_str();
            JsonObject n = kv.value().as<JsonObject>();
            Node_t nn;
            nn.is_end = false;
            nn.text = normalizeText(n["text"].as<const char *>());
            nn.is_end = n["end"].is<bool>() ? (bool)n["end"] : false;
            nn.gives_item = n["gives_item"].is<const char *>() ? n["gives_item"].as<const char *>() : "";
            
            if (n["choices"].is<JsonArray>())
            {
                for (JsonObject c : n["choices"].as<JsonArray>())
                {
                    Choice_t ch;
                    ch.text = c["text"].as<const char *>();
                    ch.next = c["next"].as<const char *>();
                    ch.required_item = c["required_item"].is<const char *>() ? c["required_item"].as<const char *>() : "";
                    ch.gives_item = c["gives_item"].is<const char *>() ? c["gives_item"].as<const char *>() : "";
                    ch.hidden_without_item = c["hidden_without_item"].is<bool>() ? (bool)c["hidden_without_item"] : false;
                    if (ch.text.length())
                        nn.choices.push_back(ch);
                }
            }
            out.nodes.push_back({key, nn});
        }
        return true;
    }
}
