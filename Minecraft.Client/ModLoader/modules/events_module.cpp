#include "stdafx.h"
#include "engine/event_bus.h"
#include "engine/module_registry.h"
#include <node_api.h>

// ---------------------------------------------------------------------------
// events.on(eventName, callback) — register a listener for game events.
// ---------------------------------------------------------------------------
static napi_value On(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 2)
    return nullptr;

  size_t len = 0;
  if (napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len) != napi_ok)
    return nullptr;
  char *name = new char[len + 1];
  napi_get_value_string_utf8(env, argv[0], name, len + 1, &len);
  name[len] = '\0';

  EventBus::On(env, name, argv[1]);
  delete[] name;
  return nullptr;
}

// ---------------------------------------------------------------------------
// events._tick() — process queued events (called by bootstrap event pump).
// ---------------------------------------------------------------------------
static napi_value Tick(napi_env env, napi_callback_info info) {
  EventBus::ProcessQueue(env);
  return nullptr;
}

static struct EventsModuleRegistrar {
  EventsModuleRegistrar() {
    ModuleRegistry::Register(
        {"events",
         {
             ModuleFunction("on", On,
                            {{"eventName", "string"},
                             {"callback", "(data: Record<string, unknown>) => void"}},
                            "void"),
             ModuleFunction("_tick", Tick, {}, "void"),
         }});
  }
} s_events_module_registrar;
