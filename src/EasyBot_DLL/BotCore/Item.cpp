#include "Item.h"


Item* Item::instance{nullptr};
std::mutex Item::mutex;


Item* Item::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new Item();
    }
    return instance;
}

std::string Item::getName(ItemPtr item) {
    if (!item) return "";
    typedef void(gameCall* GetName)(
        uintptr_t RCX,
        std::string *RDX
        );
    auto function = reinterpret_cast<GetName>(ClassMemberFunctions["Item.getName"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        std::string result;
        function(item, &result);
        return result;
    });
}

std::string Item::getDescription(ItemPtr item) {
    if (!item) return "";
    typedef void(gameCall* GetDescription)(
        uintptr_t RCX,
        std::string *RDX
        );
    auto function = reinterpret_cast<GetDescription>(ClassMemberFunctions["Item.getDescription"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        std::string result;
        function(item, &result);
        return result;
    });
}

int Item::getCount(ItemPtr item) {
    if (!item) return 0;
    typedef int(gameCall* GetCount)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetCount>(ClassMemberFunctions["Item.getCount"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        void* pMysteryPtr = nullptr;
        return function(item, &pMysteryPtr);
    });
}

int Item::getSubType(ItemPtr item) {
    if (!item) return 0;
    typedef int(gameCall* GetSubType)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetSubType>(ClassMemberFunctions["Item.getSubType"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        void* pMysteryPtr = nullptr;
        return function(item, &pMysteryPtr);
    });
}

uint32_t Item::getId(ItemPtr item) {
    if (!item) return 0;
    typedef uint32_t(gameCall* GetId)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetId>(ClassMemberFunctions["Item.getId"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        void* pMysteryPtr = nullptr;
        return function(item, &pMysteryPtr);
    });
}

std::string Item::getTooltip(ItemPtr item) {
    if (!item) return "";
    typedef void(gameCall* GetTooltip)(
        uintptr_t RCX,
        std::string *RDX
        );
    auto function = reinterpret_cast<GetTooltip>(ClassMemberFunctions["Item.getTooltip"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        std::string result;
        function(item, &result);
        return result;
    });
}

uint32_t Item::getDurationTime(ItemPtr item) {
    if (!item) return 0;
    typedef uint32_t(gameCall* GetDurationTime)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetDurationTime>(ClassMemberFunctions["Item.getDurationTime"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        void* pMysteryPtr = nullptr;
        return function(item, &pMysteryPtr);
    });
}

uint8_t Item::getTier(ItemPtr item) {
    if (!item) return 0;
    typedef void(gameCall* GetTier)(
        uintptr_t RCX,
        uint8_t *RDX
        );
    auto function = reinterpret_cast<GetTier>(ClassMemberFunctions["Item.getTier"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        uint8_t result;
        function(item, &result);
        return result;
    });
}

std::string Item::getText(ItemPtr item) {
    if (!item) return "";
    typedef void(gameCall* GetText)(
        uintptr_t RCX,
        std::string *RDX
        );
    auto function = reinterpret_cast<GetText>(ClassMemberFunctions["Item.getText"]);
    return g_dispatcher->scheduleEventEx([function, item]() {
        std::string result;
        function(item, &result);
        return result;
    });
}
