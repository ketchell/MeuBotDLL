#include "Game.h"


Game* Game::instance{nullptr};
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
    typedef void(gameCall* SafeLogout)(
    uintptr_t RCX
    );
    auto function = reinterpret_cast<SafeLogout>(SingletonFunctions["g_game.safeLogout"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function]() {
        function(SingletonFunctions["g_game.safeLogout"].second);
    });
}

void Game::walk(Otc::Direction direction)
{
    uintptr_t funcAddr = SingletonFunctions["g_game.walk"].first;
    if (!funcAddr) return;
    uintptr_t thisPtr = SingletonFunctions["g_game.walk"].second;

    bool isThiscall = (*reinterpret_cast<const uint8_t*>(funcAddr) == 0x55);
    if (isThiscall) {
        typedef bool(gameCall* Walk_tc)(uintptr_t, Otc::Direction);
        auto fn = reinterpret_cast<Walk_tc>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, direction]() {
            fn(thisPtr, direction);
        });
    } else {
        typedef bool(__cdecl* Walk_cd)(uintptr_t, Otc::Direction);
        auto fn = reinterpret_cast<Walk_cd>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, direction]() {
            fn(thisPtr, direction);
        });
    }
}

bool Game::autoWalk(const std::vector<Otc::Direction>& dirs, const Position &startPos) {
    uintptr_t funcAddr = SingletonFunctions["g_game.autoWalk"].first;
    if (!funcAddr) return false;
    uintptr_t thisPtr = SingletonFunctions["g_game.autoWalk"].second;

    // Detect calling convention by first byte of the function.
    // 0x55 (push ebp) = stdcall/thiscall — thisPtr goes in ECX.
    // Anything else   = cdecl            — all args including thisPtr go on the stack.
    bool isThiscall = (*reinterpret_cast<const uint8_t*>(funcAddr) == 0x55);

    if (isThiscall) {
        typedef bool(gameCall* AutoWalk_tc)(uintptr_t, const std::vector<Otc::Direction>*, const Position*);
        auto fn = reinterpret_cast<AutoWalk_tc>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, dirs, startPos]() -> bool {
            return fn(thisPtr, &dirs, &startPos);
        });
    } else {
        typedef bool(__cdecl* AutoWalk_cd)(uintptr_t, const std::vector<Otc::Direction>*, const Position*);
        auto fn = reinterpret_cast<AutoWalk_cd>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, dirs, startPos]() -> bool {
            return fn(thisPtr, &dirs, &startPos);
        });
    }
}

void Game::walkBatch(const std::vector<Otc::Direction>& dirs, int maxSteps) {
    uintptr_t funcAddr = SingletonFunctions["g_game.walk"].first;
    if (!funcAddr) return;
    uintptr_t thisPtr = SingletonFunctions["g_game.walk"].second;

    int n = (int)dirs.size() < maxSteps ? (int)dirs.size() : maxSteps;
    if (n <= 0) return;

    // Slice out the steps we want to send — captured by value into the lambda.
    std::vector<Otc::Direction> steps(dirs.begin(), dirs.begin() + n);

    // Single game-thread dispatch: all walk calls happen in one event.
    // The game client queues each direction internally and executes them at walking speed.
    bool isThiscall = (*reinterpret_cast<const uint8_t*>(funcAddr) == 0x55);
    if (isThiscall) {
        typedef bool(gameCall* Walk_tc)(uintptr_t, Otc::Direction);
        auto fn = reinterpret_cast<Walk_tc>(funcAddr);
        g_dispatcher->scheduleEventEx([fn, thisPtr, steps]() {
            for (auto dir : steps) fn(thisPtr, dir);
        });
    } else {
        typedef bool(__cdecl* Walk_cd)(uintptr_t, Otc::Direction);
        auto fn = reinterpret_cast<Walk_cd>(funcAddr);
        g_dispatcher->scheduleEventEx([fn, thisPtr, steps]() {
            for (auto dir : steps) fn(thisPtr, dir);
        });
    }
}

bool Game::autoWalkDest(const Position& dest, bool retry) {
    uintptr_t funcAddr = SingletonFunctions["g_game.autoWalk"].first;
    if (!funcAddr) return false;
    uintptr_t thisPtr = SingletonFunctions["g_game.autoWalk"].second;

    bool isThiscall = (*reinterpret_cast<const uint8_t*>(funcAddr) == 0x55);
    if (isThiscall) {
        typedef bool(gameCall* AutoWalkDest_tc)(uintptr_t, const Position*, bool);
        auto fn = reinterpret_cast<AutoWalkDest_tc>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, dest, retry]() -> bool {
            return fn(thisPtr, &dest, retry);
        });
    } else {
        typedef bool(__cdecl* AutoWalkDest_cd)(uintptr_t, const Position*, bool);
        auto fn = reinterpret_cast<AutoWalkDest_cd>(funcAddr);
        return g_dispatcher->scheduleEventEx([fn, thisPtr, dest, retry]() -> bool {
            return fn(thisPtr, &dest, retry);
        });
    }
}

void Game::turn(Otc::Direction direction)
{
    typedef void(gameCall* Turn)(
        uintptr_t RCX,
        Otc::Direction RDX
        );
    auto function = reinterpret_cast<Turn>(SingletonFunctions["g_game.turn"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, direction]() {
        function(SingletonFunctions["g_game.turn"].second, direction);
    });
}

void Game::stop() {
    typedef void(gameCall* Stop)(
        uintptr_t RCX
        );
    auto function = reinterpret_cast<Stop>(SingletonFunctions["g_game.stop"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function]() {
        function(SingletonFunctions["g_game.stop"].second);
    });
}


void Game::move(const ThingPtr &thing, const Position& toPos, int count) {
    if (!thing) return;
    typedef void(gameCall* Move)(
        uintptr_t RCX,
        const ThingPtr *RDX,
        const Position *R8,
        int count
        );
    auto function = reinterpret_cast<Move>(SingletonFunctions["g_game.move"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, thing, toPos, count]() {
        function(SingletonFunctions["g_game.move"].second, &thing, &toPos, count);
    });
}

void Game::moveToParentContainer(const ThingPtr& thing, const int count) {
    if (!thing) return;
    typedef void(gameCall* MoveToParentContainer)(
        uintptr_t RCX,
        const ThingPtr *RDX,
        const int count
        );
    auto function = reinterpret_cast<MoveToParentContainer>(SingletonFunctions["g_game.moveToParentContainer"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, thing, count]() {
        function(SingletonFunctions["g_game.moveToParentContainer"].second, &thing, count);
    });
}

void Game::use(const ThingPtr &thing) {
    if (!thing) return;
    typedef void(gameCall* Use)(
        uintptr_t RCX,
        const ThingPtr *RDX
        );
    auto function = reinterpret_cast<Use>(SingletonFunctions["g_game.use"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, thing]() {
        function(SingletonFunctions["g_game.use"].second, &thing);
    });
}

void Game::useWith(const ItemPtr &item, const ThingPtr &toThing) {
    if (!item || !toThing) return;
    typedef void(gameCall* UseWith)(
        uintptr_t RCX,
        const ItemPtr *RDX,
        const ThingPtr *R8
        );
    auto function = reinterpret_cast<UseWith>(SingletonFunctions["g_game.useWith"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, item, toThing]() {
        function(SingletonFunctions["g_game.useWith"].second, &item, &toThing);
    });
}

void Game::useInventoryItem(const uint16_t itemId) {
    if (!itemId) return;
    typedef void(gameCall* UseInventoryItem)(
        uintptr_t RCX,
        const uint16_t itemId
    );
    auto function = reinterpret_cast<UseInventoryItem>(SingletonFunctions["g_game.useInventoryItem"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, itemId]() {
        function(SingletonFunctions["g_game.useInventoryItem"].second, itemId);
    });
}

void Game::useInventoryItemWith(const uint16_t itemId, const ThingPtr &toThing) {
    if (!toThing) return;
    typedef void(gameCall* UseInventoryItemWith)(
        uintptr_t second_param,
        const uint16_t itemId,
        const ThingPtr *toThing
    );
    auto function = reinterpret_cast<UseInventoryItemWith>(SingletonFunctions["g_game.useInventoryItemWith"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, itemId, toThing]() {
        function(SingletonFunctions["g_game.useInventoryItemWith"].second, itemId, &toThing);
    });
}

ItemPtr Game::findItemInContainers(uint32_t itemId, int subType, uint8_t tier) {
    typedef void(gameCall* FindItemInContainers)(
        uintptr_t RCX,
        ItemPtr* RDX,
        uint32_t R8,
        int R9,
        uint8_t tier
        );
    auto function = reinterpret_cast<FindItemInContainers>(SingletonFunctions["g_game.findItemInContainers"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, itemId, subType, tier]() {
        ItemPtr result;
        function(SingletonFunctions["g_game.findItemInContainers"].second, &result, itemId, subType, tier);
        return result;
    });
}

int Game::open(const ItemPtr &item, const ContainerPtr &previousContainer) {
    if (!item) return 0;
    typedef int(gameCall* Open)(
        uintptr_t RCX,
        const ItemPtr *RDX,
        const ContainerPtr *R8
        );
    auto function = reinterpret_cast<Open>(SingletonFunctions["g_game.open"].first);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, item, previousContainer]() {
            auto ret = function(SingletonFunctions["g_game.open"].second, &item, &previousContainer);
            return ret;
    });
}

void Game::openParent(const ContainerPtr &container) {
    if (!container) return;
    typedef void(gameCall* OpenParent)(
        uintptr_t RCX,
        const ContainerPtr *RDX
        );
    auto function = reinterpret_cast<OpenParent>(SingletonFunctions["g_game.openParent"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, container]() {
        function(SingletonFunctions["g_game.openParent"].second, &container);
    });
}

void Game::close(const ContainerPtr &container) {
    if (!container) return;
    typedef void(gameCall* Close)(
        uintptr_t RCX,
        const ContainerPtr *RDX
        );
    auto function = reinterpret_cast<Close>(SingletonFunctions["g_game.close"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, container]() {
        function(SingletonFunctions["g_game.close"].second, &container);
    });
}

void Game::refreshContainer(const ContainerPtr &container) {
    if (!container) return;
    typedef void(gameCall* RefreshContainer)(
        uintptr_t RCX,
        const ContainerPtr *RDX
        );
    auto function = reinterpret_cast<RefreshContainer>(SingletonFunctions["g_game.refreshContainer"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, container]() {
        function(SingletonFunctions["g_game.refreshContainer"].second, &container);
    });
}

void Game::attack(const CreaturePtr &creature, bool cancel = false) {
    if (!creature) return;
    typedef void(gameCall* Attack)(
        uintptr_t RCX,
        CreaturePtr RDX,
        bool R8
        );
    auto function = reinterpret_cast<Attack>(SingletonFunctions["g_game.attack"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, creature, cancel]() {
        function(SingletonFunctions["g_game.attack"].second, creature, cancel);
    });
}

void Game::cancelAttack() {
    typedef void(gameCall* CancelAttack)(
        uintptr_t RCX
        );
    auto function = reinterpret_cast<CancelAttack>(SingletonFunctions["g_game.cancelAttack"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function]() {
        function(SingletonFunctions["g_game.cancelAttack"].second);
    });
}

void Game::follow(const CreaturePtr& creature) {
    if (!creature) return;
    typedef void(gameCall* Follow)(
        uintptr_t RCX,
        const CreaturePtr* RDX
        );
    auto function = reinterpret_cast<Follow>(SingletonFunctions["g_game.follow"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, creature]() {
        function(SingletonFunctions["g_game.follow"].second, &creature);
    });

}

void Game::cancelAttackAndFollow() {
    typedef void(gameCall* CancelAttackAndFollow)(
        uintptr_t RCX
    );
    auto function = reinterpret_cast<CancelAttackAndFollow>(SingletonFunctions["g_game.cancelAttackAndFollow"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function]() {
        function(SingletonFunctions["g_game.cancelAttackAndFollow"].second);
    });
}


void Game::talk(const std::string& message) {
    typedef void(gameCall *Talk)(
        uintptr_t RCX,
        const std::string& message
    );
    auto function = reinterpret_cast<Talk>(SingletonFunctions["g_game.talk"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, message]() {
        function(SingletonFunctions["g_game.talk"].second, message);
    });
}


void Game::talkChannel(const Otc::MessageMode mode, const uint16_t channelId, const std::string& message) {
    typedef void(gameCall *TalkChannel)(
        uintptr_t RCX,
        const Otc::MessageMode RDX,
        const uint16_t channelId,
        const std::string& message
        );
    auto function = reinterpret_cast<TalkChannel>(SingletonFunctions["g_game.talkChannel"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, mode, channelId, message]() {
            function(SingletonFunctions["g_game.talkChannel"].second, mode, channelId, message);
    });
}

void Game::talkPrivate(Otc::MessageMode msgMode,std::string receiver,std::string message) {
    typedef void(gameCall *TalkPrivate)(
        uintptr_t first_param,
        Otc::MessageMode msgMode,
        std::string *receiver,
        std::string *message
    );
    auto function = reinterpret_cast<TalkPrivate>(SingletonFunctions["g_game.talkPrivate"].first);
    if (!function) return;
    g_dispatcher->scheduleEventEx([function, receiver, message]() mutable {
        Otc::MessageMode m = Otc::MessageMode::MessagePrivateTo;
        function(SingletonFunctions["g_game.talkPrivate"].second, m, &receiver, &message);
    });
}



void Game::openPrivateChannel(const std::string& receiver) {
    typedef void(gameCall *OpenPrivateChannel)(
        uintptr_t RCX,
        const std::string& receiver
    );
    auto function = reinterpret_cast<OpenPrivateChannel>(SingletonFunctions["g_game.openPrivateChannel"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, receiver]() {
            function(SingletonFunctions["g_game.openPrivateChannel"].second, receiver);
    });
}

Otc::FightModes Game::getFightMode() {
    typedef Otc::FightModes(gameCall* GetFightMode)(
        uintptr_t RCX
        );
    auto function = reinterpret_cast<GetFightMode>(SingletonFunctions["g_game.getFightMode"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function]() {
            auto ret = function(SingletonFunctions["g_game.getFightMode"].second);
            return ret;
    });
}

Otc::ChaseModes Game::getChaseMode() {
    typedef Otc::ChaseModes(gameCall* GetChaseMode)(
        uintptr_t RCX
        );
    auto function = reinterpret_cast<GetChaseMode>(SingletonFunctions["g_game.getChaseMode"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function]() {
            auto ret = function(SingletonFunctions["g_game.getChaseMode"].second);
            return ret;
    });
}

void Game::setChaseMode(Otc::ChaseModes mode) {
    typedef void(gameCall *SetChaseMode)(
        uintptr_t RCX,
        Otc::ChaseModes mode
        );
    auto function = reinterpret_cast<SetChaseMode>(SingletonFunctions["g_game.setChaseMode"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, mode]() {
            function(SingletonFunctions["g_game.setChaseMode"].second, mode);
    });
}

void Game::setFightMode(Otc::FightModes mode) {
    typedef void(gameCall *SetFightMode)(
        uintptr_t RCX,
        Otc::FightModes mode
        );
    auto function = reinterpret_cast<SetFightMode>(SingletonFunctions["g_game.setFightMode"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, mode]() {
            function(SingletonFunctions["g_game.setFightMode"].second, mode);
    });
}

void Game::buyItem(const ItemPtr& item, const uint16_t amount, const bool ignoreCapacity, const bool buyWithBackpack) {
    typedef void(gameCall *BuyItem)(
        uintptr_t RCX,
        const ItemPtr *item,
        uint16_t amount,
        bool ignoreCapacity,
        bool buyWithBackpack
        );
    auto function = reinterpret_cast<BuyItem>(SingletonFunctions["g_game.buyItem"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, item, amount, ignoreCapacity, buyWithBackpack]() {
            function(SingletonFunctions["g_game.buyItem"].second, &item, amount, ignoreCapacity, buyWithBackpack);
    });
}

void Game::sellItem(const ItemPtr& item, const uint16_t amount, const bool ignoreEquipped) {
    typedef void(gameCall *SellItem)(
        uintptr_t RCX,
        const ItemPtr *item,
        uint16_t amount,
        bool ignoreEquipped
        );
    auto function = reinterpret_cast<SellItem>(SingletonFunctions["g_game.sellItem"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, item, amount, ignoreEquipped]() {
            function(SingletonFunctions["g_game.sellItem"].second, &item, amount, ignoreEquipped);
    });
}

void Game::equipItem(const ItemPtr &item) {
    if (!item) return;
    typedef void(gameCall* EquipItem)(
        uintptr_t RCX,
        const ItemPtr *item
        );
    auto function = reinterpret_cast<EquipItem>(SingletonFunctions["g_game.equipItem"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, item]() {
        function(SingletonFunctions["g_game.equipItem"].second, &item);
    });
}

void Game::equipItemId(uint16_t itemId, uint8_t tier) {
        typedef void(gameCall* EquipItemId)(
        uintptr_t RCX,
        uint16_t itemId,
        uint8_t tier
        );
    auto function = reinterpret_cast<EquipItemId>(SingletonFunctions["g_game.equipItemId"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, itemId, tier]() {
        function(SingletonFunctions["g_game.equipItemId"].second, itemId, tier);
    });
}

void Game::mount(bool mountStatus) {
    typedef void(gameCall* Mount)(
        uintptr_t RCX,
        bool mountStatus
        );
    auto function = reinterpret_cast<Mount>(SingletonFunctions["g_game.mount"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, mountStatus]() {
        function(SingletonFunctions["g_game.mount"].second, mountStatus);
    });
}

void Game::changeMapAwareRange(const uint8_t xrange, const uint8_t yrange) {
    typedef void(gameCall* ChangeMapAwareRange)(
        uintptr_t RCX,
        uint8_t xrange,
        uint8_t yrange
    );
    auto function = reinterpret_cast<ChangeMapAwareRange>(SingletonFunctions["g_game.changeMapAwareRange"].first);
    if (!function) return;
    return g_dispatcher->scheduleEventEx([function, xrange, yrange]() {
        function(SingletonFunctions["g_game.changeMapAwareRange"].second, xrange, yrange);
    });
}

bool Game::canPerformGameAction() {
    typedef bool(gameCall* CanPerformGameAction)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<CanPerformGameAction>(SingletonFunctions["g_game.canPerformGameAction"].first);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function]() {
            void* pMysteryPtr = nullptr;
            auto ret = function(SingletonFunctions["g_game.canPerformGameAction"].second, &pMysteryPtr);
            return ret;
    });
}

bool Game::isOnline() {
    typedef bool(gameCall* IsOnline)(
       uintptr_t RCX,
       void *RDX
       );
    auto function = reinterpret_cast<IsOnline>(SingletonFunctions["g_game.isOnline"].first);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function]() {
            void* pMysteryPtr = nullptr;
            auto ret = function(SingletonFunctions["g_game.isOnline"].second, &pMysteryPtr);
            return ret;
    });
}

bool Game::isAttacking() {
    typedef bool(gameCall* IsAttacking)(
       uintptr_t RCX,
       void *RDX
       );
    auto function = reinterpret_cast<IsAttacking>(SingletonFunctions["g_game.isAttacking"].first);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function]() {
            void* pMysteryPtr = nullptr;
            auto ret = function(SingletonFunctions["g_game.isAttacking"].second, &pMysteryPtr);
            return ret;
    });
}

bool Game::isFollowing() {
    typedef bool(gameCall* IsFollowing)(
       uintptr_t RCX,
       void *RDX
       );
    auto function = reinterpret_cast<IsFollowing>(SingletonFunctions["g_game.isFollowing"].first);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function]() {
            void* pMysteryPtr = nullptr;
            auto ret = function(SingletonFunctions["g_game.isFollowing"].second, &pMysteryPtr);
            return ret;
    });
}


ContainerPtr Game::getContainer(int index) {
    typedef void(gameCall* GetContainer)(
        uintptr_t RCX,
        ContainerPtr *RDX,
        int idx
        );
    auto function = reinterpret_cast<GetContainer>(SingletonFunctions["g_game.getContainer"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, index]() {
            ContainerPtr result;
            function(SingletonFunctions["g_game.getContainer"].second, &result, index);
            return result;
    });
}

std::vector<ContainerPtr> Game::getContainers() {
    typedef void(gameCall* GetContainers)(
        uintptr_t RCX,
        std::map<int, uintptr_t> *RDX
    );
    auto function = reinterpret_cast<GetContainers>(SingletonFunctions["g_game.getContainers"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function]() {
        std::map<int, uintptr_t> result;
        function(SingletonFunctions["g_game.getContainers"].second, &result);
        std::vector<ContainerPtr> containers;
        for (auto items : result){
            containers.push_back(ContainerPtr(items.second));
        }
        return containers;
    });
}

CreaturePtr Game::getAttackingCreature() {
    typedef void(gameCall* GetAttackingCreature)(
        uintptr_t RCX,
        CreaturePtr *RDX
        );
    auto function = reinterpret_cast<GetAttackingCreature>(SingletonFunctions["g_game.getAttackingCreature"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function]() {
            CreaturePtr result;
            function(SingletonFunctions["g_game.getAttackingCreature"].second, &result);
            return result;
    });
}

CreaturePtr Game::getFollowingCreature() {
    typedef void(gameCall* GetFollowingCreature)(
        uintptr_t RCX,
        CreaturePtr *RDX
        );
    auto function = reinterpret_cast<GetFollowingCreature>(SingletonFunctions["g_game.getFollowingCreature"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function]() {
            CreaturePtr result;
            function(SingletonFunctions["g_game.getFollowingCreature"].second, &result);
            return result;
    });
}

LocalPlayerPtr Game::getLocalPlayer() {
    typedef void(gameCall* GetLocalPlayer)(
        uintptr_t RCX,
        LocalPlayerPtr *RDX
        );
    auto function = reinterpret_cast<GetLocalPlayer>(SingletonFunctions["g_game.getLocalPlayer"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function]() {
        LocalPlayerPtr result;
        function(SingletonFunctions["g_game.getLocalPlayer"].second, &result);
        return result;
    });
}

int Game::getClientVersion() {
    typedef int(gameCall* GetClientVersion)(
        uintptr_t RCX
        );
    auto function = reinterpret_cast<GetClientVersion>(SingletonFunctions["g_game.getClientVersion"].first);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function]() {
        return function(SingletonFunctions["g_game.getClientVersion"].second);
    });
}

std::string Game::getCharacterName() {
    typedef void(gameCall* GetCharacterName)(
        uintptr_t RCX,
        std::string *RDX
        );
    auto function = reinterpret_cast<GetCharacterName>(SingletonFunctions["g_game.getCharacterName"].first);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function]() {
        std::string result;
        function(SingletonFunctions["g_game.getCharacterName"].second, &result);
        return result;
    });
}
