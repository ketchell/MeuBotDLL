import grpc
import bot_pb2
import bot_pb2_grpc
import time # Added for action delays
import random # Added for humanized timing

from google.protobuf.wrappers_pb2 import UInt64Value, Int32Value, UInt32Value, StringValue, BoolValue
from google.protobuf.empty_pb2 import Empty
import inspect

# ... [Keep your SERVER_ADDRESS and stub setup at the top] ...

# ============================================================================
# NEW: Action Synchronization (The "isBusy" Fix)
# ============================================================================
def wait_until_ready(timeout=5.0):
    """
    Blocks the script execution until the player is no longer 'Busy'.
    Ensures that deposit/withdraw/loot actions finish before walking.
    """
    start_time = time.time()
    while time.time() - start_time < timeout:
        if not is_busy_status():
            return True
        # Human-like polling interval
        time.sleep(random.uniform(0.3, 0.6))
    return False

def is_busy_status():
    """Checks the DLL to see if the player icon/state is 'Busy'."""
    try:
        # We call the IsBusy method we added to the Proto/DLL
        request = Empty()
        # Note: Ensure 'IsBusy' is defined in your bot.proto file!
        response = stub.CanPerformGameAction(request) 
        return not response.value # If can't perform action, it is busy
    except:
        return False

# ============================================================================
# UPDATED: Humanized Movement & Action Logic
# ============================================================================

def walk(direction):
    # Add a tiny 'reaction' delay before sending the walk command
    time.sleep(random.uniform(0.01, 0.05))
    request = Int32Value(value=direction)
    stub.Walk(request)

def move(thing, topos, count):
    """Updated move with post-action synchronization"""
    request = bot_pb2.bot_MoveRequest()
    request.thing = thing
    request.count = count
    request.toPos.x = topos['x']
    request.toPos.y = topos['y']
    request.toPos.z = topos['z']
    stub.Move(request)
    
    # After moving an item, wait a moment to simulate human drag speed
    time.sleep(random.uniform(0.15, 0.3))

def execute_deposit_logic():
    """
    Example of how to use the new sync logic in your deposit scripts.
    """
    # ... your deposit code here ...
    
    # CRITICAL: Call this after your deposit loop to ensure 
    # the walker doesn't jump to the next waypoint immediately.
    wait_until_ready()

# ... [Keep all your other get/set functions below] ...

def attack(creature, cancel):
    # The DLL now handles the 'Lock', but we can add a tiny delay here too
    if not cancel:
        time.sleep(random.uniform(0.1, 0.2))
    
    request = bot_pb2.bot_AttackRequest()
    request.creature = creature
    request.cancel = cancel
    stub.Attack(request)

# ... [Rest of your file] ...