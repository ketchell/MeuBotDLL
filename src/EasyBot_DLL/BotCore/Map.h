#ifndef MAPVIEW_H
#define MAPVIEW_H
#define g_map Map::getInstance()
#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"



class Map {
    static Map* instance;
    static std::mutex mutex;
protected:
    Map()=default;
    ~Map()= default;
public:
    Map(Map const&) = delete;
    void operator=(const Map&) = delete;
    static Map* getInstance();


    TilePtr getTile(Position tilePos);

    std::vector<CreaturePtr> getSpectators(const Position centerPos, bool multiFloor = false);
    std::vector<Otc::Direction> findPath(const Position startPos, const Position goalPos, int maxComplexity, int flags);
    bool isSightClear(const Position& fromPos, const Position& toPos);
};



#endif //MAPVIEW_H
