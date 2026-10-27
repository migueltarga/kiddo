/**
 * @file inventory_ui.cpp
 * @brief Inventory user interface components
 *
 * This module provides UI components for inventory management including
 * inventory dialogs, item notifications, and inventory choice dialogs.
 * It handles both local C image assets and fallback icon generation.
 *
 * Both popups use flex layouts sized to their content, so long item names
 * wrap and make the popup taller instead of being clipped.
 */

#include "inventory_ui.h"
#include "../fonts.h"
#include "ui_components.h"
#include "../../styles.h"
#include "../images/images.h"
#include "../../i18n.h"

namespace InventoryUI {

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

    /** @brief Text color for dialog content */
    const lv_color_t TEXT_COLOR = palette::ink();

    /** @brief Width of the inventory dialog (the screen is 240 px wide) */
    const lv_coord_t POPUP_WIDTH = 224;
    /** @brief Height after which the inventory list scrolls */
    const lv_coord_t INVENTORY_LIST_MAX_HEIGHT = 200;
    /** @brief Tallest the inventory dialog may get (the screen is 320 px tall) */
    const lv_coord_t INVENTORY_DIALOG_MAX_HEIGHT = 308;
    /** @brief The list never shrinks below one row to make room for the hint */
    const lv_coord_t INVENTORY_LIST_MIN_HEIGHT = 56;

    /** @brief Icon size in inventory rows */
    const int INVENTORY_ICON_SIZE = 36;
    /** @brief Icon size in the "found an item" card */
    const int FOUND_ICON_SIZE = 40;
}

static lv_obj_t* g_inventory_dialog = nullptr;

/**
 * @brief Pick a stable fallback color for an item name
 * @param str Item name
 * @return Color from the fallback palette
 */
static lv_color_t generateColorFromString(const String& str) {
    uint32_t hash = 0;
    for (size_t i = 0; i < str.length(); i++) {
        hash = hash * 31 + str.charAt(i);
    }
    return Constants::FALLBACK_COLORS[hash % Constants::FALLBACK_COLOR_COUNT];
}

/**
 * @brief Draw a colored square with the item's first letter
 * @param parent Icon container
 * @param text Item name
 * @param size Square size in pixels
 */
static void createFallbackIcon(lv_obj_t* parent, const String& text, int size) {
    lv_obj_t* icon_bg = lv_obj_create(parent);
    lv_obj_set_size(icon_bg, size, size);
    lv_obj_center(icon_bg);
    lv_obj_set_style_bg_color(icon_bg, generateColorFromString(text), 0);
    lv_obj_set_style_bg_opa(icon_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(icon_bg, 2, 0);
    lv_obj_set_style_border_color(icon_bg, Constants::TEXT_COLOR, 0);
    lv_obj_set_style_radius(icon_bg, 6, 0);
    lv_obj_set_style_pad_all(icon_bg, 0, 0);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE);
    // Let taps reach the item row underneath
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_CLICKABLE);

    if (text.isEmpty()) {
        return;
    }

    // Copy the whole first UTF-8 character so accented names ("Óculos") work
    uint8_t lead = (uint8_t)text[0];
    size_t len = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
    String first_letter = text.substring(0, len);
    first_letter.toUpperCase(); // ASCII only, leaves multibyte characters alone

    lv_obj_t* icon_label = lv_label_create(icon_bg);
    lv_label_set_text(icon_label, first_letter.c_str());
    lv_obj_set_style_text_font(icon_label, font16(), 0);
    lv_obj_set_style_text_color(icon_label, lv_color_white(), 0);
    lv_obj_center(icon_label);
}

/**
 * @brief Map an item ID to a bundled C image
 * @param item_id Item ID
 * @return Image descriptor, or nullptr if there is none
 */
static const lv_img_dsc_t* getLocalItemImage(const String& item_id) {
    if (item_id == "golden_key" || item_id == "key") return &key;
    if (item_id == "map" || item_id == "old_map") return &old_map;
    if (item_id == "treasure" || item_id == "chest") return &chest;
    if (item_id == "magic_potion" || item_id == "potion") return &potion;
    if (item_id == "sword") return &sword;
    if (item_id == "compass") return &compass;
    if (item_id == "backpack") return &backpack;
    if (item_id == "flashlight") return &icon_item_flashlight;
    if (item_id == "honey") return &icon_item_honey;
    if (item_id == "feather") return &icon_item_feather;
    if (item_id == "shell") return &icon_item_shell;
    return nullptr;
}

/**
 * @brief Show an item's icon, falling back to a lettered square
 * @param parent Icon container
 * @param item Item to show
 * @param size Icon size in pixels
 */
static void createItemIcon(lv_obj_t* parent, const InventoryItem_t& item, int size) {
    const lv_img_dsc_t* img_dsc = getLocalItemImage(item.id);
    if (!img_dsc) {
        createFallbackIcon(parent, item.name, size);
        return;
    }

    lv_obj_t* img_obj = lv_img_create(parent);
    lv_obj_set_size(img_obj, size, size);
    lv_obj_center(img_obj);
    lv_img_set_src(img_obj, img_dsc);
    // Line icons are alpha only and take their color from the recolor
    if (img_dsc->header.cf == LV_COLOR_FORMAT_A8) {
        lv_obj_set_style_image_recolor(img_obj, palette::ink(), 0);
        lv_obj_set_style_image_recolor_opa(img_obj, LV_OPA_COVER, 0);
    }

    // Scale down to fit, keeping the aspect ratio
    if (img_dsc->header.w > size || img_dsc->header.h > size) {
        int zoom_x = (256 * size) / img_dsc->header.w;
        int zoom_y = (256 * size) / img_dsc->header.h;
        lv_img_set_zoom(img_obj, zoom_x < zoom_y ? zoom_x : zoom_y);
    }
}

/**
 * @brief Create a transparent box with no border or padding
 * @param parent Parent object
 * @return The box
 */
static lv_obj_t* createPlainBox(lv_obj_t* parent) {
    lv_obj_t* box = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_radius(box, 0, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

/**
 * @brief Create a full-screen dimmed background that centers its child
 *
 * Tapping the background itself (not the popup) runs close_fn.
 *
 * @param parent Screen or layer to cover
 * @param close_fn Function that closes the popup
 * @return The modal background
 */
static lv_obj_t* createModalBackground(lv_obj_t* parent, void (*close_fn)()) {
    lv_obj_t* modal = createPlainBox(parent);
    lv_obj_set_size(modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_50, 0);
    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(modal, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(modal, [](lv_event_t* e) {
        if (lv_event_get_target(e) == lv_event_get_current_target(e)) {
            ((void (*)())lv_event_get_user_data(e))();
        }
    }, LV_EVENT_CLICKED, (void*)close_fn);
    return modal;
}

/**
 * @brief Create a popup card: a fixed-width column as tall as its content
 * @param parent Modal background
 * @param bg_color Card color
 * @param border_color Border color
 * @return The card
 */
static lv_obj_t* createCard(lv_obj_t* parent, lv_color_t bg_color, lv_color_t border_color) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, Constants::POPUP_WIDTH, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, bg_color, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, border_color, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/**
 * @brief Create a header row with a wrapping title and a round close button
 * @param parent Card
 * @param title Title text
 * @param title_color Title color
 * @param btn_color Close button color
 * @param close_fn Function the close button runs
 */
static void createHeader(lv_obj_t* parent, const char* title, lv_color_t title_color,
                         lv_color_t btn_color, void (*close_fn)()) {
    lv_obj_t* header = createPlainBox(parent);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(header, 8, 0);

    lv_obj_t* title_label = lv_label_create(header);
    lv_label_set_text(title_label, title);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(title_label, font16(), 0);
    lv_obj_set_style_text_color(title_label, title_color, 0);
    lv_obj_set_flex_grow(title_label, 1);

    lv_obj_t* close_btn = lv_btn_create(header);
    lv_obj_set_size(close_btn, 28, 28);
    lv_obj_set_style_bg_color(close_btn, btn_color, 0);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(close_btn, 0, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
        ((void (*)())lv_event_get_user_data(e))();
    }, LV_EVENT_CLICKED, (void*)close_fn);

    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(close_label, lv_color_white(), 0);
    lv_obj_center(close_label);
}

/**
 * @brief Close the inventory dialog if it is open
 *
 * The dialog's config and item data are freed by delete events, so this is
 * also safe when the dialog was already removed by a screen clean.
 */
static void closeInventoryDialog() {
    if (g_inventory_dialog) {
        lv_obj_t* modal = g_inventory_dialog;
        g_inventory_dialog = nullptr;
        lv_obj_delete(modal);
    }
}

/**
 * @brief Describe a tapped item at the bottom of the inventory dialog
 *
 * Used when the story is not asking for an item. The list gives up height
 * so the dialog still fits on screen with a long description.
 *
 * @param item Item that was tapped
 */
static void showItemHint(const InventoryItem_t& item) {
    lv_obj_t* dialog = g_inventory_dialog ? lv_obj_get_child(g_inventory_dialog, 0) : nullptr;
    if (!dialog) {
        return;
    }
    // Dialog children: header, list, hint
    lv_obj_t* list = lv_obj_get_child(dialog, 1);
    lv_obj_t* hint = lv_obj_get_child(dialog, 2);

    lv_label_set_text(lv_obj_get_child(hint, 0), item.name.c_str());
    lv_obj_t* description = lv_obj_get_child(hint, 1);
    lv_label_set_text(description, item.description.c_str());
    if (item.description.isEmpty()) {
        lv_obj_add_flag(description, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(description, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(hint, LV_OBJ_FLAG_HIDDEN);

    // Everything but the list and the hint: the header, gaps, padding and border
    lv_obj_update_layout(dialog);
    lv_coord_t others = lv_obj_get_height(dialog) - lv_obj_get_height(list) - lv_obj_get_height(hint);
    lv_coord_t list_max = Constants::INVENTORY_DIALOG_MAX_HEIGHT - others - lv_obj_get_height(hint);
    if (list_max > Constants::INVENTORY_LIST_MAX_HEIGHT) list_max = Constants::INVENTORY_LIST_MAX_HEIGHT;
    if (list_max < Constants::INVENTORY_LIST_MIN_HEIGHT) list_max = Constants::INVENTORY_LIST_MIN_HEIGHT;
    lv_obj_set_style_max_height(list, list_max, 0);
}

/**
 * @brief Create the hidden item details block below the inventory list
 * @param dialog Inventory dialog card
 */
static void createItemHint(lv_obj_t* dialog) {
    lv_obj_t* hint = createPlainBox(dialog);
    lv_obj_set_size(hint, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hint, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hint, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(hint, 2, 0);
    lv_obj_add_flag(hint, LV_OBJ_FLAG_HIDDEN);

    // Name, description, then the "not needed" note
    const lv_font_t* fonts[] = {font14(), font12(), font12()};
    const lv_color_t colors[] = {palette::brown_dark(), Constants::TEXT_COLOR, palette::brown()};
    for (int i = 0; i < 3; i++) {
        lv_obj_t* label = lv_label_create(hint);
        lv_obj_set_width(label, LV_PCT(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(label, fonts[i], 0);
        lv_obj_set_style_text_color(label, colors[i], 0);
    }
    lv_label_set_text(lv_obj_get_child(hint, 2), S()->item_not_needed);
}

/**
 * @brief Add one inventory item row to the dialog's list
 *
 * In choice mode a tap picks the item; otherwise it shows the item's
 * description and that it is not needed right now.
 *
 * @param list List container
 * @param item Item to show
 * @param config Dialog configuration
 */
static void createItemRow(lv_obj_t* list, const InventoryItem_t& item, const InventoryDialogConfig& config) {
    lv_obj_t* item_row = lv_obj_create(list);
    lv_obj_set_size(item_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(item_row, 50, 0);
    lv_obj_set_style_bg_color(item_row, config.slot_color, 0);
    lv_obj_set_style_bg_opa(item_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(item_row, 2, 0);
    lv_obj_set_style_border_color(item_row, config.slot_border_color, 0);
    lv_obj_set_style_radius(item_row, 6, 0);
    lv_obj_set_style_pad_all(item_row, 5, 0);
    lv_obj_set_style_pad_column(item_row, 8, 0);
    lv_obj_clear_flag(item_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(item_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* icon_container = lv_obj_create(item_row);
    lv_obj_set_size(icon_container, 40, 40);
    lv_obj_set_style_bg_color(icon_container, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(icon_container, LV_OPA_20, 0);
    lv_obj_set_style_border_width(icon_container, 1, 0);
    lv_obj_set_style_border_color(icon_container, config.slot_border_color, 0);
    lv_obj_set_style_radius(icon_container, 4, 0);
    lv_obj_set_style_pad_all(icon_container, 0, 0);
    lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_SCROLLABLE);
    // Let taps on the icon reach the row
    lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_CLICKABLE);
    createItemIcon(icon_container, item, Constants::INVENTORY_ICON_SIZE);

    lv_obj_t* name_label = lv_label_create(item_row);
    lv_label_set_text(name_label, item.name.c_str());
    lv_label_set_long_mode(name_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(name_label, font14(), 0);
    lv_obj_set_style_text_color(name_label, Constants::TEXT_COLOR, 0);
    lv_obj_set_flex_grow(name_label, 1);

    lv_obj_set_style_bg_color(item_row, palette::parchment_dark(), LV_STATE_PRESSED);

    InventoryItem_t* item_data = new InventoryItem_t(item);
    lv_obj_add_event_cb(item_row, [](lv_event_t* e) {
        lv_obj_t* dialog = g_inventory_dialog ? lv_obj_get_child(g_inventory_dialog, 0) : nullptr;
        InventoryDialogConfig* dialog_config = dialog ? (InventoryDialogConfig*)lv_obj_get_user_data(dialog) : nullptr;
        InventoryItem_t* item_data = (InventoryItem_t*)lv_event_get_user_data(e);
        if (!dialog_config || !item_data) {
            return;
        }
        if (!dialog_config->show_for_choice || !dialog_config->choice_callback) {
            showItemHint(*item_data);
            return;
        }

        // Copy what we need, then close the dialog before running the
        // callback: it usually navigates and cleans the screen, which
        // would otherwise delete the dialog underneath us.
        auto callback = dialog_config->choice_callback;
        InventoryItem_t selected = *item_data;
        closeInventoryDialog();
        callback(selected);
    }, LV_EVENT_CLICKED, item_data);
    lv_obj_add_event_cb(item_row, [](lv_event_t* e) {
        delete (InventoryItem_t*)lv_event_get_user_data(e);
    }, LV_EVENT_DELETE, item_data);
}

lv_obj_t* showInventoryDialog(const InventoryDialogConfig& config) {
    if (g_inventory_dialog != nullptr) {
        return g_inventory_dialog; // Already open
    }

    lv_obj_t* modal = createModalBackground(lv_scr_act(), closeInventoryDialog);
    g_inventory_dialog = modal;
    lv_obj_add_event_cb(modal, [](lv_event_t* e) {
        // Also reached when a screen clean deletes the dialog
        if (lv_event_get_target(e) == g_inventory_dialog) {
            g_inventory_dialog = nullptr;
        }
    }, LV_EVENT_DELETE, nullptr);

    lv_obj_t* dialog = createCard(modal, config.bg_color, config.slot_border_color);

    // Keep the config for the item rows' click handler
    InventoryDialogConfig* stored_config = new InventoryDialogConfig(config);
    lv_obj_set_user_data(dialog, stored_config);
    lv_obj_add_event_cb(dialog, [](lv_event_t* e) {
        delete (InventoryDialogConfig*)lv_event_get_user_data(e);
    }, LV_EVENT_DELETE, stored_config);

    const char* title_text = config.title ? config.title : S()->inventory_title;
    createHeader(dialog, title_text, Constants::TEXT_COLOR,
                 palette::brown(), closeInventoryDialog);

    // Grows with its rows, then scrolls
    lv_obj_t* list = createPlainBox(dialog);
    lv_obj_set_size(list, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(list, Constants::INVENTORY_LIST_MAX_HEIGHT, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

    createItemHint(dialog);

    InventoryManager::Inventory& inventory = InventoryManager::getCurrentInventory();
    int shown = 0;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        const InventoryItem_t& item = inventory.getItem(i);
        if (!item.isEmpty()) {
            createItemRow(list, item, config);
            shown++;
        }
    }
    if (shown == 0) {
        lv_obj_t* empty = lv_label_create(list);
        lv_obj_set_width(empty, LV_PCT(100));
        lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(empty, font14(), 0);
        lv_obj_set_style_text_color(empty, palette::brown(), 0);
        lv_obj_set_style_pad_ver(empty, 10, 0);
        lv_label_set_text(empty, S()->inventory_empty);
    }

    return modal;
}

lv_obj_t* createFoundItemCard(lv_obj_t* parent, const InventoryItem_t& item) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, palette::parchment(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, palette::parchment_dark(), 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_set_style_pad_column(card, 10, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // White tile, so item images with a white background look framed
    lv_obj_t* icon_container = createPlainBox(card);
    lv_obj_set_size(icon_container, Constants::FOUND_ICON_SIZE + 6, Constants::FOUND_ICON_SIZE + 6);
    lv_obj_set_style_bg_color(icon_container, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(icon_container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(icon_container, 8, 0);
    lv_obj_set_style_border_width(icon_container, 1, 0);
    lv_obj_set_style_border_color(icon_container, palette::parchment_dark(), 0);
    lv_obj_set_style_clip_corner(icon_container, true, 0);
    lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_CLICKABLE);
    createItemIcon(icon_container, item, Constants::FOUND_ICON_SIZE);

    lv_obj_t* text_area = createPlainBox(card);
    lv_obj_set_height(text_area, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(text_area, 1);
    lv_obj_set_flex_flow(text_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(text_area, 2, 0);
    lv_obj_clear_flag(text_area, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* name_label = lv_label_create(text_area);
    lv_label_set_text(name_label, item.name.c_str());
    lv_label_set_long_mode(name_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(name_label, LV_PCT(100));
    lv_obj_set_style_text_font(name_label, font14(), 0);
    lv_obj_set_style_text_color(name_label, palette::ink(), 0);

    lv_obj_t* message = lv_label_create(text_area);
    lv_label_set_text_fmt(message, LV_SYMBOL_PLUS " %s", S()->item_added_message);
    lv_label_set_long_mode(message, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(message, LV_PCT(100));
    lv_obj_set_style_text_font(message, font12(), 0);
    lv_obj_set_style_text_color(message, palette::primary_dark(), 0);

    return card;
}

lv_obj_t* createInventoryButton(lv_obj_t* parent) {
    // Same look as the header's back button
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 32, 30);
    apply_secondary_button_style(btn);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    // Simple line icon drawn for this size; the full-color backpack image
    // turned into a blur when scaled down
    lv_obj_t* icon = lv_img_create(btn);
    lv_img_set_src(icon, &icon_backpack);
    lv_obj_set_style_image_recolor(icon, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, 0);
    lv_obj_center(icon);

    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        if (g_inventory_dialog != nullptr) {
            closeInventoryDialog();
        } else {
            showInventoryDialog();
        }
    }, LV_EVENT_CLICKED, nullptr);

    return btn;
}

void updateInventoryDisplay() {
    // If inventory dialog is open, rebuild it with the same config
    if (g_inventory_dialog != nullptr) {
        lv_obj_t* dialog = lv_obj_get_child(g_inventory_dialog, 0);
        InventoryDialogConfig* stored = dialog ? (InventoryDialogConfig*)lv_obj_get_user_data(dialog) : nullptr;
        InventoryDialogConfig config = stored ? *stored : InventoryDialogConfig{};
        closeInventoryDialog();
        showInventoryDialog(config);
    }
}

void cleanup() {
    closeInventoryDialog();
}

}
