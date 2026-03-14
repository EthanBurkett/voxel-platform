#include "stdafx.h"
#include "engine/event_bus.h"
#include "engine/mod_command_registry.h"
#include "engine/module_registry.h"
#include <node_api.h>

// ---------------------------------------------------------------------------
// events.on(eventName, callback, scope?) — register a listener. scope used for hot-reload cleanup.
// ---------------------------------------------------------------------------
static napi_value On(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv[3];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 2)
    return nullptr;

  size_t len = 0;
  if (napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len) != napi_ok)
    return nullptr;
  char *name = new char[len + 1];
  napi_get_value_string_utf8(env, argv[0], name, len + 1, &len);
  name[len] = '\0';

  const char *scopeOpt = nullptr;
  if (argc >= 3) {
    napi_valuetype vt;
    if (napi_typeof(env, argv[2], &vt) == napi_ok && vt == napi_string) {
      size_t slen = 0;
      napi_get_value_string_utf8(env, argv[2], nullptr, 0, &slen);
      if (slen > 0) {
        char *scope = new char[slen + 1];
        napi_get_value_string_utf8(env, argv[2], scope, slen + 1, &slen);
        scope[slen] = '\0';
        EventBus::On(env, name, argv[1], scope);
        delete[] scope;
        delete[] name;
        return nullptr;
      }
    }
  }
  EventBus::On(env, name, argv[1], scopeOpt);
  delete[] name;
  return nullptr;
}

// ---------------------------------------------------------------------------
// events.offScope(scope) — remove all listeners for a mod scope (hot-reload).
// ---------------------------------------------------------------------------
static napi_value OffScope(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1)
    return nullptr;
  size_t len = 0;
  if (napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len) != napi_ok)
    return nullptr;
  char *scope = new char[len + 1];
  napi_get_value_string_utf8(env, argv[0], scope, len + 1, &len);
  scope[len] = '\0';
  EventBus::RemoveScope(env, scope);
  delete[] scope;
  return nullptr;
}

// ---------------------------------------------------------------------------
// events._tick() — process queued events (called by bootstrap event pump).
// ---------------------------------------------------------------------------
static napi_value Tick(napi_env env, napi_callback_info info) {
  EventBus::ProcessQueue(env);
  ModCommandRegistry::ProcessDispatchQueue(env);
  return nullptr;
}

static struct EventsModuleRegistrar {
  EventsModuleRegistrar() {
    ModuleRegistry::Register(
        {"events",
         {
             ModuleFunction("on", On,
                            {{"eventName", "string"},
                             {"callback", "(data: Record<string, unknown>) => void"},
                             {"scope", "string"}},
                            "void"),
             ModuleFunction("offScope", OffScope, {{"scope", "string"}}, "void"),
             ModuleFunction("_tick", Tick, {}, "void"),
         }});
  }
} s_events_module_registrar;
