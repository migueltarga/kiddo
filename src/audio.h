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

/** @brief Set or read the sound-effect volume (0-100 percent). */
void set_volume(uint8_t percent);
uint8_t volume();
/** @brief Limit effective volume while bedtime mode is active. */
void set_bedtime_mode(bool enabled);


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

/** @brief State of optional spoken page narration. */
enum class NarrationState : uint8_t { Idle, Loading, Playing, Paused, Error };

/** @brief Start or restart a PCM WAV recording from SPIFFS or an HTTPS URL. */
bool play_narration(const String& source);
void pause_narration();
void resume_narration();
void stop_narration();
NarrationState narration_state();

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
