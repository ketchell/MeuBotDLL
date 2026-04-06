#include "Game.h"
#include <thread>
#include <chrono>
#include <atomic>
#include "Creature.h"

// Global Atomic Counter for Synchronization with Python/gRPC
std::atomic<int32_t> g_pendingActions{ 0 };

Game* Game::instance{ nullptr };
std::mutex Game::mutex;

Game* Game::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new Game();
    }
    return instance;
}

void Game::safeLogout() {
    uintptr_t funcAddr = SingletonFunctions["g_game.safeLogout"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.safeLogout"].second;
    typedef void(gameCall* SafeLogout)(uintptr_t RCX);
    auto function = reinterpret_cast<SafeLogout>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        function(thisPtr);
        g_pendingActions--;
        });
}

void Game::walk(Otc::Direction direction)
{
    uintptr_t funcAddr = SingletonFunctions["g_game.walk"].first;
    if (!funcAddr) return;
    uintptr_t thisPtr = SingletonFunctions["g_game.walk"].second;

    bool isThiscall = (*reinterpret_cast<const uint8_t*>(funcAddr) == 0x55);
    int jitter = (rand() % 40) + 10;

    g_pendingActions++;
    if (isThiscall) {
        typedef bool(gameCall* Walk_tc)(uintptr_t, Otc::Direction);
        auto fn = reinterpret_cast<Walk_tc>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, direction, jitter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(jitter));
            fn(thisPtr, direction);
            g_pendingActions--;
            });
    }
    else {
        typedef bool(__cdecl* Walk_cd)(uintptr_t, Otc::Direction);
        auto fn = reinterpret_cast<Walk_cd>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, direction, jitter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(jitter));
            fn(thisPtr, direction);
            g_pendingActions--;
            });
    }
}

bool Game::autoWalk(const std::vector<Otc::Direction>& dirs, const Position& startPos) {
    uintptr_t funcAddr = SingletonFunctions["g_game.autoWalk"].first;
    if (!funcAddr) return false;
    uintptr_t thisPtr = SingletonFunctions["g_game.autoWalk"].second;

    bool isThiscall = (*reinterpret_cast<const uint8_t*>(funcAddr) == 0x55);

    g_pendingActions++;
    if (isThiscall) {
        typedef bool(gameCall* AutoWalk_tc)(uintptr_t, const std::vector<Otc::Direction>*, const Position*);
        auto fn = reinterpret_cast<AutoWalk_tc>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, dirs, startPos]() -> bool {
            bool ret = fn(thisPtr, &dirs, &startPos);
            g_pendingActions--;
            return ret;
            });
    }
    else {
        typedef bool(__cdecl* AutoWalk_cd)(uintptr_t, const std::vector<Otc::Direction>*, const Position*);
        auto fn = reinterpret_cast<AutoWalk_cd>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, dirs, startPos]() -> bool {
            bool ret = fn(thisPtr, &dirs, &startPos);
            g_pendingActions--;
            return ret;
            });
    }
}

void Game::walkBatch(const std::vector<Otc::Direction>& dirs, int maxSteps) {
    uintptr_t funcAddr = SingletonFunctions["g_game.walk"].first;
    if (!funcAddr) return;
    uintptr_t thisPtr = SingletonFunctions["g_game.walk"].second;

    int n = (int)dirs.size() < maxSteps ? (int)dirs.size() : maxSteps;
    if (n <= 0) return;

    std::vector<Otc::Direction> steps(dirs.begin(), dirs.begin() + n);

    bool isThiscall = (*reinterpret_cast<const uint8_t*>(funcAddr) == 0x55);
    g_pendingActions++;
    if (isThiscall) {
        typedef bool(gameCall* Walk_tc)(uintptr_t, Otc::Direction);
        auto fn = reinterpret_cast<Walk_tc>(funcAddr);
        g_dispatcher->scheduleEventEx([fn, thisPtr, steps]() {
            for (auto dir : steps) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20 + (rand() % 30)));
                fn(thisPtr, dir);
            }
            g_pendingActions--;
            });
    }
    else {
        typedef bool(__cdecl* Walk_cd)(uintptr_t, Otc::Direction);
        auto fn = reinterpret_cast<Walk_cd>(funcAddr);
        g_dispatcher->scheduleEventEx([fn, thisPtr, steps]() {
            for (auto dir : steps) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20 + (rand() % 30)));
                fn(thisPtr, dir);
            }
            g_pendingActions--;
            });
    }
}

bool Game::autoWalkDest(const Position& dest, bool retry) {
    uintptr_t funcAddr = SingletonFunctions["g_game.autoWalk"].first;
    if (!funcAddr) return false;
    uintptr_t thisPtr = SingletonFunctions["g_game.autoWalk"].second;

    bool isThiscall = (*reinterpret_cast<const uint8_t*>(funcAddr) == 0x55);
    g_pendingActions++;
    if (isThiscall) {
        typedef bool(gameCall* AutoWalkDest_tc)(uintptr_t, const Position*, bool);
        auto fn = reinterpret_cast<AutoWalkDest_tc>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, dest, retry]() -> bool {
            bool ret = fn(thisPtr, &dest, retry);
            g_pendingActions--;
            return ret;
            });
    }
    else {
        typedef bool(__cdecl* AutoWalkDest_cd)(uintptr_t, const Position*, bool);
        auto fn = reinterpret_cast<AutoWalkDest_cd>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, dest, retry]() -> bool {
            bool ret = fn(thisPtr, &dest, retry);
            g_pendingActions--;
            return ret;
            });
    }
}

void Game::turn(Otc::Direction direction)
{
    static long lastTurnRequest = 0;
    if (GetTickCount() - lastTurnRequest < 300) return;
    lastTurnRequest = GetTickCount();

    uintptr_t funcAddr = SingletonFunctions["g_game.turn"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.turn"].second;
    typedef void(gameCall* Turn)(uintptr_t RCX, Otc::Direction RDX);
    auto function = reinterpret_cast<Turn>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, direction]() {
        function(thisPtr, direction);
        g_pendingActions--;
        });
}

void Game::stop() {
    uintptr_t funcAddr = SingletonFunctions["g_game.stop"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.stop"].second;
    typedef void(gameCall* Stop)(uintptr_t RCX);
    auto function = reinterpret_cast<Stop>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        function(thisPtr);
        g_pendingActions--;
        });
}

void Game::move(const ThingPtr& thing, const Position& toPos, int count) {
    if (!thing) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.move"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.move"].second;
    typedef void(gameCall* Move)(uintptr_t RCX, const ThingPtr* RDX, const Position* R8, int count);
    auto function = reinterpret_cast<Move>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, thing, toPos, count]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (rand() % 150)));
        function(thisPtr, &thing, &toPos, count);
        g_pendingActions--;
        });
}

void Game::moveToParentContainer(const ThingPtr& thing, const int count) {
    if (!thing) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.moveToParentContainer"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.moveToParentContainer"].second;
    typedef void(gameCall* MoveToParentContainer)(uintptr_t RCX, const ThingPtr* RDX, const int count);
    auto function = reinterpret_cast<MoveToParentContainer>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, thing, count]() {
        function(thisPtr, &thing, count);
        g_pendingActions--;
        });
}

void Game::use(const ThingPtr& thing) {
    if (!thing) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.use"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.use"].second;
    typedef void(gameCall* Use)(uintptr_t RCX, const ThingPtr* RDX);
    auto function = reinterpret_cast<Use>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, thing]() {
        function(thisPtr, &thing);
        g_pendingActions--;
        });
}

void Game::useWith(const ItemPtr& item, const ThingPtr& toThing) {
    if (!item || !toThing) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.useWith"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.useWith"].second;
    typedef void(gameCall* UseWith)(uintptr_t RCX, const ItemPtr* RDX, const ThingPtr* R8);
    auto function = reinterpret_cast<UseWith>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, item, toThing]() {
        function(thisPtr, &item, &toThing);
        g_pendingActions--;
        });
}

void Game::useInventoryItem(const uint16_t itemId) {
    if (!itemId) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.useInventoryItem"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.useInventoryItem"].second;
    typedef void(gameCall* UseInventoryItem)(uintptr_t RCX, const uint16_t itemId);
    auto function = reinterpret_cast<UseInventoryItem>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, itemId]() {
        function(thisPtr, itemId);
        g_pendingActions--;
        });
}

void Game::useInventoryItemWith(const uint16_t itemId, const ThingPtr& toThing) {
    if (!toThing) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.useInventoryItemWith"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.useInventoryItemWith"].second;
    typedef void(gameCall* UseInventoryItemWith)(uintptr_t RCX, const uint16_t itemId, const ThingPtr* toThing);
    auto function = reinterpret_cast<UseInventoryItemWith>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, itemId, toThing]() {
        function(thisPtr, itemId, &toThing);
        g_pendingActions--;
        });
}

ItemPtr Game::findItemInContainers(uint32_t itemId, int subType, uint8_t tier) {
    uintptr_t funcAddr = SingletonFunctions["g_game.findItemInContainers"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.findItemInContainers"].second;
    typedef void(gameCall* FindItemInContainers)(uintptr_t RCX, ItemPtr* RDX, uint32_t R8, int R9, uint8_t tier);
    auto function = reinterpret_cast<FindItemInContainers>(funcAddr);
    if (!function) return {};

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, itemId, subType, tier]() {
        ItemPtr result;
        function(thisPtr, &result, itemId, subType, tier);
        g_pendingActions--;
        return result;
        });
}

int Game::open(const ItemPtr& item, const ContainerPtr& previousContainer) {
    if (!item) return 0;
    uintptr_t funcAddr = SingletonFunctions["g_game.open"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.open"].second;
    typedef int(gameCall* Open)(uintptr_t RCX, const ItemPtr* RDX, const ContainerPtr* R8);
    auto function = reinterpret_cast<Open>(funcAddr);
    if (!function) return 0;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, item, previousContainer]() {
        auto ret = function(thisPtr, &item, &previousContainer);
        // Wait for the server to respond with container contents before clearing
        // the pending counter — prevents the Python layer from looting too early.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        g_pendingActions--;
        return ret;
        });
}

void Game::openParent(const ContainerPtr& container) {
    if (!container) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.openParent"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.openParent"].second;
    typedef void(gameCall* OpenParent)(uintptr_t RCX, const ContainerPtr* RDX);
    auto function = reinterpret_cast<OpenParent>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, container]() {
        function(thisPtr, &container);
        g_pendingActions--;
        });
}

void Game::close(const ContainerPtr& container) {
    if (!container) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.close"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.close"].second;
    typedef void(gameCall* Close)(uintptr_t RCX, const ContainerPtr* RDX);
    auto function = reinterpret_cast<Close>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, container]() {
        function(thisPtr, &container);
        g_pendingActions--;
        });
}

void Game::refreshContainer(const ContainerPtr& container) {
    if (!container) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.refreshContainer"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.refreshContainer"].second;
    typedef void(gameCall* RefreshContainer)(uintptr_t RCX, const ContainerPtr* RDX);
    auto function = reinterpret_cast<RefreshContainer>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, container]() {
        function(thisPtr, &container);
        g_pendingActions--;
        });
}

void Game::attack(const CreaturePtr& creature, bool cancel) {
    if (!creature) return;

    static uintptr_t activeTargetId = 0;
    if (!cancel && activeTargetId == creature) {
        // The game client may have cleared the target internally (death, range, etc.)
        // without us calling cancelAttack(). If so, reset the lock so the packet fires.
        if (!isAttacking()) activeTargetId = 0;
        else return;
    }
    activeTargetId = cancel ? 0 : creature;

    uintptr_t funcAddr = SingletonFunctions["g_game.attack"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.attack"].second;
    typedef void(gameCall* Attack)(uintptr_t RCX, CreaturePtr RDX, bool R8);
    auto function = reinterpret_cast<Attack>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, creature, cancel]() {
        function(thisPtr, creature, cancel);
        g_pendingActions--;
        });
}

void Game::cancelAttack() {
    uintptr_t funcAddr = SingletonFunctions["g_game.cancelAttack"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.cancelAttack"].second;
    typedef void(gameCall* CancelAttack)(uintptr_t RCX);
    auto function = reinterpret_cast<CancelAttack>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        function(thisPtr);
        g_pendingActions--;
        });
}

void Game::follow(const CreaturePtr& creature) {
    if (!creature) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.follow"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.follow"].second;
    typedef void(gameCall* Follow)(uintptr_t RCX, const CreaturePtr* RDX);
    auto function = reinterpret_cast<Follow>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, creature]() {
        function(thisPtr, &creature);
        g_pendingActions--;
        });
}

void Game::cancelAttackAndFollow() {
    uintptr_t funcAddr = SingletonFunctions["g_game.cancelAttackAndFollow"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.cancelAttackAndFollow"].second;
    typedef void(gameCall* CancelAttackAndFollow)(uintptr_t RCX);
    auto function = reinterpret_cast<CancelAttackAndFollow>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        function(thisPtr);
        g_pendingActions--;
        });
}

void Game::talk(const std::string& message) {
    uintptr_t funcAddr = SingletonFunctions["g_game.talk"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.talk"].second;
    typedef void(gameCall* Talk)(uintptr_t RCX, const std::string& message);
    auto function = reinterpret_cast<Talk>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, message]() {
        function(thisPtr, message);
        g_pendingActions--;
        });
}

void Game::talkChannel(const Otc::MessageMode mode, const uint16_t channelId, const std::string& message) {
    uintptr_t funcAddr = SingletonFunctions["g_game.talkChannel"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.talkChannel"].second;
    typedef void(gameCall* TalkChannel)(uintptr_t RCX, const Otc::MessageMode RDX, const uint16_t channelId, const std::string& message);
    auto function = reinterpret_cast<TalkChannel>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, mode, channelId, message]() {
        function(thisPtr, mode, channelId, message);
        g_pendingActions--;
        });
}

void Game::talkPrivate(Otc::MessageMode msgMode, std::string receiver, std::string message) {
    uintptr_t funcAddr = SingletonFunctions["g_game.talkPrivate"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.talkPrivate"].second;
    typedef void(gameCall* TalkPrivate)(uintptr_t RCX, Otc::MessageMode msgMode, std::string* receiver, std::string* message);
    auto function = reinterpret_cast<TalkPrivate>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    g_dispatcher->scheduleEventEx([function, thisPtr, receiver, message]() mutable {
        Otc::MessageMode m = Otc::MessageMode::MessagePrivateTo;
        function(thisPtr, m, &receiver, &message);
        g_pendingActions--;
        });
}

void Game::openPrivateChannel(const std::string& receiver) {
    uintptr_t funcAddr = SingletonFunctions["g_game.openPrivateChannel"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.openPrivateChannel"].second;
    typedef void(gameCall* OpenPrivateChannel)(uintptr_t RCX, const std::string& receiver);
    auto function = reinterpret_cast<OpenPrivateChannel>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, receiver]() {
        function(thisPtr, receiver);
        g_pendingActions--;
        });
}

Otc::FightModes Game::getFightMode() {
    uintptr_t funcAddr = SingletonFunctions["g_game.getFightMode"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getFightMode"].second;
    typedef Otc::FightModes(gameCall* GetFightMode)(uintptr_t RCX);
    auto function = reinterpret_cast<GetFightMode>(funcAddr);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        return function(thisPtr);
        });
}

Otc::ChaseModes Game::getChaseMode() {
    uintptr_t funcAddr = SingletonFunctions["g_game.getChaseMode"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getChaseMode"].second;
    typedef Otc::ChaseModes(gameCall* GetChaseMode)(uintptr_t RCX);
    auto function = reinterpret_cast<GetChaseMode>(funcAddr);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        return function(thisPtr);
        });
}

void Game::setChaseMode(Otc::ChaseModes mode) {
    uintptr_t funcAddr = SingletonFunctions["g_game.setChaseMode"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.setChaseMode"].second;
    typedef void(gameCall* SetChaseMode)(uintptr_t RCX, Otc::ChaseModes mode);
    auto function = reinterpret_cast<SetChaseMode>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, mode]() {
        function(thisPtr, mode);
        g_pendingActions--;
        });
}

void Game::setFightMode(Otc::FightModes mode) {
    uintptr_t funcAddr = SingletonFunctions["g_game.setFightMode"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.setFightMode"].second;
    typedef void(gameCall* SetFightMode)(uintptr_t RCX, Otc::FightModes mode);
    auto function = reinterpret_cast<SetFightMode>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, mode]() {
        function(thisPtr, mode);
        g_pendingActions--;
        });
}

void Game::buyItem(const ItemPtr& item, const uint16_t amount, const bool ignoreCapacity, const bool buyWithBackpack) {
    uintptr_t funcAddr = SingletonFunctions["g_game.buyItem"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.buyItem"].second;
    typedef void(gameCall* BuyItem)(uintptr_t RCX, const ItemPtr* item, uint16_t amount, bool ignoreCapacity, bool buyWithBackpack);
    auto function = reinterpret_cast<BuyItem>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, item, amount, ignoreCapacity, buyWithBackpack]() {
        function(thisPtr, &item, amount, ignoreCapacity, buyWithBackpack);
        g_pendingActions--;
        });
}

void Game::sellItem(const ItemPtr& item, const uint16_t amount, const bool ignoreEquipped) {
    uintptr_t funcAddr = SingletonFunctions["g_game.sellItem"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.sellItem"].second;
    typedef void(gameCall* SellItem)(uintptr_t RCX, const ItemPtr* item, uint16_t amount, bool ignoreEquipped);
    auto function = reinterpret_cast<SellItem>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, item, amount, ignoreEquipped]() {
        function(thisPtr, &item, amount, ignoreEquipped);
        g_pendingActions--;
        });
}

void Game::equipItem(const ItemPtr& item) {
    if (!item) return;
    uintptr_t funcAddr = SingletonFunctions["g_game.equipItem"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.equipItem"].second;
    typedef void(gameCall* EquipItem)(uintptr_t RCX, const ItemPtr* item);
    auto function = reinterpret_cast<EquipItem>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, item]() {
        function(thisPtr, &item);
        g_pendingActions--;
        });
}

void Game::equipItemId(uint16_t itemId, uint8_t tier) {
    uintptr_t funcAddr = SingletonFunctions["g_game.equipItemId"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.equipItemId"].second;
    typedef void(gameCall* EquipItemId)(uintptr_t RCX, uint16_t itemId, uint8_t tier);
    auto function = reinterpret_cast<EquipItemId>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, itemId, tier]() {
        function(thisPtr, itemId, tier);
        g_pendingActions--;
        });
}

void Game::mount(bool mountStatus) {
    uintptr_t funcAddr = SingletonFunctions["g_game.mount"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.mount"].second;
    typedef void(gameCall* Mount)(uintptr_t RCX, bool mountStatus);
    auto function = reinterpret_cast<Mount>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, mountStatus]() {
        function(thisPtr, mountStatus);
        g_pendingActions--;
        });
}

void Game::changeMapAwareRange(const uint8_t xrange, const uint8_t yrange) {
    uintptr_t funcAddr = SingletonFunctions["g_game.changeMapAwareRange"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.changeMapAwareRange"].second;
    typedef void(gameCall* ChangeMapAwareRange)(uintptr_t RCX, uint8_t xrange, uint8_t yrange);
    auto function = reinterpret_cast<ChangeMapAwareRange>(funcAddr);
    if (!function) return;

    g_pendingActions++;
    return g_dispatcher->scheduleEventEx([function, thisPtr, xrange, yrange]() {
        function(thisPtr, xrange, yrange);
        g_pendingActions--;
        });
}

bool Game::canPerformGameAction() {
    uintptr_t funcAddr = SingletonFunctions["g_game.canPerformGameAction"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.canPerformGameAction"].second;
    typedef bool(gameCall* CanPerformGameAction)(uintptr_t RCX, void* RDX);
    auto function = reinterpret_cast<CanPerformGameAction>(funcAddr);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        void* pMysteryPtr = nullptr;
        return function(thisPtr, &pMysteryPtr);
        });
}

bool Game::isOnline() {
    uintptr_t funcAddr = SingletonFunctions["g_game.isOnline"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.isOnline"].second;
    typedef bool(gameCall* IsOnline)(uintptr_t RCX, void* RDX);
    auto function = reinterpret_cast<IsOnline>(funcAddr);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        void* pMysteryPtr = nullptr;
        return function(thisPtr, &pMysteryPtr);
        });
}

bool Game::isAttacking() {
    uintptr_t funcAddr = SingletonFunctions["g_game.isAttacking"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.isAttacking"].second;
    typedef bool(gameCall* IsAttacking)(uintptr_t RCX, void* RDX);
    auto function = reinterpret_cast<IsAttacking>(funcAddr);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        void* pMysteryPtr = nullptr;
        return function(thisPtr, &pMysteryPtr);
        });
}

bool Game::isFollowing() {
    uintptr_t funcAddr = SingletonFunctions["g_game.isFollowing"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.isFollowing"].second;
    typedef bool(gameCall* IsFollowing)(uintptr_t RCX, void* RDX);
    auto function = reinterpret_cast<IsFollowing>(funcAddr);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        void* pMysteryPtr = nullptr;
        return function(thisPtr, &pMysteryPtr);
        });
}

ContainerPtr Game::getContainer(int index) {
    uintptr_t funcAddr = SingletonFunctions["g_game.getContainer"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getContainer"].second;
    typedef void(gameCall* GetContainer)(uintptr_t RCX, ContainerPtr* RDX, int idx);
    auto function = reinterpret_cast<GetContainer>(funcAddr);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr, index]() {
        ContainerPtr result;
        function(thisPtr, &result, index);
        return result;
        });
}

std::vector<ContainerPtr> Game::getContainers() {
    uintptr_t funcAddr = SingletonFunctions["g_game.getContainers"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getContainers"].second;
    typedef void(gameCall* GetContainers)(uintptr_t RCX, std::map<int, uintptr_t>* RDX);
    auto function = reinterpret_cast<GetContainers>(funcAddr);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        std::map<int, uintptr_t> result;
        function(thisPtr, &result);
        std::vector<ContainerPtr> containers;
        for (auto items : result) {
            containers.push_back(ContainerPtr(items.second));
        }
        return containers;
        });
}

CreaturePtr Game::getAttackingCreature() {
    uintptr_t funcAddr = SingletonFunctions["g_game.getAttackingCreature"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getAttackingCreature"].second;
    typedef void(gameCall* GetAttackingCreature)(uintptr_t RCX, CreaturePtr* RDX);
    auto function = reinterpret_cast<GetAttackingCreature>(funcAddr);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        CreaturePtr result;
        function(thisPtr, &result);
        return result;
        });
}

CreaturePtr Game::getFollowingCreature() {
    uintptr_t funcAddr = SingletonFunctions["g_game.getFollowingCreature"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getFollowingCreature"].second;
    typedef void(gameCall* GetFollowingCreature)(uintptr_t RCX, CreaturePtr* RDX);
    auto function = reinterpret_cast<GetFollowingCreature>(funcAddr);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        CreaturePtr result;
        function(thisPtr, &result);
        return result;
        });
}

LocalPlayerPtr Game::getLocalPlayer() {
    uintptr_t funcAddr = SingletonFunctions["g_game.getLocalPlayer"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getLocalPlayer"].second;
    typedef void(gameCall* GetLocalPlayer)(uintptr_t RCX, LocalPlayerPtr* RDX);
    auto function = reinterpret_cast<GetLocalPlayer>(funcAddr);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        LocalPlayerPtr result;
        function(thisPtr, &result);
        return result;
        });
}

int Game::getClientVersion() {
    uintptr_t funcAddr = SingletonFunctions["g_game.getClientVersion"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getClientVersion"].second;
    typedef int(gameCall* GetClientVersion)(uintptr_t RCX);
    auto function = reinterpret_cast<GetClientVersion>(funcAddr);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        return function(thisPtr);
        });
}

std::string Game::getCharacterName() {
    uintptr_t funcAddr = SingletonFunctions["g_game.getCharacterName"].first;
    uintptr_t thisPtr  = SingletonFunctions["g_game.getCharacterName"].second;
    typedef void(gameCall* GetCharacterName)(uintptr_t RCX, std::string* RDX);
    auto function = reinterpret_cast<GetCharacterName>(funcAddr);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr]() {
        std::string result;
        function(thisPtr, &result);
        return result;
        });
}
