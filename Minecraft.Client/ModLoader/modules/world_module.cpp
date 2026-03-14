#include "engine/module_registry.h"
#include "engine/action_queue.h"
#include "engine/game_bridge.h"
#include "engine/mod_objects.h"
#include <node_api.h>
#include <string>

// ---------------------------------------------------------------------------
// world.getBlock(x, y, z) -> { id, data } | null
// ---------------------------------------------------------------------------
static napi_value GetBlock(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv[3];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 3)
    return nullptr;

  int32_t x = 0, y = 0, z = 0;
  napi_get_value_int32(env, argv[0], &x);
  napi_get_value_int32(env, argv[1], &y);
  napi_get_value_int32(env, argv[2], &z);

  int tileId = 0, data = 0;
  if (!GameBridge::GetBlockAt(x, y, z, &tileId, &data)) {
    return nullptr;
  }

  napi_value result = nullptr;
  if (napi_create_object(env, &result) != napi_ok)
    return nullptr;
  napi_value vId = nullptr, vData = nullptr;
  napi_create_int32(env, tileId, &vId);
  napi_create_int32(env, data, &vData);
  napi_set_named_property(env, result, "id", vId);
  napi_set_named_property(env, result, "data", vData);
  return result;
}

// ---------------------------------------------------------------------------
// world.getTime() -> number (game time)
// ---------------------------------------------------------------------------
static napi_value GetTime(napi_env env, napi_callback_info info) {
  int64_t t = GameBridge::GetLevelTime();
  napi_value result = nullptr;
  napi_create_double(env, static_cast<double>(t), &result);
  return result;
}

// ---------------------------------------------------------------------------
// world.getDayTime() -> number (day time)
// ---------------------------------------------------------------------------
static napi_value GetDayTime(napi_env env, napi_callback_info info) {
  int64_t t = GameBridge::GetDayTime();
  napi_value result = nullptr;
  napi_create_double(env, static_cast<double>(t), &result);
  return result;
}

static napi_value GetIsRaining(napi_env env, napi_callback_info info) {
  bool v = GameBridge::IsRaining();
  napi_value result = nullptr;
  napi_get_boolean(env, v, &result);
  return result;
}

static napi_value GetIsThundering(napi_env env, napi_callback_info info) {
  bool v = GameBridge::IsThundering();
  napi_value result = nullptr;
  napi_get_boolean(env, v, &result);
  return result;
}

static napi_value GetDifficulty(napi_env env, napi_callback_info info) {
  int d = GameBridge::GetDifficulty();
  napi_value result = nullptr;
  napi_create_int32(env, d, &result);
  return result;
}

static napi_value GetSeed(napi_env env, napi_callback_info info) {
  int64_t s = GameBridge::GetLevelSeed();
  napi_value result = nullptr;
  napi_create_double(env, static_cast<double>(s), &result);
  return result;
}

// ---------------------------------------------------------------------------
// world.getName() -> string (current level name; same as player.getWorld().name)
// ---------------------------------------------------------------------------
static napi_value GetName(napi_env env, napi_callback_info info) {
  std::string name;
  GameBridge::GetCurrentLevelName(&name);
  if (name.empty())
    name = "World";
  napi_value result = nullptr;
  napi_create_string_utf8(env, name.c_str(), name.size(), &result);
  return result;
}

// ---------------------------------------------------------------------------
// Setters — enqueue; applied next tick on main thread
// ---------------------------------------------------------------------------
static napi_value WorldSetBlock(napi_env env, napi_callback_info info) {
  size_t argc = 5;
  napi_value argv[5];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 4)
    return nullptr;
  int32_t x = 0, y = 0, z = 0, id = 0, data = 0;
  napi_get_value_int32(env, argv[0], &x);
  napi_get_value_int32(env, argv[1], &y);
  napi_get_value_int32(env, argv[2], &z);
  napi_get_value_int32(env, argv[3], &id);
  if (argc >= 5)
    napi_get_value_int32(env, argv[4], &data);
  ModActionQueue::EnqueueSetBlock(x, y, z, id, data);
  napi_value u;
  napi_get_undefined(env, &u);
  return u;
}

static napi_value WorldSetTime(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;
  double t = 0;
  napi_get_value_double(env, argv[0], &t);
  ModActionQueue::EnqueueSetLevelTime(static_cast<int64_t>(t));
  napi_value u;
  napi_get_undefined(env, &u);
  return u;
}

static napi_value WorldSetDayTime(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;
  double t = 0;
  napi_get_value_double(env, argv[0], &t);
  ModActionQueue::EnqueueSetDayTime(static_cast<int64_t>(t));
  napi_value u;
  napi_get_undefined(env, &u);
  return u;
}

static napi_value WorldSetRaining(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;
  bool v = false;
  napi_get_value_bool(env, argv[0], &v);
  ModActionQueue::EnqueueSetRaining(v);
  napi_value u;
  napi_get_undefined(env, &u);
  return u;
}

static napi_value WorldSetThundering(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;
  bool v = false;
  napi_get_value_bool(env, argv[0], &v);
  ModActionQueue::EnqueueSetThundering(v);
  napi_value u;
  napi_get_undefined(env, &u);
  return u;
}

static napi_value WorldSetDifficulty(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;
  int32_t d = 0;
  napi_get_value_int32(env, argv[0], &d);
  ModActionQueue::EnqueueSetDifficulty(d);
  napi_value u;
  napi_get_undefined(env, &u);
  return u;
}

static struct WorldModuleRegistrar {
  WorldModuleRegistrar() {
    ModuleRegistry::Register(
        {"world",
         {
             ModuleFunction("getBlock", GetBlock,
                           {{"x", "number"}, {"y", "number"}, {"z", "number"}},
                           "{ id: number; data: number } | null"),
             ModuleFunction("getTime", GetTime, {}, "number"),
             ModuleFunction("getDayTime", GetDayTime, {}, "number"),
             ModuleFunction("isRaining", GetIsRaining, {}, "boolean"),
             ModuleFunction("isThundering", GetIsThundering, {}, "boolean"),
             ModuleFunction("getDifficulty", GetDifficulty, {}, "number"),
             ModuleFunction("getSeed", GetSeed, {}, "number"),
             ModuleFunction("getName", GetName, {}, "string"),
             ModuleFunction("setBlock", WorldSetBlock,
                            {{"x", "number"}, {"y", "number"}, {"z", "number"}, {"id", "number"}, {"data", "number"}},
                            "void"),
             ModuleFunction("setTime", WorldSetTime, {{"ticks", "number"}}, "void"),
             ModuleFunction("setDayTime", WorldSetDayTime, {{"ticks", "number"}}, "void"),
             ModuleFunction("setRaining", WorldSetRaining, {{"on", "boolean"}}, "void"),
             ModuleFunction("setThundering", WorldSetThundering, {{"on", "boolean"}}, "void"),
             ModuleFunction("setDifficulty", WorldSetDifficulty, {{"d", "number"}}, "void"),
         },
         "World/level API — getters are instant; setters apply next game tick."});
  }
} s_world_module_registrar;
