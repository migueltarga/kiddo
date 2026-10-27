/**
 * @file story_progress.h
 * @brief Saved reading position for each story
 *
 * This file contains the story_progress namespace, which remembers the
 * current node and inventory of each story in NVS so a reader can resume
 * after leaving the story or powering off.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "inventory_manager.h"

namespace story_progress
{

/**
 * @brief A saved position in a story
 */
struct Saved
{
    String node;                    /**< ID of the node to resume at */
    std::vector<String> items;      /**< Item IDs in the inventory, in slot order */
};

/**
 * @brief Load the saved position of a story
 * @param story_id Story ID
 * @param out Filled with the saved position
 * @return True if a position was saved for this story
 */
bool load(const String &story_id, Saved &out);

/**
 * @brief Save the current position of a story
 * @param story_id Story ID
 * @param node Current node ID
 * @param inventory Current inventory
 */
void save(const String &story_id, const String &node, const InventoryManager::Inventory &inventory);

/**
 * @brief Forget the saved position of a story
 * @param story_id Story ID
 */
void clear(const String &story_id);

/**
 * @brief Forget the saved positions of all stories
 */
void clearAll();

}
