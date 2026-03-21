#include "Container.h"



Container* Container::instance{nullptr};
std::mutex Container::mutex;


Container* Container::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new Container();
    }

    return instance;
}

ItemPtr Container::getItem(ContainerPtr container, int slot) {
    if (!container) return 0;
    typedef void(gameCall* GetItem)(
        uintptr_t RCX,
        ItemPtr *RDX,
        int slot
        );
    auto function = reinterpret_cast<GetItem>(ClassMemberFunctions["Container.getItem"]);
    return g_dispatcher->scheduleEventEx([function, container, slot]() {
        ItemPtr result;
        function(container, &result, slot);
        return result;
    });
}


std::deque<ItemPtr> Container::getItems(ContainerPtr container) {
    if (!container) return {};
    typedef void(gameCall* GetItems)(
        uintptr_t RCX,
        std::deque<ItemPtr> *RDX
        );
    auto function = reinterpret_cast<GetItems>(ClassMemberFunctions["Container.getItems"]);
    return g_dispatcher->scheduleEventEx([function, container]() {
        std::deque<ItemPtr> result;
        function(container, &result);
        return result;
    });
}

int Container::getItemsCount(ContainerPtr container) {
    if (!container) return 0;
    typedef int(gameCall* GetItemsCount)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetItemsCount>(ClassMemberFunctions["Container.getItemsCount"]);
    return g_dispatcher->scheduleEventEx([function, container]() {
        void* pMysteryPtr = nullptr;
        return function(container, &pMysteryPtr);
    });
}

Position Container::getSlotPosition(ContainerPtr container, int slot) {
    if (!container) return {};
    typedef void(gameCall* GetSlotPosition)(
        uintptr_t RCX,
        Position *RDX,
        int slot
        );
    auto function = reinterpret_cast<GetSlotPosition>(ClassMemberFunctions["Container.getSlotPosition"]);
    return g_dispatcher->scheduleEventEx([function, container, slot]() {
        Position result;
        function(container, &result, slot);
        return result;
    });
}

std::string Container::getName(ContainerPtr container) {
    if (!container) return "";
    typedef void(gameCall* GetName)(
        uintptr_t RCX,
        std::string *RDX
        );
    auto function = reinterpret_cast<GetName>(ClassMemberFunctions["Container.getName"]);
    return g_dispatcher->scheduleEventEx([function, container]() {
        std::string result;
        function(container, &result);
        return result;
    });
}

int Container::getId(ContainerPtr container) {
    if (!container) return 0;
    typedef int(gameCall* GetId)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetId>(ClassMemberFunctions["Container.getId"]);
    return g_dispatcher->scheduleEventEx([function, container]() {
        void* pMysteryPtr = nullptr;
        return function(container, &pMysteryPtr);
    });
}

ItemPtr Container::getContainerItem(ContainerPtr container) {
    if (!container) return 0;
    typedef void(gameCall* GetContainerItem)(
        uintptr_t RCX,
        ItemPtr *RDX
        );
    auto function = reinterpret_cast<GetContainerItem>(ClassMemberFunctions["Container.getContainerItem"]);
    return g_dispatcher->scheduleEventEx([function, container]() {
        ItemPtr result;
        function(container, &result);
        return result;
    });
}

bool Container::hasParent(ContainerPtr container) {
    if (!container) return 0;
    typedef bool(gameCall* HasParent)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<HasParent>(ClassMemberFunctions["Container.hasParent"]);
    return g_dispatcher->scheduleEventEx([function, container]() {
        void* pMysteryPtr = nullptr;
        return function(container, &pMysteryPtr);
    });
}

int Container::getCapacity(ContainerPtr container) {
    if (!container) return 0;
    typedef int(gameCall* GetCapacity)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetCapacity>(ClassMemberFunctions["Container.getCapacity"]);
    return g_dispatcher->scheduleEventEx([function, container]() {
        void* pMysteryPtr = nullptr;
        return function(container, &pMysteryPtr);
    });
}

int Container::getFirstIndex(ContainerPtr container) {
    if (!container) return 0;
    typedef int(gameCall* GetFirstIndex)(
        uintptr_t RCX,
        void *RDX
        );
    auto function = reinterpret_cast<GetFirstIndex>(ClassMemberFunctions["Container.getFirstIndex"]);
    return g_dispatcher->scheduleEventEx([function, container]() {
            void* pMysteryPtr = nullptr;
            return function(container, &pMysteryPtr);
    });
}

