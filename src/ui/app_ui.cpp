#include <Arduino.h>
#include "app_ui.h"
#include <Preferences.h>
#include "styles.h"
#include "i18n.h"
#include "config.h"
#include "ui/fonts.h"
#include "remote_catalog.h"
#include "audio.h"
#include "ui/images/images.h"
#include "screens/ui_screens.h"

extern Preferences prefs;
extern uint8_t brightness;
extern uint8_t story_font_scale;
extern bool online_mode;
extern Language current_language;

static lv_obj_t *home_bg_img = nullptr;
static lv_obj_t *home_lbl_stories = nullptr;
static lv_obj_t *home_lbl_tagline = nullptr;

static void forget_home_objects(){
	home_bg_img = nullptr;
	home_lbl_stories = nullptr;
	home_lbl_tagline = nullptr;
}

// Callers clean the screen next, so just drop the pointers and recreate later
void ui_app_before_screen_change(){ forget_home_objects(); }

// --- Home Screen ---
static void open_settings(lv_event_t *){ ui_settings_screen_show(); }
static void open_library(lv_event_t *){ ui_library_screen_show(); }

static void ensure_home_background(lv_obj_t *parent){
	if(home_bg_img) return;
	home_bg_img = lv_image_create(parent);
	// The home objects share a screen, so they are deleted together
	lv_obj_add_event_cb(home_bg_img, [](lv_event_t *e){
		if(lv_event_get_target(e) == home_bg_img) forget_home_objects();
	}, LV_EVENT_DELETE, nullptr);
	// Night scene with the logo, drawn by scripts/gen_home.py
	lv_image_set_src(home_bg_img, &image_home);
	lv_obj_set_size(home_bg_img, SCREEN_WIDTH, SCREEN_HEIGHT);
	lv_obj_align(home_bg_img, LV_ALIGN_CENTER, 0, 0);
	lv_obj_move_background(home_bg_img);
}

void ui_app_show_home(){
	lv_obj_t *scr = lv_scr_act();
	if(!home_bg_img){
		lv_obj_clean(scr);
		apply_screen_bg(scr);
		ensure_home_background(scr);

		// Under the logo in the picture
		home_lbl_tagline = lv_label_create(scr);
		lv_obj_set_style_text_font(home_lbl_tagline, font14(), 0);
		lv_obj_set_style_text_color(home_lbl_tagline, lv_color_hex(0xF3DDB8), 0);
		lv_obj_align(home_lbl_tagline, LV_ALIGN_TOP_MID, 0, 152);

		// One clear main action; Settings is for grown-ups, so it stays small
		lv_obj_t *btn_stories = lv_btn_create(scr);
		ui_add_click_sound(btn_stories);
		apply_primary_button_style(btn_stories);
		lv_obj_set_size(btn_stories, 184, 46);
		lv_obj_set_style_radius(btn_stories, LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_shadow_width(btn_stories, 18, 0);
		lv_obj_set_style_shadow_color(btn_stories, palette::primary(), 0);
		lv_obj_set_style_shadow_opa(btn_stories, LV_OPA_40, 0);
		lv_obj_align(btn_stories, LV_ALIGN_BOTTOM_MID, 0, -14);
		lv_obj_add_event_cb(btn_stories, open_library, LV_EVENT_CLICKED, nullptr);
		home_lbl_stories = lv_label_create(btn_stories);
		lv_obj_set_style_text_font(home_lbl_stories, font20(), 0);
		lv_obj_center(home_lbl_stories);

		lv_obj_t *btn_settings = lv_btn_create(scr);
		ui_add_click_sound(btn_settings);
		lv_obj_set_size(btn_settings, 36, 36);
		lv_obj_set_style_radius(btn_settings, LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_bg_color(btn_settings, lv_color_white(), 0);
		lv_obj_set_style_bg_opa(btn_settings, LV_OPA_20, 0);
		lv_obj_set_style_bg_opa(btn_settings, LV_OPA_40, LV_STATE_PRESSED);
		lv_obj_set_style_shadow_width(btn_settings, 0, 0);
		lv_obj_set_style_pad_all(btn_settings, 0, 0);
		lv_obj_align(btn_settings, LV_ALIGN_TOP_LEFT, 8, 8);
		lv_obj_add_event_cb(btn_settings, open_settings, LV_EVENT_CLICKED, nullptr);
		lv_obj_t *gear = lv_label_create(btn_settings);
		lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
		lv_obj_set_style_text_color(gear, lv_color_hex(0xFFF3D6), 0);
		lv_obj_center(gear);

		ui_app_refresh_home_labels();
	}else{
		lv_obj_move_background(home_bg_img);
		ui_app_refresh_home_labels();
	}
}

void ui_app_refresh_home_labels(){
	if(home_lbl_stories) lv_label_set_text(home_lbl_stories, S()->stories_btn);
	if(home_lbl_tagline) lv_label_set_text(home_lbl_tagline, S()->home_tagline);
}

extern "C" void create_settings_window_external(){}
