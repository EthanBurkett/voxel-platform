#include "stdafx.h"
#include "engine/mod_log.h"
#include "engine/module_registry.h"
#include <node_api.h>

// ---------------------------------------------------------------------------
// engine.log(msg) — uses shared ModLog (mods.log + OutputDebugString).
// ---------------------------------------------------------------------------
static napi_value Log(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;

  size_t len = 0;
  if (napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len) != napi_ok)
    return nullptr;
  char *buf = new char[len + 1];
  napi_get_value_string_utf8(env, argv[0], buf, len + 1, &len);
  buf[len] = '\0';

  ModLogMessage(buf);
  delete[] buf;
  return nullptr;
}

static struct EngineModuleRegistrar {
  EngineModuleRegistrar() {
    ModuleRegistry::Register(
        {"engine",
         {ModuleFunction("log", Log, {{"msg", "string"}}, "void")}});
  }
} s_engine_module_registrar;
