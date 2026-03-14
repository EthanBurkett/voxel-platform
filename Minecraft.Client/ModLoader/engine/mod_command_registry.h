#pragma once

#include <node_api.h>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// ModCommandRegistry — /name args… → JS callback(args[]).
//
// - Register from commands.register(name, fn) on the Node thread.
// - handleClientSideCommand (game thread): if a handler exists for the first
//   token after '/', queue dispatch and return true (chat not sent).
// - ProcessDispatchQueue(env) runs on Node thread (e.g. after EventBus tick).
// ---------------------------------------------------------------------------
namespace ModCommandRegistry {

void Register(napi_env env, const char *name, napi_value callback);

void Unregister(napi_env env, const char *name);

bool HasHandler(const std::string &nameLowerAscii);

void QueueDispatch(const std::string &nameLower,
                   const std::vector<std::string> &args);

void ProcessDispatchQueue(napi_env env);

} // namespace ModCommandRegistry
