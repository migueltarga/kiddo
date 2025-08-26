#include "router.h"
#include "app_ui.h"
#include "screens/ui_screens.h"

namespace ui_router
{
    void show_home()
    {
        ui_app_show_home();
    }
    void show_library() { ui_library_screen_show(); }
    void show_story(const Story_t &st, const String &nodeKey) { ui_story_screen_show(st, nodeKey); }
    void show_splash(const char *msg) { ui_splash_screen_show(msg); }
    void show_settings() { ui_settings_screen_show(); }
}
