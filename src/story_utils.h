#pragma once

#include <ArduinoJson.h>
#include <String.h>
#include "i18n.h"

namespace story_utils
{
    // Check if a language matches the current language setting
    bool matchesLanguage(Language currentLang, const String& lang);
    
    // Check if content should be shown based on current language (with fallback for English)
    bool shouldShowContent(const String& contentLang);
    
    // Convert current language to language string
    String currentLanguageToString();
    
    // Load the story index
    bool loadIndex(JsonDocument& doc);
    
    // Save the story index
    bool saveIndex(const JsonDocument& doc);
    
    // Check if index contains a file
    bool indexContains(const String& file);
    
    // Add a file to the index
    bool addToIndex(const String& file, const String& name, const String& lang);
}
