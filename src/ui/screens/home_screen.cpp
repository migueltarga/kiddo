#include <lvgl.h>
#include "i18n.h"
#include "styles.h"
#include "ui/fonts.h"
#include "config.h"
#include "ui_screens.h"
#include "audio.h"
#include "ui/components/ui_components.h"

using simple_cb_t = void (*)();
static simple_cb_t g_open_settings_cb = nullptr;
static simple_cb_t g_open_stories_cb = nullptr;

void ui_home_screen_set_callbacks(simple_cb_t open_settings, simple_cb_t open_stories)
{
    g_open_settings_cb = open_settings;
    g_open_stories_cb = open_stories;
}

static void on_open_settings(lv_event_t *e)
{
    (void)e;
    if (g_open_settings_cb)
        g_open_settings_cb();
}
static void on_open_stories(lv_event_t *e)
{
    (void)e;
    if (g_open_stories_cb)
        g_open_stories_cb();
}

void ui_home_screen_show()
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    apply_screen_bg(scr);

    ui_nav_buttons_config_t nav_config = {
        .left_text = S()->settings_title,
        .right_text = S()->stories_btn,
        .left_callback = on_open_settings,
        .right_callback = on_open_stories,
        .left_user_data = NULL,
        .right_user_data = NULL
    };
    ui_nav_buttons_create(scr, &nav_config);
}
