#include "Thing.h"

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
    typedef uint32_t(gameCall* GetId)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetId>(ClassMemberFunctions["Thing.getId"]);
    if (!function) return 0;
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


