// ============================================================================
// INTEGRATION_GUIDE.cpp
// ============================================================================
//
// How to integrate OffsetResolver into your EasyBot DLL fork
//
// ============================================================================

// ============================================================================
// 1. WHERE TO CALL InitializeOffsets()
// ============================================================================
//
// In your DLL's DllMain or initialization thread, call it AFTER the client
// has fully loaded (i.e., after the game window is visible):
//

#include "OffsetResolver.h"

// Global offsets – accessible from all bot threads
GameOffsets g_offsets;

// In your DLL entry point or init function:
void BotInit() {
    // Wait for the client to be fully initialized.
    // The Lua binding tables only exist after the engine sets up the Lua state.
    // A simple approach: wait until the game window exists.
    while (!FindWindowA(nullptr, "OTClient") &&
           !FindWindowA(nullptr, "Tibia")) {
        Sleep(500);
    }
    Sleep(2000);  // Extra delay for Lua init to complete

    if (InitializeOffsets(g_offsets)) {
        OutputDebugStringA("[EasyBot] Offsets resolved successfully!");

        // Quick validation: try reading the player name
        std::string name = GetPlayerName(g_offsets);
        char buf[256];
        snprintf(buf, sizeof(buf), "[EasyBot] Player name: %s", name.c_str());
        OutputDebugStringA(buf);
    }
    else {
        OutputDebugStringA("[EasyBot] Offset resolution FAILED – check easybot_offsets.log");
    }
}


// ============================================================================
// 2. REPLACING HARDCODED OFFSETS IN THE EXISTING EASYBOT CODE
// ============================================================================
//
// The original EasyBot has code like this (pseudocode):
//
//   #ifdef BUILD_REALERA
//     #define GAME_PTR_ADDR      0x01ABCDEF
//     #define LOCAL_PLAYER_OFF   0x44
//     #define HEALTH_OFF         0x1C0
//   #endif
//
//   uintptr_t game = *(uintptr_t*)GAME_PTR_ADDR;
//   uintptr_t player = *(uintptr_t*)(game + LOCAL_PLAYER_OFF);
//   int health = *(int*)(player + HEALTH_OFF);
//
// Replace ALL of that with:
//
//   uintptr_t player = GetLocalPlayer(g_offsets);
//   uint32_t health = GetPlayerHealth(g_offsets);
//
// Or if you need to do it manually for fields we don't have a helper for:
//
//   uintptr_t game = GetGameSingleton(g_offsets);
//   uint32_t chaseMode = *(uint32_t*)(game + g_offsets.game_chaseMode);
//


// ============================================================================
// 3. EXAMPLE: GRPC SERVICE DATA PROVIDER
// ============================================================================
//
// Your gRPC service that feeds data to the GUI should use the resolved offsets:
//

/*
grpc::Status BotServiceImpl::GetPlayerInfo(
    grpc::ServerContext* context,
    const Empty* request,
    PlayerInfo* response)
{
    if (!g_offsets.resolved) {
        return grpc::Status(grpc::CANCELLED, "Offsets not resolved");
    }

    uintptr_t lp = GetLocalPlayer(g_offsets);
    if (!lp) {
        return grpc::Status(grpc::CANCELLED, "LocalPlayer is null (not logged in?)");
    }

    response->set_name(GetPlayerName(g_offsets));
    response->set_health(GetPlayerHealth(g_offsets));
    response->set_max_health(GetPlayerMaxHealth(g_offsets));
    response->set_mana(GetPlayerMana(g_offsets));
    response->set_level(GetPlayerLevel(g_offsets));

    Position pos = GetPlayerPosition(g_offsets);
    response->mutable_position()->set_x(pos.x);
    response->mutable_position()->set_y(pos.y);
    response->mutable_position()->set_z(pos.z);

    return grpc::Status::OK;
}
*/


// ============================================================================
// 4. THE LUA WRAPPER PROBLEM
// ============================================================================
//
// IMPORTANT: The function pointers in the Lua binding table are NOT always
// the raw C++ getters. They are often Lua wrapper functions that:
//
//   1. Read the Lua stack to get the 'self' userdata
//   2. Extract the C++ object pointer from it
//   3. Call the actual getter
//   4. Push the result onto the Lua stack
//
// These wrappers look something like:
//
//   push ebp
//   mov ebp, esp
//   sub esp, XX
//   mov eax, [ebp+8]          ; lua_State*
//   push 1
//   push eax
//   call luaL_checkudata      ; get the C++ object
//   mov ecx, [eax]            ; dereference shared_ptr or raw ptr
//   mov eax, [ecx+0x1C0]     ; THE ACTUAL MEMBER OFFSET (e.g., health)
//   ... push onto Lua stack ...
//   ret
//
// Our AnalyzeGetter handles the simple patterns, but for Lua wrappers
// you might need to extend it. Here's how to identify them:
//
// If AnalyzeGetter returns valid=false for a Creature/Player binding,
// the function is probably a Lua wrapper. You have two options:
//
//   Option A: Extend AnalyzeGetter to parse deeper into the function.
//             Look for the pattern: call XXX; mov ecx,[eax]; mov eax,[ecx+OFF]
//
//   Option B: Use the OTClient source code to know the struct layout.
//             Since OTClient is open source, the struct offsets are known
//             from the source. The variable part is just the compiler's
//             layout decisions (padding, vtable, etc.), which you can
//             determine from ONE working offset and extrapolate.
//


// ============================================================================
// 5. FALLBACK: MANUAL OFFSET ENTRY + AUTO-DETECTION HYBRID
// ============================================================================
//
// For maximum reliability, use a hybrid approach:
//
//   1. Auto-detect what you can (g_game ptr, localPlayer offset, etc.)
//   2. For anything auto-detect misses, fall back to a config file
//   3. The config file can be generated ONCE per server binary using
//      a helper tool (or by hand with x32dbg)
//

/*
// offsets.ini format:
// [auto]                    ; auto-detected values (written by the DLL)
// g_game_ptr = 0x01ABCDEF
// game_localPlayer = 0x44
//
// [manual]                  ; user overrides (take priority)
// creature_health = 0x1C0
// creature_position = 0x1D8

void LoadOffsetOverrides(GameOffsets& o, const char* iniPath) {
    char buf[64];

    // Only override if the manual section has a value
    GetPrivateProfileStringA("manual", "creature_health", "", buf, sizeof(buf), iniPath);
    if (buf[0]) o.creature_health = (uint32_t)strtoul(buf, nullptr, 16);

    GetPrivateProfileStringA("manual", "creature_position", "", buf, sizeof(buf), iniPath);
    if (buf[0]) o.creature_position = (uint32_t)strtoul(buf, nullptr, 16);

    // ... etc for each field
}

void SaveAutoDetectedOffsets(const GameOffsets& o, const char* iniPath) {
    char buf[64];

    snprintf(buf, sizeof(buf), "0x%08X", (uint32_t)o.g_game_ptr_addr);
    WritePrivateProfileStringA("auto", "g_game_ptr", buf, iniPath);

    snprintf(buf, sizeof(buf), "0x%X", o.game_localPlayer);
    WritePrivateProfileStringA("auto", "game_localPlayer", buf, iniPath);

    // ... etc
}
*/


// ============================================================================
// 6. DEBUGGING TIPS
// ============================================================================
//
// After running the resolver, check easybot_offsets.log in the client folder.
// It shows every step: which strings were found, which tables were walked,
// which getters were disassembled, and the first 16 bytes of any getter
// that couldn't be parsed.
//
// If a getter can't be parsed:
//   1. Open x32dbg, attach to the client
//   2. Go to the address shown in the log
//   3. Look at the disassembly – you'll see what pattern it uses
//   4. Add that pattern to AnalyzeGetter()
//
// Common reasons for failure:
//   - The getter is a Lua wrapper (see section 4 above)
//   - The function has been inlined by the compiler
//   - The function uses an unusual register (esi, edi instead of eax/ecx)
//   - The function starts with a jump to another function (jmp thunk)
//
// For jump thunks, add this to the top of AnalyzeGetter:
//   if (code[0] == 0xE9) {  // jmp rel32
//       int32_t rel = *(int32_t*)(code + 1);
//       funcAddr = funcAddr + 5 + rel;
//       code = (uint8_t*)funcAddr;
//       // ... continue with the patterns
//   }
//


// ============================================================================
// 7. CREATURE LIST READING
// ============================================================================
//
// For the creature list (needed for targeting), the approach is different.
// OTClient stores creatures in a std::unordered_map inside the Map class.
// You need:
//
//   1. g_map singleton (find "g_map" binding table the same way)
//   2. Map::m_knownCreatures offset (find "getCreatures" or similar)
//   3. Walk the std::unordered_map in memory
//
// The unordered_map layout in MSVC x86 is:
//   +0x00: _List (a std::list node chain)
//   _List._Myhead: pointer to sentinel node
//   _List._Mysize: number of elements
//   Each node: { _Next, _Prev, _Myval }
//   _Myval for map<uint32_t, shared_ptr<Creature>>:
//     { uint32_t creatureId, shared_ptr<Creature> }
//
// This is complex to walk generically. A simpler alternative:
// Hook g_game.getCreatures() at the Lua level by calling it through the
// Lua state, or hook the ProtocolGame parse functions to intercept creature
// data as it arrives from the server.
//

#if 0  // Example creature list walker (needs Map offsets resolved)
struct CreatureListEntry {
    uint32_t  id;
    uintptr_t creaturePtr;  // raw Creature*
};

std::vector<CreatureListEntry> GetCreatureList(const GameOffsets& o) {
    std::vector<CreatureListEntry> result;

    // You'd need o.g_map_ptr_addr and o.map_knownCreatures resolved.
    // This is left as a TODO – use the same FindString/WalkTable/AnalyzeGetter
    // approach on the "g_map" or "Map" binding table.

    return result;
}
#endif
