#ifndef LOCALPLAYER_H
#define LOCALPLAYER_H

#define g_localPlayer LocalPlayer::getInstance()

#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"
#include <mutex>

class LocalPlayer {
    static LocalPlayer* instance;
    static std::mutex mutex;

protected:
    LocalPlayer() = default;
    ~LocalPlayer() = default;

public:
    LocalPlayer(LocalPlayer const&) = delete;
    void operator=(const LocalPlayer&) = delete;
    static LocalPlayer* getInstance();

    // Changed to static to resolve C2352 build errors
    static Otc::PlayerStates getStates(LocalPlayerPtr localPlayer);

    // Logic for synchronization/busy-gate
    static bool isBusy(LocalPlayerPtr localPlayer);

    double getHealth(LocalPlayerPtr localPlayer);
    double getMaxHealth(LocalPlayerPtr localPlayer);
    double getFreeCapacity(LocalPlayerPtr localPlayer);
    double getLevel(LocalPlayerPtr localPlayer);
    double getMana(LocalPlayerPtr localPlayer);
    double getMaxMana(LocalPlayerPtr localPlayer);
    double getSoul(LocalPlayerPtr localPlayer);
    double getStamina(LocalPlayerPtr localPlayer);

    uint32_t getManaShield(LocalPlayerPtr localPlayer);
    bool hasEquippedItemId(LocalPlayerPtr localPlayer, uint16_t itemId, int tier);
    ItemPtr getInventoryItem(LocalPlayerPtr localPlayer, Otc::InventorySlot inventorySlot);
    int getInventoryCount(LocalPlayerPtr localPlayer, uint16_t itemId, int tier);
    bool hasSight(LocalPlayerPtr localPlayer, const Position& pos);

    bool isAutoWalking(LocalPlayerPtr localPlayer);
    void stopAutoWalk(LocalPlayerPtr localPlayer);
    bool autoWalk(LocalPlayerPtr localPlayer, const Position& destination, bool retry = false);
    void setLightHack(LocalPlayerPtr localPlayer, uint16_t lightLevel);
};

#endif //LOCALPLAYER_H