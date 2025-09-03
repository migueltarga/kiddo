/**
 * @file ui_screens.h
 * @brief UI screen management functions
 *
 * This file contains function declarations for managing different UI screens
 * including home, library, settings, story, and other application screens.
 */

#pragma once

#include <Arduino.h>
#include "story_engine.h"

// Home
/**
 * @brief Show home screen
 */
void ui_home_screen_show();

/**
 * @brief Set home screen callback functions
 * @param open_settings Callback for opening settings
 * @param open_stories Callback for opening story library
 */
void ui_home_screen_set_callbacks(void (*open_settings)(), void (*open_stories)());

// Library (stories list)
/**
 * @brief Callback type for home button in story screens
 */
using ui_story_home_cb_t = void (*)();

/**
 * @brief Show story library screen
 */
void ui_library_screen_show();

/**
 * @brief Set home callback for story screens
 * @param cb Home callback function
 */
void ui_story_set_home_cb(ui_story_home_cb_t cb);

// Settings
/**
 * @brief Show settings screen
 */
void ui_settings_screen_show();

/**
 * @brief Reset settings screen state
 */
void ui_settings_screen_reset();

// WiFi SSID Selection
/**
 * @brief Show WiFi SSID selection screen
 * @return True if screen was shown successfully
 */
bool ui_wifi_ssid_screen_show();

/**
 * @brief Clean up WiFi SSID screen resources
 */
void ui_wifi_ssid_screen_cleanup();

// Splash
/**
 * @brief Show splash screen
 * @param msg Optional loading message
 */
void ui_splash_screen_show(const char *msg = nullptr);

// Story reader
/**
 * @brief Show story reading screen
 * @param story Story to display
 * @param nodeKey Starting node key
 */
void ui_story_screen_show(const Story_t &story, const String &nodeKey);

/**
 * @brief Refresh story screen display
 */
void ui_story_screen_refresh();
