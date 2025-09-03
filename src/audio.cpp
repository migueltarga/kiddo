/**
 * @file audio.cpp
 * @brief Audio playback system using ESP32 DAC
 *
 * This module provides audio functionality for the application,
 * including click sound generation and playback using the ESP32's
 * built-in DAC (Digital-to-Analog Converter).
 */

#include "audio.h"
#include <driver/dac.h>
#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include "config.h"

#if AUDIO_DAC_PIN == 25
#define AUDIO_DAC_CHANNEL DAC_CHANNEL_1
#elif AUDIO_DAC_PIN == 26
#define AUDIO_DAC_CHANNEL DAC_CHANNEL_2
#else
#error "AUDIO_DAC_PIN must be 25 or 26 on ESP32"
#endif

namespace
{
    /** @brief PCM data for click sound */
    std::vector<uint8_t> g_click_pcm;
    /** @brief Current playback position in click sound */
    uint32_t g_click_pos = 0;
    /** @brief Playback active flag */
    bool g_playing = false;

    /**
     * @brief Generate click sound PCM data
     * Creates a short click sound using sine wave synthesis
     */
    void generate_click()
    {
        const int samples = 200;
        g_click_pcm.resize(samples);
        for (int i = 0; i < samples; i++)
        {
            float env = 1.0f - (float)i / samples;
            float freq = 1500.0f;
            float val = sinf(2.0f * 3.14159f * freq * (float)i / 20000.0f) * env;
            int v = (int)(val * 110.0f) + 128;
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            g_click_pcm[i] = (uint8_t)v;
        }
    }
}

namespace audio
{
    void init()
    {
        dac_output_enable(AUDIO_DAC_CHANNEL);
        generate_click();
    }
    
    void update()
    {
        if (g_playing)
        {
            for (int i = 0; i < 160 && g_playing; i++)
            {
                if (g_click_pos >= g_click_pcm.size())
                {
                    g_playing = false;
                    g_click_pos = 0;
                    break;
                }
                uint8_t s = g_click_pcm[g_click_pos++];
                dac_output_voltage(AUDIO_DAC_CHANNEL, s);
                delayMicroseconds(50);
            }
        }
    }
    
    void play_click()
    {
        if (g_click_pcm.empty())
            return;
        g_click_pos = 0;
        g_playing = true;
    }
}

static void btn_click_sound_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        audio::play_click();
}

void ui_add_click_sound(lv_obj_t *btn)
{
    lv_obj_add_event_cb(btn, btn_click_sound_cb, LV_EVENT_CLICKED, nullptr);
}
