#include "engine/module_registry.h"
#include "engine/mod_objects.h"
#include <node_api.h>

// ---------------------------------------------------------------------------
// WorldLocation(x, y, z) — constructor; use from JS as new WorldLocation(x, y, z).
// Avoids conflict with Node/DOM global Location (href, pathname, etc.).
// Returns a WorldLocation instance with x, y, z and distanceTo(other).
// ---------------------------------------------------------------------------
static napi_value WorldLocationConstructor(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv[3];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 3)
    return nullptr;

  double x = 0, y = 0, z = 0;
  napi_get_value_double(env, argv[0], &x);
  napi_get_value_double(env, argv[1], &y);
  napi_get_value_double(env, argv[2], &z);

  return ModObjects::CreateLocation(env, x, y, z);
}

static struct WorldLocationModuleRegistrar {
  WorldLocationModuleRegistrar() {
    ModuleRegistry::Register(
        {"WorldLocation",
         {ModuleFunction("WorldLocation", WorldLocationConstructor,
                         {{"x", "number"}, {"y", "number"}, {"z", "number"}},
                         "WorldLocation")}});
  }
} s_world_location_module_registrar;
