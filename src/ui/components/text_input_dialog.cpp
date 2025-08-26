#include "ui_components.h"
#include "../fonts.h"
#include "styles.h"
#include <lvgl.h>

lv_obj_t* ui_text_input_dialog_show(const TextInputConfig& config) {
    // Create full-screen modal
    lv_obj_t *modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(modal, 0, 0);
    lv_obj_set_style_bg_color(modal, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(modal, 0, 0);
    lv_obj_set_style_pad_all(modal, 0, 0);
    lv_obj_set_style_radius(modal, 0, 0);
    
    // Create header area (smaller for 2.8" display)
    lv_obj_t *header = lv_obj_create(modal);
    lv_obj_set_size(header, LV_PCT(100), 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xe9d4a9), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 8, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title (smaller font)
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, config.title);
    lv_obj_set_style_text_font(title, font14(), 0);
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_center(title);
    
    // Create keyboard first at bottom (smaller height)
    lv_obj_t *keyboard = lv_keyboard_create(modal);
    lv_obj_set_size(keyboard, LV_PCT(100), 120); // Smaller keyboard for 2.8" display
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Content area (between header and keyboard)
    lv_obj_t *content = lv_obj_create(modal);
    lv_obj_set_size(content, LV_PCT(100), 160); // 320 - 40 (header) - 120 (keyboard) = 160
    lv_obj_align_to(content, header, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_pad_all(content, 12, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    // Text area (more space without description)
    lv_obj_t *textarea = lv_textarea_create(content);
    lv_obj_set_size(textarea, LV_PCT(100), 80); // Increased from 50 to 80
    lv_obj_set_style_text_font(textarea, font12(), 0);
    lv_textarea_set_text(textarea, config.initial_text ? config.initial_text : "");
    lv_textarea_set_placeholder_text(textarea, config.placeholder_text ? config.placeholder_text : "https://...");
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 15); // Adjust top position slightly
    
    // Connect keyboard to textarea
    lv_keyboard_set_textarea(keyboard, textarea);
    
    // Button container (compact layout)
    lv_obj_t *btn_container = lv_obj_create(content);
    lv_obj_remove_style_all(btn_container);
    lv_obj_set_size(btn_container, LV_PCT(100), 40);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align_to(btn_container, textarea, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
    
    // Store references for cleanup
    lv_obj_set_user_data(modal, keyboard);
    
    // Cancel button (smaller)
    lv_obj_t *btn_cancel = lv_btn_create(btn_container);
    lv_obj_set_size(btn_cancel, 60, 28);
    apply_secondary_button_style(btn_cancel);
    lv_obj_add_event_cb(btn_cancel, [](lv_event_t *e) {
        TextInputConfig *config = (TextInputConfig*)lv_event_get_user_data(e);
        lv_obj_t *modal = lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e))));
        lv_obj_t *keyboard = (lv_obj_t*)lv_obj_get_user_data(modal);
        
        if (config->on_cancel) {
            config->on_cancel();
        }
        if (keyboard) {
            lv_obj_del(keyboard);
        }
        lv_obj_del(modal);
    }, LV_EVENT_CLICKED, (void*)&config);
    
    lv_obj_t *cancel_label = lv_label_create(btn_cancel);
    lv_obj_set_style_text_font(cancel_label, font12(), 0);
    lv_label_set_text(cancel_label, config.cancel_text);
    lv_obj_center(cancel_label);
    
    // Default button (if provided, smaller)
    if (config.on_default && config.default_text) {
        lv_obj_t *btn_default = lv_btn_create(btn_container);
        lv_obj_set_size(btn_default, 60, 28);
        apply_secondary_button_style(btn_default);
        lv_obj_set_style_bg_color(btn_default, lv_palette_main(LV_PALETTE_YELLOW), 0);
        lv_obj_add_event_cb(btn_default, [](lv_event_t *e) {
            TextInputConfig *config = (TextInputConfig*)lv_event_get_user_data(e);
            lv_obj_t *modal = lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e))));
            lv_obj_t *keyboard = (lv_obj_t*)lv_obj_get_user_data(modal);
            
            if (config->on_default) {
                config->on_default();
            }
            if (keyboard) {
                lv_obj_del(keyboard);
            }
            lv_obj_del(modal);
        }, LV_EVENT_CLICKED, (void*)&config);
        
        lv_obj_t *default_label = lv_label_create(btn_default);
        lv_obj_set_style_text_font(default_label, font12(), 0);
        lv_label_set_text(default_label, config.default_text);
        lv_obj_center(default_label);
    }
    
    // Save button (smaller)
    lv_obj_t *btn_save = lv_btn_create(btn_container);
    lv_obj_set_size(btn_save, 60, 28);
    apply_primary_button_style(btn_save);
    
    // Store textarea reference in button user data
    lv_obj_set_user_data(btn_save, textarea);
    
    lv_obj_add_event_cb(btn_save, [](lv_event_t *e) {
        TextInputConfig *config = (TextInputConfig*)lv_event_get_user_data(e);
        lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t *textarea = (lv_obj_t*)lv_obj_get_user_data(btn);
        lv_obj_t *modal = lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent(btn)));
        lv_obj_t *keyboard = (lv_obj_t*)lv_obj_get_user_data(modal);
        
        const char *text = lv_textarea_get_text(textarea);
        if (config->on_save) {
            config->on_save(text);
        }
        if (keyboard) {
            lv_obj_del(keyboard);
        }
        lv_obj_del(modal);
    }, LV_EVENT_CLICKED, (void*)&config);
    
    lv_obj_t *save_label = lv_label_create(btn_save);
    lv_obj_set_style_text_font(save_label, font12(), 0);
    lv_label_set_text(save_label, config.save_text);
    lv_obj_center(save_label);
    
    return modal;
}
