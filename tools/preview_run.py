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
        run_values = json.load(file)
    with open(os.path.join(unreal.Paths.project_dir(), "Data", "laufkarte.json"), encoding="utf-8") as file:
        open_space = json.load(file)["freiflaechen"][0]
    hold_seconds = run_values["fackel_halten_s"]

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

    # The sky on the way to the blood moon, and back for the rest of the pictures.
    stand_looking_at((0, 0), (4000, 0, 900))
    for stage in (3, 6):
        player().preview_corruption(stage)
        yield 5.0
        shot()
        yield AFTER_SHOT_SECONDS
    player().preview_corruption(0)
    yield 6.0

    # The overview of the contracts, down on the ground.
    yield from tap("M")
    yield 0.2
    shot()
    yield AFTER_SHOT_SECONDS
    yield from tap("M")

    # A chest: sealed from outside its circle, with its guards in it, free once they have fallen, and open.
    chest = next(c for c in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EIChest) if c.is_in_use())
    spot = chest.get_actor_location()
    front = chest.get_actor_forward_vector()
    stand_looking_at((spot.x + 1500 * front.x, spot.y + 1500 * front.y), (spot.x, spot.y, 100))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    stand_looking_at((spot.x + 900 * front.x, spot.y + 900 * front.y), (spot.x, spot.y, 100))
    yield 0.6
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_defeat_guards()
    stand_looking_at((spot.x + 220 * front.x, spot.y + 220 * front.y), (spot.x, spot.y, 60))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    yield from tap("F")
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS

    # An altar or a shrine of a contract, from afar and from inside its zone.
    altar = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EIAltar) if a.is_active())
    spot = altar.get_actor_location()
    stand_looking_at((spot.x + 1800, spot.y + 300), (spot.x, spot.y, 200))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    stand_looking_at((spot.x + 350, spot.y), (spot.x, spot.y, 200))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS

    # A leader in its camp, from afar.
    leader = next(e for e in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EIEnemy) if e.is_leader())
    spot = leader.get_actor_location()
    stand_looking_at((spot.x + 1600, spot.y), (spot.x, spot.y, 150))
    yield 0.4
    shot()
    yield AFTER_SHOT_SECONDS

    # Enough contracts: the note about the flare, with the overview at the place where it will fly from.
    player().preview_skip_contracts(4)
    open_x, open_y = open_space["x_m"] * 100, open_space["y_m"] * 100
    stand_looking_at((open_x, open_y), (0, 0, 300))
    yield from tap("M")
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    yield from tap("M")

    # The flare climbs, and then the circle shows, the wave comes and the barge is on its way.
    yield from tap("G")
    stand_looking_at((open_x + 400, open_y + 400), (open_x, open_y, 900))
    yield 0.5
    shot()
    yield AFTER_SHOT_SECONDS
    stand_looking_at((open_x + 400, open_y + 400), (open_x, open_y, 2800))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    stand_looking_at((open_x + 400, open_y + 400), (0, 0, -300))
    yield SETTLE_SECONDS
    shot()
    yield hold_seconds * 0.6
    barge = unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EISkyBarge)[0]
    target = barge.get_actor_location()
    stand_looking_at((open_x + 400, open_y + 400), (target.x, target.y, target.z))
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

    # After the flight the gear page shows what stays.
    yield from tap("I")
    yield 0.2
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
