/**
 * @file library_screen.cpp
 * @brief Story library screen implementation
 *
 * This module provides the story library screen with story listing,
 * download management, and navigation to individual stories.
 */

#include <Arduino.h>
#include <lvgl.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <set>
#include "remote_catalog.h"
#include "ui_screens.h"
#include "styles.h"
#include "ui/fonts.h"
#include "story_engine.h"
#include "config.h"
#include "file_system.h"
#include "async_manager.h"
#include "audio.h"
#include "ui/components/ui_components.h"
#include "ui/router.h"
#include "story_utils.h"
#include "story_progress.h"

/** @brief Current story index */
int g_story_idx = -1;
/** @brief Current node key */
String g_node_key;
/** @brief Home callback function */
ui_story_home_cb_t g_home_cb = nullptr;
/** @brief Remote fetch completion flag */
bool g_remote_fetch_done = false;
/** @brief Remote fetch failure flag */
bool g_remote_fetch_failed = false;
/** @brief Fetch in progress flag */
bool g_fetch_in_progress = false;
/** @brief The story list while the library is on screen, else nullptr */
static lv_obj_t *g_library_list = nullptr;
/** @brief Download overlay object */
lv_obj_t *g_download_overlay = nullptr;

#include "i18n.h"
extern void ui_library_screen_refresh_for_language_change();
extern void ui_library_screen_show();
extern Language current_language;
extern bool online_mode;
#include "audio.h"

static lv_obj_t* create_loading_overlay(lv_obj_t *parent, const char* message) {
    ui_loading_overlay_config_t config = ui_loading_overlay_config_default(message);
    return ui_loading_overlay_create(parent, &config);
}

/**
 * @brief Collect "file|lang" keys for stories already downloaded
 *
 * Reads the index once instead of opening every story file.
 */
static std::set<String> downloaded_keys()
{
	std::set<String> keys;
	JsonDocument doc;
	if (!FileSystem::loadIndex(doc))
		return keys;
	for (JsonObject story : doc["stories"].as<JsonArray>())
	{
		String file = story["file"] | "";
		String lang = story["lang"] | "";
		if (file.length() && FileSystem::fileSize(file) > 0)
			keys.insert(file + "|" + lang);
	}
	return keys;
}

void ui_library_catalog_changed()
{
	remote_catalog::invalidate();
	g_remote_fetch_done = false;
	g_remote_fetch_failed = false;
}

void ui_library_prefetch_catalog()
{
	if (!online_mode || g_remote_fetch_done || g_fetch_in_progress)
	{
		return;
	}
	g_fetch_in_progress = true;

	AsyncManager::fetchCatalog([](bool success) {
		g_remote_fetch_done = true;
		g_remote_fetch_failed = !success || remote_catalog::entries().empty();
		g_fetch_in_progress = false;

		// Only redraw if the reader is looking at the library
		if (g_library_list)
		{
			ui_library_screen_show();
		}
	});
}

/**
 * @brief Add a quiet "looking for new stories" row with a small spinner
 * @param list Library list
 */
static void create_checking_row(lv_obj_t *list)
{
	lv_obj_t *row = lv_obj_create(list);
	lv_obj_remove_style_all(row);
	lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_ver(row, 6, 0);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(row, 8, 0);

	lv_obj_t *spinner = lv_spinner_create(row);
	lv_obj_set_size(spinner, 18, 18);
	lv_obj_set_style_arc_width(spinner, 3, LV_PART_MAIN);
	lv_obj_set_style_arc_width(spinner, 3, LV_PART_INDICATOR);
	lv_obj_set_style_arc_color(spinner, palette::parchment(), LV_PART_MAIN);
	lv_obj_set_style_arc_color(spinner, palette::primary(), LV_PART_INDICATOR);

	lv_obj_t *label = lv_label_create(row);
	lv_label_set_text(label, S()->checking_stories);
	lv_obj_set_style_text_font(label, font12(), 0);
	lv_obj_set_style_text_color(label, palette::ink_soft(), 0);
}

void ui_story_set_home_cb(ui_story_home_cb_t cb) { g_home_cb = cb; }

static void on_back_button_clicked(lv_event_t *e)
{
	ui_router::show_home();
}

static void on_story_select(lv_event_t *e)
{
	intptr_t idx = (intptr_t)lv_event_get_user_data(e);
	const auto &stories = story::all();
	if (idx < 0 || (size_t)idx >= stories.size()) {
		return;
	}
	g_story_idx = (int)idx;
	g_node_key = stories[g_story_idx].start;
	ui_story_screen_open(stories[g_story_idx]);
}
static void on_remote_entry(lv_event_t *e)
{
	intptr_t ridx = (intptr_t)lv_event_get_user_data(e);
	const auto &ents = remote_catalog::entries();
	if (ridx < 0 || (size_t)ridx >= ents.size()) {
		return;
	}
	String file = ents[ridx].file;

	if (!g_download_overlay)
	{
		lv_obj_t *scr = lv_scr_act();
		g_download_overlay = create_loading_overlay(scr, S()->downloading_story);
	}

	AsyncManager::downloadStory(file, [](bool success, const String& storyId) {
		if (g_download_overlay) {
			lv_obj_del(g_download_overlay);
			g_download_overlay = nullptr;
		}
		
		if (!success) {
			ui_splash_screen_show(S()->delete_failed);
			lv_timer_create([](lv_timer_t *tt) {
				ui_library_screen_show();
				lv_timer_del(tt);
			}, 800, nullptr);
			return;
		}
		
		story::loadFromFS();
		if (storyId.length()) {
			for (const auto &st : story::all()) {
				if (st.id == storyId) {
					ui_story_screen_show(st, st.start);
					return;
				}
			}
		}
		ui_library_screen_show();
	});
}

/**
 * @brief Create a story card: round icon, title, and an optional tag line
 * @param list Library list
 * @param title Story title
 * @param symbol LVGL symbol drawn in the icon
 * @param icon_color Icon background
 * @param tag Small orange text under the title, or nullptr
 * @param cb Click handler
 * @param user_data Click handler data
 */
static void create_story_card(lv_obj_t *list, const char *title, const char *symbol, lv_color_t icon_color,
							  const char *tag, lv_event_cb_t cb, void *user_data)
{
	lv_obj_t *card = lv_obj_create(list);
	ui_add_click_sound(card);
	lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_min_height(card, 52, 0);
	lv_obj_set_style_bg_color(card, palette::card(), 0);
	lv_obj_set_style_bg_color(card, palette::parchment(), LV_STATE_PRESSED);
	lv_obj_set_style_border_width(card, 1, 0);
	lv_obj_set_style_border_color(card, palette::parchment_dark(), 0);
	lv_obj_set_style_radius(card, 12, 0);
	lv_obj_set_style_pad_all(card, 8, 0);
	lv_obj_set_style_pad_column(card, 10, 0);
	lv_obj_set_style_shadow_width(card, 0, 0);
	lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_add_event_cb(card, cb, LV_EVENT_CLICKED, user_data);

	lv_obj_t *icon = lv_obj_create(card);
	lv_obj_remove_style_all(icon);
	lv_obj_set_size(icon, 36, 36);
	lv_obj_set_style_radius(icon, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(icon, icon_color, 0);
	lv_obj_set_style_bg_opa(icon, LV_OPA_COVER, 0);
	lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_t *icon_label = lv_label_create(icon);
	lv_label_set_text(icon_label, symbol);
	lv_obj_set_style_text_color(icon_label, lv_color_white(), 0);
	lv_obj_center(icon_label);

	lv_obj_t *text = lv_obj_create(card);
	lv_obj_remove_style_all(text);
	lv_obj_set_height(text, LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(text, 1);
	lv_obj_set_flex_flow(text, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_row(text, 2, 0);
	lv_obj_clear_flag(text, LV_OBJ_FLAG_CLICKABLE);

	lv_obj_t *title_label = lv_label_create(text);
	lv_obj_set_width(title_label, LV_PCT(100));
	lv_label_set_long_mode(title_label, LV_LABEL_LONG_WRAP);
	lv_label_set_text(title_label, title);
	lv_obj_set_style_text_font(title_label, font16(), 0);
	lv_obj_set_style_text_color(title_label, palette::ink(), 0);

	if (tag) {
		lv_obj_t *tag_label = lv_label_create(text);
		lv_label_set_text(tag_label, tag);
		lv_obj_set_style_text_font(tag_label, font12(), 0);
		lv_obj_set_style_text_color(tag_label, palette::primary_dark(), 0);
	}
}

extern void ui_app_before_screen_change();
void ui_library_screen_show()
{
	ui_app_before_screen_change();
	lv_obj_t *scr = lv_scr_act();
	lv_obj_clean(scr);
	const auto *s = S();
	apply_screen_bg(scr);
	
	ui_header_config_t header_config = ui_header_config_default(s->stories_title, on_back_button_clicked);
	lv_obj_t *header = ui_header_create(scr, &header_config);
	lv_obj_t *list = lv_obj_create(scr);
	g_library_list = list;
	lv_obj_add_event_cb(list, [](lv_event_t *e) {
		if (lv_event_get_target(e) == g_library_list)
			g_library_list = nullptr;
	}, LV_EVENT_DELETE, nullptr);
	lv_obj_set_size(list, SCREEN_WIDTH, SCREEN_HEIGHT - 38);
	lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 38);
	lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(list, 0, 0);
	lv_obj_set_style_outline_width(list, 0, 0);
	lv_obj_set_style_radius(list, 0, 0);
	lv_obj_set_style_pad_all(list, 8, 0);
	lv_obj_set_style_pad_row(list, 8, 0);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
	apply_scrollbar_style(list);
	
	const auto &storiesNow = story::all();
	const auto &ents = remote_catalog::entries();
	
	bool any_local = !storiesNow.empty();
	bool any_remote = false;
	
	for (size_t i = 0; i < storiesNow.size(); ++i)
	{
		const Story_t &st = storiesNow[i];
		story_progress::Saved saved;
		bool started = story_progress::load(st.id, saved) && saved.node != st.start;
		create_story_card(list, st.title.length() ? st.title.c_str() : st.id.c_str(),
						  LV_SYMBOL_PLAY, palette::primary(), started ? s->resume_continue : nullptr,
						  on_story_select, (void *)i);
	}
	
	if (online_mode) {
		std::set<String> shown_file_lang;
		std::set<String> downloaded = downloaded_keys();
		
		for (size_t i = 0; i < ents.size(); ++i)
		{
			const auto &ent = ents[i];
			if (!story_utils::matchesLanguage(current_language, ent.lang)) continue;
			
			String unique_key = ent.file + "|" + ent.lang;
			if (shown_file_lang.count(unique_key)) continue;
			
			shown_file_lang.insert(unique_key);
			String label = ent.name.length() ? ent.name : ent.file;
			
			bool found_local = downloaded.count("/" + unique_key) > 0;
			
			if (!found_local)
			{
				any_remote = true;
				create_story_card(list, label.c_str(), LV_SYMBOL_DOWNLOAD, palette::accent(), nullptr,
								  on_remote_entry, (void *)i);
			}
		}
	} else {
		g_remote_fetch_done = false;
		g_remote_fetch_failed = false;
	}
	
	// Local stories show right away; the catalog loads below them
	bool checking = online_mode && !g_remote_fetch_done;
	
	// If nothing to show, display 'no stories' message
	if (!any_local && !any_remote && !checking)
	{
		// Create a container for the empty state that takes up the full list area
		lv_obj_t *empty_container = lv_obj_create(list);
		lv_obj_set_size(empty_container, LV_PCT(100), LV_PCT(100));
		lv_obj_set_style_bg_opa(empty_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(empty_container, 0, 0);
		lv_obj_set_style_pad_all(empty_container, 0, 0);
		lv_obj_set_flex_flow(empty_container, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(empty_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		
		// Main message
		lv_obj_t *msg_lbl = lv_label_create(empty_container);
		lv_label_set_text(msg_lbl, s->no_stories);
		lv_obj_set_style_text_font(msg_lbl, font14(), 0);
		lv_obj_set_style_text_color(msg_lbl, palette::ink_soft(), 0);
		lv_obj_set_style_text_align(msg_lbl, LV_TEXT_ALIGN_CENTER, 0);
		
		// Additional helpful text
		lv_obj_t *help_lbl = lv_label_create(empty_container);
		lv_label_set_text(help_lbl, online_mode ? s->stories_will_appear : s->enable_online_mode);
		lv_obj_set_style_text_font(help_lbl, font12(), 0);
		lv_obj_set_style_text_color(help_lbl, palette::ink_soft(), 0);
		lv_obj_set_style_text_align(help_lbl, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_margin_top(help_lbl, 8, 0);
		lv_label_set_long_mode(help_lbl, LV_LABEL_LONG_WRAP);
		lv_obj_set_width(help_lbl, LV_PCT(80));
	}	// Online mode phased loading with states
	if (online_mode)
	{
		if (checking)
		{
			create_checking_row(list);
			ui_library_prefetch_catalog();
		}
		else if (g_remote_fetch_failed)
		{
			lv_obj_t *btn_retry = lv_btn_create(list);
			ui_add_click_sound(btn_retry);
			lv_obj_set_width(btn_retry, LV_PCT(100));
			lv_obj_set_height(btn_retry, 36);
			apply_outline_button_style(btn_retry, palette::danger());
			lv_obj_add_event_cb(btn_retry, [](lv_event_t *e)
			{
				g_remote_fetch_done = false;
				g_remote_fetch_failed = false;
				remote_catalog::invalidate();
				ui_library_screen_show();
			}, LV_EVENT_CLICKED, nullptr);
			lv_obj_t *lblr = lv_label_create(btn_retry);
			lv_label_set_text_fmt(lblr, LV_SYMBOL_REFRESH "  %s", S()->retry_online_fetch);
			lv_obj_center(lblr);
		}
		else
		{
			// reconcileExisting() reloads the story list when it adds anything
			int newly = remote_catalog::reconcileExisting();
			if (newly > 0)
			{
				ui_library_screen_show();
				return;
			}
		}
	}
}
