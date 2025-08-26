#pragma once
class String;
namespace fs
{
    class File;
}

namespace storage
{
    String readFileToString(const char *path);
    bool writeStringToFile(const char *path, const String &data);
    bool exists(const char *path);
}
