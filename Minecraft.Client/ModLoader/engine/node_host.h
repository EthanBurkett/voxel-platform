#pragma once

#include <memory>
#include <node.h>
#include <node_api.h>
#include <string>
#include <v8.h>
#include <vector>

// ---------------------------------------------------------------------------
// NodeHost
//
// Owns the entire Node.js/V8 lifecycle: initialization, linked module
// registration, type generation, bootstrap execution, and event loop.
//
// Usage:
//   NodeHost host;
//   if (!host.Init(argc, argv)) return 1;
//   return host.Run();
// ---------------------------------------------------------------------------
class NodeHost {
public:
  NodeHost() = default;
  ~NodeHost();

  // Initialize Node, V8, and all registered NAPI modules.
  // Also generates .d.ts files and tsconfig.json into types/ and mods/.
  //
  // default_script: if non-empty and no script given via argv, run this file.
  //                 if empty (default), bootstrap scans mods/ automatically.
  bool Init(int argc, char **argv, const std::string &default_script = "");

  // Run the event loop. Returns the process exit code.
  int Run();

private:
  void GenerateTypes();
  bool LinkModules();

  // napi_module holds raw char* pointers — we own the strings here.
  struct LinkedModule {
    napi_module mod;
    std::string name;
    std::string filename;
  };

  std::shared_ptr<node::InitializationResult> init_result_;
  std::unique_ptr<node::CommonEnvironmentSetup> setup_;
  std::vector<std::unique_ptr<LinkedModule>> linked_modules_;
  std::string script_path_;
  bool initialized_ = false;
};