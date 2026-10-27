/**
 * @file splash_screen.cpp
 * @brief Splash screen implementation
 *
 * This module provides the splash screen with background image
 * and customizable loading message display.
 */

#include <lvgl.h>
#include "ui/fonts.h"
#include "i18n.h"
#include "ui_screens.h"
#include "ui/images/images.h"
#include "config.h"
#include "ui/components/ui_components.h"

/**
 * @brief Show splash screen
 * @param msg Loading message to display (uses default if null)
 */
void ui_splash_screen_show(const char *msg)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    ui_background_config_t bg_config = ui_background_config_default(&image_splash);
    ui_background_create(scr, &bg_config);

    lv_obj_t *label = lv_label_create(scr);
    if (!msg)
        msg = S()->loading;
    lv_label_set_text(label, msg);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, font16(), 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -16);
}
