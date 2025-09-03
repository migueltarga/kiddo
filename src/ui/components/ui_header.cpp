/**
 * @file ui_header.cpp
 * @brief Header component implementation
 *
 * This module provides a reusable header component with optional
 * back button, title text, and marquee scrolling functionality.
 */

#include "ui_components.h"
#include "styles.h"
#include "ui/fonts.h"
#include "audio.h"

/**
 * @brief Create a header component
 * @param parent Parent LVGL object
 * @param config Header configuration
 * @return Created header object
 */
lv_obj_t *ui_header_create(lv_obj_t *parent, const ui_header_config_t *config)
{
    // Create header container
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, 240, 38);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xe9d4a9), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_left(header, 6, 0);
    lv_obj_set_style_pad_right(header, 6, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    // Set flex layout based on whether we're centering and have a back button
    if (config->center_title && !config->show_back_button) {
        // For centered title without back button, center everything
        lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    } else {
        // Default layout with items starting from left
        lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    }

    // Back button (optional)
    if (config->show_back_button) {
        uint32_t btn_width = config->back_button_width > 0 ? config->back_button_width : 32;
        lv_obj_t *btn_back = lv_btn_create(header);
        ui_add_click_sound(btn_back);
        lv_obj_set_size(btn_back, btn_width, 32);  // Smaller height too
        apply_primary_button_style(btn_back);
        if (config->back_callback) {
            lv_obj_add_event_cb(btn_back, config->back_callback, LV_EVENT_CLICKED, config->back_user_data);
        }
        
        lv_obj_t *back_label = lv_label_create(btn_back);
        lv_obj_set_style_text_font(back_label, font14(), 0);
        lv_label_set_text(back_label, config->back_button_text ? config->back_button_text : "<");
        lv_obj_center(back_label);
    }

    // Title label
    if (config->title_text) {
        lv_obj_t *title_label = lv_label_create(header);
        lv_obj_set_style_text_font(title_label, font16(), 0);
        lv_label_set_text(title_label, config->title_text);
        lv_obj_set_style_text_color(title_label, lv_color_black(), 0);
        
        // Enable marquee for long titles (only if not centered)
        if (config->enable_marquee && !config->center_title) {
            lv_obj_set_width(title_label, 180);  // Fixed width for marquee
            lv_label_set_long_mode(title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        }
        
        // Store reference to title label in header user data for updates
        lv_obj_set_user_data(header, title_label);
    }

    return header;
}

void ui_header_set_title(lv_obj_t *header, const char *new_title)
{
    lv_obj_t *title_label = (lv_obj_t *)lv_obj_get_user_data(header);
    if (title_label && new_title) {
        lv_label_set_text(title_label, new_title);
    }
}

ui_header_config_t ui_header_config_default(const char *title_text, lv_event_cb_t back_callback)
{
    ui_header_config_t config = {
        .title_text = title_text,
        .back_button_text = "<",
        .back_callback = back_callback,
        .back_user_data = NULL,
        .show_back_button = (back_callback != NULL),
        .enable_marquee = true,
        .center_title = false,
        .back_button_width = 32
    };
    return config;
}
