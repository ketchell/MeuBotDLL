#pragma once
#include <atomic>
#include <cstdint>

// Snapshot of local player state written every ~100 ms on the game thread
// (inside hooked_MainLoop) and read lock-free by gRPC handlers on any thread.
struct CachedGameState {
    std::atomic<int>      health{0};
    std::atomic<int>      maxHealth{0};
    std::atomic<int>      mana{0};
    std::atomic<int>      maxMana{0};
    std::atomic<int>      level{0};
    std::atomic<int>      soul{0};
    std::atomic<int>      stamina{0};
    std::atomic<int>      posX{0};
    std::atomic<int>      posY{0};
    std::atomic<int>      posZ{0};
    std::atomic<uint32_t> id{0};
    std::atomic<uintptr_t> playerPtr{0}; // raw LocalPlayer* currently cached
    std::atomic<bool>     valid{false};
};

inline CachedGameState g_cachedState;
