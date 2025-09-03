/**
 * @file file_system.h
 * @brief Filesystem operations and utilities
 *
 * This file contains the FileSystem namespace with functions for file
 * operations, directory management, and filesystem initialization
 * using LVGL filesystem abstraction.
 */

#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include <ArduinoJson.h>

namespace FileSystem {

/**
 * @brief Initialize the filesystem
 * @return True if initialization was successful
 */
bool init();

// File operations using LVGL fs
/**
 * @brief Check if a file exists
 * @param path Path to the file
 * @return True if file exists
 */
bool exists(const String& path);

/**
 * @brief Read entire file content as string
 * @param path Path to the file
 * @return File content as string
 */
String readFile(const String& path);

/**
 * @brief Write string content to file
 * @param path Path to the file
 * @param content Content to write
 * @return True if write was successful
 */
bool writeFile(const String& path, const String& content);

/**
 * @brief Delete a file
 * @param path Path to the file
 * @return True if deletion was successful
 */
bool deleteFile(const String& path);

// Story-specific operations
/**
 * @brief Save story content to file
 * @param filename Story filename
 * @param content Story content as JSON string
 * @return True if save was successful
 */
bool saveStory(const String& filename, const String& content);

/**
 * @brief Load story content from file
 * @param filename Story filename
 * @return Story content as string
 */
String loadStory(const String& filename);

/**
 * @brief Delete a story file
 * @param filename Story filename
 * @return True if deletion was successful
 */
bool deleteStory(const String& filename);

// Image cache operations
/**
 * @brief Get cached image path for a URL
 * @param url Image URL
 * @return Path to cached image file
 */
String getCachedImagePath(const String& url);

/**
 * @brief Check if image is cached
 * @param url Image URL
 * @return True if image is cached
 */
bool isImageCached(const String& url);

/**
 * @brief Cache image data
 * @param url Image URL
 * @param data Image data buffer
 * @param size Size of image data
 * @return True if caching was successful
 */
bool cacheImage(const String& url, const uint8_t* data, size_t size);

// Index management for stories
/**
 * @brief Load story index from file
 * @param doc JsonDocument to fill with index data
 * @return True if loading was successful
 */
bool loadIndex(JsonDocument& doc);

/**
 * @brief Save story index to file
 * @param doc JsonDocument containing index data
 * @return True if saving was successful
 */
bool saveIndex(const JsonDocument& doc);

/**
 * @brief Check if index contains a story
 * @param file Story filename
 * @return True if story is in index
 */
bool indexContains(const String& file);

/**
 * @brief Add story to index
 * @param file Story filename
 * @param name Story display name
 * @param lang Story language
 * @return True if addition was successful
 */
bool addToIndex(const String& file, const String& name, const String& lang);

/**
 * @brief Remove story from index
 * @param file Story filename
 * @return True if removal was successful
 */
bool removeFromIndex(const String& file);

// HTTP operations
/**
 * @brief Perform HTTP GET request
 * @param url URL to request
 * @param response String to fill with response
 * @return True if request was successful
 */
bool httpGet(const String& url, String& response);

/**
 * @brief Download file from URL
 * @param url URL to download from
 * @param localPath Local path to save file
 * @return True if download was successful
 */
bool downloadFile(const String& url, const String& localPath);

// Cache management
/**
 * @brief Clear image cache
 */
void clearCache();

/**
 * @brief Clear all downloaded stories
 */
void clearStories();

/**
 * @brief Clear all cached data (images and stories)
 */
void clearAll();

// Utility functions
/**
 * @brief List files in directory
 * @param directory Directory path (default: root)
 * @return Vector of filenames
 */
std::vector<String> listFiles(const String& directory = "/");

/**
 * @brief Get free filesystem space
 * @return Free space in bytes
 */
size_t getFreeSpace();

/**
 * @brief Get total filesystem space
 * @return Total space in bytes
 */
size_t getTotalSpace();

}
