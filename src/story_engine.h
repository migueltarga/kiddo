#pragma once

#include <Arduino.h>
#include <vector>
#include "models.h"

namespace story
{

    const std::vector<Story_t> &all();

    void loadFromFS();

    void syncFromHTTP(const String &baseUrl, bool enabled);

    void ensureSampleStories();

    bool parseStoryJson(const String &json, Story_t &out);

    String normalizeText(const String &in);

}
