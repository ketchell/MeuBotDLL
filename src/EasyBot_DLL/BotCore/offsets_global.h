#pragma once
#include <windows.h>
#include <cstdint>

// BotOffsets mirrors the fields of GameOffsets (OffsetResolver.h) that BotCore files
// need, but without including OffsetResolver.h, which defines its own ::Position that
// conflicts with const.h's ::Position. Safe to include alongside all game headers.
struct BotOffsets {
    uintptr_t g_game_ptr_addr        = 0;  // address of the global Game* pointer
    uint32_t  game_localPlayer       = 0;  // Game + X  -> LocalPlayer*
    uint32_t  game_clientVersion     = 0;
    uint32_t  game_chaseMode         = 0;
    uint32_t  game_attackingCreature = 0;
    uint32_t  game_followingCreature = 0;

    uint32_t  creature_health        = 0;
    uint32_t  creature_maxHealth     = 0;
    uint32_t  creature_name          = 0;
    uint32_t  creature_id            = 0;

    uint32_t  player_mana            = 0;
    uint32_t  player_maxMana         = 0;
    uint32_t  player_level           = 0;
    uint32_t  player_soul            = 0;
    uint32_t  player_stamina         = 0;

    uint32_t  thing_positionOffset   = 0;  // offset of Position (x,y,z) within a Thing/Creature struct

    bool      resolved               = false;

    // True when the field is stored as a C++ double (8 bytes) in game memory.
    // Read helpers must use memcpy+double cast instead of reading 4/2/1 bytes.
    bool      creature_health_isDouble    = false;
    bool      creature_maxHealth_isDouble = false;
    bool      player_mana_isDouble        = false;
    bool      player_maxMana_isDouble     = false;
    bool      player_level_isDouble       = false;
    bool      player_soul_isDouble        = false;
    bool      player_stamina_isDouble     = false;
};

inline BotOffsets g_botOffsets;

// ---- Bridge functions (implemented in OffsetResolverBridge.cpp) ----

// Run the full OffsetResolver pipeline and populate g_botOffsets.
// Call after the client window is visible.
bool RunOffsetResolver();

// Read the live Game* singleton from game memory.
uintptr_t ReadGameSingleton();

// Read the live LocalPlayer* from game memory.
uintptr_t ReadLocalPlayerPtr();

// Retrieve the resolved g_game Lua binding entries so the caller can
// populate SingletonFunctions with both the function pointer and the
// correct this-pointer (game singleton).
struct GameBinding { char name[64]; uintptr_t funcAddr; };
int GetGameBindings(GameBinding* out, int maxCount);
