#pragma once

struct LocalizedStrings
{
  const char *settings_title;
  const char *brightness;
  const char *language_label;
  const char *close;
  const char *reset_wifi;
  const char *reset;
  const char *reset_confirmation;
  const char *cancel;
  const char *stories_btn;
  const char *stories_title;
  const char *back_to_library;
  const char *the_end;
  const char *no_stories;
  const char *end_next;
  const char *story_font_label;
  const char *loading;
  const char *clear_cache;
  const char *delete_label;
  const char *clear_cache_confirmation;
  const char *online_mode;
  const char *wifi_reset_toast;
  const char *font_opt_small;
  const char *font_opt_normal;
  const char *font_opt_large;
  const char *lang_opt_en;
  const char *lang_opt_pt;
  const char *delete_failed;
  const char *retry_online_fetch;
  const char *no_online_catalog;
  const char *stories_will_appear;
  const char *enable_online_mode;
  const char *cache_cleared_message;
  const char *reset_wifi_confirmation;
  // WiFi SSID screen strings
  const char *wifi_networks_title;
  const char *scan_for_networks;
  const char *scanning;
  const char *scan_again;
  const char *scan_instruction;
  const char *no_networks_found;
  const char *networks_found;
  const char *signal_strong;
  const char *signal_good;
  const char *signal_weak;
  const char *network_open;
  const char *network_secured;
  const char *wifi_password_title;
  const char *wifi_password_placeholder;
  const char *connect_btn;
  const char *connecting_to;
  const char *connected_successfully;
  const char *connection_failed;
  const char *connection_timeout_countdown;
  const char *catalog_url_label;
  const char *edit_btn;
  const char *catalog_url_title;
  const char *catalog_url_description;
  const char *save;
  const char *restore_default;
};

enum Language
{
  LANG_EN = 0,
  LANG_PT = 1
};

extern Language current_language;

const LocalizedStrings *S();
