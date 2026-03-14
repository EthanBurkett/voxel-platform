#pragma once

#include <node_api.h>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// EventBus — C++ side of the mod event system.
//
// - TS mods register via mc.events.on(eventName, callback).
// - Game code calls EventBus::Emit(eventName, payloadJson) to queue an event.
// - When the Node/V8 context runs (e.g. in NodeHost), call ProcessQueue(env)
//   to invoke all registered listeners for queued events.
//
// Thread safety: QueueEmit may be called from any thread; listeners are
// only invoked from the thread that calls ProcessQueue(env) (Node thread).
// ---------------------------------------------------------------------------
class EventBus {
public:
  // Register a JS callback for an event. Call from NAPI (mc.events.on).
  static void On(napi_env env, const char *eventName, napi_value callback);

  // Queue an event to be dispatched on the next ProcessQueue(env) call.
  // payloadJson is a JSON string (object) passed to the listener.
  static void QueueEmit(const char *eventName, const char *payloadJson);

  // Process all queued events and invoke registered listeners.
  // Must be called from the Node thread (same context as On()).
  static void ProcessQueue(napi_env env);

  // Called by NodeHost when the Node loop is ready. Stores env for refs.
  static void SetEnv(napi_env env);

private:
  struct QueuedEvent {
    std::string name;
    std::string payload;
  };

  static std::mutex &QueueMutex();
  static std::queue<QueuedEvent> &Queue();
  static std::unordered_map<std::string, std::vector<napi_ref>> &Listeners();
  static napi_env &StoredEnv();
};
