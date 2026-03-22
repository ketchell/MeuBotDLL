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
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>

HMODULE g_hDll = NULL;

// ============================================================================
// Address cache — persists auto-finder results across runs.
// Key: module base address (stable when ASLR is disabled).
// Invalidated by: module size change or .text header checksum mismatch.
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
    snprintf(path, sizeof(path), "C:\\easybot_cache_%08X.dat", (unsigned)moduleBase);
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    uint32_t buf[6] = {0};
    bool ok = (fread(buf, sizeof(buf), 1, f) == 1);
    fclose(f);
    if (!ok) return false;
    if (buf[0] != 0xEA5B0700u) return false;  // magic
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

    FILE* f = fopen("C:\\easybot_init.log", "w");
    if (f) { fprintf(f, "EasyBot starting\n"); fclose(f); }

    auto iLog = [](const char* msg) {
        FILE* f2 = fopen("C:\\easybot_init.log", "a");
        if (f2) { fprintf(f2, "%s\n", msg); fclose(f2); }
    };

    using clk = std::chrono::high_resolution_clock;
    using ms  = std::chrono::milliseconds;

    // Returns true if the byte at addr indicates a standard EBP-frame prologue.
    auto isStdcall = [](uintptr_t addr) -> bool {
        return addr && (*reinterpret_cast<const unsigned char*>(addr) == 0x55);
    };

    // Hook bindSingletonFunction — picks stdcall or cdecl variant automatically.
    auto hookBind = [&](uintptr_t addr) -> MH_STATUS {
        if (!addr) return MH_ERROR_NOT_EXECUTABLE;
        bool stdcall_ = isStdcall(addr);
        char buf[80];
        snprintf(buf, sizeof(buf), "  bindSingleton -> 0x%08X  (%s)",
            (unsigned)addr, stdcall_ ? "stdcall" : "cdecl");
        iLog(buf);
        LPVOID fn = stdcall_
            ? reinterpret_cast<LPVOID>(&hooked_bindSingletonFunction)
            : reinterpret_cast<LPVOID>(&hooked_bindSingletonFunction_cdecl);
        return MH_CreateHook(reinterpret_cast<LPVOID>(addr), fn,
            reinterpret_cast<LPVOID*>(&original_bindSingletonFunction));
    };

    // Hook callGlobalField — picks stdcall or cdecl variant automatically.
    auto hookCall = [&](uintptr_t addr) -> MH_STATUS {
        if (!addr) return MH_ERROR_NOT_EXECUTABLE;
        bool stdcall_ = isStdcall(addr);
        char buf[80];
        snprintf(buf, sizeof(buf), "  callGlobalField -> 0x%08X  (%s)",
            (unsigned)addr, stdcall_ ? "stdcall" : "cdecl");
        iLog(buf);
        LPVOID fn = stdcall_
            ? reinterpret_cast<LPVOID>(&hooked_callGlobalField)
            : reinterpret_cast<LPVOID>(&hooked_callGlobalField_cdecl);
        return MH_CreateHook(reinterpret_cast<LPVOID>(addr), fn,
            reinterpret_cast<LPVOID*>(&original_callGlobalField));
    };

    // ---- Step 1: MH_Initialize + get module info ----
    auto t0 = clk::now();
    iLog("MH_Initialize...");
    MH_Initialize();

    uintptr_t modBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
    // Read SizeOfImage from the PE optional header.
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(modBase);
    auto* nt  = reinterpret_cast<IMAGE_NT_HEADERS*>(modBase + dos->e_lfanew);
    uintptr_t modSize = nt->OptionalHeader.SizeOfImage;

    // ---- Step 2: FindPattern — fast (<1ms) ----
    iLog("FindPattern: all targets...");
    uintptr_t bind_func = FindPattern(bindSingletonFunction_x86_PATTERN, bindSingletonFunction_x86_MASK);
    uintptr_t call_func = FindPattern(callGlobalField_PATTERN,            callGlobalField_MASK);
    uintptr_t main_func = FindPattern(mainLoop_x86_PATTERN,               mainLoop_x86_MASK);
    auto t1 = clk::now();

    {
        char buf[192];
        snprintf(buf, sizeof(buf),
            "FindPattern: bind=0x%08X  call=0x%08X  main=0x%08X  (%lldms)",
            (unsigned)bind_func, (unsigned)call_func, (unsigned)main_func,
            (long long)std::chrono::duration_cast<ms>(t1 - t0).count());
        iLog(buf);
    }

    if (!main_func) {
        iLog("FATAL: mainLoop pattern not found — aborting");
        return 0;
    }

    // ---- Step 3: Cache / auto-finder if FindPattern missed bind or call ----
    if (!bind_func || !call_func) {
        uintptr_t cacheBind = 0, cacheCall = 0;
        if (ReadCache(modBase, modSize, &cacheBind, &cacheCall)) {
            if (!bind_func) bind_func = cacheBind;
            if (!call_func) call_func = cacheCall;
            char buf[128];
            snprintf(buf, sizeof(buf),
                "Cache hit — bind=0x%08X  call=0x%08X",
                (unsigned)bind_func, (unsigned)call_func);
            iLog(buf);
        } else {
            iLog("Cache miss — running AutoFindHookTargets (slow path, first run)...");
            AutoFinderResult ar = AutoFindHookTargets();
            auto t2 = clk::now();
            {
                char buf[128];
                snprintf(buf, sizeof(buf),
                    "AutoFind: %lldms  success=%d  bind=0x%08X  call=0x%08X",
                    (long long)std::chrono::duration_cast<ms>(t2 - t1).count(),
                    (int)ar.success,
                    (unsigned)ar.bindSingletonFunction,
                    (unsigned)ar.callGlobalField);
                iLog(buf);
            }
            if (ar.bindSingletonFunction || ar.callGlobalField)
                WriteCache(modBase, modSize, ar.bindSingletonFunction, ar.callGlobalField);

            if (!bind_func && ar.bindSingletonFunction) bind_func = ar.bindSingletonFunction;
            if (!call_func && ar.callGlobalField)       call_func = ar.callGlobalField;
        }
    }

    // ---- Step 4: Install hooks with convention detection ----
    iLog("MH_CreateHook...");
    hookBind(bind_func);

    // callGlobalField handles onTextMessage/onTalk — nice-to-have, not critical.
    // Skip it on cdecl servers to avoid stack corruption and Lua errors.
    if (call_func) {
        if (isStdcall(call_func)) {
            hookCall(call_func);
        } else {
            char buf[80];
            snprintf(buf, sizeof(buf),
                "callGlobalField 0x%08X is cdecl — hook skipped to avoid crash",
                (unsigned)call_func);
            iLog(buf);
        }
    }

    MH_STATUS r = MH_CreateHook(reinterpret_cast<LPVOID>(main_func),
        reinterpret_cast<LPVOID>(&hooked_MainLoop),
        reinterpret_cast<LPVOID*>(&original_mainLoop));
    if (r != MH_OK) {
        iLog("FATAL: mainLoop MH_CreateHook failed — aborting");
        return 0;
    }

    // ---- Step 5: Enable all hooks at once ----
    iLog("MH_EnableHook(MH_ALL_HOOKS)...");
    MH_EnableHook(MH_ALL_HOOKS);
    auto t3 = clk::now();

    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Hooks live — total init time: %lldms",
            (long long)std::chrono::duration_cast<ms>(t3 - t0).count());
        iLog(buf);
    }

    // ---- Step 6: Wait for bindings ----
    iLog("Waiting for g_game.look...");
    int waitCount = 0;
    while (!SingletonFunctions["g_game.look"].first) {
        Sleep(10);
        ++waitCount;
        if (waitCount > 3000) {
            iLog("TIMEOUT: g_game.look not captured after 30s");
            break;
        }
    }

    {
        char buf[192];
        snprintf(buf, sizeof(buf),
            "Wait done: g_game.look=0x%08X  waited=%dms  Singletons=%d  ClassMembers=%d",
            (unsigned)SingletonFunctions["g_game.look"].first,
            waitCount * 10,
            (int)SingletonFunctions.size(),
            (int)ClassMemberFunctions.size());
        iLog(buf);
    }

    // ---- Step 6b: Detect Lua wrapper servers ----
    bool isLuaWrapperServer = false;
    {
        uintptr_t hp = ClassMemberFunctions["LocalPlayer.getHealth"];
        uintptr_t mp = ClassMemberFunctions["LocalPlayer.getMana"];
        uintptr_t lv = ClassMemberFunctions["LocalPlayer.getLevel"];
        uintptr_t mh = ClassMemberFunctions["LocalPlayer.getMaxHealth"];

        char buf[256];
        if (ClassMemberFunctions.size() < 10) {
            // No class members captured at all — Lua wrapper server that exposes only singletons
            isLuaWrapperServer = true;
            snprintf(buf, sizeof(buf),
                "LuaWrapper detect: ClassMemberFunctions=%d (nearly empty) -> LUA WRAPPER SERVER",
                (int)ClassMemberFunctions.size());
            iLog(buf);
        } else {
            int dupes = 0;
            if (hp && hp == mp) dupes++;
            if (hp && hp == lv) dupes++;
            if (hp && hp == mh) dupes++;

            isLuaWrapperServer = (dupes >= 2);

            snprintf(buf, sizeof(buf),
                "LuaWrapper detect: hp=0x%08X mp=0x%08X lv=0x%08X mh=0x%08X dupes=%d -> %s",
                (unsigned)hp, (unsigned)mp, (unsigned)lv, (unsigned)mh, dupes,
                isLuaWrapperServer ? "LUA WRAPPER SERVER" : "direct C++ server");
            iLog(buf);
        }
    }

    if (isLuaWrapperServer) {
        uintptr_t isOnline = SingletonFunctions["g_game.isOnline"].first;
        uintptr_t getLP    = SingletonFunctions["g_game.getLocalPlayer"].first;
        uintptr_t walk     = SingletonFunctions["g_game.walk"].first;

        char buf[256];
        snprintf(buf, sizeof(buf),
            "Singleton check: isOnline=0x%08X getLP=0x%08X walk=0x%08X",
            (unsigned)isOnline, (unsigned)getLP, (unsigned)walk);
        iLog(buf);

        bool singletonsUnique = (isOnline != getLP) && (isOnline != walk) && (getLP != walk);
        iLog(singletonsUnique ? "Singletons are UNIQUE — function calls OK"
                              : "Singletons are SHARED — also wrappers!");
    }
    g_isLuaWrapperServer = isLuaWrapperServer;

    if (isLuaWrapperServer) {
        iLog("Running OffsetResolver for Lua wrapper server...");
        bool ok = RunOffsetResolver();
        char buf[256];
        snprintf(buf, sizeof(buf),
            "OffsetResolver: ok=%d  health=0x%X maxHealth=0x%X mana=0x%X level=0x%X soul=0x%X stamina=0x%X",
            (int)ok,
            g_botOffsets.creature_health, g_botOffsets.creature_maxHealth,
            g_botOffsets.player_mana,     g_botOffsets.player_level,
            g_botOffsets.player_soul,     g_botOffsets.player_stamina);
        iLog(buf);
        snprintf(buf, sizeof(buf),
            "OffsetResolver: g_game_ptr=0x%08X game_localPlayer=0x%X",
            (unsigned)g_botOffsets.g_game_ptr_addr, g_botOffsets.game_localPlayer);
        iLog(buf);

        // TODO: fix OffsetResolver to find these correctly.
        // Verified offsets from memory dump on Nostalrius (all values are C++ double, 8 bytes):
        //   +0x4C0=health, +0x4C8=maxHealth, +0x4D0=freeCap, +0x4D8=totalCap,
        //   +0x4E0=mana,   +0x4E8=level(1.0), +0x4F0=maxMana(unverified),
        //   +0x520=soul,   +0x530=stamina(minutes), +0x00C=position{x,y,z}
        g_botOffsets.creature_health    = 0x4C0;
        g_botOffsets.creature_maxHealth = 0x4C8;
        g_botOffsets.player_mana        = 0x4E0;
        g_botOffsets.player_maxMana     = 0x4F0;  // unverified — check memdump at 0x4F0
        g_botOffsets.player_level       = 0x4E8;
        g_botOffsets.player_soul        = 0x520;
        g_botOffsets.player_stamina     = 0x530;
        g_botOffsets.thing_positionOffset = 0x00C;
        g_botOffsets.resolved           = true;

        snprintf(buf, sizeof(buf),
            "Overrides applied: hp=0x%X mhp=0x%X mp=0x%X mmp=0x%X lvl=0x%X soul=0x%X stam=0x%X pos=0x%X",
            g_botOffsets.creature_health,    g_botOffsets.creature_maxHealth,
            g_botOffsets.player_mana,        g_botOffsets.player_maxMana,
            g_botOffsets.player_level,       g_botOffsets.player_soul,
            g_botOffsets.player_stamina,     g_botOffsets.thing_positionOffset);
        iLog(buf);

        // Diagnostic: log walker-critical function addresses to determine which are safe.
        // Map functions use SingletonFunctions (safe on Lua wrapper servers).
        // LocalPlayer walk functions use ClassMemberFunctions (may be Lua wrappers).
        char wbuf[512];
        snprintf(wbuf, sizeof(wbuf),
            "Walker singletons: autoWalk=0x%08X findPath=0x%08X getTile=0x%08X getSpectators=0x%08X isSightClear=0x%08X",
            (unsigned)SingletonFunctions["g_game.autoWalk"].first,
            (unsigned)SingletonFunctions["g_map.findPath"].first,
            (unsigned)SingletonFunctions["g_map.getTile"].first,
            (unsigned)SingletonFunctions["g_map.getSpectators"].first,
            (unsigned)SingletonFunctions["g_map.isSightClear"].first);
        iLog(wbuf);
        snprintf(wbuf, sizeof(wbuf),
            "Walker classmembers: LP.autoWalk=0x%08X LP.stopAutoWalk=0x%08X LP.isAutoWalking=0x%08X Tile.isWalkable=0x%08X",
            (unsigned)ClassMemberFunctions["LocalPlayer.autoWalk"],
            (unsigned)ClassMemberFunctions["LocalPlayer.stopAutoWalk"],
            (unsigned)ClassMemberFunctions["LocalPlayer.isAutoWalking"],
            (unsigned)ClassMemberFunctions["Tile.isWalkable"]);
        iLog(wbuf);
        iLog(SingletonFunctions.count("g_map.isWalkable") ? "g_map.isWalkable EXISTS" : "g_map.isWalkable NOT FOUND");
    }

    // ---- Step 7: Install look hook (stdcall servers only) ----
    if (SingletonFunctions["g_game.look"].first) {
        uintptr_t lookAddr = SingletonFunctions["g_game.look"].first;
        unsigned char lookFirstByte = *reinterpret_cast<const unsigned char*>(lookAddr);
        if (lookFirstByte == 0x55) {
            MH_CreateHook(reinterpret_cast<LPVOID>(lookAddr),
                          reinterpret_cast<LPVOID>(&hooked_Look),
                          reinterpret_cast<LPVOID*>(&look_original));
            MH_EnableHook(reinterpret_cast<LPVOID>(lookAddr));
            iLog("Look hook installed");
        } else {
            char buf[80];
            snprintf(buf, sizeof(buf),
                "g_game.look is cdecl (0x%02X) — look hook skipped", lookFirstByte);
            iLog(buf);
        }
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
