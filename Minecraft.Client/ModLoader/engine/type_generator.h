#pragma once

#include <string>

// ---------------------------------------------------------------------------
// TypeGenerator
//
// Generates TypeScript type files from the ModuleRegistry at startup.
// Called once by NodeHost::Init() before the event loop runs.
//
// Output layout (relative to exe / process.cwd()):
//
//   types/
//     engine.d.ts       ← one file per registered C++ module
//     physics.d.ts
//     ...
//     globals.d.ts      ← re-exports everything as globalThis properties
//
//   mods/
//     tsconfig.json     ← written once if not present; references types/
// ---------------------------------------------------------------------------
class TypeGenerator {
public:
  // outputDir  = directory to write .d.ts files into (e.g. "types")
  // modsDir    = directory where mod scripts live (e.g. "mods")
  //              tsconfig.json is written here.
  static void Generate(const std::string &outputDir,
                       const std::string &modsDir);

private:
  static void GenerateModuleDts(const std::string &outputDir);
  static void GenerateGlobalsDts(const std::string &outputDir);
  static void GenerateTsConfig(const std::string &modsDir,
                               const std::string &typesRelPath);

  // Ensure a directory exists (creates it if missing).
  static bool EnsureDir(const std::string &dir);

  // Write string to file, overwriting if exists.
  static bool WriteFile(const std::string &path, const std::string &content);
};