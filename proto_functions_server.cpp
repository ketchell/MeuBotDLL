#include "proto_functions_server.h"

#include <fstream>
#include <Windows.h>
#include "Container.h"
#include "Game.h"
#include "Tile.h"
#include "Map.h"
#include "Thing.h"
#include "LocalPlayer.h"
#include "Creature.h"
#include "CustomFunctions.h"
#include "Item.h"
// ============================================================================
// Helpers
// ============================================================================

template<typename T>
SmartPtr<T> toPtr(uint64_t val) {
    return SmartPtr<T>(val);
}

extern std::atomic<bool> g_ready;
static grpc::Status notReady() {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Bot not ready");
}

Position toPos(const bot::bot_Position& proto) {
    return { static_cast<int32_t>(proto.x()), static_cast<int32_t>(proto.y()), static_cast<int16_t>(proto.z()) };
}

void fromPos(const Position& pos, bot::bot_Position* proto) {
    proto->set_x(pos.x);
    proto->set_y(pos.y);
    proto->set_z(pos.z);
}

// ============================================================================
// Handlers
// ============================================================================

// ================= Container.h =================
Status BotServiceImpl::GetItem(ServerContext* context, const bot::bot_GetItemRequest* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_container->getItem(toPtr<Container>(request->container()), request->slot()));
    return Status::OK;
}

Status BotServiceImpl::GetItems(ServerContext* context, const google::protobuf::UInt64Value* request, bot::bot_Uint64List* response) {
    auto items = g_container->getItems(toPtr<Container>(request->value()));
    for (auto item : items) response->add_items(item);
    return Status::OK;
}

Status BotServiceImpl::GetItemsCount(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) {
    response->set_value(g_container->getItemsCount(toPtr<Container>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetCapacity(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) {
    response->set_value(g_container->getCapacity(toPtr<Container>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetSlotPosition(ServerContext* context, const bot::bot_GetSlotPositionRequest* request, bot::bot_Position* response) {
    fromPos(g_container->getSlotPosition(toPtr<Container>(request->container()), request->slot()), response);
    return Status::OK;
}

Status BotServiceImpl::GetContainerName(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) {
    response->set_value(g_container->getName(toPtr<Container>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetContainerId(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) {
    response->set_value(g_container->getId(toPtr<Container>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetContainerItem(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_container->getContainerItem(toPtr<Container>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::HasParent(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_container->hasParent(toPtr<Container>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetFirstIndex(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) {
    response->set_value(g_container->getFirstIndex(toPtr<Container>(request->value())));
    return Status::OK;
}

// ================= Creature.h =================
Status BotServiceImpl::GetCreatureName(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) {
    response->set_value(g_creature->getName(toPtr<Creature>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetManaPercent(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    response->set_value(g_creature->getManaPercent(toPtr<Creature>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetHealthPercent(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    response->set_value(g_creature->getHealthPercent(toPtr<Creature>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetSkull(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    response->set_value(static_cast<uint32_t>(g_creature->getSkull(toPtr<Creature>(request->value()))));
    return Status::OK;
}

Status BotServiceImpl::GetDirection(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) {
    response->set_value(static_cast<int32_t>(g_creature->getDirection(toPtr<Creature>(request->value()))));
    return Status::OK;
}

Status BotServiceImpl::IsDead(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_creature->isDead(toPtr<Creature>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsWalking(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_creature->isWalking(toPtr<Creature>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::CanBeSeen(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_creature->canBeSeen(toPtr<Creature>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsCovered(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_creature->isCovered(toPtr<Creature>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::CanShoot(ServerContext* context, const bot::bot_CanShootRequest* request, google::protobuf::BoolValue* response) {
    response->set_value(g_creature->canShoot(toPtr<Creature>(request->creature()), request->distance()));
    return Status::OK;
}

Status BotServiceImpl::SafeLogout(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) {
    g_game->safeLogout();
    return Status::OK;
}

// ================= Game.h =================
Status BotServiceImpl::Walk(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::Empty* response) {
    g_game->walk(static_cast<Otc::Direction>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::AutoWalkGame(ServerContext* context, const bot::bot_AutoWalkGameRequest* request, google::protobuf::Empty* response) {
    std::vector<Otc::Direction> dirs;
    for (auto item : request->direction().dirs())
        dirs.push_back(static_cast<Otc::Direction>(item));
    g_game->autoWalk(dirs, toPos(request->startpos()));
    return Status::OK;
}

Status BotServiceImpl::Turn(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::Empty* response) {
    g_game->turn(static_cast<Otc::Direction>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::Stop(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) {
    g_game->stop();
    return Status::OK;
}

Status BotServiceImpl::Move(ServerContext* context, const bot::bot_MoveRequest* request, google::protobuf::Empty* response) {
    g_game->move(toPtr<Thing>(request->thing()), toPos(request->topos()), request->count());
    return Status::OK;
}

Status BotServiceImpl::MoveToParentContainer(ServerContext* context, const bot::bot_MoveToParentContainerRequest* request, google::protobuf::Empty* response) {
    g_game->moveToParentContainer(toPtr<Thing>(request->thing()), request->count());
    return Status::OK;
}

Status BotServiceImpl::Use(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) {
    g_game->use(toPtr<Item>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::UseWith(ServerContext* context, const bot::bot_UseWithRequest* request, google::protobuf::Empty* response) {
    g_game->useWith(toPtr<Item>(request->item()), toPtr<Thing>(request->tothing()));
    return Status::OK;
}

Status BotServiceImpl::UseInventoryItem(ServerContext* context, const google::protobuf::UInt32Value* request, google::protobuf::Empty* response) {
    g_game->useInventoryItem(request->value());
    return Status::OK;
}

Status BotServiceImpl::UseInventoryItemWith(ServerContext* context, const bot::bot_UseInventoryItemWithRequest* request, google::protobuf::Empty* response) {
    g_game->useInventoryItemWith(request->itemid(), toPtr<Thing>(request->tothing()));
    return Status::OK;
}

Status BotServiceImpl::FindItemInContainers(ServerContext* context, const bot::bot_FindItemInContainersRequest* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_game->findItemInContainers(request->itemid(), request->subtype(), request->tier()));
    return Status::OK;
}

Status BotServiceImpl::Open(ServerContext* context, const bot::bot_OpenRequest* request, google::protobuf::Int32Value* response) {
    response->set_value(g_game->open(toPtr<Item>(request->item()), toPtr<Container>(request->previouscontainer())));
    return Status::OK;
}

Status BotServiceImpl::OpenParent(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) {
    g_game->openParent(toPtr<Container>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::Close(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) {
    g_game->close(toPtr<Container>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::RefreshContainer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) {
    g_game->refreshContainer(toPtr<Container>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::Attack(ServerContext* context, const bot::bot_AttackRequest* request, google::protobuf::Empty* response) {
    g_game->attack(toPtr<Creature>(request->creature()), request->cancel());
    return Status::OK;
}

Status BotServiceImpl::CancelAttack(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) {
    g_game->cancelAttack();
    return Status::OK;
}

Status BotServiceImpl::Follow(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) {
    g_game->follow(toPtr<Creature>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::CancelAttackAndFollow(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) {
    g_game->cancelAttackAndFollow();
    return Status::OK;
}

Status BotServiceImpl::Talk(ServerContext* context, const google::protobuf::StringValue* request, google::protobuf::Empty* response) {
    g_game->talk(request->value());
    return Status::OK;
}

Status BotServiceImpl::TalkChannel(ServerContext* context, const bot::bot_TalkChannelRequest* request, google::protobuf::Empty* response) {
    g_game->talkChannel(static_cast<Otc::MessageMode>(request->mode()), request->channelid(), request->message());
    return Status::OK;
}

Status BotServiceImpl::TalkPrivate(ServerContext* context, const bot::bot_TalkPrivateRequest* request, google::protobuf::Empty* response) {
    g_game->talkPrivate(static_cast<Otc::MessageMode>(request->msgmode()), request->receiver(), request->message());
    return Status::OK;
}

Status BotServiceImpl::OpenPrivateChannel(ServerContext* context, const google::protobuf::StringValue* request, google::protobuf::Empty* response) {
    g_game->openPrivateChannel(request->value());
    return Status::OK;
}

Status BotServiceImpl::GetChaseMode(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Int32Value* response) {
    response->set_value(g_game->getChaseMode());
    return Status::OK;
}

Status BotServiceImpl::GetFightMode(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Int32Value* response) {
    response->set_value(g_game->getFightMode());
    return Status::OK;
}

Status BotServiceImpl::SetChaseMode(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::Empty* response) {
    g_game->setChaseMode(static_cast<Otc::ChaseModes>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::SetFightMode(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::Empty* response) {
    g_game->setFightMode(static_cast<Otc::FightModes>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::BuyItem(ServerContext* context, const bot::bot_BuyItemRequest* request, google::protobuf::Empty* response) {
    g_game->buyItem(toPtr<Item>(request->item()), request->amount(), request->ignorecapacity(), request->buywithbackpack());
    return Status::OK;
}

Status BotServiceImpl::SellItem(ServerContext* context, const bot::bot_SellItemRequest* request, google::protobuf::Empty* response) {
    g_game->sellItem(toPtr<Item>(request->item()), request->amount(), request->ignoreequipped());
    return Status::OK;
}

Status BotServiceImpl::EquipItem(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) {
    g_game->equipItem(toPtr<Item>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::EquipItemId(ServerContext* context, const bot::bot_EquipItemIdRequest* request, google::protobuf::Empty* response) {
    g_game->equipItemId(request->itemid(), request->tier());
    return Status::OK;
}

Status BotServiceImpl::Mount(ServerContext* context, const google::protobuf::BoolValue* request, google::protobuf::Empty* response) {
    g_game->mount(request->value());
    return Status::OK;
}

Status BotServiceImpl::ChangeMapAwareRange(ServerContext* context, const bot::bot_ChangeMapAwareRangeRequest* request, google::protobuf::Empty* response) {
    g_game->changeMapAwareRange(request->xrange(), request->yrange());
    return Status::OK;
}

Status BotServiceImpl::CanPerformGameAction(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::BoolValue* response) {
    response->set_value(g_game->canPerformGameAction());
    return Status::OK;
}

Status BotServiceImpl::IsOnline(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::BoolValue* response) {
    if (!g_ready) return notReady();
    response->set_value(g_game->isOnline());
    return Status::OK;
}

Status BotServiceImpl::IsAttacking(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::BoolValue* response) {
    response->set_value(g_game->isAttacking());
    return Status::OK;
}

Status BotServiceImpl::IsFollowing(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::BoolValue* response) {
    response->set_value(g_game->isFollowing());
    return Status::OK;
}

Status BotServiceImpl::GetContainer(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_game->getContainer(request->value()));
    return Status::OK;
}

Status BotServiceImpl::GetContainers(ServerContext* context, const google::protobuf::Empty* request, bot::bot_Uint64List* response) {
    for (auto container : g_game->getContainers())
        response->add_items(container);
    return Status::OK;
}

Status BotServiceImpl::GetAttackingCreature(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_game->getAttackingCreature());
    return Status::OK;
}

Status BotServiceImpl::GetFollowingCreature(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_game->getFollowingCreature());
    return Status::OK;
}

Status BotServiceImpl::GetLocalPlayer(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::UInt64Value* response) {
    if (!g_ready) return notReady();
    response->set_value(g_game->getLocalPlayer());
    return Status::OK;
}

Status BotServiceImpl::GetClientVersion(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Int32Value* response) {
    if (!g_ready) return notReady();
    response->set_value(g_game->getClientVersion());
    return Status::OK;
}

Status BotServiceImpl::GetCharacterName(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::StringValue* response) {
    if (!g_ready) return notReady();
    response->set_value(g_game->getCharacterName());
    return Status::OK;
}

// ================= Item.h =================
Status BotServiceImpl::GetCount(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) {
    response->set_value(g_item->getCount(toPtr<Item>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetSubType(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) {
    response->set_value(g_item->getSubType(toPtr<Item>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetItemId(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    response->set_value(g_item->getId(toPtr<Item>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetTooltip(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) {
    response->set_value(g_item->getTooltip(toPtr<Item>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetDurationTime(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    response->set_value(g_item->getDurationTime(toPtr<Item>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetItemName(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) {
    response->set_value(g_item->getName(toPtr<Item>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetDescription(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) {
    response->set_value(g_item->getDescription(toPtr<Item>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetTier(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    response->set_value(g_item->getTier(toPtr<Item>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetText(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) {
    response->set_value(g_item->getText(toPtr<Item>(request->value())));
    return Status::OK;
}

// ================= LocalPlayer.h =================

Status BotServiceImpl::GetStates(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    if (!g_ready) return notReady();
    response->set_value(static_cast<uint32_t>(g_localPlayer->getStates(toPtr<LocalPlayer>(request->value()))));
    return Status::OK;
}

Status BotServiceImpl::GetHealth(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) {
    if (!g_ready) return notReady();
    response->set_value(g_localPlayer->getHealth(toPtr<LocalPlayer>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetMaxHealth(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) {
    if (!g_ready) return notReady();
    response->set_value(g_localPlayer->getMaxHealth(toPtr<LocalPlayer>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetFreeCapacity(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) {
    if (!g_ready) return notReady();
    response->set_value(g_localPlayer->getFreeCapacity(toPtr<LocalPlayer>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetLevel(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    if (!g_ready) return notReady();
    response->set_value(static_cast<uint32_t>(g_localPlayer->getLevel(toPtr<LocalPlayer>(request->value()))));
    return Status::OK;
}

Status BotServiceImpl::GetMana(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) {
    if (!g_ready) return notReady();
    response->set_value(g_localPlayer->getMana(toPtr<LocalPlayer>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetMaxMana(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) {
    if (!g_ready) return notReady();
    response->set_value(g_localPlayer->getMaxMana(toPtr<LocalPlayer>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetSoul(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    if (!g_ready) return notReady();
    response->set_value(static_cast<uint32_t>(g_localPlayer->getSoul(toPtr<LocalPlayer>(request->value()))));
    return Status::OK;
}

Status BotServiceImpl::GetStamina(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    if (!g_ready) return notReady();
    response->set_value(static_cast<uint32_t>(g_localPlayer->getStamina(toPtr<LocalPlayer>(request->value()))));
    return Status::OK;
}

Status BotServiceImpl::GetInventoryItem(ServerContext* context, const bot::bot_GetInventoryItemRequest* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_localPlayer->getInventoryItem(toPtr<LocalPlayer>(request->localplayer()), static_cast<Otc::InventorySlot>(request->inventoryslot())));
    return Status::OK;
}

Status BotServiceImpl::GetInventoryCount(ServerContext* context, const bot::bot_GetInventoryCountRequest* request, google::protobuf::UInt32Value* response) {
    response->set_value(g_localPlayer->getInventoryCount(toPtr<LocalPlayer>(request->localplayer()), request->itemid(), request->tier()));
    return Status::OK;
}

Status BotServiceImpl::HasSight(ServerContext* context, const bot::bot_HasSightRequest* request, google::protobuf::BoolValue* response) {
    response->set_value(g_localPlayer->hasSight(toPtr<LocalPlayer>(request->localplayer()), toPos(request->pos())));
    return Status::OK;
}

Status BotServiceImpl::IsAutoWalking(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_localPlayer->isAutoWalking(toPtr<LocalPlayer>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::StopAutoWalk(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) {
    g_localPlayer->stopAutoWalk(toPtr<LocalPlayer>(request->value()));
    return Status::OK;
}

Status BotServiceImpl::AutoWalk(ServerContext* context, const bot::bot_AutoWalkRequest* request, google::protobuf::BoolValue* response) {
    response->set_value(g_localPlayer->autoWalk(toPtr<LocalPlayer>(request->localplayer()), toPos(request->destination()), request->retry()));
    return Status::OK;
}

Status BotServiceImpl::SetLightHack(ServerContext* context, const bot::bot_SetLightHackRequest* request, google::protobuf::Empty* response) {
    g_localPlayer->setLightHack(toPtr<LocalPlayer>(request->localplayer()), static_cast<uint16_t>(request->lightlevel()));
    return Status::OK;
}

// ================= Map.h =================
Status BotServiceImpl::GetTile(ServerContext* context, const bot::bot_Position* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_map->getTile(toPos(*request)));
    return Status::OK;
}

Status BotServiceImpl::GetSpectators(ServerContext* context, const bot::bot_GetSpectatorsRequest* request, bot::bot_Uint64List* response) {
    for (auto s : g_map->getSpectators(toPos(request->centerpos()), request->multifloor()))
        response->add_items(s);
    return Status::OK;
}

Status BotServiceImpl::FindPath(ServerContext* context, const bot::bot_FindPathRequest* request, bot::bot_DirectionList* response) {
    for (auto dir : g_map->findPath(toPos(request->startpos()), toPos(request->goalpos()), request->maxcomplexity(), request->flags()))
        response->add_dirs(static_cast<bot::bot_Direction>(dir));
    return Status::OK;
}

Status BotServiceImpl::IsSightClear(ServerContext* context, const bot::bot_IsSightClearRequest* request, google::protobuf::BoolValue* response) {
    response->set_value(g_map->isSightClear(toPos(request->frompos()), toPos(request->topos())));
    return Status::OK;
}

// ================= Thing.h =================
Status BotServiceImpl::GetId(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) {
    response->set_value(g_thing->getId(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetPosition(ServerContext* context, const google::protobuf::UInt64Value* request, bot::bot_Position* response) {
    fromPos(g_thing->getPosition(toPtr<Thing>(request->value())), response);
    return Status::OK;
}

Status BotServiceImpl::GetParentContainer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_thing->getParentContainer(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsItem(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isItem(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsMonster(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isMonster(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsNpc(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isNpc(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsCreature(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isCreature(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsPlayer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isPlayer(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsLocalPlayer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isLocalPlayer(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsContainer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isContainer(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsUsable(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isUsable(toPtr<Thing>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::IsLyingCorpse(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) {
    response->set_value(g_thing->isLyingCorpse(toPtr<Thing>(request->value())));
    return Status::OK;
}

// ================= Tile.h =================
Status BotServiceImpl::GetTopThing(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_tile->getTopThing(toPtr<Tile>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetTopUseThing(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt64Value* response) {
    response->set_value(g_tile->getTopUseThing(toPtr<Tile>(request->value())));
    return Status::OK;
}

Status BotServiceImpl::GetTileItems(ServerContext* context, const google::protobuf::UInt64Value* request, bot::bot_Uint64List* response) {
    for (const auto& val : g_tile->getItems(toPtr<Tile>(request->value())))
        response->add_items(val);
    return Status::OK;
}

Status BotServiceImpl::IsWalkable(ServerContext* context, const bot::bot_IsWalkableRequest* request, google::protobuf::BoolValue* response) {
    response->set_value(g_tile->isWalkable(toPtr<Tile>(request->tile()), request->ignorecreatures()));
    return Status::OK;
}

// ================= CustomFunctions.h =================
Status BotServiceImpl::GetMessages(ServerContext* context, const google::protobuf::UInt32Value* request, bot::bot_GetMessages* response) {
    for (auto cpp_msg : g_custom->getMessages(request->value())) {
        bot::bot_Message* proto_msg = response->add_messages();
        proto_msg->set_name(cpp_msg.name);
        proto_msg->set_level(cpp_msg.level);
        proto_msg->set_mode(static_cast<uint32_t>(cpp_msg.mode));
        proto_msg->set_text(cpp_msg.text);
        proto_msg->set_channel_id(cpp_msg.channelId);
        fromPos(cpp_msg.pos, proto_msg->mutable_pos());
    }
    return grpc::Status::OK;
}

Status BotServiceImpl::ClearMessages(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) {
    g_custom->clearMessages();
    return grpc::Status::OK;
}

Status BotServiceImpl::DropMessages(ServerContext* context, const google::protobuf::UInt32Value* request, google::protobuf::Empty* response) {
    g_custom->dropMessages(request->value());
    return grpc::Status::OK;
}

// End of Functions


extern HMODULE g_hDll;

static void WritePortFile(int port) {
    char dllPath[MAX_PATH];
    if (!GetModuleFileNameA(g_hDll, dllPath, MAX_PATH))
        return;
    char* lastSlash = strrchr(dllPath, '\\');
    if (!lastSlash)
        return;
    *(lastSlash + 1) = '\0';
    char portFile[MAX_PATH];
    strcpy_s(portFile, dllPath);
    strcat_s(portFile, "easybot_port.txt");
    std::ofstream f(portFile, std::ios::trunc);
    f << port;
}

void RunServer() {
    BotServiceImpl service;
    int port = 50051;
    std::unique_ptr<grpc::Server> server;

    while (true) {
        std::string server_address("0.0.0.0:" + std::to_string(port));
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
        if (server) break;
        port++;
    }
    Sleep(3000);
    WritePortFile(port);
    server->Wait();
}
