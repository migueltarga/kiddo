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
LV_IMG_DECLARE(image_home);
LV_IMG_DECLARE(figure_bear);
LV_IMG_DECLARE(figure_boy);
LV_IMG_DECLARE(figure_cat);
LV_IMG_DECLARE(figure_dad);
LV_IMG_DECLARE(figure_dog);
LV_IMG_DECLARE(figure_frog);
LV_IMG_DECLARE(figure_girl);
LV_IMG_DECLARE(figure_girl2);
LV_IMG_DECLARE(figure_grandma);
LV_IMG_DECLARE(figure_grandpa);
LV_IMG_DECLARE(figure_mom);
LV_IMG_DECLARE(figure_owl);
LV_IMG_DECLARE(figure_sheep);
LV_IMG_DECLARE(figure_squirrel);
LV_IMG_DECLARE(figure_teacher);
LV_IMG_DECLARE(figure_ted);
LV_IMG_DECLARE(figure_zoe);
LV_IMG_DECLARE(fx_bird);
LV_IMG_DECLARE(fx_butterfly);
LV_IMG_DECLARE(fx_fish);
LV_IMG_DECLARE(fx_heart);
LV_IMG_DECLARE(fx_note);
LV_IMG_DECLARE(prop_ball);
LV_IMG_DECLARE(prop_blocks);
LV_IMG_DECLARE(prop_book);
LV_IMG_DECLARE(prop_cake);
LV_IMG_DECLARE(prop_chest);
LV_IMG_DECLARE(prop_door);
LV_IMG_DECLARE(prop_float);
LV_IMG_DECLARE(prop_food);
LV_IMG_DECLARE(prop_paints);
LV_IMG_DECLARE(prop_pancakes);
LV_IMG_DECLARE(prop_sandcastle);
LV_IMG_DECLARE(scene_beach);
LV_IMG_DECLARE(scene_bedroom_day);
LV_IMG_DECLARE(scene_bedroom_night);
LV_IMG_DECLARE(scene_cave);
LV_IMG_DECLARE(scene_classroom);
LV_IMG_DECLARE(scene_forest);
LV_IMG_DECLARE(scene_garden);
LV_IMG_DECLARE(scene_kitchen);
LV_IMG_DECLARE(scene_lake);
LV_IMG_DECLARE(scene_library);
LV_IMG_DECLARE(scene_living_room);
LV_IMG_DECLARE(scene_mountain);
LV_IMG_DECLARE(scene_playground);
LV_IMG_DECLARE(scene_pool);
LV_IMG_DECLARE(scene_road);
LV_IMG_DECLARE(scene_school);
LV_IMG_DECLARE(scene_sky);
LV_IMG_DECLARE(image_splash);
LV_IMG_DECLARE(backpack);
LV_IMG_DECLARE(chest);
LV_IMG_DECLARE(compass);
LV_IMG_DECLARE(key);
LV_IMG_DECLARE(old_map);
LV_IMG_DECLARE(potion);
LV_IMG_DECLARE(sword);
LV_IMG_DECLARE(icon_backpack);
LV_IMG_DECLARE(icon_item_flashlight);
LV_IMG_DECLARE(icon_item_honey);
LV_IMG_DECLARE(icon_item_feather);
LV_IMG_DECLARE(icon_item_shell);


#endif
