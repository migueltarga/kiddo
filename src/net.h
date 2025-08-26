#pragma once
class String;
namespace net
{
    bool httpGet(const String &url, String &out);
}
