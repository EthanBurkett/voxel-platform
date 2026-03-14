#include "stdafx.h"
#include "mod_objects.h"
#include "game_bridge.h"
#include <node_api.h>
#include <cmath>
#include <cstring>

namespace ModObjects {

static napi_value DistanceTo(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_value thisVal = nullptr;
  napi_get_cb_info(env, info, &argc, argv, &thisVal, nullptr);
  if (argc < 1)
    return nullptr;

  auto getNum = [env](napi_value obj, const char *key, double *out) -> bool {
    napi_value v = nullptr;
    if (napi_get_named_property(env, obj, key, &v) != napi_ok) return false;
    if (napi_get_value_double(env, v, out) != napi_ok) return false;
    return true;
  };

  double x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0;
  if (!getNum(thisVal, "x", &x1) || !getNum(thisVal, "y", &y1) || !getNum(thisVal, "z", &z1))
    return nullptr;
  if (!getNum(argv[0], "x", &x2) || !getNum(argv[0], "y", &y2) || !getNum(argv[0], "z", &z2))
    return nullptr;

  double dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
  double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

  napi_value result = nullptr;
  napi_create_double(env, dist, &result);
  return result;
}

napi_value CreateLocation(napi_env env, double x, double y, double z) {
  napi_value obj = nullptr;
  if (napi_create_object(env, &obj) != napi_ok)
    return nullptr;

  napi_value vx = nullptr, vy = nullptr, vz = nullptr;
  napi_create_double(env, x, &vx);
  napi_create_double(env, y, &vy);
  napi_create_double(env, z, &vz);
  napi_set_named_property(env, obj, "x", vx);
  napi_set_named_property(env, obj, "y", vy);
  napi_set_named_property(env, obj, "z", vz);

  napi_value fn = nullptr;
  napi_create_function(env, "distanceTo", NAPI_AUTO_LENGTH, DistanceTo, nullptr, &fn);
  napi_set_named_property(env, obj, "distanceTo", fn);

  return obj;
}

static napi_value WorldGetName(napi_env env, napi_callback_info info) {
  napi_value thisVal = nullptr;
  napi_get_cb_info(env, info, nullptr, nullptr, &thisVal, nullptr);
  napi_value nameProp = nullptr;
  if (!thisVal || napi_get_named_property(env, thisVal, "name", &nameProp) != napi_ok)
    return nullptr;
  return nameProp;
}

static napi_value WorldGetDimension(napi_env env, napi_callback_info info) {
  (void)info;
  int d = GameBridge::GetLocalPlayerDimension();
  napi_value v = nullptr;
  napi_create_int32(env, d, &v);
  return v;
}

napi_value CreateWorld(napi_env env, const char *nameUtf8) {
  napi_value obj = nullptr;
  if (napi_create_object(env, &obj) != napi_ok)
    return nullptr;
  napi_value nameVal = nullptr;
  size_t len = nameUtf8 ? strlen(nameUtf8) : 0;
  napi_create_string_utf8(env, nameUtf8 ? nameUtf8 : "", len, &nameVal);
  napi_set_named_property(env, obj, "name", nameVal);

  napi_value getNameFn = nullptr, getDimFn = nullptr;
  napi_create_function(env, "getName", NAPI_AUTO_LENGTH, WorldGetName, nullptr, &getNameFn);
  napi_create_function(env, "getDimension", NAPI_AUTO_LENGTH, WorldGetDimension, nullptr, &getDimFn);
  napi_set_named_property(env, obj, "getName", getNameFn);
  napi_set_named_property(env, obj, "getDimension", getDimFn);
  return obj;
}

napi_value CreateItemStack(napi_env env, int id, int count, int auxValue) {
  napi_value obj = nullptr;
  if (napi_create_object(env, &obj) != napi_ok)
    return nullptr;
  napi_value vId = nullptr, vCount = nullptr, vAux = nullptr;
  napi_create_int32(env, id, &vId);
  napi_create_int32(env, count, &vCount);
  napi_create_int32(env, auxValue, &vAux);
  napi_set_named_property(env, obj, "id", vId);
  napi_set_named_property(env, obj, "count", vCount);
  napi_set_named_property(env, obj, "auxValue", vAux);
  return obj;
}

napi_value CreateInventory(napi_env env, const std::vector<ItemSlot> &slots,
                          int selectedSlot, const std::vector<ItemSlot> &armor) {
  napi_value obj = nullptr;
  if (napi_create_object(env, &obj) != napi_ok)
    return nullptr;

  napi_value slotsArr = nullptr;
  if (napi_create_array_with_length(env, (size_t)slots.size(), &slotsArr) != napi_ok)
    return obj;
  for (size_t i = 0; i < slots.size(); i++) {
    napi_value item = CreateItemStack(env, slots[i].id, slots[i].count, slots[i].auxValue);
    napi_set_element(env, slotsArr, (uint32_t)i, item);
  }
  napi_set_named_property(env, obj, "slots", slotsArr);

  napi_value selectedVal = nullptr;
  napi_create_int32(env, selectedSlot, &selectedVal);
  napi_set_named_property(env, obj, "selectedSlot", selectedVal);

  napi_value armorArr = nullptr;
  if (napi_create_array_with_length(env, (size_t)armor.size(), &armorArr) != napi_ok)
    return obj;
  for (size_t i = 0; i < armor.size(); i++) {
    napi_value item = CreateItemStack(env, armor[i].id, armor[i].count, armor[i].auxValue);
    napi_set_element(env, armorArr, (uint32_t)i, item);
  }
  napi_set_named_property(env, obj, "armor", armorArr);

  return obj;
}

}  // namespace ModObjects
