/**
 * @file styles.cpp
 * @brief Shared LVGL styles
 *
 * The styles live here, not in the header: a header with static styles and
 * inline helpers gives each file its own copy, and the linker may pair one
 * file's helper with another file's never-initialized style, so buttons fell
 * back to the default blue theme.
 */

#include "styles.h"

static lv_style_t style_btn_primary;
static lv_style_t style_btn_secondary;
static lv_style_t style_scrollbar;

/**
 * @brief Initialize the shared styles on first use
 */
static void ensure_styles() {
    static bool inited = false;
    if (inited) return;
    inited = true;

    lv_style_init(&style_btn_primary);
    lv_style_set_bg_color(&style_btn_primary, palette::primary());
    lv_style_set_bg_opa(&style_btn_primary, LV_OPA_COVER);
    lv_style_set_radius(&style_btn_primary, 10);
    lv_style_set_border_width(&style_btn_primary, 0);
    lv_style_set_shadow_width(&style_btn_primary, 0);
    lv_style_set_pad_hor(&style_btn_primary, 10);
    lv_style_set_pad_ver(&style_btn_primary, 6);
    lv_style_set_text_color(&style_btn_primary, lv_color_white());

    lv_style_init(&style_btn_secondary);
    lv_style_set_bg_color(&style_btn_secondary, palette::brown());
    lv_style_set_bg_opa(&style_btn_secondary, LV_OPA_COVER);
    lv_style_set_radius(&style_btn_secondary, 10);
    lv_style_set_border_width(&style_btn_secondary, 0);
    lv_style_set_shadow_width(&style_btn_secondary, 0);
    lv_style_set_pad_hor(&style_btn_secondary, 10);
    lv_style_set_pad_ver(&style_btn_secondary, 6);
    lv_style_set_text_color(&style_btn_secondary, lv_color_white());

    lv_style_init(&style_scrollbar);
    lv_style_set_width(&style_scrollbar, 4);
    lv_style_set_radius(&style_scrollbar, 2);
    lv_style_set_bg_color(&style_scrollbar, palette::brown());
    lv_style_set_bg_opa(&style_scrollbar, LV_OPA_60);
    lv_style_set_pad_right(&style_scrollbar, 2);
    lv_style_set_pad_ver(&style_scrollbar, 4);
}

void apply_primary_button_style(lv_obj_t* btn) {
    ensure_styles();
    lv_obj_add_style(btn, &style_btn_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, palette::primary_dark(), LV_PART_MAIN | LV_STATE_PRESSED);
}

void apply_secondary_button_style(lv_obj_t* btn) {
    ensure_styles();
    lv_obj_add_style(btn, &style_btn_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, palette::brown_dark(), LV_PART_MAIN | LV_STATE_PRESSED);
}

void apply_outline_button_style(lv_obj_t* btn, lv_color_t color) {
    ensure_styles();
    lv_obj_add_style(btn, &style_btn_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, palette::card(), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, color, 0);
    lv_obj_set_style_text_color(btn, color, 0);
    lv_obj_set_style_bg_color(btn, palette::paper(), LV_PART_MAIN | LV_STATE_PRESSED);
}

void apply_scrollbar_style(lv_obj_t* obj) {
    ensure_styles();
    lv_obj_add_style(obj, &style_scrollbar, LV_PART_SCROLLBAR);
}

void apply_screen_bg(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, palette::paper(), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_NONE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}
