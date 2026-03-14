#include "node_host.h"
#include "bootstrap.h"
#include "mod_log.h"
#include "module_registry.h"
#include "stdafx.h"
#include "type_generator.h"


#include <cstdio>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Per-module init dispatch
//
// napi_module::nm_register_func is called LAZILY by Node the first time
// process._linkedBinding(name) is called from JS — NOT synchronously during
// AddLinkedBinding. This means we cannot use thread-locals to pass context,
// and napi_get_module_file_name doesn't exist in all Node versions.
//
// Solution: stamp out one static init function per module slot via a template.
// Each ModuleInitN<I> reads slot I from a global table of ModuleDefinition
// pointers. The table is populated during LinkModules() before any JS runs.
// ---------------------------------------------------------------------------

struct ModuleInitData
{
    const ModuleDefinition *def;
};

static std::vector<ModuleInitData> &InitTable()
{
    static std::vector<ModuleInitData> t;
    return t;
}

template <int N>
static napi_value ModuleInitN(napi_env env, napi_value exports)
{
    auto &table = InitTable();
    if (N >= (int)table.size())
    {
        return exports;
    }
    const ModuleDefinition *def = table[N].def;
    if (!def)
    {
        return exports;
    }

    for (const auto &fn : def->functions)
    {
        napi_value nfn;
        napi_create_function(env, fn.name.c_str(), NAPI_AUTO_LENGTH, fn.callback,
                             nullptr, &nfn);
        napi_set_named_property(env, exports, fn.name.c_str(), nfn);
    }
    ModLog("[embed] Module init: %s (%zu functions)\n",
           def->name.c_str(), def->functions.size());
    return exports;
}

// Stamp out 32 slots — enough for any reasonable number of C++ modules.
static napi_addon_register_func kInitFunctions[] = {
    ModuleInitN<0>,
    ModuleInitN<1>,
    ModuleInitN<2>,
    ModuleInitN<3>,
    ModuleInitN<4>,
    ModuleInitN<5>,
    ModuleInitN<6>,
    ModuleInitN<7>,
    ModuleInitN<8>,
    ModuleInitN<9>,
    ModuleInitN<10>,
    ModuleInitN<11>,
    ModuleInitN<12>,
    ModuleInitN<13>,
    ModuleInitN<14>,
    ModuleInitN<15>,
    ModuleInitN<16>,
    ModuleInitN<17>,
    ModuleInitN<18>,
    ModuleInitN<19>,
    ModuleInitN<20>,
    ModuleInitN<21>,
    ModuleInitN<22>,
    ModuleInitN<23>,
    ModuleInitN<24>,
    ModuleInitN<25>,
    ModuleInitN<26>,
    ModuleInitN<27>,
    ModuleInitN<28>,
    ModuleInitN<29>,
    ModuleInitN<30>,
    ModuleInitN<31>,
};
static constexpr int kMaxModules = 32;

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
NodeHost::~NodeHost()
{
    setup_.reset();
    if (initialized_)
    {
        node::TearDownOncePerProcess();
    }
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
bool NodeHost::Init(int argc, char **argv, const std::string &default_script)
{
    std::vector<std::string> args(argv, argv + argc);
    init_result_ = node::InitializeOncePerProcess(args);

    for (const std::string &err : init_result_->errors())
    {
        ModLog("[embed] Init error: %s\n", err.c_str());
    }

    if (init_result_->early_return())
    {
        return false;
    }

    if (!init_result_->platform())
    {
        ModLog("[embed] No platform — aborting\n");
        return false;
    }

    std::vector<std::string> env_args = init_result_->args();
    if (env_args.size() < 2)
    {
        env_args.push_back(default_script);
    }

    script_path_ = env_args.size() >= 2 ? env_args[1] : "";

    const bool single_script = script_path_.find('.') != std::string::npos;
    if (single_script)
    {
        ModLog("[embed] Single-script mode: %s\n", script_path_.c_str());
    }
    else
    {
        ModLog("[embed] Mods directory mode — scanning mods/\n");
    }

    std::vector<std::string> setup_errors;
    setup_ = node::CommonEnvironmentSetup::Create(init_result_->platform(),
                                                  &setup_errors, env_args,
                                                  init_result_->exec_args());

    if (!setup_)
    {
        ModLog("[embed] Environment setup failed:\n");
        for (const std::string &err : setup_errors)
        {
            ModLog("  %s\n", err.c_str());
        }
        return false;
    }

    initialized_ = true;
    ModLog("[embed] Environment ready\n");

    GenerateTypes();
    return true;
}

// ---------------------------------------------------------------------------
// GenerateTypes
// ---------------------------------------------------------------------------
void NodeHost::GenerateTypes()
{
    TypeGenerator::Generate("types", "mods");
}

// ---------------------------------------------------------------------------
// LinkModules
// ---------------------------------------------------------------------------
bool NodeHost::LinkModules()
{
    const auto &defs = ModuleRegistry::GetAll();

    if ((int)defs.size() > kMaxModules)
    {
        ModLog("[embed] Too many modules (%zu > %d)\n", defs.size(), kMaxModules);
        return false;
    }

    // Pre-populate the init table so each ModuleInitN<I> can find its definition.
    auto &table = InitTable();
    table.resize(defs.size());
    for (size_t i = 0; i < defs.size(); ++i)
    {
        table[i].def = &defs[i];
    }

    for (size_t i = 0; i < defs.size(); ++i)
    {
        const auto &def = defs[i];

        auto linked = std::make_unique<LinkedModule>();
        linked->name = def.name;
        linked->filename = "linked:" + def.name;

        linked->mod = {
            NAPI_MODULE_VERSION, 0, linked->filename.c_str(),
            kInitFunctions[i], // unique function pointer per slot
            linked->name.c_str(),
            nullptr,
            {nullptr, nullptr, nullptr, nullptr}};

        node::AddLinkedBinding(setup_->env(), linked->mod);
        linked_modules_.push_back(std::move(linked));
        ModLog("[embed] Linked module registered: %s\n", def.name.c_str());
    }

    return true;
}

// ---------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------
int NodeHost::Run()
{
    if (!initialized_ || !setup_)
    {
        ModLog("[embed] NodeHost::Run called before successful Init\n");
        return 1;
    }

    v8::Isolate *isolate = setup_->isolate();
    int exit_code = 0;

    {
        v8::Locker locker(isolate);
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Context::Scope context_scope(setup_->context());

        LinkModules();

        // Inject linked module names into JS so bootstrap can expose them as
        // globals.
        {
            v8::Local<v8::Context> ctx = setup_->context();
            v8::Local<v8::Array> names = v8::Array::New(isolate);
            const auto &defs = ModuleRegistry::GetAll();
            for (uint32_t i = 0; i < (uint32_t)defs.size(); ++i)
            {
                auto name = v8::String::NewFromUtf8(isolate, defs[i].name.c_str())
                                .ToLocalChecked();
                names->Set(ctx, i, name).Check();
            }
            ctx->Global()
                ->Set(ctx,
                      v8::String::NewFromUtf8(isolate, "__linked_modules__")
                          .ToLocalChecked(),
                      names)
                .Check();
        }

        v8::TryCatch try_catch(isolate);
        v8::MaybeLocal<v8::Value> load_result =
            node::LoadEnvironment(setup_->env(), kBootstrap);

        if (load_result.IsEmpty())
        {
            ModLog("[embed] LoadEnvironment returned empty\n");
            if (try_catch.HasCaught())
            {
                v8::Local<v8::Value> exc = try_catch.Exception();
                v8::String::Utf8Value msg(isolate, exc);
                v8::Local<v8::Message> message = try_catch.Message();
                if (!message.IsEmpty())
                {
                    v8::String::Utf8Value fname(isolate,
                                                message->GetScriptResourceName());
                    int line = message->GetLineNumber(setup_->context()).FromMaybe(-1);
                    ModLog("[embed] Exception at %s:%d — %s\n",
                           *fname ? *fname : "?", line, *msg ? *msg : "(unknown)");
                }
                else
                {
                    ModLog("[embed] Exception: %s\n", *msg ? *msg : "(unknown)");
                }
            }
            return 1;
        }

        ModLog("[embed] Entering event loop\n");

        exit_code = node::SpinEventLoop(setup_->env()).FromMaybe(1);

        ModLog("[embed] Event loop exited with code %d\n", exit_code);

        node::Stop(setup_->env());
    }

    return exit_code;
}
