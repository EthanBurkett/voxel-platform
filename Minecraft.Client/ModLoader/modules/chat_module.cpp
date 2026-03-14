#include "engine/ModEvents.h"
#include "engine/module_registry.h"
#include <node_api.h>
#include <string>

// ---------------------------------------------------------------------------
// chat.local(message) — show on this client's chat HUD only (command feedback).
// ---------------------------------------------------------------------------
static napi_value Local(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1)
    {
        return nullptr;
    }

    size_t len = 0;
    napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len);
    std::string msg(len + 1, '\0');
    napi_get_value_string_utf8(env, argv[0], &msg[0], len + 1, &len);
    msg.resize(len);

    ModEvents::SetPendingLocalChatMessageUtf8(msg.c_str());

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    return result;
}

// ---------------------------------------------------------------------------
// chat.send(message) — broadcast: sent as player chat (everyone sees <name> msg).
// ---------------------------------------------------------------------------
static napi_value Send(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1)
    {
        return nullptr;
    }

    size_t len = 0;
    napi_get_value_string_utf8(env, argv[0], nullptr, 0, &len);
    std::string msg(len + 1, '\0');
    napi_get_value_string_utf8(env, argv[0], &msg[0], len + 1, &len);
    msg.resize(len);

    ModEvents::SetPendingChatMessageUtf8(msg.c_str());

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    return result;
}

static struct ChatModuleRegistrar
{
    ChatModuleRegistrar()
    {
        ModuleRegistry::Register(
            {"chat",
             {
                 ModuleFunction("local", Local, {{"message", "string"}}, "void"),
                 ModuleFunction("send", Send, {{"message", "string"}}, "void"),
             },
             "local = HUD only. send = broadcast plain text to all."});
    }
} s_chat_module_registrar;
