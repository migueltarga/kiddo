/**
 * @file story_parse.cpp
 * @brief JSON story parsing functionality
 *
 * This module handles parsing of JSON story files into Story_t structures,
 * including nodes, choices, inventory items, and item definitions.
 */

#include "story_engine.h"
#include "models.h"
#include <ArduinoJson.h>

namespace story
{

/**
 * @brief Parse story JSON string into Story_t structure
 * @param json JSON string containing story data
 * @param out Story_t structure to fill
 * @return True if parsing was successful
 */
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
        out.item_definitions.clear();
        
        // Parse item definitions first
        if (out.has_inventory && doc["item_definitions"].is<JsonArray>()) {
            for (JsonObject item_def : doc["item_definitions"].as<JsonArray>()) {
                ItemDefinition_t item;
                item.id = item_def["id"].as<const char *>();
                item.name = item_def["name"].as<const char *>();
                item.description = item_def["description"].is<const char *>() ? item_def["description"].as<const char *>() : "";
                item.icon_url = item_def["icon_url"].is<const char *>() ? item_def["icon_url"].as<const char *>() : "";
                item.local_icon = item_def["local_icon"].is<const char *>() ? item_def["local_icon"].as<const char *>() : "";
                if (item.id.length() > 0 && item.name.length() > 0) {
                    out.item_definitions.push_back(item);
                }
            }
        }
        
        if (out.has_inventory && doc["initial_inventory"].is<JsonArray>()) {
            for (JsonObject item : doc["initial_inventory"].as<JsonArray>()) {
                String item_id = item["id"].as<const char *>();
                Serial.printf("Looking for item definition for: %s\n", item_id.c_str());
                const ItemDefinition_t* item_def = out.getItemDefinition(item_id);
                if (item_def) {
                    Serial.printf("Found item definition: %s -> %s\n", item_def->id.c_str(), item_def->name.c_str());
                    InventoryItem_t inv_item;
                    inv_item.id = item_def->id;
                    inv_item.name = item_def->name;
                    inv_item.icon = item_def->icon_url;
                    out.initial_inventory.push_back(inv_item);
                } else {
                    Serial.printf("Item definition not found for initial inventory item: %s\n", item_id.c_str());
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
            
            // Parse inventory choice fields
            nn.inventory_choice = n["inventory_choice"].is<bool>() ? (bool)n["inventory_choice"] : false;
            nn.correct_item = n["correct_item"].is<const char *>() ? n["correct_item"].as<const char *>() : "";
            nn.success_next = n["success_next"].is<const char *>() ? n["success_next"].as<const char *>() : "";
            nn.failure_next = n["failure_next"].is<const char *>() ? n["failure_next"].as<const char *>() : "";
            
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
