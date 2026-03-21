# EasyBot OffsetResolver – Quick Reference
## What This Is

A runtime offset resolver for OTClient-based Tibia private server clients.
Instead of hardcoding memory addresses per server, it automatically finds
them by scanning the Lua binding tables embedded in the client binary.

## Files

| File | Purpose |
|------|---------|
| `OffsetResolver.h` | Header – structs, class declaration, read helpers |
| `OffsetResolver.cpp` | Implementation – all scanning/disassembly logic |
| `INTEGRATION_GUIDE.cpp` | How to wire it into EasyBot's existing code |

## How It Works (30-Second Version)

1. OTClient registers C++ functions for Lua like `g_game.getLocalPlayer`
2. These registrations are in a table in `.rdata` with the function name string
   and a pointer to the C++ function
3. We find these tables by searching for known strings ("getLocalPlayer", etc.)
4. The C++ getter functions are tiny (3-5 instructions):
   `mov eax, [global_ptr]; mov eax, [eax+offset]; ret`
5. We read those instructions to extract the global pointer address and struct offset
6. Now we can read game state at runtime without hardcoded addresses

## Why Everything Was Reading 0

The old approach used hardcoded offsets from a specific server build. When those
don't match the actual binary loaded in memory, every read hits the wrong address
and returns 0 (or garbage). The resolver fixes this by finding the real addresses
at runtime.

## Build Requirements

- Windows / MSVC (uses `__try/__except`, `psapi.lib`, Win32 API)
- C++17 (`std::optional`)
- Link with `psapi.lib` (add to your CMakeLists.txt or vcxproj)
- x86 (32-bit) – this is an x86 disassembler, won't work for x64 builds

## CMake Addition

```cmake
# In your DLL's CMakeLists.txt, add:
target_sources(EasyBot PRIVATE
    OffsetResolver.h
    OffsetResolver.cpp
)
target_link_libraries(EasyBot PRIVATE psapi)
```

## Debugging

After the DLL runs, check `easybot_offsets.log` in the client's directory.
It contains a complete trace of:
- Module base address
- Which strings were found
- Which tables were walked
- Each getter's disassembly result
- Final resolved offsets

If a getter fails to parse, the log shows its first 16 bytes so you can
look it up in x32dbg and add the pattern to `AnalyzeGetter()`.

## Extending for New Fields

To add a new offset (e.g., `player_capacity`):

1. Add `uint32_t player_capacity = 0;` to `GameOffsets`
2. In `Resolve()`, add:
   ```cpp
   auto cap = FindAndAnalyze(m_playerBindings, "getCapacity");
   if (cap && cap->valid) out.player_capacity = cap->memberOff;
   ```
3. Add a read helper if you want one:
   ```cpp
   uint32_t GetPlayerCapacity(const GameOffsets& o) {
       uintptr_t lp = GetLocalPlayer(o);
       if (!lp || !o.player_capacity) return 0;
       __try { return *(uint32_t*)(lp + o.player_capacity); }
       __except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
   }
   ```

## Known Limitations

- **Lua wrapper functions**: Some Creature/Player bindings are Lua wrappers
  (they unpack Lua stack args before calling the real getter). These are harder
  to parse. See INTEGRATION_GUIDE.cpp section 4.
- **Inlined functions**: If the compiler inlined a getter, it won't appear as
  a standalone function. Fall back to manual offsets for these.
- **Creature list**: Walking `std::unordered_map` in memory is complex.
  Consider hooking `getCreatures()` through Lua instead.
- **ASLR**: Handled automatically – we use the actual runtime base address.
