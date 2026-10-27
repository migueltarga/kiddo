/**
 * @file story_utils.cpp
 * @brief Utility functions for story management and language handling
 *
 * This module provides utility functions for story management including
 * language matching and content filtering.
 */

#include "story_utils.h"
#include "i18n.h"

extern Language current_language;

namespace story_utils
{

/**
 * @brief Check if language matches current language setting
 * @param currentLang Current language setting
 * @param lang Language string to check
 * @return True if languages match
 */
bool matchesLanguage(Language currentLang, const String& lang)
{
    return (currentLang == LANG_PT && lang == "pt-br") ||
           (currentLang == LANG_EN && lang == "en");
}

/**
 * @brief Check if content should be shown based on current language
 * @param contentLang Language of the content
 * @return True if content should be shown
 */
bool shouldShowContent(const String& contentLang)
{
    // Show content if it matches current language (includes English fallback)
    return story_utils::matchesLanguage(current_language, contentLang);
}

/**
 * @brief Convert current language to language string
 * @return Current language as string
 */
String currentLanguageToString()
{
    if (current_language == LANG_PT) return "pt-br";
    if (current_language == LANG_EN) return "en";
    return "en"; // Default fallback
}

}
