// Central configuration for pins, sizes, app meta, and preferences keys
#pragma once

#include <Arduino.h>

// ---------------- App meta ----------------
#ifndef APP_NAME
#define APP_NAME "Kiddo"
#endif
#ifndef APP_VERSION
#define APP_VERSION "v1.0.0"
#endif

// ---------------- Display/Touch pins (CYD typical, verify) ----------------
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define LCD_BACKLIGHT_PIN 21

// ---------------- Audio (DAC) ----------------
// GPIO25 and GPIO26 are the two ESP32 DAC pins. GPIO25 is currently used as XPT2046 CLK.
// To avoid bus contention (touch stopped working after enabling audio), use GPIO26 for audio.
#ifndef AUDIO_DAC_PIN
#define AUDIO_DAC_PIN 26
#endif

// ---------------- Display sizes ----------------
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// LVGL draw buffer size: 1/10th screen, RGB565
#ifndef LV_COLOR_DEPTH
#define LV_COLOR_DEPTH 16
#endif
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))

// ---------------- Touch calibration (adjust if needed) ----------------
// #define TOUCH_SWAP_XY
// #define TOUCH_INVERT_X
// #define TOUCH_INVERT_Y
#ifndef TOUCH_X_MIN
#define TOUCH_X_MIN 200    // raw min X from getPoint().x
#endif
#ifndef TOUCH_X_MAX
#define TOUCH_X_MAX 3700   // raw max X
#endif
#ifndef TOUCH_Y_MIN
#define TOUCH_Y_MIN 240    // raw min Y from getPoint().y
#endif
#ifndef TOUCH_Y_MAX
#define TOUCH_Y_MAX 3800   // raw max Y
#endif
#ifndef TOUCH_ROTATION
#define TOUCH_ROTATION 0
#endif


// ---------------- Preferences keys ----------------
#define PNS "cyd"
#define PK_BRIGHTNESS "brightness"
#define PK_LANG "language"
#define PK_STORY_FONT "storyf"
#define PK_ONLINE_MODE "onmode"
#define PK_CATALOG_URL "caturl"

// ---------------- Remote catalog ----------------
#ifndef REMOTE_CATALOG_URL
#define REMOTE_CATALOG_URL "https://raw.githubusercontent.com/migueltarga/kiddo/refs/heads/main/stories/index.json"
#endif

// Backlight helpers
inline void backlight_init() { pinMode(LCD_BACKLIGHT_PIN, OUTPUT); }
inline void backlight_write(uint8_t v) { analogWrite(LCD_BACKLIGHT_PIN, v); }
