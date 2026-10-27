/**
 * @file confirmation_dialog.cpp
 * @brief Confirmation dialog component implementation
 *
 * This module provides a confirmation dialog with customizable
 * title, message, and action buttons for user confirmations.
 */

#include "ui_components.h"
#include "../fonts.h"
#include "../../styles.h"
#include <lvgl.h>

namespace {

struct CallbackData {
    std::function<void()> on_confirm;
    std::function<void()> on_cancel;
};

/**
 * @brief Close the dialog, then run the chosen callback
 *
 * The callback may clean the whole screen (e.g. navigating away), which
 * would delete the modal under us; deleting it first keeps that safe.
 */
void close_and_run(lv_obj_t *modal, bool confirmed) {
    CallbackData *data = (CallbackData*)lv_obj_get_user_data(modal);
    std::function<void()> callback;
    if (data) {
        callback = confirmed ? data->on_confirm : data->on_cancel;
    }
    lv_obj_delete(modal);
    if (callback) {
        callback();
    }
}

/**
 * @brief Create a dialog button that takes an equal share of the row
 * @param parent Button row
 * @return The button
 */
lv_obj_t *create_button(lv_obj_t *parent) {
    lv_obj_t *btn = lv_btn_create(parent);
    apply_primary_button_style(btn);
    // The two buttons share the row, so long labels can't push them off the card
    lv_obj_set_height(btn, 34);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_style_pad_hor(btn, 6, 0);
    lv_obj_set_style_pad_ver(btn, 0, 0);
    return btn;
}

}

/**
 * @brief Show confirmation dialog
 * @param config Dialog configuration
 * @return Created dialog object
 */
lv_obj_t* ui_confirmation_dialog_show(const ConfirmationConfig& config) {
    // Dimmed backdrop: blocks taps on the screen behind and holds the callbacks
    lv_obj_t *modal = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(modal);
    lv_obj_set_size(modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_40, 0);
    lv_obj_add_flag(modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(modal);
    lv_obj_set_size(card, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, palette::paper(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, palette::brown(), 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_set_style_text_color(card, palette::ink(), 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    // Column sized to its content, so long text grows the dialog instead of clipping
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Title
    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, config.title);
    lv_obj_set_style_text_font(title, font16(), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title, LV_PCT(100));
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    
    if (config.show_close) {
        // On the card's corner, so the title keeps the full width; a child
        // of the backdrop, since the card clips what sticks out of it.
        // Placed once the card has its final size, below.
        lv_obj_t *btn_close = lv_btn_create(modal);
        lv_obj_set_size(btn_close, 28, 28);
        lv_obj_set_style_border_width(btn_close, 2, 0);
        lv_obj_set_style_border_color(btn_close, palette::paper(), 0);
        lv_obj_set_style_bg_color(btn_close, palette::brown(), 0);
        lv_obj_set_style_bg_color(btn_close, palette::brown_dark(), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn_close, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_pad_all(btn_close, 0, 0);
        lv_obj_set_style_shadow_width(btn_close, 0, 0);
        lv_obj_t *lbl_close = lv_label_create(btn_close);
        lv_label_set_text(lbl_close, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(lbl_close, lv_color_white(), 0);
        lv_obj_center(lbl_close);
        lv_obj_add_event_cb(btn_close, [](lv_event_t *ee) {
            lv_obj_delete((lv_obj_t*)lv_event_get_user_data(ee));
        }, LV_EVENT_CLICKED, modal);
        
        // A tap beside the card closes it too (taps on the card don't bubble here)
        lv_obj_add_event_cb(modal, [](lv_event_t *ee) {
            if (lv_event_get_target(ee) == lv_event_get_current_target(ee))
                lv_obj_delete((lv_obj_t*)lv_event_get_current_target(ee));
        }, LV_EVENT_CLICKED, nullptr);
    }
    
    // Message
    lv_obj_t *msg = lv_label_create(card);
    lv_label_set_text(msg, config.message);
    lv_obj_set_style_text_font(msg, font14(), 0);
    lv_obj_set_style_text_color(msg, palette::ink_soft(), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(msg, LV_PCT(100));
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    
    // Button container
    lv_obj_t *btn_container = lv_obj_create(card);
    lv_obj_set_size(btn_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_style_pad_all(btn_container, 0, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_container, 8, 0);
    lv_obj_set_style_pad_top(btn_container, 4, 0);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Cancel button
    lv_obj_t *btn_cancel = create_button(btn_container);
    lv_obj_set_style_bg_color(btn_cancel, palette::brown(), 0);
    lv_obj_set_style_bg_color(btn_cancel, palette::brown_dark(), LV_STATE_PRESSED);
    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, config.cancel_text);
    lv_obj_set_style_text_font(lbl_cancel, font14(), 0);
    lv_obj_center(lbl_cancel);
    
    CallbackData* callback_data = new CallbackData{config.on_confirm, config.on_cancel};
    lv_obj_set_user_data(modal, callback_data);
    // Freed here, so the data cannot leak however the modal goes away
    lv_obj_add_event_cb(modal, [](lv_event_t *ee) {
        delete (CallbackData*)lv_event_get_user_data(ee);
    }, LV_EVENT_DELETE, callback_data);
    
    lv_obj_add_event_cb(btn_cancel, [](lv_event_t *ee) {
        close_and_run((lv_obj_t*)lv_event_get_user_data(ee), false);
    }, LV_EVENT_CLICKED, modal);
    
    // Confirm button
    lv_obj_t *btn_confirm = create_button(btn_container);
    lv_obj_set_style_bg_color(btn_confirm, config.confirm_color, 0);
    lv_obj_t *lbl_confirm = lv_label_create(btn_confirm);
    lv_label_set_text(lbl_confirm, config.confirm_text);
    lv_obj_set_style_text_font(lbl_confirm, font14(), 0);
    lv_obj_set_style_text_color(lbl_confirm, config.confirm_text_color, 0);
    lv_obj_center(lbl_confirm);
    
    lv_obj_add_event_cb(btn_confirm, [](lv_event_t *ee) {
        close_and_run((lv_obj_t*)lv_event_get_user_data(ee), true);
    }, LV_EVENT_CLICKED, modal);
    
    if (config.show_close) {
        lv_obj_t *btn_close = lv_obj_get_child(modal, -1);
        lv_obj_update_layout(card);
        lv_obj_align_to(btn_close, card, LV_ALIGN_TOP_RIGHT, 14, -14);
    }
    
    return modal;
}
