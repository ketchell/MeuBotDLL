#pragma once
#include "ServerProfile.h"

// Run full auto-detection: FindPattern / AutoFind fallback, hook installation,
// binding capture, Lua-wrapper detection, OffsetResolver.
// Hooks are installed and REMAIN live after this returns.
// profile->serverName must be set by the caller before calling.
// Returns false only on hard failure (mainLoop not found).
bool ScanServer(ServerProfile* profile);

// Apply a populated profile (from ScanServer or LoadServerProfile):
//   - Sets g_isLuaWrapperServer / g_botOffsets globals
//   - Installs hooks at saved addresses (no-ops if already installed by ScanServer)
//   - Waits for binding capture (instant if ScanServer already ran)
//   - Installs look hook
//   - Sets g_ready=true and returns
// The caller is responsible for starting the gRPC server after this returns.
void ApplyProfile(const ServerProfile* profile);
