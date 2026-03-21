#ifndef BUILDCONFIG_H
#define BUILDCONFIG_H
#include <Windows.h>

#ifdef _WIN64
    #define gameCall __fastcall
#else
    #define gameCall __thiscall
#endif

// Light hack: offset within the LocalPlayer struct to the light level field.
static const uint32_t lightHackOffset         = 0xAC;

// EBP frame offsets used inside the hook functions to read the correct values
// from the caller's stack frame via RtlCaptureContext.
static const uint32_t classFunctionOffset     = 0xC;   // offset to func ptr for class member bindings
static const uint32_t singletonFunctionOffset  = 0x10;  // offset to func ptr for g_game singleton bindings
static const uint32_t onTextMessageOffset      = 0x14;  // offset to onTextMessage args in callGlobalField frame
static const uint32_t onTalkOffset             = 0x10;  // offset to onTalk StackArgs in callGlobalField frame

// Patterns (identical across all supported servers)
static const BYTE* bindSingletonFunction_x86_PATTERN = reinterpret_cast<const BYTE*>("\x55\x8b\xec\x8b\x45\x00\x83\x78\x00\x00\x00\x00\x8b\x00\x50\x68\x00\x00\x00\x00\xff\x35\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x83\xc4");
static LPCSTR bindSingletonFunction_x86_MASK = "xxxxx?xx????xxxx????xx????x????xx";

static const BYTE* mainLoop_x86_PATTERN = reinterpret_cast<const BYTE*>("\x8b\x54\x24\x00\x8b\x00\x24\x00\x53\x56");
static LPCSTR mainLoop_x86_MASK = "xxx?x?x?xx";

static const BYTE* callGlobalField_PATTERN = reinterpret_cast<const BYTE*>("\x55\x8b\xec\x8b\x45\x00\x83\x78\x00\x00\x00\x00\x8b\x00\x50\x68\x00\x00\x00\x00\xff\x35\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x6a");
static LPCSTR callGlobalField_MASK = "xxxxx?xx????xxxx????xx????x????x";

#endif //BUILDCONFIG_H
