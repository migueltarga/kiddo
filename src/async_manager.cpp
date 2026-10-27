/**
 * @file async_manager.cpp
 * @brief Asynchronous operations manager for background tasks
 *
 * This module handles asynchronous operations like image downloading,
 * story downloading, and catalog fetching using FreeRTOS tasks and queues.
 * It provides a clean interface for UI components to request background operations
 * without blocking the main UI thread.
 *
 * The worker task only does network and file I/O. Callbacks, LVGL objects and
 * shared caches are only touched from process(), on the main thread.
 */

#include "async_manager.h"
#include "file_system.h"
#include "image_display.h"
#include "remote_catalog.h"
#include "story_utils.h"
#include "i18n.h"
#include <map>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

namespace AsyncManager {

/** @brief Operation types for async requests */
enum OperationType {
    OP_LOAD_IMAGE,      /**< Load image from URL */
    OP_DOWNLOAD_STORY,  /**< Download story file */
    OP_FETCH_CATALOG    /**< Fetch remote catalog */
};

/**
 * @brief Structure for operation requests
 *
 * Queues copy items with memcpy, so this must stay trivially copyable.
 */
struct OperationRequest {
    uint32_t id;            /**< Key into the pending table */
    OperationType type;     /**< Type of operation */
    char url[256];          /**< URL for the operation */
    char path[48];          /**< Local path (story downloads) */
    char lang[8];           /**< Fallback story language (story downloads) */
};

/**
 * @brief Structure for operation results
 *
 * Trivially copyable for the same reason as OperationRequest.
 */
struct OperationResult {
    uint32_t id;                                    /**< Key into the pending table */
    bool success;                                   /**< Success flag */
    char resultPath[128];                           /**< Cached image path or story ID */
    remote_catalog::Catalog* entries;               /**< Fetched catalog, owned by the receiver */
};

/** @brief Main-thread state for a request that has not completed yet */
struct Pending {
    OperationType type;
    lv_obj_t* imgWidget = nullptr;  /**< Cleared if the widget is deleted first */
    String file;                    /**< Story filename for downloads */
    ImageCallback imageCallback;
    StoryCallback storyCallback;
    CatalogCallback catalogCallback;
};

static QueueHandle_t requestQueue = nullptr;
static QueueHandle_t resultQueue = nullptr;
static TaskHandle_t workerTask = nullptr;
static bool initialized = false;
static std::map<uint32_t, Pending> pending;
static uint32_t nextId = 1;

static void copyString(char* dst, size_t size, const String& src) {
    strncpy(dst, src.c_str(), size - 1);
    dst[size - 1] = '\0';
}

static void workerTaskFunction(void* parameter) {
    OperationRequest request;

    while (true) {
        if (xQueueReceive(requestQueue, &request, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        OperationResult result = {};
        result.id = request.id;
        String url = String(request.url);
        uint32_t started = millis();

        if (request.type == OP_LOAD_IMAGE) {
            String cachedPath = FileSystem::getCachedImagePath(url);
            result.success = FileSystem::isImageCached(url) || FileSystem::downloadFile(url, cachedPath);
            if (result.success) {
                copyString(result.resultPath, sizeof(result.resultPath), cachedPath);
            } else {
                Serial.println("[ASYNC_MANAGER] Image download failed: " + url);
            }
        } else if (request.type == OP_DOWNLOAD_STORY) {
            String storyId;
            result.success = remote_catalog::downloadStory(url, String(request.path), String(request.lang), storyId);
            if (result.success) {
                copyString(result.resultPath, sizeof(result.resultPath), storyId);
            }
        } else if (request.type == OP_FETCH_CATALOG) {
            result.entries = new remote_catalog::Catalog();
            result.success = remote_catalog::fetchEntries(url, *result.entries);
            if (!result.success) {
                Serial.println("[ASYNC_MANAGER] Catalog fetch failed");
            }
        }

        static const char* const kNames[] = {"image", "story", "catalog"};
        Serial.printf("[ASYNC_MANAGER] %s took %lu ms\n",
                      request.type <= OP_FETCH_CATALOG ? kNames[request.type] : "?", millis() - started);

        if (xQueueSend(resultQueue, &result, portMAX_DELAY) != pdTRUE) {
            Serial.println("[ASYNC_MANAGER] Failed to send result");
            delete result.entries;
        }
    }
}

/**
 * @brief Forget the target widget of a pending image load when it is deleted
 */
static void img_widget_deleted_cb(lv_event_t* e) {
    uint32_t id = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    auto it = pending.find(id);
    if (it != pending.end()) {
        it->second.imgWidget = nullptr;
    }
}

/** @brief A downloaded image waiting for enough RAM to be decoded */
struct DecodeRetry {
    lv_obj_t* widget;
    String path;
    ImageCallback callback;
    uint8_t tries;
};

static const uint32_t kDecodeRetryMs = 300;
static const uint8_t kDecodeMaxTries = 30;  // ~9 s, longer than one download

/** @brief Outcome of trying to show a downloaded image */
enum ShowResult { SHOW_DONE, SHOW_FAILED, SHOW_WAITING };

/**
 * @brief Decode a cached image into its widget
 * @param last_try Give up instead of waiting if memory is short
 */
static ShowResult show_image(lv_obj_t* widget, const String& path, bool last_try) {
    ImageDisplay::Result res = ImageDisplay::displayJpegFromFile(path, widget);
    if (res == ImageDisplay::Result::OK) return SHOW_DONE;
    if (res == ImageDisplay::Result::NO_MEMORY && !last_try) return SHOW_WAITING;

    ImageDisplay::removeLoadingPlaceholder(widget);
    if (res == ImageDisplay::Result::DECODE_FAILED) {
        // Drop a corrupt cache entry so the next visit downloads it again
        FileSystem::deleteFile(path);
        ImageDisplay::showError(widget, S()->decode_failed);
    } else {
        ImageDisplay::showError(widget, S()->load_failed);
    }
    return SHOW_FAILED;
}

/** @brief Cancel a waiting decode when its widget is deleted */
static void decode_widget_deleted_cb(lv_event_t* e) {
    lv_timer_t* timer = (lv_timer_t*)lv_event_get_user_data(e);
    delete (DecodeRetry*)lv_timer_get_user_data(timer);
    lv_timer_delete(timer);
}

static void decode_retry_timer_cb(lv_timer_t* timer) {
    DecodeRetry* retry = (DecodeRetry*)lv_timer_get_user_data(timer);
    retry->tries++;
    ShowResult shown = show_image(retry->widget, retry->path, retry->tries >= kDecodeMaxTries);
    if (shown == SHOW_WAITING) return;

    lv_obj_remove_event_cb_with_user_data(retry->widget, decode_widget_deleted_cb, timer);
    lv_timer_delete(timer);
    ImageCallback callback = std::move(retry->callback);
    String path = retry->path;
    delete retry;
    if (callback) callback(shown == SHOW_DONE, path);
}

/**
 * @brief Retry a decode that ran out of memory until RAM frees up
 *
 * The spinner stays up meanwhile; the callback runs once it is settled.
 */
static void wait_for_memory(lv_obj_t* widget, const String& path, ImageCallback callback) {
    DecodeRetry* retry = new DecodeRetry{widget, path, std::move(callback), 0};
    lv_timer_t* timer = lv_timer_create(decode_retry_timer_cb, kDecodeRetryMs, retry);
    lv_obj_add_event_cb(widget, decode_widget_deleted_cb, LV_EVENT_DELETE, timer);
}

/**
 * @brief Register a pending operation and queue it for the worker
 * @return True if the request was queued
 */
static bool submit(OperationRequest& request, Pending&& state) {
    request.id = nextId++;
    if (xQueueSend(requestQueue, &request, 0) != pdTRUE) {
        Serial.println("[ASYNC_MANAGER] Request queue full");
        return false;
    }
    // The result is only read by process() on this same thread, so
    // registering after the send cannot race with completion.
    pending.emplace(request.id, std::move(state));
    return true;
}

void init() {
    if (initialized) return;

    requestQueue = xQueueCreate(10, sizeof(OperationRequest));
    resultQueue = xQueueCreate(10, sizeof(OperationResult));

    if (requestQueue == nullptr || resultQueue == nullptr) {
        Serial.println("[ASYNC_MANAGER] Failed to create queues");
        return;
    }

    xTaskCreate(
        workerTaskFunction,
        "AsyncManager",
        16384,
        nullptr,
        1,
        &workerTask
    );

    if (workerTask == nullptr) {
        Serial.println("[ASYNC_MANAGER] Failed to create task");
        return;
    }

    initialized = true;
}

void loadImage(const String& url, lv_obj_t* imgWidget, ImageCallback callback) {
    if (!initialized) {
        Serial.println("[ASYNC_MANAGER] Not initialized");
        if (callback) callback(false, "");
        return;
    }

    OperationRequest request = {};
    request.type = OP_LOAD_IMAGE;
    copyString(request.url, sizeof(request.url), url);

    Pending state;
    state.type = OP_LOAD_IMAGE;
    state.imgWidget = imgWidget;
    state.imageCallback = callback;

    if (!submit(request, std::move(state))) {
        if (callback) callback(false, "");
        return;
    }
    if (imgWidget) {
        lv_obj_add_event_cb(imgWidget, img_widget_deleted_cb, LV_EVENT_DELETE, (void*)(uintptr_t)request.id);
    }
}

void downloadStory(const String& filename, StoryCallback callback) {
    if (!initialized || !remote_catalog::isValidFileName(filename)) {
        Serial.println("[ASYNC_MANAGER] Cannot download story: " + filename);
        if (callback) callback(false, "");
        return;
    }

    OperationRequest request = {};
    request.type = OP_DOWNLOAD_STORY;
    copyString(request.url, sizeof(request.url), remote_catalog::storyUrl(filename));
    copyString(request.path, sizeof(request.path), "/" + filename);
    copyString(request.lang, sizeof(request.lang), story_utils::currentLanguageToString());

    Pending state;
    state.type = OP_DOWNLOAD_STORY;
    state.file = filename;
    state.storyCallback = callback;

    if (!submit(request, std::move(state))) {
        if (callback) callback(false, "");
    }
}

void fetchCatalog(CatalogCallback callback) {
    if (!initialized) {
        Serial.println("[ASYNC_MANAGER] Not initialized");
        if (callback) callback(false);
        return;
    }

    Pending state;
    state.type = OP_FETCH_CATALOG;
    state.catalogCallback = callback;

    if (remote_catalog::isFresh()) {
        // Complete through the result queue so callers always get the
        // callback from process(), never re-entrantly
        uint32_t id = nextId++;
        OperationResult result = {};
        result.id = id;
        result.success = true;
        if (xQueueSend(resultQueue, &result, 0) == pdTRUE) {
            pending.emplace(id, std::move(state));
            return;
        }
    }

    OperationRequest request = {};
    request.type = OP_FETCH_CATALOG;
    copyString(request.url, sizeof(request.url), remote_catalog::getCatalogUrl());

    if (!submit(request, std::move(state))) {
        if (callback) callback(false);
    }
}

void process() {
    if (!initialized) return;

    OperationResult result;
    while (xQueueReceive(resultQueue, &result, 0) == pdTRUE) {
        auto it = pending.find(result.id);
        if (it == pending.end()) {
            delete result.entries;
            continue;
        }
        // Remove before running callbacks, which may submit new requests
        Pending state = std::move(it->second);
        pending.erase(it);

        String resultPath = String(result.resultPath);

        if (state.type == OP_LOAD_IMAGE) {
            if (state.imgWidget) {
                lv_obj_remove_event_cb_with_user_data(state.imgWidget, img_widget_deleted_cb, (void*)(uintptr_t)result.id);
                if (!result.success) {
                    ImageDisplay::removeLoadingPlaceholder(state.imgWidget);
                    ImageDisplay::showError(state.imgWidget, S()->load_failed);
                } else {
                    ShowResult shown = show_image(state.imgWidget, resultPath, false);
                    if (shown == SHOW_WAITING) {
                        wait_for_memory(state.imgWidget, resultPath, std::move(state.imageCallback));
                        continue;
                    }
                    result.success = shown == SHOW_DONE;
                }
            }
            if (state.imageCallback) {
                state.imageCallback(result.success, resultPath);
            }
        } else if (state.type == OP_DOWNLOAD_STORY) {
            if (result.success && !remote_catalog::indexDownloaded(state.file)) {
                result.success = false;
            }
            if (state.storyCallback) {
                state.storyCallback(result.success, resultPath);
            }
        } else if (state.type == OP_FETCH_CATALOG) {
            if (result.entries) {
                remote_catalog::applyFetched(result.success, *result.entries);
                delete result.entries;
            }
            if (state.catalogCallback) {
                state.catalogCallback(result.success);
            }
        }
    }
}

}
