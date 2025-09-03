/**
 * @file image_loader.h
 * @brief Asynchronous image loading system
 *
 * This file contains the ImageLoader namespace for asynchronously loading
 * images from URLs with queue-based processing and LVGL integration.
 */

#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

namespace ImageLoader {

/**
 * @brief Structure representing an image load request
 */
struct LoadRequest {
    char url[256];              /**< URL of the image to load */
    lv_obj_t* img_obj;          /**< LVGL image object to display the image */
    bool show_loading_placeholder; /**< Whether to show loading placeholder */

    /**
     * @brief Default constructor
     */
    LoadRequest() : img_obj(nullptr), show_loading_placeholder(true) {
        url[0] = '\0';
    }

    /**
     * @brief Constructor with parameters
     * @param u Image URL
     * @param obj LVGL image object
     * @param placeholder Whether to show loading placeholder
     */
    LoadRequest(const String& u, lv_obj_t* obj, bool placeholder = true)
        : img_obj(obj), show_loading_placeholder(placeholder) {
        strncpy(url, u.c_str(), sizeof(url) - 1);
        url[sizeof(url) - 1] = '\0';
    }
};

/**
 * @brief Initialize the image loader system
 */
void init();

/**
 * @brief Load image asynchronously
 * @param url URL of the image to load
 * @param img_obj LVGL image object to display the image
 * @param show_loading_placeholder Whether to show loading placeholder
 */
void loadImageAsync(const String& url, lv_obj_t* img_obj, bool show_loading_placeholder = true);

/**
 * @brief Process pending image load requests
 * Should be called regularly from the main loop
 */
void process();

/**
 * @brief Get number of pending image load requests
 * @return Number of pending requests
 */
int getPendingCount();

/**
 * @brief Clean up image resources for an image object
 * @param img_obj LVGL image object to clean up
 */
void cleanupImageResources(lv_obj_t* img_obj);

}
