/**
 * @file file_system.cpp
 * @brief Filesystem operations and LVGL filesystem driver
 *
 * This module provides filesystem operations including file I/O,
 * image caching, story management, and HTTP downloads. It also
 * implements the LVGL filesystem driver for SPIFFS integration.
 */

#include "file_system.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

/** @brief Root CA bundle embedded from certs/x509_crt_bundle.bin (see platformio.ini) */
extern const uint8_t x509_crt_bundle_start[] asm("_binary_certs_x509_crt_bundle_bin_start");

namespace FileSystem {

/** @brief Filesystem initialization flag */
static bool fs_initialized = false;

/**
 * @brief LVGL filesystem open callback
 * @param drv LVGL filesystem driver
 * @param path File path
 * @param mode Open mode
 * @return File handle or nullptr on error
 */
static void* fs_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode) {
    const char* flags = "";
    if (mode == LV_FS_MODE_WR) flags = "w";
    else if (mode == LV_FS_MODE_RD) flags = "r";
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = "r+";

    File* file = new File();
    *file = SPIFFS.open(path, flags);
    if (!*file) {
        delete file;
        return nullptr;
    }
    return file;
}

/**
 * @brief LVGL filesystem close callback
 * @param drv LVGL filesystem driver
 * @param file_p File handle
 * @return LVGL filesystem result
 */
static lv_fs_res_t fs_close(lv_fs_drv_t* drv, void* file_p) {
    File* file = (File*)file_p;
    if (file) {
        file->close();
        delete file;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_read(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br) {
    File* file = (File*)file_p;
    if (!file) return LV_FS_RES_INV_PARAM;
    
    // A short read at end of file is not an error; LVGL checks *br itself
    *br = file->read((uint8_t*)buf, btr);
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_write(lv_fs_drv_t* drv, void* file_p, const void* buf, uint32_t btw, uint32_t* bw) {
    File* file = (File*)file_p;
    if (!file) return LV_FS_RES_INV_PARAM;
    
    *bw = file->write((const uint8_t*)buf, btw);
    return (*bw == btw) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence) {
    File* file = (File*)file_p;
    if (!file) return LV_FS_RES_INV_PARAM;
    
    SeekMode mode;
    switch (whence) {
        case LV_FS_SEEK_SET: mode = SeekSet; break;
        case LV_FS_SEEK_CUR: mode = SeekCur; break;
        case LV_FS_SEEK_END: mode = SeekEnd; break;
        default: return LV_FS_RES_INV_PARAM;
    }
    
    return file->seek(pos, mode) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

static lv_fs_res_t fs_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p) {
    File* file = (File*)file_p;
    if (!file) return LV_FS_RES_INV_PARAM;
    
    *pos_p = file->position();
    return LV_FS_RES_OK;
}

bool init() {
    if (fs_initialized) return true;
    
    if (!SPIFFS.begin(true)) {
        Serial.println("[FILE_SYSTEM] SPIFFS initialization failed");
        return false;
    }
    
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    Serial.printf("[FILE_SYSTEM] SPIFFS initialized: %d/%d bytes used (%.1f%%)\n", 
                  used, total, (float)used * 100.0 / total);
    
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);
    fs_drv.letter = 'S';
    fs_drv.cache_size = 0;
    
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;
    
    lv_fs_drv_register(&fs_drv);
    
    fs_initialized = true;
    return true;
}

bool exists(const String& path) {
    return SPIFFS.exists(path);
}

size_t fileSize(const String& path) {
    File file = SPIFFS.open(path, "r");
    if (!file) return 0;
    size_t size = file.size();
    file.close();
    return size;
}

String readFile(const String& path) {
    File file = SPIFFS.open(path, "r");
    if (!file) return "";

    size_t size = file.size();
    if (size == 0) {
        file.close();
        return "";
    }

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        file.close();
        return "";
    }

    size_t bytesRead = file.read((uint8_t*)buffer, size);
    file.close();

    if (bytesRead != size) {
        free(buffer);
        return "";
    }

    buffer[size] = '\0';
    String content = String(buffer);
    free(buffer);
    return content;
}

bool writeFile(const String& path, const String& content) {
    File file = SPIFFS.open(path, "w");
    if (!file) return false;

    size_t bytesWritten = file.write((const uint8_t*)content.c_str(), content.length());
    file.close();
    return bytesWritten == content.length();
}

bool deleteFile(const String& path) {
    return SPIFFS.remove(path);
}

bool saveStory(const String& filename, const String& content) {
    return writeFile("/" + filename, content);
}

String loadStory(const String& filename) {
    return readFile("/" + filename);
}

bool deleteStory(const String& filename) {
    return deleteFile("/" + filename);
}

String getCachedImagePath(const String& url) {
    uint32_t hash = 0;
    for (size_t i = 0; i < url.length(); i++) {
        hash = hash * 31 + url[i];
    }
    return "/cache/img_" + String(hash, HEX) + ".jpg";
}

bool isImageCached(const String& url) {
    return fileSize(getCachedImagePath(url)) > 0;
}

bool loadIndex(JsonDocument& doc) {
    String content = readFile("/index.json");
    if (content.length() == 0) {
        doc.clear();
        doc["stories"].to<JsonArray>();
        return true;
    }
    return deserializeJson(doc, content) == DeserializationError::Ok;
}

bool saveIndex(const JsonDocument& doc) {
    String output;
    serializeJson(doc, output);
    return writeFile("/index.json", output);
}

bool indexContains(const String& file) {
    JsonDocument doc;
    if (!loadIndex(doc)) {
        return false;
    }
    
    if (!doc["stories"].is<JsonArray>()) {
        return false;
    }
    
    JsonArrayConst stories = doc["stories"];
    for (JsonObjectConst story : stories) {
        const char* f = story["file"] | "";
        if (file == f) {
            return true;
        }
    }
    return false;
}

bool addToIndex(const String& file, const String& name, const String& lang) {
    JsonDocument doc;
    if (!loadIndex(doc)) return false;
    
    if (!doc["stories"].is<JsonArray>()) {
        doc["stories"].to<JsonArray>();
    }
    
    JsonArrayConst stories = doc["stories"];
    for (JsonObjectConst story : stories) {
        const char* f = story["file"] | "";
        if (file == f) return true;
    }
    
    JsonArray storiesArray = doc["stories"];
    JsonObject newStory = storiesArray.add<JsonObject>();
    newStory["file"] = file;
    if (name.length() > 0) newStory["name"] = name;
    if (lang.length() > 0) newStory["lang"] = lang;
    
    return saveIndex(doc);
}

bool removeFromIndex(const String& file) {
    JsonDocument doc;
    if (!loadIndex(doc)) return false;
    
    if (!doc["stories"].is<JsonArray>()) return false;
    
    JsonArrayConst stories = doc["stories"];
    JsonDocument newDoc;
    JsonArray newStories = newDoc["stories"].to<JsonArray>();
    
    bool found = false;
    for (JsonObjectConst story : stories) {
        const char* f = story["file"] | "";
        if (file != f) {
            JsonObject newStory = newStories.add<JsonObject>();
            newStory["file"] = f;
            if (story["name"].is<const char*>()) {
                newStory["name"] = story["name"].as<const char*>();
            }
            if (story["lang"].is<const char*>()) {
                newStory["lang"] = story["lang"].as<const char*>();
            }
        } else {
            found = true;
        }
    }
    
    if (!found) return false;
    return saveIndex(newDoc);
}

std::vector<String> listFiles(const String& directory) {
    std::vector<String> files;
    File root = SPIFFS.open(directory);
    if (!root || !root.isDirectory()) return files;
    
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            files.push_back(file.name());
        }
        file = root.openNextFile();
    }
    return files;
}

/**
 * @brief Start an HTTP(S) GET request
 *
 * HTTPS connections are verified against the embedded root CA bundle;
 * without it the TLS client would accept any certificate.
 * The clients must outlive the request, so the caller owns them.
 */
static int httpBeginGet(HTTPClient& http, WiFiClientSecure& tls, WiFiClient& plain, const String& url) {
    bool ok;
    if (url.startsWith("https://")) {
        tls.setCACertBundle(x509_crt_bundle_start);
        ok = http.begin(tls, url);
    } else {
        ok = http.begin(plain, url);
    }
    if (!ok) return HTTPC_ERROR_CONNECTION_REFUSED;

    http.setTimeout(15000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    return http.GET();
}

bool httpGet(const String& url, String& response) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    WiFiClientSecure tls;
    WiFiClient plain;
    HTTPClient http;
    int httpCode = httpBeginGet(http, tls, plain, url);
    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        http.end();
        return true;
    }

    Serial.printf("[FILE_SYSTEM] GET %s failed: %d\n", url.c_str(), httpCode);
    http.end();
    return false;
}

bool downloadFile(const String& url, const String& localPath) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[FILE_SYSTEM] WiFi not connected");
        return false;
    }

    WiFiClientSecure tls;
    WiFiClient plain;
    HTTPClient http;
    int httpCode = httpBeginGet(http, tls, plain, url);
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[FILE_SYSTEM] Download failed: %d\n", httpCode);
        http.end();
        return false;
    }

    // Download to a temporary file so an interrupted transfer never leaves
    // a truncated file at the final path.
    String tmpPath = localPath + ".tmp";
    File file = SPIFFS.open(tmpPath, "w");
    if (!file) {
        Serial.println("[FILE_SYSTEM] Failed to open file for writing: " + tmpPath);
        http.end();
        return false;
    }

    int expected = http.getSize();
    int written = http.writeToStream(&file);
    file.close();
    http.end();

    if (written <= 0 || (expected > 0 && written != expected)) {
        Serial.printf("[FILE_SYSTEM] Incomplete download of %s: %d of %d bytes\n",
                      url.c_str(), written, expected);
        SPIFFS.remove(tmpPath);
        return false;
    }

    if (SPIFFS.exists(localPath)) {
        SPIFFS.remove(localPath);
    }
    if (!SPIFFS.rename(tmpPath, localPath)) {
        Serial.println("[FILE_SYSTEM] Failed to rename " + tmpPath);
        SPIFFS.remove(tmpPath);
        return false;
    }

    Serial.printf("[FILE_SYSTEM] Downloaded %d bytes to %s\n", written, localPath.c_str());
    return true;
}

void clearCache() {
    std::vector<String> cacheFiles = listFiles("/cache");
    for (const String& filename : cacheFiles) {
        deleteFile("/cache/" + filename);
    }
}

void clearStories() {
    std::vector<String> allFiles = listFiles("/");
    for (const String& filename : allFiles) {
        if (filename.endsWith(".json") && filename != "index.json") {
            deleteFile("/" + filename);
        }
    }
    
    JsonDocument emptyIndex;
    emptyIndex["stories"].to<JsonArray>();
    saveIndex(emptyIndex);
}

void clearAll() {
    clearCache();
    clearStories();
}

size_t getFreeSpace() {
    return SPIFFS.totalBytes() - SPIFFS.usedBytes();
}

size_t getTotalSpace() {
    return SPIFFS.totalBytes();
}

}
