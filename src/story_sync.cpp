#include "story_engine.h"
#include "story_utils.h"
#include <ArduinoJson.h>
#include "storage.h"
#include "net.h"
#include <WiFi.h>

namespace story
{
    extern std::vector<Story_t> g_stories;
    void syncFromHTTP(const String &baseUrl, bool enabled)
    {
        if (!enabled || baseUrl.length() == 0) {
            return;
        }
        
        if (!story_utils::isWiFiConnected()) {
            return;
        }
        
        String idxUrl = baseUrl + "/index.json";
        String idx;
        if (!net::httpGet(idxUrl, idx)) {
            return;
        }
        
        storage::writeStringToFile("/index.json", idx);
        
        JsonDocument doc;
        if (!story_utils::parseJsonSafely(idx, doc)) {
            return;
        }
        
        if (!story_utils::hasStoriesArray(doc)) {
            return;
        }
        
        JsonArray stories = story_utils::getStoriesArray(doc);
        for (JsonObject s : stories) {
            String file = s["file"].as<const char *>();
            if (file.length() == 0) {
                continue;
            }
            
            String url = baseUrl + file;
            String payload;
            if (net::httpGet(url, payload)) {
                storage::writeStringToFile(file.c_str(), payload);
            }
        }
    }
}
