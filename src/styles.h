/**
 * @file styles.h
 * @brief Shared colors and LVGL styles
 *
 * The app uses a warm "storybook" look: cream paper backgrounds, a parchment
 * header, dark brown ink for text, and orange for actions. Screens should take
 * their colors from the palette below rather than hard-coding them.
 */

#pragma once
#include <lvgl.h>

/** @brief The app's color palette */
namespace palette {
/** @brief Screen background */
inline lv_color_t paper() { return lv_color_hex(0xfbf6ea); }
/** @brief Cards and dialogs */
inline lv_color_t card() { return lv_color_white(); }
/** @brief Header and story choices panel */
inline lv_color_t parchment() { return lv_color_hex(0xe9d4a9); }
/** @brief Borders and dividers on parchment or paper */
inline lv_color_t parchment_dark() { return lv_color_hex(0xd4b98a); }
/** @brief Main text */
inline lv_color_t ink() { return lv_color_hex(0x3b2f25); }
/** @brief Secondary text */
inline lv_color_t ink_soft() { return lv_color_hex(0x7a6a58); }
/** @brief Secondary buttons and icons */
inline lv_color_t brown() { return lv_color_hex(0x8b7355); }
/** @brief Pressed secondary buttons */
inline lv_color_t brown_dark() { return lv_color_hex(0x6f5a41); }
/** @brief Main action buttons */
inline lv_color_t primary() { return lv_color_hex(0xf28c28); }
/** @brief Pressed main action buttons */
inline lv_color_t primary_dark() { return lv_color_hex(0xd2701a); }
/** @brief Destructive actions */
inline lv_color_t danger() { return lv_color_hex(0xd9534f); }
/** @brief Downloads and success messages */
inline lv_color_t accent() { return lv_color_hex(0x4c9a6a); }
}

/**
 * @brief Style a button as a main action (orange)
 * @param btn Button
 */
void apply_primary_button_style(lv_obj_t* btn);

/**
 * @brief Style a button as a secondary action (brown)
 * @param btn Button
 */
void apply_secondary_button_style(lv_obj_t* btn);

/**
 * @brief Style a button as an outlined action on a light background
 * @param btn Button
 * @param color Border and text color
 */
void apply_outline_button_style(lv_obj_t* btn, lv_color_t color);

/**
 * @brief Give a scrollable object a slim brown scrollbar
 *
 * Needed after lv_obj_remove_style_all(), which also removes the theme's
 * scrollbar, leaving no sign that more content is below.
 *
 * @param obj Scrollable object
 */
void apply_scrollbar_style(lv_obj_t* obj);

/**
 * @brief Paint a screen with the paper background
 * @param obj Screen or full-screen container
 */
void apply_screen_bg(lv_obj_t* obj);
