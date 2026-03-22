#include "GameStateUpdater.h"
#include "CachedGameState.h"
#include "hooks.h"        // SingletonFunctions, ClassMemberFunctions
#include "const.h"        // Position

// ---------------------------------------------------------------------------
// All game functions use __thiscall (x86) / __fastcall (x64) — see BuildConfig.h.
// We are already on the game thread when this file's functions run, so we call
// them directly without going through g_dispatcher->scheduleEventEx().
// ---------------------------------------------------------------------------

// ---- DEBUG: logs first N calls then goes silent to avoid log spam ----
static int s_dbgCount = 0;
static constexpr int DBG_MAX = 5;

static void dbgLog(const char* msg) {
    FILE* f = fopen("C:\\easybot_cache_debug.log", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

void UpdateGameStateCache() {
    if (g_isLuaWrapperServer) { g_cachedState.valid.store(false); return; }

    const bool doLog = (s_dbgCount < DBG_MAX);

    // ---- 1. Get LocalPlayer* via g_game.getLocalPlayer ----
    auto& lpEntry = SingletonFunctions["g_game.getLocalPlayer"];
    uintptr_t lpFunc = lpEntry.first;
    uintptr_t lpThis = lpEntry.second;
    if (!lpFunc || !lpThis) {
        if (doLog) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                "UpdateCache: getLocalPlayer fn=0x%08X this=0x%08X — MISSING, valid=false",
                (unsigned)lpFunc, (unsigned)lpThis);
            dbgLog(buf);
            ++s_dbgCount;
        }
        g_cachedState.valid.store(false);
        return;
    }

    typedef void(gameCall* GetLocalPlayerFn)(uintptr_t thisPtr, LocalPlayerPtr* out);
    LocalPlayerPtr localPlayer = 0;
    reinterpret_cast<GetLocalPlayerFn>(lpFunc)(lpThis, &localPlayer);
    if (!localPlayer) {
        if (doLog) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                "UpdateCache: getLocalPlayer returned NULL (fn=0x%08X this=0x%08X), valid=false",
                (unsigned)lpFunc, (unsigned)lpThis);
            dbgLog(buf);
            ++s_dbgCount;
        }
        g_cachedState.valid.store(false);
        return;
    }

    g_cachedState.playerPtr.store(localPlayer);

    // ---- 2. Health ----
    {
        uintptr_t fn = ClassMemberFunctions["LocalPlayer.getHealth"];
        if (fn) {
            typedef double(gameCall* Fn)(uintptr_t, void*);
            void* m = nullptr;
            double v = reinterpret_cast<Fn>(fn)(localPlayer, &m);
            g_cachedState.health.store(static_cast<int>(v));
            if (doLog) {
                char buf[128];
                snprintf(buf, sizeof(buf), "UpdateCache: lp=0x%08X  health fn=0x%08X  raw=%.1f  stored=%d",
                    (unsigned)localPlayer, (unsigned)fn, v, static_cast<int>(v));
                dbgLog(buf);
            }
        } else if (doLog) {
            dbgLog("UpdateCache: LocalPlayer.getHealth fn=NULL");
        }
    }

    // ---- 3. MaxHealth ----
    {
        uintptr_t fn = ClassMemberFunctions["LocalPlayer.getMaxHealth"];
        if (fn) {
            typedef double(gameCall* Fn)(uintptr_t, void*);
            void* m = nullptr;
            double v = reinterpret_cast<Fn>(fn)(localPlayer, &m);
            g_cachedState.maxHealth.store(static_cast<int>(v));
        }
    }

    // ---- 4. Mana ----
    {
        uintptr_t fn = ClassMemberFunctions["LocalPlayer.getMana"];
        if (fn) {
            typedef double(gameCall* Fn)(uintptr_t, void*);
            void* m = nullptr;
            double v = reinterpret_cast<Fn>(fn)(localPlayer, &m);
            g_cachedState.mana.store(static_cast<int>(v));
        }
    }

    // ---- 5. MaxMana ----
    {
        uintptr_t fn = ClassMemberFunctions["LocalPlayer.getMaxMana"];
        if (fn) {
            typedef double(gameCall* Fn)(uintptr_t, void*);
            void* m = nullptr;
            double v = reinterpret_cast<Fn>(fn)(localPlayer, &m);
            g_cachedState.maxMana.store(static_cast<int>(v));
        }
    }

    // ---- 6. Level ----
    {
        uintptr_t fn = ClassMemberFunctions["LocalPlayer.getLevel"];
        if (fn) {
            typedef double(gameCall* Fn)(uintptr_t, void*);
            void* m = nullptr;
            double v = reinterpret_cast<Fn>(fn)(localPlayer, &m);
            g_cachedState.level.store(static_cast<int>(v));
            if (doLog) {
                char buf[128];
                snprintf(buf, sizeof(buf), "UpdateCache: lp=0x%08X  level fn=0x%08X  raw=%d  stored=%d",
                    (unsigned)localPlayer, (unsigned)fn, (int)v, static_cast<int>(v));
                dbgLog(buf);
            }
        } else if (doLog) {
            dbgLog("UpdateCache: LocalPlayer.getLevel fn=NULL");
        }
    }

    // ---- 7. Soul ----
    {
        uintptr_t fn = ClassMemberFunctions["LocalPlayer.getSoul"];
        if (fn) {
            typedef double(gameCall* Fn)(uintptr_t, void*);
            void* m = nullptr;
            double v = reinterpret_cast<Fn>(fn)(localPlayer, &m);
            g_cachedState.soul.store(static_cast<int>(v));
        }
    }

    // ---- 8. Stamina ----
    {
        uintptr_t fn = ClassMemberFunctions["LocalPlayer.getStamina"];
        if (fn) {
            typedef double(gameCall* Fn)(uintptr_t, void*);
            void* m = nullptr;
            double v = reinterpret_cast<Fn>(fn)(localPlayer, &m);
            g_cachedState.stamina.store(static_cast<int>(v));
        }
    }

    // ---- 9. Position ----
    {
        uintptr_t fn = ClassMemberFunctions["Thing.getPosition"];
        if (fn) {
            typedef void(gameCall* Fn)(uintptr_t, Position*);
            Position pos{};
            reinterpret_cast<Fn>(fn)(localPlayer, &pos);
            g_cachedState.posX.store(pos.x);
            g_cachedState.posY.store(pos.y);
            g_cachedState.posZ.store(pos.z);
        }
    }

    // ---- 10. Id ----
    {
        uintptr_t fn = ClassMemberFunctions["Thing.getId"];
        if (fn) {
            typedef uint32_t(gameCall* Fn)(uintptr_t, void*);
            void* m = nullptr;
            uint32_t v = reinterpret_cast<Fn>(fn)(localPlayer, &m);
            g_cachedState.id.store(v);
        }
    }

    if (doLog) {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "UpdateCache #%d DONE: lp=0x%08X hp=%d/%d mp=%d/%d lvl=%d soul=%d stam=%d pos=(%d,%d,%d) valid=true",
            s_dbgCount + 1,
            (unsigned)localPlayer,
            g_cachedState.health.load(), g_cachedState.maxHealth.load(),
            g_cachedState.mana.load(),   g_cachedState.maxMana.load(),
            g_cachedState.level.load(),  g_cachedState.soul.load(),
            g_cachedState.stamina.load(),
            g_cachedState.posX.load(), g_cachedState.posY.load(), g_cachedState.posZ.load());
        dbgLog(buf);
        ++s_dbgCount;
    }

    g_cachedState.valid.store(true);
}
