#pragma once

#include <Arduino.h>

namespace display_comfort {
void init(uint8_t manual_brightness, bool automatic, bool bedtime);
void update();
void set_manual_brightness(uint8_t value);
void set_automatic(bool enabled);
void set_bedtime(bool enabled);
bool automatic();
bool bedtime();
}
