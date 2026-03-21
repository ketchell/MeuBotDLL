#pragma once
// ============================================================================
// OffsetResolver.h  –  Runtime offset extraction for OTClient-based clients
// ============================================================================
//
// HOW IT WORKS
// ------------
// OTClient registers every Lua-callable function in a table embedded in .rdata.
// Each entry in that table looks like this (x86, 32-bit):
//
//   struct LuaBindEntry {        // stride = 0xDC (220 bytes)
//       ...
//       DWORD funcPtr;           // at +0x38 from the entry start
//       ...
//       char* namePtr;           // at +0x70 from the entry start
//       ...                      // padding to 0xDC total
//   };
//
// The name strings are things like "getLocalPlayer", "getHealth", etc.
// The funcPtrs point to tiny C++ getter functions like:
//
//   Game::getLocalPlayer:
//       mov eax, ds:[0x01ABCDEF]   // load g_game singleton ptr
//       mov eax, [eax+0x44]        // load m_localPlayer from Game
//       ret
//
// We pattern-scan to find these tables, extract the function addresses,
// then read the first few instructions of each getter to pull out:
//   - Global pointer addresses  (the g_game / g_map singletons)
//   - Struct member offsets     (m_localPlayer, m_health, m_mana, ...)
//
// This makes the bot resilient to server updates because the Lua binding
// strings never change – only the addresses shift.
//
// ============================================================================

#include <windows.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

// ---------------------------------------------------------------------------
// Resolved offsets – the final product that the rest of the DLL consumes
// ---------------------------------------------------------------------------

struct GameOffsets {
    // ---- Global singleton pointers (absolute virtual addresses) ----
    uintptr_t g_game_ptr_addr       = 0;  // addr of the global that holds Game*
    uintptr_t g_map_ptr_addr        = 0;  // addr of the global that holds Map*

    // ---- Game class member offsets ----
    uint32_t  game_localPlayer      = 0;  // Game + X  → LocalPlayer*
    uint32_t  game_protocolVersion  = 0;  // Game + X  → uint16 protocolVersion
    uint32_t  game_clientVersion    = 0;
    uint32_t  game_chaseMode        = 0;
    uint32_t  game_pvpMode          = 0;
    uint32_t  game_attackingCreature = 0;
    uint32_t  game_followingCreature = 0;

    // ---- Creature / Player / LocalPlayer member offsets ----
    //      (extracted from the LocalPlayer Lua binding table)
    uint32_t  creature_name         = 0;  // Creature + X → std::string name
    uint32_t  creature_health       = 0;
    uint32_t  creature_maxHealth    = 0;
    uint32_t  creature_position     = 0;  // Position { uint16 x, y, z }
    uint32_t  creature_direction    = 0;
    uint32_t  creature_speed        = 0;
    uint32_t  creature_outfit       = 0;
    uint32_t  creature_skull        = 0;
    uint32_t  creature_shield       = 0;
    uint32_t  creature_id           = 0;

    uint32_t  player_mana           = 0;
    uint32_t  player_maxMana        = 0;
    uint32_t  player_level          = 0;
    uint32_t  player_magicLevel     = 0;
    uint32_t  player_soul           = 0;
    uint32_t  player_stamina        = 0;
    uint32_t  player_vocation       = 0;
    uint32_t  player_experience     = 0;

    bool      resolved              = false;

    // ---- Double-typed field flags ----
    // True when the corresponding getter uses 'fld qword' (DD 81 ...).
    // Read helpers must load 8 bytes and reinterpret as double before casting.
    bool      creature_health_isDouble    = false;
    bool      creature_maxHealth_isDouble = false;
    bool      player_mana_isDouble        = false;
    bool      player_maxMana_isDouble     = false;
    bool      player_level_isDouble       = false;
    bool      player_magicLevel_isDouble  = false;
    bool      player_soul_isDouble        = false;
    bool      player_stamina_isDouble     = false;
    bool      player_experience_isDouble  = false;
};

// ---------------------------------------------------------------------------
// Intermediate: a single extracted Lua binding entry
// ---------------------------------------------------------------------------

struct LuaBindingEntry {
    std::string name;          // e.g. "getLocalPlayer"
    uintptr_t   funcAddr;      // absolute VA of the C++ function
    uintptr_t   tableOffset;   // img-relative offset of this entry in the table
};

// ---------------------------------------------------------------------------
// Intermediate: result of disassembling a tiny getter function
// ---------------------------------------------------------------------------

struct GetterAnalysis {
    // For simple getters like:  mov eax,[global]; mov eax,[eax+off]; ret
    uintptr_t globalAddr  = 0;    // the [global] address (g_game ptr location)
    uint32_t  memberOff   = 0;    // the struct member offset
    bool      valid       = false;

    // For slightly more complex getters that go through 'this' pointer:
    //   mov ecx,[global]; mov eax,[ecx+off1]; mov eax,[eax+off2]; ret
    uint32_t  memberOff2  = 0;    // second-level dereference offset (0 if none)
    bool      twoLevel    = false;

    // True when the getter uses 'fld qword ptr [ecx+off]' (DD 81 ...) —
    // the field is stored as a C++ double (8 bytes) that must be cast to int.
    bool      isDouble    = false;
};

// ---------------------------------------------------------------------------
// The resolver class
// ---------------------------------------------------------------------------

class OffsetResolver {
public:
    // Run the full resolution pipeline. Returns true if critical offsets found.
    bool Resolve(GameOffsets& out);

    // Get the raw Lua binding entries found (for debugging / logging)
    const std::vector<LuaBindingEntry>& GetGameBindings()   const { return m_gameBindings; }
    const std::vector<LuaBindingEntry>& GetPlayerBindings()  const { return m_playerBindings; }

    // Get the last error/status message
    const std::string& GetLastError() const { return m_lastError; }

    // Get detailed log of everything that happened
    const std::string& GetLog() const { return m_log; }

private:
    // ---- Internal steps ----

    // Step 1: Find the module base and size
    bool FindModuleInfo();

    // Step 2: Scan .rdata for a string literal and return its VA
    uintptr_t FindString(const char* str, size_t len);

    // Step 3: Find all DWORDs in the image that point to a given VA
    std::vector<uintptr_t> FindXrefs(uintptr_t targetVA);

    // Step 4: Given an xref to a name string inside a Lua binding table,
    //         walk the table backwards/forwards to extract all entries.
    std::vector<LuaBindingEntry> WalkBindingTable(
        uintptr_t nameXrefAddr,
        uint32_t  stride,
        uint32_t  funcPtrOffset,   // offset of funcPtr within stride
        uint32_t  namePtrOffset    // offset of namePtr within stride
    );

    // Step 5: Disassemble a tiny getter to extract global addr + member offset.
    GetterAnalysis AnalyzeGetter(uintptr_t funcAddr);

    // Step 6: Look up a binding by name and analyze it
    std::optional<GetterAnalysis> FindAndAnalyze(
        const std::vector<LuaBindingEntry>& bindings,
        const std::string& name
    );

    // Helpers
    bool IsValidCodeAddr(uintptr_t addr);
    bool IsValidDataAddr(uintptr_t addr);
    bool IsValidStringPtr(uintptr_t addr);
    void Log(const char* fmt, ...);

    // Find the address of the global variable that holds a named singleton pointer.
    // Searches for the name string in .rdata, then finds xrefs to it inside .text,
    // then scans ±0x40 bytes around each xref for a global memory-load instruction
    // (A1, 8B 0D, 8B 15, or FF 35 with an absolute imm32).
    uintptr_t FindGlobalSingleton(const char* name);

    // Find a Creature/Player getter function by method name without a binding table.
    // Strategy A: probe DWORDs at common slot offsets near each string xref.
    // Strategy B: scan .text for push-strAddr / push-funcAddr registration pairs.
    GetterAnalysis ResolveMethodDirect(const char* methodName);

    // ---- State ----
    uintptr_t m_base     = 0;
    size_t    m_size     = 0;
    uintptr_t m_codeBase = 0;   // .text section start
    size_t    m_codeSize = 0;
    uintptr_t m_rdataBase = 0;  // .rdata section start
    size_t    m_rdataSize = 0;

    std::vector<LuaBindingEntry> m_gameBindings;
    std::vector<LuaBindingEntry> m_playerBindings;

    // Tracks function addresses already claimed by an earlier ResolveMethodDirect
    // call, so later methods don't steal the same function (e.g. getSkull vs getShield).
    std::unordered_set<uint32_t> m_claimedFuncAddrs;

    std::string m_lastError;
    std::string m_log;

    // Binding table constants (from your scan: stride 0xDC, func at +0x38, name at +0x70)
    // These might need slight tweaks per OTClient fork, but 0xDC/0x38 is very common.
    static constexpr uint32_t TABLE_STRIDE       = 0xDC;
    static constexpr uint32_t FUNC_PTR_OFFSET    = 0x38;
    static constexpr uint32_t NAME_PTR_OFFSET    = 0x70;  // we'll auto-detect this too
};

// ---------------------------------------------------------------------------
// Convenience: global instance + init function
// ---------------------------------------------------------------------------

// Call once after DLL injection, before starting any bot threads.
bool InitializeOffsets(GameOffsets& offsets);

// Read helpers that use the resolved offsets
// (These are examples – expand as needed)

uintptr_t  GetGameSingleton(const GameOffsets& o);
uintptr_t  GetLocalPlayer(const GameOffsets& o);
uint32_t   GetPlayerHealth(const GameOffsets& o);
uint32_t   GetPlayerMaxHealth(const GameOffsets& o);
uint32_t   GetPlayerMana(const GameOffsets& o);
uint32_t   GetPlayerLevel(const GameOffsets& o);
std::string GetPlayerName(const GameOffsets& o);

struct Position { uint16_t x, y; uint8_t z; };
Position   GetPlayerPosition(const GameOffsets& o);
