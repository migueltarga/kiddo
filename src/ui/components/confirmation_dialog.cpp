#include "ui_components.h"
#include "../fonts.h"
#include <lvgl.h>

lv_obj_t* ui_confirmation_dialog_show(const ConfirmationConfig& config) {
    lv_obj_t *modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_center(modal);
    lv_obj_set_style_bg_color(modal, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(modal, 2, 0);
    lv_obj_set_style_border_color(modal, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_pad_all(modal, 16, 0);
    lv_obj_set_style_radius(modal, 8, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(modal);
    lv_label_set_text(title, config.title);
    lv_obj_set_style_text_font(title, font16(), 0);
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    // Message
    lv_obj_t *msg = lv_label_create(modal);
    lv_label_set_text(msg, config.message);
    lv_obj_set_style_text_font(msg, font14(), 0);
    lv_obj_set_style_text_color(msg, lv_color_black(), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(msg, LV_PCT(100));
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_align_to(msg, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);
    
    // Button container
    lv_obj_t *btn_container = lv_obj_create(modal);
    lv_obj_set_size(btn_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_pad_all(btn_container, 0, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align_to(btn_container, msg, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);
    
    // Cancel button
    lv_obj_t *btn_cancel = lv_btn_create(btn_container);
    lv_obj_set_size(btn_cancel, 80, 32);
    lv_obj_set_style_bg_color(btn_cancel, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, config.cancel_text);
    lv_obj_set_style_text_font(lbl_cancel, font14(), 0);
    lv_obj_center(lbl_cancel);
    
    struct CallbackData {
        std::function<void()> on_confirm;
        std::function<void()> on_cancel;
    };
    
    CallbackData* callback_data = new CallbackData{config.on_confirm, config.on_cancel};
    lv_obj_set_user_data(modal, callback_data);
    
    lv_obj_add_event_cb(btn_cancel, [](lv_event_t *ee) {
        lv_obj_t *modal = (lv_obj_t*)lv_event_get_user_data(ee);
        CallbackData* data = (CallbackData*)lv_obj_get_user_data(modal);
        
        if (data && data->on_cancel) {
            data->on_cancel();
        }
        
        delete data;
        lv_obj_del(modal);
    }, LV_EVENT_CLICKED, modal);
    
    // Confirm button
    lv_obj_t *btn_confirm = lv_btn_create(btn_container);
    lv_obj_set_size(btn_confirm, 80, 32);
    lv_obj_set_style_bg_color(btn_confirm, config.confirm_color, 0);
    lv_obj_t *lbl_confirm = lv_label_create(btn_confirm);
    lv_label_set_text(lbl_confirm, config.confirm_text);
    lv_obj_set_style_text_font(lbl_confirm, font14(), 0);
    lv_obj_set_style_text_color(lbl_confirm, config.confirm_text_color, 0);
    lv_obj_center(lbl_confirm);
    
    lv_obj_add_event_cb(btn_confirm, [](lv_event_t *ee) {
        lv_obj_t *modal = (lv_obj_t*)lv_event_get_user_data(ee);
        CallbackData* data = (CallbackData*)lv_obj_get_user_data(modal);
        
        if (data && data->on_confirm) {
            data->on_confirm();
        }
        
        delete data;
        lv_obj_del(modal);
    }, LV_EVENT_CLICKED, modal);
    
    return modal;
}
