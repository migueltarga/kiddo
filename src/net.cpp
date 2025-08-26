#include "net.h"
#include <Arduino.h>
#include <HTTPClient.h>

namespace net
{
    bool httpGet(const String &url, String &out)
    {
        HTTPClient http;
        http.setTimeout(10000);
        if (!http.begin(url))
            return false;
        int code = http.GET();
        if (code == HTTP_CODE_OK)
        {
            out = http.getString();
            http.end();
            return true;
        }
        http.end();
        return false;
    }
}
