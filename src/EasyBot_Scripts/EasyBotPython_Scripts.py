#pip install grpcio protobuf grpcio-tools
#python -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. bot.proto
import time
import threading
from Bot_Functions import *



# 2874 - Mp
# 3155 - Sd
# 3160 - Uh

def useManas():
    while True:
        if 700 > get_mana(get_local_player()):
            containers = get_containers()
            for container in containers:
                items = get_items(container)
                if items:
                    itemId = get_item_id(items[0])
                    if itemId == 2874:
                        use_with(items[0], get_local_player())

                        break
        time.sleep(0.2)

def healSd():
    while True:
        if get_health(get_local_player()) < 200:
            containers = get_containers()
            for container in containers:
                items = get_items(container)
                if items:
                    itemId = get_item_id(items[0])
                    if itemId == 3160:
                        use_with(items[0], get_local_player())

                        break
        elif get_attacking_creature():
            containers = get_containers()
            for container in containers:
                items = get_items(container)
                if items:
                    itemId = get_item_id(items[0])
                    if itemId == 3155:
                        creature = get_attacking_creature()
                        if creature:
                            tile = get_tile(get_position(creature))
                            thing = get_top_use_thing(tile)
                            use_with(items[0], thing)
                            break
        time.sleep(0.1)


'''
t1 = threading.Thread(target=useManas)
t2 = threading.Thread(target=healSd)
t1.start()
t2.start()

t1.join()
t2.join()
'''
print(get_character_name())


