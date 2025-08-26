#pragma once
#include "models.h"
#include "story_engine.h"
namespace ui_router
{
    void show_home();
    void show_library();
    void show_story(const Story_t &st, const String &nodeKey);
    void show_splash(const char *msg = nullptr);
    void show_settings();
}
