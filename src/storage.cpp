#include "storage.h"
#include <SPIFFS.h>

namespace storage
{
    String readFileToString(const char *path)
    {
        fs::File f = SPIFFS.open(path, "r");
        if (!f)
            return String();
        String out;
        out.reserve(f.size() + 16);
        while (f.available())
            out += (char)f.read();
        f.close();
        return out;
    }
    bool writeStringToFile(const char *path, const String &data)
    {
        fs::File f = SPIFFS.open(path, FILE_WRITE);
        if (!f)
            return false;
        size_t w = f.print(data);
        f.close();
        return w == data.length();
    }
    bool exists(const char *path)
    {
        return SPIFFS.exists(path);
    }
}
