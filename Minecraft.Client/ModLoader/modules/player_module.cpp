#include "engine/module_registry.h"
#include "engine/action_queue.h"
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

static napi_value GetIsSneaking(napi_env env, napi_callback_info info) {
  bool v = GameBridge::GetLocalPlayerSneaking();
  napi_value result = nullptr;
  napi_get_boolean(env, v, &result);
  return result;
}

static napi_value GetIsSprinting(napi_env env, napi_callback_info info) {
  bool v = GameBridge::GetLocalPlayerSprinting();
  napi_value result = nullptr;
  napi_get_boolean(env, v, &result);
  return result;
}

static napi_value GetSelectedItem(napi_env env, napi_callback_info info) {
  GameBridge::ItemSlot slot;
  GameBridge::GetLocalPlayerSelectedItem(&slot);
  if (slot.id == 0 && slot.count == 0)
    return nullptr;
  return ModObjects::CreateItemStack(env, slot.id, slot.count, slot.auxValue);
}

static napi_value GetDimension(napi_env env, napi_callback_info info) {
  int d = GameBridge::GetLocalPlayerDimension();
  napi_value result = nullptr;
  napi_create_int32(env, d, &result);
  return result;
}

static napi_value GetPitch(napi_env env, napi_callback_info info) {
  float pitch = 0.0f;
  GameBridge::GetLocalPlayerRotation(&pitch, nullptr);
  napi_value result = nullptr;
  napi_create_double(env, (double)pitch, &result);
  return result;
}

static napi_value GetYaw(napi_env env, napi_callback_info info) {
  float yaw = 0.0f;
  GameBridge::GetLocalPlayerRotation(nullptr, &yaw);
  napi_value result = nullptr;
  napi_create_double(env, (double)yaw, &result);
  return result;
}

static napi_value GetIsInWater(napi_env env, napi_callback_info info) {
  bool v = GameBridge::GetLocalPlayerIsInWater();
  napi_value result = nullptr;
  napi_get_boolean(env, v, &result);
  return result;
}

static napi_value GetIsOnFire(napi_env env, napi_callback_info info) {
  bool v = GameBridge::GetLocalPlayerOnFire();
  napi_value result = nullptr;
  napi_get_boolean(env, v, &result);
  return result;
}

static napi_value GetIsUsingItem(napi_env env, napi_callback_info info) {
  bool v = GameBridge::GetLocalPlayerUsingItem();
  napi_value result = nullptr;
  napi_get_boolean(env, v, &result);
  return result;
}

static napi_value GetIsFlying(napi_env env, napi_callback_info info) {
  bool flying = false;
  GameBridge::GetLocalPlayerAbilities(&flying, nullptr, nullptr);
  napi_value result = nullptr;
  napi_get_boolean(env, flying, &result);
  return result;
}

static napi_value GetCanFly(napi_env env, napi_callback_info info) {
  bool mayfly = false;
  GameBridge::GetLocalPlayerAbilities(nullptr, &mayfly, nullptr);
  napi_value result = nullptr;
  napi_get_boolean(env, mayfly, &result);
  return result;
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

  napi_value isSneakingFn = nullptr;
  napi_create_function(env, "isSneaking", NAPI_AUTO_LENGTH, GetIsSneaking, nullptr, &isSneakingFn);
  napi_set_named_property(env, result, "isSneaking", isSneakingFn);

  napi_value isSprintingFn = nullptr;
  napi_create_function(env, "isSprinting", NAPI_AUTO_LENGTH, GetIsSprinting, nullptr, &isSprintingFn);
  napi_set_named_property(env, result, "isSprinting", isSprintingFn);

  napi_value getSelectedItemFn = nullptr;
  napi_create_function(env, "getSelectedItem", NAPI_AUTO_LENGTH, GetSelectedItem, nullptr, &getSelectedItemFn);
  napi_set_named_property(env, result, "getSelectedItem", getSelectedItemFn);

  napi_value getDimensionFn = nullptr;
  napi_create_function(env, "getDimension", NAPI_AUTO_LENGTH, GetDimension, nullptr, &getDimensionFn);
  napi_set_named_property(env, result, "getDimension", getDimensionFn);

  napi_value getPitchFn = nullptr;
  napi_create_function(env, "getPitch", NAPI_AUTO_LENGTH, GetPitch, nullptr, &getPitchFn);
  napi_set_named_property(env, result, "getPitch", getPitchFn);

  napi_value getYawFn = nullptr;
  napi_create_function(env, "getYaw", NAPI_AUTO_LENGTH, GetYaw, nullptr, &getYawFn);
  napi_set_named_property(env, result, "getYaw", getYawFn);

  napi_value isInWaterFn = nullptr;
  napi_create_function(env, "isInWater", NAPI_AUTO_LENGTH, GetIsInWater, nullptr, &isInWaterFn);
  napi_set_named_property(env, result, "isInWater", isInWaterFn);

  napi_value isOnFireFn = nullptr;
  napi_create_function(env, "isOnFire", NAPI_AUTO_LENGTH, GetIsOnFire, nullptr, &isOnFireFn);
  napi_set_named_property(env, result, "isOnFire", isOnFireFn);

  napi_value isUsingItemFn = nullptr;
  napi_create_function(env, "isUsingItem", NAPI_AUTO_LENGTH, GetIsUsingItem, nullptr, &isUsingItemFn);
  napi_set_named_property(env, result, "isUsingItem", isUsingItemFn);

  napi_value isFlyingFn = nullptr;
  napi_create_function(env, "isFlying", NAPI_AUTO_LENGTH, GetIsFlying, nullptr, &isFlyingFn);
  napi_set_named_property(env, result, "isFlying", isFlyingFn);

  napi_value canFlyFn = nullptr;
  napi_create_function(env, "canFly", NAPI_AUTO_LENGTH, GetCanFly, nullptr, &canFlyFn);
  napi_set_named_property(env, result, "canFly", canFlyFn);

  napi_value fn = nullptr;
  napi_create_function(env, "setHealth", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 1; napi_value av[1]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    if (ac < 1) return nullptr; double v = 0; napi_get_value_double(e, av[0], &v);
    ModActionQueue::EnqueueSetPlayerHealth(static_cast<float>(v));
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "setHealth", fn);

  napi_create_function(env, "setFoodLevel", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 1; napi_value av[1]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    if (ac < 1) return nullptr; int32_t v = 0; napi_get_value_int32(e, av[0], &v);
    ModActionQueue::EnqueueSetFoodLevel(v);
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "setFoodLevel", fn);

  napi_create_function(env, "setSaturation", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 1; napi_value av[1]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    if (ac < 1) return nullptr; double v = 0; napi_get_value_double(e, av[0], &v);
    ModActionQueue::EnqueueSetSaturation(static_cast<float>(v));
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "setSaturation", fn);

  napi_create_function(env, "setPosition", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 3; napi_value av[3]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    if (ac < 3) return nullptr;
    double x = 0, y = 0, z = 0;
    napi_get_value_double(e, av[0], &x); napi_get_value_double(e, av[1], &y); napi_get_value_double(e, av[2], &z);
    ModActionQueue::EnqueueSetPosition(x, y, z);
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "setPosition", fn);

  napi_create_function(env, "setRotation", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 2; napi_value av[2]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    if (ac < 2) return nullptr;
    double p = 0, yv = 0; napi_get_value_double(e, av[0], &p); napi_get_value_double(e, av[1], &yv);
    ModActionQueue::EnqueueSetRotation(static_cast<float>(p), static_cast<float>(yv));
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "setRotation", fn);

  napi_create_function(env, "setGameMode", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 1; napi_value av[1]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    if (ac < 1) return nullptr; int32_t id = 0; napi_get_value_int32(e, av[0], &id);
    ModActionQueue::EnqueueSetGameMode(id);
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "setGameMode", fn);

  napi_create_function(env, "addExperienceLevels", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 1; napi_value av[1]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    if (ac < 1) return nullptr; int32_t v = 0; napi_get_value_int32(e, av[0], &v);
    ModActionQueue::EnqueueGiveExperienceLevels(v);
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "addExperienceLevels", fn);

  napi_create_function(env, "giveItem", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 3; napi_value av[3]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    int id = 0, count = 1, aux = 0;
    if (ac >= 1) napi_get_value_int32(e, av[0], &id);
    if (ac >= 2) napi_get_value_int32(e, av[1], &count);
    if (ac >= 3) napi_get_value_int32(e, av[2], &aux);
    ModActionQueue::EnqueueGiveItem(id, count, aux);
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "giveItem", fn);

  napi_create_function(env, "giveItemStack", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    size_t ac = 1; napi_value av[1]; napi_get_cb_info(e, i, &ac, av, nullptr, nullptr);
    if (ac < 1) { napi_value u; napi_get_undefined(e, &u); return u; }
    napi_value obj = av[0];
    napi_valuetype vt; if (napi_typeof(e, obj, &vt) != napi_ok || vt != napi_object) {
      napi_value u; napi_get_undefined(e, &u); return u;
    }
    int32_t id = 0, count = 1, aux = 0;
    napi_value v; if (napi_get_named_property(e, obj, "id", &v) == napi_ok) napi_get_value_int32(e, v, &id);
    if (napi_get_named_property(e, obj, "count", &v) == napi_ok) napi_get_value_int32(e, v, &count);
    if (napi_get_named_property(e, obj, "auxValue", &v) == napi_ok) napi_get_value_int32(e, v, &aux);
    std::string displayName;
    if (napi_get_named_property(e, obj, "name", &v) == napi_ok) {
      napi_valuetype t; if (napi_typeof(e, v, &t) == napi_ok && t == napi_string) {
        size_t len = 0; napi_get_value_string_utf8(e, v, nullptr, 0, &len);
        displayName.resize(len + 1); napi_get_value_string_utf8(e, v, &displayName[0], len + 1, &len);
        displayName.resize(len);
      }
    }
    std::vector<std::string> lore;
    if (napi_get_named_property(e, obj, "lore", &v) == napi_ok) {
      bool isArr = false; napi_is_array(e, v, &isArr);
      if (isArr) {
        uint32_t n = 0; napi_get_array_length(e, v, &n);
        for (uint32_t k = 0; k < n; k++) {
          napi_value el; napi_get_element(e, v, k, &el);
          size_t len = 0; napi_get_value_string_utf8(e, el, nullptr, 0, &len);
          std::string s(len + 1, '\0'); napi_get_value_string_utf8(e, el, &s[0], len + 1, &len);
          s.resize(len); lore.push_back(std::move(s));
        }
      }
    }
    const std::vector<std::string> *lorePtr = lore.empty() ? nullptr : &lore;
    const char *namePtr = displayName.empty() ? nullptr : displayName.c_str();
    ModActionQueue::EnqueueGiveItemStack(id, count, aux, namePtr, lorePtr);
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "giveItemStack", fn);

  napi_create_function(env, "clearInventory", NAPI_AUTO_LENGTH, +[](napi_env e, napi_callback_info i) -> napi_value {
    (void)e; (void)i; ModActionQueue::EnqueueClearInventory();
    napi_value u; napi_get_undefined(e, &u); return u;
  }, nullptr, &fn); napi_set_named_property(env, result, "clearInventory", fn);

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
