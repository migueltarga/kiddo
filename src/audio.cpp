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
#include <SPIFFS.h>
#include <vector>
#include <atomic>
#include "config.h"
#include "file_system.h"

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
    std::atomic<uint8_t> g_volume{DEFAULT_VOLUME};
    std::atomic<bool> g_bedtime{false};

#ifdef KIDDO_3248S035C
    SemaphoreHandle_t g_dac_mutex = nullptr;
    SemaphoreHandle_t g_narration_mutex = nullptr;
    TaskHandle_t g_narration_task = nullptr;
    String g_narration_source;
    std::atomic<uint32_t> g_narration_epoch{0};
    std::atomic<bool> g_narration_paused{false};
    std::atomic<audio::NarrationState> g_narration_state{audio::NarrationState::Idle};

    uint16_t read_u16(File &file)
    {
        uint8_t b[2];
        return file.read(b, 2) == 2 ? uint16_t(b[0] | (b[1] << 8)) : 0;
    }

    uint32_t read_u32(File &file)
    {
        uint8_t b[4];
        return file.read(b, 4) == 4
            ? uint32_t(b[0]) | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24)
            : 0;
    }

    struct WavInfo {
        uint32_t rate = 0;
        uint32_t offset = 0;
        uint32_t bytes = 0;
        uint8_t bits = 0;
    };

    bool read_wav_header(File &file, WavInfo &wav)
    {
        char tag[4];
        if (file.readBytes(tag, 4) != 4 || memcmp(tag, "RIFF", 4) != 0)
            return false;
        read_u32(file);
        if (file.readBytes(tag, 4) != 4 || memcmp(tag, "WAVE", 4) != 0)
            return false;
        bool format_ok = false;
        while (file.position() + 8 <= file.size()) {
            if (file.readBytes(tag, 4) != 4) break;
            uint32_t size = read_u32(file);
            uint32_t start = file.position();
            if (size > file.size() - start) return false;
            if (memcmp(tag, "fmt ", 4) == 0 && size >= 16) {
                uint16_t format = read_u16(file);
                uint16_t channels = read_u16(file);
                uint32_t rate = read_u32(file);
                read_u32(file); // byte rate
                read_u16(file); // block alignment
                uint16_t bits = read_u16(file);
                format_ok = format == 1 && channels == 1 &&
                            (bits == 8 || bits == 16) && rate >= 8000 && rate <= 22050;
                wav.rate = rate;
                wav.bits = bits;
            } else if (memcmp(tag, "data", 4) == 0) {
                wav.offset = start;
                wav.bytes = size;
            }
            if (!file.seek(start + size + (size & 1))) return false;
            if (format_ok && wav.bytes > 0) return file.seek(wav.offset);
        }
        return false;
    }

    String narration_cache_path(const String &url)
    {
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < url.length(); ++i)
            hash = (hash ^ uint8_t(url[i])) * 16777619u;
        char name[24];
        snprintf(name, sizeof(name), "/cache/v%08lx.wav", (unsigned long)hash);
        return String(name);
    }
#endif

#ifdef KIDDO_3248S035C
    void begin_audio_output()
    {
        dac_output_enable(AUDIO_DAC_CHANNEL);
        // Bring the DAC to its midpoint gently after its quiet idle state.
        for (int v = 0; v <= 128; v += 8) {
            dac_output_voltage(AUDIO_DAC_CHANNEL, v);
            delayMicroseconds(150);
        }
    }

    void end_audio_output()
    {
        // The amplifier stays powered. Hold its input low between sounds so
        // the DAC does not feed its idle noise into the speaker.
        for (int v = 128; v >= 0; v -= 8) {
            dac_output_voltage(AUDIO_DAC_CHANNEL, v);
            delayMicroseconds(150);
        }
        dac_output_disable(AUDIO_DAC_CHANNEL);
        pinMode(AUDIO_DAC_PIN, OUTPUT);
        digitalWrite(AUDIO_DAC_PIN, LOW);
    }
#endif

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
            uint8_t volume = g_volume.load();
            if (g_bedtime.load() && volume > 20) volume = 20;
            int v = 128 + (int)(val * env * t.volume * volume / 100.0f);
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
#ifdef KIDDO_3248S035C
            if (g_narration_state.load() != audio::NarrationState::Idle &&
                g_narration_state.load() != audio::NarrationState::Error) continue;
            xSemaphoreTake(g_dac_mutex, portMAX_DELAY);
            begin_audio_output();
#endif
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
#ifdef KIDDO_3248S035C
            end_audio_output();
            xSemaphoreGive(g_dac_mutex);
#endif
        }
    }

#ifdef KIDDO_3248S035C
    void narration_task(void *)
    {
        while (true) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            uint32_t epoch = g_narration_epoch.load();
            xSemaphoreTake(g_narration_mutex, portMAX_DELAY);
            String source = g_narration_source;
            xSemaphoreGive(g_narration_mutex);

            String path = source;
            if (source.startsWith("https://") || source.startsWith("http://")) {
                path = narration_cache_path(source);
                if (FileSystem::fileSize(path) == 0) {
                    // Voice files are much larger than pictures. Keep one cached
                    // recording at a time so the next page has room to download.
                    for (const String &name : FileSystem::listFiles("/cache")) {
                        String old_path = name.startsWith("/") ? name : "/cache/" + name;
                        if (old_path.startsWith("/cache/v") && old_path.endsWith(".wav"))
                            FileSystem::deleteFile(old_path);
                    }
                    if (!FileSystem::downloadFile(source, path)) {
                        if (epoch == g_narration_epoch.load()) g_narration_state.store(audio::NarrationState::Error);
                        continue;
                    }
                }
            }
            if (epoch != g_narration_epoch.load()) continue;
            File file = SPIFFS.open(path, "r");
            WavInfo wav;
            if (!file || !read_wav_header(file, wav)) {
                if (file) file.close();
                if (source.startsWith("https://") || source.startsWith("http://"))
                    FileSystem::deleteFile(path); // A bad cached copy must be retryable.
                if (epoch == g_narration_epoch.load()) g_narration_state.store(audio::NarrationState::Error);
                continue;
            }

            xSemaphoreTake(g_dac_mutex, portMAX_DELAY);
            if (epoch != g_narration_epoch.load()) {
                xSemaphoreGive(g_dac_mutex);
                file.close();
                continue;
            }
            begin_audio_output();
            g_narration_state.store(audio::NarrationState::Playing);
            const uint32_t sample_us = 1000000u / wav.rate;
            uint32_t next_us = micros();
            uint32_t remaining = wav.bytes;
            bool output_on = true;
            uint8_t buffer[512];
            uint8_t blocks_since_yield = 0;
            while (remaining >= wav.bits / 8 && epoch == g_narration_epoch.load()) {
                if (g_narration_paused.load()) {
                    if (output_on) {
                        end_audio_output();
                        output_on = false;
                    }
                    g_narration_state.store(audio::NarrationState::Paused);
                    vTaskDelay(pdMS_TO_TICKS(20));
                    continue;
                }
                if (!output_on) {
                    begin_audio_output();
                    output_on = true;
                    next_us = micros();
                }
                g_narration_state.store(audio::NarrationState::Playing);
                size_t wanted = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
                wanted -= wanted % (wav.bits / 8);
                size_t got = file.read(buffer, wanted);
                if (got != wanted) break;
                remaining -= got;
                for (size_t i = 0; i < got && epoch == g_narration_epoch.load(); i += wav.bits / 8) {
                    // WAV PCM is unsigned at 8 bits, signed little endian at 16 bits.
                    int sample = wav.bits == 8 ? int(buffer[i]) - 128
                        : int(int16_t(uint16_t(buffer[i]) | (uint16_t(buffer[i + 1]) << 8))) / 256;
                    int volume = g_volume.load();
                    if (g_bedtime.load() && volume > 20) volume = 20;
                    uint8_t value = uint8_t(constrain(128 + sample * volume / 100, 0, 255));
                    dac_output_voltage(AUDIO_DAC_CHANNEL, value);
                    next_us += sample_us;
                    while ((int32_t)(micros() - next_us) < 0 && epoch == g_narration_epoch.load()) {}
                }
                // A page can run for tens of seconds. Let the idle task run
                // regularly so the ESP32 task watchdog does not reset playback.
                if (++blocks_since_yield >= 4) {
                    blocks_since_yield = 0;
                    vTaskDelay(1);
                    next_us = micros();
                }
            }
            if (output_on) end_audio_output();
            xSemaphoreGive(g_dac_mutex);
            file.close();
            if (epoch == g_narration_epoch.load()) g_narration_state.store(audio::NarrationState::Idle);
        }
    }
#endif
}

namespace audio
{
    void set_volume(uint8_t percent)
    {
        g_volume.store(percent > 100 ? 100 : percent);
    }

    uint8_t volume()
    {
        return g_volume.load();
    }

    void set_bedtime_mode(bool enabled)
    {
        g_bedtime.store(enabled);
    }

    void init()
    {
#ifdef KIDDO_3248S035C
        pinMode(AUDIO_DAC_PIN, OUTPUT);
        digitalWrite(AUDIO_DAC_PIN, LOW);
#else
        dac_output_enable(AUDIO_DAC_CHANNEL);
        dac_output_voltage(AUDIO_DAC_CHANNEL, 128);
#endif
        g_queue = xQueueCreate(3, sizeof(uint8_t));
#ifdef KIDDO_3248S035C
        g_dac_mutex = xSemaphoreCreateMutex();
        g_narration_mutex = xSemaphoreCreateMutex();
        xTaskCreatePinnedToCore(narration_task, "narration", 12288, nullptr, 2,
                                &g_narration_task, xPortGetCoreID() == 0 ? 1 : 0);
#endif
        xTaskCreatePinnedToCore(audio_task, "audio", 2048, nullptr, 1, nullptr,
                                xPortGetCoreID() == 0 ? 1 : 0);
    }

    void play(Sound sound)
    {
        if (!g_queue)
            return;
#ifdef KIDDO_3248S035C
        if (g_volume.load() == 0 || (g_narration_state.load() != NarrationState::Idle &&
                                      g_narration_state.load() != NarrationState::Error))
            return;
#endif
        uint8_t id = (uint8_t)sound;
        xQueueSend(g_queue, &id, 0);
    }

    void play_click()
    {
        play(Sound::Click);
    }

    bool play_narration(const String &source)
    {
#ifdef KIDDO_3248S035C
        if (!g_narration_task || source.length() == 0 || source.length() > 240 ||
            !(source.startsWith("https://") || source.startsWith("http://") ||
              (source.startsWith("/") && source.indexOf("..") < 0))) return false;
        g_narration_epoch.fetch_add(1);
        g_narration_paused.store(false);
        g_narration_state.store(NarrationState::Loading);
        xSemaphoreTake(g_narration_mutex, portMAX_DELAY);
        g_narration_source = source;
        xSemaphoreGive(g_narration_mutex);
        xQueueReset(g_queue); // A page's old effects must not play after narration.
        xTaskNotifyGive(g_narration_task);
        return true;
#else
        (void)source;
        return false;
#endif
    }

    void pause_narration()
    {
#ifdef KIDDO_3248S035C
        if (g_narration_state.load() == NarrationState::Playing) g_narration_paused.store(true);
#endif
    }

    void resume_narration()
    {
#ifdef KIDDO_3248S035C
        if (g_narration_state.load() == NarrationState::Paused) g_narration_paused.store(false);
#endif
    }

    void stop_narration()
    {
#ifdef KIDDO_3248S035C
        g_narration_epoch.fetch_add(1);
        g_narration_paused.store(false);
        g_narration_state.store(NarrationState::Idle);
#endif
    }

    NarrationState narration_state()
    {
#ifdef KIDDO_3248S035C
        return g_narration_state.load();
#else
        return NarrationState::Idle;
#endif
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
