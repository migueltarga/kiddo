#include "story_engine.h"
#include "storage.h"
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

    // --- Sample stories (truncated to keep file manageable) ---
    void ensureSampleStories()
    {
        const char *indexJson = R"JSON({
            "stories": [
                { "file": "/story_sample.json", "name": "Sample Story", "lang": "en" },
                { "file": "/story_sample_pt.json", "name": "História de Exemplo", "lang": "pt-br" }
            ]
        })JSON";
        const char *sampleStory = R"JSON({
  "id":"sample",
  "title":"Sample Story",
  "lang":"en",
  "start":"start",
  "nodes":{
    "start": {
      "text":"This is a sample story.",
      "choices":[{"text":"The End","next":"end"}]
    },
    "end": {
      "text":"Thanks for reading!",
      "end":true
    }
  }
})JSON";
        const char *sampleStoryPt = R"JSON({
  "id":"sample_pt",
  "title":"História Exemplo",
  "lang":"pt-br",
  "start":"start",
  "nodes":{
    "start": {
      "text":"Esta é uma história exemplo.",
      "choices":[{"text":"O Fim","next":"end"}]
    },
    "end": {
      "text":"Obrigado por ler!",
      "end":true
    }
  }
})JSON";
        storage::writeStringToFile("/index.json", indexJson);
        storage::writeStringToFile("/story_sample.json", sampleStory);
        storage::writeStringToFile("/story_sample_pt.json", sampleStoryPt);
    }

}
