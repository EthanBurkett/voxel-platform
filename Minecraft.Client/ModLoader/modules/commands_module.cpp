#include "engine/module_registry.h"
#include "engine/mod_command_registry.h"
#include <node_api.h>
#include <string>

static napi_value Register(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 2)
    return nullptr;
  size_t len = 0;
  if (napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len) != napi_ok)
    return nullptr;
  std::string name(len + 1, '\0');
  napi_get_value_string_utf8(env, argv[0], &name[0], len + 1, &len);
  name.resize(len);
  ModCommandRegistry::Register(env, name.c_str(), argv[1]);
  napi_value u;
  napi_get_undefined(env, &u);
  return u;
}

static napi_value Unregister(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;
  size_t len = 0;
  if (napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len) != napi_ok)
    return nullptr;
  std::string name(len + 1, '\0');
  napi_get_value_string_utf8(env, argv[0], &name[0], len + 1, &len);
  name.resize(len);
  ModCommandRegistry::Unregister(env, name.c_str());
  napi_value u;
  napi_get_undefined(env, &u);
  return u;
}

static struct CommandsModuleRegistrar {
  CommandsModuleRegistrar() {
    ModuleRegistry::Register(
        {"commands",
         {
             ModuleFunction("register", Register,
                            {{"name", "string"}, {"callback", "(args: string[]) => void"}},
                            "void"),
             ModuleFunction("unregister", Unregister, {{"name", "string"}}, "void"),
         },
         "Register chat commands: /name args — callback receives args split by spaces."});
  }
} s_commands_module_registrar;
