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
#include "story_progress.h"
#include "ui/components/ui_components.h"
#include "ui/fonts.h"
#include "audio.h"
#include "ui/router.h"
#include "ui/app_ui.h"
#include <WiFi.h>

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
    config.confirm_color = palette::danger();
    config.confirm_text_color = lv_color_white();
    config.on_confirm = []() {
        online_mode = false;
        prefs.putBool(PK_ONLINE_MODE, online_mode);
        
        // Drop the connection and erase the stored credentials
        WiFi.disconnect(true, true);
        extern bool wifi_connected;
        wifi_connected = false;
        
        auto *s = S();
        
        lv_obj_t *toast = lv_obj_create(lv_scr_act());
        lv_obj_set_size(toast, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(toast, palette::accent(), 0);
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
        
        // The toast promises a reboot; give it time to be read first
        lv_timer_create([](lv_timer_t *t) {
            ESP.restart();
        }, 1400, nullptr);
    };
    config.on_cancel = nullptr;
    
    ui_confirmation_dialog_show(config);
}
static void rebuild_settings() {
    ui_app_refresh_home_labels();
    if (settings_root) {
        lv_obj_del(settings_root);
        settings_root = nullptr;
    }
    ui_settings_screen_show();
}

/**
 * @brief Match a settings dropdown to the theme
 * @param dd Dropdown
 */
/** @brief Catalog URLs in the order the picker lists them */
static std::vector<String> g_catalog_urls;

/**
 * @brief Show a short message at the bottom of the screen
 * @param text Message
 */
static void show_toast(const char *text)
{
    lv_obj_t *toast = lv_obj_create(lv_scr_act());
    lv_obj_set_size(toast, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(toast, palette::accent(), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(toast, 0, 0);
    lv_obj_set_style_pad_all(toast, 16, 0);
    lv_obj_set_style_radius(toast, 0, 0);
    
    lv_obj_t *toast_msg = lv_label_create(toast);
    lv_label_set_text(toast_msg, text);
    lv_obj_set_style_text_font(toast_msg, font14(), 0);
    lv_obj_set_style_text_color(toast_msg, lv_color_white(), 0);
    lv_obj_set_style_text_align(toast_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(toast_msg);
    
    lv_timer_create([](lv_timer_t *t) {
        if (t->user_data)
            lv_obj_del((lv_obj_t*)t->user_data);
        lv_timer_del(t);
    }, 2000, toast);
}

/**
 * @brief Name a catalog in the picker
 *
 * Uses the name from the catalog's own index once it has been fetched;
 * until then the URL's owner is enough to tell catalogs apart.
 */
static String catalog_label_for(const remote_catalog::SavedCatalog &c)
{
    if (remote_catalog::isDefaultCatalog(c.url))
        return S()->catalog_default_name;
    if (c.name.length())
        return c.name;
    int scheme = c.url.indexOf("://");
    String rest = scheme >= 0 ? c.url.substring(scheme + 3) : c.url;
    int slash = rest.indexOf('/');
    String host = slash > 0 ? rest.substring(0, slash) : rest;
    String path = slash > 0 ? rest.substring(slash + 1) : "";
    int owner_end = path.indexOf('/');
    String owner = owner_end > 0 ? path.substring(0, owner_end) : path;
    if (host.indexOf("github") >= 0 && owner.length())
        return owner + " (GitHub)";
    return host;
}

/**
 * @brief After the catalog changed: refetch it, confirm, and maybe redraw
 * @param toast Message to show
 * @param rebuild Redraw Settings so the picker lists a newly typed catalog
 */
static void catalog_changed(const char *toast, bool rebuild)
{
    ui_library_catalog_changed();
    if (!rebuild) {
        show_toast(toast);
        return;
    }
    // Later, not from inside the dialog's own event that got us here
    lv_async_call([](void *text) {
        rebuild_settings();
        show_toast((const char *)text);
    }, (void *)toast);
}

static void style_dropdown(lv_obj_t *dd)
{
    lv_obj_set_height(dd, 32);
    lv_obj_set_style_pad_ver(dd, 6, 0);
    // The app font has the accented letters ("Português"); the default doesn't
    lv_obj_set_style_text_font(dd, font14(), 0);
    lv_obj_set_style_pad_hor(dd, 8, 0);
    lv_obj_set_style_radius(dd, 8, 0);
    lv_obj_set_style_border_color(dd, palette::parchment_dark(), 0);
    lv_obj_set_style_text_color(dd, palette::ink(), 0);
    lv_obj_t *list = lv_dropdown_get_list(dd);
    lv_obj_set_style_text_font(list, font14(), 0);
    lv_obj_set_style_bg_color(list, palette::primary(), LV_PART_SELECTED | LV_STATE_CHECKED);
}

void ui_settings_screen_show()
{
    if (settings_root)
    {
        lv_obj_move_foreground(settings_root);
        return;
    }
    auto *s = S();
    settings_root = lv_obj_create(lv_scr_act());
    // Forget the pointer however the panel is deleted (e.g. with its screen)
    lv_obj_add_event_cb(settings_root, [](lv_event_t *e) {
        if (lv_event_get_target(e) == settings_root)
            settings_root = nullptr;
    }, LV_EVENT_DELETE, nullptr);
    lv_obj_remove_style_all(settings_root);
    lv_obj_set_size(settings_root, SCREEN_WIDTH, SCREEN_HEIGHT);
    apply_screen_bg(settings_root);
    
    auto on_close_clicked = [](lv_event_t *e) { close_settings(e); };
    ui_header_config_t config = ui_header_config_default(s->settings_title, on_close_clicked);
    lv_obj_t *header = ui_header_create(settings_root, &config);
    // White card below the header; scrolls if the rows ever outgrow it
    lv_obj_t *panel = lv_obj_create(settings_root);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 38 - 8);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(panel, palette::card(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(panel, 12, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, palette::parchment_dark(), 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_set_style_text_color(panel, palette::ink(), 0);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_AUTO);
    apply_scrollbar_style(panel);
    
    static int32_t cols[] = {88, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t rows[] = {
        LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT,
        LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(panel, cols, rows);
    lv_obj_set_style_pad_row(panel, 8, 0);
    lv_obj_set_style_pad_column(panel, 6, 0);
    
    int r = 0;
    lv_obj_t *brightness_label = lv_label_create(panel);
    lv_label_set_text(brightness_label, s->brightness);
    lv_obj_set_style_text_font(brightness_label, font14(), 0);
    lv_obj_set_style_min_height(brightness_label, 22, 0);
    lv_obj_set_grid_cell(brightness_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, r, 1);
    
    lv_obj_t *sl = lv_slider_create(panel);
    lv_slider_set_range(sl, 10, 255);
    lv_slider_set_value(sl, brightness, LV_ANIM_OFF);
    lv_obj_set_style_height(sl, 10, 0);
    lv_obj_set_style_pad_all(sl, 1, 0);
    lv_obj_set_style_bg_color(sl, palette::parchment(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, palette::primary(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, palette::primary_dark(), LV_PART_KNOB);
    lv_obj_add_event_cb(sl, [](lv_event_t *e) { 
        auto *o = (lv_obj_t*)lv_event_get_target(e);
        brightness = (uint8_t)lv_slider_get_value(o);
        backlight_write(brightness);
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    // Persist once per drag rather than on every step, to spare the flash
    lv_obj_add_event_cb(sl, [](lv_event_t *e) {
        prefs.putUChar(PK_BRIGHTNESS, brightness);
    }, LV_EVENT_RELEASED, nullptr);
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
    style_dropdown(dd_font);
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
    style_dropdown(dd_lang);
    lv_obj_add_event_cb(dd_lang, [](lv_event_t *e) {
        uint16_t sel = lv_dropdown_get_selected((lv_obj_t *)lv_event_get_target(e));
        Language new_lang = sel == 0 ? LANG_EN : LANG_PT;
        current_language = new_lang;
        prefs.putUInt(PK_LANG, (uint32_t)current_language);
        
        // Reload local stories with new language filter
        story::loadFromFS();
        
        ui_library_catalog_changed();
        
        rebuild_settings();
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_grid_cell(dd_lang, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *lbl_on = lv_label_create(panel);
    lv_label_set_text(lbl_on, s->online_mode);
    lv_obj_set_style_text_font(lbl_on, font14(), 0);
    lv_obj_set_style_min_height(lbl_on, 22, 0);
    lv_obj_set_grid_cell(lbl_on, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, r, 1);
    
    lv_obj_t *sw = lv_switch_create(panel);
    lv_obj_set_style_bg_color(sw, palette::parchment(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, palette::primary(), LV_PART_INDICATOR | LV_STATE_CHECKED);
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
    lv_obj_set_size(btn_catalog, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_pad_ver(btn_catalog, 0, 0);
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
            catalog_changed(S()->catalog_url_updated, true);
        };
        input_config->on_cancel = nullptr;
        input_config->on_default = []() {
            remote_catalog::setCatalogUrl("");
            catalog_changed(S()->catalog_url_reset, true);
        };
        
        ui_text_input_dialog_show(*input_config);
    }, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t *catalog_label = lv_label_create(btn_catalog);
    lv_label_set_text(catalog_label, LV_SYMBOL_EDIT " ");
    lv_label_ins_text(catalog_label, LV_LABEL_POS_LAST, s->edit_btn);
    lv_obj_center(catalog_label);
    lv_obj_set_grid_cell(btn_catalog, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    // Every catalog used on this device, so switching never means typing a URL
    g_catalog_urls.clear();
    String catalog_opts;
    uint16_t catalog_sel = 0;
    String current_catalog = remote_catalog::getCatalogUrl();
    for (const auto &c : remote_catalog::savedCatalogs()) {
        if (c.url == current_catalog)
            catalog_sel = g_catalog_urls.size();
        String label = catalog_label_for(c);
        label.replace("\n", " ");
        if (catalog_opts.length())
            catalog_opts += "\n";
        catalog_opts += label;
        g_catalog_urls.push_back(c.url);
    }
    lv_obj_t *dd_catalog = lv_dropdown_create(panel);
    lv_dropdown_set_options(dd_catalog, catalog_opts.c_str());
    lv_dropdown_set_selected(dd_catalog, catalog_sel);
    style_dropdown(dd_catalog);
    lv_obj_add_event_cb(dd_catalog, [](lv_event_t *e) {
        uint16_t sel = lv_dropdown_get_selected((lv_obj_t *)lv_event_get_target(e));
        if (sel >= g_catalog_urls.size())
            return;
        const String &url = g_catalog_urls[sel];
        remote_catalog::setCatalogUrl(remote_catalog::isDefaultCatalog(url) ? String("") : url);
        catalog_changed(S()->catalog_url_updated, false);
    }, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_set_grid_cell(dd_catalog, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *btn_clear = lv_btn_create(panel);
    ui_add_click_sound(btn_clear);
    apply_outline_button_style(btn_clear, palette::brown());
    lv_obj_set_height(btn_clear, 32);
    lv_obj_add_event_cb(btn_clear, [](lv_event_t *e) {
        auto *s = S();
        
        ConfirmationConfig config;
        config.title = s->clear_cache;
        config.message = s->clear_cache_confirmation;
        config.confirm_text = s->delete_label;
        config.cancel_text = s->cancel;
        config.confirm_color = palette::danger();
        config.confirm_text_color = lv_color_white();
        config.on_confirm = []() {
            FileSystem::clearAll();
            story_progress::clearAll();
            story::loadFromFS();
            auto *s = S();
            
            lv_obj_t *toast = lv_obj_create(lv_scr_act());
            lv_obj_set_size(toast, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_obj_set_style_bg_color(toast, palette::accent(), 0);
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
    lv_obj_t *clear_label = lv_label_create(btn_clear);
    lv_label_set_text_fmt(clear_label, LV_SYMBOL_TRASH "  %s", s->clear_cache);
    lv_obj_center(clear_label);
    lv_obj_set_grid_cell(btn_clear, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, r, 1);
    r++;

    lv_obj_t *btn_wifi = lv_btn_create(panel);
    ui_add_click_sound(btn_wifi);
    apply_outline_button_style(btn_wifi, palette::danger());
    lv_obj_set_height(btn_wifi, 32);
    lv_obj_add_event_cb(btn_wifi, wifi_reset_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *wifi_label = lv_label_create(btn_wifi);
    lv_label_set_text_fmt(wifi_label, LV_SYMBOL_WIFI "  %s", s->reset_wifi);
    lv_obj_center(wifi_label);
    lv_obj_set_grid_cell(btn_wifi, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, r, 1);
    r++;
}
