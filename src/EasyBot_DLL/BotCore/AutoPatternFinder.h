#pragma once
#include <cstdint>

// ============================================================================
// AutoPatternFinder
//
// Locates hook target functions in any OTClient x86 binary by scanning for
// known string literals and following code cross-references to their call
// sites — no hardcoded byte patterns required.
//
// Usage:
//   AutoFinderResult r = AutoFindHookTargets();
//   if (r.success) { /* use r.bindSingletonFunction, r.callGlobalField */ }
//
// mainLoop is intentionally left at 0 — the caller should fall back to the
// existing FindPattern() scan for that target (it has no reliable string refs).
//
// Full diagnostic log is written to C:\easybot_autofind.log.
// ============================================================================

struct AutoFinderResult {
    uintptr_t bindSingletonFunction; // 0 if not found
    uintptr_t callGlobalField;       // 0 if not found
    uintptr_t mainLoop;              // always 0 — use pattern scan fallback
    bool      success;               // true if both non-mainLoop targets found
    char      errorMsg[256];         // human-readable failure reason
};

AutoFinderResult AutoFindHookTargets();

// ============================================================================
// ExtractLuaStateGlobal
//
// Scans the first 64 bytes of bindSingletonFunction for the sequence:
//   68 EE D8 FF FF  (push LUA_REGISTRYINDEX)
//   FF 35 XX XX XX XX  (push [imm32])
// and returns the imm32 — the address of the global lua_State* variable.
// Dereference the returned address to get the live lua_State*.
//
// Returns 0 if the pattern is not found.
// ============================================================================

uintptr_t ExtractLuaStateGlobal(uintptr_t bindSingletonFunc);
