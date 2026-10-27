/**
 * @file image_display.h
 * @brief Image display and management utilities
 *
 * This file contains the ImageDisplay namespace with functions for
 * displaying JPEG images, creating loading placeholders, and managing
 * image resources in LVGL.
 */

#pragma once

#include <Arduino.h>
#include <lvgl.h>

namespace ImageDisplay {

/** @brief Outcome of displayJpegFromFile() */
enum class Result {
    OK,             /**< Image shown */
    NO_MEMORY,      /**< Not enough RAM right now; img_obj is left untouched */
    DECODE_FAILED   /**< The file is not a JPEG the decoder can read */
};

/**
 * @brief Display a JPEG image from file
 * @param filepath Path to the JPEG file
 * @param img_obj LVGL image object to display the image
 * @return Whether the image was shown, and why not
 */
Result displayJpegFromFile(const String& filepath, lv_obj_t* img_obj);

/**
 * @brief Show a spinner centered in the image's area while it loads
 * @param img_obj LVGL image object the spinner goes inside
 */
void createLoadingPlaceholder(lv_obj_t* img_obj);

/**
 * @brief Remove the loading placeholder created for an image object
 * @param img_obj LVGL image object the placeholder belongs to
 */
void removeLoadingPlaceholder(lv_obj_t* img_obj);

/**
 * @brief Show an error message in place of an image
 * @param img_obj LVGL image object
 * @param message Text to display
 */
void showError(lv_obj_t* img_obj, const char* message);

/**
 * @brief Clean up image resources associated with an image object
 * @param img_obj LVGL image object to clean up
 */
void cleanupImageResources(lv_obj_t* img_obj);

}
