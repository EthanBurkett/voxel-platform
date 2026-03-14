#include "engine/module_registry.h"
#include "engine/game_bridge.h"
#include "engine/mod_objects.h"
#include <node_api.h>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// getWorld() / getLocation() — callbacks attached to the Player instance.
// ---------------------------------------------------------------------------
static napi_value GetWorld(napi_env env, napi_callback_info info) {
  std::string name;
  GameBridge::GetCurrentLevelName(&name);
  return ModObjects::CreateWorld(env, name.empty() ? "World" : name.c_str());
}

static napi_value GetLocation(napi_env env, napi_callback_info info) {
  double x = 0, y = 0, z = 0;
  GameBridge::GetLocalPlayerPosition(&x, &y, &z);
  return ModObjects::CreateLocation(env, x, y, z);
}

static napi_value GetHealth(napi_env env, napi_callback_info info) {
  float h = GameBridge::GetLocalPlayerHealth();
  napi_value result = nullptr;
  napi_create_double(env, (double)h, &result);
  return result;
}

static napi_value GetGameMode(napi_env env, napi_callback_info info) {
  int id = GameBridge::GetLocalPlayerGameModeId();
  const char *name = "unknown";
  if (id == 0) name = "survival";
  else if (id == 1) name = "creative";
  else if (id == 2) name = "adventure";
  napi_value result = nullptr;
  napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &result);
  return result;
}

static napi_value GetFoodLevel(napi_env env, napi_callback_info info) {
  int food = 0;
  float sat = 0.0f;
  GameBridge::GetLocalPlayerFood(&food, &sat);
  napi_value result = nullptr;
  napi_create_int32(env, food, &result);
  return result;
}

static napi_value GetSaturation(napi_env env, napi_callback_info info) {
  int food = 0;
  float sat = 0.0f;
  GameBridge::GetLocalPlayerFood(&food, &sat);
  napi_value result = nullptr;
  napi_create_double(env, (double)sat, &result);
  return result;
}

static napi_value GetExperienceLevel(napi_env env, napi_callback_info info) {
  int level = 0, total = 0;
  GameBridge::GetLocalPlayerExperience(&level, &total);
  napi_value result = nullptr;
  napi_create_int32(env, level, &result);
  return result;
}

static napi_value GetTotalExperience(napi_env env, napi_callback_info info) {
  int level = 0, total = 0;
  GameBridge::GetLocalPlayerExperience(&level, &total);
  napi_value result = nullptr;
  napi_create_int32(env, total, &result);
  return result;
}

static napi_value GetInventory(napi_env env, napi_callback_info info) {
  GameBridge::InventoryData data;
  if (!GameBridge::GetLocalPlayerInventory(&data)) {
    napi_value empty = nullptr;
    std::vector<ModObjects::ItemSlot> emptySlots, emptyArmor;
    return ModObjects::CreateInventory(env, emptySlots, 0, emptyArmor);
  }
  std::vector<ModObjects::ItemSlot> slots, armor;
  for (const auto &s : data.slots) {
    ModObjects::ItemSlot m;
    m.id = s.id;
    m.count = s.count;
    m.auxValue = s.auxValue;
    slots.push_back(m);
  }
  for (const auto &a : data.armor) {
    ModObjects::ItemSlot m;
    m.id = a.id;
    m.count = a.count;
    m.auxValue = a.auxValue;
    armor.push_back(m);
  }
  return ModObjects::CreateInventory(env, slots, data.selectedSlot, armor);
}

// ---------------------------------------------------------------------------
// player.get() — returns the local player (Player instance with name, id,
// getWorld(), getLocation()).
// ---------------------------------------------------------------------------
static napi_value Get(napi_env env, napi_callback_info info) {
  napi_value result = nullptr;
  if (napi_create_object(env, &result) != napi_ok)
    return nullptr;

  std::string displayName;
  GameBridge::GetLocalPlayerName(&displayName);
  if (displayName.empty())
    displayName = "Player";

  napi_value nameVal = nullptr;
  napi_create_string_utf8(env, displayName.c_str(), displayName.size(), &nameVal);
  napi_set_named_property(env, result, "name", nameVal);

  napi_value idVal = nullptr;
  napi_create_int32(env, 0, &idVal);
  napi_set_named_property(env, result, "id", idVal);

  napi_value getWorldFn = nullptr;
  napi_create_function(env, "getWorld", NAPI_AUTO_LENGTH, GetWorld, nullptr, &getWorldFn);
  napi_set_named_property(env, result, "getWorld", getWorldFn);

  napi_value getLocationFn = nullptr;
  napi_create_function(env, "getLocation", NAPI_AUTO_LENGTH, GetLocation, nullptr, &getLocationFn);
  napi_set_named_property(env, result, "getLocation", getLocationFn);

  napi_value getHealthFn = nullptr;
  napi_create_function(env, "getHealth", NAPI_AUTO_LENGTH, GetHealth, nullptr, &getHealthFn);
  napi_set_named_property(env, result, "getHealth", getHealthFn);

  napi_value getGameModeFn = nullptr;
  napi_create_function(env, "getGameMode", NAPI_AUTO_LENGTH, GetGameMode, nullptr, &getGameModeFn);
  napi_set_named_property(env, result, "getGameMode", getGameModeFn);

  napi_value getFoodLevelFn = nullptr;
  napi_create_function(env, "getFoodLevel", NAPI_AUTO_LENGTH, GetFoodLevel, nullptr, &getFoodLevelFn);
  napi_set_named_property(env, result, "getFoodLevel", getFoodLevelFn);

  napi_value getSaturationFn = nullptr;
  napi_create_function(env, "getSaturation", NAPI_AUTO_LENGTH, GetSaturation, nullptr, &getSaturationFn);
  napi_set_named_property(env, result, "getSaturation", getSaturationFn);

  napi_value getExperienceLevelFn = nullptr;
  napi_create_function(env, "getExperienceLevel", NAPI_AUTO_LENGTH, GetExperienceLevel, nullptr, &getExperienceLevelFn);
  napi_set_named_property(env, result, "getExperienceLevel", getExperienceLevelFn);

  napi_value getTotalExperienceFn = nullptr;
  napi_create_function(env, "getTotalExperience", NAPI_AUTO_LENGTH, GetTotalExperience, nullptr, &getTotalExperienceFn);
  napi_set_named_property(env, result, "getTotalExperience", getTotalExperienceFn);

  napi_value getInventoryFn = nullptr;
  napi_create_function(env, "getInventory", NAPI_AUTO_LENGTH, GetInventory, nullptr, &getInventoryFn);
  napi_set_named_property(env, result, "getInventory", getInventoryFn);

  return result;
}

// ---------------------------------------------------------------------------
// player.fromUuid(uuid) — get a player by UUID (placeholder for now).
// ---------------------------------------------------------------------------
static napi_value FromUuid(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;

  size_t len = 0;
  napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len);
  char *uuid = new char[len + 1];
  napi_get_value_string_utf8(env, argv[0], uuid, len + 1, &len);
  uuid[len] = '\0';

  napi_value result = nullptr;
  napi_create_object(env, &result);
  napi_value name = nullptr;
  napi_create_string_utf8(env, "Player", NAPI_AUTO_LENGTH, &name);
  napi_set_named_property(env, result, "name", name);
  napi_value uuidVal = nullptr;
  napi_create_string_utf8(env, uuid, len, &uuidVal);
  napi_set_named_property(env, result, "uuid", uuidVal);

  delete[] uuid;
  return result;
}

static struct PlayerModuleRegistrar {
  PlayerModuleRegistrar() {
    ModuleRegistry::Register(
        {"player",
         {
             ModuleFunction("get", Get, {}, "Player"),
             ModuleFunction("fromUuid", FromUuid, {{"uuid", "string"}},
                            "{ name: string; uuid: string }"),
         }});
  }
} s_player_module_registrar;
