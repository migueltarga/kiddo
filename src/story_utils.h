/**
 * @file story_utils.h
 * @brief Story utility functions
 *
 * This file contains utility functions for story management including
 * language matching, content filtering, and language conversion utilities.
 */

#pragma once

#include <ArduinoJson.h>
#include <String.h>
#include "i18n.h"

namespace story_utils
{

/**
 * @brief Check if a language matches the current language setting
 * @param currentLang Current language setting
 * @param lang Language to check
 * @return True if languages match
 */
bool matchesLanguage(Language currentLang, const String& lang);

/**
 * @brief Check if content should be shown based on current language (with fallback for English)
 * @param contentLang Language of the content
 * @return True if content should be shown
 */
bool shouldShowContent(const String& contentLang);

/**
 * @brief Convert current language to language string
 * @return Current language as string
 */
String currentLanguageToString();

/**
 * @brief Load the story index from file
 * @param doc JsonDocument to fill with index data
 * @return True if loading was successful
 */
bool loadIndex(JsonDocument& doc);

/**
 * @brief Save the story index to file
 * @param doc JsonDocument containing index data
 * @return True if saving was successful
 */
bool saveIndex(const JsonDocument& doc);

/**
 * @brief Check if index contains a specific file
 * @param file Filename to check
 * @return True if file is in index
 */
bool indexContains(const String& file);

/**
 * @brief Add a file to the story index
 * @param file Filename to add
 * @param name Display name of the story
 * @param lang Language of the story
 * @return True if addition was successful
 */
bool addToIndex(const String& file, const String& name, const String& lang);

}
