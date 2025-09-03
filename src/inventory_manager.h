/**
 * @file inventory_manager.h
 * @brief Inventory management system
 *
 * This file contains the InventoryManager namespace and Inventory class
 * for managing story inventory items, including adding, removing, and
 * tracking items with callback notifications.
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "models.h"

namespace InventoryManager {

typedef void (*ItemAddedCallback)(const InventoryItem_t& item);

/**
 * @brief Manages inventory items with a fixed-size array
 */
class Inventory {
private:
    InventoryItem_t items[INVENTORY_SIZE];
    ItemAddedCallback notification_callback = nullptr;
    
public:
    Inventory();
    
    /**
     * @brief Set callback for item addition notifications
     * @param callback Function to call when items are added
     */
    void setNotificationCallback(ItemAddedCallback callback);
    
    /**
     * @brief Initialize inventory with story's initial items
     * @param initial_items Vector of items to add
     */
    void initialize(const std::vector<InventoryItem_t>& initial_items);
    
    /**
     * @brief Clear all inventory slots
     */
    void clear();
    
    /**
     * @brief Add item to first available slot
     * @param item Item to add
     * @param notify Whether to trigger notification callback
     * @return true if item was added successfully
     */
    bool addItem(const InventoryItem_t& item, bool notify = true);
    
    /**
     * @brief Add item by ID and name
     * @param id Item ID
     * @param name Item display name
     * @param icon Item icon (unused, kept for compatibility)
     * @param notify Whether to trigger notification callback
     * @return true if item was added successfully
     */
    bool addItem(const String& id, const String& name, const String& icon = "", bool notify = true);
    
    /**
     * @brief Remove item by ID
     * @param id Item ID to remove
     * @return true if item was found and removed
     */
    bool removeItem(const String& id);
    
    /**
     * @brief Check if inventory contains item
     * @param id Item ID to check
     * @return true if item is in inventory
     */
    bool hasItem(const String& id) const;
    
    /**
     * @brief Get item by slot index
     * @param slot Slot index (0 to INVENTORY_SIZE-1)
     * @return Reference to item at slot
     */
    const InventoryItem_t& getItem(int slot) const;
    
    /**
     * @brief Get all items (for external iteration)
     * @param out_items Array to fill with items
     */
    void getAllItems(InventoryItem_t* out_items) const;
    
    /**
     * @brief Get number of items in inventory
     * @return Number of non-empty slots
     */
    int getItemCount() const;
    
    /**
     * @brief Check if inventory is full
     * @return true if no empty slots remain
     */
    bool isFull() const;
    
    /**
     * @brief Get first empty slot index
     * @return Slot index or -1 if full
     */
    int getFirstEmptySlot() const;
};

// Global inventory instance
extern Inventory g_inventory;

/**
 * @brief Initialize inventory system
 */
void initialize();

/**
 * @brief Get current inventory instance
 * @return Reference to global inventory
 */
Inventory& getCurrentInventory();

}
