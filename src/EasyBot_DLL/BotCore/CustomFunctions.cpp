#include "CustomFunctions.h"


CustomFunctions* CustomFunctions::instance{nullptr};
std::mutex CustomFunctions::mutex;

CustomFunctions* CustomFunctions::getInstance() {
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr) {
        instance = new CustomFunctions();
    }
    return instance;
}

void CustomFunctions::onTalk(std::string name, uint16_t level, Otc::MessageMode mode, std::string text,
    uint16_t channelId, const Position &pos) {
    if (messages.size() >= MAX_MESSAGES) {
        messages.erase(messages.begin());
    }
    MessageStruct record = {std::move(name), level, mode, std::move(text), channelId, pos};
    messages.push_back(std::move(record));
}


std::vector<MessageStruct> CustomFunctions::getMessages(int messageNumber) {
    auto count = static_cast<size_t>(messageNumber);
    size_t actual_size = messages.size();
    size_t start_index;
    if (count >= actual_size) {
        start_index = 0;
    } else {
        start_index = actual_size - count;
    }
    return std::vector<MessageStruct>(
        messages.begin() + start_index,
        messages.end()
    );
}

void CustomFunctions::clearMessages() {
    messages.clear();
}

void CustomFunctions::dropMessages(int messageNumber) {
    auto count = static_cast<size_t>(messageNumber);
    size_t current_size = messages.size();
    if (count >= current_size) {
        messages.clear();
    } else {
        messages.resize(current_size - count);
    }
}


uintptr_t* CustomFunctions::getMessagePtr(uintptr_t message_address) {
    return *reinterpret_cast<uintptr_t**>(message_address);
}





