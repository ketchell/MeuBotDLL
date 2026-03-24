#pragma once
#include <cstdint>

#define SERVER_PROFILE_MAGIC 0xEB07504Fu  // 'EB07' + 'PF'

// Bump this when the layout changes — old profiles will be rejected.
#define SERVER_PROFILE_VERSION 1

struct ServerProfile {
    // ---- Header ----
    uint32_t magic;          // SERVER_PROFILE_MAGIC
    uint32_t version;        // SERVER_PROFILE_VERSION

    // ---- Identity ----
    char     serverName[64];
    uint32_t moduleBase;
    uint32_t moduleSize;
    uint32_t exeChecksum;    // first-4096-byte sum — detect exe updates

    // ---- Hook targets ----
    uint32_t bindSingletonFunc;
    uint32_t callGlobalFieldFunc;
    uint32_t mainLoopFunc;

    // ---- Calling convention ----
    // 0 = stdcall (push EBP / 0x55 prologue, e.g. Realera)
    // 1 = cdecl   (no EBP frame, e.g. Nostalrius)
    uint32_t callingConvention;

    // ---- Server type ----
    uint32_t isLuaWrapperServer; // 1 if class-member funcs are Lua dispatchers
    uint32_t singletonsUnique;   // 1 if g_game.* funcs have distinct addresses
    uint32_t useFunctionCalls;   // 1 if direct class-member calls work

    // ---- Memory offsets (all relative to LocalPlayer/Creature pointer) ----
    uint32_t healthOffset;
    uint32_t maxHealthOffset;
    uint32_t manaOffset;
    uint32_t maxManaOffset;
    uint32_t levelOffset;
    uint32_t soulOffset;
    uint32_t staminaOffset;
    uint32_t positionOffset;     // thing_positionOffset inside Thing/Creature
    uint32_t statesOffset;
    uint32_t experienceOffset;
    uint32_t magicLevelOffset;
    uint32_t freeCapacityOffset;
    uint32_t speedOffset;
    uint32_t directionOffset;
    uint32_t skullOffset;
    uint32_t shieldOffset;
    uint32_t lightHackOffset;

    // ---- Padding / reserved ----
    uint32_t reserved[16];
};

// Returns the directory where profiles are stored: C:\EasyBot\profiles\
// Creates the directory if it does not exist.
void GetProfileDir(char* out, int outSize);

// Load a profile by server name.  Returns false if file missing or magic wrong.
bool LoadServerProfile(const char* serverName, ServerProfile* out);

// Save a profile.  Overwrites any existing file for that server name.
bool SaveServerProfile(const ServerProfile* profile);

// Fill names[0..maxCount) with all saved server names found in the profile dir.
// Sets *count to the number found.
void ListServerProfiles(char names[][64], int* count, int maxCount);
