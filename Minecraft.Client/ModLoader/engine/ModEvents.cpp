#include "stdafx.h"
#include "ModEvents.h"
#include <cstdio>
#include <string>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ModEvents {

static std::mutex s_pendingChatMutex;
static std::wstring s_pendingChatMessage;
static bool s_hasPendingChat = false;

static std::mutex s_pendingLocalMutex;
static std::vector<std::wstring> s_pendingLocalMessages;

void SetPendingChatMessage(const std::wstring& message) {
  std::lock_guard<std::mutex> lock(s_pendingChatMutex);
  s_pendingChatMessage = message;
  s_hasPendingChat = true;
}

bool ConsumePendingChatMessage(std::wstring* out) {
  if (!out) return false;
  std::lock_guard<std::mutex> lock(s_pendingChatMutex);
  if (!s_hasPendingChat) return false;
  *out = s_pendingChatMessage;
  s_pendingChatMessage.clear();
  s_hasPendingChat = false;
  return true;
}

void SetPendingChatMessageUtf8(const char* messageUtf8) {
  if (!messageUtf8) {
    SetPendingChatMessage(std::wstring());
    return;
  }
#ifdef _WIN32
  int len = MultiByteToWideChar(CP_UTF8, 0, messageUtf8, -1, nullptr, 0);
  if (len <= 0) {
    SetPendingChatMessage(std::wstring());
    return;
  }
  std::wstring ws(static_cast<size_t>(len), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, messageUtf8, -1, &ws[0], len);
  ws.resize(static_cast<size_t>(len - 1));
  SetPendingChatMessage(ws);
#else
  (void)messageUtf8;
  SetPendingChatMessage(std::wstring());
#endif
}

void SetPendingLocalChatMessageUtf8(const char* messageUtf8) {
  if (!messageUtf8 || !*messageUtf8)
    return;
#ifdef _WIN32
  int len = MultiByteToWideChar(CP_UTF8, 0, messageUtf8, -1, nullptr, 0);
  if (len <= 0)
    return;
  std::wstring ws(static_cast<size_t>(len), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, messageUtf8, -1, &ws[0], len);
  ws.resize(static_cast<size_t>(len - 1));
  std::lock_guard<std::mutex> lock(s_pendingLocalMutex);
  s_pendingLocalMessages.push_back(std::move(ws));
#else
  (void)messageUtf8;
#endif
}

void ConsumePendingLocalChatMessages(std::vector<std::wstring>* out) {
  if (!out)
    return;
  std::lock_guard<std::mutex> lock(s_pendingLocalMutex);
  out->clear();
  out->swap(s_pendingLocalMessages);
}

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

void EmitCommand(const char* command, const char* argsJson) {
  if (!command) command = "";
  if (!argsJson) argsJson = "[]";
  std::string escapedCmd;
  for (const char* p = command; *p; ++p) {
    if (*p == '\\' || *p == '"') escapedCmd += '\\';
    escapedCmd += *p;
  }
  char buf[512];
  snprintf(buf, sizeof(buf), "{\"command\":\"%s\",\"args\":%s}", escapedCmd.c_str(), argsJson);
  EventBus::QueueEmit("command", buf);
}
} // namespace ModEvents
