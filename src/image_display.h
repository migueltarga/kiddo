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

/**
 * @brief Display a JPEG image from file
 * @param filepath Path to the JPEG file
 * @param img_obj LVGL image object to display the image
 * @return True if display was successful
 */
bool displayJpegFromFile(const String& filepath, lv_obj_t* img_obj);

/**
 * @brief Create a loading placeholder for image display
 * @param img_obj LVGL image object to add placeholder to
 */
void createLoadingPlaceholder(lv_obj_t* img_obj);

/**
 * @brief Clean up image resources associated with an image object
 * @param img_obj LVGL image object to clean up
 */
void cleanupImageResources(lv_obj_t* img_obj);

}
