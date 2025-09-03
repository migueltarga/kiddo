/**
 * @file remote_catalog.h
 * @brief Remote story catalog management
 *
 * This file contains the remote_catalog namespace for managing remote
 * story catalogs, including fetching catalog data and managing catalog URLs.
 */

#pragma once
#include <Arduino.h>
#include <vector>

namespace remote_catalog {

/**
 * @brief Structure representing a remote catalog entry
 */
struct Entry {
    String file;    /**< Story filename */
    String name;    /**< Story display name */
    String lang;    /**< Story language */
};

/**
 * @brief Get the current catalog URL
 * @return Current catalog URL string
 */
String getCatalogUrl();

/**
 * @brief Set the catalog URL
 * @param url New catalog URL
 */
void setCatalogUrl(const String& url);

/**
 * @brief Fetch the remote catalog
 * @return True if fetch was successful
 */
bool fetch();

/**
 * @brief Get all catalog entries
 * @return Reference to vector of catalog entries
 */
const std::vector<Entry>& entries();

/**
 * @brief Check if last fetch was successful
 * @return True if last fetch was OK
 */
bool last_ok();

/**
 * @brief Invalidate cached catalog data
 */
void invalidate();

/**
 * @brief Reconcile existing downloaded stories with catalog
 * @return Number of stories reconciled
 */
int reconcileExisting();

/**
 * @brief Ensure a story is downloaded or at least indexed
 * @param file Story filename
 * @param outStoryId Optional output parameter for story ID
 * @return True if story is available
 */
bool ensureDownloadedOrIndexed(const String& file, String* outStoryId = nullptr);

/**
 * @brief Clear all downloaded stories
 * @return Number of stories cleared
 */
int clearDownloads();

}
