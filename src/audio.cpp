/**
 * @file audio.cpp
 * @brief Audio playback system using ESP32 DAC
 *
 * Sound effects (clicks, jumps, chimes, fanfares) are synthesized note by
 * note as they play, through the ESP32's built-in DAC (Digital-to-Analog
 * Converter), so they take no flash or RAM for recordings.
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
    /** @brief Sample rate of the generated sounds, in Hz */
    const uint32_t SAMPLE_RATE = 20000;

    enum Wave : uint8_t { SINE, TRIANGLE, NOISE };

    /** @brief One note: a pitch (sliding from f0 to f1), fading out over its length */
    struct Tone {
        uint16_t f0, f1;    /**< Start and end frequency, Hz */
        uint16_t ms;        /**< Length */
        Wave wave;
        uint8_t volume;     /**< Peak swing around the DAC midpoint, up to 127 */
    };

    struct Effect {
        const Tone* tones;
        uint8_t count;
    };

    // Small speakers favor the 500 Hz - 2.5 kHz range, so the notes stay there
    const Tone kClick[] = {{1500, 1500, 10, SINE, 110}};
    const Tone kBoing[] = {{200, 560, 110, TRIANGLE, 100}, {560, 420, 140, TRIANGLE, 90}};
    const Tone kChime[] = {{1319, 1319, 90, SINE, 90}, {1568, 1568, 90, SINE, 90}, {2093, 2093, 260, SINE, 100}};
    const Tone kSuccess[] = {{523, 523, 100, TRIANGLE, 100}, {659, 659, 100, TRIANGLE, 100},
                             {784, 784, 100, TRIANGLE, 100}, {1047, 1047, 280, TRIANGLE, 110}};
    const Tone kOops[] = {{466, 440, 170, TRIANGLE, 90}, {392, 330, 280, TRIANGLE, 90}};
    const Tone kPage[] = {{0, 0, 110, NOISE, 35}};
    const Tone kFanfare[] = {{392, 392, 120, TRIANGLE, 100}, {523, 523, 120, TRIANGLE, 100}, {659, 659, 120, TRIANGLE, 100},
                             {784, 784, 260, TRIANGLE, 110}, {659, 659, 120, TRIANGLE, 100}, {784, 784, 450, TRIANGLE, 110}};

    const Effect kEffects[] = {
        {kClick, 1}, {kBoing, 2}, {kChime, 3}, {kSuccess, 4}, {kOops, 2}, {kPage, 1}, {kFanfare, 6},
    };

    /** @brief Sounds waiting to play */
    QueueHandle_t g_queue = nullptr;

    /**
     * @brief Play one note, sample by sample
     * @param next_us Time of the next sample, carried over between notes
     */
    void play_tone(const Tone &t, uint32_t &next_us)
    {
        const uint32_t samples = (uint32_t)t.ms * SAMPLE_RATE / 1000;
        const uint32_t attack = SAMPLE_RATE / 200; // 5 ms, avoids a pop at the start
        const uint32_t sample_us = 1000000 / SAMPLE_RATE;
        float phase = 0.0f;
        uint32_t noise = 0x12345678;
        for (uint32_t i = 0; i < samples; i++)
        {
            float progress = (float)i / samples;
            float freq = t.f0 + (t.f1 - t.f0) * progress;
            phase += freq / SAMPLE_RATE;
            phase -= (int)phase;

            float val;
            if (t.wave == SINE)
                val = sinf(2.0f * 3.14159f * phase);
            else if (t.wave == TRIANGLE)
                val = 4.0f * fabsf(phase - 0.5f) - 1.0f;
            else
            {
                noise = noise * 1664525u + 1013904223u;
                val = (float)(noise >> 24) / 127.5f - 1.0f;
            }

            float env = i < attack ? (float)i / attack : 1.0f - progress;
            int v = 128 + (int)(val * env * t.volume);
            dac_output_voltage(AUDIO_DAC_CHANNEL, (uint8_t)constrain(v, 0, 255));

            // Pace by the clock, so the time spent computing doesn't lower the pitch
            next_us += sample_us;
            while ((int32_t)(micros() - next_us) < 0)
            {
            }
        }
    }

    /**
     * @brief Play queued sounds
     *
     * Runs on the core opposite the Arduino loop, so the busy-wait between
     * samples neither stalls the UI nor gets interrupted by it.
     */
    void audio_task(void *)
    {
        uint8_t id;
        while (true)
        {
            if (xQueueReceive(g_queue, &id, portMAX_DELAY) != pdTRUE || id >= sizeof(kEffects) / sizeof(kEffects[0]))
                continue;
            const Effect &effect = kEffects[id];
            uint32_t next_us = micros();
            for (uint8_t i = 0; i < effect.count; i++)
            {
                play_tone(effect.tones[i], next_us);
                // A breath between notes so they don't blur together
                if (effect.count > 1)
                    next_us += 12000;
            }
            dac_output_voltage(AUDIO_DAC_CHANNEL, 128);
        }
    }
}

namespace audio
{
    void init()
    {
        dac_output_enable(AUDIO_DAC_CHANNEL);
        dac_output_voltage(AUDIO_DAC_CHANNEL, 128);
        g_queue = xQueueCreate(3, sizeof(uint8_t));
        xTaskCreatePinnedToCore(audio_task, "audio", 2048, nullptr, 1, nullptr,
                                xPortGetCoreID() == 0 ? 1 : 0);
    }

    void play(Sound sound)
    {
        if (!g_queue)
            return;
        uint8_t id = (uint8_t)sound;
        xQueueSend(g_queue, &id, 0);
    }

    void play_click()
    {
        play(Sound::Click);
    }

    bool sound_from_name(const String &name, Sound &out)
    {
        static const struct { const char *name; Sound sound; } kNames[] = {
            {"click", Sound::Click}, {"boing", Sound::Boing}, {"chime", Sound::Chime},
            {"success", Sound::Success}, {"oops", Sound::Oops}, {"page", Sound::Page},
            {"fanfare", Sound::Fanfare},
        };
        for (const auto &entry : kNames)
        {
            if (name == entry.name)
            {
                out = entry.sound;
                return true;
            }
        }
        return false;
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
