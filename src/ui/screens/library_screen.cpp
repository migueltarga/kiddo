
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
int g_story_idx = -1;
String g_node_key;
ui_story_home_cb_t g_home_cb = nullptr;
bool g_remote_fetch_done = false;
bool g_remote_fetch_failed = false;
bool g_fetch_in_progress = false;
lv_timer_t *g_fetch_timer = nullptr;
lv_obj_t *g_fetch_overlay = nullptr;
lv_obj_t *g_download_overlay = nullptr;
String g_pending_download_file;
int g_pending_download_idx = -1;
lv_timer_t *g_download_timer = nullptr;

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
static bool file_exists_strict(const String &path)
{
	return FileSystem::exists(path) && FileSystem::readFile(path).length() > 0;
}

static void library_fetch_timer_cb(lv_timer_t *t)
{
	if (g_remote_fetch_done || g_fetch_in_progress)
	{
		return;
	}
	g_fetch_in_progress = true;
	
	AsyncManager::fetchCatalog([](bool success) {
		g_remote_fetch_done = true;
		g_remote_fetch_failed = !success || remote_catalog::entries().empty();
		g_fetch_in_progress = false;
		
		if (g_fetch_overlay)
		{
			lv_obj_del(g_fetch_overlay);
			g_fetch_overlay = nullptr;
		}
		
		story::loadFromFS();
		ui_library_screen_show();
		
		if (g_fetch_timer)
		{
			lv_timer_del(g_fetch_timer);
			g_fetch_timer = nullptr;
		}
	});
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
	ui_story_screen_show(stories[g_story_idx], g_node_key);
}
static void on_remote_entry(lv_event_t *e)
{
	intptr_t ridx = (intptr_t)lv_event_get_user_data(e);
	const auto &ents = remote_catalog::entries();
	if (ridx < 0 || (size_t)ridx >= ents.size()) {
		return;
	}
	g_pending_download_idx = (int)ridx;
	g_pending_download_file = ents[ridx].file;

	if (!g_download_overlay)
	{
		lv_obj_t *scr = lv_scr_act();
		g_download_overlay = create_loading_overlay(scr, S()->loading);
	}

	if (g_download_timer)
	{
		lv_timer_reset(g_download_timer);
	}
	else
	{
		String file = g_pending_download_file;
		g_pending_download_file.clear();
		
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
			
			if (storyId.length()) {
				story::loadFromFS();
				const auto &stories = story::all();
				for (const auto &st : stories) {
					if (st.id == storyId) {
						ui_story_screen_show(st, st.start);
						return;
					}
				}
			}
			ui_library_screen_show();
		});
	}
}

extern void ui_app_before_screen_change();
void ui_library_screen_show()
{
	ui_app_before_screen_change();
	lv_obj_t *scr = lv_scr_act();
	lv_obj_clean(scr);
	g_fetch_overlay = nullptr;
	const auto *s = S();
	apply_screen_bg(scr);
	lv_obj_t *panel = lv_obj_create(scr);
	lv_obj_set_size(panel, SCREEN_WIDTH, SCREEN_HEIGHT);
	lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_color(panel, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(panel, 0, 0);
	lv_obj_set_style_pad_all(panel, 0, 0);
	
	ui_header_config_t header_config = ui_header_config_default(s->stories_title, on_back_button_clicked);
	lv_obj_t *header = ui_header_create(scr, &header_config);
	lv_obj_t *list = lv_obj_create(scr);
	lv_obj_set_size(list, 240, 262);
	lv_obj_align_to(list, header, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
	lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(list, 0, 0);
	lv_obj_set_style_outline_width(list, 0, 0);
	lv_obj_set_style_pad_left(list, 2, 0);
	lv_obj_set_style_pad_right(list, 2, 0);
	lv_obj_set_style_pad_top(list, 2, 0);
	lv_obj_set_style_pad_bottom(list, 2, 0);
	lv_obj_set_style_pad_row(list, 2, 0);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_style_max_height(list, 262, 0);
	
	const auto &storiesNow = story::all();
	const auto &ents = remote_catalog::entries();
	
	bool any_local = !storiesNow.empty();
	bool any_remote = false;
	
	for (size_t i = 0; i < storiesNow.size(); ++i)
	{
		const Story_t &st = storiesNow[i];
		lv_obj_t *btn = lv_btn_create(list);
		ui_add_click_sound(btn);
		lv_obj_set_width(btn, LV_PCT(100));
		lv_obj_set_height(btn, 36);
		apply_primary_button_style(btn);
		lv_obj_add_event_cb(btn, on_story_select, LV_EVENT_CLICKED, (void *)i);
		lv_obj_t *lbl = lv_label_create(btn);
		lv_label_set_text(lbl, st.title.length() ? st.title.c_str() : st.id.c_str());
		lv_obj_set_style_text_font(lbl, font16(), 0);
		lv_obj_center(lbl);
	}
	
	if (online_mode) {
		std::set<String> shown_file_lang;
		
		for (size_t i = 0; i < ents.size(); ++i)
		{
			const auto &ent = ents[i];
			if (!story_utils::matchesLanguage(current_language, ent.lang)) continue;
			
			String unique_key = ent.file + "|" + ent.lang;
			if (shown_file_lang.count(unique_key)) continue;
			
			shown_file_lang.insert(unique_key);
			String label = ent.name.length() ? ent.name : ent.file;
			
			bool found_local = false;
			String localPath = "/" + ent.file;
			
			// Check if file exists and is in the index with matching language
			if (FileSystem::exists(localPath)) {
				String payload = FileSystem::readFile(localPath);
				if (payload.length() > 0) {
					JsonDocument doc;
					if (deserializeJson(doc, payload) == DeserializationError::Ok) {
						String lang = doc["lang"].as<String>();
						if (lang == ent.lang && FileSystem::indexContains(localPath)) {
							found_local = true;
						}
					}
				}
			}
			
			if (!found_local)
			{
				any_remote = true;
				lv_obj_t *btn = lv_btn_create(list);
				ui_add_click_sound(btn);
				lv_obj_set_width(btn, LV_PCT(100));
				lv_obj_set_height(btn, 34);
				apply_primary_button_style(btn);
				lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
				lv_obj_add_event_cb(btn, on_remote_entry, LV_EVENT_CLICKED, (void *)i);
				lv_obj_t *lbl = lv_label_create(btn);
				lv_label_set_text(lbl, label.c_str());
				lv_obj_set_style_text_font(lbl, font14(), 0);
				lv_obj_center(lbl);
			}
		}
	} else {
		g_remote_fetch_done = false;
		g_remote_fetch_failed = false;
		if (g_fetch_timer) {
			lv_timer_del(g_fetch_timer);
			g_fetch_timer = nullptr;
		}
		if (g_fetch_overlay) {
			lv_obj_del(g_fetch_overlay);
			g_fetch_overlay = nullptr;
		}
	}
	
	// If nothing to show, display 'no stories' message
	if (!any_local && !any_remote)
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
		lv_obj_set_style_text_color(msg_lbl, lv_palette_main(LV_PALETTE_GREY), 0);
		lv_obj_set_style_text_align(msg_lbl, LV_TEXT_ALIGN_CENTER, 0);
		
		// Additional helpful text
		lv_obj_t *help_lbl = lv_label_create(empty_container);
		lv_label_set_text(help_lbl, online_mode ? s->stories_will_appear : s->enable_online_mode);
		lv_obj_set_style_text_font(help_lbl, font12(), 0);
		lv_obj_set_style_text_color(help_lbl, lv_palette_main(LV_PALETTE_GREY), 0);
		lv_obj_set_style_text_align(help_lbl, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_margin_top(help_lbl, 8, 0);
		lv_label_set_long_mode(help_lbl, LV_LABEL_LONG_WRAP);
		lv_obj_set_width(help_lbl, LV_PCT(80));
	}	// Online mode phased loading with states
	if (online_mode)
	{
		if (!g_remote_fetch_done)
		{
			// Full-screen overlay with spinner
			if (!g_fetch_overlay)
			{
				g_fetch_overlay = create_loading_overlay(scr, S()->loading);
			}
			if (!g_fetch_timer)
			{
				g_fetch_timer = lv_timer_create(library_fetch_timer_cb, 50, nullptr);
			}
		}
		else if (g_remote_fetch_failed)
		{
			lv_obj_t *btn_retry = lv_btn_create(list);
			ui_add_click_sound(btn_retry);
			lv_obj_set_width(btn_retry, LV_PCT(100));
			lv_obj_set_height(btn_retry, 36);
			apply_primary_button_style(btn_retry);
			lv_obj_set_style_bg_color(btn_retry, lv_palette_main(LV_PALETTE_RED), 0);
			lv_obj_add_event_cb(btn_retry, [](lv_event_t *e)
			{
				g_remote_fetch_done = false;
				g_remote_fetch_failed = false;
				remote_catalog::invalidate();
				if (g_fetch_timer)
				{
					lv_timer_del(g_fetch_timer);
					g_fetch_timer = nullptr;
				}
				ui_library_screen_show();
			}, LV_EVENT_CLICKED, nullptr);
			lv_obj_t *lblr = lv_label_create(btn_retry);
			lv_label_set_text(lblr, S()->retry_online_fetch);
			lv_obj_center(lblr);
		}
		else
		{
			int newly = remote_catalog::reconcileExisting();
			if (newly > 0)
			{
				story::loadFromFS();
				ui_library_screen_show();
				return;
			}
		}
	}
}
