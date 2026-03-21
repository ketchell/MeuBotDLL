#ifndef TILE_H
#define TILE_H
#define g_tile Tile::getInstance()
#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"


class Tile {
    static Tile* instance;
    static std::mutex mutex;
protected:
    Tile()=default;
    ~Tile()=default;
public:
    Tile(Tile const&) = delete;
    void operator=(const Tile&) = delete;
    static Tile* getInstance();

    ThingPtr getTopThing(TilePtr tile);
    ThingPtr getTopUseThing(TilePtr tile);
    std::vector<ItemPtr> getItems(TilePtr tile);
    bool isWalkable(TilePtr tile, bool ignoreMonsters);
};



#endif //TILE_H
