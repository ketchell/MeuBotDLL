#include "hooks.h"
#include "BuildConfig.h"
#include "Creature.h"
#include "CustomFunctions.h"
#include "pattern_scan.h"
#include "MinHook.h"

// ============================================================================
// SEH-protected pointer read — prevents a bad EBP offset from crashing the
// client.  Returns 0 on access fault.
// No C++ objects — __try is valid.
// ============================================================================
static uintptr_t tryReadPtr(uintptr_t addr) {
    __try {
        return *reinterpret_cast<const uintptr_t*>(addr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}

// ============================================================================
// .text range — set once by InitTextRange(), used by IsInText().
// ============================================================================
static uintptr_t s_textBase = 0;
static uintptr_t s_textEnd  = 0;

void InitTextRange() {
    HMODULE hMod = GetModuleHandleW(nullptr);
    if (!hMod) return;
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
    auto* nt  = reinterpret_cast<IMAGE_NT_HEADERS*>(
        reinterpret_cast<uintptr_t>(hMod) + dos->e_lfanew);
    auto* sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        if (memcmp(sec->Name, ".text", 5) == 0) {
            s_textBase = reinterpret_cast<uintptr_t>(hMod) + sec->VirtualAddress;
            s_textEnd  = s_textBase + sec->Misc.VirtualSize;
            return;
        }
    }
}

static bool IsInText(uintptr_t addr) {
    return addr && s_textBase && addr >= s_textBase && addr < s_textEnd;
}

// ============================================================================
// Calibration state — accumulated across the first N binding calls.
// ============================================================================
static const int CALIB_SLOTS  = 22;  // scan EBP+0x08 .. EBP+0x08+(21*4)
static const int CALIB_TARGET = 8;   // need this many valid samples each

static int s_singletonHits[CALIB_SLOTS] = {};
static int s_classHits[CALIB_SLOTS]     = {};  // only Thing/Item/Creature/Player/LocalPlayer
static int s_singletonTotal = 0;
static int s_classTotal     = 0;      // counts only calibration-class bindings

// High-water marks for the best calibration seen so far.  Neither variable is
// ever cleared by ResetClassCalibration(), so ForceCalibrate() can detect that
// a real calibration already ran even after the histogram has been wiped.
static int      s_bestClassHits   = 0;
static uint32_t s_bestClassOffset = 0;   // 0 means "not yet calibrated"

// Set to true the first time a non-forced calibration produces a valid result
// (offset >= 0x10).  Once locked, ForceCalibrate() is always a no-op.
static bool g_classCalibrationLocked = false;

// Shared counter for diagnostic log numbering across both hook variants.
static int s_calCount = 0;

// Buffer for ALL class member calls that arrive during calibration (including
// non-calibration classes like LuaObject/UIWidget).  Their EBP slot data is
// stored so we can extract the correct function pointer after calibration.
struct CalibrationEntry {
    char      key[128];
    uintptr_t values[CALIB_SLOTS];
};
static CalibrationEntry s_classBuffer[1024];
static int              s_classBufferCount = 0;

// ============================================================================
// IsCalibrationClass
//
// Returns true for class names whose bindSingletonFunction EBP layout is
// representative of the game's C++ member-function calling convention.
//
// LuaObject, UIWidget, and other Lua-bridge classes use a different wrapper
// frame that places the function pointer at a higher offset (e.g. 0x3C),
// which contaminates the histogram and causes classFunctionOffset to be wrong.
// ============================================================================
static bool IsCalibrationClass(const std::string& name) {
    return name == "Thing"       ||
           name == "Item"        ||
           name == "Creature"    ||
           name == "Player"      ||
           name == "LocalPlayer";
}

// ============================================================================
// PickClassOffset
//
// Selects classFunctionOffset from the filtered histogram.
//
// For each slot in descending hit-count order (minimum 3 hits):
//   - Scan the calibration buffer for an address at that slot that is both
//     within the .text segment AND begins with a recognised x86 function-
//     prologue byte (0x55 push ebp, 0x53 push ebx).
//   - The first slot that satisfies this wins — no arbitrary offset ceiling.
//
// outWinAddr receives the first validated function pointer found at the
// winning slot so callers can log it.
// ============================================================================
static const uint32_t CLASS_OFFSET_FALLBACK = 0x08;

// SEH-safe single-byte read — lives in its own function so __try is valid
// (no C++ objects with destructors in scope).
static uint8_t safeReadByte(uintptr_t addr) {
    __try { return *reinterpret_cast<const uint8_t*>(addr); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}

static uint32_t PickClassOffset(
    const int* hits, int nSlots,
    const CalibrationEntry* buf, int bufCount,
    const char** outSource, uintptr_t* outWinAddr)
{
    if (outWinAddr) *outWinAddr = 0;

    // Iterate slots from highest to lowest hit count.
    bool tried[22] = {};   // CALIB_SLOTS == 22, all POD — safe with __try in callees
    for (int pass = 0; pass < nSlots; ++pass) {
        // Find the untried slot with the most hits.
        int bestCI = -1;
        for (int i = 0; i < nSlots; ++i) {
            if (tried[i]) continue;
            if (bestCI < 0 || hits[i] > hits[bestCI]) bestCI = i;
        }
        if (bestCI < 0 || hits[bestCI] < 3) break;  // too few samples — stop
        tried[bestCI] = true;

        // Validate: look for a buffered address at this slot that is in .text
        // and starts with a recognised function-prologue byte.
        for (int bi = 0; bi < bufCount; ++bi) {
            uintptr_t addr = buf[bi].values[bestCI];
            if (!IsInText(addr)) continue;
            uint8_t prologue = safeReadByte(addr);
            if (prologue == 0x55 || prologue == 0x53) {
                if (outWinAddr) *outWinAddr = addr;
                if (outSource)  *outSource  = "histogram+prologue";
                return 0x08 + (uint32_t)bestCI * 4;
            }
        }
        // This slot's addresses don't pass — try the next best.
    }

    if (outSource) *outSource = "fallback(no valid prologue)";
    return CLASS_OFFSET_FALLBACK;
}

// ============================================================================
// ApplyCalibrationResult
//
// Single implementation shared by the normal calibration path (both stdcall
// and cdecl hook variants) and by ForceCalibrate().  Picks both offsets,
// replays the class buffer, and writes diagnostic logs.
// ============================================================================
static void ApplyCalibrationResult(bool forced) {
    // Hard guard: once a real calibration has locked in a good offset, a forced
    // (0-sample) run must never overwrite it.  g_classCalibrationLocked is set
    // below the first time a non-forced call produces offset >= 0x10.
    if (forced && g_classCalibrationLocked) {
        classFunctionOffset = s_bestClassOffset;   // repair any reset damage
        g_offsetsCalibrated = true;

        char msg[128];
        snprintf(msg, sizeof(msg),
            "ForceCalibrate SKIPPED (locked): offset=0x%X hits=%d",
            s_bestClassOffset, s_bestClassHits);
        FILE* fc = fopen("C:\\easybot_calibrate.log", "a");
        if (fc) { fprintf(fc, "\n=== %s ===\n", msg); fclose(fc); }
        FILE* fi = fopen("C:\\easybot_init.log", "a");
        if (fi) { fprintf(fi, "%s\n", msg); fclose(fi); }
        return;
    }

    // Singleton offset: take the histogram winner unconditionally.
    int bestSI = 0;
    for (int i = 1; i < CALIB_SLOTS; ++i)
        if (s_singletonHits[i] >= s_singletonHits[bestSI]) bestSI = i;
    if (s_singletonHits[bestSI] > 0) singletonFunctionOffset = 0x08 + bestSI * 4;

    // Class offset: pick the best slot whose buffered addresses pass the
    // .text + prologue validation.  No arbitrary offset ceiling.
    const char* classSource = nullptr;
    uintptr_t   winAddr     = 0;
    classFunctionOffset = PickClassOffset(
        s_classHits, CALIB_SLOTS,
        s_classBuffer, s_classBufferCount,
        &classSource, &winAddr);

    g_offsetsCalibrated = true;

    // Lock calibration so ForceCalibrate() can never overwrite this result.
    if (!forced && classFunctionOffset >= 0x10) {
        g_classCalibrationLocked = true;
        s_bestClassOffset = classFunctionOffset;
        FILE* fi = fopen("C:\\easybot_init.log", "a");
        if (fi) {
            fprintf(fi, "Calibration locked: offset=0x%X\n", classFunctionOffset);
            fclose(fi);
        }

        // Install the look hook now — SingletonFunctions["g_game.look"] is
        // guaranteed to be populated at this point because calibration only
        // completes after the bind hooks have fired enough times to fill the
        // histogram, which means g_game.look has already been captured.
        uintptr_t lookAddr = SingletonFunctions["g_game.look"].first;
        FILE* fl = fopen("C:\\easybot_init.log", "a");
        if (fl) {
            fprintf(fl, "Look hook install (post-calib): lookAddr=0x%08X\n",
                (unsigned)lookAddr);
            fclose(fl);
        }
        if (lookAddr) {
            MH_STATUS cs = MH_CreateHook(
                reinterpret_cast<LPVOID>(lookAddr),
                reinterpret_cast<LPVOID>(&hooked_Look),
                reinterpret_cast<LPVOID*>(&look_original));
            MH_STATUS es = MH_EnableHook(reinterpret_cast<LPVOID>(lookAddr));
            FILE* fl2 = fopen("C:\\easybot_init.log", "a");
            if (fl2) {
                fprintf(fl2, "Look hook install: createStatus=%d  enableStatus=%d\n",
                    (int)cs, (int)es);
                fclose(fl2);
            }
        }
    }

    // Replay the buffer.  All buffered class members (including LuaObject etc.)
    // are replayed using the validated offset so their pointers are available.
    int slotIdx = (classFunctionOffset - 0x08) / 4;
    // Persist the best hit count for diagnostic logging.
    if (s_classHits[slotIdx] > s_bestClassHits)
        s_bestClassHits = s_classHits[slotIdx];
    FILE* fh = fopen("C:\\easybot_hooks.log", "a");
    for (int i = 0; i < s_classBufferCount; ++i) {
        uintptr_t fp = s_classBuffer[i].values[slotIdx];
        if (fp) ClassMemberFunctions[s_classBuffer[i].key] = fp;
        if (fh) fprintf(fh, "CLASS(%s): %s func=0x%08X\n",
            forced ? "force" : "replay", s_classBuffer[i].key, (unsigned)fp);
    }
    if (fh) {
        fprintf(fh, "%s %d entries at classOffset=0x%02X (source=%s)\n",
            forced ? "ForceCalibrate: replayed" : "Replayed",
            s_classBufferCount, classFunctionOffset, classSource ? classSource : "?");
        fclose(fh);
    }

    FILE* f = fopen("C:\\easybot_calibrate.log", "a");
    if (f) {
        fprintf(f, "\n=== %s ===\n",
            forced ? "FORCE CALIBRATION" : "CALIBRATION COMPLETE");
        if (forced)
            fprintf(f, "(singleton=%d class=%d filtered samples)\n",
                s_singletonTotal, s_classTotal);
        fprintf(f, "singletonFunctionOffset = 0x%02X  (hits=%d/%d)\n",
            singletonFunctionOffset, s_singletonHits[bestSI], s_singletonTotal);
        fprintf(f, "classFunctionOffset     = 0x%02X  (hits=%d/%d, source=%s)\n",
            classFunctionOffset,
            s_classHits[slotIdx], s_classTotal,
            classSource ? classSource : "?");
        fprintf(f, "singleton histogram:");
        for (int i = 0; i < CALIB_SLOTS; ++i) fprintf(f, " %d", s_singletonHits[i]);
        fprintf(f, "\nclass histogram (filtered):   ");
        for (int i = 0; i < CALIB_SLOTS; ++i) fprintf(f, " %d", s_classHits[i]);
        fprintf(f, "\nReplayed %d buffered class members with offset 0x%02X\n",
            s_classBufferCount, classFunctionOffset);
        fclose(f);
    }

    // Write the key result to easybot_started.txt for quick at-a-glance diagnosis.
    FILE* fs = fopen("C:\\easybot_started.txt", "a");
    if (fs) {
        fprintf(fs, "classFunctionOffset=0x%02X  winAddr=0x%08X  source=%s  hits=%d/%d\n",
            classFunctionOffset, (unsigned)winAddr,
            classSource ? classSource : "?",
            s_classHits[slotIdx], s_classTotal);
        fclose(fs);
    }
}

// ============================================================================
// ForceCalibrate — public entry point called by EasyBotInit after a timeout.
// ============================================================================
void ForceCalibrate() {
    if (g_offsetsCalibrated) return;

    // If a real calibration already locked in a good offset, skip entirely.
    // ApplyCalibrationResult(true) has the same check, but catching it here
    // avoids any work before we even enter that function.
    if (g_classCalibrationLocked) {
        classFunctionOffset = s_bestClassOffset;
        g_offsetsCalibrated = true;

        char msg[128];
        snprintf(msg, sizeof(msg),
            "ForceCalibrate SKIPPED (locked): offset=0x%X hits=%d",
            s_bestClassOffset, s_bestClassHits);
        FILE* f = fopen("C:\\easybot_calibrate.log", "a");
        if (f) { fprintf(f, "\n=== %s ===\n", msg); fclose(f); }
        FILE* fi = fopen("C:\\easybot_init.log", "a");
        if (fi) { fprintf(fi, "%s\n", msg); fclose(fi); }
        return;
    }

    ApplyCalibrationResult(true);
}

// ============================================================================
// ResetClassCalibration
//
// Called by Thing::getId when it finds its function pointer is null despite
// g_offsetsCalibrated being true — meaning the chosen classFunctionOffset was
// wrong.  Resets only class-side state; singletonFunctionOffset is preserved.
// Setting g_offsetsCalibrated = false re-opens the hook's accumulation path so
// the next wave of binding calls rebuilds the histogram from filtered samples.
// ============================================================================
void ResetClassCalibration() {
    memset(s_classHits, 0, sizeof(s_classHits));
    s_classTotal       = 0;
    s_classBufferCount = 0;
    classFunctionOffset = CLASS_OFFSET_FALLBACK;
    g_offsetsCalibrated = false;

    FILE* f = fopen("C:\\easybot_calibrate.log", "a");
    if (f) {
        fprintf(f, "\n=== CLASS RE-CALIBRATION TRIGGERED (Thing.getId was null) ===\n");
        fprintf(f, "classFunctionOffset reset to 0x%02X, histogram cleared\n",
            CLASS_OFFSET_FALLBACK);
        fclose(f);
    }
}

// ============================================================================
// ForceClassOffset
//
// Re-replays the calibration buffer using a caller-supplied offset and locks
// classFunctionOffset to that value.  Called by Thing::getId when the current
// offset yields a null function pointer — allows a brute-force recovery to
// 0x08 or 0x0C without discarding the buffered binding data.
// ============================================================================
void ForceClassOffset(uint32_t newOffset) {
    int slotIdx = (int)((newOffset - 0x08) / 4);
    if (slotIdx < 0 || slotIdx >= CALIB_SLOTS) return;

    classFunctionOffset = newOffset;
    g_offsetsCalibrated = true;

    for (int i = 0; i < s_classBufferCount; ++i) {
        uintptr_t fp = s_classBuffer[i].values[slotIdx];
        if (fp) ClassMemberFunctions[s_classBuffer[i].key] = fp;
    }

    FILE* f = fopen("C:\\easybot_calibrate.log", "a");
    if (f) {
        fprintf(f, "\n=== ForceClassOffset: locked to 0x%02X, replayed %d entries ===\n",
            newOffset, s_classBufferCount);
        fclose(f);
    }
}

// ============================================================================
// hooked_bindSingletonFunction (stdcall)
// Calibration runs INSIDE this function so RtlCaptureContext captures a real
// EBP frame.  #pragma optimize off forces the compiler to maintain EBP.
// ============================================================================
#pragma optimize("", off)
void __stdcall hooked_bindSingletonFunction(uintptr_t a1, uintptr_t a2, uintptr_t a3) {
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    uintptr_t ebp = ctx.Ebp;

    auto global = *reinterpret_cast<std::string*>(a1);
    auto field  = *reinterpret_cast<std::string*>(a2);
    bool isSingleton = (global.size() > 1 && global[1] == '_');

    if (!g_offsetsCalibrated) {
        if (!s_textBase) InitTextRange();
        ++s_calCount;

        // Only trusted game classes vote on classFunctionOffset.
        bool isCalibClass = !isSingleton && IsCalibrationClass(global);

        if (isSingleton) {
            for (int i = 0; i < CALIB_SLOTS; ++i) {
                uintptr_t val = *reinterpret_cast<uintptr_t*>(ebp + 0x08 + i * 4);
                if (IsInText(val)) ++s_singletonHits[i];
            }
            ++s_singletonTotal;
        } else if (isCalibClass) {
            for (int i = 0; i < CALIB_SLOTS; ++i) {
                uintptr_t val = *reinterpret_cast<uintptr_t*>(ebp + 0x08 + i * 4);
                if (IsInText(val)) ++s_classHits[i];
            }
            ++s_classTotal;
        }

        // Buffer ALL class members for replay (including non-calibration classes).
        if (!isSingleton && s_classBufferCount < 1024) {
            snprintf(s_classBuffer[s_classBufferCount].key, 128,
                "%s.%s", global.c_str(), field.c_str());
            for (int i = 0; i < CALIB_SLOTS; ++i)
                s_classBuffer[s_classBufferCount].values[i] = tryReadPtr(ebp + 0x08 + i * 4);
            ++s_classBufferCount;
        }

        // Dump the first 3 calls for diagnostics.
        if (s_calCount <= 3) {
            FILE* f = fopen("C:\\easybot_calibrate.log", "a");
            if (f) {
                fprintf(f, "call #%d (stdcall) %s.%s  ebp=0x%08X  "
                    "isSingleton=%d  isCalibClass=%d  text=[0x%08X,0x%08X)\n",
                    s_calCount, global.c_str(), field.c_str(),
                    (unsigned)ebp, (int)isSingleton, (int)isCalibClass,
                    (unsigned)s_textBase, (unsigned)s_textEnd);
                for (int i = 0; i < CALIB_SLOTS; ++i) {
                    uint32_t off = 0x08 + i * 4;
                    uintptr_t val = *reinterpret_cast<uintptr_t*>(ebp + off);
                    fprintf(f, "  ebp+0x%02X = 0x%08X%s\n", off, (unsigned)val,
                        IsInText(val) ? "  <-- IN TEXT" : "");
                }
                fclose(f);
            }
        }

        if (s_singletonTotal >= CALIB_TARGET && s_classTotal >= CALIB_TARGET)
            ApplyCalibrationResult(false);

        // Capture singletons immediately — singletonFunctionOffset is stable.
        // Skip class members until classFunctionOffset is calibrated.
        if (isSingleton) {
            uintptr_t tmp        = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset);
            uintptr_t second_tmp = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset + 0x04);
            if (tmp) SingletonFunctions[global + "." + field] = {tmp, second_tmp};
            FILE* fh = fopen("C:\\easybot_hooks.log", "a");
            if (fh) {
                fprintf(fh, "BIND(calib): %s.%s  func=0x%08X  this=0x%08X\n",
                    global.c_str(), field.c_str(), (unsigned)tmp, (unsigned)second_tmp);
                fclose(fh);
            }
        }

        original_bindSingletonFunction(a1, a2, a3);
        return;
    }

    // Calibration complete: capture with correct offsets.
    uintptr_t tmp, second_tmp;
    if (!isSingleton) {
        tmp = *reinterpret_cast<uintptr_t*>(ebp + classFunctionOffset);
        if (tmp) ClassMemberFunctions[global + "." + field] = tmp;
    } else {
        tmp        = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset);
        second_tmp = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset + 0x04);
        if (tmp) SingletonFunctions[global + "." + field] = {tmp, second_tmp};
    }

    FILE* fh = fopen("C:\\easybot_hooks.log", "a");
    if (fh) {
        if (!isSingleton)
            fprintf(fh, "CLASS: %s.%s  func=0x%08X\n",
                global.c_str(), field.c_str(), (unsigned)tmp);
        else
            fprintf(fh, "BIND:  %s.%s  func=0x%08X  this=0x%08X\n",
                global.c_str(), field.c_str(), (unsigned)tmp, (unsigned)second_tmp);
        fclose(fh);
    }

    original_bindSingletonFunction(a1, a2, a3);
}
#pragma optimize("", on)

// ============================================================================
// hooked_bindSingletonFunction_cdecl — same logic, caller cleans the stack.
// ============================================================================
#pragma optimize("", off)
void __cdecl hooked_bindSingletonFunction_cdecl(uintptr_t a1, uintptr_t a2, uintptr_t a3) {
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    uintptr_t ebp = ctx.Ebp;

    auto global = *reinterpret_cast<std::string*>(a1);
    auto field  = *reinterpret_cast<std::string*>(a2);
    bool isSingleton = (global.size() > 1 && global[1] == '_');

    if (!g_offsetsCalibrated) {
        if (!s_textBase) InitTextRange();
        ++s_calCount;

        bool isCalibClass = !isSingleton && IsCalibrationClass(global);

        if (isSingleton) {
            for (int i = 0; i < CALIB_SLOTS; ++i) {
                uintptr_t val = *reinterpret_cast<uintptr_t*>(ebp + 0x08 + i * 4);
                if (IsInText(val)) ++s_singletonHits[i];
            }
            ++s_singletonTotal;
        } else if (isCalibClass) {
            for (int i = 0; i < CALIB_SLOTS; ++i) {
                uintptr_t val = *reinterpret_cast<uintptr_t*>(ebp + 0x08 + i * 4);
                if (IsInText(val)) ++s_classHits[i];
            }
            ++s_classTotal;
        }

        // Buffer ALL class members for replay.
        if (!isSingleton && s_classBufferCount < 1024) {
            snprintf(s_classBuffer[s_classBufferCount].key, 128,
                "%s.%s", global.c_str(), field.c_str());
            for (int i = 0; i < CALIB_SLOTS; ++i)
                s_classBuffer[s_classBufferCount].values[i] = tryReadPtr(ebp + 0x08 + i * 4);
            ++s_classBufferCount;
        }

        if (s_calCount <= 3) {
            FILE* f = fopen("C:\\easybot_calibrate.log", "a");
            if (f) {
                fprintf(f, "call #%d (cdecl)  %s.%s  ebp=0x%08X  "
                    "isSingleton=%d  isCalibClass=%d  text=[0x%08X,0x%08X)\n",
                    s_calCount, global.c_str(), field.c_str(),
                    (unsigned)ebp, (int)isSingleton, (int)isCalibClass,
                    (unsigned)s_textBase, (unsigned)s_textEnd);
                for (int i = 0; i < CALIB_SLOTS; ++i) {
                    uint32_t off = 0x08 + i * 4;
                    uintptr_t val = *reinterpret_cast<uintptr_t*>(ebp + off);
                    fprintf(f, "  ebp+0x%02X = 0x%08X%s\n", off, (unsigned)val,
                        IsInText(val) ? "  <-- IN TEXT" : "");
                }
                fclose(f);
            }
        }

        if (s_singletonTotal >= CALIB_TARGET && s_classTotal >= CALIB_TARGET)
            ApplyCalibrationResult(false);

        if (isSingleton) {
            uintptr_t tmp        = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset);
            uintptr_t second_tmp = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset + 0x04);
            if (tmp) SingletonFunctions[global + "." + field] = {tmp, second_tmp};
            FILE* fh = fopen("C:\\easybot_hooks.log", "a");
            if (fh) {
                fprintf(fh, "BIND(calib): %s.%s  func=0x%08X  this=0x%08X\n",
                    global.c_str(), field.c_str(), (unsigned)tmp, (unsigned)second_tmp);
                fclose(fh);
            }
        }

        typedef void(__cdecl* orig_t)(uintptr_t, uintptr_t, uintptr_t);
        reinterpret_cast<orig_t>(original_bindSingletonFunction)(a1, a2, a3);
        return;
    }

    // Calibration complete: capture with correct offsets.
    uintptr_t tmp, second_tmp;
    if (!isSingleton) {
        tmp = *reinterpret_cast<uintptr_t*>(ebp + classFunctionOffset);
        if (tmp) ClassMemberFunctions[global + "." + field] = tmp;
    } else {
        tmp        = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset);
        second_tmp = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset + 0x04);
        if (tmp) SingletonFunctions[global + "." + field] = {tmp, second_tmp};
    }

    FILE* fh = fopen("C:\\easybot_hooks.log", "a");
    if (fh) {
        if (!isSingleton)
            fprintf(fh, "CLASS: %s.%s  func=0x%08X\n",
                global.c_str(), field.c_str(), (unsigned)tmp);
        else
            fprintf(fh, "BIND:  %s.%s  func=0x%08X  this=0x%08X\n",
                global.c_str(), field.c_str(), (unsigned)tmp, (unsigned)second_tmp);
        fclose(fh);
    }

    typedef void(__cdecl* orig_t)(uintptr_t, uintptr_t, uintptr_t);
    reinterpret_cast<orig_t>(original_bindSingletonFunction)(a1, a2, a3);
}
#pragma optimize("", on)

void __stdcall hooked_callLuaField(uintptr_t* a1) {
    original_callLuaField(a1);
}

// ============================================================================
// hooked_callGlobalField
// ============================================================================

inline uint32_t itemId;

#pragma optimize( "", off )
void __stdcall hooked_callGlobalField(uintptr_t **a1, uintptr_t **a2) {
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    uintptr_t ebp = ctx.Ebp;
    auto global = *reinterpret_cast<std::string*>(a1);
    auto field = *reinterpret_cast<std::string*>(a2);
    if (global == "g_game") {
        if (field == "onTextMessage") {
            uintptr_t onTextMessage_address = ebp + onTextMessageOffset;
            auto ptr_messageText = g_custom->getMessagePtr(onTextMessage_address);
            auto message_address = reinterpret_cast<std::string*>(ptr_messageText);
            if (message_address->find("You see") != std::string::npos)
            {
                *message_address = "ID: " + std::to_string(itemId) + "\n" + *reinterpret_cast<std::string*>(ptr_messageText);
            }
        } else if (field == "onTalk") {
            auto args = reinterpret_cast<StackArgs*>(ebp + onTalkOffset);
            g_custom->onTalk(
                *args->name,
                reinterpret_cast<uint16_t>(args->level),
                *args->mode,
                *args->text,
                *args->channelId,
                *args->pos
            );
        }
    }
    original_callGlobalField(a1, a2);
}
#pragma optimize( "", on )

// cdecl variant — same logic, caller cleans the stack.
#pragma optimize( "", off )
void __cdecl hooked_callGlobalField_cdecl(uintptr_t **a1, uintptr_t **a2) {
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    uintptr_t ebp = ctx.Ebp;
    auto global = *reinterpret_cast<std::string*>(a1);
    auto field  = *reinterpret_cast<std::string*>(a2);
    if (global == "g_game") {
        if (field == "onTextMessage") {
            uintptr_t onTextMessage_address = ebp + onTextMessageOffset;
            auto ptr_messageText = g_custom->getMessagePtr(onTextMessage_address);
            auto message_address = reinterpret_cast<std::string*>(ptr_messageText);
            if (message_address->find("You see") != std::string::npos)
                *message_address = "ID: " + std::to_string(itemId) + "\n" + *reinterpret_cast<std::string*>(ptr_messageText);
        } else if (field == "onTalk") {
            auto args = reinterpret_cast<StackArgs*>(ebp + onTalkOffset);
            g_custom->onTalk(
                *args->name,
                reinterpret_cast<uint16_t>(args->level),
                *args->mode,
                *args->text,
                *args->channelId,
                *args->pos
            );
        }
    }
    typedef void(__cdecl* orig_t)(uintptr_t**, uintptr_t**);
    reinterpret_cast<orig_t>(original_callGlobalField)(a1, a2);
}
#pragma optimize( "", on )

// ============================================================================
// hooked_MainLoop
// ============================================================================
int hooked_MainLoop(int a1) {
    g_dispatcher->executeEvent();
    return original_mainLoop(a1);
}

// ============================================================================
// hooked_Look
// ============================================================================
typedef uint32_t(gameCall* GetId)(uintptr_t RCX, void* RDX);

// Dump the first 16 bytes of an object to the init log.
// Kept in a separate function with NO C++ objects so __try/__except is valid
// (MSVC C2712: __try cannot coexist with object unwinding in the same scope).
static void SafeDumpMemory(uintptr_t addr) {
    FILE* f = fopen("C:\\easybot_init.log", "a");
    if (!f) return;
    fprintf(f, "hooked_Look: thing=0x%08X  first 64 bytes:\n", (unsigned)addr);
    // Print 64 bytes as hex, 16 per line.
    for (int row = 0; row < 4; ++row) {
        fprintf(f, "  [+%02X]", row * 16);
        for (int col = 0; col < 16; ++col) {
            uint8_t byte = 0;
            __try { byte = *reinterpret_cast<const uint8_t*>(addr + row * 16 + col); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            fprintf(f, " %02X", byte);
        }
        fprintf(f, "\n");
    }
    // Print the 16 uint32 words for easier vtable/field identification.
    fprintf(f, "  as uint32:");
    for (int i = 0; i < 16; ++i) {
        uint32_t word = 0;
        __try { word = *reinterpret_cast<const uint32_t*>(addr + (uint32_t)i * 4); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        fprintf(f, " [+%02X]=0x%08X", i * 4, word);
        if (i % 4 == 3) fprintf(f, "\n          ");
    }
    fprintf(f, "\n");
    fclose(f);
}

void __stdcall hooked_Look(const uintptr_t& thing, const bool isBattleList) {
    if (look_original) look_original(&thing, isBattleList);

    // Log the first 20 Look calls: thing pointer + getId result every time;
    // full hex dump (SafeDumpMemory) only for the first 5 to avoid log spam.
    static int s_diagCount = 0;
    if (thing && s_diagCount < 20) {
        ++s_diagCount;

        // Full memory layout dump for the first 5 calls only.
        if (s_diagCount <= 5) SafeDumpMemory(thing);

        // Map diagnostic: print the raw getId address, map size, and every key
        // containing "getId".  Repeats until calibration is locked so we can
        // see exactly which call first has a populated map.
        if (!g_classCalibrationLocked || s_diagCount == 1) {
            FILE* fd = fopen("C:\\easybot_init.log", "a");
            if (fd) {
                uintptr_t rawAddr = ClassMemberFunctions.count("Thing.getId")
                    ? ClassMemberFunctions.at("Thing.getId") : 0;
                fprintf(fd,
                    "hooked_Look[%d] MAP DIAG: Thing.getId=0x%08X  mapSize=%d"
                    "  calibLocked=%d\n",
                    s_diagCount, (unsigned)rawAddr,
                    (int)ClassMemberFunctions.size(),
                    (int)g_classCalibrationLocked);
                for (auto& kv : ClassMemberFunctions) {
                    if (kv.first.find("getId") != std::string::npos)
                        fprintf(fd, "  getId key: [%s] = 0x%08X\n",
                            kv.first.c_str(), (unsigned)kv.second);
                }
                fclose(fd);
            }
        }

        // Call getId directly to verify the pointer and offset are correct.
        GetId getIdFn = reinterpret_cast<GetId>(ClassMemberFunctions["Thing.getId"]);
        void* pDiagMystery = nullptr;
        uint32_t diagId = getIdFn ? getIdFn(thing, &pDiagMystery) : 0xDEAD;

        uintptr_t lp    = ReadLocalPlayerPtr();
        uintptr_t deref = tryReadPtr(thing);

        FILE* f = fopen("C:\\easybot_init.log", "a");
        if (f) {
            fprintf(f, "hooked_Look[%d]: thing=0x%08X  *thing=0x%08X  "
                       "getId=%u  localPlayer=0x%08X\n",
                s_diagCount, (unsigned)thing, (unsigned)deref,
                diagId, (unsigned)lp);
            fclose(f);
        }
    }

    if (g_isLuaWrapperServer) return;  // can't call class member functions
    auto function = reinterpret_cast<GetId>(ClassMemberFunctions["Thing.getId"]);
    if (!function) return;
    void* pMysteryPtr = nullptr;
    itemId = function(thing, &pMysteryPtr);
}
