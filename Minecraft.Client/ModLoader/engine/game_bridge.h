#pragma once

#include <string>
#include <vector>

class Player;

// Bridge to read game state from the main thread for mod callbacks.
// Used by the player module to implement getWorld(), getLocation(), health, inventory, etc.
// All functions are safe to call from the Node/mod thread (read-only).

namespace GameBridge {

// Get the current level/world name (UTF-8). Returns empty string if not in a world.
void GetCurrentLevelName(std::string *out);

// Get the local player position. Returns false if no player (e.g. in menu).
bool GetLocalPlayerPosition(double *x, double *y, double *z);

// Get the local player display name (UTF-8). Returns "Player" if unavailable.
void GetLocalPlayerName(std::string *out);

// Get local player health (0..20). Returns -1 if no player.
float GetLocalPlayerHealth();

// Get local player game mode: 0 = survival, 1 = creative, 2 = adventure. Returns -1 if no player/level.
int GetLocalPlayerGameModeId();

// Get local player food level (0..20) and saturation. Returns false if no player.
bool GetLocalPlayerFood(int *foodLevel, float *saturation);

// Get local player experience level and total XP. Returns false if no player.
bool GetLocalPlayerExperience(int *level, int *totalXP);

// One inventory slot: item id (0 = empty), count, aux/damage value.
struct ItemSlot {
  int id = 0;
  int count = 0;
  int auxValue = 0;
};

// Inventory snapshot: main slots (36), selected hotbar index (0..8), armor (4).
struct InventoryData {
  std::vector<ItemSlot> slots;
  int selectedSlot = 0;
  std::vector<ItemSlot> armor;
};

// Fill out with local player inventory. Returns false if no player.
// Reads from a cache updated on the game thread (safe to call from mod thread).
bool GetLocalPlayerInventory(InventoryData *out);

// Update the inventory cache from the given player. Call only from the game/main thread.
void UpdateLocalPlayerInventoryCache(Player *player);

// --- Player state (read-only) ---
bool GetLocalPlayerSneaking();
bool GetLocalPlayerSprinting();
bool GetLocalPlayerSelectedItem(ItemSlot *out);
int GetLocalPlayerDimension();
void GetLocalPlayerRotation(float *pitchDeg, float *yawDeg);
bool GetLocalPlayerIsInWater();
bool GetLocalPlayerOnFire();
bool GetLocalPlayerUsingItem();
void GetLocalPlayerAbilities(bool *flying, bool *mayfly, bool *invulnerable);

// --- World/level (read-only) ---
bool GetBlockAt(int x, int y, int z, int *tileId, int *data);
int64_t GetLevelTime();
int64_t GetDayTime();
bool IsRaining();
bool IsThundering();
int GetDifficulty();
int64_t GetLevelSeed();

}  // namespace GameBridge
