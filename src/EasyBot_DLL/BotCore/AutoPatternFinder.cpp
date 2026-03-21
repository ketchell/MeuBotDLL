#include "AutoPatternFinder.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

// ============================================================================
// Internal types
// ============================================================================

struct SectionRange {
    uintptr_t base;
    uint32_t  size;
    bool      valid;
};

// ============================================================================
// Logging helpers
// ============================================================================

static void afLog(FILE* f, const char* msg) {
    if (f) { fputs(msg, f); fputc('\n', f); fflush(f); }
}

static void afLogFmt(FILE* f, const char* fmt, ...) {
    if (!f) return;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    afLog(f, buf);
}

// ============================================================================
// PE section parser
// ============================================================================

static bool GetPESections(uintptr_t modBase,
                          SectionRange* outText,
                          SectionRange* outRdata) {
    outText->valid  = false;
    outRdata->valid = false;

    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(modBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;

    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(modBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

    IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    WORD n = nt->FileHeader.NumberOfSections;

    for (WORD i = 0; i < n; i++) {
        char name[9] = {};
        memcpy(name, sec[i].Name, 8);
        uintptr_t secBase = modBase + sec[i].VirtualAddress;
        uint32_t  secSize = sec[i].Misc.VirtualSize;

        if (strcmp(name, ".text") == 0) {
            outText->base  = secBase;
            outText->size  = secSize;
            outText->valid = true;
        } else if (strcmp(name, ".rdata") == 0) {
            outRdata->base  = secBase;
            outRdata->size  = secSize;
            outRdata->valid = true;
        }
    }

    return outText->valid;
}

// ============================================================================
// CountCallers
//
// Count all E8 rel32 instructions in .text that resolve to targetAddr.
// No C++ objects — __try is valid.
// ============================================================================

static int CountCallers(uintptr_t textBase, uint32_t textSize, uintptr_t targetAddr) {
    int count = 0;
    __try {
        uintptr_t end = textBase + textSize - 5;
        for (uintptr_t p = textBase; p < end; ++p) {
            if (*reinterpret_cast<const uint8_t*>(p) == 0xE8) {
                int32_t   rel    = *reinterpret_cast<const int32_t*>(p + 1);
                uintptr_t target = p + 5 + static_cast<uintptr_t>(static_cast<intptr_t>(rel));
                if (target == targetAddr) ++count;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return count;
}

// ============================================================================
// FindFunctionStartBefore
//
// Walk backward from 'addr' (a mid-function match site) to locate the
// function prologue.  Stops when it finds CC (int3), C3 (ret), or C2 (ret N)
// — the end of the preceding function.  Returns the first non-padding byte
// after the boundary, i.e. the function start.
// No C++ objects — __try is valid.
// ============================================================================

static uintptr_t FindFunctionStartBefore(uintptr_t addr,
                                         uintptr_t textBase,
                                         uint32_t  maxLookback) {
    if (addr <= textBase) return 0;
    uintptr_t limit = (addr - textBase > maxLookback) ? addr - maxLookback : textBase;

    __try {
        for (uintptr_t p = addr - 1; p >= limit; --p) {
            uint8_t b = *reinterpret_cast<const uint8_t*>(p);

            uintptr_t boundary = 0;
            if (b == 0xCC || b == 0xC3)
                boundary = p + 1;
            else if (b == 0xC2 && p + 3 <= addr)  // ret N  (2-byte operand)
                boundary = p + 3;

            if (boundary) {
                // Skip any int3 / nop padding between the boundary and real start
                while (boundary < addr) {
                    uint8_t fb = *reinterpret_cast<const uint8_t*>(boundary);
                    if (fb != 0xCC && fb != 0x90) break;
                    ++boundary;
                }
                return boundary;
            }

            if (p == limit) break;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}

// ============================================================================
// ScanBySSO_Tail  (Method A — primary)
//
// Scan .text for the 9-byte SSO prologue tail at offset +5:
//   08 83 78 14 10 72 02 8B 00
//
// Works on compilers where the std::string SSO check uses exactly these bytes
// (MSVC / libstdc++ with threshold=16, jb short).  Confirmed on Realera.
//
// Returns the number of function starts written into out[] (max outCap).
// ============================================================================

static const unsigned char SSO_TAIL[]   = { 0x08, 0x83, 0x78, 0x14, 0x10, 0x72, 0x02, 0x8B, 0x00 };
static const int           SSO_TAIL_LEN = 9;

static int ScanBySSO_Tail(uintptr_t  textBase,
                          uint32_t   textSize,
                          uintptr_t* out,
                          int        outCap,
                          FILE*      f) {
    afLog(f, "  Method A (SSO tail): scanning for 08 83 78 14 10 72 02 8B 00");
    int count = 0;

    __try {
        uintptr_t end = textBase + textSize - SSO_TAIL_LEN;
        for (uintptr_t addr = textBase + 5; addr < end; ++addr) {
            if (memcmp(reinterpret_cast<const void*>(addr), SSO_TAIL, SSO_TAIL_LEN) != 0)
                continue;

            uintptr_t funcStart = addr - 5;
            uint8_t   firstByte = *reinterpret_cast<const uint8_t*>(funcStart);
            if (firstByte != 0x55 && firstByte != 0xE9)
                continue;

            afLogFmt(f, "    tail match at 0x%08X  funcStart=0x%08X  firstByte=0x%02X (%s)",
                (unsigned)addr, (unsigned)funcStart, firstByte,
                firstByte == 0x55 ? "original" : "MinHook detour");

            if (count < outCap) out[count++] = funcStart;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        afLog(f, "  Method A: EXCEPTION during scan");
    }

    afLogFmt(f, "  Method A: %d match(es)", count);
    return count;
}

// ============================================================================
// ScanByLuaRegPush  (Method B — fallback)
//
// Scan .text for the 7-byte sequence:
//   68 EE D8 FF FF FF 35
//   (push LUA_REGISTRYINDEX=-10002  /  push [mem])
//
// This sequence appears in both bindSingletonFunction and callGlobalField on
// all tested OTClient builds regardless of compiler/STL variant.  For each
// match, walk backward up to 256 bytes to find the function start (first byte
// after a CC/C3/C2 boundary).
//
// Returns the number of function starts written into out[] (max outCap).
// ============================================================================

static const unsigned char LUA_REG_PUSH[]   = { 0x68, 0xEE, 0xD8, 0xFF, 0xFF, 0xFF, 0x35 };
static const int           LUA_REG_PUSH_LEN = 7;

static int ScanByLuaRegPush(uintptr_t  textBase,
                            uint32_t   textSize,
                            uintptr_t* out,
                            int        outCap,
                            FILE*      f) {
    afLog(f, "  Method B (LuaRegPush): scanning for 68 EE D8 FF FF FF 35");
    int count = 0;

    __try {
        uintptr_t end = textBase + textSize - LUA_REG_PUSH_LEN;
        for (uintptr_t addr = textBase; addr < end; ++addr) {
            if (memcmp(reinterpret_cast<const void*>(addr), LUA_REG_PUSH, LUA_REG_PUSH_LEN) != 0)
                continue;

            afLogFmt(f, "    pattern match at 0x%08X — walking back for function start", (unsigned)addr);

            uintptr_t funcStart = FindFunctionStartBefore(addr, textBase, 256);
            if (!funcStart) {
                afLogFmt(f, "      could not find function start (no boundary in 256 bytes)");
                continue;
            }

            // Deduplicate — same function may contain the pattern more than once
            bool seen = false;
            for (int i = 0; i < count; ++i) if (out[i] == funcStart) { seen = true; break; }
            if (seen) {
                afLogFmt(f, "      funcStart=0x%08X (duplicate, skipped)", (unsigned)funcStart);
                continue;
            }

            uint8_t firstByte = *reinterpret_cast<const uint8_t*>(funcStart);
            afLogFmt(f, "      funcStart=0x%08X  firstByte=0x%02X", (unsigned)funcStart, firstByte);

            if (count < outCap) out[count++] = funcStart;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        afLog(f, "  Method B: EXCEPTION during scan");
    }

    afLogFmt(f, "  Method B: %d unique function start(s)", count);
    return count;
}

// ============================================================================
// AssignByCallerCount
//
// Given an array of candidate function starts, count callers for each and
// assign: most callers = bindSingletonFunction, second = callGlobalField.
// ============================================================================

static void AssignByCallerCount(uintptr_t* funcs,
                                int        count,
                                uintptr_t  textBase,
                                uint32_t   textSize,
                                uintptr_t* outBind,
                                uintptr_t* outCall,
                                FILE*      f) {
    *outBind = 0;
    *outCall = 0;
    int bestCount   = 0;
    int secondCount = 0;

    afLog(f, "  Caller counts:");
    for (int i = 0; i < count; ++i) {
        int c = CountCallers(textBase, textSize, funcs[i]);
        afLogFmt(f, "    0x%08X  callers=%d", (unsigned)funcs[i], c);

        if (c > bestCount) {
            *outCall    = *outBind;   secondCount = bestCount;
            *outBind    = funcs[i];   bestCount   = c;
        } else if (c > secondCount) {
            *outCall    = funcs[i];   secondCount = c;
        }
    }

    afLogFmt(f, "  -> bindSingletonFunction: 0x%08X (%d callers)", (unsigned)*outBind,  bestCount);
    afLogFmt(f, "  -> callGlobalField:       0x%08X (%d callers)", (unsigned)*outCall, secondCount);
}

// ============================================================================
// AutoFindHookTargets  — public entry point
// ============================================================================

AutoFinderResult AutoFindHookTargets() {
    AutoFinderResult result = {};
    result.mainLoop = 0;

    FILE* f = fopen("C:\\easybot_autofind.log", "w");
    afLog(f, "AutoFindHookTargets: starting");

    uintptr_t modBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
    afLogFmt(f, "Module base: 0x%08X", (unsigned)modBase);

    SectionRange textSec  = {};
    SectionRange rdataSec = {};
    if (!GetPESections(modBase, &textSec, &rdataSec)) {
        strncpy_s(result.errorMsg, sizeof(result.errorMsg),
            "Failed to parse PE sections (.text not found)", _TRUNCATE);
        afLog(f, result.errorMsg);
        if (f) fclose(f);
        return result;
    }

    afLogFmt(f, ".text:  base=0x%08X  size=0x%X", (unsigned)textSec.base, textSec.size);

    uintptr_t matches[64];
    int       matchCount = 0;

    // ------------------------------------------------------------------
    // Method A: SSO tail scan (fast, compiler-specific)
    // ------------------------------------------------------------------
    afLog(f, "");
    afLog(f, "--- Method A: SSO tail ---");
    matchCount = ScanBySSO_Tail(textSec.base, textSec.size, matches, 64, f);

    if (matchCount >= 2) {
        afLog(f, "Method A succeeded — assigning by caller count");
        AssignByCallerCount(matches, matchCount,
            textSec.base, textSec.size,
            &result.bindSingletonFunction, &result.callGlobalField, f);
    } else {
        afLogFmt(f, "Method A insufficient (%d match(es)) — trying Method B", matchCount);

        // ------------------------------------------------------------------
        // Method B: LuaRegPush scan (universal)
        // ------------------------------------------------------------------
        afLog(f, "");
        afLog(f, "--- Method B: LuaRegPush ---");
        matchCount = ScanByLuaRegPush(textSec.base, textSec.size, matches, 64, f);

        if (matchCount >= 2) {
            afLog(f, "Method B succeeded — assigning by caller count");
            AssignByCallerCount(matches, matchCount,
                textSec.base, textSec.size,
                &result.bindSingletonFunction, &result.callGlobalField, f);
        } else {
            afLogFmt(f, "Method B insufficient (%d match(es)) — both methods failed", matchCount);
        }
    }

    result.success = (result.bindSingletonFunction != 0) && (result.callGlobalField != 0);

    afLog(f, "");
    afLogFmt(f,
        "Result: bindSingleton=0x%08X  callGlobalField=0x%08X  mainLoop=0  success=%d",
        (unsigned)result.bindSingletonFunction,
        (unsigned)result.callGlobalField,
        (int)result.success);

    if (!result.success) {
        _snprintf_s(result.errorMsg, sizeof(result.errorMsg), _TRUNCATE,
            "AutoFind failed: bindSingleton=%s  callGlobalField=%s  methodA=%d  methodB=%d matches",
            result.bindSingletonFunction ? "OK" : "MISSING",
            result.callGlobalField       ? "OK" : "MISSING",
            matchCount, matchCount);
        afLog(f, result.errorMsg);
    }

    if (f) fclose(f);
    return result;
}
