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


/** @brief Sound effects, synthesized as they play */
enum class Sound : uint8_t {
    Click,      /**< Button press */
    Boing,      /**< Something jumps */
    Chime,      /**< An item was found */
    Success,    /**< A right answer */
    Oops,       /**< A wrong answer, gently */
    Page,       /**< A soft swish */
    Fanfare     /**< The story ended */
};

/**
 * @brief Play a click sound effect
 * Used for UI button interactions
 */
void play_click();

/**
 * @brief Queue a sound effect; returns at once
 *
 * Sounds play one after another; when several are already waiting, the new
 * one is dropped rather than playing late.
 */
void play(Sound sound);

/**
 * @brief Find a sound by the name stories use ("success", "oops", "chime"...)
 * @return True if the name is known
 */
bool sound_from_name(const String& name, Sound& out);

}

/**
 * @brief Add click sound to a UI button
 * @param btn LVGL button object to add sound to
 */
void ui_add_click_sound(lv_obj_t* btn);
