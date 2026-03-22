// ============================================================================
// OffsetResolverBridge.cpp
//
// IMPORTANT: Do NOT add game headers here (Game.h, LocalPlayer.h, const.h …).
// This TU is intentionally isolated so that OffsetResolver.h (which defines
// its own ::Position with uint16_t fields) never sees const.h's ::Position
// (int32_t fields). Mixing both in one TU causes a redefinition error.
// ============================================================================

#include "OffsetResolver.h"   // must come before any game headers
#include "offsets_global.h"   // BotOffsets + bridge declarations (no const.h)

#include <cstring>
#include <algorithm>

static GameOffsets  s_gameOffsets;
static OffsetResolver s_resolver;

bool RunOffsetResolver() {
    bool ok = s_resolver.Resolve(s_gameOffsets);

    // Write the log next to the exe for debugging.
    const std::string& log = s_resolver.GetLog();
    FILE* f = fopen("easybot_offsets.log", "w");
    if (f) { fwrite(log.c_str(), 1, log.size(), f); fclose(f); }

    if (!ok) return false;

    // Copy resolved fields into the game-header-safe BotOffsets struct.
    g_botOffsets.g_game_ptr_addr        = s_gameOffsets.g_game_ptr_addr;
    g_botOffsets.game_localPlayer       = s_gameOffsets.game_localPlayer;
    g_botOffsets.game_clientVersion     = s_gameOffsets.game_clientVersion;
    g_botOffsets.game_chaseMode         = s_gameOffsets.game_chaseMode;
    g_botOffsets.game_attackingCreature = s_gameOffsets.game_attackingCreature;
    g_botOffsets.game_followingCreature = s_gameOffsets.game_followingCreature;
    g_botOffsets.creature_health        = s_gameOffsets.creature_health;
    g_botOffsets.creature_maxHealth     = s_gameOffsets.creature_maxHealth;
    g_botOffsets.creature_name          = s_gameOffsets.creature_name;
    g_botOffsets.creature_id            = s_gameOffsets.creature_id;
    g_botOffsets.player_mana            = s_gameOffsets.player_mana;
    g_botOffsets.player_maxMana         = s_gameOffsets.player_maxMana;
    g_botOffsets.player_level           = s_gameOffsets.player_level;
    g_botOffsets.player_soul            = s_gameOffsets.player_soul;
    g_botOffsets.player_stamina         = s_gameOffsets.player_stamina;
    g_botOffsets.thing_positionOffset   = s_gameOffsets.creature_position;
    g_botOffsets.resolved               = s_gameOffsets.resolved;

    g_botOffsets.creature_health_isDouble    = s_gameOffsets.creature_health_isDouble;
    g_botOffsets.creature_maxHealth_isDouble = s_gameOffsets.creature_maxHealth_isDouble;
    g_botOffsets.player_mana_isDouble        = s_gameOffsets.player_mana_isDouble;
    g_botOffsets.player_maxMana_isDouble     = s_gameOffsets.player_maxMana_isDouble;
    g_botOffsets.player_level_isDouble       = s_gameOffsets.player_level_isDouble;
    g_botOffsets.player_soul_isDouble        = s_gameOffsets.player_soul_isDouble;
    g_botOffsets.player_stamina_isDouble     = s_gameOffsets.player_stamina_isDouble;

    return true;
}

uintptr_t ReadGameSingleton() {
    return GetGameSingleton(s_gameOffsets);
}

uintptr_t ReadLocalPlayerPtr() {
    return GetLocalPlayer(s_gameOffsets);
}

int GetGameBindings(GameBinding* out, int maxCount) {
    const auto& bindings = s_resolver.GetGameBindings();
    int count = 0;
    for (const auto& b : bindings) {
        if (count >= maxCount) break;
        strncpy_s(out[count].name, sizeof(out[count].name), b.name.c_str(), _TRUNCATE);
        out[count].funcAddr = b.funcAddr;
        ++count;
    }
    return count;
}
