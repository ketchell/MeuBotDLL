#ifndef CREATURE_H
#define CREATURE_H
#define g_creature Creature::getInstance()
#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"


class Creature {
    static Creature* instance;
    static std::mutex mutex;
protected:
    Creature()=default;
    ~Creature()= default;
public:
    Creature(Creature const&) = delete;
    void operator=(const Creature&) = delete;
    static Creature* getInstance();

    std::string getName(CreaturePtr creature);
    uint8_t getManaPercent(CreaturePtr creature);
    uint8_t getHealthPercent(CreaturePtr creature);
    Otc::PlayerSkulls getSkull(CreaturePtr creature);
    Otc::Direction getDirection(CreaturePtr creature);
    bool isDead(CreaturePtr creature);
    bool isWalking(CreaturePtr creature);
    bool canBeSeen(CreaturePtr creature);
    bool isCovered(CreaturePtr creature);
    bool canShoot(CreaturePtr creature, int distance);

};



#endif //CREATURE_H
