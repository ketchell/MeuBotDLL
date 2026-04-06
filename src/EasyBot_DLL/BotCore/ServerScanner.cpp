#include "ServerScanner.h"
#include "hooks.h"
#include "offsets_global.h"
#include "AutoPatternFinder.h"
#include "pattern_scan.h"
#include "MinHook.h"
#include <windows.h>
#include <chrono>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static void sLog(const char* msg) {
    FILE* f = fopen("C:\\easybot_init.log", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

static uint32_t CalcChecksum(uintptr_t base, uintptr_t size) {
    uint32_t sum = 0;
    uint32_t n = (size > 4096) ? 4096 : static_cast<uint32_t>(size);
    for (uint32_t i = 0; i < n; i++)
        sum += *reinterpret_cast<const unsigned char*>(base + i);
    return sum;
}

static bool IsStdcall(uintptr_t addr) {
    return addr && (*reinterpret_cast<const unsigned char*>(addr) == 0x55);
}

// ---------------------------------------------------------------------------
// Address cache (keyed on module base; invalidated by size or checksum change)
// ---------------------------------------------------------------------------

static void CachePath(uintptr_t moduleBase, char* out, int sz) {
    snprintf(out, sz, "C:\\easybot_cache_%08X.dat", (unsigned)moduleBase);
}

static bool ReadCache(uintptr_t moduleBase, uintptr_t moduleSize,
                      uintptr_t* outBind, uintptr_t* outCall) {
    char path[MAX_PATH];
    CachePath(moduleBase, path, sizeof(path));
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    uint32_t buf[6] = {};
    bool ok = (fread(buf, sizeof(buf), 1, f) == 1);
    fclose(f);
    if (!ok || buf[0] != 0xEA5B0700u) return false;
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
    CachePath(moduleBase, path, sizeof(path));
    FILE* f = fopen(path, "wb");
    if (f) { fwrite(buf, sizeof(buf), 1, f); fclose(f); }
}

// ---------------------------------------------------------------------------
// ScanServer
// ---------------------------------------------------------------------------

bool ScanServer(ServerProfile* profile) {
    using clk = std::chrono::high_resolution_clock;
    using ms  = std::chrono::milliseconds;
    auto t0 = clk::now();

    // ---- Module info ----
    uintptr_t modBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(modBase);
    auto* nt  = reinterpret_cast<IMAGE_NT_HEADERS*>(modBase + dos->e_lfanew);
    uintptr_t modSize = nt->OptionalHeader.SizeOfImage;

    profile->moduleBase  = (uint32_t)modBase;
    profile->moduleSize  = (uint32_t)modSize;
    profile->exeChecksum = CalcChecksum(modBase, modSize);

    // ---- Step 1: MH_Initialize ----
    MH_Initialize();  // idempotent; MH_ERROR_ALREADY_INITIALIZED is fine
    sLog("MH_Initialize done");

    // ---- Step 2: FindPattern ----
    sLog("FindPattern: all targets...");
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
        sLog(buf);
    }

    if (!main_func) {
        sLog("FATAL: mainLoop pattern not found — aborting scan");
        return false;
    }

    // ---- Step 3: Cache / AutoFind fallback ----
    if (!bind_func || !call_func) {
        uintptr_t cacheBind = 0, cacheCall = 0;
        if (ReadCache(modBase, modSize, &cacheBind, &cacheCall)) {
            if (!bind_func) bind_func = cacheBind;
            if (!call_func) call_func = cacheCall;
            char buf[128];
            snprintf(buf, sizeof(buf), "Cache hit — bind=0x%08X  call=0x%08X",
                (unsigned)bind_func, (unsigned)call_func);
            sLog(buf);
        } else {
            sLog("Cache miss — running AutoFindHookTargets (slow, first run)...");
            AutoFinderResult ar = AutoFindHookTargets();
            auto t2 = clk::now();
            {
                char buf[128];
                snprintf(buf, sizeof(buf),
                    "AutoFind: %lldms  success=%d  bind=0x%08X  call=0x%08X",
                    (long long)std::chrono::duration_cast<ms>(t2 - t1).count(),
                    (int)ar.success,
                    (unsigned)ar.bindSingletonFunction, (unsigned)ar.callGlobalField);
                sLog(buf);
            }
            if (ar.bindSingletonFunction || ar.callGlobalField)
                WriteCache(modBase, modSize, ar.bindSingletonFunction, ar.callGlobalField);
            if (!bind_func && ar.bindSingletonFunction) bind_func = ar.bindSingletonFunction;
            if (!call_func && ar.callGlobalField)       call_func = ar.callGlobalField;
        }
    }

    profile->bindSingletonFunc   = (uint32_t)bind_func;
    profile->callGlobalFieldFunc = (uint32_t)call_func;
    profile->mainLoopFunc        = (uint32_t)main_func;
    profile->callingConvention   = IsStdcall(bind_func) ? 0u : 1u;

    // ---- Step 4: Install hooks with calling-convention detection ----
    sLog("Installing hooks...");

    // bindSingletonFunction
    if (bind_func) {
        char buf[80];
        snprintf(buf, sizeof(buf), "  bindSingleton -> 0x%08X  (%s)",
            (unsigned)bind_func, IsStdcall(bind_func) ? "stdcall" : "cdecl");
        sLog(buf);
        LPVOID fn = IsStdcall(bind_func)
            ? reinterpret_cast<LPVOID>(&hooked_bindSingletonFunction)
            : reinterpret_cast<LPVOID>(&hooked_bindSingletonFunction_cdecl);
        MH_CreateHook(reinterpret_cast<LPVOID>(bind_func), fn,
            reinterpret_cast<LPVOID*>(&original_bindSingletonFunction));
    }

    // callGlobalField — skip on cdecl to avoid stack corruption
    if (call_func) {
        if (IsStdcall(call_func)) {
            char buf[80];
            snprintf(buf, sizeof(buf), "  callGlobalField -> 0x%08X  (stdcall)", (unsigned)call_func);
            sLog(buf);
            MH_CreateHook(reinterpret_cast<LPVOID>(call_func),
                reinterpret_cast<LPVOID>(&hooked_callGlobalField),
                reinterpret_cast<LPVOID*>(&original_callGlobalField));
        } else {
            char buf[80];
            snprintf(buf, sizeof(buf),
                "  callGlobalField 0x%08X is cdecl — skipped to avoid crash", (unsigned)call_func);
            sLog(buf);
        }
    }

    // mainLoop
    {
        char buf[80];
        snprintf(buf, sizeof(buf), "  mainLoop -> 0x%08X", (unsigned)main_func);
        sLog(buf);
    }
    MH_STATUS r = MH_CreateHook(reinterpret_cast<LPVOID>(main_func),
        reinterpret_cast<LPVOID>(&hooked_MainLoop),
        reinterpret_cast<LPVOID*>(&original_mainLoop));
    if (r != MH_OK) {
        sLog("FATAL: mainLoop MH_CreateHook failed");
        return false;
    }

    // ---- Step 5: Enable all hooks ----
    MH_EnableHook(MH_ALL_HOOKS);
    {
        auto t3 = clk::now();
        char buf[80];
        snprintf(buf, sizeof(buf), "Hooks live — %lldms so far",
            (long long)std::chrono::duration_cast<ms>(t3 - t0).count());
        sLog(buf);
    }

    // ---- Step 6: Wait for binding capture ----
    sLog("Waiting for g_game.look...");
    int waited = 0;
    while (!SingletonFunctions["g_game.look"].first) {
        Sleep(10);
        if (++waited > 3000) { sLog("TIMEOUT waiting for g_game.look"); break; }
    }
    {
        char buf[192];
        snprintf(buf, sizeof(buf),
            "Bindings captured: Singletons=%d  ClassMembers=%d  waited=%dms",
            (int)SingletonFunctions.size(), (int)ClassMemberFunctions.size(), waited * 10);
        sLog(buf);
    }

    // ---- Step 6b: Detect Lua wrapper ----
    bool isLuaWrapper = false;
    {
        uintptr_t hp = ClassMemberFunctions["LocalPlayer.getHealth"];
        uintptr_t mp = ClassMemberFunctions["LocalPlayer.getMana"];
        uintptr_t lv = ClassMemberFunctions["LocalPlayer.getLevel"];
        uintptr_t mh = ClassMemberFunctions["LocalPlayer.getMaxHealth"];

        char buf[256];
        if (ClassMemberFunctions.size() < 10) {
            isLuaWrapper = true;
            snprintf(buf, sizeof(buf),
                "LuaWrapper detect: ClassMembers=%d (nearly empty) -> LUA WRAPPER",
                (int)ClassMemberFunctions.size());
        } else {
            int dupes = 0;
            if (hp && hp == mp) dupes++;
            if (hp && hp == lv) dupes++;
            if (hp && hp == mh) dupes++;
            isLuaWrapper = (dupes >= 2);
            snprintf(buf, sizeof(buf),
                "LuaWrapper detect: hp=0x%08X mp=0x%08X lv=0x%08X mh=0x%08X dupes=%d -> %s",
                (unsigned)hp, (unsigned)mp, (unsigned)lv, (unsigned)mh, dupes,
                isLuaWrapper ? "LUA WRAPPER" : "direct C++");
        }
        sLog(buf);
    }

    profile->isLuaWrapperServer = isLuaWrapper ? 1u : 0u;
    profile->useFunctionCalls   = isLuaWrapper ? 0u : 1u;

    // ---- Step 6c: Singleton uniqueness check ----
    {
        uintptr_t isOnline = SingletonFunctions["g_game.isOnline"].first;
        uintptr_t getLP    = SingletonFunctions["g_game.getLocalPlayer"].first;
        uintptr_t walk     = SingletonFunctions["g_game.walk"].first;
        bool unique = (isOnline != getLP) && (isOnline != walk) && (getLP != walk);
        profile->singletonsUnique = unique ? 1u : 0u;
        char buf[128];
        snprintf(buf, sizeof(buf),
            "Singletons: isOnline=0x%08X getLP=0x%08X walk=0x%08X -> %s",
            (unsigned)isOnline, (unsigned)getLP, (unsigned)walk,
            unique ? "UNIQUE (OK)" : "SHARED (also wrappers!)");
        sLog(buf);
    }

    // ---- Step 6d: OffsetResolver (Lua wrapper servers only) ----
    if (isLuaWrapper) {
        sLog("Running OffsetResolver...");
        RunOffsetResolver();
        {
            char buf[256];
            snprintf(buf, sizeof(buf),
                "OffsetResolver: health=0x%X maxHealth=0x%X mana=0x%X level=0x%X soul=0x%X stamina=0x%X",
                g_botOffsets.creature_health, g_botOffsets.creature_maxHealth,
                g_botOffsets.player_mana,     g_botOffsets.player_level,
                g_botOffsets.player_soul,     g_botOffsets.player_stamina);
            sLog(buf);
        }

        // Trust the OffsetResolver values — no server-specific overrides.
    }

    // ---- Populate profile offsets from g_botOffsets ----
    profile->healthOffset    = g_botOffsets.creature_health;
    profile->maxHealthOffset = g_botOffsets.creature_maxHealth;
    profile->manaOffset      = g_botOffsets.player_mana;
    profile->maxManaOffset   = g_botOffsets.player_maxMana;
    profile->levelOffset     = g_botOffsets.player_level;
    profile->soulOffset      = g_botOffsets.player_soul;
    profile->staminaOffset   = g_botOffsets.player_stamina;
    profile->positionOffset  = g_botOffsets.thing_positionOffset;

    auto tEnd = clk::now();
    char buf[80];
    snprintf(buf, sizeof(buf), "ScanServer complete — total %lldms",
        (long long)std::chrono::duration_cast<ms>(tEnd - t0).count());
    sLog(buf);
    return true;
}

// ---------------------------------------------------------------------------
// ApplyProfile
// ---------------------------------------------------------------------------

void ApplyProfile(const ServerProfile* profile) {
    // ---- Set globals from profile ----
    g_isLuaWrapperServer = (profile->isLuaWrapperServer != 0);

    if (profile->isLuaWrapperServer) {
        g_botOffsets.creature_health      = profile->healthOffset;
        g_botOffsets.creature_maxHealth   = profile->maxHealthOffset;
        g_botOffsets.player_mana          = profile->manaOffset;
        g_botOffsets.player_maxMana       = profile->maxManaOffset;
        g_botOffsets.player_level         = profile->levelOffset;
        g_botOffsets.player_soul          = profile->soulOffset;
        g_botOffsets.player_stamina       = profile->staminaOffset;
        g_botOffsets.thing_positionOffset = profile->positionOffset;
        g_botOffsets.resolved             = true;

        char buf[256];
        snprintf(buf, sizeof(buf),
            "ApplyProfile offsets: hp=0x%X mhp=0x%X mp=0x%X lvl=0x%X soul=0x%X stam=0x%X pos=0x%X",
            profile->healthOffset, profile->maxHealthOffset, profile->manaOffset,
            profile->levelOffset,  profile->soulOffset,      profile->staminaOffset,
            profile->positionOffset);
        sLog(buf);
    }

    // ---- Install hooks at saved addresses (no-op if already installed) ----
    // MH_Initialize is idempotent.
    MH_Initialize();

    auto installHook = [](uintptr_t addr, LPVOID hookFn, LPVOID* orig, const char* name) {
        if (!addr) return;
        MH_STATUS s = MH_CreateHook(reinterpret_cast<LPVOID>(addr), hookFn, orig);
        char buf[80];
        snprintf(buf, sizeof(buf), "  %s -> 0x%08X  MH=%d", name, (unsigned)addr, (int)s);
        sLog(buf);
        // MH_ERROR_ALREADY_CREATED (5) is fine — hook was installed by ScanServer
    };

    installHook(profile->bindSingletonFunc,
        IsStdcall(profile->bindSingletonFunc)
            ? reinterpret_cast<LPVOID>(&hooked_bindSingletonFunction)
            : reinterpret_cast<LPVOID>(&hooked_bindSingletonFunction_cdecl),
        reinterpret_cast<LPVOID*>(&original_bindSingletonFunction), "bindSingleton");

    if (profile->callGlobalFieldFunc && IsStdcall(profile->callGlobalFieldFunc)) {
        installHook(profile->callGlobalFieldFunc,
            reinterpret_cast<LPVOID>(&hooked_callGlobalField),
            reinterpret_cast<LPVOID*>(&original_callGlobalField), "callGlobalField");
    }

    installHook(profile->mainLoopFunc,
        reinterpret_cast<LPVOID>(&hooked_MainLoop),
        reinterpret_cast<LPVOID*>(&original_mainLoop), "mainLoop");

    MH_EnableHook(MH_ALL_HOOKS);

    // ---- Wait for bindings (instant if ScanServer already ran) ----
    if (!SingletonFunctions["g_game.look"].first) {
        sLog("Waiting for g_game.look (load path)...");
        int waited = 0;
        while (!SingletonFunctions["g_game.look"].first) {
            Sleep(10);
            if (++waited > 3000) { sLog("TIMEOUT waiting for g_game.look"); break; }
        }
        char buf[128];
        snprintf(buf, sizeof(buf), "Bindings captured: Singletons=%d  ClassMembers=%d",
            (int)SingletonFunctions.size(), (int)ClassMemberFunctions.size());
        sLog(buf);
    } else {
        sLog("Bindings already captured (scan path) — skipping wait");
    }

    // ---- Look hook ----
    uintptr_t lookAddr = SingletonFunctions["g_game.look"].first;
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Look hook: lookAddr=0x%08X", (unsigned)lookAddr);
        sLog(buf);
    }
    if (lookAddr) {
        MH_STATUS createStatus = MH_CreateHook(
            reinterpret_cast<LPVOID>(lookAddr),
            reinterpret_cast<LPVOID>(&hooked_Look),
            reinterpret_cast<LPVOID*>(&look_original));
        MH_STATUS enableStatus = MH_EnableHook(reinterpret_cast<LPVOID>(lookAddr));
        char buf[128];
        snprintf(buf, sizeof(buf),
            "Look hook: createStatus=%d  enableStatus=%d  lookAddr=0x%08X",
            (int)createStatus, (int)enableStatus, (unsigned)lookAddr);
        sLog(buf);
    } else {
        sLog("g_game.look not found — look hook skipped");
    }

    // Signal ready — caller (EasyBot_DLL.cpp) starts gRPC after this returns.
    g_ready = true;
    sLog("ApplyProfile complete — g_ready=true");
}
