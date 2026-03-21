#include "Tile.h"

Tile* Tile::instance{nullptr};
std::mutex Tile::mutex;


Tile* Tile::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new Tile();
    }
    return instance;
}

ThingPtr Tile::getTopThing(TilePtr tile) {
    if (!tile) return 0;
    typedef void(gameCall* GetTopThing)(
        uintptr_t RCX,
        ThingPtr *RDX
        );
    auto function = reinterpret_cast<GetTopThing>(ClassMemberFunctions["Tile.getTopThing"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, tile]() {
        ThingPtr result;
        function(tile, &result);
        return result;
    });
}

ThingPtr Tile::getTopUseThing(TilePtr tile) {
    if (!tile) return 0;
    typedef void(gameCall* GetTopUseThing)(
        uintptr_t RCX,
        ThingPtr *RDX
        );
    auto function = reinterpret_cast<GetTopUseThing>(ClassMemberFunctions["Tile.getTopUseThing"]);
    if (!function) return 0;
    return g_dispatcher->scheduleEventEx([function, tile]() {
        ThingPtr result;
        function(tile, &result);
        return result;
    });
}

std::vector<ItemPtr> Tile::getItems(TilePtr tile) {
    if (!tile) return {};
    typedef void(gameCall* GetItems)(
        uintptr_t RCX,
        std::vector<ItemPtr> *RDX
        );
    auto function = reinterpret_cast<GetItems>(ClassMemberFunctions["Tile.getItems"]);
    if (!function) return {};
    return g_dispatcher->scheduleEventEx([function, tile]() {
        std::vector<ItemPtr> result;
        function(tile, &result);
        return result;
    });
}

bool Tile::isWalkable(TilePtr tile, bool ignoreMonsters) {
    if (!tile) return false;
    typedef void(gameCall* IsWalkable)(
        uintptr_t RCX,
        bool *RDX,
        bool ignoreCreatures
        );
    auto function = reinterpret_cast<IsWalkable>(ClassMemberFunctions["Tile.isWalkable"]);
    if (!function) return false;
    return g_dispatcher->scheduleEventEx([function, tile, ignoreMonsters]() {
        bool result;
        function(tile, &result, ignoreMonsters);
        return result;
    });
}
