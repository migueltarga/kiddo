/**
 * @file ui_components.h
 * @brief Common UI components and utilities
 *
 * This file contains common UI component definitions and utility functions
 * for creating reusable UI elements like backgrounds, headers, and dialogs.
 */

#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <lvgl.h>
#include <functional>

/**
 * @brief Background component configuration
 */
typedef struct {
    const lv_image_dsc_t *image_src;  /**< Background image source */
    lv_color_t bg_color;              /**< Background color */
} ui_background_config_t;

/**
 * @brief Create a background component
 * @param parent Parent LVGL object
 * @param config Background configuration
 * @return Created background object
 */
lv_obj_t *ui_background_create(lv_obj_t *parent, const ui_background_config_t *config);

/**
 * @brief Get default background configuration
 * @param image_src Background image source
 * @return Default configuration
 */
ui_background_config_t ui_background_config_default(const lv_image_dsc_t *image_src);

/**
 * @brief Header component configuration
 */
typedef struct {
    const char *title_text;           /**< Header title text */
    const char *back_button_text;     /**< Back button text */
    lv_event_cb_t back_callback;      /**< Back button callback */
    void *back_user_data;             /**< Back button user data */
    bool show_back_button;            /**< Whether to show back button */
    bool enable_marquee;              /**< Whether to enable marquee effect */
    bool center_title;                /**< Whether to center the title */
    uint32_t back_button_width;       /**< Back button width */
} ui_header_config_t;

/**
 * @brief Create a header component
 * @param parent Parent LVGL object
 * @param config Header configuration
 * @return Created header object
 */
lv_obj_t *ui_header_create(lv_obj_t *parent, const ui_header_config_t *config);

/**
 * @brief Get default header configuration
 * @param title_text Header title
 * @param back_callback Back button callback
 * @return Default configuration
 */
ui_header_config_t ui_header_config_default(const char *title_text, lv_event_cb_t back_callback);

/**
 * @brief Set header title text
 * @param header Header object
 * @param new_title New title text
 */
void ui_header_set_title(lv_obj_t *header, const char *new_title);

/**
 * @brief Navigation buttons component configuration
 */
typedef struct {
    const char *left_text;        /**< Left button text */
    const char *right_text;       /**< Right button text */
    lv_event_cb_t left_callback;  /**< Left button callback */
    lv_event_cb_t right_callback; /**< Right button callback */
    void *left_user_data;         /**< Left button user data */
    void *right_user_data;        /**< Right button user data */
} ui_nav_buttons_config_t;

/**
 * @brief Create navigation buttons component
 * @param parent Parent LVGL object
 * @param config Navigation buttons configuration
 */
void ui_nav_buttons_create(lv_obj_t *parent, const ui_nav_buttons_config_t *config);

/**
 * @brief Confirmation dialog configuration
 */
struct ConfirmationConfig {
    const char* title;                    /**< Dialog title */
    const char* message;                  /**< Dialog message */
    const char* confirm_text;             /**< Confirm button text */
    const char* cancel_text;              /**< Cancel button text */
    lv_color_t confirm_color = lv_palette_main(LV_PALETTE_RED);     /**< Confirm button color */
    lv_color_t confirm_text_color = lv_color_white();               /**< Confirm button text color */
    std::function<void()> on_confirm = nullptr;                     /**< Confirm callback */
    std::function<void()> on_cancel = nullptr;                      /**< Cancel callback */
};

/**
 * @brief Show confirmation dialog
 * @param config Dialog configuration
 * @return Created dialog object
 */
lv_obj_t* ui_confirmation_dialog_show(const ConfirmationConfig& config);

/**
 * @brief Text input dialog configuration
 */
struct TextInputConfig {
    const char* title;                    /**< Dialog title */
    const char* description;              /**< Dialog description */
    const char* initial_text;             /**< Initial text in input field */
    const char* placeholder_text = "https://..."; /**< Placeholder text */
    const char* save_text;                /**< Save button text */
    const char* cancel_text;              /**< Cancel button text */
    const char* default_text;             /**< Default button text */
    std::function<void(const char*)> on_save = nullptr;  /**< Save callback */
    std::function<void()> on_cancel = nullptr;           /**< Cancel callback */
    std::function<void()> on_default = nullptr;          /**< Default callback */
};

/**
 * @brief Show text input dialog
 * @param config Dialog configuration
 * @return Created dialog object
 */
lv_obj_t* ui_text_input_dialog_show(const TextInputConfig& config);

/**
 * @brief Loading overlay component configuration
 */
typedef struct {
    const char* message;      /**< Loading message text */
    lv_color_t box_color;     /**< Overlay box color */
    lv_color_t spinner_color; /**< Spinner color */
    lv_color_t text_color;    /**< Text color */
    uint16_t box_width;       /**< Overlay box width */
    uint16_t box_height;      /**< Overlay box height */
} ui_loading_overlay_config_t;

/**
 * @brief Create loading overlay component
 * @param parent Parent LVGL object
 * @param config Loading overlay configuration
 * @return Created overlay object
 */
lv_obj_t* ui_loading_overlay_create(lv_obj_t* parent, const ui_loading_overlay_config_t* config);

/**
 * @brief Get default loading overlay configuration
 * @param message Loading message
 * @return Default configuration
 */
ui_loading_overlay_config_t ui_loading_overlay_config_default(const char* message);

// Include inventory UI
#include "inventory_ui.h"

#endif // UI_COMPONENTS_H
