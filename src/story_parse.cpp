#include "story_engine.h"
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
        JsonObject nodes = doc["nodes"].as<JsonObject>();
        for (JsonPair kv : nodes)
        {
            String key = kv.key().c_str();
            JsonObject n = kv.value().as<JsonObject>();
            Node_t nn;
            nn.is_end = false;
            nn.text = normalizeText(n["text"].as<const char *>());
            nn.is_end = n["end"].is<bool>() ? (bool)n["end"] : false;
            if (n["choices"].is<JsonArray>())
            {
                for (JsonObject c : n["choices"].as<JsonArray>())
                {
                    Choice_t ch;
                    ch.text = c["text"].as<const char *>();
                    ch.next = c["next"].as<const char *>();
                    if (ch.text.length())
                        nn.choices.push_back(ch);
                }
            }
            out.nodes.push_back({key, nn});
        }
        return true;
    }
}
