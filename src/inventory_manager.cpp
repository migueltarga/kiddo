/**
 * @file inventory_manager.cpp
 * @brief Inventory management system
 *
 * This module provides inventory management functionality including
 * adding/removing items, inventory persistence, and notification
 * callbacks for UI updates.
 */

#include "inventory_manager.h"

namespace InventoryManager {

/** @brief Global inventory instance */
Inventory g_inventory;

/**
 * @brief Inventory constructor
 * Initializes empty inventory and clears notification callback
 */
Inventory::Inventory() {
    clear();
    notification_callback = nullptr;
}

/**
 * @brief Set callback for item addition notifications
 * @param callback Function to call when items are added
 */
void Inventory::setNotificationCallback(ItemAddedCallback callback) {
    notification_callback = callback;
}

/**
 * @brief Initialize inventory with story's initial items
 * @param initial_items Vector of items to add
 */
void Inventory::initialize(const std::vector<InventoryItem_t>& initial_items) {
    clear();
    for (const auto& item : initial_items) {
        addItem(item, false);
    }
}

/**
 * @brief Clear all items from inventory
 */
void Inventory::clear() {
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        items[i] = InventoryItem_t();
    }
}

/**
 * @brief Add item to inventory
 * @param item Item to add
 * @param notify Whether to trigger notification callback
 * @return True if item was added successfully
 */
bool Inventory::addItem(const InventoryItem_t& item, bool notify) {
    if (item.isEmpty()) {
        Serial.println("[INVENTORY] Warning: Attempted to add empty item");
        return false;
    }
    
    if (item.id.length() == 0) {
        Serial.println("[INVENTORY] Warning: Attempted to add item with empty ID");
        return false;
    }
    
    int slot = getFirstEmptySlot();
    if (slot == -1) {
        Serial.println("[INVENTORY] Warning: Inventory is full, cannot add item");
        return false; // Inventory full
    }
    
    items[slot] = item;
    
    if (notify && notification_callback) {
        notification_callback(item);
    }
    
    Serial.printf("[INVENTORY] Added item '%s' (ID: %s) to slot %d\n", 
                  item.name.c_str(), item.id.c_str(), slot);
    return true;
}

bool Inventory::addItem(const String& id, const String& name, const String& icon, bool notify) {
    return addItem(InventoryItem_t(id, name, icon), notify);
}

bool Inventory::removeItem(const String& id) {
    if (id.length() == 0) {
        Serial.println("[INVENTORY] Warning: Attempted to remove item with empty ID");
        return false;
    }
    
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (items[i].id == id) {
            Serial.printf("[INVENTORY] Removed item '%s' (ID: %s) from slot %d\n", 
                          items[i].name.c_str(), items[i].id.c_str(), i);
            items[i] = InventoryItem_t(); // Clear slot
            return true;
        }
    }
    
    Serial.printf("[INVENTORY] Warning: Item with ID '%s' not found in inventory\n", id.c_str());
    return false;
}

bool Inventory::hasItem(const String& id) const {
    if (id.length() == 0) return true; // Empty requirement always satisfied
    
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (items[i].id == id) {
            return true;
        }
    }
    return false;
}

const InventoryItem_t& Inventory::getItem(int slot) const {
    if (slot < 0 || slot >= INVENTORY_SIZE) {
        Serial.printf("[INVENTORY] Warning: Invalid slot index %d, returning empty item\n", slot);
        static InventoryItem_t empty;
        return empty;
    }
    return items[slot];
}

void Inventory::getAllItems(InventoryItem_t* out_items) const {
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        out_items[i] = items[i];
    }
}

int Inventory::getItemCount() const {
    int count = 0;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (!items[i].isEmpty()) {
            count++;
        }
    }
    return count;
}

bool Inventory::isFull() const {
    return getFirstEmptySlot() == -1;
}

int Inventory::getFirstEmptySlot() const {
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (items[i].isEmpty()) {
            return i;
        }
    }
    return -1;
}

void initialize() {
    g_inventory.clear();
}

Inventory& getCurrentInventory() {
    return g_inventory;
}

}
