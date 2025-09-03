#pragma once

#include <Arduino.h>
#include <vector>

#define INVENTORY_SIZE 5

/**
 * @brief Structure representing an inventory item
 */
struct InventoryItem_t
{
    String id;      /**< Unique identifier for the item */
    String name;    /**< Display name of the item */
    String icon;    /**< URL or path to icon image */

    /**
     * @brief Default constructor
     */
    InventoryItem_t() = default;

    /**
     * @brief Constructor with parameters
     * @param item_id Unique identifier
     * @param item_name Display name
     * @param item_icon Icon path/URL (optional)
     */
    InventoryItem_t(const String& item_id, const String& item_name, const String& item_icon = "")
        : id(item_id), name(item_name), icon(item_icon) {}

    /**
     * @brief Check if item is empty
     * @return True if item has no ID
     */
    bool isEmpty() const { return id.length() == 0; }
};

/**
 * @brief Structure representing an item definition
 */
struct ItemDefinition_t
{
    String id;           /**< Unique identifier for the item */
    String name;         /**< Display name of the item */
    String description;  /**< Description of the item */
    String icon_url;     /**< URL to remote icon image */
    String local_icon;   /**< Path to local icon image */

    /**
     * @brief Default constructor
     */
    ItemDefinition_t() = default;

    /**
     * @brief Constructor with parameters
     * @param item_id Unique identifier
     * @param item_name Display name
     * @param item_desc Item description
     * @param item_icon_url Remote icon URL (optional)
     * @param item_local_icon Local icon path (optional)
     */
    ItemDefinition_t(const String& item_id, const String& item_name, const String& item_desc, const String& item_icon_url = "", const String& item_local_icon = "")
        : id(item_id), name(item_name), description(item_desc), icon_url(item_icon_url), local_icon(item_local_icon) {}
};

/**
 * @brief Structure representing a story choice
 */
struct Choice_t
{
    String text;               /**< Display text for the choice */
    String next;               /**< ID of next story node */
    String required_item;      /**< Item ID required to show this choice */
    String gives_item;         /**< Item ID to add to inventory when selected */
    bool hidden_without_item;  /**< Hide choice if required item not in inventory */
};

/**
 * @brief Structure representing a story node
 */
struct Node_t
{
    String text;                    /**< Node text content */
    bool is_end;                    /**< True if this is the story end */
    std::vector<Choice_t> choices;  /**< Available choices from this node */
    String gives_item;              /**< Item ID to add to inventory on entry */

    // Inventory choice fields
    bool inventory_choice;     /**< True if node shows inventory for selection */
    String correct_item;       /**< Correct item ID for inventory choice */
    String success_next;       /**< Next node if correct item chosen */
    String failure_next;       /**< Next node if wrong item chosen */
};

/**
 * @brief Structure representing a complete story
 */
struct Story_t
{
    String id;                              /**< Unique story identifier */
    String title;                           /**< Story title */
    String start;                           /**< ID of starting node */
    bool has_inventory;                     /**< True if story uses inventory system */
    std::vector<ItemDefinition_t> item_definitions;     /**< All item definitions */
    std::vector<InventoryItem_t> initial_inventory;     /**< Starting inventory */
    std::vector<std::pair<String, Node_t>> nodes;       /**< Story nodes (ID -> Node) */

    /**
     * @brief Get a node by ID
     * @param k Node ID to find
     * @return Pointer to node, or nullptr if not found
     */
    const Node_t *get(const String &k) const
    {
        for (const auto &kv : nodes)
        {
            if (kv.first == k)
                return &kv.second;
        }
        return nullptr;
    }

    /**
     * @brief Get an item definition by ID
     * @param item_id Item ID to find
     * @return Pointer to item definition, or nullptr if not found
     */
    const ItemDefinition_t *getItemDefinition(const String &item_id) const
    {
        for (const auto &item : item_definitions)
        {
            if (item.id == item_id)
                return &item;
        }
        return nullptr;
    }
};
