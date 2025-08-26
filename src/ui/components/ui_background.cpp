#include "ui_components.h"
#include "config.h"

lv_obj_t *ui_background_create(lv_obj_t *parent, const ui_background_config_t *config)
{
    // Set background color on parent
    lv_obj_set_style_bg_color(parent, config->bg_color, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    
    // Create and configure image
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, config->image_src);
    
    // Auto-scale image to fit screen
    int aw = config->image_src->header.w;
    int ah = config->image_src->header.h;
    if (aw > 0 && ah > 0) {
        uint32_t sw = ((uint32_t)SCREEN_WIDTH * 256u) / (uint32_t)aw;
        uint32_t sh = ((uint32_t)SCREEN_HEIGHT * 256u) / (uint32_t)ah;
        uint32_t s = sw > sh ? sw : sh;
        if (s == 0)
            s = 256;
        lv_image_set_scale(img, s);
    }
    
    // Center and move to background
    lv_obj_center(img);
    lv_obj_move_background(img);
    
    return img;
}

ui_background_config_t ui_background_config_default(const lv_image_dsc_t *image_src)
{
    ui_background_config_t config;
    config.image_src = image_src;
    config.bg_color = lv_color_black();
    return config;
}
