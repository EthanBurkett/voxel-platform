#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Level;
class Player;

namespace ModActionQueue {

// Called from mod (Node) thread — thread-safe enqueue.
void EnqueueSetBlock(int x, int y, int z, int tileId, int data);
void EnqueueSetPlayerHealth(float health);
void EnqueueSetFoodLevel(int level);
void EnqueueSetSaturation(float saturation);
void EnqueueSetPosition(double x, double y, double z);
void EnqueueSetRotation(float pitch, float yaw);
void EnqueueSetGameMode(int modeId); // 0 survival, 1 creative, 2 adventure
void EnqueueSetLevelTime(int64_t ticks);
void EnqueueSetDayTime(int64_t ticks);
void EnqueueSetRaining(bool on);
void EnqueueSetThundering(bool on);
void EnqueueSetDifficulty(int difficulty);
void EnqueueGiveExperienceLevels(int levels);
void EnqueueGiveItem(int itemId, int count, int auxValue);
/** Give item with optional display name and lore (UTF-8). loreLines can be null or empty. */
void EnqueueGiveItemStack(int itemId, int count, int auxValue,
                          const char *displayNameUtf8,
                          const std::vector<std::string> *loreLines);
void EnqueueClearInventory(); // clear main inventory + armor slots

// Called only from game main thread — applies and clears queue.
void ApplyPendingActions(Level *level, Player *player);

} // namespace ModActionQueue
