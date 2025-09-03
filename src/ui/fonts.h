#pragma once
#include <lvgl.h>

/** @brief Montserrat 12pt font */
extern const lv_font_t montserrat_12;
/** @brief Montserrat 14pt font */
extern const lv_font_t montserrat_14;
/** @brief Montserrat 16pt font */
extern const lv_font_t montserrat_16;
/** @brief Montserrat 20pt font */
extern const lv_font_t montserrat_20;

#define KIDDO_FONT_12 montserrat_12
#define KIDDO_FONT_14 montserrat_14
#define KIDDO_FONT_16 montserrat_16
#define KIDDO_FONT_20 montserrat_20

/**
 * @brief Get 12pt font
 * @return Pointer to 12pt font
 */
inline const lv_font_t* font12(){
  return &KIDDO_FONT_12;
}

/**
 * @brief Get 14pt font
 * @return Pointer to 14pt font
 */
inline const lv_font_t* font14(){
  return &KIDDO_FONT_14;
}

/**
 * @brief Get 16pt font
 * @return Pointer to 16pt font
 */
inline const lv_font_t* font16(){
  return &KIDDO_FONT_16;
}

/**
 * @brief Get 20pt font
 * @return Pointer to 20pt font
 */
inline const lv_font_t* font20(){
  return &KIDDO_FONT_20;
}
