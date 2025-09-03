/**
 * @file images.h
 * @brief LVGL image declarations
 *
 * This file contains declarations for all embedded images used in the application,
 * including backgrounds, splash screens, and inventory item icons.
 */

#ifndef IMAGES_H
#define IMAGES_H

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

LV_IMG_DECLARE(image_background);
LV_IMG_DECLARE(image_splash);
LV_IMG_DECLARE(backpack);
LV_IMG_DECLARE(chest);
LV_IMG_DECLARE(compass);
LV_IMG_DECLARE(key);
LV_IMG_DECLARE(old_map);
LV_IMG_DECLARE(potion);
LV_IMG_DECLARE(sword);


#endif
