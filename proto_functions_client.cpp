#include "proto_functions_client.h"
#include <string>

BotClient* BotClient::instance{nullptr};
std::mutex BotClient::mutex;


BotClient::BotClient() {
    connect(50051);
}

void BotClient::connect(int port) {
    std::shared_ptr<Channel> channel = grpc::CreateChannel("localhost:" + std::to_string(port), grpc::InsecureChannelCredentials());
    stub = BotService::NewStub(channel);
}

void BotClient::disconnect(int port) {
}

BotClient* BotClient::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (instance == nullptr)
    {
        instance = new BotClient();
    }
    return instance;
}

// Functions:
// Container
uintptr_t BotClient::getItem(uintptr_t container, uint8_t slot)
{
    bot_GetItemRequest request;
    request.set_container(container);
    request.set_slot(slot);
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetItem(&context, request, &response);
    return response.value();
}

std::deque<uintptr_t> BotClient::getItems(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    bot_Uint64List response;
    ClientContext context;
    Status status = stub->GetItems(&context, request, &response);
    std::deque<uintptr_t> items;
    if (status.ok()) {
        for (const auto& item : response.items()) {
            items.push_back(item);
        }
    }
    return items;
}

int32_t BotClient::getItemsCount(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    Int32Value response;
    ClientContext context;
    Status status = stub->GetItemsCount(&context, request, &response);
    return response.value();
}

Position BotClient::getSlotPosition(uintptr_t container, int slot)
{
    bot_GetSlotPositionRequest request;
    request.set_container(container);
    request.set_slot(slot);
    bot_Position response;
    ClientContext context;
    Status status = stub->GetSlotPosition(&context, request, &response);
    if (status.ok()) {
        Position position{};
        position.x = response.x();
        position.y = response.y();
        position.z = response.z();
        return position;
    }
    return {};
}

std::string BotClient::getContainerName(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::StringValue response;
    ClientContext context;
    Status status = stub->GetContainerName(&context, request, &response);
    return response.value();
}

int32_t BotClient::getContainerId(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    Int32Value response;
    ClientContext context;
    Status status = stub->GetContainerId(&context, request, &response);
    return response.value();
}

uintptr_t BotClient::getContainerItem(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetContainerItem(&context, request, &response);
    return response.value();
}

bool BotClient::hasParent(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->HasParent(&context, request, &response);
    return response.value();
}

int32_t BotClient::getCapacity(uintptr_t container) {
    UInt64Value request;
    request.set_value(container);
    Int32Value response;
    ClientContext context;
    Status status = stub->GetCapacity(&context, request, &response);
    return response.value();
}

int32_t BotClient::getFirstIndex(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    Int32Value response;
    ClientContext context;
    Status status = stub->GetFirstIndex(&context, request, &response);
    return response.value();
}

// Creature
std::string BotClient::getCreatureName(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::StringValue response;
    ClientContext context;
    Status status = stub->GetCreatureName(&context, request, &response);
    return response.value();
}

uint8_t BotClient::getHealthPercent(uintptr_t creature)
{
    UInt64Value request;
    request.set_value(creature);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetHealthPercent(&context, request, &response);
    return response.value();
}

uint8_t BotClient::getManaPercent(uintptr_t creature) {
    UInt64Value request;
    request.set_value(creature);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetManaPercent(&context, request, &response);
    return response.value();
}

uint8_t BotClient::getSkull(uintptr_t creature) {
    UInt64Value request;
    request.set_value(creature);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetSkull(&context, request, &response);
    return response.value();
}

Otc::Direction BotClient::getDirection(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    Int32Value response;
    ClientContext context;
    Status status = stub->GetDirection(&context, request, &response);
    return static_cast<Otc::Direction>(response.value());
}

bool BotClient::isWalking(uintptr_t creature) {
    UInt64Value request;
    request.set_value(creature);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsWalking(&context, request, &response);
    return response.value();
}

bool BotClient::isDead(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsDead(&context, request, &response);
    return response.value();
}

bool BotClient::canBeSeen(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->CanBeSeen(&context, request, &response);
    return response.value();
}

bool BotClient::isCovered(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsCovered(&context, request, &response);
    return response.value();
}

bool BotClient::canShoot(uintptr_t value, int distance)
{
    bot_CanShootRequest request;
    request.set_creature(value);
    request.set_distance(distance);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->CanShoot(&context, request, &response);
    return response.value();
}

// Game

void BotClient::safeLogout() {
    Empty request;
    Empty response;
    ClientContext context;
    Status status = stub->SafeLogout(&context, request, &response);
}

void BotClient::walk(Otc::Direction direction)
{
    Int32Value request;
    request.set_value(direction);
    Empty response;
    ClientContext context;
    Status status = stub->Walk(&context, request, &response);
}

void BotClient::autoWalkGame(const std::vector<Otc::Direction> &dirs, const Position &startPos)
{
    bot_AutoWalkGameRequest request;
    for (auto dir : dirs) {
        request.mutable_direction()->add_dirs(static_cast<bot_Direction>(dir));
    }
    request.mutable_startpos()->set_x(startPos.x);
    request.mutable_startpos()->set_y(startPos.y);
    request.mutable_startpos()->set_z(startPos.z);
    Empty response;
    ClientContext context;
    Status status = stub->AutoWalkGame(&context, request, &response);
}

void BotClient::turn(Otc::Direction direction) {
    Int32Value request;
    request.set_value(direction);
    Empty response;
    ClientContext context;
    Status status = stub->Turn(&context, request, &response);
}

void BotClient::stop() {
    Empty request;
    Empty response;
    ClientContext context;
    Status status = stub->Stop(&context, request, &response);
}

void BotClient::move(const uintptr_t &thing, const Position& toPos, int count)
{
    bot_MoveRequest request;
    request.set_thing(thing);
    request.set_count(count);
    request.mutable_topos()->set_x(toPos.x);
    request.mutable_topos()->set_y(toPos.y);
    request.mutable_topos()->set_z(toPos.z);
    Empty response;
    ClientContext context;
    Status status = stub->Move(&context, request, &response);
}

void BotClient::moveToParentContainer(const uintptr_t& thing, const int count)
{
    bot_MoveToParentContainerRequest request;
    request.set_thing(thing);
    request.set_count(count);
    Empty response;
    ClientContext context;
    Status status = stub->MoveToParentContainer(&context, request, &response);
}

void BotClient::use(const uintptr_t &thing)
{
    UInt64Value request;
    request.set_value(thing);
    Empty response;
    ClientContext context;
    Status status = stub->Use(&context, request, &response);
}

void BotClient::useWith(const uintptr_t &item, const uintptr_t &toThing)
{
    bot_UseWithRequest request;
    request.set_item(item);
    request.set_tothing(toThing);
    Empty response;
    ClientContext context;
    Status status = stub->UseWith(&context, request, &response);
}

void BotClient::useInventoryItem(const uint16_t itemId)
{
    google::protobuf::UInt32Value request;
    request.set_value(itemId);
    Empty response;
    ClientContext context;
    Status status = stub->UseInventoryItem(&context, request, &response);
}

void BotClient::useInventoryItemWith(const uint16_t itemId, const uintptr_t& toThing)
{
    bot_UseInventoryItemWithRequest request;
    request.set_tothing(toThing);
    request.set_itemid(itemId);
    Empty response;
    ClientContext context;
    Status status = stub->UseInventoryItemWith(&context, request, &response);
}

uintptr_t BotClient::findItemInContainers(uint32_t itemId, int subType, int tier)
{
    bot_FindItemInContainersRequest request;
    request.set_itemid(itemId);
    request.set_subtype(subType);
    request.set_tier(tier);
    UInt64Value response;
    ClientContext context;
    Status status = stub->FindItemInContainers(&context, request, &response);
    return response.value();
}

int32_t BotClient::open(const uintptr_t &item, const uintptr_t &previousContainer)
{
    bot_OpenRequest request;
    request.set_item(item);
    request.set_previouscontainer(previousContainer);
    Int32Value response;
    ClientContext context;
    Status status = stub->Open(&context, request, &response);
    return response.value();
}

void BotClient::openParent(const uintptr_t &container)
{
    UInt64Value request;
    request.set_value(container);
    Empty response;
    ClientContext context;
    Status status = stub->OpenParent(&context, request, &response);
}

void BotClient::close(const uintptr_t &container)
{
    UInt64Value request;
    request.set_value(container);
    Empty response;
    ClientContext context;
    Status status = stub->Close(&context, request, &response);
}

void BotClient::refreshContainer(const uintptr_t &container)
{
    UInt64Value request;
    request.set_value(container);
    Empty response;
    ClientContext context;
    Status status = stub->RefreshContainer(&context, request, &response);
}

void BotClient::attack(const uintptr_t &creature, bool cancel = false) {
    bot_AttackRequest request;
    request.set_creature(creature);
    request.set_cancel(cancel);
    Empty response;
    ClientContext context;
    Status status = stub->Attack(&context, request, &response);
}

void BotClient::cancelAttack()
{
    Empty request;
    Empty response;
    ClientContext context;
    Status status = stub->CancelAttack(&context, request, &response);
}

void BotClient::follow(const uintptr_t &creature)
{
    UInt64Value request;
    request.set_value(creature);
    Empty response;
    ClientContext context;
    Status status = stub->Follow(&context, request, &response);
}

void BotClient::cancelAttackAndFollow()
{
    Empty request;
    Empty response;
    ClientContext context;
    Status status = stub->CancelAttackAndFollow(&context, request, &response);
}

void BotClient::talk(const std::string& message)
{
    google::protobuf::StringValue request;
    request.set_value(message);
    Empty response;
    ClientContext context;
    Status status = stub->Talk(&context, request, &response);
}

void BotClient::talkChannel(const Otc::MessageMode mode, const uint16_t channelId, const std::string& message)
{
    bot_TalkChannelRequest request;
    request.set_mode(mode);
    request.set_channelid(channelId);
    request.set_message(message);
    Empty response;
    ClientContext context;
    Status status = stub->TalkChannel(&context, request, &response);
}

void BotClient::talkPrivate(const Otc::MessageMode msgMode, const std::string& receiver, const std::string& message)
{
    bot_TalkPrivateRequest request;
    request.set_msgmode(msgMode);
    request.set_receiver(receiver);
    request.set_message(message);
    Empty response;
    ClientContext context;
    Status status = stub->TalkPrivate(&context, request, &response);
}

void BotClient::openPrivateChannel(const std::string& receiver)
{
    google::protobuf::StringValue request;
    request.set_value(receiver);
    Empty response;
    ClientContext context;
    Status status = stub->OpenPrivateChannel(&context, request, &response);
}

int BotClient::getChaseMode() {
    Empty request;
    Int32Value response;
    ClientContext context;
    Status status = stub->GetChaseMode(&context, request, &response);
    return response.value();

}

int BotClient::getFightMode() {
    Empty request;
    Int32Value response;
    ClientContext context;
    Status status = stub->GetFightMode(&context, request, &response);
    return response.value();
}

void BotClient::setChaseMode(Otc::ChaseModes mode)
{
    Int32Value request;
    request.set_value(mode);
    Empty response;
    ClientContext context;
    Status status = stub->SetChaseMode(&context, request, &response);
}

void BotClient::setFightMode(Otc::FightModes mode)
{
    Int32Value request;
    request.set_value(mode);
    Empty response;
    ClientContext context;
    Status status = stub->SetFightMode(&context, request, &response);
}

void BotClient::buyItem(const uintptr_t& item, const uint16_t amount, const bool ignoreCapacity = false, const bool buyWithBackpack = false)
{
    bot_BuyItemRequest request;
    request.set_item(item);
    request.set_amount(amount);
    request.set_ignorecapacity(ignoreCapacity);
    request.set_buywithbackpack(buyWithBackpack);
    Empty response;
    ClientContext context;
    Status status = stub->BuyItem(&context, request, &response);
}

void BotClient::sellItem(const uintptr_t& item, const uint16_t amount, const bool ignoreEquipped = true)
{
    bot_SellItemRequest request;
    request.set_item(item);
    request.set_amount(amount);
    request.set_ignoreequipped(ignoreEquipped);
    Empty response;
    ClientContext context;
    Status status = stub->SellItem(&context, request, &response);
}

void BotClient::equipItem(const uintptr_t &item)
{
    UInt64Value request;
    request.set_value(item);
    Empty response;
    ClientContext context;
    Status status = stub->EquipItem(&context, request, &response);
}

void BotClient::equipItemId(uint16_t itemId, int tier)
{
    bot_EquipItemIdRequest request;
    request.set_itemid(itemId);
    request.set_tier(tier);
    Empty response;
    ClientContext context;
    Status status = stub->EquipItemId(&context, request, &response);
}

void BotClient::mount(bool mountStatus)
{
    google::protobuf::BoolValue request;
    request.set_value(mountStatus);
    Empty response;
    ClientContext context;
    Status status = stub->Mount(&context, request, &response);
}

void BotClient::changeMapAwareRange(const uint8_t xrange, const uint8_t yrange)
{
    bot_ChangeMapAwareRangeRequest request;
    request.set_xrange(xrange);
    request.set_yrange(yrange);
    Empty response;
    ClientContext context;
    Status status = stub->ChangeMapAwareRange(&context, request, &response);
}

bool BotClient::canPerformGameAction()
{
    Empty request;
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->CanPerformGameAction(&context, request, &response);
    return response.value();
}

bool BotClient::isOnline()
{
    Empty request;
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsOnline(&context, request, &response);
    return response.value();
}

bool BotClient::isAttacking()
{
    Empty request;
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsAttacking(&context, request, &response);
    return response.value();
}

bool BotClient::isFollowing()
{
    Empty request;
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsFollowing(&context, request, &response);
    return response.value();
}

uintptr_t BotClient::getContainer(int index)
{
    Int32Value request;
    request.set_value(index);
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetContainer(&context, request, &response);
    return response.value();
}

std::vector<uintptr_t> BotClient::getContainers()
{
    Empty request;
    bot_Uint64List response;
    ClientContext context;
    Status status = stub->GetContainers(&context, request, &response);
    std::vector<uintptr_t> containers;
    if (status.ok()) {
        for (const auto& item : response.items()) {
            containers.push_back(item);
        }
        return containers;
    }
    return {};
}

uintptr_t BotClient::getAttackingCreature()
{
    Empty request;
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetAttackingCreature(&context, request, &response);
    return response.value();
}

uintptr_t BotClient::getFollowingCreature()
{
    Empty request;
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetFollowingCreature(&context, request, &response);
    return response.value();
}

uintptr_t BotClient::getLocalPlayer()
{
    Empty request;
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetLocalPlayer(&context, request, &response);
    return response.value();
}

int BotClient::getClientVersion()
{
    Empty request;
    Int32Value response;
    ClientContext context;
    Status status = stub->GetClientVersion(&context, request, &response);
    return response.value();
}

std::string BotClient::getCharacterName()
{
    Empty request;
    google::protobuf::StringValue response;
    ClientContext context;
    Status status = stub->GetCharacterName(&context, request, &response);
    return response.value();
}
// Item
int BotClient::getCount(uintptr_t item)
{
    UInt64Value request;
    request.set_value(item);
    Int32Value response;
    ClientContext context;
    Status status = stub->GetCount(&context, request, &response);
    return response.value();
}

int BotClient::getSubType(uintptr_t item)
{
    UInt64Value request;
    request.set_value(item);
    Int32Value response;
    ClientContext context;
    Status status = stub->GetSubType(&context, request, &response);
    return response.value();
}

uint32_t BotClient::getItemId(uintptr_t item)
{
    UInt64Value request;
    request.set_value(item);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetItemId(&context, request, &response);
    return response.value();
}

std::string BotClient::getTooltip(uintptr_t item)
{
    UInt64Value request;
    request.set_value(item);
    google::protobuf::StringValue response;
    ClientContext context;
    Status status = stub->GetTooltip(&context, request, &response);
    return response.value();
}

uint32_t BotClient::getDurationTime(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetDurationTime(&context, request, &response);
    return response.value();
}

std::string BotClient::getItemName(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::StringValue response;
    ClientContext context;
    Status status = stub->GetItemName(&context, request, &response);
    return response.value();
}

std::string BotClient::getDescription(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::StringValue response;
    ClientContext context;
    Status status = stub->GetDescription(&context, request, &response);
    return response.value();
}

uint8_t BotClient::getTier(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetTier(&context, request, &response);
    return response.value();
}

std::string BotClient::getText(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::StringValue response;
    ClientContext context;
    Status status = stub->GetText(&context, request, &response);
    return response.value();
}

// LocalPlayer
uint32_t BotClient::getStates(uintptr_t value) {
    UInt64Value request;
    request.set_value(value);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetStates(&context, request, &response);
    return response.value();
}

double BotClient::getHealth(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::DoubleValue response;
    ClientContext context;
    Status status = stub->GetHealth(&context, request, &response);
    return response.value();
}

double BotClient::getMaxHealth(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::DoubleValue response;
    ClientContext context;
    Status status = stub->GetMaxHealth(&context, request, &response);
    return response.value();
}

double BotClient::getFreeCapacity(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::DoubleValue response;
    ClientContext context;
    Status status = stub->GetFreeCapacity(&context, request, &response);
    return response.value();
}

uint16_t BotClient::getLevel(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetLevel(&context, request, &response);
    return response.value();
}

void BotClient::setLightHack(uintptr_t localPlayer, uint16_t lightLevel)
{
    bot::bot_SetLightHackRequest request;
    request.set_localplayer(localPlayer);
    request.set_lightlevel(lightLevel);
    google::protobuf::Empty response;
    ClientContext context;
    Status status = stub->SetLightHack(&context, request, &response);
    if (!status.ok()) std::cout << "SetLightHack failed: " << status.error_message() << std::endl;
}

double BotClient::getMana(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::DoubleValue response;
    ClientContext context;
    Status status = stub->GetMana(&context, request, &response);
    return response.value();
}

double BotClient::getMaxMana(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::DoubleValue response;
    ClientContext context;
    Status status = stub->GetMaxMana(&context, request, &response);
    return response.value();
}

uint32_t BotClient::getManaShield(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetManaShield(&context, request, &response);
    return response.value();
}

uint8_t BotClient::getSoul(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetSoul(&context, request, &response);
    return response.value();
}

uint16_t BotClient::getStamina(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetStamina(&context, request, &response);
    return response.value();
}

uintptr_t BotClient::getInventoryItem(uintptr_t localPlayer, Otc::InventorySlot inventorySlot)
{
    bot_GetInventoryItemRequest request;
    request.set_localplayer(localPlayer);
    request.set_inventoryslot(inventorySlot);
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetInventoryItem(&context, request, &response);
    return response.value();
}

bool BotClient::hasEquippedItemId(uintptr_t localPlayer, uint16_t itemId, int tier) {
    bot_HasEquippedItemIdRequest request;
    request.set_localplayer(localPlayer);
    request.set_itemid(itemId);
    request.set_tier(tier);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->HasEquippedItemId(&context, request, &response);
    return response.value();
}

int BotClient::getInventoryCount(uintptr_t localPlayer, uint16_t itemId, int tier)
{
    bot_GetInventoryCountRequest request;
    request.set_localplayer(localPlayer);
    request.set_itemid(itemId);
    request.set_tier(tier);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetInventoryCount(&context, request, &response);
    return response.value();
}

bool BotClient::hasSight(uintptr_t localPlayer, const Position &pos)
{
    bot_HasSightRequest request;
    request.set_localplayer(localPlayer);
    request.mutable_pos()->set_x(pos.x);
    request.mutable_pos()->set_y(pos.y);
    request.mutable_pos()->set_z(pos.z);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->HasSight(&context, request, &response);
    return response.value();
}

bool BotClient::isAutoWalking(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsAutoWalking(&context, request, &response);
    return response.value();
}

void BotClient::stopAutoWalk(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    Empty response;
    ClientContext context;
    Status status = stub->StopAutoWalk(&context, request, &response);
}

bool BotClient::autoWalk(uintptr_t localPlayer, const Position& destination, bool retry)
{
    bot_AutoWalkRequest request;
    request.set_localplayer(localPlayer);
    request.set_retry(retry);
    request.mutable_destination()->set_x(destination.x);
    request.mutable_destination()->set_y(destination.y);
    request.mutable_destination()->set_z(destination.z);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->AutoWalk(&context, request, &response);
    return response.value();
}

// Map
uintptr_t BotClient::getTile(Position tilePos)
{
    bot_Position request;
    request.set_x(tilePos.x);
    request.set_y(tilePos.y);
    request.set_z(tilePos.z);
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetTile(&context, request, &response);
    return response.value();
}

std::vector<uintptr_t> BotClient::getSpectators(const Position &centerPos, bool multiFloor) {
    bot_GetSpectatorsRequest request;
    request.set_multifloor(multiFloor);
    request.mutable_centerpos()->set_x(centerPos.x);
    request.mutable_centerpos()->set_y(centerPos.y);
    request.mutable_centerpos()->set_z(centerPos.z);
    bot_Uint64List response;
    ClientContext context;
    Status status = stub->GetSpectators(&context, request, &response);
    std::vector<uintptr_t> spectators;
    if (status.ok()) {
        spectators.reserve(response.items_size());
        for (const auto& item : response.items()) {
            spectators.push_back(item);
        }
        return spectators;
    }
    return {};
}

std::vector<Otc::Direction> BotClient::findPath(const Position& startPos, const Position& goalPos, int maxComplexity, int flags)
{
    bot_FindPathRequest request;
    request.set_flags(flags);
    request.set_maxcomplexity(maxComplexity);
    request.mutable_startpos()->set_x(startPos.x);
    request.mutable_startpos()->set_y(startPos.y);
    request.mutable_startpos()->set_z(startPos.z);
    request.mutable_goalpos()->set_x(goalPos.x);
    request.mutable_goalpos()->set_y(goalPos.y);
    request.mutable_goalpos()->set_z(goalPos.z);
    bot_DirectionList response;
    ClientContext context;
    Status status = stub->FindPath(&context, request, &response);
    std::vector<Otc::Direction> dirs;
    if (status.ok()) {
        dirs.reserve(response.dirs_size());
        for (auto item : response.dirs()) {
            dirs.push_back(static_cast<Otc::Direction>(item));
        }
        return dirs;
    }
    return {};
}

bool BotClient::isSightClear(const Position& fromPos, const Position& toPos)
{
    bot_IsSightClearRequest request;
    request.mutable_frompos()->set_x(fromPos.x);
    request.mutable_frompos()->set_y(fromPos.y);
    request.mutable_frompos()->set_z(fromPos.z);
    request.mutable_topos()->set_x(toPos.x);
    request.mutable_topos()->set_y(toPos.y);
    request.mutable_topos()->set_z(toPos.z);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsSightClear(&context, request, &response);
    return response.value();
}

//Thing
uint32_t BotClient::getId(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::UInt32Value response;
    ClientContext context;
    Status status = stub->GetId(&context, request, &response);
    return response.value();
}

Position BotClient::getPosition(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    bot_Position response;
    ClientContext context;
    Status status = stub->GetPosition(&context, request, &response);
    if (status.ok()) {
        Position position{};
        position.x = response.x();
        position.y = response.y();
        position.z = response.z();
        return position;
    }
    return {};
}

uintptr_t BotClient::getParentContainer(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetParentContainer(&context, request, &response);
    return response.value();
}

bool BotClient::isItem(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsItem(&context, request, &response);
    return response.value();
}

bool BotClient::isMonster(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsMonster(&context, request, &response);
    return response.value();
}

bool BotClient::isNpc(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsNpc(&context, request, &response);
    return response.value();
}

bool BotClient::isCreature(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsCreature(&context, request, &response);
    return response.value();
}

bool BotClient::isPlayer(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsPlayer(&context, request, &response);
    return response.value();
}

bool BotClient::isLocalPlayer(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsLocalPlayer(&context, request, &response);
    return response.value();
}

bool BotClient::isContainer(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsContainer(&context, request, &response);
    return response.value();
}

bool BotClient::isUsable(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsUsable(&context, request, &response);
    return response.value();
}

bool BotClient::isLyingCorpse(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsLyingCorpse(&context, request, &response);
    return response.value();
}

// Tile
uintptr_t BotClient::getTopThing(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetTopThing(&context, request, &response);
    return response.value();
}

uintptr_t BotClient::getTopUseThing(uintptr_t value)
{
    UInt64Value request;
    request.set_value(value);
    UInt64Value response;
    ClientContext context;
    Status status = stub->GetTopUseThing(&context, request, &response);
    return response.value();
}

std::vector<uintptr_t> BotClient::getTileItems(uintptr_t tile) {
    UInt64Value request;
    request.set_value(tile);
    bot_Uint64List response;
    ClientContext context;
    Status status = stub->GetTileItems(&context, request, &response);
    std::vector<uintptr_t> items;
    if (status.ok()) {
        for (const auto& item : response.items()) {
            items.push_back(item);
        }
    }
    return items;
}

bool BotClient::isWalkable(uintptr_t tile, bool ignoreCreatures) {
    bot_IsWalkableRequest request;
    request.set_ignorecreatures(ignoreCreatures);
    request.set_tile(tile);
    google::protobuf::BoolValue response;
    ClientContext context;
    Status status = stub->IsWalkable(&context, request, &response);
    return response.value();

}

// Custom Functions
void BotClient::clearMessages() {
    Empty request;
    Empty response;
    ClientContext context;
    Status status = stub->ClearMessages(&context, request, &response);
}

void BotClient::dropMessages(int messageNumber) {
    google::protobuf::UInt32Value request;
    request.set_value(messageNumber);
    Empty response;
    ClientContext context;
    Status status = stub->DropMessages(&context, request, &response);
}

