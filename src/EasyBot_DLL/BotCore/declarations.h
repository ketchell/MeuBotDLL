#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include <list>
#include <memory>
#include <vector>
#include "BuildConfig.h"
// core
class Map;
class Game;
class MapView;
class LightView;
class Tile;
class Thing;
class Item;
class Container;
class Creature;
class Monster;
class Npc;
class Player;
class LocalPlayer;
class Effect;
class Missile;
class AnimatedText;
class StaticText;
class Animator;
class ThingType;
class ItemType;
class TileBlock;
class AttachedEffect;
class AttachableObject;


template<typename T>
struct SmartPtr {
    uintptr_t address;
    uintptr_t controlBlock;

    SmartPtr() : address(0), controlBlock(0) {}
    SmartPtr(uintptr_t addr) : address(addr), controlBlock(0) {}

    template<typename U>
    SmartPtr(const SmartPtr<U>& other) : address(other.address), controlBlock(other.controlBlock) {}

    operator uintptr_t() const { return address; }
    bool operator!() const { return address == 0; }
    bool operator==(const SmartPtr& other) const { return address == other.address; }
    bool operator!=(const SmartPtr& other) const { return address != other.address; }
};

using CreaturePtr = uintptr_t;
using ItemPtr = uintptr_t;
using ContainerPtr = uintptr_t;
using LocalPlayerPtr = uintptr_t;
using ThingPtr = uintptr_t;
using TilePtr = uintptr_t;




#endif //DECLARATIONS_H
