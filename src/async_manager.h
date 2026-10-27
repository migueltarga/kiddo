/**
 * @file async_manager.h
 * @brief Asynchronous operation management
 *
 * This file contains the AsyncManager namespace for handling asynchronous
 * operations like image loading, story downloading, and catalog fetching
 * with callback-based completion handling.
 */

#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <functional>

namespace AsyncManager {

/**
 * @brief Callback function type for image loading operations
 * @param success True if image was loaded successfully
 * @param cachedPath Path to the cached image file, or error message if failed
 */
typedef std::function<void(bool success, const String& cachedPath)> ImageCallback;

/**
 * @brief Callback function type for story download operations
 * @param success True if story was downloaded successfully
 * @param storyId ID of the downloaded story, or error message if failed
 */
typedef std::function<void(bool success, const String& storyId)> StoryCallback;

/**
 * @brief Callback function type for catalog fetch operations
 * @param success True if catalog was fetched successfully
 */
typedef std::function<void(bool success)> CatalogCallback;

/**
 * @brief Initialize the async manager system
 * Sets up queues and tasks for background operations
 */
void init();

/**
 * @brief Load an image from URL asynchronously
 * @param url URL of the image to download
 * @param imgWidget LVGL image widget to display the loaded image
 * @param callback Function called when operation completes
 */
void loadImage(const String& url, lv_obj_t* imgWidget, ImageCallback callback);

/**
 * @brief Download a story file asynchronously
 * @param filename Name of the story file to download
 * @param callback Function called when operation completes
 */
void downloadStory(const String& filename, StoryCallback callback);

/**
 * @brief Fetch the remote story catalog
 * @param callback Function called when operation completes
 */
void fetchCatalog(CatalogCallback callback);

/**
 * @brief Process pending async operations
 * Should be called regularly from the main loop
 */
void process();

}