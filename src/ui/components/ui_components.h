#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <lvgl.h>
#include <functional>

// Background component
typedef struct {
    const lv_image_dsc_t *image_src;
    lv_color_t bg_color;
} ui_background_config_t;

lv_obj_t *ui_background_create(lv_obj_t *parent, const ui_background_config_t *config);
ui_background_config_t ui_background_config_default(const lv_image_dsc_t *image_src);

// Header component
typedef struct {
    const char *title_text;
    const char *back_button_text;
    lv_event_cb_t back_callback;
    void *back_user_data;
    bool show_back_button;
    bool enable_marquee;
    bool center_title;
    uint32_t back_button_width;
} ui_header_config_t;

lv_obj_t *ui_header_create(lv_obj_t *parent, const ui_header_config_t *config);
ui_header_config_t ui_header_config_default(const char *title_text, lv_event_cb_t back_callback);

// Navigation buttons component
typedef struct {
    const char *left_text;
    const char *right_text;
    lv_event_cb_t left_callback;
    lv_event_cb_t right_callback;
    void *left_user_data;
    void *right_user_data;
} ui_nav_buttons_config_t;

void ui_nav_buttons_create(lv_obj_t *parent, const ui_nav_buttons_config_t *config);

// Confirmation dialog component
struct ConfirmationConfig {
    const char* title;
    const char* message;
    const char* confirm_text;
    const char* cancel_text;
    lv_color_t confirm_color = lv_palette_main(LV_PALETTE_RED);
    lv_color_t confirm_text_color = lv_color_white();
    std::function<void()> on_confirm = nullptr;
    std::function<void()> on_cancel = nullptr;
};

lv_obj_t* ui_confirmation_dialog_show(const ConfirmationConfig& config);

// Text input dialog component
struct TextInputConfig {
    const char* title;
    const char* description;
    const char* initial_text;
    const char* placeholder_text = "https://..."; // Default placeholder
    const char* save_text;
    const char* cancel_text;
    const char* default_text;
    std::function<void(const char*)> on_save = nullptr;
    std::function<void()> on_cancel = nullptr;
    std::function<void()> on_default = nullptr;
};

lv_obj_t* ui_text_input_dialog_show(const TextInputConfig& config);

// Loading overlay component
typedef struct {
    const char* message;
    lv_color_t box_color;
    lv_color_t spinner_color;
    lv_color_t text_color;
    uint16_t box_width;
    uint16_t box_height;
} ui_loading_overlay_config_t;

lv_obj_t* ui_loading_overlay_create(lv_obj_t* parent, const ui_loading_overlay_config_t* config);
ui_loading_overlay_config_t ui_loading_overlay_config_default(const char* message);

// Include inventory UI
#include "inventory_ui.h"

#endif // UI_COMPONENTS_H
