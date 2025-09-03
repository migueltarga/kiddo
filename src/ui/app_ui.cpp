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
static lv_obj_t *home_btn_settings = nullptr;
static lv_obj_t *home_btn_stories = nullptr;
static lv_obj_t *home_lbl_settings = nullptr;
static lv_obj_t *home_lbl_stories = nullptr;

void ui_app_before_screen_change(){
	if(home_bg_img){
		home_bg_img = nullptr; // just drop pointer so we recreate next time
	}
}

// --- Home Screen ---
static void open_settings(lv_event_t *){ ui_settings_screen_show(); }
static void open_library(lv_event_t *){ ui_library_screen_show(); }

static void ensure_home_background(lv_obj_t *parent){
	if(home_bg_img) return;
	home_bg_img = lv_image_create(parent);
	lv_image_set_src(home_bg_img, &image_background);
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
		home_btn_settings = lv_btn_create(scr); ui_add_click_sound(home_btn_settings); apply_primary_button_style(home_btn_settings); lv_obj_set_size(home_btn_settings,100,44); lv_obj_align(home_btn_settings, LV_ALIGN_BOTTOM_LEFT, 8, -8); lv_obj_add_event_cb(home_btn_settings, open_settings, LV_EVENT_CLICKED, nullptr);
		home_lbl_settings = lv_label_create(home_btn_settings); lv_obj_set_style_text_font(home_lbl_settings, font16(),0); lv_label_set_text(home_lbl_settings, S()->settings_title); lv_obj_center(home_lbl_settings);
		home_btn_stories = lv_btn_create(scr); ui_add_click_sound(home_btn_stories); apply_primary_button_style(home_btn_stories); lv_obj_set_size(home_btn_stories,100,44); lv_obj_align(home_btn_stories, LV_ALIGN_BOTTOM_RIGHT, -8, -8); lv_obj_add_event_cb(home_btn_stories, open_library, LV_EVENT_CLICKED, nullptr);
		home_lbl_stories = lv_label_create(home_btn_stories); lv_obj_set_style_text_font(home_lbl_stories, font16(),0); lv_label_set_text(home_lbl_stories, S()->stories_btn); lv_obj_center(home_lbl_stories);
	}else{
		lv_obj_move_background(home_bg_img);
		ui_app_refresh_home_labels();
	}
}

void ui_app_refresh_home_labels(){
	if(home_lbl_settings) lv_label_set_text(home_lbl_settings, S()->settings_title);
	if(home_lbl_stories) lv_label_set_text(home_lbl_stories, S()->stories_btn);
}

void ui_app_set_story_font_scale(uint8_t scale){ story_font_scale = scale; ui_story_screen_refresh(); }
extern "C" void create_settings_window_external(){}
