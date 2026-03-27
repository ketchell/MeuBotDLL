#include "hooks.h"
#include "BuildConfig.h"
#include "Creature.h"
#include "CustomFunctions.h"
#include "pattern_scan.h"

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
static const int   CALIB_SLOTS  = 22;   // scan EBP+0x08 .. EBP+0x08+(21*4)
static const int   CALIB_TARGET = 15;   // need this many valid samples each

static int s_singletonHits[CALIB_SLOTS] = {};
static int s_classHits[CALIB_SLOTS]     = {};
static int s_singletonTotal = 0;
static int s_classTotal     = 0;

// Shared counter for diagnostic log numbering across both hook variants.
static int s_calCount = 0;

// Buffer for class member calls that arrive during calibration.
// Stores values at every candidate offset so we can pick the right one after calibration.
struct CalibrationEntry {
    char      key[128];
    uintptr_t values[CALIB_SLOTS];  // value at ebp+0x08+i*4 for i in [0, CALIB_SLOTS)
};
static CalibrationEntry s_classBuffer[1024];
static int              s_classBufferCount = 0;

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

        int* hits  = isSingleton ? s_singletonHits : s_classHits;
        int& total = isSingleton ? s_singletonTotal : s_classTotal;
        for (int i = 0; i < CALIB_SLOTS; ++i) {
            uintptr_t val = *reinterpret_cast<uintptr_t*>(ebp + 0x08 + i * 4);
            if (IsInText(val)) ++hits[i];
        }
        ++total;

        // Buffer class member calls during calibration for later replay.
        if (!isSingleton && s_classBufferCount < 1024) {
            snprintf(s_classBuffer[s_classBufferCount].key, 128, "%s.%s", global.c_str(), field.c_str());
            for (int i = 0; i < CALIB_SLOTS; ++i)
                s_classBuffer[s_classBufferCount].values[i] = tryReadPtr(ebp + 0x08 + i * 4);
            ++s_classBufferCount;
        }

        // Dump the first 3 calls so we can see raw EBP content in the log.
        if (s_calCount <= 3) {
            FILE* f = fopen("C:\\easybot_calibrate.log", "a");
            if (f) {
                fprintf(f, "call #%d (stdcall) %s.%s  ebp=0x%08X  isSingleton=%d  text=[0x%08X,0x%08X)\n",
                    s_calCount, global.c_str(), field.c_str(),
                    (unsigned)ebp, (int)isSingleton,
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

        if (s_singletonTotal >= CALIB_TARGET && s_classTotal >= CALIB_TARGET) {
            int bestSI = 0, bestCI = 0;
            for (int i = 1; i < CALIB_SLOTS; ++i) {
                if (s_singletonHits[i] >= s_singletonHits[bestSI]) bestSI = i;
                if (s_classHits[i]     >= s_classHits[bestCI])     bestCI = i;
            }
            if (s_singletonHits[bestSI] > 0) singletonFunctionOffset = 0x08 + bestSI * 4;
            if (s_classHits[bestCI]     > 0) classFunctionOffset     = 0x08 + bestCI * 4;
            g_offsetsCalibrated = true;

            // Replay buffered class members with the now-known classFunctionOffset.
            {
                int slotIdx = (classFunctionOffset - 0x08) / 4;
                FILE* fh = fopen("C:\\easybot_hooks.log", "a");
                for (int i = 0; i < s_classBufferCount; ++i) {
                    uintptr_t fp = s_classBuffer[i].values[slotIdx];
                    if (fp) ClassMemberFunctions[s_classBuffer[i].key] = fp;
                    if (fh) fprintf(fh, "CLASS(replay): %s func=0x%08X\n", s_classBuffer[i].key, (unsigned)fp);
                }
                if (fh) {
                    fprintf(fh, "Replayed %d class members (offset=0x%02X)\n",
                        s_classBufferCount, classFunctionOffset);
                    fclose(fh);
                }
            }

            FILE* f = fopen("C:\\easybot_calibrate.log", "a");
            if (f) {
                fprintf(f, "\n=== CALIBRATION COMPLETE ===\n");
                fprintf(f, "singletonFunctionOffset = 0x%02X  (hits=%d/%d)\n",
                    singletonFunctionOffset, s_singletonHits[bestSI], s_singletonTotal);
                fprintf(f, "classFunctionOffset     = 0x%02X  (hits=%d/%d)\n",
                    classFunctionOffset, s_classHits[bestCI], s_classTotal);
                fprintf(f, "singleton histogram:");
                for (int i = 0; i < CALIB_SLOTS; ++i) fprintf(f, " %d", s_singletonHits[i]);
                fprintf(f, "\nclass histogram:   ");
                for (int i = 0; i < CALIB_SLOTS; ++i) fprintf(f, " %d", s_classHits[i]);
                fprintf(f, "\nReplayed %d buffered class members with offset 0x%02X\n",
                    s_classBufferCount, classFunctionOffset);
                fclose(f);
            }
        }

        // Capture singletons now — singletonFunctionOffset=0x10 is correct on all servers.
        // Skip class members — classFunctionOffset not calibrated yet.
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

        int* hits  = isSingleton ? s_singletonHits : s_classHits;
        int& total = isSingleton ? s_singletonTotal : s_classTotal;
        for (int i = 0; i < CALIB_SLOTS; ++i) {
            uintptr_t val = *reinterpret_cast<uintptr_t*>(ebp + 0x08 + i * 4);
            if (IsInText(val)) ++hits[i];
        }
        ++total;

        // Buffer class member calls during calibration for later replay.
        if (!isSingleton && s_classBufferCount < 1024) {
            snprintf(s_classBuffer[s_classBufferCount].key, 128, "%s.%s", global.c_str(), field.c_str());
            for (int i = 0; i < CALIB_SLOTS; ++i)
                s_classBuffer[s_classBufferCount].values[i] = tryReadPtr(ebp + 0x08 + i * 4);
            ++s_classBufferCount;
        }

        if (s_calCount <= 3) {
            FILE* f = fopen("C:\\easybot_calibrate.log", "a");
            if (f) {
                fprintf(f, "call #%d (cdecl)  %s.%s  ebp=0x%08X  isSingleton=%d  text=[0x%08X,0x%08X)\n",
                    s_calCount, global.c_str(), field.c_str(),
                    (unsigned)ebp, (int)isSingleton,
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

        if (s_singletonTotal >= CALIB_TARGET && s_classTotal >= CALIB_TARGET) {
            int bestSI = 0, bestCI = 0;
            for (int i = 1; i < CALIB_SLOTS; ++i) {
                if (s_singletonHits[i] >= s_singletonHits[bestSI]) bestSI = i;
                if (s_classHits[i]     >= s_classHits[bestCI])     bestCI = i;
            }
            if (s_singletonHits[bestSI] > 0) singletonFunctionOffset = 0x08 + bestSI * 4;
            if (s_classHits[bestCI]     > 0) classFunctionOffset     = 0x08 + bestCI * 4;
            g_offsetsCalibrated = true;

            // Replay buffered class members with the now-known classFunctionOffset.
            {
                int slotIdx = (classFunctionOffset - 0x08) / 4;
                FILE* fh = fopen("C:\\easybot_hooks.log", "a");
                for (int i = 0; i < s_classBufferCount; ++i) {
                    uintptr_t fp = s_classBuffer[i].values[slotIdx];
                    if (fp) ClassMemberFunctions[s_classBuffer[i].key] = fp;
                    if (fh) fprintf(fh, "CLASS(replay): %s func=0x%08X\n", s_classBuffer[i].key, (unsigned)fp);
                }
                if (fh) {
                    fprintf(fh, "Replayed %d class members (offset=0x%02X)\n",
                        s_classBufferCount, classFunctionOffset);
                    fclose(fh);
                }
            }

            FILE* f = fopen("C:\\easybot_calibrate.log", "a");
            if (f) {
                fprintf(f, "\n=== CALIBRATION COMPLETE ===\n");
                fprintf(f, "singletonFunctionOffset = 0x%02X  (hits=%d/%d)\n",
                    singletonFunctionOffset, s_singletonHits[bestSI], s_singletonTotal);
                fprintf(f, "classFunctionOffset     = 0x%02X  (hits=%d/%d)\n",
                    classFunctionOffset, s_classHits[bestCI], s_classTotal);
                fprintf(f, "singleton histogram:");
                for (int i = 0; i < CALIB_SLOTS; ++i) fprintf(f, " %d", s_singletonHits[i]);
                fprintf(f, "\nclass histogram:   ");
                for (int i = 0; i < CALIB_SLOTS; ++i) fprintf(f, " %d", s_classHits[i]);
                fprintf(f, "\nReplayed %d buffered class members with offset 0x%02X\n",
                    s_classBufferCount, classFunctionOffset);
                fclose(f);
            }
        }

        // Capture singletons now — singletonFunctionOffset=0x10 is correct on all servers.
        // Skip class members — classFunctionOffset not calibrated yet.
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

void __stdcall hooked_Look(const uintptr_t& thing, const bool isBattleList) {
    if (look_original) look_original(&thing, isBattleList);
    if (g_isLuaWrapperServer) return;  // can't call class member functions
    auto function = reinterpret_cast<GetId>(ClassMemberFunctions["Item.getId"]);
    if (!function) return;
    void* pMysteryPtr = nullptr;
    itemId = function(thing, &pMysteryPtr);
}
