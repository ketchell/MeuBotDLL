#ifndef GAME_H
#define GAME_H
#define g_game Game::getInstance()
#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"
#include <atomic>

extern std::atomic<int32_t> g_pendingActions;

class Game {
    static Game* instance;
    static std::mutex mutex;
protected:
    Game()=default;
    ~Game()= default;
public:
    Game(Game const&) = delete;
    void operator=(const Game&) = delete;
    static Game* getInstance();

    void safeLogout();
    void walk(Otc::Direction direction);
    void walkBatch(const std::vector<Otc::Direction>& dirs, int maxSteps);
    bool autoWalk(const std::vector<Otc::Direction> &dirs, const Position &startPos);
    // Lua-wrapper servers bind g_game.autoWalk as LocalPlayer::autoWalk — takes dest + retry.
    bool autoWalkDest(const Position& dest, bool retry);
    void turn(Otc::Direction direction);
    void stop();
    void move(const ThingPtr& thing, const Position& toPos, int count);
    void moveToParentContainer(const ThingPtr& thing, const int count);
    void use(const ThingPtr& thing);
    void useWith(const ItemPtr& item, const ThingPtr& toThing);
    void useInventoryItem(const uint16_t itemId);
    void useInventoryItemWith(const uint16_t itemId, const ThingPtr& toThing);
    ItemPtr findItemInContainers(uint32_t itemId, int subType, uint8_t tier);
    int open(const ItemPtr& item, const ContainerPtr& previousContainer);
    void openParent(const ContainerPtr& container);
    void close(const ContainerPtr& container);
    void refreshContainer(const ContainerPtr& container);
    void attack(const CreaturePtr& creature, bool cancel =false);
    void cancelAttack();
    void follow(const CreaturePtr& creature);
    void cancelAttackAndFollow();
    void talk(const std::string& message);
    void talkChannel(const Otc::MessageMode mode, const uint16_t channelId, const std::string& message);
    void talkPrivate(Otc::MessageMode msgMode, std::string receiver, std::string message);
    void openPrivateChannel(const std::string& receiver);
    Otc::FightModes getFightMode();
    Otc::ChaseModes getChaseMode();
    void setChaseMode(Otc::ChaseModes mode);
    void setFightMode(Otc::FightModes mode);
    void buyItem(const ItemPtr& item, const uint16_t amount, const bool ignoreCapacity, const bool buyWithBackpack);
    void sellItem(const ItemPtr& item, const uint16_t amount, const bool ignoreEquipped = false);
    void equipItem(const ItemPtr& item);
    void equipItemId(uint16_t itemId, uint8_t tier);
    void mount(bool mountStatus);
    void changeMapAwareRange(const uint8_t xrange, const uint8_t yrange);
    bool canPerformGameAction();
    bool isOnline();
    bool isAttacking();
    bool isFollowing();
    ContainerPtr getContainer(int index);
    std::vector<ContainerPtr> getContainers();
    CreaturePtr getAttackingCreature();
    CreaturePtr getFollowingCreature();
    LocalPlayerPtr getLocalPlayer();
    int getClientVersion();
    std::string getCharacterName();

};



#endif //GAME_H
