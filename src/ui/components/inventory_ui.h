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

namespace InventoryUI {

/**
 * @brief Configuration for inventory dialog
 */
struct InventoryDialogConfig {
    const char* title = nullptr;  /**< Dialog title (uses localized string if null) */
    lv_color_t bg_color = lv_color_hex(0xf5f5dc);  /**< Background color */
    lv_color_t slot_color = lv_color_hex(0xe9d4a9);  /**< Inventory slot color */
    lv_color_t slot_border_color = lv_color_hex(0x8b7355);  /**< Slot border color */
    std::function<void()> on_close = nullptr;  /**< Close callback */

    bool show_for_choice = false;  /**< Whether showing for item choice */
    std::function<void(const InventoryItem_t&)> choice_callback = nullptr;  /**< Choice callback */
};

/**
 * @brief Configuration for item notification
 */
struct ItemNotificationConfig {
    const char* title = nullptr;  /**< Notification title (uses localized string if null) */
    const char* message = nullptr;  /**< Notification message (uses localized string if null) */
    const char* item_name = "";  /**< Item name for display */
    const char* item_id = "";  /**< Item ID for local image lookup */
    lv_color_t bg_color = lv_color_hex(0xf0f8ff);  /**< Background color */
    lv_color_t accent_color = lv_color_hex(0x4682b4);  /**< Accent color */
    uint32_t auto_close_ms = 3000;  /**< Auto-close delay in milliseconds */
    std::function<void()> on_close = nullptr;  /**< Close callback */
};

lv_obj_t* showInventoryDialog(const InventoryDialogConfig& config = InventoryDialogConfig{});

/**
 * @brief Create and show item notification dialog
 * @param item_name Display name of the item
 * @param item_id ID for local image lookup (optional)
 * @param custom_message Custom message (optional)
 */
void showItemNotification(const char* item_name, const char* item_id = nullptr, const char* custom_message = nullptr);

lv_obj_t* createInventoryButton(lv_obj_t* parent);

void updateInventoryDisplay();

void notifyItemAdded(const InventoryItem_t& item, bool play_sound = true);

void cleanup();

}
