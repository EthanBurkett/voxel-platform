#pragma once

#include <string>
#include <vector>

// Registry of all game item/block IDs for the mod SDK (Item enum + getItemId).
// Used by items_module and type_generator.
namespace ItemRegistry {

// Returns item/block id by name (e.g. "DIAMOND_SWORD" -> 267). Returns -1 if not found.
int GetItemId(const char *name);

// Returns all (name, id) pairs for type generator and getItemIds().
const std::vector<std::pair<std::string, int>> &GetAllItemIds();

} // namespace ItemRegistry
