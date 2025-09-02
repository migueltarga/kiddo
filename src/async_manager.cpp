#include "async_manager.h"
#include "file_system.h"
#include "image_display.h"
#include "remote_catalog.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

namespace AsyncManager {

enum OperationType {
    OP_LOAD_IMAGE,
    OP_DOWNLOAD_STORY,
    OP_FETCH_CATALOG
};

struct OperationRequest {
    OperationType type;
    char url[256];
    lv_obj_t* imgWidget;
    ImageCallback imageCallback;
    StoryCallback storyCallback;
    CatalogCallback catalogCallback;
};

struct OperationResult {
    OperationType type;
    bool success;
    char resultPath[128];
    lv_obj_t* imgWidget;
    ImageCallback imageCallback;
    StoryCallback storyCallback;
    CatalogCallback catalogCallback;
};

static QueueHandle_t requestQueue = nullptr;
static QueueHandle_t resultQueue = nullptr;
static TaskHandle_t workerTask = nullptr;
static bool initialized = false;

static void workerTaskFunction(void* parameter) {
    OperationRequest request;
    
    while (true) {
        if (xQueueReceive(requestQueue, &request, portMAX_DELAY) == pdTRUE) {
            OperationResult result;
            result.type = request.type;
            result.imgWidget = request.imgWidget;
            result.imageCallback = request.imageCallback;
            result.storyCallback = request.storyCallback;
            result.catalogCallback = request.catalogCallback;
            result.success = false;
            
            String url_str = String(request.url);
            
            if (request.type == OP_LOAD_IMAGE) {
                String cachedPath = FileSystem::getCachedImagePath(url_str);
                
                if (FileSystem::isImageCached(url_str)) {
                    result.success = true;
                    strncpy(result.resultPath, cachedPath.c_str(), sizeof(result.resultPath) - 1);
                } else {
                    if (FileSystem::downloadFile(url_str, cachedPath)) {
                        result.success = true;
                        strncpy(result.resultPath, cachedPath.c_str(), sizeof(result.resultPath) - 1);
                    } else {
                        Serial.println("[ASYNC_MANAGER] Image download failed: " + url_str);
                        result.success = false;
                    }
                }
            } else if (request.type == OP_DOWNLOAD_STORY) {
                String storyId;
                if (remote_catalog::ensureDownloadedOrIndexed(url_str, &storyId)) {
                    result.success = true;
                    strncpy(result.resultPath, storyId.c_str(), sizeof(result.resultPath) - 1);
                } else {
                    result.success = false;
                }
            } else if (request.type == OP_FETCH_CATALOG) {
                if (remote_catalog::fetch()) {
                    result.success = true;
                } else {
                    Serial.println("[ASYNC_MANAGER] Catalog fetch failed");
                    result.success = false;
                }
            }
            
            result.resultPath[sizeof(result.resultPath) - 1] = '\0';
            
            if (xQueueSend(resultQueue, &result, portMAX_DELAY) != pdTRUE) {
                Serial.println("[ASYNC_MANAGER] Failed to send result");
            }
        }
    }
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
    
    OperationRequest request;
    request.type = OP_LOAD_IMAGE;
    strncpy(request.url, url.c_str(), sizeof(request.url) - 1);
    request.url[sizeof(request.url) - 1] = '\0';
    request.imgWidget = imgWidget;
    request.imageCallback = callback;
    request.storyCallback = nullptr;
    request.catalogCallback = nullptr;
    
    if (xQueueSend(requestQueue, &request, 0) != pdTRUE) {
        Serial.println("[ASYNC_MANAGER] Request queue full");
        if (callback) callback(false, "");
    }
}

void downloadStory(const String& filename, StoryCallback callback) {
    if (!initialized) {
        Serial.println("[ASYNC_MANAGER] Not initialized");
        if (callback) callback(false, "");
        return;
    }
    
    OperationRequest request;
    request.type = OP_DOWNLOAD_STORY;
    strncpy(request.url, filename.c_str(), sizeof(request.url) - 1);
    request.url[sizeof(request.url) - 1] = '\0';
    request.imgWidget = nullptr;
    request.imageCallback = nullptr;
    request.storyCallback = callback;
    request.catalogCallback = nullptr;
    
    if (xQueueSend(requestQueue, &request, 0) != pdTRUE) {
        Serial.println("[ASYNC_MANAGER] Request queue full");
        if (callback) callback(false, "");
    }
}

void fetchCatalog(CatalogCallback callback) {
    if (!initialized) {
        Serial.println("[ASYNC_MANAGER] Not initialized");
        if (callback) callback(false);
        return;
    }
    
    OperationRequest request;
    request.type = OP_FETCH_CATALOG;
    request.url[0] = '\0';
    request.imgWidget = nullptr;
    request.imageCallback = nullptr;
    request.storyCallback = nullptr;
    request.catalogCallback = callback;
    
    if (xQueueSend(requestQueue, &request, 0) != pdTRUE) {
        Serial.println("[ASYNC_MANAGER] Request queue full");
        if (callback) callback(false);
    }
}

void process() {
    if (!initialized) return;
    
    OperationResult result;
    while (xQueueReceive(resultQueue, &result, 0) == pdTRUE) {
        if (result.type == OP_LOAD_IMAGE && result.imageCallback) {
            if (result.success) {
                if (result.imgWidget && lv_obj_is_valid(result.imgWidget)) {
                    lv_obj_t* parent = lv_obj_get_parent(result.imgWidget);
                    if (parent) {
                        uint32_t child_count = lv_obj_get_child_cnt(parent);
                        for (uint32_t i = 0; i < child_count; i++) {
                            lv_obj_t* child = lv_obj_get_child(parent, i);
                            if (lv_obj_get_user_data(child) && 
                                strcmp((char*)lv_obj_get_user_data(child), "loading_placeholder") == 0) {
                                lv_obj_del(child);
                                break;
                            }
                        }
                    }
                    
                    if (!ImageDisplay::displayJpegFromFile(String(result.resultPath), result.imgWidget)) {
                        Serial.println("[ASYNC_MANAGER] Failed to display JPEG");
                    }
                }
            }
            result.imageCallback(result.success, String(result.resultPath));
        } else if (result.type == OP_DOWNLOAD_STORY && result.storyCallback) {
            result.storyCallback(result.success, String(result.resultPath));
        } else if (result.type == OP_FETCH_CATALOG && result.catalogCallback) {
            result.catalogCallback(result.success);
        }
    }
}

}