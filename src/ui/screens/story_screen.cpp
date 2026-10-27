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
#include "ui/components/story_scene.h"
#include "ui/fonts.h"
#include "ui_screens.h"
#include "kiddo_parser.h"
#include "story_progress.h"
#include <set>

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
	/** @brief Current story being displayed (an owned copy, so reloading
	 *  the story list never leaves it dangling) */
	Story_t g_story;
	lv_obj_t *g_inventory_btn = nullptr;
	/** @brief Items picked up on the way to the next page, shown at its end */
	std::vector<InventoryItem_t> g_found_items;
	/** @brief Scene of the last page without a picture: its place, people and things carry over */
	StoryScene::Spec g_last_scene;
	/** @brief Figures on every scene of this story (Zoe and Ted) */
	std::vector<String> g_scene_leads;
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

/**
 * @brief Font for choice buttons: one step below the story text, so
 * choices wrap less and leave more room for the story
 */
static const lv_font_t *choice_font()
{
	return story_font_scale == 2 ? font16() : font14();
}

/** @brief Shortest choice button, for a single line of text */
static const lv_coord_t CHOICE_MIN_HEIGHT = 30;

/**
 * @brief Create a full-width choice button whose height fits its text
 *
 * The label always wraps to the button's inner width, so long choices
 * grow taller instead of running past the edge.
 *
 * @param parent Choices panel
 * @param text Button text
 * @return The button
 */
static lv_obj_t *create_choice_button(lv_obj_t *parent, const char *text)
{
	lv_obj_t *b = lv_btn_create(parent);
	ui_add_click_sound(b);
	apply_primary_button_style(b);
	lv_obj_set_width(b, LV_PCT(100));
	lv_obj_set_height(b, LV_SIZE_CONTENT);
	lv_obj_set_style_min_height(b, CHOICE_MIN_HEIGHT, 0);
	lv_obj_set_style_pad_hor(b, 8, 0);
	lv_obj_set_style_pad_ver(b, 4, 0);
	lv_obj_set_layout(b, LV_LAYOUT_FLEX);
	lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t *l = lv_label_create(b);
	lv_label_set_text(l, text);
	lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(l, LV_PCT(100));
	lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(l, choice_font(), 0);
	return b;
}

static void show_end_fullscreen()
{
	lv_obj_t *scr = lv_scr_act();
	lv_obj_clean(scr);
	const auto *s = S();
	apply_screen_bg(scr);
	lv_obj_t *lbl = lv_label_create(scr);
	lv_label_set_text(lbl, s->the_end);
	lv_obj_set_style_text_font(lbl, font20(), 0);
	lv_obj_set_style_text_color(lbl, palette::ink(), 0);
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

static void show_node(const String &key);

/** @brief Image URLs already queued for download in this story */
static std::set<String> g_prefetched_images;

/**
 * @brief Download the pictures of the pages the reader can go to next
 *
 * Started once the current page's own pictures are shown: a download needs
 * a lot of RAM, and decoding a picture at the same time could run short.
 * By the time a choice is tapped its picture is usually on disk.
 *
 * @param n Current node
 */
static void prefetch_next_images(const Node_t *n)
{
	std::vector<String> next;
	for (const auto &ch : n->choices)
		next.push_back(ch.next);
	if (n->inventory_choice) {
		next.push_back(n->success_next);
		next.push_back(n->failure_next);
	}

	for (const String &key : next) {
		const Node_t *target = g_story.get(key);
		if (!target)
			continue;
		for (const auto &segment : KiddoParser::parseText(target->text).segments) {
			if (segment.type != KiddoParser::ContentSegment::IMAGE)
				continue;
			const String &url = segment.content;
			if (g_prefetched_images.count(url) || FileSystem::isImageCached(url))
				continue;
			g_prefetched_images.insert(url);
			AsyncManager::loadImage(url, nullptr, nullptr);
		}
	}
}

/**
 * @brief Prefetch for a page, unless the reader has already left it
 * @param key Page the pictures were shown on
 */
static void prefetch_after_page(const String &key)
{
	extern bool wifi_connected;
	const Node_t *n = g_story.get(key);
	if (wifi_connected && n && key == g_current_node)
		prefetch_next_images(n);
}

/** @brief Heap data attached to a choice button, freed when the button is deleted */
struct ChoiceData {
	String next;
	String gives_item;
	String required_item;
};

/** @brief Heap data attached to an inventory-choice button */
struct InventoryChoiceData {
	String correct_item;
	String success_next;
	String failure_next;
};

template <typename T>
static void free_user_data_cb(lv_event_t *e)
{
	delete (T *)lv_event_get_user_data(e);
}

/**
 * @brief Add a story item to the inventory and queue its "found" card
 *
 * Items already carried are skipped, so revisiting a page or resuming a
 * saved story doesn't add a second copy.
 *
 * @param item_id Item ID from the story, falling back to the bare ID
 */
static void give_item(const String &item_id)
{
	InventoryManager::Inventory &inventory = InventoryManager::getCurrentInventory();
	if (inventory.hasItem(item_id))
		return;
	const ItemDefinition_t *item_def = g_story.getItemDefinition(item_id);
	InventoryItem_t item = item_def
		? InventoryItem_t(item_def->id, item_def->name, item_def->icon_url, item_def->description)
		: InventoryItem_t(item_id, item_id, "");
	if (inventory.addItem(item, false))
		g_found_items.push_back(item);
}

static void on_choice_clicked(lv_event_t *e)
{
	ChoiceData *data = (ChoiceData *)lv_event_get_user_data(e);
	if (!data)
		return;

	if (g_story.has_inventory && data->required_item.length() > 0) {
		InventoryManager::getCurrentInventory().removeItem(data->required_item);
	}
	// Before rendering, so choices on the next node see the new item
	if (g_story.has_inventory && data->gives_item.length() > 0) {
		give_item(data->gives_item);
	}

	// Copy before show_node(): it deletes this button, which frees data
	String key = data->next;
	show_node(key);
}

static void on_inventory_choice_clicked(lv_event_t *e)
{
	InventoryChoiceData *data = (InventoryChoiceData *)lv_event_get_user_data(e);
	if (!data)
		return;

	// Capture copies: by the time an item is chosen the button and its
	// data may be gone
	String correct_item = data->correct_item;
	String success_next = data->success_next;
	String failure_next = data->failure_next;

	InventoryUI::InventoryDialogConfig config;
	config.title = S()->choose_item;
	config.show_for_choice = true;
	config.choice_callback = [correct_item, success_next, failure_next](const InventoryItem_t &selected_item) {
		Serial.printf("Inventory choice made: %s (correct: %s)\n", selected_item.id.c_str(), correct_item.c_str());
		bool correct = selected_item.id == correct_item;
		audio::play(correct ? audio::Sound::Success : audio::Sound::Oops);
		// Only the right item is used up; a wrong guess keeps the item, so
		// trying something doesn't cost the reader a later puzzle
		if (correct)
			InventoryManager::getCurrentInventory().removeItem(selected_item.id);
		show_node(correct ? success_next : failure_next);
	};
	InventoryUI::showInventoryDialog(config);
}

static void show_node(const String &key)
{
	if (g_story.id.length() == 0)
		return;
	const Node_t *n = g_story.get(key);
	if (!n)
	{
		ui_library_screen_show();
		return;
	}
	g_current_node = key;
	
	if (key != g_story.start) {
		g_story_progress_made = true;
	}
	
	lv_obj_t *scr = lv_scr_act();
	lv_obj_clean(scr);
	const auto *s = S();
	apply_screen_bg(scr);
	
	auto on_back_clicked = [](lv_event_t *e) { 
		InventoryUI::cleanup();
		
		if (g_story_progress_made) {
			const auto *s = S();
			ConfirmationConfig config;
			config.title = s->leave_story_title;
			config.message = s->leave_story_message;
			config.confirm_text = s->leave_story_confirm;
			// Progress is saved, so leaving is not destructive
			config.confirm_color = palette::primary();
			config.cancel_text = s->leave_story_cancel;
			config.on_confirm = safe_ui_library_screen_show; // Use wrapper function
			ui_confirmation_dialog_show(config);
		} else {
			safe_ui_library_screen_show();
		}
	};
	ui_header_config_t config = ui_header_config_default(g_story.title.c_str(), on_back_clicked);
	config.enable_marquee = true;
	
	lv_obj_t *header = ui_header_create(scr, &config);
	int header_h = 38;
	
	g_inventory_btn = nullptr;
	if (g_story.has_inventory) {
		g_inventory_btn = InventoryUI::createInventoryButton(header);
		
		lv_obj_remove_flag(g_inventory_btn, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
		lv_obj_set_pos(g_inventory_btn, 240 - 32 - 6, (38 - 30) / 2); // x = header_width - button_width - padding, y = centered
		
		lv_obj_t *title_label = (lv_obj_t *)lv_obj_get_user_data(header);
		if (title_label) {
			lv_obj_set_width(title_label, 150); // Increased from 130 to 160 for better layout
		}
	}
	
	if (g_story.has_inventory && n->gives_item.length() > 0) {
		give_item(n->gives_item);
	}
	
	// Remember the place so the story can be resumed; an ending starts over
	bool at_end = n->is_end || (n->choices.empty() && !n->inventory_choice);
	if (at_end) {
		story_progress::clear(g_story.id);
	} else {
		story_progress::save(g_story.id, key, InventoryManager::getCurrentInventory());
	}
	
	// One scrolling page: text, found items, then the choices at the end,
	// so the choices never cover the text
	lv_coord_t content_h = SCREEN_HEIGHT - header_h;
	lv_obj_t *content = lv_obj_create(scr);
	lv_obj_remove_style_all(content);
	lv_obj_set_size(content, 240, content_h);
	lv_obj_align(content, LV_ALIGN_TOP_MID, 0, header_h);
	apply_screen_bg(content);
	lv_obj_set_style_pad_hor(content, 10, 0);
	lv_obj_set_style_pad_top(content, 8, 0);
	lv_obj_set_style_pad_bottom(content, 10, 0);
	lv_obj_set_style_pad_row(content, 10, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
						  LV_FLEX_ALIGN_START);
	lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_scroll_dir(content, LV_DIR_VER);
	// Visible whenever the page continues below
	lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
	apply_scrollbar_style(content);

	lv_obj_t *text_wrap = lv_obj_create(content);
	lv_obj_remove_style_all(text_wrap);
	lv_obj_set_size(text_wrap, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_row(text_wrap, 6, 0);
	lv_obj_set_flex_flow(text_wrap, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(text_wrap, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
						  LV_FLEX_ALIGN_START);
	lv_obj_clear_flag(text_wrap, LV_OBJ_FLAG_SCROLLABLE);
	
	KiddoParser::ParsedContent parsed = KiddoParser::parseText(n->text);
	size_t images_left = 0;
	for (const auto &segment : parsed.segments)
		if (segment.type == KiddoParser::ContentSegment::IMAGE)
			images_left++;
	bool has_images = images_left > 0;
	
	// Pages without a picture get an animated scene to look at while someone reads
	if (!has_images) {
		g_last_scene = StoryScene::pick(g_story, *n, g_scene_leads, g_last_scene);
		lv_obj_t *scene = StoryScene::create(content, g_last_scene);
		lv_obj_move_to_index(scene, 0);
	}
	
	for (const auto& segment : parsed.segments) {
		if (segment.type == KiddoParser::ContentSegment::TEXT) {
			if (segment.content.length() > 0) {
				lv_obj_t *text_label = lv_label_create(text_wrap);
				lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
				lv_obj_set_width(text_label, LV_PCT(100));
				lv_label_set_text(text_label, segment.content.c_str());
				lv_obj_set_style_text_font(text_label, story_body_font(), 0);
				lv_obj_set_style_text_color(text_label, palette::ink(), 0);
				lv_obj_set_style_text_line_space(text_label, 2, 0);
			}
		} else if (segment.type == KiddoParser::ContentSegment::IMAGE) {
			lv_obj_t *img_wrapper = lv_obj_create(text_wrap);
			lv_obj_remove_style_all(img_wrapper);
			lv_obj_set_style_bg_opa(img_wrapper, LV_OPA_TRANSP, 0);
			lv_obj_set_width(img_wrapper, LV_PCT(100));
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

			// The last picture starts the prefetch for the next pages
			AsyncManager::ImageCallback on_done = nullptr;
			if (--images_left == 0)
				on_done = [key](bool, const String &) { prefetch_after_page(key); };
			AsyncManager::loadImage(segment.content, img, on_done);
		}
	}
	// Items found on this page, right where the story mentions them
	// The page's own sound first (a right answer, say), then the item's chime
	audio::Sound page_sound;
	if (audio::sound_from_name(n->sound, page_sound))
		audio::play(page_sound);
	else if (at_end)
		audio::play(audio::Sound::Fanfare);
	if (!g_found_items.empty())
		audio::play(audio::Sound::Chime);
	for (const auto &item : g_found_items)
		InventoryUI::createFoundItemCard(content, item);
	g_found_items.clear();

	// Pushes the choices to the bottom when the page is short; takes no
	// room when the page scrolls
	lv_obj_t *spacer = lv_obj_create(content);
	lv_obj_remove_style_all(spacer);
	lv_obj_set_width(spacer, LV_PCT(100));
	lv_obj_set_flex_grow(spacer, 1);

	lv_obj_t *choices = lv_obj_create(content);
	lv_obj_remove_style_all(choices);
	lv_obj_set_size(choices, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_top(choices, 10, 0);
	lv_obj_set_style_pad_row(choices, 6, 0);
	lv_obj_set_style_border_width(choices, 1, 0);
	lv_obj_set_style_border_side(choices, LV_BORDER_SIDE_TOP, 0);
	lv_obj_set_style_border_color(choices, palette::parchment_dark(), 0);
	lv_obj_clear_flag(choices, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(choices, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(choices, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
						  LV_FLEX_ALIGN_START);
	// Inventory-choice nodes continue through the item picker, not "Next"
	if (at_end)
	{
		lv_obj_t *b = create_choice_button(choices, S()->end_next);
		lv_obj_add_event_cb(
			b, [](lv_event_t *e)
			{ show_end_fullscreen(); }, LV_EVENT_CLICKED,
			nullptr);
	}
	else
	{
		for (const auto &ch : n->choices)
		{
			// Check inventory requirements
			if (g_story.has_inventory) {
				bool has_required_item = InventoryManager::getCurrentInventory().hasItem(ch.required_item);
				
				// Skip hidden choices if requirement not met
				if (ch.hidden_without_item && !has_required_item) {
					continue;
				}
			}
			
			lv_obj_t *b = create_choice_button(choices, ch.text.c_str());
			
			// Check if choice should be disabled due to missing inventory item
			bool is_disabled = false;
			if (g_story.has_inventory && ch.required_item.length() > 0) {
				is_disabled = !InventoryManager::getCurrentInventory().hasItem(ch.required_item);
			}
			
			if (is_disabled) {
				// Grey it out; same size as the other choices
				lv_obj_set_style_bg_color(b, palette::parchment_dark(), 0);
				lv_obj_set_style_text_color(b, palette::ink_soft(), 0);
				lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);
			} else {
				ChoiceData *choice_data = new ChoiceData{ch.next, ch.gives_item, ch.required_item};
				lv_obj_add_event_cb(b, on_choice_clicked, LV_EVENT_CLICKED, choice_data);
				lv_obj_add_event_cb(b, free_user_data_cb<ChoiceData>, LV_EVENT_DELETE, choice_data);
			}
		}
	}
	
	if (n->inventory_choice && g_story.has_inventory) {
		Serial.println("Displaying inventory choice dialog");
		
		lv_obj_t *inv_btn = create_choice_button(choices, S()->choose_item);
		
		InventoryChoiceData *inv_data = new InventoryChoiceData{n->correct_item, n->success_next, n->failure_next};
		lv_obj_add_event_cb(inv_btn, on_inventory_choice_clicked, LV_EVENT_CLICKED, inv_data);
		lv_obj_add_event_cb(inv_btn, free_user_data_cb<InventoryChoiceData>, LV_EVENT_DELETE, inv_data);
	}
	
	if (!has_images)
		prefetch_after_page(key);
}

/**
 * @brief Load a story and show a node, optionally restoring a saved place
 * @param st Story summary from story::all()
 * @param nodeKey Node to show
 * @param resume Saved inventory to restore, or nullptr for a fresh start
 */
static void start_story(const Story_t &st, const String &nodeKey, const story_progress::Saved *resume)
{
	// Copy first: st and nodeKey may refer into the story list
	String start = nodeKey;
	Story_t loaded;
	if (!story::load(st, loaded)) {
		Serial.println("[STORY] Failed to load story: " + st.file);
		ui_library_screen_show();
		return;
	}
	g_story = std::move(loaded);
	g_prefetched_images.clear();
	g_found_items.clear();
	g_last_scene = StoryScene::Spec();
	g_scene_leads = StoryScene::leads(g_story);
	
	// A saved node can vanish when a story is updated; start over then
	if (resume && !g_story.get(start)) {
		Serial.println("[STORY] Saved node missing, starting over: " + start);
		story_progress::clear(g_story.id);
		start = g_story.start;
		resume = nullptr;
	}
	
	g_story_progress_made = resume != nullptr;
	
	if (g_story.has_inventory) {
		std::vector<InventoryItem_t> items;
		if (resume) {
			for (const String &id : resume->items) {
				const ItemDefinition_t *def = g_story.getItemDefinition(id);
				items.push_back(def ? InventoryItem_t(def->id, def->name, def->icon_url, def->description) : InventoryItem_t(id, id));
			}
		} else {
			items = g_story.initial_inventory;
		}
		InventoryManager::getCurrentInventory().initialize(items);
	}
	
	show_node(start);
}

void ui_story_screen_show(const Story_t &st, const String &nodeKey)
{
	start_story(st, nodeKey, nullptr);
}

void ui_story_screen_open(const Story_t &st)
{
	story_progress::Saved saved;
	if (!story_progress::load(st.id, saved) || saved.node == st.start) {
		start_story(st, st.start, nullptr);
		return;
	}
	
	// Copies: the dialog outlives any reference into the story list
	Story_t summary = st;
	const auto *s = S();
	ConfirmationConfig config;
	config.title = s->resume_title;
	config.message = s->resume_message;
	config.confirm_text = s->resume_continue;
	config.cancel_text = s->resume_start_over;
	config.confirm_color = palette::primary();
	// Opened by mistake: back to the library, progress untouched
	config.show_close = true;
	config.on_confirm = [summary, saved]() {
		start_story(summary, saved.node, &saved);
	};
	config.on_cancel = [summary]() {
		story_progress::clear(summary.id);
		start_story(summary, summary.start, nullptr);
	};
	ui_confirmation_dialog_show(config);
}
