#include "../../proto_functions_server.h"
#include "pattern_scan.h"
#include "MinHook.h"
#include "hooks.h"
#include "Game.h"
#include "LocalPlayer.h"
#include "Thing.h"
#include "Container.h"
#include "Item.h"

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

    // ---- MinHook init ----
    iLog("MH_Initialize...");
    MH_Initialize();
    iLog("MH_Initialize done");

    // ---- Pattern scan — log hex addresses so we can spot 0 or garbage ----
    iLog("FindPattern: bindSingletonFunction...");
    uintptr_t bindSingleton_func = FindPattern(bindSingletonFunction_x86_PATTERN, bindSingletonFunction_x86_MASK);

    iLog("FindPattern: callGlobalField...");
    uintptr_t callGlobalField_func = FindPattern(callGlobalField_PATTERN, callGlobalField_MASK);

    iLog("FindPattern: mainLoop...");
    uintptr_t mainLoop_func = FindPattern(mainLoop_x86_PATTERN, mainLoop_x86_MASK);

    {
        char buf[192];
        snprintf(buf, sizeof(buf),
            "Patterns: bindSingleton=0x%08X  callGlobalField=0x%08X  mainLoop=0x%08X",
            (unsigned)bindSingleton_func,
            (unsigned)callGlobalField_func,
            (unsigned)mainLoop_func);
        iLog(buf);
    }

    // Guard: if callGlobalField or mainLoop are 0 the MH_CreateHook call will
    // corrupt the process — abort early so the client can still open.
    if (!callGlobalField_func || !mainLoop_func) {
        iLog("FATAL: callGlobalField or mainLoop pattern returned 0 — aborting to preserve client");
        return 0;
    }

    // ---- Install hooks ----
    iLog("MH_CreateHook: bindSingletonFunction...");
    MH_STATUS r1 = MH_ERROR_NOT_EXECUTABLE; // default: skipped
    if (bindSingleton_func) {
        r1 = MH_CreateHook(
            reinterpret_cast<LPVOID>(bindSingleton_func),
            &hooked_bindSingletonFunction,
            reinterpret_cast<LPVOID*>(&original_bindSingletonFunction));
    } else {
        iLog("  bindSingletonFunction pattern=0 — hook skipped (ClassMemberFunctions won't be populated)");
    }

    iLog("MH_CreateHook: callGlobalField...");
    MH_STATUS r2 = MH_CreateHook(
        reinterpret_cast<LPVOID>(callGlobalField_func),
        &hooked_callGlobalField,
        reinterpret_cast<LPVOID*>(&original_callGlobalField));

    iLog("MH_CreateHook: mainLoop...");
    MH_STATUS r3 = MH_CreateHook(
        reinterpret_cast<LPVOID>(mainLoop_func),
        &hooked_MainLoop,
        reinterpret_cast<LPVOID*>(&original_mainLoop));

    {
        char buf[128];
        snprintf(buf, sizeof(buf),
            "MH_CreateHook results: bindSingleton=%d  callGlobalField=%d  mainLoop=%d",
            (int)r1, (int)r2, (int)r3);
        iLog(buf);
    }

    // Abort if the mandatory hooks failed (MH_OK == 0).
    if (r2 != MH_OK || r3 != MH_OK) {
        iLog("FATAL: mandatory MH_CreateHook failed — aborting");
        return 0;
    }

    iLog("MH_EnableHook(MH_ALL_HOOKS)...");
    MH_EnableHook(MH_ALL_HOOKS);
    iLog("Hooks enabled — waiting for hooked_bindSingletonFunction to fire...");

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
