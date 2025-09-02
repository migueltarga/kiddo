#include "inventory_ui.h"
#include "../fonts.h"
#include "ui_components.h"
#include "../../styles.h"

LV_IMG_DECLARE(image_backpack); // We'll need to create this later

namespace InventoryUI {

static lv_obj_t* g_inventory_dialog = nullptr;

// Simple backpack symbol for the button (using text for now)
static const char* BACKPACK_SYMBOL = LV_SYMBOL_HOME; // We'll replace this with a proper icon

lv_obj_t* showInventoryDialog(const InventoryDialogConfig& config) {
    if (g_inventory_dialog != nullptr) {
        return g_inventory_dialog; // Already open
    }
    
    // Create modal background
    lv_obj_t* modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_50, 0);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(modal, 0, 0);
    lv_obj_set_style_pad_all(modal, 0, 0);
    
    // Create dialog container
    lv_obj_t* dialog = lv_obj_create(modal);
    lv_obj_set_size(dialog, 280, 240);
    lv_obj_center(dialog);
    lv_obj_set_style_bg_color(dialog, config.bg_color, 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_border_color(dialog, config.slot_border_color, 0);
    lv_obj_set_style_radius(dialog, 8, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    lv_obj_t* title = lv_label_create(dialog);
    lv_label_set_text(title, config.title);
    lv_obj_set_style_text_font(title, font16(), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x333333), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Close button
    lv_obj_t* close_btn = lv_btn_create(dialog);
    lv_obj_set_size(close_btn, 30, 30);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_radius(close_btn, 15, 0);
    
    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_label, lv_color_white(), 0);
    lv_obj_center(close_label);
    
    // Store references for cleanup
    g_inventory_dialog = modal;
    
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
        InventoryDialogConfig* cfg = (InventoryDialogConfig*)lv_event_get_user_data(e);
        if (g_inventory_dialog) {
            lv_obj_del(g_inventory_dialog);
            g_inventory_dialog = nullptr;
        }
        if (cfg && cfg->on_close) {
            cfg->on_close();
        }
    }, LV_EVENT_CLICKED, (void*)&config);
    
    // Create inventory grid (5 slots in a row)
    lv_obj_t* grid_container = lv_obj_create(dialog);
    lv_obj_set_size(grid_container, 250, 120);
    lv_obj_align(grid_container, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_opa(grid_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_container, 0, 0);
    lv_obj_set_style_pad_all(grid_container, 5, 0);
    lv_obj_set_flex_flow(grid_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(grid_container, 8, 0);
    lv_obj_clear_flag(grid_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create inventory slots
    InventoryManager::Inventory& inventory = InventoryManager::getCurrentInventory();
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        lv_obj_t* slot = lv_obj_create(grid_container);
        lv_obj_set_size(slot, 40, 40);
        lv_obj_set_style_bg_color(slot, config.slot_color, 0);
        lv_obj_set_style_bg_opa(slot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(slot, 2, 0);
        lv_obj_set_style_border_color(slot, config.slot_border_color, 0);
        lv_obj_set_style_radius(slot, 4, 0);
        lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(slot, 2, 0);
        
        const InventoryItem_t& item = inventory.getItem(i);
        if (!item.isEmpty()) {
            // For now, just show the first letter of the item name
            // Later we can replace this with actual icons
            lv_obj_t* item_label = lv_label_create(slot);
            String display_text = item.name.substring(0, 1);
            display_text.toUpperCase();
            lv_label_set_text(item_label, display_text.c_str());
            lv_obj_set_style_text_font(item_label, font16(), 0);
            lv_obj_set_style_text_color(item_label, lv_color_hex(0x333333), 0);
            lv_obj_center(item_label);
            
            // Add tooltip with full item name
            lv_obj_set_user_data(slot, (void*)item.name.c_str());
        }
    }
    
    // Click modal background to close
    lv_obj_add_event_cb(modal, [](lv_event_t* e) {
        if (lv_event_get_target(e) == lv_event_get_current_target(e)) { // Only if clicked on background
            if (g_inventory_dialog) {
                lv_obj_del(g_inventory_dialog);
                g_inventory_dialog = nullptr;
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    return modal;
}

lv_obj_t* createInventoryButton(lv_obj_t* parent, std::function<void()> on_click) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 35, 35);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x8b7355), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x654321), 0);
    lv_obj_set_style_radius(btn, 4, 0);
    
    // Create backpack icon (using text symbol for now)
    lv_obj_t* icon = lv_label_create(btn);
    lv_label_set_text(icon, "B"); // Backpack symbol
    lv_obj_set_style_text_font(icon, font16(), 0);
    lv_obj_center(icon);
    
    // Store callback
    static std::function<void()> stored_callback;
    stored_callback = on_click;
    
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        if (stored_callback) {
            stored_callback();
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    return btn;
}

void updateInventoryDisplay() {
    // If inventory dialog is open, refresh it
    if (g_inventory_dialog != nullptr) {
        lv_obj_del(g_inventory_dialog);
        g_inventory_dialog = nullptr;
        showInventoryDialog();
    }
}

}
