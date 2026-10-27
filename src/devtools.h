/**
 * @file devtools.h
 * @brief Serial commands for driving and inspecting the UI from a computer
 *
 * Only compiled with -D KIDDO_DEVTOOLS (the esp32dev-devtools environment).
 * scripts/devtools.py is the host side. Commands, one per line:
 *   shot                 Stream a screenshot as "@S" lines
 *   tap X Y              Press and release at a point
 *   drag X1 Y1 X2 Y2     Press, move and release (scrolling)
 */

#pragma once

#include <lvgl.h>

namespace devtools {

#ifdef KIDDO_DEVTOOLS

/**
 * @brief Hook the display so screenshots can copy what is flushed
 * @param disp Display created by the TFT driver
 */
void init(lv_display_t* disp);

/**
 * @brief Read and run pending serial commands (call from loop)
 */
void poll();

/**
 * @brief Report an injected touch, if one is in progress
 * @param data Input data to fill
 * @return True if the injected touch replaced the real touchscreen
 */
bool read_touch(lv_indev_data_t* data);

#else

inline void init(lv_display_t*) {}
inline void poll() {}
inline bool read_touch(lv_indev_data_t*) { return false; }

#endif

}
