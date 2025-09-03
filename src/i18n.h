/**
 * @file i18n.h
 * @brief Internationalization system
 *
 * This file contains the LocalizedStrings structure and internationalization
 * utilities for supporting multiple languages in the application.
 */

#pragma once

/**
 * @brief Structure containing all localized strings for the application
 */
struct LocalizedStrings
{
  // Settings screen strings
  const char *settings_title;           /**< Settings screen title */
  const char *brightness;               /**< Brightness label */
  const char *language_label;           /**< Language label */
  const char *close;                    /**< Close button text */

  // WiFi reset strings
  const char *reset_wifi;               /**< Reset WiFi button text */
  const char *reset;                    /**< Reset button text */
  const char *reset_confirmation;       /**< WiFi reset confirmation message */
  const char *cancel;                   /**< Cancel button text */

  // Library screen strings
  const char *stories_btn;              /**< Stories button text */
  const char *stories_title;            /**< Stories screen title */
  const char *back_to_library;          /**< Back to library button text */
  const char *the_end;                  /**< Story end text */
  const char *no_stories;               /**< No stories found message */
  const char *end_next;                 /**< Next button text at story end */

  // Story display strings
  const char *story_font_label;         /**< Story font size label */

  // General UI strings
  const char *loading;                  /**< Loading message */

  // Cache management strings
  const char *clear_cache;              /**< Clear cache button text */
  const char *delete_label;             /**< Delete button text */
  const char *clear_cache_confirmation; /**< Clear cache confirmation message */
  const char *online_mode;              /**< Online mode toggle text */
  const char *wifi_reset_toast;         /**< WiFi reset success message */

  // Font size options
  const char *font_opt_small;           /**< Small font option */
  const char *font_opt_normal;          /**< Normal font option */
  const char *font_opt_large;           /**< Large font option */

  // Language options
  const char *lang_opt_en;              /**< English language option */
  const char *lang_opt_pt;              /**< Portuguese language option */

  // Error messages
  const char *delete_failed;            /**< Delete failed message */
  const char *retry_online_fetch;       /**< Retry online fetch message */
  const char *no_online_catalog;        /**< No online catalog message */
  const char *stories_will_appear;      /**< Stories will appear message */
  const char *enable_online_mode;       /**< Enable online mode message */
  const char *cache_cleared_message;    /**< Cache cleared success message */
  const char *reset_wifi_confirmation;  /**< WiFi reset confirmation message */

  // WiFi SSID screen strings
  const char *wifi_networks_title;      /**< WiFi networks screen title */
  const char *scan_for_networks;        /**< Scan for networks button text */
  const char *scanning;                 /**< Scanning message */
  const char *scan_again;               /**< Scan again button text */
  const char *scan_instruction;         /**< Scan instruction message */
  const char *no_networks_found;        /**< No networks found message */
  const char *networks_found;           /**< Networks found message */
  const char *signal_strong;            /**< Strong signal text */
  const char *signal_good;              /**< Good signal text */
  const char *signal_weak;              /**< Weak signal text */
  const char *network_open;             /**< Open network text */
  const char *network_secured;          /**< Secured network text */
  const char *wifi_password_title;      /**< WiFi password screen title */
  const char *wifi_password_placeholder;/**< Password placeholder text */
  const char *connect_btn;              /**< Connect button text */
  const char *connecting_to;            /**< Connecting to message */
  const char *connected_successfully;   /**< Connected successfully message */
  const char *connection_failed;        /**< Connection failed message */
  const char *connection_timeout_countdown;/**< Connection timeout countdown */

  // Catalog URL strings
  const char *catalog_url_label;        /**< Catalog URL label */
  const char *edit_btn;                 /**< Edit button text */
  const char *catalog_url_title;        /**< Catalog URL screen title */
  const char *catalog_url_description;  /**< Catalog URL description */
  const char *save;                     /**< Save button text */
  const char *restore_default;          /**< Restore default button text */

  // Story screen strings
  const char *leave_story_title;        /**< Leave story dialog title */
  const char *leave_story_message;      /**< Leave story dialog message */
  const char *leave_story_confirm;      /**< Leave story confirm button */
  const char *leave_story_cancel;       /**< Leave story cancel button */

  // Inventory strings
  const char *inventory_title;          /**< Inventory dialog title */
  const char *new_item_title;           /**< New item notification title */
  const char *new_item_message;         /**< New item notification message */
  const char *item_added_message;       /**< Item added message */
  const char *choose_item;              /**< Choose item button text */

  // Image loading strings
  const char *loading_image;            /**< Loading image message */
  const char *decode_failed;            /**< Image decode failed message */
  const char *load_failed;              /**< Image load failed message */
};

/**
 * @brief Supported languages enumeration
 */
enum Language
{
  LANG_EN = 0,  /**< English language */
  LANG_PT = 1   /**< Portuguese language */
};

extern Language current_language;

/**
 * @brief Get localized strings for current language
 * @return Pointer to localized strings structure
 */
const LocalizedStrings *S();
