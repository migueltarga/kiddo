#pragma once

#include <Arduino.h>
#include "story_engine.h"

// Home
void ui_home_screen_show();
void ui_home_screen_set_callbacks(void (*open_settings)(), void (*open_stories)());

// Library (stories list)
using ui_story_home_cb_t = void (*)();
void ui_library_screen_show();
void ui_story_set_home_cb(ui_story_home_cb_t cb);

// Settings
void ui_settings_screen_show();
void ui_settings_screen_reset();

// WiFi SSID Selection
bool ui_wifi_ssid_screen_show();
void ui_wifi_ssid_screen_cleanup();

// Splash
void ui_splash_screen_show(const char *msg = nullptr);

// Story reader
void ui_story_screen_show(const Story_t &story, const String &nodeKey);
void ui_story_screen_refresh();
