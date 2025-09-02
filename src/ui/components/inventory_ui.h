#pragma once

#include <lvgl.h>
#include <functional>
#include "inventory_manager.h"

namespace InventoryUI {

// Inventory dialog configuration
struct InventoryDialogConfig {
    const char* title = "Backpack";
    lv_color_t bg_color = lv_color_hex(0xf5f5dc);
    lv_color_t slot_color = lv_color_hex(0xe9d4a9);
    lv_color_t slot_border_color = lv_color_hex(0x8b7355);
    std::function<void()> on_close = nullptr;
};

// Create and show inventory dialog
lv_obj_t* showInventoryDialog(const InventoryDialogConfig& config = InventoryDialogConfig{});

// Create inventory button/icon for story screen
lv_obj_t* createInventoryButton(lv_obj_t* parent, std::function<void()> on_click);

// Update inventory display (call when inventory changes)
void updateInventoryDisplay();

}
