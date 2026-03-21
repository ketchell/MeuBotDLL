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
#include <chrono>

HMODULE g_hDll = NULL;

// ============================================================================
// Main DLL worker thread
// ============================================================================
DWORD WINAPI EasyBot(HMODULE /*hModule*/) {
    // Delete any stale port file from a previous run.
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

    // Step-by-step init log — every line written before each operation.
    // If the DLL crashes, the last line in the file shows where it happened.
    FILE* f = fopen("C:\\easybot_init.log", "w");
    if (f) { fprintf(f, "EasyBot starting\n"); fclose(f); }

    auto iLog = [](const char* msg) {
        FILE* f2 = fopen("C:\\easybot_init.log", "a");
        if (f2) { fprintf(f2, "%s\n", msg); fclose(f2); }
    };

    using clk = std::chrono::high_resolution_clock;
    using ms  = std::chrono::milliseconds;

    // Helper: create a bindSingleton hook and log the target address.
    auto hookBindSingleton = [&](uintptr_t addr) -> MH_STATUS {
        if (!addr) return MH_ERROR_NOT_EXECUTABLE;
        char buf[56];
        snprintf(buf, sizeof(buf), "  bindSingleton -> 0x%08X", (unsigned)addr);
        iLog(buf);
        return MH_CreateHook(reinterpret_cast<LPVOID>(addr),
            &hooked_bindSingletonFunction,
            reinterpret_cast<LPVOID*>(&original_bindSingletonFunction));
    };

    // ---- MinHook init ----
    auto t0 = clk::now();
    iLog("MH_Initialize...");
    MH_Initialize();
    iLog("MH_Initialize done");

    // ---- Step 1: FindPattern — fast (<1ms), gets hooks live ASAP ----
    iLog("FindPattern: all targets...");
    uintptr_t bindSingleton_func   = FindPattern(bindSingletonFunction_x86_PATTERN, bindSingletonFunction_x86_MASK);
    uintptr_t callGlobalField_func = FindPattern(callGlobalField_PATTERN, callGlobalField_MASK);
    uintptr_t mainLoop_func        = FindPattern(mainLoop_x86_PATTERN, mainLoop_x86_MASK);
    auto t1 = clk::now();

    {
        char buf[192];
        snprintf(buf, sizeof(buf),
            "FindPattern: bindSingleton=0x%08X  callGlobalField=0x%08X  mainLoop=0x%08X",
            (unsigned)bindSingleton_func, (unsigned)callGlobalField_func, (unsigned)mainLoop_func);
        iLog(buf);
    }

    // mainLoop has no auto-finder fallback — abort now if missing.
    if (!mainLoop_func) {
        iLog("FATAL: mainLoop pattern=0 — aborting to preserve client");
        return 0;
    }

    // ---- Step 2: Install hooks for whatever FindPattern found ----
    iLog("MH_CreateHook...");
    MH_STATUS r1 = hookBindSingleton(bindSingleton_func);

    MH_STATUS r2 = MH_ERROR_NOT_EXECUTABLE;
    if (callGlobalField_func)
        r2 = MH_CreateHook(reinterpret_cast<LPVOID>(callGlobalField_func),
            &hooked_callGlobalField,
            reinterpret_cast<LPVOID*>(&original_callGlobalField));

    MH_STATUS r3 = MH_CreateHook(reinterpret_cast<LPVOID>(mainLoop_func),
        &hooked_MainLoop,
        reinterpret_cast<LPVOID*>(&original_mainLoop));

    if (r3 != MH_OK) {
        iLog("FATAL: mainLoop MH_CreateHook failed — aborting");
        return 0;
    }
    auto t2 = clk::now();

    // ---- Step 3: Enable immediately — binding window is open now ----
    iLog("MH_EnableHook(MH_ALL_HOOKS)...");
    MH_EnableHook(MH_ALL_HOOKS);
    auto t3 = clk::now();

    {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "Timing: MH_Init+FindPattern=%lldms  CreateHook=%lldms  EnableHook=%lldms  total_to_live=%lldms",
            (long long)std::chrono::duration_cast<ms>(t1 - t0).count(),
            (long long)std::chrono::duration_cast<ms>(t2 - t1).count(),
            (long long)std::chrono::duration_cast<ms>(t3 - t2).count(),
            (long long)std::chrono::duration_cast<ms>(t3 - t0).count());
        iLog(buf);
    }

    // ---- Step 4: Auto-finder for anything FindPattern missed ----
    // Runs after hooks are live — won't delay the binding window.
    // Note: auto-finder sees MinHook E9 detours on already-hooked functions,
    // which is fine since SSO-tail and LuaRegPush patterns are at offset >5.
    if (!bindSingleton_func || !callGlobalField_func) {
        iLog("FindPattern missed targets — running AutoFindHookTargets...");
        AutoFinderResult autoResult = AutoFindHookTargets();
        auto t4 = clk::now();
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "AutoFind took %lldms  success=%d",
                (long long)std::chrono::duration_cast<ms>(t4 - t3).count(),
                (int)autoResult.success);
            iLog(buf);
        }

        if (!bindSingleton_func && autoResult.bindSingletonFunction) {
            bindSingleton_func = autoResult.bindSingletonFunction;
            MH_STATUS ra = hookBindSingleton(bindSingleton_func);
            MH_EnableHook(reinterpret_cast<LPVOID>(bindSingleton_func));
            char buf[80];
            snprintf(buf, sizeof(buf), "  auto bindSingleton hook: MH=%d", (int)ra);
            iLog(buf);
        }

        if (!callGlobalField_func && autoResult.callGlobalField) {
            callGlobalField_func = autoResult.callGlobalField;
            MH_STATUS ra = MH_CreateHook(reinterpret_cast<LPVOID>(callGlobalField_func),
                &hooked_callGlobalField,
                reinterpret_cast<LPVOID*>(&original_callGlobalField));
            MH_EnableHook(reinterpret_cast<LPVOID>(callGlobalField_func));
            char buf[80];
            snprintf(buf, sizeof(buf), "  auto callGlobalField hook: MH=%d", (int)ra);
            iLog(buf);
        }
    }

    iLog("Hooks live — waiting for hooked_bindSingletonFunction to fire...");

    // ---- Wait for g_game.look to be populated by hooked_bindSingletonFunction ----
    // The game registers its Lua bindings at startup; once the hook captures
    // g_game.look we know the full binding pass has run.
    int waitCount = 0;
    while (!SingletonFunctions["g_game.look"].first) {
        Sleep(10);
        ++waitCount;
        if (waitCount > 3000) { // 30-second timeout
            iLog("TIMEOUT: g_game.look not captured after 30 s — binding hook may have missed the registration window");
            break;
        }
    }

    {
        char buf[192];
        snprintf(buf, sizeof(buf),
            "Wait done: g_game.look=0x%08X  waited=%d ms  SingletonFunctions=%d  ClassMemberFunctions=%d",
            (unsigned)SingletonFunctions["g_game.look"].first,
            waitCount * 10,
            (int)SingletonFunctions.size(),
            (int)ClassMemberFunctions.size());
        iLog(buf);
    }

    // ---- Install the look hook ----
    if (SingletonFunctions["g_game.look"].first) {
        iLog("Installing look hook...");
        MH_STATUS rLook = MH_CreateHook(
            reinterpret_cast<LPVOID>(SingletonFunctions["g_game.look"].first),
            &hooked_Look,
            reinterpret_cast<LPVOID*>(&look_original));
        MH_EnableHook(reinterpret_cast<LPVOID>(SingletonFunctions["g_game.look"].first));
        char buf[64];
        snprintf(buf, sizeof(buf), "Look hook installed (MH=%d)", (int)rLook);
        iLog(buf);
    } else {
        iLog("g_game.look not found — look hook skipped");
    }

    g_ready = true;
    iLog("g_ready=true — starting gRPC server");
    RunServer();
    return 0;
}


struct BotLoader {
    BotLoader() {
        CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(EasyBot), NULL, 0, NULL);
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
