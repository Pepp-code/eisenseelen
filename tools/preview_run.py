"""Takes preview pictures of a whole run. Started by tools/preview.ps1 -Szene lauf. Ends the game after the last picture.

Runs inside `-game`, where only the engine's runtime is up. The scenario below is a generator: it does something,
then yields how many seconds to wait. Keys go through the real controls.
"""
import json
import math
import os

import unreal

WORLD_PATH = "/Game/Maps/L_ErsterLauf.L_ErsterLauf"
SHADER_WAIT_SECONDS = 25.0
SETTLE_SECONDS = 1.5
AFTER_SHOT_SECONDS = 1.0
RESOLUTION = "1600x900"
STAND_HEIGHT = 92


def world():
    return unreal.find_object(None, WORLD_PATH)


def player():
    return unreal.GameplayStatics.get_player_controller(world(), 0)


def pawn():
    return unreal.GameplayStatics.get_player_pawn(world(), 0)


def movement():
    return pawn().get_component_by_class(unreal.CharacterMovementComponent)


def shot():
    unreal.SystemLibrary.execute_console_command(world(), f"HighResShot {RESOLUTION}")


def tap(key):
    player().preview_key(key, True)
    yield 0.05
    player().preview_key(key, False)


def stand_looking_at(location, target, height=STAND_HEIGHT):
    """Stands on the ground and looks at a point, horizontally aimed at its height."""
    pawn().set_actor_location(unreal.Vector(location[0], location[1], height), False, True)
    movement().set_movement_mode(unreal.MovementMode.MOVE_WALKING)
    yaw = math.degrees(math.atan2(target[1] - location[1], target[0] - location[0]))
    distance = math.hypot(target[0] - location[0], target[1] - location[1])
    pitch = math.degrees(math.atan2(target[2] - height, distance))
    player().set_control_rotation(unreal.Rotator(0, pitch, yaw))


def scenario():
    with open(os.path.join(unreal.Paths.project_dir(), "Data", "lauf.json"), encoding="utf-8") as file:
        hold_seconds = json.load(file)["leuchtfeuer_halten_s"]

    yield SHADER_WAIT_SECONDS
    player().preview_invulnerable()

    # On the deck, along the barge, and then down over the map.
    shot()
    yield AFTER_SHOT_SECONDS
    player().set_control_rotation(unreal.Rotator(0, -40, 0))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS

    # The jump: off the deck and down in a dive.
    player().preview_key("W", True)
    yield 3.5
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_key("W", False)
    yield 6.0

    # A beacon from afar and from close by.
    beacon = unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EIBeacon)[0].get_actor_location()
    fire = (beacon.x, beacon.y, 950)
    outward = math.hypot(beacon.x, beacon.y)
    behind_beacon = (beacon.x + beacon.x / outward * 900, beacon.y + beacon.y / outward * 900)
    stand_looking_at((beacon.x / outward * (outward - 3400), beacon.y / outward * (outward - 3400)), fire)
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    stand_looking_at((beacon.x + 300, beacon.y), (beacon.x, beacon.y, 300))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS

    # The fire burns, the zone shows, the wave comes and the barge is on its way. The hero waits behind the beacon.
    yield from tap("F")
    stand_looking_at(behind_beacon, (0, 0, -300))
    yield SETTLE_SECONDS
    shot()
    yield hold_seconds * 0.6
    barge = unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EISkyBarge)[0]
    target = barge.get_actor_location()
    stand_looking_at(behind_beacon, (target.x, target.y, target.z))
    yield SETTLE_SECONDS
    shot()
    yield hold_seconds * 0.4

    # The barge is there. In we go, and up.
    target = barge.get_actor_location()
    stand_looking_at((target.x, target.y + 450), (target.x, target.y, 150))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    yield from tap("F")
    yield 3.0
    player().set_control_rotation(unreal.Rotator(0, -35, 0))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS

    unreal.SystemLibrary.execute_console_command(world(), "Quit")
    yield 60.0


steps = scenario()
state = {"wait": 0.0}


def tick(delta_seconds):
    if world() is None:
        return
    state["wait"] -= delta_seconds
    if state["wait"] <= 0.0:
        state["wait"] = next(steps, 60.0)


unreal.register_slate_post_tick_callback(tick)
