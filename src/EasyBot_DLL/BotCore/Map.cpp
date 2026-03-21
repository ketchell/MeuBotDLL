#include "Map.h"

Map* Map::instance{nullptr};
std::mutex Map::mutex;


Map* Map::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new Map();
    }
    return instance;
}

TilePtr Map::getTile(Position tilePos) {
    typedef TilePtr*(gameCall* GetTile)(
        uintptr_t a1,
        const Position *a2
        );
    auto function = reinterpret_cast<GetTile>(SingletonFunctions["g_map.getTile"].first);
    if (!function) return {};
    uintptr_t thisPtr = SingletonFunctions["g_map.getTile"].second;
    if (!thisPtr) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr, tilePos]() -> TilePtr {
        TilePtr* ret = function(thisPtr, &tilePos);
        if (!ret) return TilePtr{};
        return *ret;
    });
}

std::vector<CreaturePtr> Map::getSpectators(const Position centerPos, bool multiFloor) {
    typedef void(gameCall* GetSpectators)(
        uintptr_t RCX,
        std::vector<CreaturePtr> *RDX,
        const Position *R8,
        bool R9
        );
    auto function = reinterpret_cast<GetSpectators>(SingletonFunctions["g_map.getSpectators"].first);
    if (!function) return {};
    uintptr_t thisPtr = SingletonFunctions["g_map.getSpectators"].second;
    if (!thisPtr) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr, centerPos, multiFloor]() {
        std::vector<CreaturePtr> result;
        function(thisPtr, &result, &centerPos, multiFloor);
        return result;
    });
}

std::vector<Otc::Direction> Map::findPath(const Position startPos, const Position goalPos, int maxComplexity, int flags) {
    typedef uintptr_t*(gameCall* FindPath)(
        uintptr_t RCX,
        std::tuple<std::vector<Otc::Direction>, Otc::PathFindResult> *result,
        const Position *R8,
        const Position *R9,
        int maxComplexity,
        int flags
        );
    auto function = reinterpret_cast<FindPath>(SingletonFunctions["g_map.findPath"].first);
    if (!function) return {};
    uintptr_t thisPtr = SingletonFunctions["g_map.findPath"].second;
    if (!thisPtr) return {};
    return g_dispatcher->scheduleEventEx([function, thisPtr, startPos, goalPos, maxComplexity, flags]() {
        std::tuple<std::vector<Otc::Direction>, Otc::PathFindResult> pMysteryPtr;
        function(thisPtr, &pMysteryPtr, &startPos, &goalPos, maxComplexity, flags);
        if (std::get<1>(pMysteryPtr) == Otc::PathFindResultOk)
            return std::get<0>(pMysteryPtr);
        return std::vector<Otc::Direction>{};
    });
}

bool Map::isSightClear(const Position& fromPos, const Position& toPos) {
    typedef bool(gameCall* IsSightClear)(
        uintptr_t RCX,
        void *RDX,
        const Position *fromPos,
        const Position *toPos
        );
    auto function = reinterpret_cast<IsSightClear>(SingletonFunctions["g_map.isSightClear"].first);
    if (!function) return false;
    uintptr_t thisPtr = SingletonFunctions["g_map.isSightClear"].second;
    if (!thisPtr) return false;
    return g_dispatcher->scheduleEventEx([function, thisPtr, fromPos, toPos]() {
        void* pMysteryPtr = nullptr;
        return function(thisPtr, &pMysteryPtr, &fromPos, &toPos);
    });
}
