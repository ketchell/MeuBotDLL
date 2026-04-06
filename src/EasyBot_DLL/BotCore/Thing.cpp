#include "Thing.h"
#include <algorithm>
#include <cctype>
#include <windows.h>

// SEH-protected 32-bit read — returns 0 on access fault.
// Kept local so Thing.cpp has no dependency on hooks.cpp internals.
static uint32_t safeRead32(uintptr_t addr) {
    if (!addr) return 0;
    __try { return *reinterpret_cast<const uint32_t*>(addr); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}

// Returns true if addr points into a committed, executable memory page.
// Used to validate vtable function-pointer candidates.
static bool isExecutable(uintptr_t addr) {
    if (!addr) return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) return false;
    return (mbi.State == MEM_COMMIT) &&
           (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
                           PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));
}

Thing* Thing::instance{nullptr};
std::mutex Thing::mutex;


Thing* Thing::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new Thing();
    }
    return instance;
}

uint32_t Thing::getId(ThingPtr thing) {
    if (!thing) return 0;
    typedef uint32_t(gameCall* GetId)(uintptr_t RCX, void* RDX);

    auto function = reinterpret_cast<GetId>(ClassMemberFunctions["Thing.getId"]);

    // --- Stage 1: hook-map offset brute-force (0x08, 0x0C) ---
    if (!function) {
        static const uint32_t kOffsets[] = { 0x08, 0x0C };
        for (uint32_t off : kOffsets) {
            if (off == classFunctionOffset) continue;
            ForceClassOffset(off);
            function = reinterpret_cast<GetId>(ClassMemberFunctions["Thing.getId"]);
            if (function) {
                FILE* f = fopen("C:\\easybot_init.log", "a");
                if (f) {
                    fprintf(f, "getId hook-map brute-force: locked classFunctionOffset=0x%02X\n", off);
                    fclose(f);
                }
                break;
            }
        }
    }

    // --- Stage 2: direct vtable walk ---
    // The hook captured the wrong (Lua-wrapper) address. Read the real C++
    // vtable directly from the object: thing[0] = vptr, then check indices 1-2.
    if (!function) {
        uintptr_t vptr = safeRead32(thing);
        if (vptr) {
            for (int vIdx = 1; vIdx <= 2; ++vIdx) {
                uintptr_t candidate = safeRead32(vptr + (uint32_t)vIdx * 4);
                if (isExecutable(candidate)) {
                    function = reinterpret_cast<GetId>(candidate);
                    // Pin it so all subsequent calls (IsItem, looting, etc.) skip this walk.
                    ClassMemberFunctions["Thing.getId"] = candidate;
                    FILE* f = fopen("C:\\easybot_init.log", "a");
                    if (f) {
                        fprintf(f, "getId vtable walk: found at vptr=0x%08X vtable[%d]=0x%08X\n",
                            (unsigned)vptr, vIdx, (unsigned)candidate);
                        fclose(f);
                    }
                    break;
                }
            }
        }
    }

    // --- Stage 3: legacy struct-offset direct read (OTCv8 standard) ---
    // If no callable function found, read the ID field directly from the struct.
    // Standard OTCv8 offsets: +0x10 then +0x14.
    if (!function) {
        static const uint32_t kIdOffsets[] = { 0x10, 0x14 };
        for (uint32_t off : kIdOffsets) {
            uint32_t id = safeRead32(thing + off);
            if (id && id < 0xFFFF) {   // sanity: item IDs are < 65535
                FILE* f = fopen("C:\\easybot_init.log", "a");
                if (f) {
                    fprintf(f, "getId legacy read: id=%u at thing+0x%02X (no function found)\n",
                        id, off);
                    fclose(f);
                }
                return id;
            }
        }
        // All stages exhausted.
        if (g_offsetsCalibrated) ResetClassCalibration();
        return 0;
    }

    {
        static bool s_logged = false;
        if (!s_logged) {
            s_logged = true;
            FILE* f = fopen("C:\\easybot_init.log", "a");
            if (f) {
                fprintf(f, "getId resolved via function at classFunctionOffset=0x%02X\n",
                    classFunctionOffset);
                fclose(f);
            }
        }
    }

    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}


Position Thing::getPosition(ThingPtr thing) {
    if (!thing) return {};
    typedef void(gameCall* GetPosition)(
        uintptr_t RCX,
        Position *RDX
        );
    auto function = reinterpret_cast<GetPosition>(ClassMemberFunctions["Thing.getPosition"]);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thing]() {
        Position result{};
        function(thing, &result);
        return result;
    });
}

ContainerPtr Thing::getParentContainer(ThingPtr thing) {
    if (!thing) return 0;
    typedef void(gameCall* GetParentContainer)(
        uintptr_t RCX,
        ContainerPtr *RDX
        );
    auto function = reinterpret_cast<GetParentContainer>(ClassMemberFunctions["Thing.getParentContainer"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        ContainerPtr result;
        function(thing, &result);
        return result;
    });
}

bool Thing::isItem(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsItem)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsItem>(ClassMemberFunctions["Thing.isItem"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}

bool Thing::isMonster(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsMonster)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsMonster>(ClassMemberFunctions["Thing.isMonster"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}

bool Thing::isNpc(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsNpc)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsNpc>(ClassMemberFunctions["Thing.isNpc"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}

bool Thing::isCreature(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsCreature)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsCreature>(ClassMemberFunctions["Thing.isCreature"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}

bool Thing::isPlayer(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsPlayer)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsPlayer>(ClassMemberFunctions["Thing.isPlayer"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}

bool Thing::isLocalPlayer(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsLocalPlayer)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsLocalPlayer>(ClassMemberFunctions["Thing.isLocalPlayer"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}

bool Thing::isContainer(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsContainer)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsContainer>(ClassMemberFunctions["Thing.isContainer"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}

bool Thing::isUsable(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsUsable)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsUsable>(ClassMemberFunctions["Thing.isUsable"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}

bool Thing::isLyingCorpse(ThingPtr thing) {
    if (!thing) return false;
    typedef bool(gameCall* IsLyingCorpse)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<IsLyingCorpse>(ClassMemberFunctions["Thing.isLyingCorpse"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        void* pMysteryPtr = nullptr;
        return function(thing, &pMysteryPtr);
    });
}


