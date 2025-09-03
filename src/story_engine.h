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
 * @return Reference to vector containing all loaded stories
 */
const std::vector<Story_t> &all();

/**
 * @brief Load all story files from the filesystem
 * Scans the stories directory and parses all JSON story files
 */
void loadFromFS();

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
