#pragma once

// Called from hooked_MainLoop (game thread) with a ~100 ms throttle.
// Reads local player stats and position by calling real game functions directly
// (no dispatcher needed — already on the game thread) and stores results in
// g_cachedState so gRPC handlers can read them without pointer chasing.
void UpdateGameStateCache();
