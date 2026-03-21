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
