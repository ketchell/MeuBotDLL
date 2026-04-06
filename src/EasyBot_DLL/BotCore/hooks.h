#ifndef HOOKS_H
#define HOOKS_H
#include "declarations.h"
#include "EventDispatcher.h"
#include "CustomFunctions.h"
#include "BuildConfig.h"
#include "offsets_global.h"
#include <map>
#include <fstream>
#include <atomic>


inline std::map<std::string, std::pair<uintptr_t, uintptr_t>> SingletonFunctions;
inline std::map<std::string, uintptr_t> ClassMemberFunctions;
// Set to true in EasyBot() after all string scans complete.
// Checked by the gRPC service to return UNAVAILABLE instead of calling
// game functions through potentially-null function pointers.
inline std::atomic<bool> g_ready{false};
inline bool g_isLuaWrapperServer = false;

// ---------------------------------------------------------------------------
// Auto-calibration of EBP frame offsets for bindSingletonFunction hooks.
//
// The hook reads the function pointer and this-pointer from the caller's EBP
// frame.  The exact offsets vary per server build.  We scan EBP+0x08 through
// EBP+0x08+(22*4) across the first ~15 singleton and class bindings and pick
// the slot that most consistently holds a .text address.
// ---------------------------------------------------------------------------
inline uint32_t singletonFunctionOffset = 0x10;  // default; updated by calibration
inline uint32_t classFunctionOffset     = 0x0C;  // default; updated by calibration
inline bool     g_offsetsCalibrated     = false;

// Called once before hooks are enabled to record the .text section range.
void InitTextRange();

// Forces calibration to complete with whatever samples have been collected so far.
// Call this after a timeout if g_offsetsCalibrated is still false.
void ForceCalibrate();

// Clears class calibration state so the histogram can rebuild from scratch.
// Call when Thing.getId returns null after calibration has already completed.
void ResetClassCalibration();

// Re-replays the calibration buffer using newOffset and locks classFunctionOffset
// to that value.  Used by Thing::getId brute-force fallback.
void ForceClassOffset(uint32_t newOffset);

typedef void(__stdcall* bindSingletonFunction_t)(uintptr_t, uintptr_t, uintptr_t);
inline bindSingletonFunction_t original_bindSingletonFunction = nullptr;
void __stdcall hooked_bindSingletonFunction(uintptr_t, uintptr_t, uintptr_t);

// cdecl variant — used when the target function has no EBP frame (first byte != 0x55).
void __cdecl hooked_bindSingletonFunction_cdecl(uintptr_t, uintptr_t, uintptr_t);

typedef void(__stdcall* callGlobalField_t)(uintptr_t**, uintptr_t**);
inline callGlobalField_t original_callGlobalField = nullptr;
void __stdcall hooked_callGlobalField(uintptr_t**, uintptr_t**);
void __cdecl   hooked_callGlobalField_cdecl(uintptr_t**, uintptr_t**);

typedef void(__stdcall* callLuaField_t)(uintptr_t*);
inline callLuaField_t original_callLuaField = nullptr;
void __stdcall hooked_callLuaField(uintptr_t*);

typedef int(__cdecl* mainLoop)(int a1);
inline mainLoop original_mainLoop = nullptr;
int __cdecl hooked_MainLoop(int a1);

typedef void(__stdcall* look_t)(const uintptr_t *RDX,const bool isBattleList);
inline look_t look_original = nullptr;
void __stdcall hooked_Look(const uintptr_t& thing, const bool isBattleList);



#endif //HOOKS_H