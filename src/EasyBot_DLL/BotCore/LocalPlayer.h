#ifndef LOCALPLAYER_H
#define LOCALPLAYER_H
#define g_localPlayer LocalPlayer::getInstance()
#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"


class LocalPlayer{
    static LocalPlayer* instance;
    static std::mutex mutex;
protected:
    LocalPlayer()=default;
    ~LocalPlayer()= default;
public:
    LocalPlayer(LocalPlayer const&) = delete;
    void operator=(const LocalPlayer&) = delete;
    static LocalPlayer* getInstance();

    Otc::PlayerStates getStates(LocalPlayerPtr localPlayer);
    double getHealth(LocalPlayerPtr localPlayer);
    double getMaxHealth(LocalPlayerPtr localPlayer);
    double getFreeCapacity(LocalPlayerPtr localPlayer);
    double getLevel(LocalPlayerPtr localPlayer);
    double getMana(LocalPlayerPtr localPlayer);
    double getMaxMana(LocalPlayerPtr localPlayer);
    double getSoul(LocalPlayerPtr localPlayer);
    double getStamina(LocalPlayerPtr localPlayer);
    ItemPtr getInventoryItem(LocalPlayerPtr localPlayer, Otc::InventorySlot inventorySlot);
    int getInventoryCount(LocalPlayerPtr localPlayer, uint16_t itemId, int tier);
    bool hasSight(LocalPlayerPtr localPlayer, const Position &pos);
    bool isAutoWalking(LocalPlayerPtr localPlayer);
    void stopAutoWalk(LocalPlayerPtr localPlayer);
    bool autoWalk(LocalPlayerPtr localPlayer, const Position& destination, bool retry = false);
    void setLightHack(LocalPlayerPtr localPlayer, uint16_t lightLevel);

};




#endif //LOCALPLAYER_H
