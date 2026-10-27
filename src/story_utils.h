/**
 * @file story_utils.h
 * @brief Story utility functions
 *
 * This file contains utility functions for story management including
 * language matching, content filtering, and language conversion utilities.
 */

#pragma once

#include <Arduino.h>
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

}
