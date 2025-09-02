#include "story_engine.h"
#include "file_system.h"
namespace story
{

    std::vector<Story_t> g_stories;

    const std::vector<Story_t> &all() { return g_stories; }

    String normalizeText(const String &in)
    {
        String out;
        out.reserve(in.length() + 8);
        bool lastWasCR = false;
        for (size_t i = 0; i < in.length(); ++i)
        {
            char c = in[i];
            if (c == '\r')
            {
                lastWasCR = true;
                continue;
            }
            if (c == '\n')
            {
                out += '\n';
                lastWasCR = false;
                continue;
            }
            lastWasCR = false;
            out += c;
        }
        String cleaned;
        cleaned.reserve(out.length());
        bool atLineStart = true;
        bool spaceRun = false;
        for (size_t i = 0; i < out.length(); ++i)
        {
            char c = out[i];
            if (c == '\n')
            {
                while (cleaned.length() && cleaned[cleaned.length() - 1] == ' ')
                    cleaned.remove(cleaned.length() - 1);
                cleaned += '\n';
                atLineStart = true;
                spaceRun = false;
                continue;
            }
            if (c == ' ')
            {
                if (atLineStart)
                    continue;
                if (spaceRun)
                    continue;
                spaceRun = true;
                cleaned += c;
                continue;
            }
            spaceRun = false;
            atLineStart = false;
            cleaned += c;
        }
        return cleaned;
    }

}
