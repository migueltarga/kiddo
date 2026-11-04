#pragma once

#include <Arduino.h>

namespace led_effects {
// The 3.5-inch board has a common-anode RGB LED. Other boards are no-ops.
void init();
void show(const String& cue, const String& scene, const String& sound,
          bool night, bool ending, bool found_item);
void off();
void update();
}
