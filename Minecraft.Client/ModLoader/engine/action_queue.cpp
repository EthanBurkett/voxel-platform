#include "stdafx.h"
#include "action_queue.h"
#include "game_bridge.h"
#include <deque>
#include <mutex>

#ifndef _XBOX
#include "../../../Minecraft.World/CompoundTag.h"
#include "../../../Minecraft.World/FoodData.h"
#include "../../../Minecraft.World/Inventory.h"
#include "../../../Minecraft.World/Item.h"
#include "../../../Minecraft.World/ItemInstance.h"
#include "../../../Minecraft.World/Level.h"
#include "../../../Minecraft.World/LevelData.h"
#include "../../../Minecraft.World/LevelSettings.h"
#include "../../../Minecraft.World/ListTag.h"
#include "../../../Minecraft.World/Player.h"
#include "../../../Minecraft.World/StringTag.h"
#include "../../../Minecraft.World/Tile.h"
#include "../../Minecraft.h"
#include "../../MinecraftServer.h"
#include "../../PlayerList.h"
#include "../../ServerLevel.h"
#include "../../ServerPlayer.h"
#ifdef _WIN32
#include <windows.h>
#endif
#endif

namespace ModActionQueue {

enum Kind : int {
  KSetBlock,
  KSetHealth,
  KSetFood,
  KSetSaturation,
  KSetPosition,
  KSetRotation,
  KSetGameMode,
  KSetLevelTime,
  KSetDayTime,
  KSetRaining,
  KSetThundering,
  KSetDifficulty,
  KGiveXPLevels,
  KGiveItem,
  KClearInventory,
};

struct Entry {
  Kind kind;
  int i[8];
  int64_t i64[2];
  float f[4];
  double d[3];
  bool b[2];
};

struct GiveItemStackEntry {
  int id = 0;
  int count = 1;
  int aux = 0;
  std::string displayName;
  std::vector<std::string> lore;
};

static std::mutex s_mutex;
static std::deque<Entry> s_queue;
static std::mutex s_stackMutex;
static std::deque<GiveItemStackEntry> s_giveStackQueue;

static void push(const Entry &e) {
  std::lock_guard<std::mutex> lock(s_mutex);
  s_queue.push_back(e);
}

void EnqueueSetBlock(int x, int y, int z, int tileId, int data) {
  Entry e{};
  e.kind = KSetBlock;
  e.i[0] = x;
  e.i[1] = y;
  e.i[2] = z;
  e.i[3] = tileId;
  e.i[4] = data;
  push(e);
}

void EnqueueSetPlayerHealth(float health) {
  Entry e{};
  e.kind = KSetHealth;
  e.f[0] = health;
  push(e);
}

void EnqueueSetFoodLevel(int level) {
  Entry e{};
  e.kind = KSetFood;
  e.i[0] = level;
  push(e);
}

void EnqueueSetSaturation(float saturation) {
  Entry e{};
  e.kind = KSetSaturation;
  e.f[0] = saturation;
  push(e);
}

void EnqueueSetPosition(double x, double y, double z) {
  Entry e{};
  e.kind = KSetPosition;
  e.d[0] = x;
  e.d[1] = y;
  e.d[2] = z;
  push(e);
}

void EnqueueSetRotation(float pitch, float yaw) {
  Entry e{};
  e.kind = KSetRotation;
  e.f[0] = pitch;
  e.f[1] = yaw;
  push(e);
}

void EnqueueSetGameMode(int modeId) {
  Entry e{};
  e.kind = KSetGameMode;
  e.i[0] = modeId;
  push(e);
}

void EnqueueSetLevelTime(int64_t ticks) {
  Entry e{};
  e.kind = KSetLevelTime;
  e.i64[0] = ticks;
  push(e);
}

void EnqueueSetDayTime(int64_t ticks) {
  Entry e{};
  e.kind = KSetDayTime;
  e.i64[0] = ticks;
  push(e);
}

void EnqueueSetRaining(bool on) {
  Entry e{};
  e.kind = KSetRaining;
  e.b[0] = on;
  push(e);
}

void EnqueueSetThundering(bool on) {
  Entry e{};
  e.kind = KSetThundering;
  e.b[0] = on;
  push(e);
}

void EnqueueSetDifficulty(int difficulty) {
  Entry e{};
  e.kind = KSetDifficulty;
  e.i[0] = difficulty;
  push(e);
}

void EnqueueGiveExperienceLevels(int levels) {
  Entry e{};
  e.kind = KGiveXPLevels;
  e.i[0] = levels;
  push(e);
}

void EnqueueGiveItem(int itemId, int count, int auxValue) {
  Entry e{};
  e.kind = KGiveItem;
  e.i[0] = itemId;
  e.i[1] = count;
  e.i[2] = auxValue;
  push(e);
}

#ifndef _XBOX
static std::wstring Utf8ToWstring(const char *utf8) {
  if (!utf8 || !*utf8)
    return std::wstring();
#ifdef _WIN32
  int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
  if (len <= 0)
    return std::wstring();
  std::wstring ws(static_cast<size_t>(len), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &ws[0], len);
  ws.resize(static_cast<size_t>(len - 1));
  return ws;
#else
  (void)utf8;
  return std::wstring();
#endif
}
#endif

void EnqueueGiveItemStack(int itemId, int count, int auxValue,
                          const char *displayNameUtf8,
                          const std::vector<std::string> *loreLines) {
#ifndef _XBOX
  GiveItemStackEntry e;
  e.id = itemId;
  e.count = count <= 0 ? 1 : count;
  e.aux = auxValue;
  if (displayNameUtf8 && *displayNameUtf8)
    e.displayName = displayNameUtf8;
  if (loreLines)
    e.lore = *loreLines;
  std::lock_guard<std::mutex> lock(s_stackMutex);
  s_giveStackQueue.push_back(std::move(e));
#else
  (void)itemId;
  (void)count;
  (void)auxValue;
  (void)displayNameUtf8;
  (void)loreLines;
#endif
}

void EnqueueClearInventory() {
  Entry e{};
  e.kind = KClearInventory;
  push(e);
}

#ifndef _XBOX
// Resolve the ServerPlayer for the current local player. Used to keep server state in sync with mod actions.
static shared_ptr<ServerPlayer> GetServerPlayerForLocal() {
  MinecraftServer *server = MinecraftServer::getInstance();
  if (!server) return nullptr;
  PlayerList *list = server->getPlayers();
  if (!list || list->players.empty()) return nullptr;
  Minecraft *mc = Minecraft::GetInstance();
  if (!mc) return nullptr;
  int idx = mc->getLocalPlayerIdx();
  if (idx < 0 || static_cast<size_t>(idx) >= list->players.size()) idx = 0;
  return list->players[idx];
}

// When a server exists, give only on the server and sync to client via refreshContainer.
// This keeps server as authority so the item doesn't disappear on use.
// Returns true if we gave on the server (client will get it via packet); then caller should not add to client.
static bool GiveItemOnServerOnly(const shared_ptr<ItemInstance> &item) {
  shared_ptr<ServerPlayer> serverPlayer = GetServerPlayerForLocal();
  if (!serverPlayer || !serverPlayer->inventory) return false;
  shared_ptr<ItemInstance> serverCopy = item->copy();
  if (!serverPlayer->inventory->add(serverCopy)) return false;
  serverPlayer->inventory->setChanged();
  serverPlayer->refreshContainer(serverPlayer->inventoryMenu);
  return true;
}
#endif

void ApplyPendingActions(Level *level, Player *player) {
#ifndef _XBOX
  shared_ptr<ServerPlayer> serverPlayer = GetServerPlayerForLocal();
  Level *serverLevel = nullptr;
  if (serverPlayer && MinecraftServer::getInstance())
    serverLevel = MinecraftServer::getInstance()->getLevel(serverPlayer->dimension);
  std::deque<Entry> batch;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    batch.swap(s_queue);
  }
  for (const Entry &e : batch) {
    switch (e.kind) {
    case KSetBlock:
      if (level)
        level->setTileAndData(e.i[0], e.i[1], e.i[2], e.i[3], e.i[4],
                             Tile::UPDATE_ALL);
      if (serverLevel)
        serverLevel->setTileAndData(e.i[0], e.i[1], e.i[2], e.i[3], e.i[4],
                                   Tile::UPDATE_ALL);
      break;
    case KSetHealth:
      if (player) {
        player->setHealth(e.f[0]);
        if (serverPlayer) serverPlayer->setHealth(e.f[0]);
      }
      break;
    case KSetFood:
      if (player && player->getFoodData()) {
        player->getFoodData()->setFoodLevel(e.i[0]);
        if (serverPlayer && serverPlayer->getFoodData())
          serverPlayer->getFoodData()->setFoodLevel(e.i[0]);
      }
      break;
    case KSetSaturation:
      if (player && player->getFoodData()) {
        player->getFoodData()->setSaturation(e.f[0]);
        if (serverPlayer && serverPlayer->getFoodData())
          serverPlayer->getFoodData()->setSaturation(e.f[0]);
      }
      break;
    case KSetPosition:
      if (player) {
        player->setPos(e.d[0], e.d[1], e.d[2]);
        if (serverPlayer) serverPlayer->setPos(e.d[0], e.d[1], e.d[2]);
      }
      break;
    case KSetRotation:
      if (player) {
        player->xRot = e.f[0];
        player->yRot = e.f[1];
        if (serverPlayer) {
          serverPlayer->xRot = e.f[0];
          serverPlayer->yRot = e.f[1];
        }
      }
      break;
    case KSetGameMode:
      if (player) {
        GameType *gt = GameType::byId(e.i[0]);
        if (gt) {
          player->setGameMode(gt);
          if (serverPlayer) serverPlayer->setGameMode(gt);
        }
      }
      break;
    case KSetLevelTime:
      if (level)
        level->setGameTime(e.i64[0]);
      if (serverLevel)
        serverLevel->setGameTime(e.i64[0]);
      break;
    case KSetDayTime:
      if (level)
        level->setDayTime(e.i64[0]);
      if (serverLevel)
        serverLevel->setDayTime(e.i64[0]);
      break;
    case KSetRaining:
      if (level && level->getLevelData())
        level->getLevelData()->setRaining(e.b[0]);
      if (serverLevel && serverLevel->getLevelData())
        serverLevel->getLevelData()->setRaining(e.b[0]);
      break;
    case KSetThundering:
      if (level && level->getLevelData())
        level->getLevelData()->setThundering(e.b[0]);
      if (serverLevel && serverLevel->getLevelData())
        serverLevel->getLevelData()->setThundering(e.b[0]);
      break;
    case KSetDifficulty:
      if (level)
        level->difficulty = e.i[0];
      if (serverLevel)
        serverLevel->difficulty = e.i[0];
      break;
    case KGiveXPLevels:
      if (player) {
        player->giveExperienceLevels(e.i[0]);
        if (serverPlayer) serverPlayer->giveExperienceLevels(e.i[0]);
      }
      break;
    case KGiveItem: {
      int id = e.i[0], count = e.i[1], aux = e.i[2];
      if (player && player->inventory && id >= 0 &&
          static_cast<unsigned>(id) < Item::items.length &&
          Item::items[id] != nullptr) {
        auto item = std::make_shared<ItemInstance>(id, count, aux);
        if (GiveItemOnServerOnly(item))
          ; // client will receive item via container sync
        else if (player->inventory->add(item))
          player->inventory->setChanged();
      }
      break;
    }
    case KClearInventory:
      if (player && player->inventory) {
        player->inventory->clearInventory(-1, -1);
        if (serverPlayer && serverPlayer->inventory) {
          serverPlayer->inventory->clearInventory(-1, -1);
          serverPlayer->inventory->setChanged();
          serverPlayer->refreshContainer(serverPlayer->inventoryMenu);
        }
      }
      break;
    default:
      break;
    }
  }

  // Process give-item-stack queue (name + lore)
  std::deque<GiveItemStackEntry> stackBatch;
  {
    std::lock_guard<std::mutex> lock(s_stackMutex);
    stackBatch.swap(s_giveStackQueue);
  }
  for (const GiveItemStackEntry &se : stackBatch) {
    if (!player || !player->inventory)
      continue;
    int id = se.id, count = se.count, aux = se.aux;
    if (id < 0 || static_cast<unsigned>(id) >= Item::items.length ||
        Item::items[id] == nullptr)
      continue;
    auto item = std::make_shared<ItemInstance>(id, count, aux);
    if (!se.displayName.empty()) {
      std::wstring nameW = Utf8ToWstring(se.displayName.c_str());
      if (!nameW.empty())
        item->setHoverName(nameW);
    }
    if (!se.lore.empty()) {
      if (!item->getTag())
        item->setTag(new CompoundTag());
      CompoundTag *tag = item->getTag();
      if (!tag->contains(L"display"))
        tag->putCompound(L"display", new CompoundTag());
      CompoundTag *display = tag->getCompound(L"display");
      ListTag<StringTag> *loreList = new ListTag<StringTag>(L"Lore");
      for (const std::string &line : se.lore) {
        std::wstring lineW = Utf8ToWstring(line.c_str());
        loreList->add(new StringTag(L"", lineW));
      }
      display->put(L"Lore", loreList);
    }
    if (GiveItemOnServerOnly(item))
      ; // client will receive item via container sync
    else if (player->inventory->add(item))
      player->inventory->setChanged();
  }

  // Refresh inventory cache so mod thread sees current slots when it calls getInventory().
  if (player)
    GameBridge::UpdateLocalPlayerInventoryCache(player);
#endif
}

} // namespace ModActionQueue
