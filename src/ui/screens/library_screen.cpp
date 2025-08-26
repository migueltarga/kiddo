
#include <Arduino.h>
#include <lvgl.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <set>
#include <SPIFFS.h>
#include "remote_catalog.h"
#include "ui_screens.h"
#include "styles.h"
#include "ui/fonts.h"
#include "story_engine.h"
#include "config.h"
#include "storage.h"
#include "audio.h"
#include "ui/components/ui_components.h"
#include "ui/router.h"
// Global state variables
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

#include "i18n.h" // Must be after externs for Language, S(), etc.
extern void ui_library_screen_refresh_for_language_change();
extern void ui_library_screen_show();
extern Language current_language;
extern bool online_mode;
#include "storage.h"
#include "audio.h"
static bool file_exists_strict(const String &path)
{
	if (!storage::exists(path.c_str()))
	{
		return false;
	}
	File f = SPIFFS.open(path.c_str(), "r");
	if (!f)
	{
		return false;
	}
	size_t sz = f.size();
	f.close();
	return sz > 0;
}

static void library_fetch_timer_cb(lv_timer_t *t)
{
	if (g_remote_fetch_done || g_fetch_in_progress)
	{
		return;
	}
	g_fetch_in_progress = true;
	bool ok = remote_catalog::fetch();
	g_remote_fetch_done = true;
	g_remote_fetch_failed = !ok || remote_catalog::entries().empty();
	g_fetch_in_progress = false;
	// Remove fetch overlay if present
	if (g_fetch_overlay)
	{
		lv_obj_del(g_fetch_overlay);
		g_fetch_overlay = nullptr;
	}
	// Rebuild UI after fetch
	story::loadFromFS();
	ui_library_screen_show();
	if (g_fetch_timer)
	{
		lv_timer_del(g_fetch_timer);
		g_fetch_timer = nullptr;
	}
}
void ui_story_set_home_cb(ui_story_home_cb_t cb) { g_home_cb = cb; }

// Back button callback for header component
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
		g_download_overlay = lv_obj_create(scr);
		lv_obj_remove_style_all(g_download_overlay);
		lv_obj_set_size(g_download_overlay, LV_PCT(100), LV_PCT(100));
		lv_obj_set_style_bg_color(g_download_overlay, lv_color_black(), 0);
		lv_obj_set_style_bg_opa(g_download_overlay, LV_OPA_50, 0);
		lv_obj_center(g_download_overlay);
		lv_obj_add_flag(g_download_overlay, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_t *box = lv_obj_create(g_download_overlay);
		lv_obj_set_size(box, 140, 100);
		lv_obj_center(box);
		lv_obj_set_style_bg_color(box, lv_palette_main(LV_PALETTE_ORANGE), 0);
		lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
		lv_obj_set_style_radius(box, 12, 0);
		lv_obj_set_style_pad_all(box, 8, 0);
		lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_t *spinner = lv_spinner_create(box);
		lv_obj_set_size(spinner, 32, 32);
		lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 4);
		lv_obj_t *lbl = lv_label_create(box);
		lv_label_set_text(lbl, S()->loading);
		lv_obj_set_style_text_font(lbl, font14(), 0);
		lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
		lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -4);
	}

	if (g_download_timer)
	{
		lv_timer_reset(g_download_timer);
	}
	else
	{
		g_download_timer = lv_timer_create([](lv_timer_t *t)
										   {
			if (g_pending_download_file.length() == 0) {
				if (g_download_timer) {
					lv_timer_del(g_download_timer);
					g_download_timer = nullptr;
				}
				return;
			}
			String file = g_pending_download_file;
			String storyId;
			bool ok = remote_catalog::ensureDownloadedOrIndexed(file, &storyId);
			if (g_download_overlay)
			{
				lv_obj_del(g_download_overlay);
				g_download_overlay = nullptr;
			}
			g_pending_download_file.clear();

			if (!ok)
			{
				ui_splash_screen_show(S()->delete_failed);
				lv_timer_create([](lv_timer_t *tt)
								{
									ui_library_screen_show();
									lv_timer_del(tt);
								},
								800, nullptr);
				if (g_download_timer) {
					lv_timer_del(g_download_timer);
					g_download_timer = nullptr;
				}
				return;
			}
			if (storyId.length())
			{
				story::loadFromFS();
				const auto &stories = story::all();
				for (const auto &st : stories)
				{
					if (st.id == storyId)
					{
						ui_story_screen_show(st, st.start);
						if (g_download_timer) {
							lv_timer_del(g_download_timer);
							g_download_timer = nullptr;
						}
						return;
					}
				}
			}
			ui_library_screen_show();
			if (g_download_timer) {
				lv_timer_del(g_download_timer);
				g_download_timer = nullptr;
			}
		}, 25, nullptr);
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
	
	// Create header using component
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
	
	// Show local stories first
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
	
	// Only process remote catalog if online mode is enabled
	if (online_mode) {
		// Show remote catalog entries for download (if missing locally)
		std::set<String> shown_file_lang;
		
		for (size_t i = 0; i < ents.size(); ++i)
		{
			const auto &ent = ents[i];
			// Strictly filter by language BEFORE deduplication
			bool lang_match = false;
			if (current_language == LANG_PT && ent.lang == "pt-br") lang_match = true;
			if (current_language == LANG_EN && ent.lang == "en") lang_match = true;
			if (!lang_match) continue;
			
			String unique_key = ent.file + "|" + ent.lang;
			if (shown_file_lang.count(unique_key)) continue; // Only one button per file+lang
			
			shown_file_lang.insert(unique_key);
			String label = ent.name.length() ? ent.name : ent.file;
			
			// Check for a local story with matching file and language
			bool found_local = false;
			String localPath = "/" + ent.file;
			if (file_exists_strict(localPath)) {
				String payload = storage::readFileToString(localPath.c_str());
				if (payload.length() > 0) {
					JsonDocument doc;
					if (deserializeJson(doc, payload) == DeserializationError::Ok) {
						String lang = doc["lang"].as<String>();
						if (lang == ent.lang) {
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
		// Clean up remote fetch state when online mode is disabled
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
				g_fetch_overlay = lv_obj_create(scr);
				lv_obj_remove_style_all(g_fetch_overlay);
				lv_obj_set_size(g_fetch_overlay, LV_PCT(100), LV_PCT(100));
				lv_obj_set_style_bg_color(g_fetch_overlay, lv_color_black(), 0);
				lv_obj_set_style_bg_opa(g_fetch_overlay, LV_OPA_50, 0);
				lv_obj_center(g_fetch_overlay);
				lv_obj_add_flag(g_fetch_overlay, LV_OBJ_FLAG_CLICKABLE);
				lv_obj_t *box = lv_obj_create(g_fetch_overlay);
				lv_obj_set_size(box, 140, 100);
				lv_obj_center(box);
				lv_obj_set_style_bg_color(box, lv_palette_main(LV_PALETTE_ORANGE), 0);
				lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
				lv_obj_set_style_radius(box, 12, 0);
				lv_obj_set_style_pad_all(box, 8, 0);
				lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
				lv_obj_t *spinner = lv_spinner_create(box);
				lv_obj_set_size(spinner, 32, 32);
				lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 4);
				lv_obj_t *lbl = lv_label_create(box);
				lv_label_set_text(lbl, S()->loading);
				lv_obj_set_style_text_font(lbl, font14(), 0);
				lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
				lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -4);
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
