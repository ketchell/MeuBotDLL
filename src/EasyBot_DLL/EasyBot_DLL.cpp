#include "../../proto_functions_server.h"
#include "pattern_scan.h"
#include "MinHook.h"
#include "hooks.h"
#include "Game.h"
#include "LocalPlayer.h"
#include "Thing.h"
#include "Container.h"
#include "Item.h"
#include "AutoPatternFinder.h"
#include "offsets_global.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime> // Added for seeding

HMODULE g_hDll = NULL;

// ============================================================================
// EasyBot DLL entry point
// ============================================================================

static uint32_t CalcChecksum(uintptr_t base, uintptr_t size) {
    uint32_t sum = 0;
    uint32_t n = (size > 4096) ? 4096 : static_cast<uint32_t>(size);
    for (uint32_t i = 0; i < n; i++)
        sum += *reinterpret_cast<const unsigned char*>(base + i);
    return sum;
}

static bool ReadCache(uintptr_t moduleBase, uintptr_t moduleSize,
    uintptr_t* outBind, uintptr_t* outCall) {
    char path[MAX_PATH];
    // Path redirected to generic LocalAppData cache
    snprintf(path, sizeof(path), "C:\\easybot_cache_%08X.dat", (unsigned)moduleBase);
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    uint32_t buf[6] = { 0 };
    bool ok = (fread(buf, sizeof(buf), 1, f) == 1);
    fclose(f);
    if (!ok) return false;
    if (buf[0] != 0xEA5B0700u) return false;
    if (buf[1] != (uint32_t)moduleBase) return false;
    if (buf[2] != (uint32_t)moduleSize) return false;
    if (buf[3] != CalcChecksum(moduleBase, moduleSize)) return false;
    if (!buf[4]) return false;
    *outBind = buf[4];
    *outCall = buf[5];
    return true;
}

static void WriteCache(uintptr_t moduleBase, uintptr_t moduleSize,
    uintptr_t bind, uintptr_t call) {
    uint32_t buf[6];
    buf[0] = 0xEA5B0700u;
    buf[1] = (uint32_t)moduleBase;
    buf[2] = (uint32_t)moduleSize;
    buf[3] = CalcChecksum(moduleBase, moduleSize);
    buf[4] = (uint32_t)bind;
    buf[5] = (uint32_t)call;
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "C:\\easybot_cache_%08X.dat", (unsigned)moduleBase);
    FILE* f = fopen(path, "wb");
    if (f) { fwrite(buf, sizeof(buf), 1, f); fclose(f); }
}

// ============================================================================
// Main Application Thread
// ============================================================================
DWORD WINAPI EasyBotInit(LPVOID /*pParam*/) {
    // SEEDING: Initialize unique entropy for this specific process instance
    srand(static_cast<unsigned int>(time(NULL)) ^ GetCurrentProcessId());

    // Clean up stale metadata from previous instances
    {
        char dllDir[MAX_PATH];
        if (GetModuleFileNameA(g_hDll, dllDir, MAX_PATH)) {
            char* sl = strrchr(dllDir, '\\');
            if (sl) {
                *(sl + 1) = '\0';
                char pf[MAX_PATH];
                strcpy_s(pf, dllDir);
                strcat_s(pf, "easybot_port.txt");
                DeleteFileA(pf);
            }
        }
    }

    FILE* f = fopen("C:\\easybot_init.log", "w");
    if (f) { fprintf(f, "EasyBot starting\n"); fclose(f); }

    auto iLog = [](const char* msg) {
        FILE* f2 = fopen("C:\\easybot_init.log", "a");
        if (f2) { fprintf(f2, "%s\n", msg); fclose(f2); }
        };

    // ---- Window enumeration diagnostic ----
    // Log every top-level window (hwnd, visible, class name, title) so we can
    // identify what class name the game client actually uses on this build.
    struct WndEnum {
        static BOOL CALLBACK Proc(HWND hwnd, LPARAM /*lp*/) {
            char cls[256]   = "<no class>";
            char title[256] = "<no title>";
            GetClassNameA(hwnd, cls, sizeof(cls));
            GetWindowTextA(hwnd, title, sizeof(title));
            BOOL visible = IsWindowVisible(hwnd);
            char line[640];
            wsprintfA(line,
                "  HWND=0x%p  visible=%d  class=\"%s\"  title=\"%s\"",
                hwnd, (int)visible, cls, title);
            FILE* fl = fopen("C:\\easybot_init.log", "a");
            if (fl) { fprintf(fl, "%s\n", line); fclose(fl); }
            return TRUE; // continue
        }
    };
    iLog("--- EnumWindows: all top-level windows at inject time ---");
    EnumWindows(WndEnum::Proc, 0);
    iLog("--- EnumWindows: done ---");

    auto isStdcall = [](uintptr_t addr) -> bool {
        return addr && (*reinterpret_cast<const unsigned char*>(addr) == 0x55);
        };

    auto hookBind = [&](uintptr_t addr) -> MH_STATUS {
        if (!addr) return MH_ERROR_NOT_EXECUTABLE;
        bool stdcall_ = isStdcall(addr);
        LPVOID fn = stdcall_
            ? reinterpret_cast<LPVOID>(&hooked_bindSingletonFunction)
            : reinterpret_cast<LPVOID>(&hooked_bindSingletonFunction_cdecl);
        return MH_CreateHook(reinterpret_cast<LPVOID>(addr), fn,
            reinterpret_cast<LPVOID*>(&original_bindSingletonFunction));
        };

    auto hookCall = [&](uintptr_t addr) -> MH_STATUS {
        if (!addr) return MH_ERROR_NOT_EXECUTABLE;
        bool stdcall_ = isStdcall(addr);
        LPVOID fn = stdcall_
            ? reinterpret_cast<LPVOID>(&hooked_callGlobalField)
            : reinterpret_cast<LPVOID>(&hooked_callGlobalField_cdecl);
        return MH_CreateHook(reinterpret_cast<LPVOID>(addr), fn,
            reinterpret_cast<LPVOID*>(&original_callGlobalField));
        };

    // ---- Step 1: Initialize hooks and memory mapping ----
    MH_Initialize();
    InitTextRange();

    uintptr_t modBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(modBase);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(modBase + dos->e_lfanew);
    uintptr_t modSize = nt->OptionalHeader.SizeOfImage;

    // ---- Step 2: Address Resolution ----
    uintptr_t bind_func = FindPattern(bindSingletonFunction_x86_PATTERN, bindSingletonFunction_x86_MASK);
    uintptr_t call_func = FindPattern(callGlobalField_PATTERN, callGlobalField_MASK);
    uintptr_t main_func = FindPattern(mainLoop_x86_PATTERN, mainLoop_x86_MASK);

    if (!main_func) {
        iLog("CRITICAL: Component mismatch - aborting");
        return 0;
    }

    // Cache retrieval for optimized startup
    if (!bind_func || !call_func) {
        uintptr_t cacheBind = 0, cacheCall = 0;
        if (ReadCache(modBase, modSize, &cacheBind, &cacheCall)) {
            if (!bind_func) bind_func = cacheBind;
            if (!call_func) call_func = cacheCall;
        }
        else {
            AutoFinderResult ar = AutoFindHookTargets();
            if (ar.bindSingletonFunction || ar.callGlobalField)
                WriteCache(modBase, modSize, ar.bindSingletonFunction, ar.callGlobalField);
            if (!bind_func && ar.bindSingletonFunction) bind_func = ar.bindSingletonFunction;
            if (!call_func && ar.callGlobalField)       call_func = ar.callGlobalField;
        }
    }

    // ---- Step 3: Deployment ----
    hookBind(bind_func);
    if (call_func && isStdcall(call_func)) {
        hookCall(call_func);
    }

    MH_STATUS r = MH_CreateHook(reinterpret_cast<LPVOID>(main_func),
        reinterpret_cast<LPVOID>(&hooked_MainLoop),
        reinterpret_cast<LPVOID*>(&original_mainLoop));

    if (r != MH_OK) return 0;

    MH_EnableHook(MH_ALL_HOOKS);

    // ---- Step 4: Calibration Wait (2 s timeout, then force with available samples) ----
    // Wait up to 1 s for natural class bindings to arrive first.
    for (int i = 0; i < 100 && !g_offsetsCalibrated; ++i) Sleep(10);

    // If still uncalibrated, inject a synthetic sample: call getLocalPlayer() and
    // then getId() on the result.  These dispatch through scheduleEventEx, which
    // fires on the next main-loop tick and may trigger lazy Lua binding calls for
// The LocalPlayer/Thing class — giving the histogram a real sample to read.
    if (!g_offsetsCalibrated) {
        iLog("Calibration sample inject: triggering Thing::getId via internal map");

        uintptr_t lp = g_game->getLocalPlayer();
        {
            char msg[128];
            snprintf(msg, sizeof(msg),
                "  localPlayer ptr=0x%08X  isOnline=%d",
                (unsigned)lp, (int)g_game->isOnline());
            iLog(msg);
        }

        if (lp && ClassMemberFunctions.count("Thing.getId")) {
            uintptr_t fnAddr = ClassMemberFunctions["Thing.getId"];
            // Call via explicit __fastcall cast — bypasses the gameCall macro and
            // avoids C2227/E3364 since lp is a uintptr_t, not a class pointer.
            uint32_t probeId = reinterpret_cast<uint32_t(__fastcall*)(uintptr_t)>(fnAddr)(lp);
            char msg[128];
            snprintf(msg, sizeof(msg),
                "  Thing::getId fn=0x%08X  ptr=0x%08X  result=%u",
                (unsigned)fnAddr, (unsigned)lp, probeId);
            iLog(msg);
        } else {
            char msg[128];
            snprintf(msg, sizeof(msg),
                "  Thing.getId not found in ClassMemberFunctions  lp=0x%08X", (unsigned)lp);
            iLog(msg);
        }

        // Wait the remaining 1 s for the sample to propagate
        for (int i = 0; i < 100 && !g_offsetsCalibrated; ++i) Sleep(10);
    }
    if (!g_offsetsCalibrated) ForceCalibrate();

    // ---- Step 5: Wait for game to be online (replaces window detection) ----
    // Poll g_game->isOnline() every 100 ms for up to 30 s.  Works on any
    // OTClient-based server (Miracle, Arcanum, etc.) without relying on window
    // class names or process enumeration.
    {
        bool online = false;
        for (int i = 0; i < 300; ++i) {   // 300 × 100 ms = 30 s
            if (g_game->isOnline()) { online = true; break; }
            Sleep(100);
        }
        if (online) iLog("isOnline: true — proceeding");
        else        iLog("isOnline: timeout (30 s) — proceeding anyway");
    }

    // ---- Step 6: Service Ready ----
    g_ready = true;

    // Heartbeat thread: log isOnline() every 5 s so the init log confirms the
    // bot remains active while in-game.  Runs until the process exits.
    struct Heartbeat {
        static DWORD WINAPI Run(LPVOID) {
            for (int tick = 1; ; ++tick) {
                Sleep(5000);
                bool online = g_game->isOnline();
                char msg[64];
                snprintf(msg, sizeof(msg),
                    "Heartbeat tick=%d  isOnline=%d", tick, (int)online);
                FILE* f = fopen("C:\\easybot_init.log", "a");
                if (f) { fprintf(f, "%s\n", msg); fclose(f); }
            }
            return 0;
        }
    };
    CreateThread(NULL, 0, Heartbeat::Run, NULL, 0, NULL);

    RunServer();
    return 0;
}

struct BotLoader {
    BotLoader() {
        // Launches EasyBot on its own thread
        CreateThread(NULL, 0, EasyBotInit, NULL, 0, NULL);
    }
};
static BotLoader loader;

static void DeletePortFile() {
    char dllPath[MAX_PATH];
    if (!GetModuleFileNameA(g_hDll, dllPath, MAX_PATH)) return;
    char* lastSlash = strrchr(dllPath, '\\');
    if (!lastSlash) return;
    *(lastSlash + 1) = '\0';
    char portFile[MAX_PATH];
    strcpy_s(portFile, dllPath);
    strcat_s(portFile, "easybot_port.txt");
    DeleteFileA(portFile);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        g_hDll = hModule;
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
        DeletePortFile();
    return TRUE;
}