#ifndef CONTAINER_H
#define CONTAINER_H
#define g_container Container::getInstance()
#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"

class Container {
    static Container* instance;
    static std::mutex mutex;
protected:
    Container()=default;
    ~Container()= default;
public:
    Container(Container const&) = delete;
    void operator=(const Container&) = delete;
    static Container* getInstance();

    ItemPtr getItem(ContainerPtr container, int slot);
    std::deque<ItemPtr> getItems(ContainerPtr container);
    int getItemsCount(ContainerPtr container);
    Position getSlotPosition(ContainerPtr container, int slot);
    std::string getName(ContainerPtr container);
    int getId(ContainerPtr container);
    ItemPtr getContainerItem(ContainerPtr container);
    bool hasParent(ContainerPtr container);
    int getCapacity(ContainerPtr container);
    int getFirstIndex(ContainerPtr container);

};



#endif //CONTAINER_H
