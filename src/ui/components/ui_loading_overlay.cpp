/**
 * @file ui_loading_overlay.cpp
 * @brief Loading overlay component implementation
 *
 * This module provides a loading overlay component with spinner
 * and customizable text, colors, and sizing.
 */

#include "ui_components.h"
#include "styles.h"
#include "ui/fonts.h"

/**
 * @brief Create a loading overlay component
 * @param parent Parent LVGL object
 * @param config Loading overlay configuration
 * @return Created overlay object
 */
lv_obj_t* ui_loading_overlay_create(lv_obj_t* parent, const ui_loading_overlay_config_t* config)
{
    lv_obj_t *overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
    lv_obj_center(overlay);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *box = lv_obj_create(overlay);
    lv_obj_set_size(box, config->box_width, config->box_height);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, config->box_color, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_pad_all(box, 8, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *spinner = lv_spinner_create(box);
    lv_obj_set_size(spinner, 32, 32);
    lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, config->spinner_color, LV_PART_INDICATOR);

    lv_obj_t *lbl = lv_label_create(box);
    lv_label_set_text(lbl, config->message);
    lv_obj_set_style_text_font(lbl, font14(), 0);
    lv_obj_set_style_text_color(lbl, config->text_color, 0);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -4);

    return overlay;
}

ui_loading_overlay_config_t ui_loading_overlay_config_default(const char* message)
{
    ui_loading_overlay_config_t config;
    config.message = message;
    config.box_color = lv_palette_main(LV_PALETTE_ORANGE);
    config.spinner_color = lv_color_hex(0x2196F3);
    config.text_color = lv_color_white();
    config.box_width = 140;
    config.box_height = 100;
    return config;
}
