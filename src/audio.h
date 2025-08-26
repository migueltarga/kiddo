#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include <vector>

namespace audio {
  void init();
  void update();
  void play_click();
}

void ui_add_click_sound(lv_obj_t* btn);
