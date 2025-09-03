/**
 * @file inventory_ui.cpp
 * @brief Inventory user interface components
 *
 * This module provides UI components for inventory management including
 * inventory dialogs, item notifications, and inventory choice dialogs.
 * It handles both local C image assets and fallback icon generation.
 */

#include "inventory_ui.h"
#include "../fonts.h"
#include "ui_components.h"
#include "../../styles.h"
#include "../../audio.h"
#include "../images/images.h"
#include "../../i18n.h"

namespace InventoryUI {

// Constants for consistent styling
namespace Constants {
    /** @brief Colors for fallback item icons */
    const lv_color_t FALLBACK_COLORS[] = {
        lv_color_hex(0x4CAF50), // Green
        lv_color_hex(0x2196F3), // Blue
        lv_color_hex(0xFF9800), // Orange
        lv_color_hex(0x9C27B0), // Purple
        lv_color_hex(0xF44336), // Red
        lv_color_hex(0x607D8B)  // Blue Grey
    };
    /** @brief Number of fallback colors available */
    const size_t FALLBACK_COLOR_COUNT = sizeof(FALLBACK_COLORS) / sizeof(FALLBACK_COLORS[0]);

    /** @brief Border color for UI elements */
    const lv_color_t BORDER_COLOR = lv_color_hex(0x333333);
    /** @brief Border width for UI elements */
    const int BORDER_WIDTH = 2;
    /** @brief Small border radius */
    const int BORDER_RADIUS_SMALL = 6;
    /** @brief Medium border radius */
    const int BORDER_RADIUS_MEDIUM = 8;

    // Icon sizes
    const int INVENTORY_ICON_SIZE = 36;
    const int NOTIFICATION_ICON_SIZE = 48;
    const int SMALL_ICON_SIZE = 38;
}

// Utility function to generate consistent colors from strings
lv_color_t generateColorFromString(const String& str) {
    if (str.isEmpty()) return Constants::FALLBACK_COLORS[0];
    
    uint32_t hash = 0;
    for (size_t i = 0; i < str.length(); i++) {
        hash = hash * 31 + str.charAt(i);
    }
    
    return Constants::FALLBACK_COLORS[hash % Constants::FALLBACK_COLOR_COUNT];
}

// Utility function to create a fallback icon with consistent styling
lv_obj_t* createFallbackIcon(lv_obj_t* parent, const String& text, int size, int border_radius = Constants::BORDER_RADIUS_SMALL) {
    lv_obj_t* icon_bg = lv_obj_create(parent);
    lv_obj_set_size(icon_bg, size, size);
    lv_obj_center(icon_bg);
    
    lv_color_t color = generateColorFromString(text);
    lv_obj_set_style_bg_color(icon_bg, color, 0);
    lv_obj_set_style_bg_opa(icon_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(icon_bg, Constants::BORDER_WIDTH, 0);
    lv_obj_set_style_border_color(icon_bg, Constants::BORDER_COLOR, 0);
    lv_obj_set_style_radius(icon_bg, border_radius, 0);
    
    // Add first letter as text
    if (!text.isEmpty()) {
        lv_obj_t* icon_label = lv_label_create(icon_bg);
        char first_letter[2] = {text.charAt(0), '\0'};
        if (first_letter[0] >= 'a' && first_letter[0] <= 'z') {
            first_letter[0] = first_letter[0] - 'a' + 'A'; // Convert to uppercase
        }
        lv_label_set_text(icon_label, first_letter);
        lv_obj_set_style_text_font(icon_label, font16(), 0);
        lv_obj_set_style_text_color(icon_label, lv_color_white(), 0);
        lv_obj_center(icon_label);
    }
    
    return icon_bg;
}

// Map item IDs to C image declarations
const lv_img_dsc_t* getLocalItemImage(const String& item_id) {
    if (item_id == "golden_key" || item_id == "key") return &key;
    if (item_id == "map" || item_id == "old_map") return &old_map;
    if (item_id == "treasure" || item_id == "chest") return &chest;
    if (item_id == "magic_potion" || item_id == "potion") return &potion;
    if (item_id == "sword") return &sword;
    if (item_id == "compass") return &compass;
    if (item_id == "backpack") return &backpack;
    return nullptr;
}

bool displayLocalInventoryIcon(const String& item_id, lv_obj_t* parent, int target_size) {
    const lv_img_dsc_t* img_dsc = getLocalItemImage(item_id);
    if (!img_dsc) {
        Serial.printf("[INVENTORY_ICON] No local image found for item: %s\n", item_id.c_str());
        return false;
    }
    
    // Create image object
    lv_obj_t* img_obj = lv_img_create(parent);
    lv_obj_set_size(img_obj, target_size, target_size);
    lv_obj_center(img_obj);
    
    // Set the image source
    lv_img_set_src(img_obj, img_dsc);
    
    // Enable zoom to fit the target size if image is larger
    if (img_dsc->header.w > target_size || img_dsc->header.h > target_size) {
        // Calculate zoom factor to fit the image in the target size
        int zoom_x = (256 * target_size) / img_dsc->header.w;
        int zoom_y = (256 * target_size) / img_dsc->header.h;
        int zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y; // Use smaller zoom to maintain aspect ratio
        lv_img_set_zoom(img_obj, zoom);
        Serial.printf("[INVENTORY_ICON] Scaled image from %dx%d to fit %dx%d (zoom: %d)\n", 
                      img_dsc->header.w, img_dsc->header.h, target_size, target_size, zoom);
    }
    
    Serial.printf("[INVENTORY_ICON] Successfully displayed local icon for: %s\n", item_id.c_str());
    return true;
}

static lv_obj_t* g_inventory_dialog = nullptr;
static lv_obj_t* g_notification_dialog = nullptr;
static lv_timer_t* g_notification_timer = nullptr;

// Helper function to properly close inventory dialog and clean up config
void closeInventoryDialog() {
    if (g_inventory_dialog) {
        // Find the dialog container (child of modal)
        lv_obj_t* dialog = lv_obj_get_child(g_inventory_dialog, 0);
        if (dialog) {
            // Clean up stored config
            InventoryDialogConfig* stored_config = (InventoryDialogConfig*)lv_obj_get_user_data(dialog);
            if (stored_config) {
                delete stored_config;
            }
        }
        
        lv_obj_del(g_inventory_dialog);
        g_inventory_dialog = nullptr;
    }
}

// Helper function to properly close notification dialog and clean up timer
void closeNotificationDialog() {
    if (g_notification_timer) {
        lv_timer_del(g_notification_timer);
        g_notification_timer = nullptr;
    }
    if (g_notification_dialog) {
        // Check if the object is still valid before deleting
        if (lv_obj_is_valid(g_notification_dialog)) {
            lv_obj_del(g_notification_dialog);
        }
        g_notification_dialog = nullptr;
    }
}

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
    lv_obj_set_size(dialog, 260, 320);
    lv_obj_center(dialog);
    lv_obj_set_style_bg_color(dialog, config.bg_color, 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_border_color(dialog, config.slot_border_color, 0);
    lv_obj_set_style_radius(dialog, 8, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    lv_obj_t* title = lv_label_create(dialog);
    const char* title_text = config.title ? config.title : S()->inventory_title;
    lv_label_set_text(title, title_text);
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
    
    // Store config in dialog user data for choice callbacks
    InventoryDialogConfig* stored_config = new InventoryDialogConfig(config);
    lv_obj_set_user_data(dialog, stored_config);
    
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
        closeInventoryDialog();
    }, LV_EVENT_CLICKED, nullptr);
    
    // Create inventory list (vertical layout with item names)
    lv_obj_t* list_container = lv_obj_create(dialog);
    lv_obj_set_size(list_container, 240, 220);
    lv_obj_align(list_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(list_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_container, 0, 0);
    lv_obj_set_style_pad_all(list_container, 8, 0);
    lv_obj_set_flex_flow(list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(list_container, 5, 0);
    lv_obj_set_scroll_dir(list_container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list_container, LV_SCROLLBAR_MODE_AUTO);
    
    // Create inventory items
    InventoryManager::Inventory& inventory = InventoryManager::getCurrentInventory();
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        const InventoryItem_t& item = inventory.getItem(i);
        if (!item.isEmpty()) {
            // Create item row container
            lv_obj_t* item_row = lv_obj_create(list_container);
            lv_obj_set_size(item_row, 220, 50);
            lv_obj_set_style_bg_color(item_row, config.slot_color, 0);
            lv_obj_set_style_bg_opa(item_row, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(item_row, 2, 0);
            lv_obj_set_style_border_color(item_row, config.slot_border_color, 0);
            lv_obj_set_style_radius(item_row, 6, 0);
            lv_obj_clear_flag(item_row, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(item_row, 5, 0);
            lv_obj_set_flex_flow(item_row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(item_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(item_row, 10, 0);
            
            // Create icon container
            lv_obj_t* icon_container = lv_obj_create(item_row);
            lv_obj_set_size(icon_container, 40, 40);
            lv_obj_set_style_bg_color(icon_container, lv_color_white(), 0);
            lv_obj_set_style_bg_opa(icon_container, LV_OPA_20, 0);
            lv_obj_set_style_border_width(icon_container, 1, 0);
            lv_obj_set_style_border_color(icon_container, config.slot_border_color, 0);
            lv_obj_set_style_radius(icon_container, 4, 0);
            lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(icon_container, 2, 0);
            
            bool icon_displayed = false;
            
            // Try to use local C image
            if (displayLocalInventoryIcon(item.id, icon_container, 36)) {
                icon_displayed = true;
                Serial.println("Successfully displayed local icon for: " + item.id);
            }
            
            // If icon wasn't displayed, show colored fallback
            if (!icon_displayed) {
                createFallbackIcon(icon_container, item.name, Constants::INVENTORY_ICON_SIZE);
            }
            
            // Create item name label
            lv_obj_t* name_label = lv_label_create(item_row);
            lv_label_set_text(name_label, item.name.c_str());
            lv_obj_set_style_text_font(name_label, font14(), 0);
            lv_obj_set_style_text_color(name_label, lv_color_hex(0x333333), 0);
            lv_label_set_long_mode(name_label, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(name_label, 180);
            lv_obj_set_flex_grow(name_label, 1);
            
            // Add click handling for choice mode
            if (config.show_for_choice && config.choice_callback) {
                // Store item data for the click callback
                InventoryItem_t* item_data = new InventoryItem_t(item);
                
                // Make the whole row clickable
                lv_obj_add_flag(item_row, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_style_bg_color(item_row, lv_color_hex(0xd4c5a3), LV_STATE_PRESSED);
                
                lv_obj_add_event_cb(item_row, [](lv_event_t* e) {
                    // Get the config from the dialog user data
                    lv_obj_t* dialog = lv_obj_get_child(g_inventory_dialog, 0);
                    InventoryDialogConfig* dialog_config = dialog ? (InventoryDialogConfig*)lv_obj_get_user_data(dialog) : nullptr;
                    InventoryItem_t* item_data = (InventoryItem_t*)lv_event_get_user_data(e);
                    
                    if (dialog_config && dialog_config->choice_callback && item_data) {
                        // Call the choice callback
                        dialog_config->choice_callback(*item_data);
                        
                        // Close the dialog (this will clean up the config)
                        closeInventoryDialog();
                        
                        // Clean up item data
                        delete item_data;
                    }
                }, LV_EVENT_CLICKED, item_data);
            }
        }
    }
    
    // Click modal background to close
    lv_obj_add_event_cb(modal, [](lv_event_t* e) {
        if (lv_event_get_target(e) == lv_event_get_current_target(e)) { // Only if clicked on background
            closeInventoryDialog();
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    return modal;
}

lv_obj_t* showItemNotification(const ItemNotificationConfig& config) {
    // Close existing notification if any
    closeNotificationDialog();
    
    // Create modal background on the top layer to ensure it's above everything
    lv_obj_t* modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_50, 0); // Semi-transparent background
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(modal, 0, 0);
    lv_obj_set_style_pad_all(modal, 0, 0);
    
    // Add destructor callback to clean up global variables when modal is destroyed
    lv_obj_add_event_cb(modal, [](lv_event_t* e) {
        if (lv_event_get_code(e) == LV_EVENT_DELETE) {
            Serial.println("Notification modal being destroyed, clearing global variables");
            if (g_notification_timer) {
                lv_timer_del(g_notification_timer);
                g_notification_timer = nullptr;
            }
            g_notification_dialog = nullptr;
        }
    }, LV_EVENT_DELETE, nullptr);
    
    // Ensure the modal is absolutely on top
    lv_obj_move_foreground(modal);
    lv_obj_move_to_index(modal, -1); // Move to the very top of the layer stack
    Serial.println("Modal background created on top layer");
    
    // Create notification container as child of modal (proper parent-child relationship)
    lv_obj_t* notification = lv_obj_create(modal);
    lv_obj_set_size(notification, 240, 160);
    lv_obj_center(notification);
    lv_obj_set_style_bg_color(notification, config.bg_color, 0);
    lv_obj_set_style_bg_opa(notification, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(notification, 2, 0);
    lv_obj_set_style_border_color(notification, config.accent_color, 0);
    lv_obj_set_style_radius(notification, 8, 0);
    lv_obj_set_style_pad_all(notification, 0, 0); // Remove all padding
    lv_obj_clear_flag(notification, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_shadow_width(notification, 8, 0);
    lv_obj_set_style_shadow_color(notification, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(notification, LV_OPA_30, 0);
    lv_obj_move_foreground(notification);
    Serial.println("Notification container created");
    
    // Header with title and close button
    lv_obj_t* header = lv_obj_create(notification);
    lv_obj_set_size(header, 240, 35);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, config.accent_color, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 6, 0);
    lv_obj_set_style_pad_all(header, 0, 0); // Remove header padding
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Header title
    lv_obj_t* title = lv_label_create(header);
    const char* title_text = config.title ? config.title : S()->new_item_title;
    lv_label_set_text(title, title_text);
    lv_obj_set_style_text_font(title, font14(), 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);
    
    // Close button
    lv_obj_t* close_btn = lv_btn_create(header);
    lv_obj_set_size(close_btn, 25, 25);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, -5, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(close_btn, 1, 0);
    lv_obj_set_style_border_color(close_btn, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_radius(close_btn, 12, 0);
    
    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_label, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(close_label, font12(), 0);
    lv_obj_center(close_label);
    
    // Close button event
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
        closeNotificationDialog();
    }, LV_EVENT_CLICKED, nullptr);
    
    // Content area
    lv_obj_t* content = lv_obj_create(notification);
    lv_obj_set_size(content, 240, 125);
    lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(content, 12, 0);
    
    // Item icon container
    lv_obj_t* icon_container = lv_obj_create(content);
    lv_obj_set_size(icon_container, 48, 48);
    lv_obj_set_style_bg_opa(icon_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_container, 0, 0);
    lv_obj_set_style_pad_all(icon_container, 0, 0);
    lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Try to load item icon - local images first
    bool icon_displayed = false;
    
    // First try to display local image based on item ID
    String item_id = config.item_id ? String(config.item_id) : "";
    String display_name = config.item_name ? String(config.item_name) : "";
    
    // Use provided item_id or derive from item_name if not provided
    if (item_id.isEmpty() && !display_name.isEmpty()) {
        item_id = display_name;
        item_id.toLowerCase();
        item_id.replace(" ", "_");
    }
    
    if (!item_id.isEmpty()) {
        if (displayLocalInventoryIcon(item_id, icon_container, 48)) {
            icon_displayed = true;
            Serial.println("[NOTIFICATION] Successfully displayed local icon for: " + item_id);
        }
    }
    
    // If no local icon was displayed, show colored fallback
    if (!icon_displayed) {
        createFallbackIcon(icon_container, config.item_name ? String(config.item_name) : "item", Constants::SMALL_ICON_SIZE, Constants::BORDER_RADIUS_MEDIUM);
    }
    
    // Text content area (to the right of icon)
    lv_obj_t* text_area = lv_obj_create(content);
    lv_obj_set_size(text_area, 150, 48);
    lv_obj_set_style_bg_opa(text_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(text_area, 0, 0);
    lv_obj_set_style_pad_all(text_area, 0, 0);
    lv_obj_clear_flag(text_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(text_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(text_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(text_area, 2, 0);
    
    // Item name (highlighted)
    lv_obj_t* item_label = lv_label_create(text_area);
    lv_label_set_text(item_label, display_name.isEmpty() ? config.item_name : display_name.c_str());
    lv_obj_set_style_text_font(item_label, font16(), 0);
    lv_obj_set_style_text_color(item_label, lv_color_hex(0x2e8b57), 0); // Forest green
    lv_label_set_long_mode(item_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_label, 145);
    
    // Message
    lv_obj_t* message = lv_label_create(text_area);
    const char* message_text = config.message ? config.message : S()->new_item_message;
    lv_label_set_text(message, message_text);
    lv_obj_set_style_text_font(message, font12(), 0);
    lv_obj_set_style_text_color(message, lv_color_hex(0x555555), 0);
    lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(message, 145);
    
    // Store reference
    g_notification_dialog = modal;
    
    // Click to close - safer implementation (only when clicking background)
    lv_obj_add_event_cb(modal, [](lv_event_t* e) {
        // Only close if clicking directly on the modal background, not the notification content
        if (lv_event_get_target(e) == lv_event_get_current_target(e)) {
            closeNotificationDialog();
        }
    }, LV_EVENT_CLICKED, nullptr);
    
    // Auto-close timer with safer implementation
    if (config.auto_close_ms > 0) {
        g_notification_timer = lv_timer_create([](lv_timer_t* timer) {
            Serial.println("Auto-close timer triggered");
            // Extra safety check - only proceed if we still have a valid dialog pointer
            if (g_notification_dialog != nullptr) {
                closeNotificationDialog();
            } else {
                Serial.println("Notification dialog already closed");
                // Clear the timer reference since dialog is gone
                g_notification_timer = nullptr;
            }
        }, config.auto_close_ms, nullptr);
        lv_timer_set_repeat_count(g_notification_timer, 1); // Run only once
    }
    
    return modal;
}

lv_obj_t* createInventoryButton(lv_obj_t* parent) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 32, 32); // Keep original button size
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x8b7355), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x654321), 0);
    lv_obj_set_style_radius(btn, 4, 0);
    
    // Create backpack icon using the actual image with proper scaling and reduced padding
    lv_obj_t* icon = lv_img_create(btn);
    lv_img_set_src(icon, &backpack);
    
    // Set image to scale with zoom (128x128 to 28x28 = ~22% scale) - slightly larger
    lv_img_set_zoom(icon, 28 * 256 / 128); // LVGL zoom: 256 = 100%, so 56 = ~22%
    lv_obj_set_size(icon, 28, 28); // Less padding inside button
    lv_obj_center(icon);
    
    // Use a simple event handler without std::function
    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        // Simple inventory dialog toggle - safer implementation
        if (g_inventory_dialog != nullptr) {
            lv_obj_del(g_inventory_dialog);
            g_inventory_dialog = nullptr;
        } else {
            // Create with default config
            InventoryDialogConfig default_config;
            showInventoryDialog(default_config);
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

void notifyItemAdded(const InventoryItem_t& item, bool play_sound) {
    Serial.printf("notifyItemAdded called for: %s (ID: %s)\n", item.name.c_str(), item.id.c_str());
    
    // Play sound effect if requested
    if (play_sound) {
        // Play a pickup sound
        audio::play_click();
        Serial.println("Playing pickup sound");
    }
    
    // Show notification dialog with local icons only
    ItemNotificationConfig config;
    config.item_name = item.name.c_str();
    config.item_id = item.id.c_str();
    config.message = S()->item_added_message;
    config.auto_close_ms = 8000;
    
    Serial.printf("Showing notification for item: %s (ID: %s)\n", config.item_name, config.item_id);
    
    lv_obj_t* notification_result = showItemNotification(config);
    if (notification_result) {
        Serial.println("Notification dialog created successfully");
    } else {
        Serial.println("Failed to create notification dialog");
    }
    
    if (g_inventory_dialog) {
        updateInventoryDisplay();
    }
}

void showItemNotification(const char* item_name, const char* item_id, const char* custom_message) {
    ItemNotificationConfig config;
    config.item_name = item_name;
    config.item_id = item_id ? item_id : "";
    config.message = custom_message ? custom_message : "You found a new item!";
    
    showItemNotification(config);
}

void cleanup() {
    closeNotificationDialog();
    closeInventoryDialog();
}

}
