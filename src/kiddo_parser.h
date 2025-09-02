#pragma once

#include <Arduino.h>
#include <vector>

namespace KiddoParser {

struct ContentSegment {
    enum Type { TEXT, IMAGE };
    Type type;
    String content;
    
    ContentSegment(Type t, const String& c) : type(t), content(c) {}
};

struct ParsedContent {
    std::vector<ContentSegment> segments;
    String plain_text;
};

ParsedContent parseText(const String& input);

std::vector<String> getImageUrls(const String& input);

}
