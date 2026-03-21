#ifndef THING_H
#define THING_H
#define g_thing Thing::getInstance()
#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"


class Thing {
    static Thing* instance;
    static std::mutex mutex;
protected:
    Thing()=default;
    ~Thing()=default;
public:
    Thing(Thing const&) = delete;
    void operator=(const Thing&) = delete;
    static Thing* getInstance();

    uint32_t getId(ThingPtr thing);
    Position getPosition(ThingPtr thing);
    ContainerPtr getParentContainer(ThingPtr thing);
    bool isItem(ThingPtr thing);
    bool isMonster(ThingPtr thing);
    bool isNpc(ThingPtr thing);
    bool isCreature(ThingPtr thing);
    bool isPlayer(ThingPtr thing);
    bool isLocalPlayer(ThingPtr thing);
    bool isContainer(ThingPtr thing);
    bool isUsable(ThingPtr thing);
    bool isLyingCorpse(ThingPtr thing);
};



#endif //THING_H
