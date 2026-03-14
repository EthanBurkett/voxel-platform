#pragma once

// kBootstrap: first JS executed inside the embedded Node environment.
//
// Responsibilities:
//   1. Patch Module._load to intercept require() for linked C++ modules.
//   2. Scan the mods/ directory and discover all mods.
//   3. For each mod, determine its type and resolve its entry point:
//        - Single file mod:     mods/foo.ts  or  mods/foo.js
//        - Directory mod:       mods/foo/    (reads package.json for "main",
//        falls back to index.ts/index.js)
//   4. Compile TypeScript in-process via esbuild (no build step for mod
//   authors).
//   5. Run each mod in an isolated module context with its own require:
//        - Directory mods get their own node_modules (mods/foo/node_modules).
//        - Single file mods share the global node_modules next to the exe.
//   6. Expose all linked C++ modules as globals and via require() in every mod
//   context.

static const char *kBootstrap = R"JS(
(function() {
  'use strict';

  process.stderr.write('[embed] Bootstrap entered\n');

  // ============================================================
  // Utilities
  // ============================================================
  const Module = require('module');
  const fs     = require('fs');
  const path   = require('path');
  const vm     = require('vm');

  // Root = directory containing the executable (process.cwd()).
  const ROOT     = process.cwd();
  const MODS_DIR = path.join(ROOT, 'mods');

  // ============================================================
  // 1. Linked C++ module setup
  //    __linked_modules__ is injected by NodeHost::Run() before bootstrap runs.
  //
  //    process._linkedBinding(name) triggers the NAPI Init function on first
  //    call, which populates the exports object. ModuleInitDispatch identifies
  //    which module to initialize by reading nm_filename from the env.
  // ============================================================
  const linkedNames = globalThis.__linked_modules__ || [];
  const linkedCache = {};

  // Patch Module._load first so require('engine') works from mod scripts.
  const _origLoad = Module._load;
  Module._load = function(request, parent, isMain) {
    if (Object.prototype.hasOwnProperty.call(linkedCache, request))
      return linkedCache[request];
    return _origLoad.apply(this, arguments);
  };

  // Initialize each linked module via _linkedBinding which triggers NAPI Init.
  for (const name of linkedNames) {
    try {
      const binding = process._linkedBinding(name);
      linkedCache[name] = binding;
      globalThis[name]  = binding;
      // Expose WorldLocation constructor globally (avoids conflict with Node/DOM Location)
      if (name === 'WorldLocation' && typeof binding.WorldLocation === 'function') {
        globalThis.WorldLocation = binding.WorldLocation;
      }
      process.stderr.write('[embed] C++ module bound: ' + name +
        ' (keys: ' + Object.keys(binding).join(', ') + ')\n');
    } catch (e) {
      process.stderr.write('[embed] Warning: could not bind "' + name + '": ' + e.message + '\n');
    }
  }

  // Strictly typed helper: events.onJoinWorld((player, world) => void) — subscribes to join_world and passes Player + World instances
  if (globalThis.events && globalThis.player && typeof globalThis.events.on === 'function') {
    globalThis.events.onJoinWorld = function(cb) {
      globalThis.events.on('join_world', function() {
        var p = globalThis.player.get();
        if (p && typeof p.getWorld === 'function') cb(p, p.getWorld());
      });
    };
  }

  // ============================================================
  // 2. esbuild loader
  //    Loaded lazily from ROOT/node_modules. Directory mods can
  //    also ship their own esbuild but we try root first.
  // ============================================================
  let _esbuild = null;
  function getEsbuild(modNodeModules) {
    if (_esbuild) return _esbuild;
    // Try the mod's own node_modules first, then root.
    const searchDirs = modNodeModules
      ? [modNodeModules, path.join(ROOT, 'node_modules')]
      : [path.join(ROOT, 'node_modules')];

    for (const dir of searchDirs) {
      try {
        const req = Module.createRequire(dir + '/');
        _esbuild = req('esbuild');
        process.stderr.write('[embed] esbuild loaded from: ' + dir + '\n');
        return _esbuild;
      } catch (_) {}
    }
    throw new Error(
      '[embed] esbuild not found. Install it in your mods directory or next to the exe:\n' +
      '  npm install esbuild'
    );
  }

  // ============================================================
  // 3. TypeScript compilation helpers
  // ============================================================

  // Build a JS preamble that defines each linked C++ module as a local var.
  // This is prepended to every compiled mod so that esbuild's internal
  // __require() shim resolves them from the preamble instead of hitting
  // Node's module loader (which doesn't know about linked bindings).
  //
  // Result looks like:
  //   var __linkedModules = { engine: globalThis.engine, ... };
  //   var __origRequire = require;
  //   var require = function(id) {
  //     if (Object.prototype.hasOwnProperty.call(__linkedModules, id))
  //       return __linkedModules[id];
  //     return __origRequire(id);
  //   };
  function makeLinkedPreamble() {
    if (linkedNames.length === 0) return '';
    // Inject each linked module as a const so any require('engine') or
    // direct reference resolves to the already-bound globalThis value.
    const lines = linkedNames.map(n =>
      'var ' + n + ' = globalThis[' + JSON.stringify(n) + '];'
    ).join('\n');
    return lines + '\n';
  }

  // Compile a single .ts file to CJS JS string.
  // We use stdin mode so esbuild strips TypeScript syntax only — it does NOT
  // bundle or resolve require() calls. Those are left as-is and handled at
  // runtime by our wrappedRequire which knows about linked C++ modules.
  function compileFile(filePath, modNodeModules) {
    const esbuild = getEsbuild(modNodeModules);
    const src = fs.readFileSync(filePath, 'utf8');

    const result = esbuild.buildSync({
      stdin: {
        contents:   src,
        sourcefile: filePath,
        loader:     'ts',
      },
      bundle:        false,   // do NOT bundle — leave require() calls intact
      write:         false,
      platform:      'node',
      format:        'cjs',
      target:        'node20',
      sourcemap:     'inline',
    });

    if (result.errors && result.errors.length > 0) {
      const msgs = result.errors.map(e => e.text).join('\n');
      throw new Error('esbuild errors:\n' + msgs);
    }

    return makeLinkedPreamble() + result.outputFiles[0].text;
  }

  // Bundle an entire directory mod to a single CJS JS string.
  // entry = resolved path to index.ts / index.js / package.json "main".
  // For directory mods we DO bundle (resolves local imports across files),
  // and mark linked C++ modules as external so they pass through to require().
  function bundleDirectory(entryFile, modDir, modNodeModules) {
    const esbuild = getEsbuild(modNodeModules);

    const result = esbuild.buildSync({
      entryPoints:   [entryFile],
      bundle:        true,
      write:         false,
      platform:      'node',
      format:        'cjs',
      target:        'node20',
      sourcemap:     'inline',
      external:      [...linkedNames],
      nodePaths:     modNodeModules ? [modNodeModules] : [],
      absWorkingDir: modDir,
    });

    if (result.errors && result.errors.length > 0) {
      const msgs = result.errors.map(e => e.text).join('\n');
      throw new Error('esbuild bundle errors:\n' + msgs);
    }

    return makeLinkedPreamble() + result.outputFiles[0].text;
  }

  // ============================================================
  // 4. Mod descriptor resolution
  //    Returns: { name, entryFile, modDir, nodeModulesDir, isDirectory, isTs }
  // ============================================================
  function resolveMod(modPath) {
    const stat = fs.statSync(modPath);
    const name = path.basename(modPath, path.extname(modPath));

    if (stat.isDirectory()) {
      const modDir        = modPath;
      const pkgJsonPath   = path.join(modDir, 'package.json');
      const nodeModulesDir = path.join(modDir, 'node_modules');
      const hasNodeModules = fs.existsSync(nodeModulesDir);

      // Resolve entry point from package.json "main", or fall back to index.ts/index.js.
      let entryFile = null;

      if (fs.existsSync(pkgJsonPath)) {
        try {
          const pkg = JSON.parse(fs.readFileSync(pkgJsonPath, 'utf8'));
          if (pkg.main) {
            const candidate = path.join(modDir, pkg.main);
            if (fs.existsSync(candidate)) {
              entryFile = candidate;
              process.stderr.write('[mod:' + name + '] Entry from package.json: ' + pkg.main + '\n');
            } else {
              process.stderr.write('[mod:' + name + '] Warning: package.json "main" not found: ' + pkg.main + '\n');
            }
          }
        } catch (e) {
          process.stderr.write('[mod:' + name + '] Warning: could not parse package.json: ' + e.message + '\n');
        }
      }

      if (!entryFile) {
        // Fallback priority: index.ts > index.js
        const candidates = [
          path.join(modDir, 'index.ts'),
          path.join(modDir, 'index.js'),
        ];
        for (const c of candidates) {
          if (fs.existsSync(c)) { entryFile = c; break; }
        }
      }

      if (!entryFile) {
        throw new Error('No entry point found for directory mod "' + name + '". ' +
          'Add a package.json "main" field or create index.ts / index.js.');
      }

      const isTs = path.extname(entryFile).toLowerCase() === '.ts';
      return {
        name,
        entryFile,
        modDir,
        nodeModulesDir: hasNodeModules ? nodeModulesDir : null,
        isDirectory: true,
        isTs,
      };

    } else {
      // Single file mod.
      const ext = path.extname(modPath).toLowerCase();
      if (ext !== '.ts' && ext !== '.js') return null; // skip non-mod files

      return {
        name,
        entryFile: modPath,
        modDir:    path.dirname(modPath),
        nodeModulesDir: null, // use root node_modules
        isDirectory: false,
        isTs: ext === '.ts',
      };
    }
  }

  // ============================================================
  // 5. Mod execution
  //    Each mod gets an isolated require() rooted at its own directory
  //    (or mod's node_modules if it has one), so mods can't accidentally
  //    stomp each other's dependencies.
  // ============================================================
  function runMod(descriptor) {
    const { name, entryFile, modDir, nodeModulesDir, isDirectory, isTs } = descriptor;

    process.stderr.write('[mod:' + name + '] Loading ' + entryFile + '\n');

    // Build the require for this mod.
    // If the mod has its own node_modules, resolve from there.
    // Otherwise resolve from ROOT so the global node_modules is used.
    const requireBase = nodeModulesDir
      ? path.join(modDir, 'index.js') // anchor require to mod dir
      : path.join(ROOT, 'index.js');  // anchor require to root

    const modRequire = Module.createRequire(requireBase);

    // Wrap modRequire to also intercept linked C++ modules.
    const wrappedRequire = function(id) {
      if (Object.prototype.hasOwnProperty.call(linkedCache, id))
        return linkedCache[id];
      return modRequire(id);
    };
    wrappedRequire.resolve = modRequire.resolve.bind(modRequire);
    wrappedRequire.cache   = modRequire.cache;
    wrappedRequire.main    = modRequire.main;

    // Compile / load code.
    let code;
    if (isDirectory) {
      // Directory mods are fully bundled — all local imports resolved by esbuild.
      code = bundleDirectory(entryFile, modDir, nodeModulesDir);
    } else if (isTs) {
      // Single .ts file — transform only.
      code = compileFile(entryFile, null);
    } else {
      // Plain .js file.
      code = fs.readFileSync(entryFile, 'utf8');
    }

    process.stderr.write('[mod:' + name + '] Running (' + code.length + ' bytes)\n');

    // Wrap in a CommonJS-like IIFE with all standard Node module-scope globals
    // explicitly passed in. vm.runInThisContext shares the same V8 context so
    // globalThis works, but module-scope names like process, Buffer, setTimeout
    // are not automatically in scope inside the function — pass them explicitly.
    const wrapped =
      '(function(module, exports, require, __dirname, __filename,' +
      ' process, Buffer, setTimeout, clearTimeout, setInterval, clearInterval,' +
      ' setImmediate, clearImmediate, URL, URLSearchParams, TextEncoder, TextDecoder,' +
      ' console) {\n' +
      code +
      '\n})';

    const fn = vm.runInThisContext(wrapped, {
      filename:      entryFile,
      displayErrors: true,
    });

    const modModule = { exports: {}, id: entryFile, filename: entryFile, loaded: false };

    fn(
      modModule, modModule.exports, wrappedRequire,
      path.dirname(entryFile), entryFile,
      // Node globals:
      process, Buffer, setTimeout, clearTimeout, setInterval, clearInterval,
      setImmediate, clearImmediate, URL, URLSearchParams,
      typeof TextEncoder !== 'undefined' ? TextEncoder : undefined,
      typeof TextDecoder !== 'undefined' ? TextDecoder : undefined,
      console
    );

    modModule.loaded = true;
    process.stderr.write('[mod:' + name + '] Done\n');
  }

  // ============================================================
  // 6. Mods directory scanner + runner
  // ============================================================
  function loadAllMods() {
    if (!fs.existsSync(MODS_DIR)) {
      process.stderr.write('[embed] No mods directory found at: ' + MODS_DIR + '\n');
      return;
    }

    const entries = fs.readdirSync(MODS_DIR);
    if (entries.length === 0) {
      process.stderr.write('[embed] mods/ directory is empty\n');
      return;
    }

    process.stderr.write('[embed] Scanning mods/: ' + entries.length + ' entries\n');

    let loaded = 0;
    let failed = 0;

    for (const entry of entries) {
      const fullPath = path.join(MODS_DIR, entry);

      // Skip hidden files and non-mod files at the top level.
      if (entry.startsWith('.')) continue;

      let descriptor;
      try {
        descriptor = resolveMod(fullPath);
      } catch (e) {
        process.stderr.write('[mod:' + entry + '] Resolve error: ' + e.message + '\n');
        failed++;
        continue;
      }

      if (!descriptor) continue; // not a mod file (e.g. .json, .txt at top level)

      try {
        runMod(descriptor);
        loaded++;
      } catch (e) {
        process.stderr.write('[mod:' + descriptor.name + '] Runtime error: ' +
          (e && e.stack ? e.stack : String(e)) + '\n');
        failed++;
        // Continue loading other mods — one bad mod doesn't kill the rest.
      }
    }

    process.stderr.write(
      '[embed] Mods loaded: ' + loaded + ' ok, ' + failed + ' failed\n'
    );
  }

  // ============================================================
  // Event pump: process C++-queued game events every tick so mods receive them.
  // ============================================================
  if (globalThis.events && typeof globalThis.events._tick === 'function') {
    function eventPump() {
      try {
        events._tick();
      } catch (e) {
        process.stderr.write('[embed] event pump error: ' + (e && e.message ? e.message : String(e)) + '\n');
      }
      setImmediate(eventPump);
    }
    setImmediate(eventPump);
    process.stderr.write('[embed] Event pump started\n');
  }

  // ============================================================
  // 7. Single-script mode (legacy / CLI override)
  //    If process.argv[1] was explicitly set to a specific file
  //    rather than being the default, run just that one file
  //    instead of scanning mods/.
  // ============================================================
  function isSingleScriptMode() {
    const arg = process.argv[1];
    if (!arg) return false;
    // If it points directly to a file (not a directory named "mods"),
    // and it exists as a file, treat as single-script mode.
    if (fs.existsSync(arg) && !fs.statSync(arg).isDirectory()) return true;
    if (arg.endsWith('.ts') || arg.endsWith('.js')) return true;
    return false;
  }

  try {
    if (isSingleScriptMode()) {
      const scriptPath = process.argv[1];
      process.stderr.write('[embed] Single-script mode: ' + scriptPath + '\n');

      let descriptor;
      try {
        descriptor = resolveMod(scriptPath);
      } catch(e) {
        throw new Error('Could not resolve script "' + scriptPath + '": ' + e.message);
      }

      if (!descriptor)
        throw new Error('Not a valid mod file: ' + scriptPath);

      runMod(descriptor);
    } else {
      loadAllMods();
    }
  } catch (e) {
    process.stderr.write('[embed] Fatal: ' + (e && e.stack ? e.stack : String(e)) + '\n');
    process.exitCode = 1;
  }

})();
)JS";