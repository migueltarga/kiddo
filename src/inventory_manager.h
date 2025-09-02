#pragma once

#include <Arduino.h>
#include <vector>
#include "models.h"

namespace InventoryManager {

class Inventory {
private:
    InventoryItem_t items[INVENTORY_SIZE];
    
public:
    Inventory();
    
    // Initialize inventory with story's initial items
    void initialize(const std::vector<InventoryItem_t>& initial_items);
    
    // Clear all inventory slots
    void clear();
    
    // Add item to first available slot
    bool addItem(const InventoryItem_t& item);
    bool addItem(const String& id, const String& name, const String& icon = "");
    
    // Remove item by ID
    bool removeItem(const String& id);
    
    // Check if inventory contains item
    bool hasItem(const String& id) const;
    
    // Get item by slot index
    const InventoryItem_t& getItem(int slot) const;
    
    // Get all items
    void getAllItems(InventoryItem_t* out_items) const;
    
    // Get number of items in inventory
    int getItemCount() const;
    
    // Check if inventory is full
    bool isFull() const;
    
    // Get first empty slot index (-1 if full)
    int getFirstEmptySlot() const;
};

// Global inventory instance
extern Inventory g_inventory;

// Initialize inventory system
void initialize();

// Get current inventory
Inventory& getCurrentInventory();

}
