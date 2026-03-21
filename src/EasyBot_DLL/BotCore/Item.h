#ifndef ITEM_H
#define ITEM_H
#define g_item Item::getInstance()
#include "declarations.h"
#include "const.h"
#include "EventDispatcher.h"
#include "hooks.h"


class Item {
    static Item* instance;
    static std::mutex mutex;
protected:
    Item()=default;
    ~Item()= default;
public:
    Item(Item const&) = delete;
    void operator=(const Item&) = delete;
    static Item* getInstance();

    int getCount(ItemPtr item);
    int getSubType(ItemPtr item);
    uint32_t getId(ItemPtr item);
    std::string getTooltip(ItemPtr item);
    uint32_t getDurationTime(ItemPtr item);
    std::string getName(ItemPtr item);
    std::string getDescription(ItemPtr item);
    uint8_t getTier(ItemPtr item);
    std::string getText(ItemPtr item);


};



#endif //ITEM_H
