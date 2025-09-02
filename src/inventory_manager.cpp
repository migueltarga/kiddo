#include "inventory_manager.h"

namespace InventoryManager {

Inventory g_inventory;

Inventory::Inventory() {
    clear();
}

void Inventory::initialize(const std::vector<InventoryItem_t>& initial_items) {
    clear();
    for (const auto& item : initial_items) {
        addItem(item);
    }
}

void Inventory::clear() {
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        items[i] = InventoryItem_t();
    }
}

bool Inventory::addItem(const InventoryItem_t& item) {
    if (item.isEmpty()) return false;
    
    int slot = getFirstEmptySlot();
    if (slot == -1) return false; // Inventory full
    
    items[slot] = item;
    return true;
}

bool Inventory::addItem(const String& id, const String& name, const String& icon) {
    return addItem(InventoryItem_t(id, name, icon));
}

bool Inventory::removeItem(const String& id) {
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (items[i].id == id) {
            items[i] = InventoryItem_t(); // Clear slot
            return true;
        }
    }
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
