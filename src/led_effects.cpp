#include "led_effects.h"
#include "config.h"

#ifdef KIDDO_3248S035C
namespace {
constexpr uint8_t kPins[3] = {4, 16, 17};
constexpr uint8_t kChannels[3] = {4, 5, 6};
enum Mode : uint8_t { OFF, STEADY, PULSE, BLINK, RAINBOW };
Mode mode = OFF;
bool bedtime_mode = false;
uint8_t color[3] = {0, 0, 0};
uint32_t started = 0;
uint32_t last_update = 0;

bool named_color(const String& name, uint8_t out[3]) {
    static const struct { const char* name; uint8_t r, g, b; } colors[] = {
        {"red", 16, 0, 0}, {"green", 0, 14, 0}, {"blue", 0, 3, 16},
        {"yellow", 16, 9, 0}, {"purple", 12, 0, 16},
        {"cyan", 0, 12, 13}, {"white", 9, 9, 9},
    };
    for (const auto& c : colors) {
        if (name == c.name) {
            out[0] = c.r; out[1] = c.g; out[2] = c.b;
            return true;
        }
    }
    return false;
}

void write_rgb(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(kChannels[0], 255 - r);
    ledcWrite(kChannels[1], 255 - g);
    ledcWrite(kChannels[2], 255 - b);
}
}
#endif

namespace led_effects {
void set_bedtime_mode(bool enabled) {
#ifdef KIDDO_3248S035C
    bedtime_mode = enabled;
    last_update = 0;
    update();
#else
    (void)enabled;
#endif
}

void init() {
#ifdef KIDDO_3248S035C
    for (int i = 0; i < 3; ++i) {
        pinMode(kPins[i], OUTPUT);
        digitalWrite(kPins[i], HIGH); // active LOW
        ledcSetup(kChannels[i], 20000, 8);
        ledcAttachPin(kPins[i], kChannels[i]);
    }
    off();
#endif
}

void off() {
#ifdef KIDDO_3248S035C
    mode = OFF;
    write_rgb(0, 0, 0);
#endif
}

void show(const String& cue, const String& scene, const String& sound,
          bool night, bool ending, bool found_item) {
#ifdef KIDDO_3248S035C
    String effect = cue;
    if (effect.isEmpty()) {
        if (ending || sound == "fanfare") effect = "rainbow";
        else if (sound == "oops") effect = "blink:red";
        else if (sound == "success") effect = "blink:green";
        else if (found_item || sound == "chime") effect = "pulse:yellow";
        else if (night) effect = "pulse:blue";
        else if (scene == "forest" || scene == "garden" || scene == "playground") effect = "pulse:green";
        else if (scene == "pool" || scene == "lake" || scene == "beach") effect = "pulse:cyan";
        else if (scene == "cave" || scene == "sky") effect = "pulse:purple";
        else effect = "pulse:yellow";
    }
    if (effect == "off") { off(); return; }
    if (effect == "rainbow") mode = RAINBOW;
    else {
        mode = STEADY;
        if (effect.startsWith("pulse:")) {
            mode = PULSE;
            effect.remove(0, 6);
        } else if (effect.startsWith("blink:")) {
            mode = BLINK;
            effect.remove(0, 6);
        }
        if (!named_color(effect, color)) { off(); return; }
    }
    started = millis();
    last_update = 0;
    update();
#else
    (void)cue; (void)scene; (void)sound;
    (void)night; (void)ending; (void)found_item;
#endif
}

void update() {
#ifdef KIDDO_3248S035C
    if (mode == OFF) return;
    uint32_t now = millis();
    if (now - last_update < 25) return;
    last_update = now;
    uint32_t elapsed = now - started;
    uint16_t duration = mode == STEADY ? 700 : mode == PULSE ? 1300
                        : mode == BLINK ? 900 : 1500;
    if (elapsed >= duration) { off(); return; }
    if (bedtime_mode) {
        if (mode == RAINBOW) write_rgb(4, 2, 0);
        else write_rgb(color[0] / 4, color[1] / 4, color[2] / 4);
        return;
    }
    if (mode == RAINBOW) {
        static const uint8_t wheel[6][3] = {
            {16, 0, 0}, {16, 9, 0}, {0, 14, 0},
            {0, 12, 13}, {0, 3, 16}, {12, 0, 16}
        };
        uint32_t step = elapsed / 250;
        write_rgb(wheel[step][0], wheel[step][1], wheel[step][2]);
        return;
    }
    uint16_t level = 255;
    if (mode == STEADY) {
        if (elapsed < 100) level = elapsed * 255 / 100;
        else if (elapsed > 500) level = (700 - elapsed) * 255 / 200;
    }
    if (mode == BLINK) level = (elapsed < 180 || (elapsed >= 450 && elapsed < 630)) ? 255 : 0;
    if (mode == PULSE) {
        uint16_t phase = elapsed % 650;
        level = phase < 325 ? phase * 255 / 325
                            : (650 - phase) * 255 / 325;
    }
    write_rgb(color[0] * level / 255, color[1] * level / 255, color[2] * level / 255);
#endif
}
}
