#include "hooks.h"
#include "BuildConfig.h"
#include "Creature.h"
#include "CustomFunctions.h"
#include "pattern_scan.h"

// ============================================================================
// SEH-protected pointer read — prevents a bad EBP offset from crashing the
// client.  Returns 0 on access fault.
// No C++ objects — __try is valid.
// ============================================================================
static uintptr_t tryReadPtr(uintptr_t addr) {
    __try {
        return *reinterpret_cast<const uintptr_t*>(addr);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}

// ============================================================================
// hooked_bindSingletonFunction
// ============================================================================
void __stdcall hooked_bindSingletonFunction(uintptr_t a1, uintptr_t a2, uintptr_t a3) {
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    uintptr_t ebp = ctx.Ebp;

    auto global = *reinterpret_cast<std::string*>(a1);
    auto field  = *reinterpret_cast<std::string*>(a2);

    uintptr_t tmp, second_tmp;
    if (global[1] != '_') {
        tmp = tryReadPtr(ebp + classFunctionOffset);
        if (tmp) ClassMemberFunctions[global + "." + field] = tmp;
    } else {
        tmp        = tryReadPtr(ebp + singletonFunctionOffset);
        second_tmp = tryReadPtr(ebp + singletonFunctionOffset + 0x04);
        if (tmp) SingletonFunctions[global + "." + field] = {tmp, second_tmp};
    }

    original_bindSingletonFunction(a1, a2, a3);
}

// cdecl variant — used when the target has no EBP frame (first byte != 0x55).
// Same capture logic as the stdcall version; caller cleans the stack.
void __cdecl hooked_bindSingletonFunction_cdecl(uintptr_t a1, uintptr_t a2, uintptr_t a3) {
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    uintptr_t ebp = ctx.Ebp;

    auto global = *reinterpret_cast<std::string*>(a1);
    auto field  = *reinterpret_cast<std::string*>(a2);

    uintptr_t tmp, second_tmp;
    if (global[1] != '_') {
        tmp = tryReadPtr(ebp + classFunctionOffset);
        if (tmp) ClassMemberFunctions[global + "." + field] = tmp;
    } else {
        tmp        = tryReadPtr(ebp + singletonFunctionOffset);
        second_tmp = tryReadPtr(ebp + singletonFunctionOffset + 0x04);
        if (tmp) SingletonFunctions[global + "." + field] = {tmp, second_tmp};
    }

    typedef void(__cdecl* orig_t)(uintptr_t, uintptr_t, uintptr_t);
    reinterpret_cast<orig_t>(original_bindSingletonFunction)(a1, a2, a3);
}

void __stdcall hooked_callLuaField(uintptr_t* a1) {
    original_callLuaField(a1);
}

// ============================================================================
// hooked_callGlobalField
// ============================================================================

inline uint32_t itemId;

#pragma optimize( "", off )
void __stdcall hooked_callGlobalField(uintptr_t **a1, uintptr_t **a2) {
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    uintptr_t ebp = ctx.Ebp;
    auto global = *reinterpret_cast<std::string*>(a1);
    auto field = *reinterpret_cast<std::string*>(a2);
    if (global == "g_game") {
        if (field == "onTextMessage") {
            uintptr_t onTextMessage_address = ebp + onTextMessageOffset;
            auto ptr_messageText = g_custom->getMessagePtr(onTextMessage_address);
            auto message_address = reinterpret_cast<std::string*>(ptr_messageText);
            if (message_address->find("You see") != std::string::npos)
            {
                *message_address = "ID: " + std::to_string(itemId) + "\n" + *reinterpret_cast<std::string*>(ptr_messageText);
            }
        } else if (field == "onTalk") {
            auto args = reinterpret_cast<StackArgs*>(ebp + onTalkOffset);
            g_custom->onTalk(
                *args->name,
                reinterpret_cast<uint16_t>(args->level),
                *args->mode,
                *args->text,
                *args->channelId,
                *args->pos
            );
        }
    }
    original_callGlobalField(a1, a2);
}
#pragma optimize( "", on )

// cdecl variant — same logic, caller cleans the stack.
#pragma optimize( "", off )
void __cdecl hooked_callGlobalField_cdecl(uintptr_t **a1, uintptr_t **a2) {
    CONTEXT ctx;
    RtlCaptureContext(&ctx);
    uintptr_t ebp = ctx.Ebp;
    auto global = *reinterpret_cast<std::string*>(a1);
    auto field  = *reinterpret_cast<std::string*>(a2);
    if (global == "g_game") {
        if (field == "onTextMessage") {
            uintptr_t onTextMessage_address = ebp + onTextMessageOffset;
            auto ptr_messageText = g_custom->getMessagePtr(onTextMessage_address);
            auto message_address = reinterpret_cast<std::string*>(ptr_messageText);
            if (message_address->find("You see") != std::string::npos)
                *message_address = "ID: " + std::to_string(itemId) + "\n" + *reinterpret_cast<std::string*>(ptr_messageText);
        } else if (field == "onTalk") {
            auto args = reinterpret_cast<StackArgs*>(ebp + onTalkOffset);
            g_custom->onTalk(
                *args->name,
                reinterpret_cast<uint16_t>(args->level),
                *args->mode,
                *args->text,
                *args->channelId,
                *args->pos
            );
        }
    }
    typedef void(__cdecl* orig_t)(uintptr_t**, uintptr_t**);
    reinterpret_cast<orig_t>(original_callGlobalField)(a1, a2);
}
#pragma optimize( "", on )

// ============================================================================
// hooked_MainLoop
// ============================================================================
int hooked_MainLoop(int a1) {
    g_dispatcher->executeEvent();
    return original_mainLoop(a1);
}

// ============================================================================
// hooked_Look
// ============================================================================
typedef uint32_t(gameCall* GetId)(uintptr_t RCX, void* RDX);

void __stdcall hooked_Look(const uintptr_t& thing, const bool isBattleList) {
    if (look_original) look_original(&thing, isBattleList);
    auto function = reinterpret_cast<GetId>(ClassMemberFunctions["Item.getId"]);
    if (!function) return;
    void* pMysteryPtr = nullptr;
    itemId = function(thing, &pMysteryPtr);
}
