#include "ui_screens.h"
#include "ui/app_ui.h"
#include "ui/fonts.h"
#include "ui/components/ui_components.h"
#include "ui/router.h"
#include "config.h"
#include "styles.h"
#include "i18n.h"
#include <lvgl.h>
#include <Preferences.h>
#include <WiFi.h>
#include <vector>
#include <algorithm>

struct WiFiNetworkInfo {
    String ssid;
    int32_t rssi;
    wifi_auth_mode_t auth_mode;
    bool is_open;
};

static lv_obj_t *ssid_screen = nullptr;
static lv_obj_t *ssid_list = nullptr;
static lv_obj_t *scan_btn = nullptr;
static lv_obj_t *status_label = nullptr;
static std::vector<WiFiNetworkInfo> networks;
static String selected_ssid = "";
static bool scanning = false;

static void ssid_back_clicked(lv_event_t *e);
static void ssid_scan_clicked(lv_event_t *e);
static void ssid_network_clicked(lv_event_t *e);
static void perform_network_scan();
static void refresh_network_list();
static void connect_to_selected_network(const String& ssid, const String& password);

static void ssid_back_clicked(lv_event_t *e) {
    extern bool online_mode;
    extern Preferences prefs;
    if (!WiFi.isConnected()) {
        online_mode = false;
        prefs.putBool(PK_ONLINE_MODE, online_mode);
    }
    
    lv_obj_t *clean_screen = lv_obj_create(NULL);
    lv_scr_load(clean_screen);
    lv_timer_handler();
    
    ssid_screen = nullptr;
    ssid_list = nullptr;
    scan_btn = nullptr;
    status_label = nullptr;
    
    extern void ui_app_before_screen_change();
    ui_app_before_screen_change();
    
    extern void ui_settings_screen_reset();
    ui_settings_screen_reset();
    
    ui_router::show_home();
    lv_timer_handler();
    delay(10);
    ui_router::show_settings();
}

static void ssid_scan_clicked(lv_event_t *e) {
    if (scanning) return;
    
    scanning = true;
    
    if (status_label) {
        lv_label_set_text(status_label, S()->scanning);
    }
    
    if (scan_btn) {
        lv_obj_add_state(scan_btn, LV_STATE_DISABLED);
        lv_label_set_text(lv_obj_get_child(scan_btn, 0), S()->scanning);
    }
    
    lv_timer_create([](lv_timer_t *t) {
        perform_network_scan();
        lv_timer_del(t);
    }, 100, nullptr);
}

static void perform_network_scan() {
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();
    
    networks.clear();
    
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            WiFiNetworkInfo network;
            network.ssid = WiFi.SSID(i);
            network.rssi = WiFi.RSSI(i);
            network.auth_mode = WiFi.encryptionType(i);
            network.is_open = (network.auth_mode == WIFI_AUTH_OPEN);
            
            if (network.ssid.length() > 0) {
                networks.push_back(network);
            }
        }
        
        std::sort(networks.begin(), networks.end(), [](const WiFiNetworkInfo& a, const WiFiNetworkInfo& b) {
            return a.rssi > b.rssi;
        });
        
        auto end = std::unique(networks.begin(), networks.end(), [](const WiFiNetworkInfo& a, const WiFiNetworkInfo& b) {
            return a.ssid == b.ssid;
        });
        networks.erase(end, networks.end());
    }
    
    scanning = false;
    refresh_network_list();
}

static void refresh_network_list() {
    if (!ssid_list) return;
    
    lv_obj_clean(ssid_list);
    
    if (status_label) {
        if (networks.empty()) {
            lv_label_set_text(status_label, S()->no_networks_found);
        } else {
            char status_text[64];
            snprintf(status_text, sizeof(status_text), S()->networks_found, networks.size());
            lv_label_set_text(status_label, status_text);
        }
    }
    
    if (scan_btn) {
        lv_obj_clear_state(scan_btn, LV_STATE_DISABLED);
        lv_label_set_text(lv_obj_get_child(scan_btn, 0), S()->scan_again);
    }
    
    for (size_t i = 0; i < networks.size(); i++) {
        const WiFiNetworkInfo& network = networks[i];
        
        lv_obj_t *btn = lv_btn_create(ssid_list);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_height(btn, 50);
        lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
        lv_obj_set_style_border_color(btn, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_pad_all(btn, 10, 0);
        
        lv_obj_t *ssid_label = lv_label_create(btn);
        lv_label_set_text(ssid_label, network.ssid.c_str());
        lv_obj_set_style_text_font(ssid_label, font14(), 0);
        lv_obj_set_style_text_color(ssid_label, lv_color_black(), 0);
        lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 0, 0);
        
        String info_text = "";
        
        if (network.rssi > -50) {
            info_text += S()->signal_strong;
        } else if (network.rssi > -70) {
            info_text += S()->signal_good;
        } else {
            info_text += S()->signal_weak;
        }
        
        info_text += " • ";
        
        if (network.is_open) {
            info_text += S()->network_open;
        } else {
            info_text += S()->network_secured;
        }
        
        lv_obj_t *info_label = lv_label_create(btn);
        lv_label_set_text(info_label, info_text.c_str());
        lv_obj_set_style_text_font(info_label, font12(), 0);
        lv_obj_set_style_text_color(info_label, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_align(info_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        
        lv_obj_add_event_cb(btn, ssid_network_clicked, LV_EVENT_CLICKED, (void*)i);
    }
}

static void ssid_network_clicked(lv_event_t *e) {
    size_t index = (size_t)lv_event_get_user_data(e);
    if (index >= networks.size()) return;
    
    const WiFiNetworkInfo& network = networks[index];
    selected_ssid = network.ssid;
    
    if (network.is_open) {
        connect_to_selected_network(selected_ssid, "");
    } else {
        static TextInputConfig *password_config = new TextInputConfig();
        password_config->title = S()->wifi_password_title;
        
        static String desc_string;
        desc_string = String("Enter password for ") + selected_ssid;
        password_config->description = desc_string.c_str();
        password_config->initial_text = "";
        password_config->placeholder_text = S()->wifi_password_placeholder;
        password_config->save_text = S()->connect_btn;
        password_config->cancel_text = S()->cancel;
        password_config->default_text = nullptr;
        
        password_config->on_save = [](const char* password) {
            connect_to_selected_network(selected_ssid, String(password));
        };
        
        password_config->on_cancel = []() {};
        password_config->on_default = nullptr;
        
        ui_text_input_dialog_show(*password_config);
    }
}

static void connect_to_selected_network(const String& ssid, const String& password) {
    if (status_label) {
        char status_text[128];
        snprintf(status_text, sizeof(status_text), S()->connecting_to, ssid.c_str());
        lv_label_set_text(status_label, status_text);
    }
    
    WiFi.mode(WIFI_STA);
    
    const char* password_ptr = nullptr;
    if (password.length() > 0) {
        password_ptr = password.c_str();
    }
    
    WiFi.begin(ssid.c_str(), password_ptr);
    
    static int connection_attempts = 0;
    connection_attempts = 0;
    
    lv_timer_t *connection_timer = lv_timer_create([](lv_timer_t *t) {
        connection_attempts++;
        lv_timer_handler();
        
        if (WiFi.status() == WL_CONNECTED) {
            extern bool online_mode;
            extern Preferences prefs;
            online_mode = true;
            prefs.putBool(PK_ONLINE_MODE, online_mode);
            
            extern bool wifi_connected;
            wifi_connected = true;
            
            if (status_label) {
                lv_label_set_text(status_label, S()->connected_successfully);
            }
            
            lv_timer_create([](lv_timer_t *return_timer) {
                extern void ui_app_before_screen_change();
                ui_app_before_screen_change();
                
                extern void ui_settings_screen_reset();
                ui_settings_screen_reset();
                
                ui_router::show_settings();
                lv_timer_del(return_timer);
            }, 1500, nullptr);
            
            lv_timer_del(t);
            
        } else if (connection_attempts >= 20) {
            if (status_label) {
                lv_label_set_text(status_label, S()->connection_failed);
            }
            
            WiFi.disconnect();
            lv_timer_del(t);
        } else {
            if (status_label && (connection_attempts % 4 == 0)) {
                char status_text[128];
                snprintf(status_text, sizeof(status_text), S()->connection_timeout_countdown, selected_ssid.c_str(), 20 - connection_attempts);
                lv_label_set_text(status_label, status_text);
            }
        }
    }, 500, nullptr);
    
    lv_timer_set_repeat_count(connection_timer, -1);
}

bool ui_wifi_ssid_screen_show() {
    networks.clear();
    selected_ssid = "";
    scanning = false;
    
    ssid_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ssid_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(ssid_screen, LV_OPA_COVER, 0);
    
    ui_header_config_t config = ui_header_config_default(S()->wifi_networks_title, ssid_back_clicked);
    lv_obj_t *header = ui_header_create(ssid_screen, &config);
    
    lv_obj_t *main_container = lv_obj_create(ssid_screen);
    lv_obj_set_size(main_container, SCREEN_WIDTH, SCREEN_HEIGHT - 38);
    lv_obj_align(main_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(main_container, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(main_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(main_container, 0, 0);
    lv_obj_set_style_pad_all(main_container, 16, 0);
    lv_obj_clear_flag(main_container, LV_OBJ_FLAG_SCROLLABLE);
    
    scan_btn = lv_btn_create(main_container);
    lv_obj_set_size(scan_btn, LV_PCT(100), 36);
    lv_obj_set_style_bg_color(scan_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t *scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, S()->scan_for_networks);
    lv_obj_set_style_text_font(scan_label, font14(), 0);
    lv_obj_set_style_text_color(scan_label, lv_color_white(), 0);
    lv_obj_center(scan_label);
    
    lv_obj_add_event_cb(scan_btn, ssid_scan_clicked, LV_EVENT_CLICKED, nullptr);
    
    status_label = lv_label_create(main_container);
    lv_label_set_text(status_label, S()->scan_instruction);
    lv_obj_set_style_text_font(status_label, font12(), 0);
    lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_width(status_label, LV_PCT(100));
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 0, 45);
    
    ssid_list = lv_obj_create(main_container);
    lv_obj_set_size(ssid_list, LV_PCT(100), SCREEN_HEIGHT - 38 - 75);
    lv_obj_align(ssid_list, LV_ALIGN_TOP_LEFT, 0, 70);
    lv_obj_set_style_bg_opa(ssid_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ssid_list, 0, 0);
    lv_obj_set_style_pad_all(ssid_list, 0, 0);
    lv_obj_set_style_pad_row(ssid_list, 8, 0);
    lv_obj_set_flex_flow(ssid_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(ssid_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ssid_list, LV_SCROLLBAR_MODE_ACTIVE);
    
    lv_scr_load(ssid_screen);
    return true;
}

void ui_wifi_ssid_screen_cleanup() {
    if (ssid_screen) {
        lv_obj_del(ssid_screen);
        ssid_screen = nullptr;
    }
    
    ssid_list = nullptr;
    scan_btn = nullptr;
    status_label = nullptr;
    
    networks.clear();
    selected_ssid = "";
    scanning = false;
}
