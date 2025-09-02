#pragma once

#include <Arduino.h>
#include <vector>


struct Choice_t
{
    String text;
    String next;
};

struct Node_t
{
    String text;
    bool is_end;
    std::vector<Choice_t> choices;
};

struct Story_t
{
    String id;
    String title;
    String start;
    std::vector<std::pair<String, Node_t>> nodes;

    const Node_t *get(const String &k) const
    {
        for (const auto &kv : nodes)
        {
            if (kv.first == k)
                return &kv.second;
        }
        return nullptr;
    }
};
