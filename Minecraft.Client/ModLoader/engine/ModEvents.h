#pragma once

// ---------------------------------------------------------------------------
// ModEvents — emit game events to TS mods from C++.
//
// Include this from game code and call ModEvents::Emit when something
// happens. Listeners registered via mc.events.on() in TS will receive
// the event (when the Node event loop runs and processes the queue).
//
// Event names: "player_join", "player_chat", "world_loaded", "tick", etc.
// Payload is a JSON object string, e.g. "{}" or "{\"playerId\":0}".
// ---------------------------------------------------------------------------
#include "event_bus.h"
#include <string>

namespace ModEvents {

// Emit a raw event with a JSON payload string.
inline void Emit(const char *eventName, const char *payloadJson = "{}") {
  EventBus::QueueEmit(eventName, payloadJson);
}

// Emit player_join with optional player index.
inline void EmitPlayerJoin(int playerId = 0) {
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"playerId\":%d}", playerId);
  EventBus::QueueEmit("player_join", buf);
}

// Emit player_chat. Message is UTF-8; pass narrow string from game if possible.
inline void EmitPlayerChat(const char *message, int playerId = 0) {
  if (!message)
    message = "";
  std::string escaped;
  for (const char *p = message; *p; ++p) {
    if (*p == '\\' || *p == '"')
      escaped += '\\';
    escaped += *p;
  }
  char buf[512];
  snprintf(buf, sizeof(buf), "{\"message\":\"%s\",\"playerId\":%d}",
           escaped.c_str(), playerId);
  EventBus::QueueEmit("player_chat", buf);
}

// Emit world_loaded (e.g. when entering a world).
inline void EmitWorldLoaded(const char *worldName = "") {
  if (!worldName)
    worldName = "";
  std::string escaped;
  for (const char *p = worldName; *p; ++p) {
    if (*p == '\\' || *p == '"')
      escaped += '\\';
    escaped += *p;
  }
  char buf[256];
  snprintf(buf, sizeof(buf), "{\"worldName\":\"%s\"}", escaped.c_str());
  EventBus::QueueEmit("world_loaded", buf);
}

// Emit tick (every frame or every N ms) for debug / testing.
inline void EmitTick() { EventBus::QueueEmit("tick", "{}"); }

// Emit join_world (player + world). Payload: { playerId, worldName }. Implemented in ModEvents.cpp.
void EmitJoinWorldW(int playerId, const std::wstring& worldName);

// Emit player_chat from wide string (e.g. game chat message). Implemented in ModEvents.cpp.
void EmitPlayerChatW(const wchar_t *message, int playerId = 0);

// Emit world_loaded with level name from wide string. Implemented in ModEvents.cpp.
void EmitWorldLoadedW(const std::wstring& worldName);
} // namespace ModEvents
