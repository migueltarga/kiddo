/**
 * @file story_engine.cpp
 * @brief Story management and parsing engine
 *
 * This module handles loading, parsing, and managing story files.
 * It provides functionality to load stories from the filesystem,
 * parse JSON story definitions, and normalize text content.
 */

#include "story_engine.h"
#include "file_system.h"

namespace story
{

/** @brief Global collection of loaded stories */
std::vector<Story_t> g_stories;

/**
 * @brief Get all loaded stories
 * @return Reference to vector of all stories
 */
const std::vector<Story_t> &all() { return g_stories; }

/**
 * @brief Normalize text content for display
 *
 * Handles line ending normalization and text processing
 * for consistent display across platforms.
 *
 * @param in Input text string
 * @return Normalized text string
 */
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
