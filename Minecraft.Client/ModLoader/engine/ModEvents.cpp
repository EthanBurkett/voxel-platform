#include "stdafx.h"
#include "ModEvents.h"
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ModEvents {

void EmitPlayerChatW(const wchar_t *message, int playerId) {
  if (!message) {
    EmitPlayerChat(nullptr, playerId);
    return;
  }
#ifdef _WIN32
  int len = WideCharToMultiByte(CP_UTF8, 0, message, -1, nullptr, 0, nullptr, nullptr);
  if (len <= 0) {
    EmitPlayerChat("", playerId);
    return;
  }
  std::string utf8(static_cast<size_t>(len), '\0');
  WideCharToMultiByte(CP_UTF8, 0, message, -1, &utf8[0], len, nullptr, nullptr);
  utf8.resize(static_cast<size_t>(len - 1)); // drop NUL
  EmitPlayerChat(utf8.c_str(), playerId);
#else
  (void)playerId;
  // Non-Windows: could use iconv or similar; for now no-op
  EventBus::QueueEmit("player_chat", "{\"message\":\"\",\"playerId\":0}");
#endif
}

void EmitWorldLoadedW(const std::wstring& worldName) {
#ifdef _WIN32
  if (worldName.empty()) {
    EmitWorldLoaded("");
    return;
  }
  int len = WideCharToMultiByte(CP_UTF8, 0, worldName.c_str(), (int)worldName.size(), nullptr, 0, nullptr, nullptr);
  if (len <= 0) {
    EmitWorldLoaded("");
    return;
  }
  std::string utf8(static_cast<size_t>(len), '\0');
  WideCharToMultiByte(CP_UTF8, 0, worldName.c_str(), (int)worldName.size(), &utf8[0], len, nullptr, nullptr);
  EmitWorldLoaded(utf8.c_str());
#else
  (void)worldName;
  EmitWorldLoaded("");
#endif
}

static void WstringToUtf8(const std::wstring& ws, std::string* out) {
  if (!out) return;
  out->clear();
  if (ws.empty()) return;
#ifdef _WIN32
  int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
  if (len <= 0) return;
  out->resize(static_cast<size_t>(len));
  WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &(*out)[0], len, nullptr, nullptr);
#endif
}

void EmitJoinWorldW(int playerId, const std::wstring& worldName) {
  std::string utf8;
  WstringToUtf8(worldName, &utf8);
  std::string escaped;
  for (char c : utf8) {
    if (c == '\\' || c == '"') escaped += '\\';
    escaped += c;
  }
  char buf[320];
  snprintf(buf, sizeof(buf), "{\"playerId\":%d,\"worldName\":\"%s\"}", playerId, escaped.c_str());
  EventBus::QueueEmit("join_world", buf);
}
} // namespace ModEvents
