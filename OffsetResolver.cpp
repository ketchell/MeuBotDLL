// ============================================================================
// OffsetResolver.cpp  –  Runtime offset extraction for OTClient-based clients
// ============================================================================

#define NOMINMAX
#include "OffsetResolver.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <algorithm>
#include <Psapi.h>

#pragma comment(lib, "psapi.lib")

// ============================================================================
// Logging helper
// ============================================================================

void OffsetResolver::Log(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    m_log += buf;
    m_log += "\n";

    // Also OutputDebugString so you can see it in x32dbg / DebugView
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}

// ============================================================================
// Step 1: Find the main module (the OTClient .exe)
// ============================================================================

bool OffsetResolver::FindModuleInfo() {
    // The main module is always the first one / the process base
    HMODULE hMod = GetModuleHandleA(nullptr);
    if (!hMod) {
        m_lastError = "GetModuleHandle failed";
        return false;
    }

    MODULEINFO mi = {};
    if (!GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi))) {
        m_lastError = "GetModuleInformation failed";
        return false;
    }

    m_base = reinterpret_cast<uintptr_t>(mi.lpBaseOfDll);
    m_size = mi.SizeOfImage;

    Log("[+] Module base: 0x%08X  size: 0x%08X", (uint32_t)m_base, (uint32_t)m_size);

    // Parse PE headers to find .text and .rdata sections
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(m_base);
    auto* nt  = reinterpret_cast<IMAGE_NT_HEADERS*>(m_base + dos->e_lfanew);
    auto* sec = IMAGE_FIRST_SECTION(nt);

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        char name[9] = {};
        memcpy(name, sec[i].Name, 8);

        uintptr_t secBase = m_base + sec[i].VirtualAddress;
        size_t    secSize = sec[i].Misc.VirtualSize;

        if (strcmp(name, ".text") == 0) {
            m_codeBase = secBase;
            m_codeSize = secSize;
            Log("[+] .text section: 0x%08X  size: 0x%08X", (uint32_t)secBase, (uint32_t)secSize);
        }
        else if (strcmp(name, ".rdata") == 0) {
            m_rdataBase = secBase;
            m_rdataSize = secSize;
            Log("[+] .rdata section: 0x%08X  size: 0x%08X", (uint32_t)secBase, (uint32_t)secSize);
        }
    }

    if (!m_codeBase || !m_rdataBase) {
        // Fallback: scan the whole image
        Log("[!] Couldn't find .text/.rdata sections, using full image scan");
        m_codeBase  = m_base;
        m_codeSize  = m_size;
        m_rdataBase = m_base;
        m_rdataSize = m_size;
    }

    return true;
}

// ============================================================================
// Step 2: Find a null-terminated string in the image
// ============================================================================

uintptr_t OffsetResolver::FindString(const char* str, size_t len) {
    // Search the entire image (strings can be in .rdata or other sections)
    const uint8_t* start = reinterpret_cast<const uint8_t*>(m_base);
    const uint8_t* end   = start + m_size - len - 1;

    for (const uint8_t* p = start; p < end; p++) {
        if (memcmp(p, str, len) == 0 && p[len] == '\0') {
            return reinterpret_cast<uintptr_t>(p);
        }
    }
    return 0;
}

// ============================================================================
// Step 3: Find all DWORDs in the image that contain a specific value
// ============================================================================

std::vector<uintptr_t> OffsetResolver::FindXrefs(uintptr_t targetVA) {
    std::vector<uintptr_t> results;
    uint32_t target32 = static_cast<uint32_t>(targetVA);

    const uint8_t* start = reinterpret_cast<const uint8_t*>(m_base);
    const uint8_t* end   = start + m_size - 4;

    for (const uint8_t* p = start; p < end; p++) {
        if (*reinterpret_cast<const uint32_t*>(p) == target32) {
            results.push_back(reinterpret_cast<uintptr_t>(p));
        }
    }
    return results;
}

// ============================================================================
// Validation helpers
// ============================================================================

bool OffsetResolver::IsValidCodeAddr(uintptr_t addr) {
    // Accept anything within the module's code range
    // Some OTClient builds have multiple executable sections
    if (addr >= m_base && addr < m_base + m_size) {
        // Quick heuristic: code addresses should be in the lower ~2/3 of the image
        // and should not be obviously invalid (0xFFFF????)
        if ((addr & 0xFFFF0000) == 0xFFFF0000) return false;
        return true;
    }
    return false;
}

bool OffsetResolver::IsValidDataAddr(uintptr_t addr) {
    return addr >= m_base && addr < m_base + m_size;
}

bool OffsetResolver::IsValidStringPtr(uintptr_t addr) {
    if (!IsValidDataAddr(addr)) return false;

    // Check that it points to printable ASCII followed by a null
    __try {
        const char* s = reinterpret_cast<const char*>(addr);
        for (int i = 0; i < 128; i++) {
            if (s[i] == '\0') return i > 0;  // non-empty string
            if (s[i] < 0x20 || s[i] > 0x7E) return false;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return false;
}

// ============================================================================
// Step 4: Walk a Lua binding table from a known name-string xref
// ============================================================================

std::vector<LuaBindingEntry> OffsetResolver::WalkBindingTable(
    uintptr_t nameXrefAddr,
    uint32_t  stride,
    uint32_t  funcPtrOff,
    uint32_t  namePtrOff
) {
    std::vector<LuaBindingEntry> entries;

    // The nameXrefAddr is where we found the name pointer.
    // The entry start is at: nameXrefAddr - namePtrOff
    // But we need to figure out namePtrOff. From your scan, the name pointer
    // was at a certain position within each 0xDC-byte entry.

    // Actually, let's be smarter: we know the stride and where the funcPtr is
    // relative to the entry start. Let's auto-detect namePtrOff by looking at
    // the structure around nameXrefAddr.

    // The entry base for this entry:
    // We'll try both: nameXrefAddr itself is the namePtr slot,
    // so entryBase = nameXrefAddr - namePtrOff

    // From your scan data:
    //   Entry at img+09F5E4: FuncPtr at +0x00, StrPtr at +0x38
    //   Wait – looking more carefully at your scan:
    //   "g_game.look  FuncPtr=00D48760  StrPtr=0146CF34  (img+09F5E4)"
    //   The img offset is for the FuncPtr position.
    //   And StrPtr @ img+09F61C which is 09F61C - 09F5E4 = 0x38 bytes after FuncPtr.
    //
    // So within each entry: funcPtr is at offset 0, namePtr is at offset 0x38.
    // The stride between entries is 0xDC.
    //
    // Let's use: entryBase = the funcPtr slot address
    //            funcPtr   @ entryBase + 0
    //            namePtr   @ entryBase + 0x38

    // nameXrefAddr points to the name pointer slot.
    // So the funcPtr slot is at nameXrefAddr - 0x38 (if funcPtrOff=0, namePtrOff=0x38)
    // But our caller might pass these differently, so let's be explicit:

    uintptr_t entryBase = nameXrefAddr - namePtrOff;

    Log("[*] Walking table from entry @ 0x%08X (stride=0x%X, func@+0x%X, name@+0x%X)",
        (uint32_t)entryBase, stride, funcPtrOff, namePtrOff);

    // Walk backwards to find the table start
    uintptr_t tableStart = entryBase;
    for (int i = 0; i < 200; i++) {
        uintptr_t prev = tableStart - stride;
        if (prev < m_base) break;

        uintptr_t prevFuncAddr = *reinterpret_cast<uint32_t*>(prev + funcPtrOff);
        uintptr_t prevNameAddr = *reinterpret_cast<uint32_t*>(prev + namePtrOff);

        // Validate: funcPtr should be a valid code address, namePtr should be a string
        if (!IsValidCodeAddr(prevFuncAddr) && (prevFuncAddr & 0xFFFF0000) != 0xFFFF0000) break;
        if (!IsValidStringPtr(prevNameAddr)) break;

        tableStart = prev;
    }

    Log("[*] Table starts at 0x%08X", (uint32_t)tableStart);

    // Walk forwards from tableStart, collecting entries
    for (uintptr_t cur = tableStart; cur < m_base + m_size - stride; cur += stride) {
        uintptr_t funcAddr = *reinterpret_cast<uint32_t*>(cur + funcPtrOff);
        uintptr_t nameAddr = *reinterpret_cast<uint32_t*>(cur + namePtrOff);

        if (!IsValidStringPtr(nameAddr)) break;

        // Read the name
        const char* nameStr = reinterpret_cast<const char*>(nameAddr);
        std::string name(nameStr);

        // Filter out obviously bad function pointers but still record the entry
        // (some entries have 0xFFFF???? which are stubs/unimplemented)
        LuaBindingEntry entry;
        entry.name        = name;
        entry.funcAddr    = funcAddr;
        entry.tableOffset = cur - m_base;

        entries.push_back(entry);
    }

    Log("[*] Found %zu entries in table", entries.size());
    return entries;
}

// ============================================================================
// Step 5: Disassemble a tiny getter function
// ============================================================================
//
// We handle these common patterns (x86 32-bit):
//
// Pattern A – Direct global read + member access:
//   A1 XX XX XX XX          mov eax, ds:[XXXXXXXX]    ; load singleton
//   8B 40 YY                mov eax, [eax+YY]         ; load member (8-bit offset)
//   C3                      ret
//
// Pattern A2 – Same but with 32-bit displacement:
//   A1 XX XX XX XX          mov eax, ds:[XXXXXXXX]
//   8B 80 YY YY YY YY      mov eax, [eax+YYYYYYYY]   ; 32-bit offset
//   C3                      ret
//
// Pattern B – Via ECX (thiscall-style):
//   8B 0D XX XX XX XX       mov ecx, ds:[XXXXXXXX]    ; load singleton
//   8B 41 YY                mov eax, [ecx+YY]         ; load member
//   C3                      ret
//
// Pattern C – Two-level dereference:
//   A1 XX XX XX XX          mov eax, ds:[XXXXXXXX]
//   8B 40 YY                mov eax, [eax+YY]         ; first member (pointer)
//   8B 40 ZZ                mov eax, [eax+ZZ]         ; second member
//   C3                      ret
//
// Pattern D – Return global directly:
//   A1 XX XX XX XX          mov eax, ds:[XXXXXXXX]
//   C3                      ret
//
// Pattern E – Push/mov through esp-relative (some MSVC optimizations):
//   Various – we handle the common sub-patterns
//
// Pattern F – XOR + conditional (for bool getters like isAttacking):
//   A1 XX XX XX XX          mov eax, ds:[XXXXXXXX]
//   83 78 YY 00             cmp dword ptr [eax+YY], 0
//   ... (setnz or conditional logic)
//

GetterAnalysis OffsetResolver::AnalyzeGetter(uintptr_t funcAddr) {
    GetterAnalysis result;

    if (!IsValidCodeAddr(funcAddr)) return result;

    __try {
        const uint8_t* code = reinterpret_cast<const uint8_t*>(funcAddr);

        // --- Pattern A/A2/C/D: starts with A1 (mov eax, ds:[imm32]) ---
        if (code[0] == 0xA1) {
            result.globalAddr = *reinterpret_cast<const uint32_t*>(code + 1);

            const uint8_t* next = code + 5;

            // Pattern D: immediate ret
            if (next[0] == 0xC3) {
                result.memberOff = 0;
                result.valid = true;
                return result;
            }

            // Pattern A: mov eax, [eax+byte]
            if (next[0] == 0x8B && next[1] == 0x40) {
                result.memberOff = next[2];
                const uint8_t* after = next + 3;

                // Check for Pattern C: another mov eax,[eax+byte]
                if (after[0] == 0x8B && after[1] == 0x40) {
                    result.memberOff2 = after[2];
                    result.twoLevel = true;
                }
                // Pattern C with 32-bit offset on second level
                else if (after[0] == 0x8B && after[1] == 0x80) {
                    result.memberOff2 = *reinterpret_cast<const uint32_t*>(after + 2);
                    result.twoLevel = true;
                }

                result.valid = true;
                return result;
            }

            // Pattern A2: mov eax, [eax+dword]
            if (next[0] == 0x8B && next[1] == 0x80) {
                result.memberOff = *reinterpret_cast<const uint32_t*>(next + 2);
                const uint8_t* after = next + 6;

                if (after[0] == 0x8B && after[1] == 0x40) {
                    result.memberOff2 = after[2];
                    result.twoLevel = true;
                }
                else if (after[0] == 0x8B && after[1] == 0x80) {
                    result.memberOff2 = *reinterpret_cast<const uint32_t*>(after + 2);
                    result.twoLevel = true;
                }

                result.valid = true;
                return result;
            }

            // mov ecx, [eax+byte] then mov eax, [ecx+byte]
            if (next[0] == 0x8B && next[1] == 0x48) {
                result.memberOff = next[2];
                const uint8_t* after = next + 3;
                if (after[0] == 0x8B && after[1] == 0x41) {
                    result.memberOff2 = after[2];
                    result.twoLevel = true;
                }
                result.valid = true;
                return result;
            }

            // mov eax, [eax+byte] with 0x00 base (member offset 0)
            if (next[0] == 0x8B && next[1] == 0x00) {
                result.memberOff = 0;
                result.valid = true;
                return result;
            }

            // Pattern F: cmp dword ptr [eax+YY], 0  (for bool checks)
            if (next[0] == 0x83 && next[1] == 0x78) {
                result.memberOff = next[2];
                result.valid = true;
                return result;
            }
            // cmp with 32-bit offset
            if (next[0] == 0x83 && next[1] == 0xB8) {
                result.memberOff = *reinterpret_cast<const uint32_t*>(next + 2);
                result.valid = true;
                return result;
            }

            // If we got the global but can't parse the member access, still useful
            result.valid = true;
            return result;
        }

        // --- Pattern B: starts with 8B 0D (mov ecx, ds:[imm32]) ---
        if (code[0] == 0x8B && code[1] == 0x0D) {
            result.globalAddr = *reinterpret_cast<const uint32_t*>(code + 2);

            const uint8_t* next = code + 6;

            // mov eax, [ecx+byte]
            if (next[0] == 0x8B && next[1] == 0x41) {
                result.memberOff = next[2];
                result.valid = true;
                return result;
            }
            // mov eax, [ecx+dword]
            if (next[0] == 0x8B && next[1] == 0x81) {
                result.memberOff = *reinterpret_cast<const uint32_t*>(next + 2);
                result.valid = true;
                return result;
            }
            // mov eax, [ecx] (offset 0)
            if (next[0] == 0x8B && next[1] == 0x01) {
                result.memberOff = 0;
                result.valid = true;
                return result;
            }

            result.valid = true;
            return result;
        }

        // --- Pattern: starts with 8B 15 (mov edx, ds:[imm32]) ---
        if (code[0] == 0x8B && code[1] == 0x15) {
            result.globalAddr = *reinterpret_cast<const uint32_t*>(code + 2);
            const uint8_t* next = code + 6;

            if (next[0] == 0x8B && next[1] == 0x42) {
                result.memberOff = next[2];
                result.valid = true;
                return result;
            }
            if (next[0] == 0x8B && next[1] == 0x82) {
                result.memberOff = *reinterpret_cast<const uint32_t*>(next + 2);
                result.valid = true;
                return result;
            }

            result.valid = true;
            return result;
        }

        // --- Pattern: plain thiscall getter (ecx = this, no global embedded) ---
        // 8B 41 XX [8B 40/80 YY] [C3]  →  mov eax,[ecx+byte]; [optional 2nd deref]
        if (code[0] == 0x8B && code[1] == 0x41) {
            result.memberOff  = code[2];
            result.globalAddr = 0;
            // Peek for double-deref: mov eax,[eax+byte/dword]
            if (code[3] == 0x8B && code[4] == 0x40) {
                result.memberOff2 = code[5];
                result.twoLevel   = true;
            } else if (code[3] == 0x8B && code[4] == 0x80) {
                result.memberOff2 = *reinterpret_cast<const uint32_t*>(code + 5);
                result.twoLevel   = true;
            }
            result.valid = true;
            return result;
        }
        // 8B 81 XX XX XX XX [8B 40/80 YY] [C3]  →  mov eax,[ecx+dword]; [optional 2nd deref]
        if (code[0] == 0x8B && code[1] == 0x81) {
            result.memberOff  = *reinterpret_cast<const uint32_t*>(code + 2);
            result.globalAddr = 0;
            if (code[6] == 0x8B && code[7] == 0x40) {
                result.memberOff2 = code[8];
                result.twoLevel   = true;
            } else if (code[6] == 0x8B && code[7] == 0x80) {
                result.memberOff2 = *reinterpret_cast<const uint32_t*>(code + 8);
                result.twoLevel   = true;
            }
            result.valid = true;
            return result;
        }
        // 8A 41 XX [C3]  →  mov al, [ecx+byte_off]; ret  (byte/bool thiscall getter)
        if (code[0] == 0x8A && code[1] == 0x41) {
            result.memberOff  = code[2];
            result.globalAddr = 0;
            result.valid      = true;
            return result;
        }
        // 8A 81 XX XX XX XX [C3]  →  mov al, [ecx+dword_off]; ret  (byte, large offset)
        if (code[0] == 0x8A && code[1] == 0x81) {
            result.memberOff  = *reinterpret_cast<const uint32_t*>(code + 2);
            result.globalAddr = 0;
            result.valid      = true;
            return result;
        }
        // DD 81 XX XX XX XX [C3]  →  fld qword ptr [ecx+dword_off]; ret  (double member)
        if (code[0] == 0xDD && code[1] == 0x81) {
            result.memberOff  = *reinterpret_cast<const uint32_t*>(code + 2);
            result.globalAddr = 0;
            result.isDouble   = true;
            result.valid      = true;
            return result;
        }
        // --- movzx / 66-prefix thiscall getters (no prologue) ---
        // 0F B7 41 XX — movzx eax, word ptr [ecx+byte]
        if (code[0] == 0x0F && code[1] == 0xB7 && code[2] == 0x41) {
            result.memberOff  = code[3];
            result.globalAddr = 0;
            result.valid      = true;
            return result;
        }
        // 0F B6 41 XX — movzx eax, byte ptr [ecx+byte]
        if (code[0] == 0x0F && code[1] == 0xB6 && code[2] == 0x41) {
            result.memberOff  = code[3];
            result.globalAddr = 0;
            result.valid      = true;
            return result;
        }
        // 0F B7 81 XX XX XX XX — movzx eax, word ptr [ecx+dword]
        if (code[0] == 0x0F && code[1] == 0xB7 && code[2] == 0x81) {
            result.memberOff  = *reinterpret_cast<const uint32_t*>(code + 3);
            result.globalAddr = 0;
            result.valid      = true;
            return result;
        }
        // 0F B6 81 XX XX XX XX — movzx eax, byte ptr [ecx+dword]
        if (code[0] == 0x0F && code[1] == 0xB6 && code[2] == 0x81) {
            result.memberOff  = *reinterpret_cast<const uint32_t*>(code + 3);
            result.globalAddr = 0;
            result.valid      = true;
            return result;
        }
        // 66 8B 41 XX — mov ax, word ptr [ecx+byte]
        if (code[0] == 0x66 && code[1] == 0x8B && code[2] == 0x41) {
            result.memberOff  = code[3];
            result.globalAddr = 0;
            result.valid      = true;
            return result;
        }
        // 66 8B 81 XX XX XX XX — mov ax, word ptr [ecx+dword]
        if (code[0] == 0x66 && code[1] == 0x8B && code[2] == 0x81) {
            result.memberOff  = *reinterpret_cast<const uint32_t*>(code + 3);
            result.globalAddr = 0;
            result.valid      = true;
            return result;
        }

        // --- Pattern: push ebp; mov ebp,esp; then A1 or mov (function prologue) ---
        if (code[0] == 0x55 && code[1] == 0x8B && code[2] == 0xEC) {
            // Skip prologue and try again from code+3
            const uint8_t* inner = code + 3;

            if (inner[0] == 0xA1) {
                result.globalAddr = *reinterpret_cast<const uint32_t*>(inner + 1);
                const uint8_t* next = inner + 5;

                if (next[0] == 0x8B && next[1] == 0x40) {
                    result.memberOff = next[2];
                    result.valid = true;
                }
                else if (next[0] == 0x8B && next[1] == 0x80) {
                    result.memberOff = *reinterpret_cast<const uint32_t*>(next + 2);
                    result.valid = true;
                }
                else {
                    result.valid = true;  // got global at least
                }
                return result;
            }

            if (inner[0] == 0x8B && inner[1] == 0x0D) {
                result.globalAddr = *reinterpret_cast<const uint32_t*>(inner + 2);
                const uint8_t* next = inner + 6;

                if (next[0] == 0x8B && next[1] == 0x41) {
                    result.memberOff = next[2];
                    result.valid = true;
                }
                else if (next[0] == 0x8B && next[1] == 0x81) {
                    result.memberOff = *reinterpret_cast<const uint32_t*>(next + 2);
                    result.valid = true;
                }
                else {
                    result.valid = true;
                }
                return result;
            }

            // Prologue + thiscall: push ebp; mov ebp,esp; mov eax,[ecx+byte]; ...
            if (inner[0] == 0x8B && inner[1] == 0x41) {
                result.memberOff  = inner[2];
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            if (inner[0] == 0x8B && inner[1] == 0x81) {
                result.memberOff  = *reinterpret_cast<const uint32_t*>(inner + 2);
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // Bool/byte thiscall with prologue: 55 8B EC 8A 41 XX
            if (inner[0] == 0x8A && inner[1] == 0x41) {
                result.memberOff  = inner[2];
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // 55 8B EC 8A 81 XX XX XX XX  →  mov al, [ecx+dword_off]  (byte, large off)
            if (inner[0] == 0x8A && inner[1] == 0x81) {
                result.memberOff  = *reinterpret_cast<const uint32_t*>(inner + 2);
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // 55 8B EC DD 81 XX XX XX XX  →  fld qword ptr [ecx+dword_off]  (double)
            if (inner[0] == 0xDD && inner[1] == 0x81) {
                result.memberOff  = *reinterpret_cast<const uint32_t*>(inner + 2);
                result.globalAddr = 0;
                result.isDouble   = true;
                result.valid      = true;
                return result;
            }
            // movzx with prologue: 55 8B EC 0F B7/B6 41 XX  (word/byte member)
            if (inner[0] == 0x0F && (inner[1] == 0xB7 || inner[1] == 0xB6) &&
                inner[2] == 0x41) {
                result.memberOff  = inner[3];
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // movzx with prologue + dword offset: 55 8B EC 0F B7/B6 81 XX XX XX XX
            if (inner[0] == 0x0F && (inner[1] == 0xB7 || inner[1] == 0xB6) &&
                inner[2] == 0x81) {
                result.memberOff  = *reinterpret_cast<const uint32_t*>(inner + 3);
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // 66 8B with prologue: 55 8B EC 66 8B 41 XX
            if (inner[0] == 0x66 && inner[1] == 0x8B && inner[2] == 0x41) {
                result.memberOff  = inner[3];
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // 66 8B with prologue + dword offset: 55 8B EC 66 8B 81 XX XX XX XX
            if (inner[0] == 0x66 && inner[1] == 0x8B && inner[2] == 0x81) {
                result.memberOff  = *reinterpret_cast<const uint32_t*>(inner + 3);
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // Position struct out-param: 55 8B EC 8B 45 08 8B 51 XX 8B 49 YY 89 10 ...
            // getPosition(Position* out): mov edx,[ecx+XX]; mov ecx,[ecx+YY]; mov [eax],edx; ...
            // XX is the position struct base offset inside the Creature object.
            if (inner[0] == 0x8B && inner[1] == 0x45 && inner[2] == 0x08 &&
                inner[3] == 0x8B && inner[4] == 0x51 &&
                inner[6] == 0x8B && inner[7] == 0x49) {
                result.memberOff  = inner[5];   // XX from "8B 51 XX"
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // Setter: 55 8B EC 8B 45 08 89 81 XX XX XX XX 5D C2 04 00
            // mov [ecx+dword], eax — not a getter but offset is still useful
            if (inner[0] == 0x8B && inner[1] == 0x45 && inner[2] == 0x08 &&
                inner[3] == 0x89 && inner[4] == 0x81) {
                uint32_t off = *reinterpret_cast<const uint32_t*>(inner + 5);
                Log("  [!] setter pattern at 0x%08X offset 0x%X — extracting offset",
                    (uint32_t)funcAddr, off);
                result.memberOff  = off;
                result.globalAddr = 0;
                result.valid      = true;
                return result;
            }
            // SEH prologue: 55 8B EC 6A FF 68 [handler imm32] 64 A1 00 00 00 00 ...
            // Scan past the SEH frame setup for the real member-access instruction.
            //
            // Decode ModRM properly:
            //   mod=01 rm=101 → [ebp+disp8]  = stack/param → SKIP
            //   mod=01 rm=100 → SIB byte      = esp-relative → SKIP
            //   mod=01 other  → [reg+disp8]   = member access; skip if disp==0 (vtable)
            //   mod=10 (same rules but disp32)
            if (inner[0] == 0x6A && inner[1] == 0xFF && inner[2] == 0x68) {
                // Start at 0x20 to clear the SEH frame-install sequence
                // (push -1 / push handler / mov eax,fs:[0] / push eax /
                //  sub esp,N / push regs / lea eax,[ebp-M] / mov fs:[0],eax).
                // End at 0x80 — real member access is always within the first 128 bytes.
                // Only accept offsets in range (0, 0x1000) to avoid large constants.
                const uint8_t* scan    = code + 0x20;
                const uint8_t* scanEnd = code + 0x80;
                while (scan < scanEnd) {
                    // A1 imm32 — global singleton load; extract member off from next insn
                    if (scan[0] == 0xA1) {
                        uintptr_t ga = *reinterpret_cast<const uint32_t*>(scan + 1);
                        const uint8_t* nxt = scan + 5;
                        if (nxt[0] == 0x8B) {
                            uint8_t nmod = (nxt[1] >> 6) & 3;
                            uint8_t nrm  = nxt[1] & 7;
                            if (nmod == 1 && nrm != 4 && nrm != 5 && nxt[2] != 0) {
                                result.globalAddr = ga; result.memberOff = nxt[2];
                                result.valid = true; return result;
                            }
                            if (nmod == 2 && nrm != 4 && nrm != 5) {
                                uint32_t d = *reinterpret_cast<const uint32_t*>(nxt + 2);
                                if (d != 0 && d < 0x1000u) {
                                    result.globalAddr = ga; result.memberOff = d;
                                    result.valid = true; return result;
                                }
                            }
                        }
                        scan += 5; continue;
                    }
                    // 8B ModRM [Disp8|Disp32] — generic member-load decoder
                    if (scan[0] == 0x8B) {
                        uint8_t modrm = scan[1];
                        uint8_t mod   = (modrm >> 6) & 3;
                        uint8_t rm    = modrm & 7;
                        // mod=01: [reg+disp8] — skip ebp-based (rm=5) and SIB (rm=4)
                        if (mod == 1 && rm != 4 && rm != 5) {
                            uint8_t disp = scan[2];
                            if (disp != 0) {
                                result.memberOff  = disp;
                                result.globalAddr = 0;
                                result.valid      = true;
                                return result;
                            }
                            scan += 3; continue;
                        }
                        // mod=10: [reg+disp32] — skip ebp-based and SIB
                        if (mod == 2 && rm != 4 && rm != 5) {
                            uint32_t disp = *reinterpret_cast<const uint32_t*>(scan + 2);
                            if (disp != 0 && disp < 0x1000u) {
                                result.memberOff  = disp;
                                result.globalAddr = 0;
                                result.valid      = true;
                                return result;
                            }
                            scan += 6; continue;
                        }
                    }
                    // 0F B7/B6 ModRM [Disp] — movzx from word/byte member
                    if (scan[0] == 0x0F && (scan[1] == 0xB7 || scan[1] == 0xB6)) {
                        uint8_t modrm = scan[2];
                        uint8_t mod   = (modrm >> 6) & 3;
                        uint8_t rm    = modrm & 7;
                        if (mod == 1 && rm != 4 && rm != 5) {
                            uint8_t disp = scan[3];
                            if (disp != 0) {
                                result.memberOff  = disp;
                                result.globalAddr = 0;
                                result.valid      = true;
                                return result;
                            }
                            scan += 4; continue;
                        }
                        if (mod == 2 && rm != 4 && rm != 5) {
                            uint32_t disp = *reinterpret_cast<const uint32_t*>(scan + 3);
                            if (disp != 0 && disp < 0x1000u) {
                                result.memberOff  = disp;
                                result.globalAddr = 0;
                                result.valid      = true;
                                return result;
                            }
                            scan += 7; continue;
                        }
                    }
                    // 66 8B ModRM [Disp] — 16-bit mov from member
                    if (scan[0] == 0x66 && scan[1] == 0x8B) {
                        uint8_t modrm = scan[2];
                        uint8_t mod   = (modrm >> 6) & 3;
                        uint8_t rm    = modrm & 7;
                        if (mod == 1 && rm != 4 && rm != 5) {
                            uint8_t disp = scan[3];
                            if (disp != 0) {
                                result.memberOff  = disp;
                                result.globalAddr = 0;
                                result.valid      = true;
                                return result;
                            }
                            scan += 4; continue;
                        }
                        if (mod == 2 && rm != 4 && rm != 5) {
                            uint32_t disp = *reinterpret_cast<const uint32_t*>(scan + 3);
                            if (disp != 0 && disp < 0x1000u) {
                                result.memberOff  = disp;
                                result.globalAddr = 0;
                                result.valid      = true;
                                return result;
                            }
                            scan += 7; continue;
                        }
                    }
                    scan++;
                }
                return result;
            }
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("[!] Exception analyzing getter at 0x%08X", (uint32_t)funcAddr);
    }

    return result;
}

// ============================================================================
// Step 6: Find a binding by name and analyze its getter
// ============================================================================

std::optional<GetterAnalysis> OffsetResolver::FindAndAnalyze(
    const std::vector<LuaBindingEntry>& bindings,
    const std::string& name
) {
    for (const auto& entry : bindings) {
        if (entry.name == name) {
            auto analysis = AnalyzeGetter(entry.funcAddr);
            if (analysis.valid) {
                Log("  [+] %s @ 0x%08X → global=0x%08X member=0x%X%s",
                    name.c_str(), (uint32_t)entry.funcAddr,
                    (uint32_t)analysis.globalAddr, analysis.memberOff,
                    analysis.twoLevel ? " (two-level)" : "");
                return analysis;
            }
            else {
                Log("  [-] %s @ 0x%08X → could not disassemble",
                    name.c_str(), (uint32_t)entry.funcAddr);

                // Dump first 32 bytes for debugging
                const uint8_t* code = reinterpret_cast<const uint8_t*>(entry.funcAddr);
                char hex[128];
                snprintf(hex, sizeof(hex),
                    "      bytes: %02X %02X %02X %02X %02X %02X %02X %02X "
                    "%02X %02X %02X %02X %02X %02X %02X %02X "
                    "%02X %02X %02X %02X %02X %02X %02X %02X "
                    "%02X %02X %02X %02X %02X %02X %02X %02X",
                    code[0], code[1], code[2], code[3], code[4], code[5], code[6], code[7],
                    code[8], code[9], code[10],code[11],code[12],code[13],code[14],code[15],
                    code[16],code[17],code[18],code[19],code[20],code[21],code[22],code[23],
                    code[24],code[25],code[26],code[27],code[28],code[29],code[30],code[31]);
                Log("%s", hex);
            }
            return std::nullopt;
        }
    }

    Log("  [-] %s not found in binding table", name.c_str());
    return std::nullopt;
}

// ============================================================================
// FindGlobalSingleton: locate the global variable that holds a named singleton
// ============================================================================
//
// Strategy:
//   1. Find the name string (e.g. "g_game") in .rdata.
//   2. Find all DWORDs in .text that equal that string's VA — these are
//      `push offset "g_game"` or similar references in the Lua registration code.
//   3. Scan ±0x40 bytes around each such xref for a global memory-load:
//        A1 XX XX XX XX          mov eax, ds:[imm32]
//        8B 0D XX XX XX XX       mov ecx, ds:[imm32]
//        8B 15 XX XX XX XX       mov edx, ds:[imm32]
//        FF 35 XX XX XX XX       push dword ptr [imm32]
//   4. Validate the candidate: must be inside the module but NOT in .rdata
//      (globals live in .data/.bss, not in read-only data).
//
uintptr_t OffsetResolver::FindGlobalSingleton(const char* name) {
    Log("[*] FindGlobalSingleton(\"%s\")", name);

    uintptr_t nameStr = FindString(name, strlen(name));
    if (!nameStr) {
        Log("  [-] String \"%s\" not found in image", name);
        return 0;
    }
    Log("  [+] String \"%s\" at 0x%08X", name, (uint32_t)nameStr);

    auto xrefs = FindXrefs(nameStr);
    Log("  [+] %zu xref(s) to string", xrefs.size());

    for (uintptr_t xref : xrefs) {
        // Only care about xrefs that sit inside the code section (.text).
        // Xrefs in .rdata are binding-table entries — not useful here.
        if (xref < m_codeBase || xref >= m_codeBase + m_codeSize) continue;

        Log("  [*] .text xref at 0x%08X — scanning ±0x40 for global load", (uint32_t)xref);

        uintptr_t winStart = (xref > m_codeBase + 0x40) ? xref - 0x40 : m_codeBase;
        uintptr_t winEnd   = (std::min)(xref + 0x40, m_codeBase + m_codeSize - 6);

        const uint8_t* p   = reinterpret_cast<const uint8_t*>(winStart);
        const uint8_t* end = reinterpret_cast<const uint8_t*>(winEnd);

        for (; p < end; p++) {
            uintptr_t candidate = 0;

            // A1 imm32  —  mov eax, ds:[imm32]  (5 bytes)
            if (p[0] == 0xA1) {
                candidate = *reinterpret_cast<const uint32_t*>(p + 1);
            }
            // 8B 0D imm32  —  mov ecx, ds:[imm32]  (6 bytes)
            else if (p[0] == 0x8B && p[1] == 0x0D) {
                candidate = *reinterpret_cast<const uint32_t*>(p + 2);
            }
            // 8B 15 imm32  —  mov edx, ds:[imm32]  (6 bytes)
            else if (p[0] == 0x8B && p[1] == 0x15) {
                candidate = *reinterpret_cast<const uint32_t*>(p + 2);
            }
            // FF 35 imm32  —  push dword ptr [imm32]  (6 bytes)
            else if (p[0] == 0xFF && p[1] == 0x35) {
                candidate = *reinterpret_cast<const uint32_t*>(p + 2);
            }

            if (!candidate || !IsValidDataAddr(candidate)) continue;

            // Reject addresses that fall inside .rdata — singletons live in
            // writable .data/.bss, not in the read-only data section.
            if (candidate >= m_rdataBase && candidate < m_rdataBase + m_rdataSize) continue;

            Log("  [+] g_game global ptr at 0x%08X (insn @ 0x%08X near xref 0x%08X)",
                (uint32_t)candidate,
                (uint32_t)reinterpret_cast<uintptr_t>(p),
                (uint32_t)xref);
            return candidate;
        }
    }

    Log("  [-] No global load found near \"%s\" xrefs", name);
    return 0;
}

// Isolated helper: safely reads 32 bytes from a raw address into buf.
// No C++ objects on the stack → MSVC allows __try/__except (no C2712).
// Returns true on success, false if the address faults.
static bool TryReadBytes32(uint32_t addr, uint8_t buf[32]) {
    __try {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(addr);
        for (int i = 0; i < 32; i++) buf[i] = src[i];
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// ============================================================================
// ResolveMethodDirect
// ============================================================================
//
// Finds the getter function for a named Creature/Player Lua method without
// relying on a contiguous binding table.  Two complementary strategies:
//
//  Strategy A – near-xref probe:
//    Find all DWORDs in the image that equal &"methodName".  For each such
//    xref, read the DWORD at common slot offsets (±N bytes) and check if it
//    points into .text.  Try AnalyzeGetter on every code candidate.
//
//  Strategy B – push-string / push-func scan:
//    OTClient's Lua registration typically emits something like:
//        push offset "methodName"       ; 68 XX XX XX XX
//        push offset Creature__method   ; 68 YY YY YY YY
//        call bindClassMemberFunction
//    Scan .text for `push strAddr`, then search ±0x25 bytes for another
//    `push imm32` whose imm32 lands inside .text.
//
GetterAnalysis OffsetResolver::ResolveMethodDirect(const char* methodName) {
    Log("  [direct] '%s'", methodName);

    uintptr_t strAddr = FindString(methodName, strlen(methodName));
    if (!strAddr) {
        Log("    [-] string not found");
        return {};
    }
    Log("    [+] string @ 0x%08X", (uint32_t)strAddr);

    auto allXrefs = FindXrefs(strAddr);
    Log("    [+] %zu xrefs", allXrefs.size());

    // --- Strategy A: probe DWORDs at common slot offsets near each xref ---
    // These cover the funcPtr positions in 0xDC-stride tables (namePtrOff=0x38,
    // 0x70, etc.) and also adjacent-DWORD layouts used in some forks.
    static const int32_t kProbeOffsets[] = {
        -0x38, -0x34, -0x30, -0x3C,
        -0x70, -0x74,
        -0x04, -0x08, +0x04, +0x08,
    };

    for (uintptr_t xref : allXrefs) {
        for (int32_t off : kProbeOffsets) {
            uintptr_t slot = (uintptr_t)((intptr_t)xref + off);
            if (slot < m_base || slot + 4 > m_base + m_size) continue;

            uint32_t candidate = *reinterpret_cast<const uint32_t*>(slot);
            if (candidate < m_codeBase || candidate >= m_codeBase + m_codeSize) continue;

            if (m_claimedFuncAddrs.count(candidate)) {
                Log("    [A] xref=0x%08X off=%+d → cand=0x%08X (claimed, skip)",
                    (uint32_t)xref, off, candidate);
                continue;
            }
            Log("    [A] xref=0x%08X off=%+d → cand=0x%08X",
                (uint32_t)xref, off, candidate);

            GetterAnalysis ga = AnalyzeGetter(candidate);
            if (ga.valid && ga.memberOff > 0) {
                m_claimedFuncAddrs.insert(candidate);
                Log("    [+] probe hit: func=0x%08X member=0x%X", candidate, ga.memberOff);
                return ga;
            }
            // Dump 32 bytes so unrecognised patterns are visible in the log
            {
                uint8_t diagBuf[32];
                if (TryReadBytes32(candidate, diagBuf)) {
                    Log("    [?] no match @ 0x%08X: "
                        "%02X %02X %02X %02X %02X %02X %02X %02X "
                        "%02X %02X %02X %02X %02X %02X %02X %02X "
                        "%02X %02X %02X %02X %02X %02X %02X %02X "
                        "%02X %02X %02X %02X %02X %02X %02X %02X",
                        candidate,
                        diagBuf[0], diagBuf[1], diagBuf[2], diagBuf[3],
                        diagBuf[4], diagBuf[5], diagBuf[6], diagBuf[7],
                        diagBuf[8], diagBuf[9], diagBuf[10],diagBuf[11],
                        diagBuf[12],diagBuf[13],diagBuf[14],diagBuf[15],
                        diagBuf[16],diagBuf[17],diagBuf[18],diagBuf[19],
                        diagBuf[20],diagBuf[21],diagBuf[22],diagBuf[23],
                        diagBuf[24],diagBuf[25],diagBuf[26],diagBuf[27],
                        diagBuf[28],diagBuf[29],diagBuf[30],diagBuf[31]);
                } else {
                    Log("    [?] no match @ 0x%08X: <read fault>", candidate);
                }
            }
        }
    }

    // --- Strategy B: scan .text for `push strAddr`, find nearby `push funcAddr` ---
    uint32_t strAddr32 = (uint32_t)strAddr;
    const uint8_t* tStart = reinterpret_cast<const uint8_t*>(m_codeBase);
    const uint8_t* tEnd   = tStart + m_codeSize - 6;

    for (const uint8_t* p = tStart; p < tEnd; p++) {
        if (p[0] != 0x68) continue;
        if (*reinterpret_cast<const uint32_t*>(p + 1) != strAddr32) continue;

        Log("    [B] push strAddr @ 0x%08X",
            (uint32_t)reinterpret_cast<uintptr_t>(p));

        const uint8_t* s0 = (p > tStart + 0x25) ? p - 0x25 : tStart;
        const uint8_t* s1 = (p + 0x25 < tEnd)   ? p + 0x25 : tEnd;

        for (const uint8_t* q = s0; q < s1; q++) {
            if (q == p) continue;
            if (q[0] != 0x68) continue;
            uint32_t fc = *reinterpret_cast<const uint32_t*>(q + 1);
            if (fc < m_codeBase || fc >= m_codeBase + m_codeSize) continue;

            if (m_claimedFuncAddrs.count(fc)) {
                Log("    [B] cand=0x%08X (claimed, skip)", fc);
                continue;
            }
            Log("    [B] cand=0x%08X (push @ 0x%08X)",
                fc, (uint32_t)reinterpret_cast<uintptr_t>(q));

            GetterAnalysis ga = AnalyzeGetter(fc);
            if (ga.valid && ga.memberOff > 0) {
                m_claimedFuncAddrs.insert(fc);
                Log("    [+] push-scan hit: func=0x%08X member=0x%X", fc, ga.memberOff);
                return ga;
            }
            {
                uint8_t diagBuf[32];
                if (TryReadBytes32(fc, diagBuf)) {
                    Log("    [?] no match @ 0x%08X: "
                        "%02X %02X %02X %02X %02X %02X %02X %02X "
                        "%02X %02X %02X %02X %02X %02X %02X %02X "
                        "%02X %02X %02X %02X %02X %02X %02X %02X "
                        "%02X %02X %02X %02X %02X %02X %02X %02X",
                        fc,
                        diagBuf[0], diagBuf[1], diagBuf[2], diagBuf[3],
                        diagBuf[4], diagBuf[5], diagBuf[6], diagBuf[7],
                        diagBuf[8], diagBuf[9], diagBuf[10],diagBuf[11],
                        diagBuf[12],diagBuf[13],diagBuf[14],diagBuf[15],
                        diagBuf[16],diagBuf[17],diagBuf[18],diagBuf[19],
                        diagBuf[20],diagBuf[21],diagBuf[22],diagBuf[23],
                        diagBuf[24],diagBuf[25],diagBuf[26],diagBuf[27],
                        diagBuf[28],diagBuf[29],diagBuf[30],diagBuf[31]);
                } else {
                    Log("    [?] no match @ 0x%08X: <read fault>", fc);
                }
            }
        }
    }

    Log("    [-] unresolved");
    return {};
}

// ============================================================================
// Main resolution pipeline
// ============================================================================

bool OffsetResolver::Resolve(GameOffsets& out) {
    Log("=== OffsetResolver starting ===");

    // ---- Step 1: Module info ----
    if (!FindModuleInfo()) {
        Log("[!] Failed to get module info: %s", m_lastError.c_str());
        return false;
    }

    // ---- Step 2: Find "g_game" Lua binding table ----
    // We look for a known function name string and find the table from there.
    // "look" is a good anchor because it's short, unique in the g_game table,
    // and your scan confirmed it works.

    // Strategy: find "getLocalPlayer\0" string, find its xref in the table,
    // then walk the table.

    Log("\n[*] Searching for g_game binding table...");

    // Try multiple anchor strings in case one fails
    const char* gameAnchors[] = {
        "getLocalPlayer",
        "getCharacterName",
        "getProtocolVersion",
        "look",
        "walk",
        "attack",
        nullptr
    };

    bool foundGameTable = false;
    for (int i = 0; gameAnchors[i] != nullptr; i++) {
        const char* anchor = gameAnchors[i];
        size_t len = strlen(anchor);

        uintptr_t strAddr = FindString(anchor, len);
        if (!strAddr) {
            Log("  [-] String '%s' not found", anchor);
            continue;
        }
        Log("  [+] String '%s' found at 0x%08X", anchor, (uint32_t)strAddr);

        // Find xrefs to this string
        auto xrefs = FindXrefs(strAddr);
        Log("  [+] %zu xrefs to '%s'", xrefs.size(), anchor);

        // Try each xref – one of them should be in the binding table
        for (uintptr_t xref : xrefs) {
            // Check if this looks like it's inside a binding table:
            // The name pointer slot should be at a consistent offset from
            // a funcPtr slot that contains a valid code address.

            // From your scan: funcPtr is at the entry start, namePtr is at +0x38.
            // So if xref is the namePtr slot, funcPtr is at xref - 0x38
            uint32_t namePtrOff = 0x38;

            uintptr_t possibleFuncSlot = xref - namePtrOff;
            if (possibleFuncSlot < m_base) continue;

            uint32_t possibleFunc = *reinterpret_cast<uint32_t*>(possibleFuncSlot);
            if (!IsValidCodeAddr(possibleFunc)) {
                // Try other common name offsets
                namePtrOff = 0x70;
                possibleFuncSlot = xref - namePtrOff + 0x38;  // funcPtr at +0x38
                if (possibleFuncSlot < m_base) continue;
                possibleFunc = *reinterpret_cast<uint32_t*>(possibleFuncSlot);
                if (!IsValidCodeAddr(possibleFunc)) continue;
            }

            Log("  [*] Trying table walk from xref 0x%08X (namePtrOff=0x%X)",
                (uint32_t)xref, namePtrOff);

            m_gameBindings = WalkBindingTable(xref, TABLE_STRIDE, 0, namePtrOff);

            if (m_gameBindings.size() >= 20) {
                Log("  [+] Found g_game table with %zu entries!", m_gameBindings.size());
                foundGameTable = true;
                break;
            }
        }

        if (foundGameTable) break;
    }

    if (!foundGameTable) {
        m_lastError = "Could not find g_game Lua binding table";
        Log("[!] %s", m_lastError.c_str());
        return false;
    }

    // ---- Step 3: Analyze g_game getters ----
    Log("\n[*] Analyzing g_game getter functions...");

    // getLocalPlayer gives us the localPlayer member offset, and — when the
    // getter embeds a global load — the g_game singleton address too.
    // For plain thiscall getters (8B 41 XX C3) globalAddr comes back 0; in
    // that case we find the global separately via "g_game" string xrefs.
    auto lpAnalysis = FindAndAnalyze(m_gameBindings, "getLocalPlayer");
    if (lpAnalysis && lpAnalysis->valid) {
        out.game_localPlayer = lpAnalysis->memberOff;
        Log("  >> Game::m_localPlayer offset:  0x%X", out.game_localPlayer);

        if (lpAnalysis->globalAddr != 0) {
            out.g_game_ptr_addr = lpAnalysis->globalAddr;
            Log("  >> g_game global ptr (from getter): 0x%08X", (uint32_t)out.g_game_ptr_addr);
        }
        else {
            // Thiscall getter — ecx was already the Game* this-pointer.
            // The global that holds it is not encoded in the function; find it
            // by scanning for the "g_game" Lua registration string.
            Log("  [*] Thiscall getter (globalAddr==0) — searching via \"g_game\" xrefs...");
            out.g_game_ptr_addr = FindGlobalSingleton("g_game");
            if (out.g_game_ptr_addr) {
                Log("  >> g_game global ptr (from xref scan): 0x%08X", (uint32_t)out.g_game_ptr_addr);
            }
            else {
                Log("  [!] Could not locate g_game global via xref scan");
            }
        }
    }
    else {
        m_lastError = "Failed to analyze getLocalPlayer";
        Log("[!] %s", m_lastError.c_str());
        return false;
    }

    // Other game-level getters
    auto pv = FindAndAnalyze(m_gameBindings, "getProtocolVersion");
    if (pv && pv->valid) out.game_protocolVersion = pv->memberOff;

    auto cv = FindAndAnalyze(m_gameBindings, "getClientVersion");
    if (cv && cv->valid) out.game_clientVersion = cv->memberOff;

    auto cm = FindAndAnalyze(m_gameBindings, "getChaseMode");
    if (cm && cm->valid) out.game_chaseMode = cm->memberOff;

    auto pm = FindAndAnalyze(m_gameBindings, "getPVPMode");
    if (pm && pm->valid) out.game_pvpMode = pm->memberOff;

    auto ia = FindAndAnalyze(m_gameBindings, "isAttacking");
    if (ia && ia->valid) out.game_attackingCreature = ia->memberOff;

    auto fc = FindAndAnalyze(m_gameBindings, "getFollowingCreature");
    if (fc && fc->valid) out.game_followingCreature = fc->memberOff;

    // ---- Step 4: Find the Creature/Player/LocalPlayer binding table ----
    // These are registered under different class names. OTClient typically
    // registers them as "Creature", "Player", "LocalPlayer" classes.
    // The getter names are like "getHealth", "getMana", "getPosition", etc.

    Log("\n[*] Searching for Creature/Player binding tables...");

    const char* playerAnchors[] = {
        "getHealth",
        "getMana",
        "getName",
        "getId",
        "getLevel",
        "getPosition",
        "getSpeed",
        nullptr
    };

    // Try all xrefs for every anchor; keep the largest table found.
    // After exhausting all options, validate it contains the required methods.
    std::vector<LuaBindingEntry> bestPlayerTable;
    uintptr_t    bestPlayerXref   = 0;
    const char*  bestPlayerAnchor = nullptr;

    auto hasRequiredCreatureMethods = [](const std::vector<LuaBindingEntry>& tbl) -> bool {
        bool hasHealth = false, hasMana = false, hasName = false;
        for (const auto& e : tbl) {
            if (e.name == "getHealth") hasHealth = true;
            if (e.name == "getMana")   hasMana   = true;
            if (e.name == "getName")   hasName   = true;
        }
        return hasHealth && hasMana && hasName;
    };

    for (int i = 0; playerAnchors[i] != nullptr; i++) {
        const char* anchor = playerAnchors[i];
        size_t len = strlen(anchor);

        uintptr_t strAddr = FindString(anchor, len);
        if (!strAddr) {
            Log("  [-] String '%s' not found", anchor);
            continue;
        }
        Log("  [+] String '%s' found at 0x%08X", anchor, (uint32_t)strAddr);

        auto xrefs = FindXrefs(strAddr);
        Log("  [+] %zu xrefs to '%s'", xrefs.size(), anchor);

        for (uintptr_t xref : xrefs) {
            uint32_t namePtrOff = 0x38;
            uintptr_t possibleFuncSlot = xref - namePtrOff;
            if (possibleFuncSlot < m_base) continue;

            uint32_t possibleFunc = *reinterpret_cast<uint32_t*>(possibleFuncSlot);
            if (!IsValidCodeAddr(possibleFunc)) continue;

            auto tbl = WalkBindingTable(xref, TABLE_STRIDE, 0, namePtrOff);
            Log("  [*] xref 0x%08X → %zu entries", (uint32_t)xref, tbl.size());

            if (tbl.size() > bestPlayerTable.size()) {
                bestPlayerTable  = tbl;
                bestPlayerXref   = xref;
                bestPlayerAnchor = anchor;
            }

            // Early-out once we have a large, fully-validated table
            if (bestPlayerTable.size() >= 40 && hasRequiredCreatureMethods(bestPlayerTable))
                break;
        }

        if (bestPlayerTable.size() >= 40 && hasRequiredCreatureMethods(bestPlayerTable))
            break;
    }

    bool foundPlayerTable = false;
    if (!bestPlayerTable.empty()) {
        bool valid = hasRequiredCreatureMethods(bestPlayerTable);
        Log("  [+] Best candidate: anchor='%s' xref=0x%08X entries=%zu%s",
            bestPlayerAnchor ? bestPlayerAnchor : "?",
            (uint32_t)bestPlayerXref,
            bestPlayerTable.size(),
            valid ? " [validated: has getHealth+getMana+getName]"
                  : " [WARNING: missing required methods — may be wrong table]");
        m_playerBindings = std::move(bestPlayerTable);
        foundPlayerTable = (m_playerBindings.size() >= 10);
    }

    if (foundPlayerTable) {
        Log("\n[*] Analyzing Creature/Player getter functions...");

        // These getters operate on 'this' (the creature/player pointer),
        // not on a global. So they typically look like:
        //   mov eax, [ecx+XX]  ; ret
        // where ecx = this pointer (thiscall convention).
        //
        // OR they might be Lua wrappers that:
        //   1. Pop the Lua userdata to get the C++ object pointer
        //   2. Call the actual getter
        //
        // We handle both in AnalyzeGetter.

        auto hp = FindAndAnalyze(m_playerBindings, "getHealth");
        if (hp && hp->valid) { out.creature_health = hp->memberOff; out.creature_health_isDouble = hp->isDouble; }

        auto mhp = FindAndAnalyze(m_playerBindings, "getMaxHealth");
        if (mhp && mhp->valid) { out.creature_maxHealth = mhp->memberOff; out.creature_maxHealth_isDouble = mhp->isDouble; }

        auto pos = FindAndAnalyze(m_playerBindings, "getPosition");
        if (pos && pos->valid) out.creature_position = pos->memberOff;

        auto dir = FindAndAnalyze(m_playerBindings, "getDirection");
        if (dir && dir->valid) out.creature_direction = dir->memberOff;

        auto spd = FindAndAnalyze(m_playerBindings, "getSpeed");
        if (spd && spd->valid) out.creature_speed = spd->memberOff;

        auto skl = FindAndAnalyze(m_playerBindings, "getSkull");
        if (skl && skl->valid) out.creature_skull = skl->memberOff;

        auto shd = FindAndAnalyze(m_playerBindings, "getShield");
        if (shd && shd->valid) out.creature_shield = shd->memberOff;

        auto cid = FindAndAnalyze(m_playerBindings, "getId");
        if (cid && cid->valid) out.creature_id = cid->memberOff;

        auto nm = FindAndAnalyze(m_playerBindings, "getName");
        if (nm && nm->valid) out.creature_name = nm->memberOff;

        auto mana = FindAndAnalyze(m_playerBindings, "getMana");
        if (mana && mana->valid) { out.player_mana = mana->memberOff; out.player_mana_isDouble = mana->isDouble; }

        auto mmana = FindAndAnalyze(m_playerBindings, "getMaxMana");
        if (mmana && mmana->valid) { out.player_maxMana = mmana->memberOff; out.player_maxMana_isDouble = mmana->isDouble; }

        auto lvl = FindAndAnalyze(m_playerBindings, "getLevel");
        if (lvl && lvl->valid) { out.player_level = lvl->memberOff; out.player_level_isDouble = lvl->isDouble; }

        auto ml = FindAndAnalyze(m_playerBindings, "getMagicLevel");
        if (ml && ml->valid) { out.player_magicLevel = ml->memberOff; out.player_magicLevel_isDouble = ml->isDouble; }

        auto soul = FindAndAnalyze(m_playerBindings, "getSoul");
        if (soul && soul->valid) { out.player_soul = soul->memberOff; out.player_soul_isDouble = soul->isDouble; }

        auto stam = FindAndAnalyze(m_playerBindings, "getStamina");
        if (stam && stam->valid) { out.player_stamina = stam->memberOff; out.player_stamina_isDouble = stam->isDouble; }

        auto voc = FindAndAnalyze(m_playerBindings, "getVocation");
        if (voc && voc->valid) out.player_vocation = voc->memberOff;

        auto exp = FindAndAnalyze(m_playerBindings, "getExperience");
        if (exp && exp->valid) { out.player_experience = exp->memberOff; out.player_experience_isDouble = exp->isDouble; }
    }
    else {
        Log("[!] Could not find Creature/Player binding table — will rely on direct resolution");
    }

    // ---- Step 5: Per-method direct resolution fallback ----
    // Fills any offset that is still 0 after the table walk (or all offsets if
    // no valid table was found).  Uses ResolveMethodDirect which tries both the
    // near-xref probe and the push-string/push-func registration pattern.
    Log("\n[*] Direct method resolution (filling gaps)...");
    m_claimedFuncAddrs.clear();
    {
        auto tryDirect = [&](const char* name, uint32_t& field, bool* isDbl) {
            if (field != 0) return;   // already resolved via table
            auto ga = ResolveMethodDirect(name);
            if (ga.valid && ga.memberOff > 0) {
                field = ga.memberOff;
                if (isDbl) *isDbl = ga.isDouble;
                Log("  >> %s = 0x%X%s (direct)", name, field, ga.isDouble ? " [double]" : "");
            }
        };

        tryDirect("getHealth",     out.creature_health,     &out.creature_health_isDouble);
        tryDirect("getMaxHealth",  out.creature_maxHealth,  &out.creature_maxHealth_isDouble);
        tryDirect("getPosition",   out.creature_position,   nullptr);
        tryDirect("getDirection",  out.creature_direction,  nullptr);
        tryDirect("getSpeed",      out.creature_speed,      nullptr);
        tryDirect("getSkull",      out.creature_skull,      nullptr);
        tryDirect("getShield",     out.creature_shield,     nullptr);
        tryDirect("getId",         out.creature_id,         nullptr);
        tryDirect("getName",       out.creature_name,       nullptr);
        tryDirect("getMana",       out.player_mana,         &out.player_mana_isDouble);
        tryDirect("getMaxMana",    out.player_maxMana,      &out.player_maxMana_isDouble);
        tryDirect("getLevel",      out.player_level,        &out.player_level_isDouble);
        tryDirect("getMagicLevel", out.player_magicLevel,   &out.player_magicLevel_isDouble);
        tryDirect("getSoul",       out.player_soul,         &out.player_soul_isDouble);
        tryDirect("getStamina",    out.player_stamina,      &out.player_stamina_isDouble);
        tryDirect("getVocation",   out.player_vocation,     nullptr);
        tryDirect("getExperience", out.player_experience,   &out.player_experience_isDouble);
    }

    // ---- Validation ----
    out.resolved = (out.g_game_ptr_addr != 0 && out.game_localPlayer != 0);

    Log("\n=== Resolution %s ===", out.resolved ? "SUCCEEDED" : "PARTIAL");
    Log("  g_game ptr addr:     0x%08X", (uint32_t)out.g_game_ptr_addr);
    Log("  Game::localPlayer:   0x%X", out.game_localPlayer);
    Log("  Game::protoVer:      0x%X", out.game_protocolVersion);
    Log("  Game::chaseMode:     0x%X", out.game_chaseMode);
    Log("  Creature::health:    0x%X", out.creature_health);
    Log("  Creature::maxHealth: 0x%X", out.creature_maxHealth);
    Log("  Creature::position:  0x%X", out.creature_position);
    Log("  Creature::id:        0x%X", out.creature_id);
    Log("  Player::mana:        0x%X", out.player_mana);
    Log("  Player::level:       0x%X", out.player_level);

    return out.resolved;
}

// ============================================================================
// Convenience: global init function
// ============================================================================

bool InitializeOffsets(GameOffsets& offsets) {
    OffsetResolver resolver;
    bool ok = resolver.Resolve(offsets);

    // Write the full log to a file for debugging
    const std::string& log = resolver.GetLog();
    FILE* f = fopen("easybot_offsets.log", "w");
    if (f) {
        fwrite(log.c_str(), 1, log.size(), f);
        fclose(f);
    }

    return ok;
}

// ============================================================================
// Read helpers
// ============================================================================

// Isolated helper: reads a double (8 bytes) from addr and returns it as uint32_t.
// No C++ objects on stack → MSVC allows __try/__except (no C2712).
static uint32_t TryReadDoubleAsUint32(uintptr_t addr) {
    __try {
        double val;
        const uint8_t* src = reinterpret_cast<const uint8_t*>(addr);
        uint8_t* dst = reinterpret_cast<uint8_t*>(&val);
        for (int i = 0; i < 8; i++) dst[i] = src[i];
        if (val < 0.0 || val > 4294967295.0) return 0;
        return static_cast<uint32_t>(val);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

uintptr_t GetGameSingleton(const GameOffsets& o) {
    if (!o.g_game_ptr_addr) return 0;
    __try {
        return *reinterpret_cast<uintptr_t*>(o.g_game_ptr_addr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

uintptr_t GetLocalPlayer(const GameOffsets& o) {
    uintptr_t game = GetGameSingleton(o);
    if (!game || !o.game_localPlayer) return 0;
    __try {
        return *reinterpret_cast<uintptr_t*>(game + o.game_localPlayer);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

uint32_t GetPlayerHealth(const GameOffsets& o) {
    uintptr_t lp = GetLocalPlayer(o);
    if (!lp || !o.creature_health) return 0;
    if (o.creature_health_isDouble) return TryReadDoubleAsUint32(lp + o.creature_health);
    __try {
        return *reinterpret_cast<uint32_t*>(lp + o.creature_health);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

uint32_t GetPlayerMaxHealth(const GameOffsets& o) {
    uintptr_t lp = GetLocalPlayer(o);
    if (!lp || !o.creature_maxHealth) return 0;
    if (o.creature_maxHealth_isDouble) return TryReadDoubleAsUint32(lp + o.creature_maxHealth);
    __try {
        return *reinterpret_cast<uint32_t*>(lp + o.creature_maxHealth);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

uint32_t GetPlayerMana(const GameOffsets& o) {
    uintptr_t lp = GetLocalPlayer(o);
    if (!lp || !o.player_mana) return 0;
    if (o.player_mana_isDouble) return TryReadDoubleAsUint32(lp + o.player_mana);
    __try {
        return *reinterpret_cast<uint32_t*>(lp + o.player_mana);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

uint32_t GetPlayerLevel(const GameOffsets& o) {
    uintptr_t lp = GetLocalPlayer(o);
    if (!lp || !o.player_level) return 0;
    if (o.player_level_isDouble) return TryReadDoubleAsUint32(lp + o.player_level);
    __try {
        return *reinterpret_cast<uint32_t*>(lp + o.player_level);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

// Isolated helper: no C++ objects, so MSVC allows __try/__except (C2712).
// OTClient uses std::string which in MSVC x86 is:
//   offset+0x00: union { char buf[16]; char* ptr; }  (SSO buffer or heap ptr)
//   offset+0x10: size_t length
//   offset+0x14: size_t capacity
// If capacity >= 16, the string is heap-allocated and buf[0..3] is a pointer.
// If capacity < 16, the string is stored inline in buf[].
static bool TryReadPlayerNameRaw(uintptr_t strObj, char* outBuf, size_t bufSize, size_t* outLen) {
    __try {
        size_t length   = *reinterpret_cast<size_t*>(strObj + 0x10);
        size_t capacity = *reinterpret_cast<size_t*>(strObj + 0x14);

        if (length == 0 || length > 256) return false;

        const char* data;
        if (capacity >= 16) {
            data = *reinterpret_cast<const char**>(strObj);
        } else {
            data = reinterpret_cast<const char*>(strObj);
        }

        size_t toCopy = (length < bufSize - 1) ? length : bufSize - 1;
        memcpy(outBuf, data, toCopy);
        outBuf[toCopy] = '\0';
        *outLen = toCopy;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

std::string GetPlayerName(const GameOffsets& o) {
    uintptr_t lp = GetLocalPlayer(o);
    if (!lp || !o.creature_name) return "";
    char buf[257];
    size_t len = 0;
    if (!TryReadPlayerNameRaw(lp + o.creature_name, buf, sizeof(buf), &len)) return "";
    return std::string(buf, len);
}

Position GetPlayerPosition(const GameOffsets& o) {
    Position pos = { 0, 0, 0 };
    uintptr_t lp = GetLocalPlayer(o);
    if (!lp || !o.creature_position) return pos;
    __try {
        // OTClient Position struct: { uint16_t x; uint16_t y; uint8_t z; }
        // But some builds use int32 for x,y. We'll try uint16 first (standard OTClient).
        pos.x = *reinterpret_cast<uint16_t*>(lp + o.creature_position);
        pos.y = *reinterpret_cast<uint16_t*>(lp + o.creature_position + 2);
        pos.z = *reinterpret_cast<uint8_t*>(lp + o.creature_position + 4);

        // Sanity check – Tibia coords are roughly x:0-65535, y:0-65535, z:0-15
        if (pos.z > 15) {
            // Might be int32 layout, try that
            uint32_t x32 = *reinterpret_cast<uint32_t*>(lp + o.creature_position);
            uint32_t y32 = *reinterpret_cast<uint32_t*>(lp + o.creature_position + 4);
            uint8_t  z8  = *reinterpret_cast<uint8_t*>(lp + o.creature_position + 8);
            if (z8 <= 15 && x32 < 65536 && y32 < 65536) {
                pos.x = (uint16_t)x32;
                pos.y = (uint16_t)y32;
                pos.z = z8;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return pos;
}
