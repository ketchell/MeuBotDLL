import grpc
import bot_pb2
import bot_pb2_grpc

from google.protobuf.wrappers_pb2 import UInt64Value
from google.protobuf.wrappers_pb2 import Int32Value
from google.protobuf.wrappers_pb2 import UInt32Value
from google.protobuf.wrappers_pb2 import StringValue
from google.protobuf.wrappers_pb2 import BoolValue
from google.protobuf.empty_pb2 import Empty
import inspect

SERVER_ADDRESS = 'localhost:50051'
channel = grpc.insecure_channel(SERVER_ADDRESS)
stub = bot_pb2_grpc.BotServiceStub(channel)

def list_protobuf_messages():
    print("--- All Needed Requests ---")
    for name, member in inspect.getmembers(bot_pb2):

        if inspect.isclass(member):
            if member.__module__ == 'bot_pb2':
                print(f"* {name}")
    print("-----------------------------------------------------")
    print("--- All Available Methodss ---")
    available_methods = [attr for attr in dir(stub) if not attr.startswith('_') and not attr in ['channel', 'channel_ready_future', 'close']]
    for method in available_methods:
        print(f"* {method}")

def get_item(container, slot):
    request = bot_pb2.bot_GetItemRequest()
    request.container = container
    request.slot = slot
    response = stub.GetItem(request)
    return response.value

def get_items(value):
    request = UInt64Value(value=value)
    response = stub.GetItems(request)
    return list(response.items)

def get_items_count(value):
    request = UInt64Value(value=value)
    response = stub.GetItemsCount(request)
    return response.value

def get_slot_position(container, slot):
    request = bot_pb2.bot_GetSlotPositionRequest()
    request.container = container
    request.slot = slot
    response = stub.GetSlotPosition(request)
    return {'x': response.x, 'y': response.y, 'z': response.z}

def get_container_name(value):
    request = UInt64Value(value=value)
    response = stub.GetContainerName(request)
    return response.value

def get_container_id(value):
    request = UInt64Value(value=value)
    response = stub.GetContainerId(request)
    return response.value

def get_container_item(value):
    request = UInt64Value(value=value)
    response = stub.GetContainerItem(request)
    return response.value

def has_parent(value):
    request = UInt64Value(value=value)
    response = stub.HasParent(request)
    return response.value

def get_size(value):
    request = UInt64Value(value=value)
    response = stub.GetSize(request)
    return response.value

def get_first_index(value):
    request = UInt64Value(value=value)
    response = stub.GetFirstIndex(request)
    return response.value

def get_creature_name(value):
    request = UInt64Value(value=value)
    response = stub.GetCreatureName(request)
    return response.value

def get_health_percent(value):
    request = UInt64Value(value=value)
    response = stub.GetHealthPercent(request)
    return response.value

def get_direction(value):
    request = UInt64Value(value=value)
    response = stub.GetDirection(request)
    return response.value

def is_dead(value):
    request = UInt64Value(value=value)
    response = stub.IsDead(request)
    return response.value

def can_be_seen(value):
    request = UInt64Value(value=value)
    response = stub.CanBeSeen(request)
    return response.value

def is_covered(value):
    request = UInt64Value(value=value)
    response = stub.IsCovered(request)
    return response.value

def can_shoot(value):
    request = UInt64Value(value=value)
    response = stub.CanShoot(request)
    return response.value

def walk(direction):
    request = Int32Value(value=direction)
    response = stub.Walk(request)

def auto_walk_game(dirs, startpos):
    request = bot_pb2.bot_AutoWalkGameRequest()
    for item in dirs:
        request.dir.append(item)
    request.startPos.x = startpos['x']
    request.startPos.y = startpos['y']
    request.startPos.z = startpos['z']
    response = stub.AutoWalkGame(request)

def turn(direction):
    request = Int32Value(value=direction)
    response = stub.Turn(request)

def stop():
    request = Empty()
    response = stub.Stop(request)

def move(thing, topos, count):
    request = bot_pb2.bot_MoveRequest()
    request.thing = thing
    request.count = count
    request.toPos.x = topos['x']
    request.toPos.y = topos['y']
    request.toPos.z = topos['z']
    response = stub.Move(request)

def move_to_parent_container(thing, count):
    request = bot_pb2.bot_MoveToParentContainerRequest()
    request.thing = thing
    request.count = count
    response = stub.MoveToParentContainer(request)

def use(thing):
    request = UInt64Value(value=thing)
    response = stub.Use(request)

def use_with(item, toThing):
    request = bot_pb2.bot_UseWithRequest()
    request.item = item
    request.toThing = toThing
    response = stub.UseWith(request)

def use_inventory_item(itemId):
    request = UInt32Value(value=itemId)
    response = stub.UseInventoryItem(request)

def use_inventory_item_with(itemId, toThing):
    request = bot_pb2.bot_UseInventoryItemWithRequest()
    request.tothing = toThing
    request.itemid = itemId
    response = stub.UseInventoryItemWith(request)

def find_item_in_containers(itemId, subType, tier):
    request = bot_pb2.bot_FindItemInContainersRequest()
    request.itemid = itemId
    request.subtype = subType
    request.tier = tier
    response = stub.FindItemInContainers(request)
    return response.value

def open(item, previousContainer):
    request = bot_pb2.bot_OpenRequest()
    request.item = item
    request.previousContainer = previousContainer
    response = stub.Open(request)
    return response.value

def open_parent(container):
    request = UInt64Value(value=container)
    response = stub.OpenParent(request)

def close(container):
    request = UInt64Value(value=container)
    response = stub.Close(request)

def refresh_container(container):
    request = UInt64Value(value=container)
    response = stub.RefreshContainer(request)

def attack(creature, cancel):
    request = bot_pb2.bot_AttackRequest()
    request.creature = creature
    request.cancel = cancel
    response = stub.Attack(request)

def cancel_attack():
    request = Empty()
    response = stub.CancelAttack(request)

def follow(creature):
    request = UInt64Value(value=creature)
    response = stub.Follow(request)

def cancel_attack_and_follow():
    request = Empty()
    response = stub.CancelAttackAndFollow(request)

def talk(message):
    request = StringValue(value=message)
    response = stub.Talk(request)

def talk_channel(mode, channelId, message):
    request = bot_pb2.bot_TalkChannelRequest()
    request.mode = mode
    request.channelId = channelId
    request.message = message
    response = stub.TalkChannel(request)

def talk_private(msgMode, receiver, message):
    request = bot_pb2.bot_TalkPrivateRequest()
    request.msgMode = msgMode
    request.receiver = receiver
    request.message = message
    response = stub.TalkPrivate(request)

def open_private_channel(receiver):
    request = StringValue(value=receiver)
    response = stub.OpenPrivateChannel(request)

def set_chase_mode(mode):
    request = Int32Value(value=mode)
    response = stub.SetChaseMode(request)

def set_fight_mode(mode):
    request = Int32Value(value=mode)
    response = stub.SetFightMode(request)

def buy_item(item, amount, ignoreCapacity, buyWithBackpack):
    request = bot_pb2.bot_BuyItemRequest()
    request.item = item
    request.amount = amount
    request.ignoreCapacity = ignoreCapacity
    request.buyWithBackpack = buyWithBackpack
    response = stub.BuyItem(request)

def sell_item(item, amount, ignoreEquipped):
    request = bot_pb2.bot_SellItemRequest()
    request.item = item
    request.amount = amount
    request.ignoreEquipped = ignoreEquipped
    response = stub.SellItem(request)

def equip_item(item):
    request = UInt64Value(value=item)
    response = stub.EquipItem(request)

def equip_item_id(itemId, tier):
    request = bot_pb2.bot_EquipItemIdRequest()
    request.itemid = itemId
    request.tier = tier
    response = stub.EquipItemId(request)

def mount(mountStatus):
    request = BoolValue(value=mountStatus)
    response = stub.Mount(request)

def change_map_aware_range(xrange, yrange):
    request = bot_pb2.bot_ChangeMapAwareRangeRequest()
    request.xrange = xrange
    request.yrange = yrange
    response = stub.ChangeMapAwareRange(request)

def can_perform_game_action():
    request = Empty()
    response = stub.CanPerformGameAction(request)
    return response.value

def is_online():
    request = Empty()
    response = stub.IsOnline(request)
    return response.value

def is_attacking():
    request = Empty()
    response = stub.IsAttacking(request)
    return response.value

def is_following():
    request = Empty()
    response = stub.IsFollowing(request)
    return response.value

def get_container(index):
    request = Int32Value(value=index)
    response = stub.GetContainer(request)
    return response.value

def get_containers():
    request = Empty()
    response = stub.GetContainers(request)
    return list(response.items)

def get_attacking_creature():
    request = Empty()
    response = stub.GetAttackingCreature(request)
    return response.value

def get_following_creature():
    request = Empty()
    response = stub.GetFollowingCreature(request)
    return response.value

def get_local_player():
    request = Empty()
    response = stub.GetLocalPlayer(request)
    return response.value

def get_client_version():
    request = Empty()
    response = stub.GetClientVersion(request)
    return response.value

def get_character_name():
    request = Empty()
    response = stub.GetCharacterName(request)
    return response.value

def get_count(item):
    request = UInt64Value(value=item)
    response = stub.GetCount(request)
    return response.value

def get_sub_type(item):
    request = UInt64Value(value=item)
    response = stub.GetSubType(request)
    return response.value

def get_item_id(item):
    request = UInt64Value(value=item)
    response = stub.GetItemId(request)
    return response.value

def get_tooltip(item):
    request = UInt64Value(value=item)
    response = stub.GetTooltip(request)
    return response.value

def get_duration_time(value):
    request = UInt64Value(value=value)
    response = stub.GetDurationTime(request)
    return response.value

def get_item_name(value):
    request = UInt64Value(value=value)
    response = stub.GetItemName(request)
    return response.value

def get_description(value):
    request = UInt64Value(value=value)
    response = stub.GetDescription(request)
    return response.value

def get_tier(value):
    request = UInt64Value(value=value)
    response = stub.GetTier(request)
    return response.value

def get_text(value):
    request = UInt64Value(value=value)
    response = stub.GetText(request)
    return response.value

def is_walk_locked(value):
    request = UInt64Value(value=value)
    response = stub.IsWalkLocked(request)
    return response.value

def get_states(value):
    request = UInt64Value(value=value)
    response = stub.GetStates(request)
    return response.value

def get_health(value):
    request = UInt64Value(value=value)
    response = stub.GetHealth(request)
    return response.value

def get_max_health(value):
    request = UInt64Value(value=value)
    response = stub.GetMaxHealth(request)
    return response.value

def get_free_capacity(value):
    request = UInt64Value(value=value)
    response = stub.GetFreeCapacity(request)
    return response.value

def get_level(value):
    request = UInt64Value(value=value)
    response = stub.GetLevel(request)
    return response.value

def get_mana(value):
    request = UInt64Value(value=value)
    response = stub.GetMana(request)
    return response.value

def get_max_mana(value):
    request = UInt64Value(value=value)
    response = stub.GetMaxMana(request)
    return response.value

def get_mana_shield(value):
    request = UInt64Value(value=value)
    response = stub.GetManaShield(request)
    return response.value

def get_soul(value):
    request = UInt64Value(value=value)
    response = stub.GetSoul(request)
    return response.value

def get_stamina(value):
    request = UInt64Value(value=value)
    response = stub.GetStamina(request)
    return response.value

def get_inventory_item(localPlayer, inventorySlot):
    request = bot_pb2.bot_GetInventoryItemRequest()
    request.localPlayer = localPlayer
    request.inventorySlot = inventorySlot
    response = stub.GetInventoryItem(request)
    return response.value

def has_equipped_item_id(localPlayer, itemId, tier):
    request = bot_pb2.bot_HasEquippedItemIdRequest()
    request.localPlayer = localPlayer
    request.itemid = itemId
    request.tier = tier
    response = stub.HasEquippedItemId(request)
    return response.value

def get_inventory_count(localPlayer, itemId, tier):
    request = bot_pb2.bot_GetInventoryCountRequest()
    request.localPlayer = localPlayer
    request.itemid = itemId
    request.tier = tier
    response = stub.GetInventoryCount(request)
    return response.value

def has_sight(localPlayer, pos):
    request = bot_pb2.bot_HasSightRequest()
    request.localPlayer = localPlayer
    request.pos.x = pos['x']
    request.pos.y = pos['y']
    request.pos.z = pos['z']
    response = stub.HasSight(request)
    return response.value

def is_auto_walking(value):
    request = UInt64Value(value=value)
    response = stub.IsAutoWalking(request)
    return response.value

def stop_auto_walk(value):
    request = UInt64Value(value=value)
    response = stub.StopAutoWalk(request)

def auto_walk(localPlayer, destination, retry):
    request = bot_pb2.bot_AutoWalkRequest()
    request.localPlayer = localPlayer
    request.retry = retry
    request.destination.x = destination['x']
    request.destination.y = destination['y']
    request.destination.z = destination['z']
    response = stub.AutoWalk(request)
    return response.value

def get_tile(tilePos):
    request = bot_pb2.bot_Position()
    request.x = tilePos['x']
    request.y = tilePos['y']
    request.z = tilePos['z']
    response = stub.GetTile(request)
    return response.value

def get_spectators(centerpos, multiFloor):
    request = bot_pb2.bot_GetSpectatorsRequest()
    request.multiFloor = multiFloor
    request.centerPos.x = centerpos['x']
    request.centerPos.y = centerpos['y']
    request.centerPos.z = centerpos['z']
    response = stub.GetSpectators(request)
    return list(response.items)

def find_path(startpos, goalpos, maxComplexity, flags):
    request = bot_pb2.bot_FindPathRequest()
    request.flags = flags
    request.maxComplexity = maxComplexity
    request.startPos.x = startpos['x']
    request.startPos.y = startpos['y']
    request.startPos.z = startpos['z']
    request.goalPos.x = goalpos['x']
    request.goalPos.y = goalpos['y']
    request.goalPos.z = goalpos['z']
    response = stub.FindPath(request)

def is_walkable(pos, ignoreCreatures):
    request = bot_pb2.bot_IsWalkableRequest()
    request.ignoreCreatures = ignoreCreatures
    request.pos.x = pos['x']
    request.pos.y = pos['y']
    request.pos.z = pos['z']
    response = stub.IsWalkable(request)
    return response.value

def is_sight_clear(fromPos, toPos):
    request = bot_pb2.bot_IsSightClearRequest()
    request.fromPos.x = fromPos['x']
    request.fromPos.y = fromPos['y']
    request.fromPos.z = fromPos['z']
    request.toPos.x = toPos['x']
    request.toPos.y = toPos['y']
    request.toPos.z = toPos['z']
    response = stub.IsSightClear(request)
    return response.value

def get_id(value):
    request = UInt64Value(value=value)
    response = stub.GetId(request)
    return response.value

def get_position(value):
    request = UInt64Value(value=value)
    response = stub.GetPosition(request)
    return {'x': response.x, 'y': response.y, 'z': response.z}

def get_parent_container(value):
    request = UInt64Value(value=value)
    response = stub.GetParentContainer(request)
    return response.value

def is_item(value):
    request = UInt64Value(value=value)
    response = stub.IsItem(request)
    return response.value

def is_monster(value):
    request = UInt64Value(value=value)
    response = stub.IsMonster(request)
    return response.value

def is_npc(value):
    request = UInt64Value(value=value)
    response = stub.IsNpc(request)
    return response.value

def is_creature(value):
    request = UInt64Value(value=value)
    response = stub.IsCreature(request)
    return response.value

def is_player(value):
    request = UInt64Value(value=value)
    response = stub.IsPlayer(request)
    return response.value

def is_local_player(value):
    request = UInt64Value(value=value)
    response = stub.IsLocalPlayer(request)
    return response.value

def is_container(value):
    request = UInt64Value(value=value)
    response = stub.IsContainer(request)
    return response.value

def is_usable(value):
    request = UInt64Value(value=value)
    response = stub.IsUsable(request)
    return response.value

def is_lying_corpse(value):
    request = UInt64Value(value=value)
    response = stub.IsLyingCorpse(request)
    return response.value

def get_top_thing(value):
    request = UInt64Value(value=value)
    response = stub.GetTopThing(request)
    return response.value

def get_top_use_thing(value):
    request = UInt64Value(value=value)
    response = stub.GetTopUseThing(request)
    return response.value

def get_messages(value):
    request = UInt32Value(value=value)
    response = stub.GetMessages(request)
    return response.messages

def clear_messages():
    request = Empty()
    stub.ClearMessages(request)

def drop_messages(value):
    request = UInt32Value(value=value)
    stub.DropMessages(request)
