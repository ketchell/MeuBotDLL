#include "LocalPlayer.h"
LocalPlayer* LocalPlayer::instance{nullptr};
std::mutex LocalPlayer::mutex;

LocalPlayer* LocalPlayer::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new LocalPlayer();
    }
    return instance;
}

Otc::PlayerStates LocalPlayer::getStates(LocalPlayerPtr localPlayer) {
    if (!localPlayer) {
        FILE* f = fopen("C:\\easybot_lp_debug.log", "a");
        if (f) { fprintf(f, "getStates: localPlayer is NULL\n"); fclose(f); }
        return Otc::IconNone;
    }
    typedef Otc::PlayerStates(gameCall* GetStates)(uintptr_t RCX);
    auto function = reinterpret_cast<GetStates>(ClassMemberFunctions["LocalPlayer.getStates"]);
    {
        FILE* f = fopen("C:\\easybot_lp_debug.log", "a");
        if (f) { fprintf(f, "getStates: lp=0x%08X fn=0x%08X\n", (unsigned)localPlayer, (unsigned)(uintptr_t)function); fclose(f); }
    }
    if (!function) {
        FILE* f = fopen("C:\\easybot_lp_debug.log", "a");
        if (f) { fprintf(f, "getStates: fn is NULL!\n"); fclose(f); }
        return Otc::IconNone;
    }
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        auto result = function(localPlayer);
        FILE* f = fopen("C:\\easybot_lp_debug.log", "a");
        if (f) { fprintf(f, "getStates RESULT: %d\n", (int)result); fclose(f); }
        return result;
    });
}

double LocalPlayer::getHealth(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef double(gameCall* GetHealth)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<GetHealth>(ClassMemberFunctions["LocalPlayer.getHealth"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

double LocalPlayer::getMaxHealth(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef double(gameCall* GetMaxHealth)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<GetMaxHealth>(ClassMemberFunctions["LocalPlayer.getMaxHealth"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

double LocalPlayer::getFreeCapacity(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef double(gameCall* GetFreeCapacity)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<GetFreeCapacity>(ClassMemberFunctions["LocalPlayer.getFreeCapacity"]);
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

double LocalPlayer::getLevel(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef double(gameCall* GetLevel)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<GetLevel>(ClassMemberFunctions["LocalPlayer.getLevel"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

double LocalPlayer::getMana(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef double(gameCall* GetMana)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<GetMana>(ClassMemberFunctions["LocalPlayer.getMana"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

double LocalPlayer::getMaxMana(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef double(gameCall* GetMaxMana)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<GetMaxMana>(ClassMemberFunctions["LocalPlayer.getMaxMana"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

double LocalPlayer::getSoul(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef double(gameCall* GetSoul)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<GetSoul>(ClassMemberFunctions["LocalPlayer.getSoul"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

double LocalPlayer::getStamina(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef double(gameCall* GetStamina)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<GetStamina>(ClassMemberFunctions["LocalPlayer.getStamina"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

ItemPtr LocalPlayer::getInventoryItem(LocalPlayerPtr localPlayer, Otc::InventorySlot inventorySlot) {
    if (!localPlayer) return 0;
    typedef void(gameCall* GetInventoryItem)(uintptr_t RCX, ItemPtr *RDX, Otc::InventorySlot inventorySlot);
    auto function = reinterpret_cast<GetInventoryItem>(ClassMemberFunctions["LocalPlayer.getInventoryItem"]);
    return g_dispatcher->scheduleEventEx([function, localPlayer, inventorySlot]() {
        ItemPtr result;
        function(localPlayer, &result, inventorySlot);
        return result;
    });
}

int LocalPlayer::getInventoryCount(LocalPlayerPtr localPlayer, uint16_t itemId, int tier) {
    if (!localPlayer) return 0;
    typedef int(gameCall* GetInventoryCount)(uintptr_t RCX, void *RDX, uint16_t itemId, int tier);
    auto function = reinterpret_cast<GetInventoryCount>(ClassMemberFunctions["LocalPlayer.getInventoryCount"]);
    return g_dispatcher->scheduleEventEx([function, localPlayer, itemId, tier]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr, itemId, tier);
    });
}

bool LocalPlayer::hasSight(LocalPlayerPtr localPlayer, const Position &pos) {
    if (!localPlayer) return 0;
    typedef bool(gameCall* HasSight)(uintptr_t RCX, void *RDX, const Position *pos);
    auto function = reinterpret_cast<HasSight>(ClassMemberFunctions["LocalPlayer.hasSight"]);
    return g_dispatcher->scheduleEventEx([function, localPlayer, pos]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr, &pos);
    });
}

bool LocalPlayer::isAutoWalking(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return 0;
    typedef bool(gameCall* IsAutoWalking)(uintptr_t RCX, void *RDX);
    auto function = reinterpret_cast<IsAutoWalking>(ClassMemberFunctions["LocalPlayer.isAutoWalking"]);
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        void* pMysteryPtr = nullptr;
        return function(localPlayer, &pMysteryPtr);
    });
}

void LocalPlayer::stopAutoWalk(LocalPlayerPtr localPlayer) {
    if (!localPlayer) return;
    typedef void(gameCall* StopAutoWalk)(uintptr_t RCX);
    auto function = reinterpret_cast<StopAutoWalk>(ClassMemberFunctions["LocalPlayer.stopAutoWalk"]);
    return g_dispatcher->scheduleEventEx([function, localPlayer]() {
        return function(localPlayer);
    });
}

bool LocalPlayer::autoWalk(LocalPlayerPtr localPlayer, const Position &destination, bool retry) {
    if (!localPlayer) return 0;
    typedef bool(gameCall* AutoWalk)(uintptr_t RCX, Position RDX, bool R8);
    auto function = reinterpret_cast<AutoWalk>(ClassMemberFunctions["LocalPlayer.autoWalk"]);
    return g_dispatcher->scheduleEventEx([function, localPlayer, destination, retry]() {
        return function(localPlayer, destination, retry);
    });
}

void LocalPlayer::setLightHack(LocalPlayerPtr localPlayer, uint16_t lightLevel) {
    if (!localPlayer) return;
    g_dispatcher->scheduleEventEx([localPlayer, lightLevel]() {
        auto lightPtr = reinterpret_cast<uint16_t*>(localPlayer + lightHackOffset);
        if (*lightPtr != lightLevel) *lightPtr = lightLevel;
    });
}
