#pragma once

#include <Arduino.h>
#include <vector>

#define INVENTORY_SIZE 5

struct InventoryItem_t
{
    String id;
    String name;
    String icon; // URL or path to icon image
    
    InventoryItem_t() = default;
    InventoryItem_t(const String& item_id, const String& item_name, const String& item_icon = "")
        : id(item_id), name(item_name), icon(item_icon) {}
    
    bool isEmpty() const { return id.length() == 0; }
};

struct Choice_t
{
    String text;
    String next;
    String required_item; // Item ID required to show this choice
    String gives_item; // Item ID to add to inventory when this choice is selected
    bool hidden_without_item; // If true, choice is hidden when required_item is not in inventory
};

struct Node_t
{
    String text;
    bool is_end;
    std::vector<Choice_t> choices;
    String gives_item; // Item ID to add to inventory when entering this node
};

struct Story_t
{
    String id;
    String title;
    String start;
    bool has_inventory;
    std::vector<InventoryItem_t> initial_inventory;
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
