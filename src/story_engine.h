/**
 * @file story_engine.h
 * @brief Story management and parsing engine
 *
 * This file contains the story namespace with functions for loading,
 * parsing, and managing story data from JSON files and filesystem.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "models.h"

namespace story
{

/**
 * @brief Get all available stories
 *
 * Entries are summaries (id, title, start, file, has_inventory) without
 * nodes or items; use load() to read a complete story.
 *
 * @return Reference to vector containing the story summaries
 */
const std::vector<Story_t> &all();

/**
 * @brief Scan the filesystem for stories in the current language
 *
 * Rebuilds all(), which invalidates references into it.
 */
void loadFromFS();

/**
 * @brief Load a complete story
 * @param summary Entry from all()
 * @param out Reference to Story_t structure to fill
 * @return True if the story file was read and parsed
 */
bool load(const Story_t &summary, Story_t &out);

/**
 * @brief Parse a story JSON string into a Story_t structure
 * @param json JSON string containing story data
 * @param out Reference to Story_t structure to fill
 * @return True if parsing was successful
 */
bool parseStoryJson(const String &json, Story_t &out);

/**
 * @brief Normalize text for display (handle special characters, formatting)
 * @param in Input text string
 * @return Normalized text string
 */
String normalizeText(const String &in);

}
