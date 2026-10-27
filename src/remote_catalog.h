/**
 * @file remote_catalog.h
 * @brief Remote story catalog management
 *
 * This file contains the remote_catalog namespace for managing remote
 * story catalogs, including fetching catalog data and managing catalog URLs.
 *
 * Threading: the "worker" functions only touch the network and SPIFFS and may
 * run on the AsyncManager task. Everything else reads or writes the cached
 * catalog and must only be called from the main (LVGL) thread.
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

/** @brief A fetched catalog index */
struct Catalog {
    String name;                /**< Optional "name" from the index, shown when picking a catalog */
    std::vector<Entry> entries; /**< Valid entries, any language */
};

/** @brief A catalog the reader can switch back to */
struct SavedCatalog {
    String url;     /**< Catalog index URL */
    String name;    /**< Name from its index, empty until it has been fetched */
};

// ---------------- Main thread ----------------

/**
 * @brief Get the current catalog URL
 * @return Current catalog URL string
 */
String getCatalogUrl();

/**
 * @brief Set the catalog URL and remember it in the saved list
 * @param url New catalog URL (empty restores the default)
 */
void setCatalogUrl(const String& url);

/**
 * @brief Catalogs to pick from: the default first, then the others used on
 * this device, most recent first
 *
 * Typing a long URL on the on-screen keyboard is slow, so every catalog that
 * was set once stays one tap away.
 */
std::vector<SavedCatalog> savedCatalogs();

/**
 * @brief Check whether a URL is the built-in default catalog
 */
bool isDefaultCatalog(const String& url);

/**
 * @brief Get catalog entries for the current language
 * @return Reference to vector of catalog entries
 */
const std::vector<Entry>& entries();

/**
 * @brief Check if last fetch was successful
 * @return True if last fetch was OK
 */
bool last_ok();

/**
 * @brief Check if the cached entries can be reused without fetching
 * @return True if fetched recently for the current language
 */
bool isFresh();

/**
 * @brief Replace the cached entries with a fetch result
 * @param ok True if the fetch succeeded
 * @param fetched Catalog returned by fetchEntries()
 */
void applyFetched(bool ok, const Catalog& fetched);

/**
 * @brief Invalidate cached catalog data
 */
void invalidate();

/**
 * @brief Build the download URL for a catalog story file
 * @param file Story filename from the catalog
 * @return Absolute URL next to the catalog index
 */
String storyUrl(const String& file);

/**
 * @brief Add a downloaded story file to the local index
 * @param file Story filename from the catalog
 * @return True if the story is indexed
 */
bool indexDownloaded(const String& file);

/**
 * @brief Reconcile existing downloaded stories with catalog
 * @return Number of stories added to the index
 */
int reconcileExisting();

// ---------------- Worker safe ----------------

/**
 * @brief Check that a catalog filename is safe to store locally
 *
 * Rejects paths, the index file itself, and names too long for SPIFFS.
 *
 * @param file Story filename from the catalog
 * @return True if the name can be used as "/<file>"
 */
bool isValidFileName(const String& file);

/**
 * @brief Download and parse the catalog index
 * @param url Catalog URL
 * @param out Filled with the catalog name and all valid entries (any language)
 * @return True if the catalog was fetched and contains stories
 */
bool fetchEntries(const String& url, Catalog& out);

/**
 * @brief Ensure a story file exists locally, downloading it if needed
 * @param url Story URL
 * @param localPath Local SPIFFS path
 * @param lang Language to record if the story does not declare one
 * @param outStoryId Filled with the story ID
 * @return True if a valid story file is available
 */
bool downloadStory(const String& url, const String& localPath, const String& lang, String& outStoryId);

}
