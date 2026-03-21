#include "hooks.h"
#include "BuildConfig.h"
#include "Creature.h"
#include "CustomFunctions.h"
#include "pattern_scan.h"

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
        tmp = *reinterpret_cast<uintptr_t*>(ebp + classFunctionOffset);
        ClassMemberFunctions[global + "." + field] = tmp;
    } else {
        tmp        = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset);
        second_tmp = *reinterpret_cast<uintptr_t*>(ebp + singletonFunctionOffset + 0x04);
        SingletonFunctions[global + "." + field] = {tmp, second_tmp};
    }

    original_bindSingletonFunction(a1, a2, a3);
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
    look_original(&thing, isBattleList);
    auto function = reinterpret_cast<GetId>(ClassMemberFunctions["Item.getId"]);
    void* pMysteryPtr = nullptr;
    itemId = function(thing, &pMysteryPtr);
}
