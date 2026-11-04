#include "display_comfort.h"
#include "config.h"
#include "audio.h"
#include "led_effects.h"

namespace {
uint8_t manual_level = 200;
bool automatic_enabled = false;
bool bedtime_enabled = false;
#ifdef KIDDO_3248S035C
constexpr uint8_t kSensorPin = 34;
constexpr uint16_t kDarkReading = 1100;
constexpr uint8_t kLowLightLevel = 60;
constexpr uint8_t kBedtimeLimit = 55;
uint16_t filtered_reading = 0;
uint8_t output_level = 200;
uint32_t last_read = 0;

uint8_t target_level() {
    uint8_t level = manual_level;
    if (automatic_enabled) {
        uint16_t raw = min(filtered_reading, kDarkReading);
        uint8_t floor = min(manual_level, kLowLightLevel);
        level = floor + (uint32_t)(manual_level - floor) * (kDarkReading - raw) / kDarkReading;
    }
    return bedtime_enabled ? min(level, kBedtimeLimit) : level;
}

void apply(bool immediate) {
    uint8_t target = target_level();
    if (immediate) output_level = target;
    else if (output_level + 12 < target) output_level += 12;
    else if (output_level > target + 12) output_level -= 12;
    else output_level = target;
    backlight_write(output_level);
}
#endif
}

namespace display_comfort {
void init(uint8_t brightness, bool automatic, bool bedtime) {
    manual_level = brightness;
    automatic_enabled = automatic;
    bedtime_enabled = bedtime;
#ifdef KIDDO_3248S035C
    analogReadResolution(12);
    analogSetPinAttenuation(kSensorPin, ADC_11db);
    delay(20);
    analogRead(kSensorPin);
    analogRead(kSensorPin);
    uint32_t total = 0;
    for (int i = 0; i < 8; ++i) {
        total += analogRead(kSensorPin);
        delay(2);
    }
    filtered_reading = total / 8;
    Serial.printf("[LIGHT] GPIO34 raw %u, auto %d, bedtime %d\n",
                  filtered_reading, automatic_enabled, bedtime_enabled);
    audio::set_bedtime_mode(bedtime_enabled);
    led_effects::set_bedtime_mode(bedtime_enabled);
    apply(true);
#else
    backlight_write(manual_level);
#endif
}

void update() {
#ifdef KIDDO_3248S035C
    if (millis() - last_read < 500) return;
    last_read = millis();
    if (automatic_enabled) {
        uint16_t raw = analogRead(kSensorPin);
        filtered_reading = (filtered_reading * 7u + raw) / 8u;
    }
    apply(false);
#endif
}

void set_manual_brightness(uint8_t value) {
    manual_level = value;
#ifdef KIDDO_3248S035C
    apply(true);
#else
    backlight_write(value);
#endif
}

void set_automatic(bool enabled) {
    automatic_enabled = enabled;
#ifdef KIDDO_3248S035C
    if (enabled) filtered_reading = analogRead(kSensorPin);
    apply(true);
#endif
}

void set_bedtime(bool enabled) {
    bedtime_enabled = enabled;
#ifdef KIDDO_3248S035C
    audio::set_bedtime_mode(enabled);
    led_effects::set_bedtime_mode(enabled);
    apply(true);
#endif
}

bool automatic() { return automatic_enabled; }
bool bedtime() { return bedtime_enabled; }
}
