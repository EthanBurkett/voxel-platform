#pragma once

#include <cstdarg>

// ---------------------------------------------------------------------------
// ModLog — single logging path for the ModLoader.
// Writes to mods.log next to the executable and (on Windows) OutputDebugString.
// Thread-safe. Use for all embed/engine/mod logging so output is in one place.
// ---------------------------------------------------------------------------

// printf-style: ModLog("[embed] message: %s\n", str.c_str());
void ModLog(const char *fmt, ...);

// Variant for already-built string (no format args).
void ModLogMessage(const char *msg);
