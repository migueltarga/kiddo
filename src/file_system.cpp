#include "file_system.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <MD5Builder.h>
#include <HTTPClient.h>
#include <WiFi.h>

namespace FileSystem {

static bool fs_initialized = false;

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
    
    *br = file->read((uint8_t*)buf, btr);
    return (*br == btr) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
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
    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, ("S:" + path).c_str(), LV_FS_MODE_RD);
    if (res == LV_FS_RES_OK) {
        lv_fs_close(&file);
        return true;
    }
    return false;
}

String readFile(const String& path) {
    lv_fs_file_t file;
    if (lv_fs_open(&file, ("S:" + path).c_str(), LV_FS_MODE_RD) != LV_FS_RES_OK) {
        return "";
    }
    
    uint32_t size;
    lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    lv_fs_tell(&file, &size);
    lv_fs_seek(&file, 0, LV_FS_SEEK_SET);
    
    if (size == 0) {
        lv_fs_close(&file);
        return "";
    }
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        lv_fs_close(&file);
        return "";
    }
    
    uint32_t bytesRead;
    lv_fs_res_t res = lv_fs_read(&file, buffer, size, &bytesRead);
    lv_fs_close(&file);
    
    if (res != LV_FS_RES_OK || bytesRead != size) {
        free(buffer);
        return "";
    }
    
    buffer[size] = '\0';
    String content = String(buffer);
    free(buffer);
    return content;
}

bool writeFile(const String& path, const String& content) {
    lv_fs_file_t file;
    if (lv_fs_open(&file, ("S:" + path).c_str(), LV_FS_MODE_WR) != LV_FS_RES_OK) {
        Serial.println("[FILE_SYSTEM] Failed to open file for writing: " + path);
        return false;
    }
    
    uint32_t bytesWritten;
    lv_fs_res_t res = lv_fs_write(&file, content.c_str(), content.length(), &bytesWritten);
    lv_fs_close(&file);
    
    if (res != LV_FS_RES_OK || bytesWritten != content.length()) {
        Serial.printf("[FILE_SYSTEM] Write failed: res=%d, written=%d, expected=%d\n", 
                     res, bytesWritten, content.length());
        return false;
    }
    
    return true;
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
    return exists(getCachedImagePath(url));
}

bool cacheImage(const String& url, const uint8_t* data, size_t size) {
    String path = getCachedImagePath(url);
    
    Serial.printf("[FILE_SYSTEM] Caching image to: %s (%d bytes)\n", path.c_str(), size);
    
    lv_fs_file_t file;
    if (lv_fs_open(&file, ("S:" + path).c_str(), LV_FS_MODE_WR) != LV_FS_RES_OK) {
        Serial.printf("[FILE_SYSTEM] Failed to open cache file for writing: %s\n", path.c_str());
        return false;
    }
    
    uint32_t bytesWritten;
    lv_fs_res_t res = lv_fs_write(&file, data, size, &bytesWritten);
    lv_fs_close(&file);
    
    if (res != LV_FS_RES_OK || bytesWritten != size) {
        Serial.printf("[FILE_SYSTEM] Cache write failed: res=%d, written=%d, expected=%d\n", 
                     res, bytesWritten, size);
        return false;
    }
    
    Serial.printf("[FILE_SYSTEM] Successfully cached image: %s (%d bytes)\n", path.c_str(), size);
    return true;
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
    if (!loadIndex(doc)) return false;
    
    if (!doc["stories"].is<JsonArray>()) return false;
    
    JsonArrayConst stories = doc["stories"];
    for (JsonObjectConst story : stories) {
        const char* f = story["file"] | "";
        if (file == f) return true;
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

bool httpGet(const String& url, String& response) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[FILE_SYSTEM] WiFi not connected");
        return false;
    }
    
    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        http.end();
        return true;
    }
    
    Serial.printf("[FILE_SYSTEM] HTTP GET failed: %d\n", httpCode);
    http.end();
    return false;
}

bool downloadFile(const String& url, const String& localPath) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[FILE_SYSTEM] WiFi not connected");
        return false;
    }
    
    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        WiFiClient* stream = http.getStreamPtr();
        
        File file = SPIFFS.open(localPath, "w");
        if (!file) {
            Serial.println("[FILE_SYSTEM] Failed to open file for writing: " + localPath);
            http.end();
            return false;
        }
        
        uint8_t buffer[1024];
        int len = http.getSize();
        int totalRead = 0;
        
        while (http.connected() && (len > 0 || len == -1)) {
            size_t size = stream->available();
            if (size) {
                int c = stream->readBytes(buffer, min(size, sizeof(buffer)));
                file.write(buffer, c);
                totalRead += c;
                if (len > 0) len -= c;
            }
            delay(1);
        }
        
        file.close();
        http.end();
        
        Serial.printf("[FILE_SYSTEM] Downloaded %d bytes to %s\n", totalRead, localPath.c_str());
        return totalRead > 0;
    }
    
    Serial.printf("[FILE_SYSTEM] Download failed: %d\n", httpCode);
    http.end();
    return false;
}

void clearCache() {
    std::vector<String> cacheFiles = listFiles("/cache");
    for (const String& filename : cacheFiles) {
        String path = "/cache/" + filename;
        if (deleteFile(path)) {
        }
    }
}

void clearStories() {
    std::vector<String> allFiles = listFiles("/");
    for (const String& filename : allFiles) {
        if (filename.endsWith(".json") && filename != "index.json") {
            String path = "/" + filename;
            if (deleteFile(path)) {
            }
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
