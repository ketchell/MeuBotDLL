#ifndef PROTO_FUNCTIONS_H
#define PROTO_FUNCTIONS_H
#include <grpcpp/grpcpp.h>
#include "bot.pb.h"
#include "bot.grpc.pb.h"

using grpc::Status;
using grpc::ServerContext;

class BotServiceImpl : public bot::BotService::Service {
public:
    // --- Container.h ---
    Status GetItem(ServerContext* context, const bot::bot_GetItemRequest* request, google::protobuf::UInt64Value* response) override;
    Status GetItems(ServerContext* context, const google::protobuf::UInt64Value* request, bot::bot_Uint64List* response) override;
    Status GetItemsCount(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) override;
    Status GetSlotPosition(ServerContext* context, const bot::bot_GetSlotPositionRequest* request, bot::bot_Position* response) override;
    Status GetContainerName(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) override;
    Status GetContainerId(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) override;
    Status GetContainerItem(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt64Value* response) override;
    Status HasParent(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status GetCapacity(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) override;
    Status GetFirstIndex(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) override;

    // --- Creature.h ---
    Status GetCreatureName(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) override;
    Status GetManaPercent(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetHealthPercent(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetSkull(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetDirection(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) override;
    Status IsDead(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsWalking(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status CanBeSeen(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsCovered(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status CanShoot(ServerContext* context, const bot::bot_CanShootRequest* request, google::protobuf::BoolValue* response) override;

    // --- Game.h ---
    Status SafeLogout(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) override;
    Status Walk(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::Empty* response) override;
    Status AutoWalkGame(ServerContext* context, const bot::bot_AutoWalkGameRequest* request, google::protobuf::Empty* response) override;
    Status Turn(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::Empty* response) override;
    Status Stop(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) override;
    Status Move(ServerContext* context, const bot::bot_MoveRequest* request, google::protobuf::Empty* response) override;
    Status MoveToParentContainer(ServerContext* context, const bot::bot_MoveToParentContainerRequest* request, google::protobuf::Empty* response) override;
    Status Use(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) override;
    Status UseWith(ServerContext* context, const bot::bot_UseWithRequest* request, google::protobuf::Empty* response) override;
    Status UseInventoryItem(ServerContext* context, const google::protobuf::UInt32Value* request, google::protobuf::Empty* response) override;
    Status UseInventoryItemWith(ServerContext* context, const bot::bot_UseInventoryItemWithRequest* request, google::protobuf::Empty* response) override;
    Status FindItemInContainers(ServerContext* context, const bot::bot_FindItemInContainersRequest* request, google::protobuf::UInt64Value* response) override;
    Status Open(ServerContext* context, const bot::bot_OpenRequest* request, google::protobuf::Int32Value* response) override;
    Status OpenParent(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) override;
    Status Close(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) override;
    Status RefreshContainer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) override;
    Status Attack(ServerContext* context, const bot::bot_AttackRequest* request, google::protobuf::Empty* response) override;
    Status CancelAttack(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) override;
    Status Follow(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) override;
    Status CancelAttackAndFollow(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) override;
    Status Talk(ServerContext* context, const google::protobuf::StringValue* request, google::protobuf::Empty* response) override;
    Status TalkChannel(ServerContext* context, const bot::bot_TalkChannelRequest* request, google::protobuf::Empty* response) override;
    Status TalkPrivate(ServerContext* context, const bot::bot_TalkPrivateRequest* request, google::protobuf::Empty* response) override;
    Status OpenPrivateChannel(ServerContext* context, const google::protobuf::StringValue* request, google::protobuf::Empty* response) override;
    Status GetChaseMode(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Int32Value* response) override;
    Status GetFightMode(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Int32Value* response) override;
    Status SetChaseMode(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::Empty* response) override;
    Status SetFightMode(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::Empty* response) override;
    Status BuyItem(ServerContext* context, const bot::bot_BuyItemRequest* request, google::protobuf::Empty* response) override;
    Status SellItem(ServerContext* context, const bot::bot_SellItemRequest* request, google::protobuf::Empty* response) override;
    Status EquipItem(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) override;
    Status EquipItemId(ServerContext* context, const bot::bot_EquipItemIdRequest* request, google::protobuf::Empty* response) override;
    Status Mount(ServerContext* context, const google::protobuf::BoolValue* request, google::protobuf::Empty* response) override;
    Status ChangeMapAwareRange(ServerContext* context, const bot::bot_ChangeMapAwareRangeRequest* request, google::protobuf::Empty* response) override;
    Status CanPerformGameAction(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::BoolValue* response) override;
    Status IsOnline(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::BoolValue* response) override;
    Status IsAttacking(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::BoolValue* response) override;
    Status IsFollowing(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::BoolValue* response) override;
    Status GetContainer(ServerContext* context, const google::protobuf::Int32Value* request, google::protobuf::UInt64Value* response) override;
    Status GetContainers(ServerContext* context, const google::protobuf::Empty* request, bot::bot_Uint64List* response) override;
    Status GetAttackingCreature(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::UInt64Value* response) override;
    Status GetFollowingCreature(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::UInt64Value* response) override;
    Status GetLocalPlayer(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::UInt64Value* response) override;
    Status GetClientVersion(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Int32Value* response) override;
    Status GetCharacterName(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::StringValue* response) override;

    // --- Item.h ---
    Status GetCount(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) override;
    Status GetSubType(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Int32Value* response) override;
    Status GetItemId(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetTooltip(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) override;
    Status GetDurationTime(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetItemName(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) override;
    Status GetDescription(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) override;
    Status GetTier(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetText(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::StringValue* response) override;

    // --- LocalPlayer.h ---
    Status GetStates(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetHealth(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) override;
    Status GetMaxHealth(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) override;
    Status GetFreeCapacity(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) override;
    Status GetLevel(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetMana(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) override;
    Status GetMaxMana(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::DoubleValue* response) override;
    Status GetSoul(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetStamina(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetInventoryItem(ServerContext* context, const bot::bot_GetInventoryItemRequest* request, google::protobuf::UInt64Value* response) override;
    Status GetInventoryCount(ServerContext* context, const bot::bot_GetInventoryCountRequest* request, google::protobuf::UInt32Value* response) override;
    Status HasSight(ServerContext* context, const bot::bot_HasSightRequest* request, google::protobuf::BoolValue* response) override;
    Status IsAutoWalking(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status StopAutoWalk(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::Empty* response) override;
    Status AutoWalk(ServerContext* context, const bot::bot_AutoWalkRequest* request, google::protobuf::BoolValue* response) override;
    Status SetLightHack(ServerContext* context, const bot::bot_SetLightHackRequest* request, google::protobuf::Empty* response) override;

    // --- Map.h ---
    Status GetTile(ServerContext* context, const bot::bot_Position* request, google::protobuf::UInt64Value* response) override;
    Status GetSpectators(ServerContext* context, const bot::bot_GetSpectatorsRequest* request, bot::bot_Uint64List* response) override;
    Status FindPath(ServerContext* context, const bot::bot_FindPathRequest* request, bot::bot_DirectionList* response) override;
    Status IsSightClear(ServerContext* context, const bot::bot_IsSightClearRequest* request, google::protobuf::BoolValue* response) override;

    // --- Thing.h ---
    Status GetId(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt32Value* response) override;
    Status GetPosition(ServerContext* context, const google::protobuf::UInt64Value* request, bot::bot_Position* response) override;
    Status GetParentContainer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt64Value* response) override;
    Status IsItem(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsMonster(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsNpc(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsCreature(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsPlayer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsLocalPlayer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsContainer(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsUsable(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;
    Status IsLyingCorpse(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::BoolValue* response) override;

    // --- Tile.h ---
    Status GetTopThing(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt64Value* response) override;
    Status GetTopUseThing(ServerContext* context, const google::protobuf::UInt64Value* request, google::protobuf::UInt64Value* response) override;
    Status GetTileItems(ServerContext* context, const google::protobuf::UInt64Value* request, bot::bot_Uint64List* response) override;
    Status IsWalkable(ServerContext* context, const bot::bot_IsWalkableRequest* request, google::protobuf::BoolValue* response) override;


    // --- CustomFunctions.h ---
    Status GetMessages(ServerContext* context, const google::protobuf::UInt32Value* request, bot::bot_GetMessages* response) override;
    Status ClearMessages(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* response) override;
    Status DropMessages(ServerContext* context, const google::protobuf::UInt32Value* request, google::protobuf::Empty* response) override;


};

void RunServer();

#endif // PROTO_FUNCTIONS_H

