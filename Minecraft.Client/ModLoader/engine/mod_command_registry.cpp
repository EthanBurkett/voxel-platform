#include "stdafx.h"
#include "mod_command_registry.h"
#include "mod_log.h"
#include <cctype>
#include <deque>
#include <mutex>
#include <unordered_map>

namespace ModCommandRegistry {

static std::string ToLowerAscii(const std::string &s) {
  std::string o;
  o.reserve(s.size());
  for (unsigned char c : s) {
    if (c >= 'A' && c <= 'Z')
      o += static_cast<char>(c - 'A' + 'a');
    else
      o += static_cast<char>(c);
  }
  return o;
}

static std::mutex s_handlersMutex;
static std::unordered_map<std::string, napi_ref> s_handlers;

static std::mutex s_dispatchMutex;
struct Pending {
  std::string name;
  std::vector<std::string> args;
};
static std::deque<Pending> s_pending;

void Register(napi_env env, const char *name, napi_value callback) {
  if (!name || !callback)
    return;
  napi_valuetype vt;
  if (napi_typeof(env, callback, &vt) != napi_ok || vt != napi_function)
    return;
  std::string key = ToLowerAscii(name);
  napi_ref ref = nullptr;
  if (napi_create_reference(env, callback, 1, &ref) != napi_ok)
    return;
  std::lock_guard<std::mutex> lock(s_handlersMutex);
  auto it = s_handlers.find(key);
  if (it != s_handlers.end() && it->second) {
    napi_delete_reference(env, it->second);
  }
  s_handlers[key] = ref;
  ModLog("[commands] registered /%s\n", key.c_str());
}

void Unregister(napi_env env, const char *name) {
  if (!name)
    return;
  std::string key = ToLowerAscii(name);
  std::lock_guard<std::mutex> lock(s_handlersMutex);
  auto it = s_handlers.find(key);
  if (it != s_handlers.end() && it->second) {
    napi_delete_reference(env, it->second);
    s_handlers.erase(it);
  }
}

bool HasHandler(const std::string &nameLowerAscii) {
  if (nameLowerAscii.empty())
    return false;
  std::lock_guard<std::mutex> lock(s_handlersMutex);
  return s_handlers.find(nameLowerAscii) != s_handlers.end();
}

void QueueDispatch(const std::string &nameLower,
                   const std::vector<std::string> &args) {
  Pending p;
  p.name = nameLower;
  p.args = args;
  std::lock_guard<std::mutex> lock(s_dispatchMutex);
  s_pending.push_back(std::move(p));
}

void ProcessDispatchQueue(napi_env env) {
  if (!env)
    return;
  std::deque<Pending> batch;
  {
    std::lock_guard<std::mutex> lock(s_dispatchMutex);
    batch.swap(s_pending);
  }
  napi_value recv = nullptr;
  if (napi_get_global(env, &recv) != napi_ok)
    return;

  for (const Pending &p : batch) {
    napi_ref ref = nullptr;
    {
      std::lock_guard<std::mutex> lock(s_handlersMutex);
      auto it = s_handlers.find(p.name);
      if (it == s_handlers.end() || !it->second)
        continue;
      ref = it->second;
    }
    napi_value fn = nullptr;
    if (napi_get_reference_value(env, ref, &fn) != napi_ok)
      continue;

    napi_value argsArr = nullptr;
    if (napi_create_array_with_length(env, p.args.size(), &argsArr) != napi_ok)
      continue;
    for (size_t i = 0; i < p.args.size(); i++) {
      napi_value s = nullptr;
      if (napi_create_string_utf8(env, p.args[i].c_str(), p.args[i].size(),
                                  &s) != napi_ok)
        continue;
      napi_set_element(env, argsArr, (uint32_t)i, s);
    }

    napi_value argv[] = {argsArr};
    napi_value result = nullptr;
    napi_status st =
        napi_call_function(env, recv, fn, 1, argv, &result);
    if (st != napi_ok)
      ModLog("[commands] callback /%s failed status=%d\n", p.name.c_str(),
             (int)st);
  }
}

} // namespace ModCommandRegistry
