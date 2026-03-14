#include "stdafx.h"
#include "event_bus.h"
#include "mod_log.h"
#include <cstring>
#include <stdexcept>

std::mutex &EventBus::QueueMutex() {
  static std::mutex m;
  return m;
}

std::queue<EventBus::QueuedEvent> &EventBus::Queue() {
  static std::queue<QueuedEvent> q;
  return q;
}

std::unordered_map<std::string, std::vector<napi_ref>> &EventBus::Listeners() {
  static std::unordered_map<std::string, std::vector<napi_ref>> m;
  return m;
}

napi_env &EventBus::StoredEnv() {
  static napi_env env = nullptr;
  return env;
}

void EventBus::SetEnv(napi_env env) { StoredEnv() = env; }

void EventBus::On(napi_env env, const char *eventName, napi_value callback) {
  if (!eventName || !callback)
    return;
  napi_valuetype vt;
  if (napi_typeof(env, callback, &vt) != napi_ok || vt != napi_function)
    return;
  napi_ref ref = nullptr;
  if (napi_create_reference(env, callback, 1, &ref) != napi_ok)
    return;
  std::string key(eventName);
  Listeners()[key].push_back(ref);
}

void EventBus::QueueEmit(const char *eventName, const char *payloadJson) {
  if (!eventName)
    return;
  QueuedEvent e;
  e.name = eventName;
  e.payload = payloadJson ? payloadJson : "{}";
  std::lock_guard<std::mutex> lock(QueueMutex());
  Queue().push(std::move(e));
}

void EventBus::ProcessQueue(napi_env env) {
  if (!env)
    return;
  std::vector<QueuedEvent> batch;
  {
    std::lock_guard<std::mutex> lock(QueueMutex());
    while (!Queue().empty()) {
      batch.push_back(std::move(Queue().front()));
      Queue().pop();
    }
  }
  if (batch.empty())
    return;
  for (const QueuedEvent &e : batch) {
    auto it = Listeners().find(e.name);
    if (it == Listeners().end())
      continue;
    napi_value payloadVal = nullptr;
    if (!e.payload.empty()) {
      napi_value jsonGlobal = nullptr;
      if (napi_get_global(env, &jsonGlobal) != napi_ok) {
        ModLog("[EventBus] ProcessQueue: get_global failed for '%s'\n", e.name.c_str());
        continue;
      }
      napi_value parseFn = nullptr;
      if (napi_get_named_property(env, jsonGlobal, "JSON", &parseFn) != napi_ok) {
        ModLog("[EventBus] ProcessQueue: get JSON failed for '%s'\n", e.name.c_str());
        continue;
      }
      napi_value parse = nullptr;
      if (napi_get_named_property(env, parseFn, "parse", &parse) != napi_ok) {
        ModLog("[EventBus] ProcessQueue: get JSON.parse failed for '%s'\n", e.name.c_str());
        continue;
      }
      napi_value payloadStr = nullptr;
      if (napi_create_string_utf8(env, e.payload.c_str(), e.payload.size(),
                                  &payloadStr) != napi_ok) {
        ModLog("[EventBus] ProcessQueue: create_string failed for '%s'\n", e.name.c_str());
        continue;
      }
      napi_value argv[] = {payloadStr};
      if (napi_call_function(env, parseFn, parse, 1, argv, &payloadVal) != napi_ok)
        payloadVal = nullptr;
    }
    if (!payloadVal) {
      if (napi_create_object(env, &payloadVal) != napi_ok) {
        ModLog("[EventBus] ProcessQueue: create_object failed for '%s'\n", e.name.c_str());
        continue;
      }
    }
    // napi_call_function requires a valid recv (not nullptr). Use global as receiver.
    napi_value recv = nullptr;
    if (napi_get_global(env, &recv) != napi_ok) {
      ModLog("[EventBus] ProcessQueue: get_global failed for recv '%s'\n", e.name.c_str());
      continue;
    }
    for (napi_ref ref : it->second) {
      napi_value fn = nullptr;
      if (napi_get_reference_value(env, ref, &fn) != napi_ok) {
        ModLog("[EventBus] ProcessQueue: get_reference_value failed for '%s'\n", e.name.c_str());
        continue;
      }
      napi_value argv[] = {payloadVal};
      napi_value result = nullptr;
      napi_status s = napi_call_function(env, recv, fn, 1, argv, &result);
      if (s != napi_ok)
        ModLog("[EventBus] ProcessQueue: napi_call_function failed for '%s' status=%d\n", e.name.c_str(), (int)s);
    }
  }
}
