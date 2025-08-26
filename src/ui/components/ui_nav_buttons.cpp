#include "ui_components.h"
#include "styles.h"
#include "ui/fonts.h"
#include "audio.h"

void ui_nav_buttons_create(lv_obj_t *parent, const ui_nav_buttons_config_t *config)
{
    // Left button (Settings)
    lv_obj_t *btn_left = lv_btn_create(parent);
    ui_add_click_sound(btn_left);
    lv_obj_set_size(btn_left, 100, 44);
    apply_primary_button_style(btn_left);
    lv_obj_align(btn_left, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    if (config->left_callback) {
        lv_obj_add_event_cb(btn_left, config->left_callback, LV_EVENT_CLICKED, config->left_user_data);
    }
    
    lv_obj_t *label_left = lv_label_create(btn_left);
    lv_obj_set_style_text_font(label_left, font16(), 0);
    lv_label_set_text(label_left, config->left_text);
    lv_obj_center(label_left);

    // Right button (Stories)
    lv_obj_t *btn_right = lv_btn_create(parent);
    ui_add_click_sound(btn_right);
    lv_obj_set_size(btn_right, 100, 44);
    apply_primary_button_style(btn_right);
    lv_obj_align(btn_right, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
    if (config->right_callback) {
        lv_obj_add_event_cb(btn_right, config->right_callback, LV_EVENT_CLICKED, config->right_user_data);
    }
    
    lv_obj_t *label_right = lv_label_create(btn_right);
    lv_obj_set_style_text_font(label_right, font16(), 0);
    lv_label_set_text(label_right, config->right_text);
    lv_obj_center(label_right);
}
