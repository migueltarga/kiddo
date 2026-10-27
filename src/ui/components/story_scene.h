/**
 * @file story_scene.h
 * @brief Animated picture at the top of story pages that have none
 *
 * While a grown-up reads, the child gets something to look at: a scene that
 * matches the page (bedroom, pool, school, forest...), the people and animals
 * it talks about, things it mentions (a cake, a float, a book), and gentle
 * movement. Everyone jumps when the scene is tapped.
 *
 * All of it comes from the page's words, so any story gets fitting scenes;
 * a story can also say exactly what to show, see docs/STORY_FORMAT.md.
 */

#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include "../../models.h"

namespace StoryScene {

/** @brief What a page's scene shows */
struct Spec {
    String place;                   /**< Scene name ("pool", "bedroom_night"...) */
    std::vector<String> cast;       /**< Figures, left to right ("mom", "zoe", "ted") */
    std::vector<String> props;      /**< Things on the ground ("ball", "cake") */
    std::vector<String> extras;     /**< Moving bits ("heart", "note", "fish"...) */
    bool night = false;
    bool dusk = false;              /**< Sunset: warm light outdoors */
};

/**
 * @brief Figures that appear on every page of a story
 *
 * The story's "cast" if it has one; otherwise Zoe and Ted, each if the story
 * names them.
 */
std::vector<String> leads(const Story_t& story);

/**
 * @brief Decide what a page's scene shows
 * @param story The story, for its night flag and character names
 * @param node The page; its "scene", "cast" and "props" win over its words
 * @param leads From leads(), shown on every page
 * @param previous The previous page's scene: the place, company and props
 *        stay when the page doesn't name new ones
 */
Spec pick(const Story_t& story, const Node_t& node, const std::vector<String>& leads, const Spec& previous);

/**
 * @brief Build the scene banner
 * @param parent Page column it goes in (full width, 100 px tall)
 * @param spec From pick()
 * @return The banner
 */
lv_obj_t* create(lv_obj_t* parent, const Spec& spec);

}
