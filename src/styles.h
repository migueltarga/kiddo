/**
 * @file styles.h
 * @brief LVGL style definitions and utilities
 *
 * This file contains predefined LVGL styles for buttons, text, and other UI elements,
 * along with utility functions for style initialization and management.
 */

#pragma once
#include <lvgl.h>

// Minimal primary button style helpers
/** @brief Primary button style (orange background) */
static lv_style_t style_btn_primary;
/** @brief Secondary button style (gray background) */
static lv_style_t style_btn_secondary;
/** @brief Flag indicating if styles have been initialized */
static bool styles_inited = false;
/** @brief Compact style for minimal padding */
static lv_style_t style_compact;
/** @brief Flag indicating if compact style has been initialized */
static bool compact_inited = false;

/**
 * @brief Ensure button styles are initialized
 * Creates primary and secondary button styles if not already done
 */
inline void ensure_styles(){
  if(styles_inited) return;
  styles_inited = true;
  lv_style_init(&style_btn_primary);
  lv_style_set_bg_color(&style_btn_primary, lv_palette_main(LV_PALETTE_ORANGE));
  lv_style_set_bg_opa(&style_btn_primary, LV_OPA_COVER);
  lv_style_set_radius(&style_btn_primary, 10);
  lv_style_set_border_width(&style_btn_primary, 0);
  lv_style_set_pad_left(&style_btn_primary, 10);
  lv_style_set_pad_right(&style_btn_primary, 10);
  lv_style_set_pad_top(&style_btn_primary, 6);
  lv_style_set_pad_bottom(&style_btn_primary, 6);
  lv_style_set_text_color(&style_btn_primary, lv_color_white());
  // secondary
  lv_style_init(&style_btn_secondary);
  lv_style_set_bg_color(&style_btn_secondary, lv_palette_lighten(LV_PALETTE_GREY,2));
  lv_style_set_bg_opa(&style_btn_secondary, LV_OPA_COVER);
  lv_style_set_radius(&style_btn_secondary, 10);
  lv_style_set_border_width(&style_btn_secondary, 0);
  lv_style_set_pad_left(&style_btn_secondary, 10);
  lv_style_set_pad_right(&style_btn_secondary, 10);
  lv_style_set_pad_top(&style_btn_secondary, 6);
  lv_style_set_pad_bottom(&style_btn_secondary, 6);
  lv_style_set_text_color(&style_btn_secondary, lv_color_black());
}

/**
 * @brief Apply primary button style to a button object
 * @param btn LVGL button object to style
 */
inline void apply_primary_button_style(lv_obj_t* btn){
  ensure_styles();
  lv_obj_add_style(btn, &style_btn_primary, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn, lv_palette_darken(LV_PALETTE_ORANGE, 1), LV_PART_MAIN | LV_STATE_PRESSED);
}

/**
 * @brief Apply secondary button style to a button object
 * @param btn LVGL button object to style
 */
inline void apply_secondary_button_style(lv_obj_t* btn){
  ensure_styles();
  lv_obj_add_style(btn, &style_btn_secondary, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN | LV_STATE_PRESSED);
}

/**
 * @brief Ensure compact style is initialized
 * Creates a style with minimal padding and margins
 */
inline void ensure_compact_style(){
  if(compact_inited) return;
  compact_inited = true;
  lv_style_init(&style_compact);
  lv_style_set_pad_all(&style_compact, 0);
  lv_style_set_pad_hor(&style_compact, 0);
  lv_style_set_pad_ver(&style_compact, 0);
  lv_style_set_pad_row(&style_compact, 0);
  lv_style_set_pad_column(&style_compact, 2);
  lv_style_set_margin_all(&style_compact, 0);
  lv_style_set_text_line_space(&style_compact, 0);
  lv_style_set_bg_opa(&style_compact, LV_OPA_TRANSP);
}

/**
 * @brief Apply screen background gradient to an object
 * Creates a blue gradient background suitable for screens
 * @param obj LVGL object to apply background to
 */
inline void apply_screen_bg(lv_obj_t* obj){
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x4c8cb9), 0);
  lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xa6cdec), 0);
  lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}
