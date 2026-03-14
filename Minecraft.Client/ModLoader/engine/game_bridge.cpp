#include "game_bridge.h"
#include "stdafx.h"

#ifndef _XBOX
#include "../../../Minecraft.World/Abilities.h"
#include "../../../Minecraft.World/FoodData.h"
#include "../../../Minecraft.World/Inventory.h"
#include "../../../Minecraft.World/ItemInstance.h"
#include "../../../Minecraft.World/Level.h"
#include "../../../Minecraft.World/LevelData.h"
#include "../../../Minecraft.World/LevelSettings.h"
#include "../../Minecraft.h"
#include "../../MinecraftServer.h"
#include "../../MultiPlayerLevel.h"
#include "../../MultiPlayerLocalPlayer.h"
#include "../../PlayerList.h"
#include "../../ServerPlayer.h"
#include "../Player.h"
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif
#endif

namespace GameBridge
{

#ifndef _XBOX
static void WstringToUtf8(const std::wstring &ws, std::string *out)
{
    if (ws.empty())
    {
        out->clear();
        return;
    }
#ifdef _WIN32
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    if (len <= 0)
    {
        out->clear();
        return;
    }
    out->resize(static_cast<size_t>(len));
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &(*out)[0], len, nullptr, nullptr);
#else
    (void)ws;
    out->clear();
#endif
}
#endif

void GetCurrentLevelName(std::string *out)
{
    if (!out)
    {
        return;
    }
    out->clear();
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->level || !mc->level->getLevelData())
    {
        return;
    }
    std::wstring name = mc->level->getLevelData()->getLevelName();
    WstringToUtf8(name, out);
#endif
}

bool GetLocalPlayerPosition(double *x, double *y, double *z)
{
    if (!x || !y || !z)
    {
        return false;
    }
    *x = *y = *z = 0;
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->player)
    {
        return false;
    }
    *x = mc->player->x;
    *y = mc->player->y;
    *z = mc->player->z;
    return true;
#endif
    return false;
}

void GetLocalPlayerName(std::string *out)
{
    if (!out)
    {
        return;
    }
    out->clear();
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->player)
    {
        std::wstring name = mc->player->m_displayName;
        WstringToUtf8(name, out);
    }
#endif
    if (out->empty())
    {
        *out = "Player";
    }
}

float GetLocalPlayerHealth()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->player)
    {
        return mc->player->getHealth();
    }
#endif
    return -1.0f;
}

int GetLocalPlayerGameModeId()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->level || !mc->level->getLevelData())
    {
        return -1;
    }
    GameType *gt = mc->level->getLevelData()->getGameType();
    if (gt)
    {
        return gt->getId();
    }
#endif
    return -1;
}

bool GetLocalPlayerFood(int *foodLevel, float *saturation)
{
    if (!foodLevel || !saturation)
    {
        return false;
    }
    *foodLevel = 0;
    *saturation = 0.0f;
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->player)
    {
        return false;
    }

    FoodData *fd = mc->player->getFoodData();
    if (!fd)
    {
        return false;
    }
    *foodLevel = fd->getFoodLevel();
    *saturation = fd->getSaturationLevel();
    return true;
#endif
    return false;
}

bool GetLocalPlayerExperience(int *level, int *totalXP)
{
    if (!level || !totalXP)
    {
        return false;
    }
    *level = 0;
    *totalXP = 0;
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->player)
    {
        return false;
    }
    *level = mc->player->experienceLevel;
    *totalXP = mc->player->totalExperience;
    return true;
#endif
    return false;
}

#ifndef _XBOX
static std::mutex s_inventoryCacheMutex;
static InventoryData s_cachedInventory;
static bool s_inventoryCacheValid = false;
#endif

// Called from game thread only (e.g. end of ApplyPendingActions).
// Prefer the server player's inventory when in a local game so the cache reflects
// the authoritative state (client inventory can show zeros if not yet synced).
void UpdateLocalPlayerInventoryCache(Player *player)
{
#ifndef _XBOX
    if (!player)
    {
        std::lock_guard<std::mutex> lock(s_inventoryCacheMutex);
        s_inventoryCacheValid = false;
        return;
    }
    Inventory *inv = player->inventory ? player->inventory.get() : nullptr;
    // In local/hosted games the server is authoritative; use its inventory for the cache.
    MinecraftServer *server = MinecraftServer::getInstance();
    if (server)
    {
        PlayerList *list = server->getPlayers();
        if (list && !list->players.empty())
        {
            Minecraft *mc = Minecraft::GetInstance();
            int idx = mc ? mc->getLocalPlayerIdx() : 0;
            if (idx < 0 || static_cast<size_t>(idx) >= list->players.size())
                idx = 0;
            shared_ptr<ServerPlayer> serverPlayer = list->players[idx];
            if (serverPlayer && serverPlayer->inventory)
                inv = serverPlayer->inventory.get();
        }
    }
    if (!inv)
    {
        std::lock_guard<std::mutex> lock(s_inventoryCacheMutex);
        s_inventoryCacheValid = false;
        return;
    }
    InventoryData fresh;
    fresh.slots.clear();
    fresh.armor.clear();
    fresh.selectedSlot = inv->selected >= 0 && inv->selected < 9 ? inv->selected : 0;
    for (unsigned int i = 0; i < 36u; i++)
    {
        ItemSlot slot;
        slot.id = 0;
        slot.count = 0;
        slot.auxValue = 0;
        if (i < inv->items.length && inv->items.data)
        {
            shared_ptr<ItemInstance> &itemRef = inv->items[i];
            if (itemRef)
            {
                slot.id = itemRef->id;
                slot.count = itemRef->count;
                slot.auxValue = itemRef->getAuxValue();
            }
        }
        fresh.slots.push_back(slot);
    }
    const unsigned int armorCount = inv->armor.length <= 4 ? inv->armor.length : 4u;
    for (unsigned int i = 0; i < armorCount; i++)
    {
        ItemSlot slot;
        slot.id = 0;
        slot.count = 0;
        slot.auxValue = 0;
        shared_ptr<ItemInstance> &armorRef = inv->armor[i];
        if (armorRef)
        {
            slot.id = armorRef->id;
            slot.count = armorRef->count;
            slot.auxValue = armorRef->getAuxValue();
        }
        fresh.armor.push_back(slot);
    }
    {
        std::lock_guard<std::mutex> lock(s_inventoryCacheMutex);
        s_cachedInventory = fresh;
        s_inventoryCacheValid = true;
    }
#endif
}

bool GetLocalPlayerInventory(InventoryData *out)
{
    if (!out)
        return false;
    out->slots.clear();
    out->armor.clear();
    out->selectedSlot = 0;
#ifndef _XBOX
    {
        std::lock_guard<std::mutex> lock(s_inventoryCacheMutex);
        if (s_inventoryCacheValid)
        {
            *out = s_cachedInventory;
            return true;
        }
    }
    // Fallback: cache not populated yet (e.g. before first tick) — read directly.
    // Prefer same player resolution as cache path so we see the active local player.
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc)
        return false;
    shared_ptr<MultiplayerLocalPlayer> targetPlayer = mc->player;
    int idx = mc->getLocalPlayerIdx();
    if (idx >= 0 && idx < static_cast<int>(XUSER_MAX_COUNT) && mc->localplayers[idx])
        targetPlayer = mc->localplayers[idx];
    if (!targetPlayer || !targetPlayer->inventory)
        return false;
    Inventory *inv = targetPlayer->inventory.get();
    if (!inv)
        return false;
    for (unsigned int i = 0; i < 36u; i++)
    {
        ItemSlot slot;
        slot.id = 0;
        slot.count = 0;
        slot.auxValue = 0;
        if (i < inv->items.length && inv->items.data)
        {
            shared_ptr<ItemInstance> &itemRef = inv->items[i];
            if (itemRef)
            {
                slot.id = itemRef->id;
                slot.count = itemRef->count;
                slot.auxValue = itemRef->getAuxValue();
            }
        }
        out->slots.push_back(slot);
    }
    out->selectedSlot = inv->selected >= 0 && inv->selected < 9 ? inv->selected : 0;
    const unsigned int armorCount = inv->armor.length <= 4 ? inv->armor.length : 4u;
    for (unsigned int i = 0; i < armorCount; i++)
    {
        ItemSlot slot;
        slot.id = 0;
        slot.count = 0;
        slot.auxValue = 0;
        shared_ptr<ItemInstance> &armorRef = inv->armor[i];
        if (armorRef)
        {
            slot.id = armorRef->id;
            slot.count = armorRef->count;
            slot.auxValue = armorRef->getAuxValue();
        }
        out->armor.push_back(slot);
    }
    return true;
#endif
    return false;
}

// --- Player state ---
bool GetLocalPlayerSneaking()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->player)
        return mc->player->isSneaking();
#endif
    return false;
}

bool GetLocalPlayerSprinting()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->player)
        return mc->player->isSprinting();
#endif
    return false;
}

bool GetLocalPlayerSelectedItem(ItemSlot *out)
{
    if (!out)
        return false;
    out->id = 0;
    out->count = 0;
    out->auxValue = 0;
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->player)
        return false;
    shared_ptr<ItemInstance> sel = mc->player->getSelectedItem();
    if (!sel)
        return true;
    out->id = sel->id;
    out->count = sel->count;
    out->auxValue = sel->getAuxValue();
#endif
    return true;
}

int GetLocalPlayerDimension()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->player)
        return mc->player->dimension;
#endif
    return 0;
}

void GetLocalPlayerRotation(float *pitchDeg, float *yawDeg)
{
    if (pitchDeg)
        *pitchDeg = 0.0f;
    if (yawDeg)
        *yawDeg = 0.0f;
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->player)
        return;
    if (pitchDeg)
        *pitchDeg = mc->player->xRot;
    if (yawDeg)
        *yawDeg = mc->player->yRot;
#endif
}

bool GetLocalPlayerIsInWater()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->player)
        return mc->player->isInWater();
#endif
    return false;
}

bool GetLocalPlayerOnFire()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->player)
        return mc->player->isOnFire();
#endif
    return false;
}

bool GetLocalPlayerUsingItem()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->player)
        return mc->player->isUsingItem();
#endif
    return false;
}

void GetLocalPlayerAbilities(bool *flying, bool *mayfly, bool *invulnerable)
{
    if (flying)
        *flying = false;
    if (mayfly)
        *mayfly = false;
    if (invulnerable)
        *invulnerable = false;
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->player)
        return;
    Abilities &a = mc->player->abilities;
    if (flying)
        *flying = a.flying;
    if (mayfly)
        *mayfly = a.mayfly;
    if (invulnerable)
        *invulnerable = a.invulnerable;
#endif
}

// --- World/level ---
bool GetBlockAt(int x, int y, int z, int *tileId, int *data)
{
    if (!tileId || !data)
        return false;
    *tileId = 0;
    *data = 0;
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->level)
        return false;
    Level *level = mc->level;
    *tileId = level->getTile(x, y, z);
    *data = level->getData(x, y, z);
#endif
    return true;
}

int64_t GetLevelTime()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->level && mc->level->getLevelData())
        return mc->level->getLevelData()->getGameTime();
#endif
    return 0;
}

int64_t GetDayTime()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->level && mc->level->getLevelData())
        return mc->level->getLevelData()->getDayTime();
#endif
    return 0;
}

bool IsRaining()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->level && mc->level->getLevelData())
        return mc->level->getLevelData()->isRaining();
#endif
    return false;
}

bool IsThundering()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->level && mc->level->getLevelData())
        return mc->level->getLevelData()->isThundering();
#endif
    return false;
}

int GetDifficulty()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->level)
        return mc->level->difficulty;
#endif
    return 0;
}

int64_t GetLevelSeed()
{
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (mc && mc->level && mc->level->getLevelData())
        return mc->level->getLevelData()->getSeed();
#endif
    return 0;
}

} // namespace GameBridge
