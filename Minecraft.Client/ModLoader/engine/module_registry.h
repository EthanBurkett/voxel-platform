#pragma once

#include <node_api.h>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Type metadata for .d.ts generation
//
// Use TypeScript type strings directly: "string", "number", "boolean",
// "void", "any", "string[]", "Record<string, unknown>", etc.
//
// Example:
//   { "msg", napi_string },  →  just for runtime
//
//   TsParam{ "msg", "string" }   →  for type generation
// ---------------------------------------------------------------------------

struct TsParam
{
    std::string name;
    std::string
        type; // TypeScript type string, e.g. "string", "number", "boolean"
};

// A single exported function on a NAPI module.
struct ModuleFunction
{
    std::string name;
    napi_callback callback;

    // Type metadata — used only for .d.ts generation, not at runtime.
    std::vector<TsParam> params;
    std::string returnType = "void"; // default to void

    // Convenience constructor: no type metadata (generates any-typed signature).
    ModuleFunction(std::string n, napi_callback cb)
        : name(std::move(n)), callback(cb)
    {
    }

    // Full constructor with type metadata.
    ModuleFunction(std::string n, napi_callback cb, std::vector<TsParam> p,
                   std::string ret = "void")
        : name(std::move(n)), callback(cb), params(std::move(p)),
          returnType(std::move(ret))
    {
    }
};

// A complete NAPI module: name + exported functions.
struct ModuleDefinition
{
    std::string name;
    std::vector<ModuleFunction> functions;

    // Optional JSDoc comment emitted at the top of the generated .d.ts.
    std::string description;
};

// ---------------------------------------------------------------------------
// ModuleRegistry
//
// Static registry of all C++ modules to be linked into Node.
// Modules self-register via static initializers in their own .cpp files.
//
// Usage:
//   ModuleRegistry::Register({
//     "engine",
//     {
//       ModuleFunction("log", Log, {{"msg", "string"}}, "void"),
//       ModuleFunction("getTime", GetTime, {}, "number"),
//     },
//     "Core engine bindings exposed to mod scripts."
//   });
// ---------------------------------------------------------------------------
class ModuleRegistry
{
  public:
    static void Register(ModuleDefinition def);

    // Returns all registered modules. Called by NodeHost — do not call directly.
    static const std::vector<ModuleDefinition> &GetAll();

  private:
    static std::vector<ModuleDefinition> &Modules();
};
