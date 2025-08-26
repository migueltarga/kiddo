#pragma once
#include <lvgl.h>

extern const lv_font_t montserrat_12;
extern const lv_font_t montserrat_14;
extern const lv_font_t montserrat_16;
extern const lv_font_t montserrat_20;

#define KIDDO_FONT_12 montserrat_12
#define KIDDO_FONT_14 montserrat_14
#define KIDDO_FONT_16 montserrat_16
#define KIDDO_FONT_20 montserrat_20


inline const lv_font_t* font12(){
  return &KIDDO_FONT_12;
}

inline const lv_font_t* font14(){
  return &KIDDO_FONT_14;
}

inline const lv_font_t* font16(){
  return &KIDDO_FONT_16;
}

inline const lv_font_t* font20(){
  return &KIDDO_FONT_20;
}
