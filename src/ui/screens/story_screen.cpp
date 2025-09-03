/**
 * @file story_screen.cpp
 * @brief Story display screen implementation
 *
 * This module handles the story display screen, including text rendering,
 * choice navigation, inventory management, and image display. It manages
 * the main story reading experience and user interactions.
 */

#include <lvgl.h>

#include "audio.h"
#include "config.h"
#include "i18n.h"
#include "story_engine.h"
#include "file_system.h"
#include "async_manager.h"
#include "image_display.h"
#include "styles.h"
#include "inventory_manager.h"
#include "ui/components/ui_components.h"
#include "ui/components/inventory_ui.h"
#include "ui/fonts.h"
#include "ui_screens.h"
#include "kiddo_parser.h"

extern void ui_library_screen_show();
extern void ui_story_set_home_cb(void (*cb)());

/**
 * @brief Safe wrapper for library screen show
 * Cleans up inventory UI before showing library
 */
static void safe_ui_library_screen_show() {
	InventoryUI::cleanup();
	ui_library_screen_show();
}

namespace
{
	/** @brief Current story node ID */
	String g_current_node;
	/** @brief Flag indicating if story progress has been made */
	bool g_story_progress_made = false;
	/** @brief Current story being displayed */
	const Story_t *g_story = nullptr;
	lv_obj_t *g_inventory_btn = nullptr;
	
	void onItemAdded(const InventoryItem_t& item) {
		InventoryUI::notifyItemAdded(item, true);
	}
}

extern uint8_t story_font_scale;
static const lv_font_t *story_body_font()
{
	switch (story_font_scale)
	{
	case 0:
		return font14();
	case 2:
		return font20();
	default:
		return font16();
	}
}

static lv_coord_t measure_text_width(const char *txt, const lv_font_t *font)
{
	if (!txt || !*txt)
		return 0;
	lv_point_t sz;
	lv_txt_get_size(&sz, txt, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
	return sz.x;
}

static bool should_wrap_choice(const String &text)
{
	const lv_coord_t max_line_width = 200;
	lv_coord_t w = measure_text_width(text.c_str(), story_body_font());
	if (story_font_scale == 2)
		return w > max_line_width;
	return w > (max_line_width + 40);
}

static void show_end_fullscreen()
{
	lv_obj_t *scr = lv_scr_act();
	lv_obj_clean(scr);
	const auto *s = S();
	lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_t *lbl = lv_label_create(scr);
	lv_label_set_text(lbl, s->the_end);
	lv_obj_set_style_text_font(lbl, font20(), 0);
	lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), 0);
	lv_obj_center(lbl);
	lv_obj_add_event_cb(
		scr,
		[](lv_event_t *e)
		{
			if (lv_event_get_code(e) == LV_EVENT_CLICKED)
				ui_library_screen_show();
		},
		LV_EVENT_CLICKED, nullptr);
}

static void show_node(const String &key)
{
	if (!g_story)
		return;
	const Node_t *n = g_story->get(key);
	if (!n)
	{
		ui_library_screen_show();
		return;
	}
	g_current_node = key;
	
	if (g_story && key != g_story->start) {
		g_story_progress_made = true;
	}
	
	lv_obj_t *scr = lv_scr_act();
	lv_obj_clean(scr);
	const auto *s = S();
	lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	
	auto on_back_clicked = [](lv_event_t *e) { 
		InventoryUI::cleanup();
		
		if (g_story_progress_made) {
			const auto *s = S();
			ConfirmationConfig config;
			config.title = s->leave_story_title;
			config.message = s->leave_story_message;
			config.confirm_text = s->leave_story_confirm;
			config.cancel_text = s->leave_story_cancel;
			config.on_confirm = safe_ui_library_screen_show; // Use wrapper function
			ui_confirmation_dialog_show(config);
		} else {
			safe_ui_library_screen_show();
		}
	};
	ui_header_config_t config = ui_header_config_default(g_story->title.c_str(), on_back_clicked);
	config.enable_marquee = true;
	
	lv_obj_t *header = ui_header_create(scr, &config);
	int header_h = 44;
	
	g_inventory_btn = nullptr;
	if (g_story->has_inventory) {
		g_inventory_btn = InventoryUI::createInventoryButton(header);
		
		lv_obj_remove_flag(g_inventory_btn, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
		lv_obj_set_pos(g_inventory_btn, 240 - 32 - 6, (38 - 32) / 2); // x = header_width - button_width - padding, y = centered
		
		lv_obj_t *title_label = (lv_obj_t *)lv_obj_get_user_data(header);
		if (title_label) {
			lv_obj_set_width(title_label, 150); // Increased from 130 to 160 for better layout
		}
	}
	
	if (g_story->has_inventory && n->gives_item.length() > 0) {
		const ItemDefinition_t* item_def = g_story->getItemDefinition(n->gives_item);
		if (item_def) {
			InventoryItem_t item(item_def->id, item_def->name, item_def->icon_url);
			InventoryManager::getCurrentInventory().addItem(item, true); // Enable notifications
		} else {
			InventoryItem_t basic_item(n->gives_item, n->gives_item, "");
			InventoryManager::getCurrentInventory().addItem(basic_item, true);
		}
	}
	
	lv_coord_t content_h = SCREEN_HEIGHT - header_h;
	lv_obj_t *content = lv_obj_create(scr);
	lv_obj_remove_style_all(content);
	lv_obj_set_size(content, 240, content_h);
	lv_obj_align(content, LV_ALIGN_TOP_MID, 0, header_h);
	lv_obj_set_style_bg_color(content, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
						  LV_FLEX_ALIGN_START);
	lv_obj_t *text_wrap = lv_obj_create(content);
	lv_obj_remove_style_all(text_wrap);
	lv_obj_set_style_bg_opa(text_wrap, LV_OPA_TRANSP, 0);
	lv_obj_set_width(text_wrap, 240);
	lv_obj_set_style_pad_left(text_wrap, 6, 0);
	lv_obj_set_style_pad_right(text_wrap, 6, 0);
	lv_obj_set_style_pad_top(text_wrap, 4, 0);
	lv_obj_set_style_pad_bottom(text_wrap, 4, 0);
	lv_obj_set_scroll_dir(text_wrap, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(text_wrap, LV_SCROLLBAR_MODE_AUTO);
	lv_obj_set_flex_flow(text_wrap, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(text_wrap, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
						  LV_FLEX_ALIGN_START);
	lv_obj_set_flex_grow(text_wrap, 1);
	
	KiddoParser::ParsedContent parsed = KiddoParser::parseText(n->text);
	
	for (const auto& segment : parsed.segments) {
		if (segment.type == KiddoParser::ContentSegment::TEXT) {
			if (segment.content.length() > 0) {
				lv_obj_t *text_label = lv_label_create(text_wrap);
				lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
				lv_obj_set_width(text_label, 228);
				lv_label_set_text(text_label, segment.content.c_str());
				lv_obj_set_style_text_font(text_label, story_body_font(), 0);
				lv_obj_set_style_text_color(text_label, lv_color_hex(0x000000), 0);
			}
		} else if (segment.type == KiddoParser::ContentSegment::IMAGE) {
			lv_obj_t *img_wrapper = lv_obj_create(text_wrap);
			lv_obj_remove_style_all(img_wrapper);
			lv_obj_set_style_bg_opa(img_wrapper, LV_OPA_TRANSP, 0);
			lv_obj_set_width(img_wrapper, 228);
			lv_obj_set_height(img_wrapper, 140);
			lv_obj_set_style_pad_bottom(img_wrapper, 6, 0);
			lv_obj_set_style_pad_top(img_wrapper, 3, 0);
			lv_obj_set_style_pad_left(img_wrapper, 0, 0);
			lv_obj_set_style_pad_right(img_wrapper, 0, 0);
			lv_obj_set_scroll_dir(img_wrapper, LV_DIR_NONE);
			lv_obj_set_flex_flow(img_wrapper, LV_FLEX_FLOW_COLUMN);
			lv_obj_set_flex_align(img_wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			
			lv_obj_t *img = lv_img_create(img_wrapper);
			lv_obj_set_width(img, 220);
			lv_obj_set_height(img, 130);
			lv_obj_set_style_pad_all(img, 0, 0);

			ImageDisplay::createLoadingPlaceholder(img);

			AsyncManager::loadImage(segment.content, img, [img](bool success, const String& cachedPath) {
				if (success) {
				} else {
				}
			});
		}
	}
	lv_obj_t *choices = lv_obj_create(content);
	lv_obj_remove_style_all(choices);
	lv_obj_set_width(choices, 240);
	lv_obj_set_style_bg_color(choices, lv_color_hex(0xe9d4a9), 0);
	lv_obj_set_style_bg_opa(choices, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(choices, 6, 0);
	lv_obj_set_style_pad_row(choices, 6, 0);
	lv_obj_set_style_border_width(choices, 0, 0);
	lv_obj_set_flex_flow(choices, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(choices, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
						  LV_FLEX_ALIGN_START);
	if (n->is_end || n->choices.empty())
	{
		lv_obj_t *b = lv_btn_create(choices);
		ui_add_click_sound(b);
		ui_add_click_sound(b);
		lv_obj_set_width(b, LV_PCT(100));
		bool wrap = should_wrap_choice(String(S()->end_next));
		if (wrap)
		{
			lv_obj_set_height(b, LV_SIZE_CONTENT);
			lv_obj_set_style_min_height(b, 34, 0);
		}
		else
		{
			lv_obj_set_height(b, 34);
		}
		apply_primary_button_style(b);
		lv_obj_add_event_cb(
			b, [](lv_event_t *e)
			{ show_end_fullscreen(); }, LV_EVENT_CLICKED,
			nullptr);
		lv_obj_t *l = lv_label_create(b);
		lv_label_set_text(l, S()->end_next);
		lv_obj_set_style_text_font(l, story_body_font(), 0);
		if (wrap)
		{
			lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
			lv_obj_set_width(l, 200);
		}
		lv_obj_center(l);
	}
	else
	{
		for (const auto &ch : n->choices)
		{
			// Check inventory requirements
			if (g_story->has_inventory) {
				bool has_required_item = InventoryManager::getCurrentInventory().hasItem(ch.required_item);
				
				// Skip hidden choices if requirement not met
				if (ch.hidden_without_item && !has_required_item) {
					continue;
				}
			}
			
			lv_obj_t *b = lv_btn_create(choices);
			ui_add_click_sound(b);
			ui_add_click_sound(b);
			lv_obj_set_width(b, LV_PCT(100));
			bool wrap = should_wrap_choice(ch.text);
			if (wrap)
			{
				lv_obj_set_height(b, LV_SIZE_CONTENT);
				lv_obj_set_style_min_height(b, 34, 0);
			}
			else
			{
				lv_obj_set_height(b, 34);
			}
			
			// Check if choice should be disabled due to missing inventory item
			bool is_disabled = false;
			if (g_story->has_inventory && ch.required_item.length() > 0) {
				is_disabled = !InventoryManager::getCurrentInventory().hasItem(ch.required_item);
			}
			
			if (is_disabled) {
				// Apply disabled styling
				lv_obj_set_style_bg_color(b, lv_color_hex(0xcccccc), 0);
				lv_obj_set_style_text_color(b, lv_color_hex(0x888888), 0);
				lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);
			} else {
				apply_primary_button_style(b);
			}
			
			// Store choice data for callback
			struct ChoiceData {
				String next;
				String gives_item;
				String required_item;
			};
			ChoiceData* choice_data = new ChoiceData{ch.next, ch.gives_item, ch.required_item};
			
			if (!is_disabled) {
				lv_obj_add_event_cb(
					b,
					[](lv_event_t *e)
					{
						ChoiceData* data = (ChoiceData*)lv_event_get_user_data(e);
						if (!data)
							return;

						if (g_story->has_inventory && data->required_item.length() > 0) {
							bool removed = InventoryManager::getCurrentInventory().removeItem(data->required_item);
						}
						
						String key = data->next;
						delete data;
						show_node(key);
					},
					LV_EVENT_CLICKED, choice_data);
			}
			
			lv_obj_t *l = lv_label_create(b);
			lv_label_set_text(l, ch.text.c_str());
			lv_obj_set_style_text_font(l, story_body_font(), 0);
			if (wrap)
			{
				lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
				lv_obj_set_width(l, 200);
			}
			lv_obj_center(l);
		}
	}
	
	if (n->inventory_choice && g_story->has_inventory) {
		Serial.println("Displaying inventory choice dialog");
		
		lv_obj_t *inv_btn = lv_btn_create(choices);
		ui_add_click_sound(inv_btn);
		lv_obj_set_width(inv_btn, LV_PCT(100));
		lv_obj_set_height(inv_btn, 34);
		apply_primary_button_style(inv_btn);
		
		struct InventoryChoiceData {
			String correct_item;
			String success_next;
			String failure_next;
		};
		InventoryChoiceData* inv_data = new InventoryChoiceData{n->correct_item, n->success_next, n->failure_next};
		
		lv_obj_add_event_cb(inv_btn, [](lv_event_t *e) {
			InventoryChoiceData* data = (InventoryChoiceData*)lv_event_get_user_data(e);
			if (!data) return;
			
			InventoryUI::InventoryDialogConfig config;
			config.title = "Choose Item";
			config.show_for_choice = true;
			config.choice_callback = [data](const InventoryItem_t& selected_item) {
				Serial.printf("Inventory choice made: %s (correct: %s)\n", selected_item.id.c_str(), data->correct_item.c_str());
				
				String next_node;
				if (selected_item.id == data->correct_item) {
					Serial.println("Correct item chosen!");
					next_node = data->success_next;
				} else {
					Serial.println("Wrong item chosen!");
					next_node = data->failure_next;
				}
				
				// Remove the chosen item from inventory
				InventoryManager::getCurrentInventory().removeItem(selected_item.id);
				
				// Navigate to next node
				show_node(next_node);
				
				// Clean up
				delete data;
			};
			InventoryUI::showInventoryDialog(config);
		}, LV_EVENT_CLICKED, inv_data);
		
		lv_obj_t *inv_label = lv_label_create(inv_btn);
		lv_label_set_text(inv_label, S()->choose_item);
		lv_obj_set_style_text_font(inv_label, story_body_font(), 0);
		lv_obj_center(inv_label);
	}
	
	uint32_t choice_cnt = lv_obj_get_child_cnt(choices);
	int base_pad = 6;
	int row_space = 6;
	int btn_h = 34;
	int desired =
		base_pad * 2 + (choice_cnt > 0 ? (btn_h * (int)choice_cnt +
										  row_space * ((int)choice_cnt - 1))
									   : 0);
	int max_compact_two =
		base_pad * 2 +
		(btn_h * 2 + row_space);
	int max_cap = 120;
	
	if (choice_cnt <= 2)
	{
		lv_obj_set_height(choices, desired);
		lv_obj_clear_flag(choices, LV_OBJ_FLAG_SCROLLABLE);
	}
	else if (choice_cnt == 3)
	{
		int three_choice_height = base_pad * 2 + (btn_h * 3 + row_space * 2);
		if (three_choice_height <= max_cap + 10) {
			lv_obj_set_height(choices, three_choice_height);
			lv_obj_clear_flag(choices, LV_OBJ_FLAG_SCROLLABLE);
		} else {
			lv_obj_set_height(choices, max_cap);
			lv_obj_set_scroll_dir(choices, LV_DIR_VER);
			lv_obj_add_flag(choices, LV_OBJ_FLAG_SCROLLABLE);
		}
	}
	else
	{
		lv_obj_set_height(choices, max_cap);
		lv_obj_set_scroll_dir(choices, LV_DIR_VER);
		lv_obj_add_flag(choices, LV_OBJ_FLAG_SCROLLABLE);
	}
	lv_obj_move_foreground(choices);
}

void ui_story_screen_show(const Story_t &st, const String &nodeKey)
{
	g_story = &st;
	
	g_story_progress_made = false;
	
	if (g_story->has_inventory) {
		InventoryManager::getCurrentInventory().initialize(g_story->initial_inventory);
		InventoryManager::getCurrentInventory().setNotificationCallback(onItemAdded);
	}
	
	show_node(nodeKey);
}
void ui_story_screen_refresh()
{
	if (g_story && g_current_node.length())
		show_node(g_current_node);
}
