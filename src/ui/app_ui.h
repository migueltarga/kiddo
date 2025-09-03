/**
 * @file app_ui.h
 * @brief Main application UI management
 *
 * This file contains functions for managing the overall application UI,
 * including screen navigation, font scaling, and UI refresh operations.
 */

#pragma once
#include <lvgl.h>

/**
 * @brief Show home screen in the application
 */
void ui_app_show_home();

/**
 * @brief Set story font scale for the application
 * @param scale Font scale value
 */
void ui_app_set_story_font_scale(uint8_t scale);

/**
 * @brief Refresh home screen labels (for language changes)
 */
void ui_app_refresh_home_labels();
