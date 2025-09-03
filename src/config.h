/**
 * @file config.h
 * @brief Central configuration file
 *
 * This file contains all configuration constants including pins, sizes,
 * application metadata, and preferences keys used throughout the application.
 */

// Central configuration for pins, sizes, app meta, and preferences keys
#pragma once

#include <Arduino.h>

// ---------------- App meta ----------------
/** @brief Application name */
#ifndef APP_NAME
#define APP_NAME "Kiddo"
#endif

/** @brief Application version */
#ifndef APP_VERSION
#define APP_VERSION "v1.0.0"
#endif

// ---------------- Display/Touch pins (CYD typical, verify) ----------------
/** @brief Touchscreen interrupt pin */
#define XPT2046_IRQ 36
/** @brief Touchscreen MOSI pin */
#define XPT2046_MOSI 32
/** @brief Touchscreen MISO pin */
#define XPT2046_MISO 39
/** @brief Touchscreen clock pin */
#define XPT2046_CLK 25
/** @brief Touchscreen chip select pin */
#define XPT2046_CS 33
/** @brief LCD backlight control pin */
#define LCD_BACKLIGHT_PIN 21

// ---------------- Audio (DAC) ----------------
// GPIO25 and GPIO26 are the two ESP32 DAC pins. GPIO25 is currently used as XPT2046 CLK.
// To avoid bus contention (touch stopped working after enabling audio), use GPIO26 for audio.
/** @brief Audio DAC output pin */
#ifndef AUDIO_DAC_PIN
#define AUDIO_DAC_PIN 26
#endif

// ---------------- Display sizes ----------------
/** @brief Screen width in pixels */
#define SCREEN_WIDTH 240
/** @brief Screen height in pixels */
#define SCREEN_HEIGHT 320

// LVGL draw buffer size: 1/10th screen, RGB565
/** @brief LVGL color depth */
#ifndef LV_COLOR_DEPTH
#define LV_COLOR_DEPTH 16
#endif
/** @brief LVGL draw buffer size in bytes */
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))

// ---------------- Touch calibration (adjust if needed) ----------------
// #define TOUCH_SWAP_XY
// #define TOUCH_INVERT_X
// #define TOUCH_INVERT_Y
/** @brief Minimum raw X touch value */
#ifndef TOUCH_X_MIN
#define TOUCH_X_MIN 200    // raw min X from getPoint().x
#endif
/** @brief Maximum raw X touch value */
#ifndef TOUCH_X_MAX
#define TOUCH_X_MAX 3700   // raw max X
#endif
/** @brief Minimum raw Y touch value */
#ifndef TOUCH_Y_MIN
#define TOUCH_Y_MIN 240    // raw min Y from getPoint().y
#endif
/** @brief Maximum raw Y touch value */
#ifndef TOUCH_Y_MAX
#define TOUCH_Y_MAX 3800   // raw max Y
#endif
/** @brief Touch rotation */
#ifndef TOUCH_ROTATION
#define TOUCH_ROTATION 0
#endif


// ---------------- Preferences keys ----------------
/** @brief Preferences namespace */
#define PNS "cyd"
/** @brief Brightness preference key */
#define PK_BRIGHTNESS "brightness"
/** @brief Language preference key */
#define PK_LANG "language"
/** @brief Story font preference key */
#define PK_STORY_FONT "storyf"
/** @brief Online mode preference key */
#define PK_ONLINE_MODE "onmode"
/** @brief Catalog URL preference key */
#define PK_CATALOG_URL "caturl"

// ---------------- Remote catalog ----------------
/** @brief Default remote catalog URL */
#ifndef REMOTE_CATALOG_URL
#define REMOTE_CATALOG_URL "https://raw.githubusercontent.com/migueltarga/kiddo/refs/heads/develop/stories/index.json"
#endif

// Backlight helpers
/**
 * @brief Initialize backlight pin
 */
inline void backlight_init() { pinMode(LCD_BACKLIGHT_PIN, OUTPUT); }

/**
 * @brief Write backlight value (0-255)
 * @param v Backlight value
 */
inline void backlight_write(uint8_t v) { analogWrite(LCD_BACKLIGHT_PIN, v); }
