#ifndef PROTO_FUNCTIONS_CLIENT_H
#define PROTO_FUNCTIONS_CLIENT_H
#include <grpcpp/grpcpp.h>
#include "bot.grpc.pb.h"
#include "const.h"
#include "google/protobuf/empty.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using google::protobuf::Empty;
using google::protobuf::UInt64Value;
using google::protobuf::Int32Value;
using namespace bot;

#define proto BotClient::getInstance()


class BotClient {
private:
    static BotClient* instance;
    static std::mutex mutex;
protected:
    BotClient();
    ~BotClient()= default;
public:
    BotClient(BotClient const&) = delete;
    void operator=(const BotClient&) = delete;
    static BotClient* getInstance();
    std::vector<int> availablePorts();
    void connect(int port);
    void disconnect(int port);

    // Functions:
    // Container
    uintptr_t getItem(uintptr_t container, uint8_t slot);
    std::deque<uintptr_t> getItems(uintptr_t container);
    int getItemsCount(uintptr_t container);
    Position getSlotPosition(uintptr_t container, int slot);
    std::string getContainerName(uintptr_t container);
    int getContainerId(uintptr_t container);
    uintptr_t getContainerItem(uintptr_t container);
    bool hasParent(uintptr_t container);
    int getCapacity(uintptr_t container);
    int getFirstIndex(uintptr_t container);

    // Creature
    std::string getCreatureName(uintptr_t creature);
    uint8_t getHealthPercent(uintptr_t creature);
    uint8_t getManaPercent(uintptr_t creature);
    uint8_t getSkull(uintptr_t creature);
    Otc::Direction getDirection(uintptr_t creature);
    bool isWalking(uintptr_t creature);
    bool isDead(uintptr_t creature);
    bool canBeSeen(uintptr_t creature);
    bool isCovered(uintptr_t creature);
    bool canShoot(uintptr_t creature, int distance);

    // Game
    void safeLogout();
    void walk(Otc::Direction direction);
    void autoWalkGame(const std::vector<Otc::Direction> &dirs, const Position &startPos);
    void turn(Otc::Direction direction);
    void stop();
    void move(const uintptr_t &thing, const Position& toPos, int count);
    void moveToParentContainer(const uintptr_t& thing, const int count);
    void use(const uintptr_t &thing);
    void useWith(const uintptr_t &item, const uintptr_t &toThing);
    void useInventoryItem(const uint16_t itemId);
    void useInventoryItemWith(const uint16_t itemId, const uintptr_t& toThing);
    uintptr_t findItemInContainers(uint32_t itemId, int subType, int tier);
    int open(const uintptr_t &item, const uintptr_t &previousContainer);
    void openParent(const uintptr_t &container);
    void close(const uintptr_t &container);
    void refreshContainer(const uintptr_t &container);
    void attack(const uintptr_t &creature, bool cancel);
    void cancelAttack();
    void follow(const uintptr_t &creature);
    void cancelAttackAndFollow();
    void talk(const std::string& message);
    void talkChannel(const Otc::MessageMode mode, const uint16_t channelId, const std::string& message);
    void talkPrivate(const Otc::MessageMode msgMode, const std::string& receiver, const std::string& message);
    void openPrivateChannel(const std::string& receiver);
    int getChaseMode();
    int getFightMode();
    void setChaseMode(Otc::ChaseModes mode);
    void setFightMode(Otc::FightModes mode);
    void buyItem(const uintptr_t& item, const uint16_t amount, const bool ignoreCapacity, const bool buyWithBackpack);
    void sellItem(const uintptr_t& item, const uint16_t amount, const bool ignoreEquipped);
    void equipItem(const uintptr_t &item);
    void equipItemId(uint16_t itemId, int tier);
    void mount(bool mountStatus);
    void changeMapAwareRange(const uint8_t xrange, const uint8_t yrange);
    bool canPerformGameAction();
    bool isOnline();
    bool isAttacking();
    bool isFollowing();
    uintptr_t getContainer(int index);
    std::vector<uintptr_t> getContainers();
    uintptr_t getAttackingCreature();
    uintptr_t getFollowingCreature();
    uintptr_t getLocalPlayer();
    int getClientVersion();
    std::string getCharacterName();

    // Item
    int getCount(uintptr_t item);
    int getSubType(uintptr_t item);
    uint32_t getItemId(uintptr_t item);
    std::string getTooltip(uintptr_t item);
    uint32_t getDurationTime(uintptr_t item);
    std::string getItemName(uintptr_t item);
    std::string getDescription(uintptr_t item);
    uint8_t getTier(uintptr_t item);
    std::string getText(uintptr_t item);

    // LocalPlayer
    uint32_t getStates(uintptr_t localPlayer);
    double getHealth(uintptr_t localPlayer);
    double getMaxHealth(uintptr_t localPlayer);
    double getFreeCapacity(uintptr_t localPlayer);
    uint16_t getLevel(uintptr_t localPlayer);
    double getMana(uintptr_t localPlayer);
    double getMaxMana(uintptr_t localPlayer);
    uint32_t getManaShield(uintptr_t localPlayer);
    uint8_t getSoul(uintptr_t localPlayer);
    uint16_t getStamina(uintptr_t localPlayer);
    uintptr_t getInventoryItem(uintptr_t localPlayer, Otc::InventorySlot inventorySlot);
    bool hasEquippedItemId(uintptr_t localPlayer, uint16_t itemId, int tier);
    int getInventoryCount(uintptr_t localPlayer, uint16_t itemId, int tier);
    bool hasSight(uintptr_t localPlayer, const Position &pos);
    bool isAutoWalking(uintptr_t localPlayer);
    void stopAutoWalk(uintptr_t localPlayer);
    bool autoWalk(uintptr_t localPlayer, const Position& destination, bool retry = false);
    void setLightHack(uintptr_t localPlayer, uint16_t lightLevel);

    // Map
    uintptr_t getTile(Position tilePos);
    std::vector<uintptr_t> getSpectators(const Position &centerPos, bool multiFloor = false);
    std::vector<Otc::Direction> findPath(const Position& startPos, const Position& goalPos, int maxComplexity, int flags);
    bool isSightClear(const Position& fromPos, const Position& toPos);

    // Thing
    uint32_t getId(uintptr_t thing);
    Position getPosition(uintptr_t thing);
    uintptr_t getParentContainer(uintptr_t thing);
    bool isItem(uintptr_t thing);
    bool isMonster(uintptr_t thing);
    bool isNpc(uintptr_t thing);
    bool isCreature(uintptr_t thing);
    bool isPlayer(uintptr_t thing);
    bool isLocalPlayer(uintptr_t thing);
    bool isContainer(uintptr_t thing);
    bool isUsable(uintptr_t thing);
    bool isLyingCorpse(uintptr_t thing);

    // Tile
    uintptr_t getTopThing(uintptr_t tile);
    uintptr_t getTopUseThing(uintptr_t tile);
    std::vector<uintptr_t> getTileItems(uintptr_t tile);
    bool isWalkable(uintptr_t tile, bool ignoreCreatures);

    // Custom Functions
    void clearMessages();
    void dropMessages(int messageNumber);


private:
    std::unique_ptr<BotService::Stub> stub;
};



#endif //PROTO_FUNCTIONS_CLIENT_H
