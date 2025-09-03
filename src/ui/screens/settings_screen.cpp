/**
 * @file settings_screen.cpp
 * @brief Settings screen implementation
 *
 * This module provides the settings screen with controls for
 * brightness, language, font size, WiFi management, and
 * catalog configuration.
 */

#include "ui_screens.h"
#include <Preferences.h>
#include "config.h"
#include "i18n.h"
#include "styles.h"
#include "remote_catalog.h"
#include "story_engine.h"
#include "file_system.h"
#include "ui/components/ui_components.h"
#include "ui/fonts.h"
#include "audio.h"
#include "ui/router.h"
#include "ui/app_ui.h"

extern bool g_remote_fetch_done;
extern bool g_remote_fetch_failed;
extern Preferences prefs;
extern uint8_t brightness;
extern uint8_t story_font_scale;
extern bool online_mode;
extern Language current_language;

static lv_obj_t *settings_root = nullptr;

/**
 * @brief Close settings screen
 * @param e LVGL event (unused)
 */
static void close_settings(lv_event_t *)
{
    if (settings_root)
    {
        lv_obj_del(settings_root);
        settings_root = nullptr;
    }
    ui_app_refresh_home_labels();
    ui_router::show_home();
}
static void wifi_reset_cb(lv_event_t *) {
    auto *s = S();
    
    ConfirmationConfig config;
    config.title = s->reset_wifi;
    config.message = s->reset_wifi_confirmation;
    config.confirm_text = s->reset;
    config.cancel_text = s->cancel;
    config.confirm_color = lv_palette_main(LV_PALETTE_RED);
    config.confirm_text_color = lv_color_white();
    config.on_confirm = []() {
        
        if (online_mode) {
            online_mode = false;
            Preferences prefs;
            prefs.begin("kiddo", false);
            prefs.putBool(PK_ONLINE_MODE, online_mode);
            prefs.end();
        }
        
        extern bool wifi_connected;
        wifi_connected = false;
        
        auto *s = S();
        
        lv_obj_t *toast = lv_obj_create(lv_scr_act());
        lv_obj_set_size(toast, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(toast, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_set_style_bg_opa(toast, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(toast, 0, 0);
        lv_obj_set_style_pad_all(toast, 16, 0);
        lv_obj_set_style_radius(toast, 0, 0);
        
        lv_obj_t *toast_msg = lv_label_create(toast);
        lv_label_set_text(toast_msg, s->wifi_reset_toast);
        lv_obj_set_style_text_font(toast_msg, font14(), 0);
        lv_obj_set_style_text_color(toast_msg, lv_color_white(), 0);
        lv_obj_set_style_text_align(toast_msg, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(toast_msg);
        
        lv_timer_create([](lv_timer_t *t) {
            if (t->user_data)
                lv_obj_del((lv_obj_t *)t->user_data);
            lv_timer_del(t);
            
            if (settings_root) {
                lv_obj_del(settings_root);
                settings_root = nullptr;
            }
            ui_settings_screen_show();
        }, 1400, toast);
    };
    config.on_cancel = nullptr;
    
    ui_confirmation_dialog_show(config);
}
static void rebuild_lang() {
    ui_app_refresh_home_labels();
    if (settings_root) {
        lv_obj_del(settings_root);
        settings_root = nullptr;
    }
    ui_settings_screen_show();
}

void ui_settings_screen_reset() {
    if (settings_root) {
        lv_obj_del(settings_root);
        settings_root = nullptr;
    }
}

void ui_settings_screen_show()
{
    if (settings_root)
    {
        lv_obj_move_foreground(settings_root);
        return;
    }
    ensure_compact_style();
    auto *s = S();
    settings_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(settings_root);
    lv_obj_set_size(settings_root, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(settings_root, lv_color_hex(0xf5f1e6), 0);
    lv_obj_set_style_bg_opa(settings_root, LV_OPA_COVER, 0);
    
    auto on_close_clicked = [](lv_event_t *e) { close_settings(e); };
    ui_header_config_t config = ui_header_config_default(s->settings_title, on_close_clicked);
    lv_obj_t *header = ui_header_create(settings_root, &config);
    lv_obj_t *panel = lv_obj_create(settings_root);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, SCREEN_WIDTH, SCREEN_HEIGHT - 38);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(panel, 4, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    
    static int32_t cols[] = {100, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t rows[] = {
        LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
        LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(panel, cols, rows);
    lv_obj_set_style_pad_row(panel, 8, 0);
    lv_obj_set_style_pad_column(panel, 6, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    
    int r = 0;
    lv_obj_t *brightness_label = lv_label_create(panel);
    lv_label_set_text(brightness_label, s->brightness);
    lv_obj_set_style_text_font(brightness_label, font14(), 0);
    lv_obj_set_style_min_height(brightness_label, 22, 0);
    lv_obj_set_grid_cell(brightness_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, r, 1);
    
    lv_obj_t *sl = lv_slider_create(panel);
    lv_slider_set_range(sl, 10, 255);
    lv_slider_set_value(sl, brightness, LV_ANIM_OFF);
    lv_obj_set_style_height(sl, 12, 0);
    lv_obj_set_style_pad_all(sl, 1, 0);
    lv_obj_add_event_cb(sl, [](lv_event_t *e) { 
        auto *o = (lv_obj_t*)lv_event_get_target(e);
        brightness = (uint8_t)lv_slider_get_value(o);
        backlight_write(brightness);
        prefs.putUChar(PK_BRIGHTNESS, brightness);
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_grid_cell(sl, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *lbl_font = lv_label_create(panel);
    lv_label_set_text(lbl_font, s->story_font_label);
    lv_obj_set_style_text_font(lbl_font, font14(), 0);
    lv_obj_set_style_min_height(lbl_font, 22, 0);
    lv_obj_set_grid_cell(lbl_font, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, r, 1);
    
    lv_obj_t *dd_font = lv_dropdown_create(panel);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s\n%s\n%s", s->font_opt_small, s->font_opt_normal, s->font_opt_large);
    lv_dropdown_set_options(dd_font, buf);
    if (story_font_scale > 2) story_font_scale = 1;
    lv_dropdown_set_selected(dd_font, story_font_scale);
    lv_obj_set_height(dd_font, 36);
    lv_obj_set_style_pad_all(dd_font, 4, 0);
    lv_obj_add_event_cb(dd_font, [](lv_event_t *e) {
        uint16_t sel = lv_dropdown_get_selected((lv_obj_t *)lv_event_get_target(e));
        if (sel > 2) sel = 1;
        story_font_scale = sel;
        prefs.putUChar(PK_STORY_FONT, story_font_scale);
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_grid_cell(dd_font, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *lbl_lang = lv_label_create(panel);
    lv_label_set_text(lbl_lang, s->language_label);
    lv_obj_set_style_text_font(lbl_lang, font14(), 0);
    lv_obj_set_style_min_height(lbl_lang, 22, 0);
    lv_obj_set_grid_cell(lbl_lang, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, r, 1);
    
    lv_obj_t *dd_lang = lv_dropdown_create(panel);
    char lang_buf[64];
    snprintf(lang_buf, sizeof(lang_buf), "%s\n%s", s->lang_opt_en, s->lang_opt_pt);
    lv_dropdown_set_options(dd_lang, lang_buf);
    lv_dropdown_set_selected(dd_lang, (current_language == LANG_EN) ? 0 : 1);
    lv_obj_set_height(dd_lang, 36);
    lv_obj_set_style_pad_all(dd_lang, 4, 0);
    lv_obj_add_event_cb(dd_lang, [](lv_event_t *e) {
        uint16_t sel = lv_dropdown_get_selected((lv_obj_t *)lv_event_get_target(e));
        Language new_lang = sel == 0 ? LANG_EN : LANG_PT;
        current_language = new_lang;
        prefs.putUInt(PK_LANG, (uint32_t)current_language);
        
        // Reload local stories with new language filter
        story::loadFromFS();
        
        remote_catalog::invalidate();
        extern bool g_remote_fetch_done;
        extern bool g_remote_fetch_failed;
        g_remote_fetch_done = false;
        g_remote_fetch_failed = false;
        
        rebuild_lang();
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_grid_cell(dd_lang, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *lbl_on = lv_label_create(panel);
    lv_label_set_text(lbl_on, s->online_mode);
    lv_obj_set_style_text_font(lbl_on, font14(), 0);
    lv_obj_set_style_min_height(lbl_on, 22, 0);
    lv_obj_set_grid_cell(lbl_on, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, r, 1);
    
    lv_obj_t *sw = lv_switch_create(panel);
    if (online_mode) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, [](lv_event_t *e) {
        lv_obj_t *sw = (lv_obj_t*)lv_event_get_target(e);
        bool new_state = lv_obj_has_state(sw, LV_STATE_CHECKED);
        
        if (new_state && !online_mode) {
            extern bool wifi_connected;
            if (!wifi_connected) {
                ui_wifi_ssid_screen_show();
                return;
            } else {
                online_mode = true;
                prefs.putBool(PK_ONLINE_MODE, online_mode);
            }
        } else if (!new_state && online_mode) {
            online_mode = false;
            prefs.putBool(PK_ONLINE_MODE, online_mode);
        }
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_grid_cell(sw, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *lbl_catalog = lv_label_create(panel);
    lv_label_set_text(lbl_catalog, s->catalog_url_label);
    lv_obj_set_style_text_font(lbl_catalog, font14(), 0);
    lv_obj_set_style_min_height(lbl_catalog, 22, 0);
    lv_obj_set_grid_cell(lbl_catalog, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, r, 1);
    
    lv_obj_t *btn_catalog = lv_btn_create(panel);
    ui_add_click_sound(btn_catalog);
    apply_secondary_button_style(btn_catalog);
    lv_obj_set_size(btn_catalog, 60, 26);
    lv_obj_set_style_pad_all(btn_catalog, 4, 0);
    lv_obj_add_event_cb(btn_catalog, [](lv_event_t *e) {
        auto *s = S();
        
        String current_url = remote_catalog::getCatalogUrl();
        
        static TextInputConfig *input_config = new TextInputConfig();
        input_config->title = s->catalog_url_title;
        input_config->description = s->catalog_url_description;
        input_config->initial_text = current_url.c_str();
        input_config->save_text = s->save;
        input_config->cancel_text = s->cancel;
        input_config->default_text = s->restore_default;
        input_config->on_save = [](const char* new_url) {
            remote_catalog::setCatalogUrl(String(new_url));
            
            lv_obj_t *toast = lv_obj_create(lv_scr_act());
            lv_obj_set_size(toast, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_obj_set_style_bg_color(toast, lv_palette_main(LV_PALETTE_GREEN), 0);
            lv_obj_set_style_bg_opa(toast, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(toast, 0, 0);
            lv_obj_set_style_pad_all(toast, 16, 0);
            lv_obj_set_style_radius(toast, 0, 0);
            
            lv_obj_t *toast_msg = lv_label_create(toast);
            lv_label_set_text(toast_msg, "Catalog URL updated");
            lv_obj_set_style_text_font(toast_msg, font14(), 0);
            lv_obj_set_style_text_color(toast_msg, lv_color_white(), 0);
            lv_obj_set_style_text_align(toast_msg, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(toast_msg);
            
            lv_timer_create([](lv_timer_t *t) {
                if (t->user_data)
                    lv_obj_del((lv_obj_t*)t->user_data);
                lv_timer_del(t);
            }, 2000, toast);
        };
        input_config->on_cancel = nullptr;
        input_config->on_default = []() {
            remote_catalog::setCatalogUrl("");
            
            lv_obj_t *toast = lv_obj_create(lv_scr_act());
            lv_obj_set_size(toast, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_obj_set_style_bg_color(toast, lv_palette_main(LV_PALETTE_BLUE), 0);
            lv_obj_set_style_bg_opa(toast, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(toast, 0, 0);
            lv_obj_set_style_pad_all(toast, 16, 0);
            lv_obj_set_style_radius(toast, 0, 0);
            
            lv_obj_t *toast_msg = lv_label_create(toast);
            lv_label_set_text(toast_msg, "Using default catalog URL");
            lv_obj_set_style_text_font(toast_msg, font14(), 0);
            lv_obj_set_style_text_color(toast_msg, lv_color_white(), 0);
            lv_obj_set_style_text_align(toast_msg, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(toast_msg);
            
            lv_timer_create([](lv_timer_t *t) {
                if (t->user_data)
                    lv_obj_del((lv_obj_t*)t->user_data);
                lv_timer_del(t);
            }, 2000, toast);
        };
        
        ui_text_input_dialog_show(*input_config);
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t *catalog_label = lv_label_create(btn_catalog);
    lv_label_set_text(catalog_label, s->edit_btn);
    lv_obj_center(catalog_label);
    lv_obj_set_grid_cell(btn_catalog, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *btn_clear = lv_btn_create(panel);
    ui_add_click_sound(btn_clear);
    apply_secondary_button_style(btn_clear);
    lv_obj_set_style_min_height(btn_clear, 26, 0);
    lv_obj_set_style_bg_color(btn_clear, lv_palette_main(LV_PALETTE_YELLOW), 0);
    lv_obj_set_style_text_color(btn_clear, lv_color_black(), 0);
    lv_obj_add_event_cb(btn_clear, [](lv_event_t *e) {
        auto *s = S();
        
        ConfirmationConfig config;
        config.title = s->clear_cache;
        config.message = s->clear_cache_confirmation;
        config.confirm_text = s->delete_label;
        config.cancel_text = s->cancel;
        config.confirm_color = lv_palette_main(LV_PALETTE_RED);
        config.confirm_text_color = lv_color_white();
        config.on_confirm = []() {
            FileSystem::clearAll();
            story::loadFromFS();
            auto *s = S();
            
            lv_obj_t *toast = lv_obj_create(lv_scr_act());
            lv_obj_set_size(toast, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_obj_set_style_bg_color(toast, lv_palette_main(LV_PALETTE_GREEN), 0);
            lv_obj_set_style_bg_opa(toast, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(toast, 0, 0);
            lv_obj_set_style_pad_all(toast, 16, 0);
            lv_obj_set_style_radius(toast, 0, 0);
            
            lv_obj_t *toast_msg = lv_label_create(toast);
            lv_label_set_text(toast_msg, s->cache_cleared_message);
            lv_obj_set_style_text_font(toast_msg, font14(), 0);
            lv_obj_set_style_text_color(toast_msg, lv_color_white(), 0);
            lv_obj_set_style_text_align(toast_msg, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(toast_msg);
            
            lv_timer_create([](lv_timer_t *t) {
                if (t->user_data)
                    lv_obj_del((lv_obj_t*)t->user_data);
                lv_timer_del(t);
            }, 2000, toast);
        };
        config.on_cancel = nullptr;
        
        ui_confirmation_dialog_show(config);
    }, LV_EVENT_CLICKED, nullptr);
    lv_label_set_text(lv_label_create(btn_clear), s->clear_cache);
    lv_obj_set_grid_cell(btn_clear, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *btn_wifi = lv_btn_create(panel);
    ui_add_click_sound(btn_wifi);
    apply_secondary_button_style(btn_wifi);
    lv_obj_set_style_min_height(btn_wifi, 26, 0);
    lv_obj_add_event_cb(btn_wifi, wifi_reset_cb, LV_EVENT_CLICKED, nullptr);
    lv_label_set_text(lv_label_create(btn_wifi), s->reset_wifi);
    lv_obj_set_style_bg_color(btn_wifi, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_color(btn_wifi, lv_color_white(), 0);
    lv_obj_set_grid_cell(btn_wifi, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, r, 1);
    r++;
}
