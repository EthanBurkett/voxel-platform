#include "stdafx.h"
#include "engine/module_registry.h"
#include "engine/item_registry.h"
#include <node_api.h>
#include <string>

static napi_value GetItemId(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1) {
    napi_value result = nullptr;
    napi_create_int32(env, -1, &result);
    return result;
  }
  napi_valuetype vt = napi_undefined;
  if (napi_typeof(env, argv[0], &vt) != napi_ok || vt != napi_string) {
    napi_value result = nullptr;
    napi_create_int32(env, -1, &result);
    return result;
  }
  size_t len = 0;
  if (napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len) != napi_ok) {
    napi_value result = nullptr;
    napi_create_int32(env, -1, &result);
    return result;
  }
  std::string name(len + 1, '\0');
  napi_get_value_string_utf8(env, argv[0], &name[0], len + 1, &len);
  name.resize(len);

  int id = ItemRegistry::GetItemId(name.c_str());
  napi_value result = nullptr;
  napi_create_int32(env, id, &result);
  return result;
}

static napi_value GetItemIds(napi_env env, napi_callback_info info) {
  (void)info;
  napi_value obj = nullptr;
  if (napi_create_object(env, &obj) != napi_ok)
    return nullptr;

  const auto &list = ItemRegistry::GetAllItemIds();
  for (const auto &p : list) {
    napi_value val = nullptr;
    napi_create_int32(env, p.second, &val);
    napi_set_named_property(env, obj, p.first.c_str(), val);
  }
  return obj;
}

static struct ItemsModuleRegistrar {
  ItemsModuleRegistrar() {
    ModuleRegistry::Register(
        {"items",
         {
             ModuleFunction("getItemId", GetItemId, {{"name", "string"}}, "number"),
             ModuleFunction("getItemIds", GetItemIds, {}, "Record<string, number>"),
         },
         "Item/block ID registry. Use Item.SWORD_DIAMOND etc. (from getItemIds()) or getItemId('SWORD_DIAMOND')."});
  }
} s_items_module_registrar;
