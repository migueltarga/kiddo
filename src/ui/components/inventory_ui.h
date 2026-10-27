/**
 * @file inventory_ui.h
 * @brief Inventory UI components and dialogs
 *
 * This file contains the InventoryUI namespace with functions for creating
 * inventory dialogs, item notifications, and managing inventory-related UI elements.
 */

#pragma once

#include <lvgl.h>
#include <functional>
#include "inventory_manager.h"
#include "styles.h"

namespace InventoryUI {

/**
 * @brief Configuration for inventory dialog
 */
struct InventoryDialogConfig {
    const char* title = nullptr;  /**< Dialog title (uses localized string if null) */
    lv_color_t bg_color = palette::paper();  /**< Background color */
    lv_color_t slot_color = palette::parchment();  /**< Inventory slot color */
    lv_color_t slot_border_color = palette::brown();  /**< Slot border color */
    bool show_for_choice = false;  /**< Whether showing for item choice */
    std::function<void(const InventoryItem_t&)> choice_callback = nullptr;  /**< Choice callback */
};

/**
 * @brief Show the inventory dialog
 * @param config Dialog configuration
 * @return The dialog's modal background
 */
lv_obj_t* showInventoryDialog(const InventoryDialogConfig& config = InventoryDialogConfig{});

/**
 * @brief Create the header button that toggles the inventory dialog
 * @param parent Header object
 * @return The button
 */
lv_obj_t* createInventoryButton(lv_obj_t* parent);

/**
 * @brief Rebuild the inventory dialog if it is open
 */
void updateInventoryDisplay();

/**
 * @brief Create the "you found an item" card shown in a story page
 *
 * Placed after the page's text, so the reader meets the item where the story
 * mentions it instead of in a popup before reading.
 *
 * @param parent Page content
 * @param item Item that was added
 * @return The card
 */
lv_obj_t* createFoundItemCard(lv_obj_t* parent, const InventoryItem_t& item);

/**
 * @brief Close the inventory dialog
 */
void cleanup();

}
