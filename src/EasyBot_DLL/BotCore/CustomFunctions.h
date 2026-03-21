#ifndef CUSTOM_FUNCTIONS_H
#define CUSTOM_FUNCTIONS_H
#define g_custom CustomFunctions::getInstance()
#include "const.h"
#include "EventDispatcher.h"

struct StackArgs {
    std::string* name;
    uintptr_t* level;
    Otc::MessageMode* mode;
    std::string* text;
    uint16_t* channelId;
    Position* pos;
};

struct MessageStruct {
    std::string name;
    uint16_t level;
    Otc::MessageMode mode;
    std::string text;
    uint16_t channelId;
    Position pos;
};

struct ChannelStruct
{
    uint16_t channelId;
    std::string channelName;
};

class CustomFunctions{
    static CustomFunctions* instance;
    static std::mutex mutex;
    std::vector<MessageStruct> messages;
    std::vector<ChannelStruct> channels;

    const size_t MAX_MESSAGES = 100;
protected:
    CustomFunctions()=default;
    ~CustomFunctions()= default;
public:
    CustomFunctions(CustomFunctions const&) = delete;
    void operator=(const CustomFunctions&) = delete;
    static CustomFunctions* getInstance();


    void onTalk(std::string name, uint16_t level, Otc::MessageMode mode, std::string text, uint16_t channelId, const Position& pos);

    std::vector<MessageStruct> getMessages(int messageNumber);
    void clearMessages();
    void dropMessages(int messageNumber);
    uintptr_t* getMessagePtr(uintptr_t message_address);
};






#endif //CUSTOM_FUNCTIONS_H
