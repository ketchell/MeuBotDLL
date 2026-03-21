#pragma once
#include <windows.h>
#include <cstdint>
#include "const.h"

// ============================================================================
// SEH-safe memory read helpers
//
// All functions contain __try/__except with NO C++ objects anywhere in scope,
// so they satisfy the MSVC C2712 constraint. Callers may have C++ objects —
// only these leaf functions need to be clean.
// ============================================================================

// Reads 8 bytes as a double (the layout OTClient uses for stat fields).
static double SafeReadDouble(uintptr_t base, uint32_t offset) {
    __try {
        double v;
        const void* src = reinterpret_cast<const void*>(base + offset);
        memcpy(&v, src, sizeof(double));
        return v;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0.0;
    }
}

// Reads 4 bytes as uint32_t and widens to double.
static double SafeReadU32AsDouble(uintptr_t base, uint32_t offset) {
    __try {
        uint32_t v = *reinterpret_cast<const uint32_t*>(base + offset);
        return static_cast<double>(v);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0.0;
    }
}

// Dispatches to SafeReadDouble or SafeReadU32AsDouble based on the isDouble flag.
static double SafeReadBotValue(uintptr_t base, uint32_t offset, bool isDouble) {
    if (isDouble)
        return SafeReadDouble(base, offset);
    return SafeReadU32AsDouble(base, offset);
}

// Reads a Position (int32 x, int32 y, int16 z) from (base + offset).
static Position SafeReadPosition(uintptr_t base, uint32_t offset) {
    Position result{};
    __try {
        uintptr_t ptr = base + offset;
        result.x = *reinterpret_cast<const int32_t*>(ptr);
        result.y = *reinterpret_cast<const int32_t*>(ptr + 4);
        result.z = *reinterpret_cast<const int16_t*>(ptr + 8);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return result;
}
