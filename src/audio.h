/**
 * @file audio.h
 * @brief Audio system management
 *
 * This file contains the audio namespace with functions for initializing
 * the audio system, playing sound effects, and managing audio playback.
 */

#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include <vector>

namespace audio {

/**
 * @brief Initialize the audio system
 * Sets up audio hardware and prepares sound playback
 */
void init();

/**
 * @brief Update audio system state
 * Should be called regularly from the main loop
 */
void update();

/**
 * @brief Play a click sound effect
 * Used for UI button interactions
 */
void play_click();

}

/**
 * @brief Add click sound to a UI button
 * @param btn LVGL button object to add sound to
 */
void ui_add_click_sound(lv_obj_t* btn);
