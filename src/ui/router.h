/**
 * @file router.h
 * @brief UI navigation and routing system
 *
 * This file contains functions for navigating between different UI screens
 * and managing the application's navigation state.
 */

#pragma once
#include "models.h"
#include "story_engine.h"

namespace ui_router
{

/**
 * @brief Show home screen
 */
void show_home();

/**
 * @brief Show story library screen
 */
void show_library();

/**
 * @brief Show story reading screen
 * @param st Story to display
 * @param nodeKey Starting node key
 */
void show_story(const Story_t &st, const String &nodeKey);

/**
 * @brief Show splash screen
 * @param msg Optional loading message
 */
void show_splash(const char *msg = nullptr);

/**
 * @brief Show settings screen
 */
void show_settings();

}
