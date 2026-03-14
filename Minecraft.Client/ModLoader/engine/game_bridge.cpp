#include "game_bridge.h"
#include "stdafx.h"

#ifndef _XBOX
#include "../../../Minecraft.World/FoodData.h"
#include "../../../Minecraft.World/Inventory.h"
#include "../../../Minecraft.World/ItemInstance.h"
#include "../../../Minecraft.World/Level.h"
#include "../../../Minecraft.World/LevelData.h"
#include "../../../Minecraft.World/LevelSettings.h"
#include "../../Minecraft.h"
#include "../../MultiPlayerLevel.h"
#include "../../MultiPlayerLocalPlayer.h"
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

bool GetLocalPlayerInventory(InventoryData *out)
{
    if (!out)
    {
        return false;
    }
    out->slots.clear();
    out->armor.clear();
    out->selectedSlot = 0;
#ifndef _XBOX
    Minecraft *mc = Minecraft::GetInstance();
    if (!mc || !mc->player || !mc->player->inventory)
    {
        return false;
    }
    Inventory *inv = mc->player->inventory.get();
    if (!inv || inv->items.length == 0)
    {
        return false;
    }
    // Read main inventory from the items array directly (same as getItem(i))
    const unsigned int mainSlots = inv->items.length <= 36 ? inv->items.length : 36u;
    for (unsigned int i = 0; i < mainSlots; i++)
    {
        ItemSlot slot;
        slot.id = 0;
        slot.count = 0;
        slot.auxValue = 0;
        shared_ptr<ItemInstance> &itemRef = inv->items[i];
        if (itemRef)
        {
            slot.id = itemRef->id;
            slot.count = itemRef->count;
            slot.auxValue = itemRef->getAuxValue();
        }
        out->slots.push_back(slot);
    }
    out->selectedSlot = inv->selected >= 0 && inv->selected < 9 ? inv->selected : 0;
    // Read armor from the armor array directly
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

} // namespace GameBridge
