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
    hold_seconds = run_values["signalfeuer_halten_s"]

    yield SHADER_WAIT_SECONDS

    # The dock: from the middle of it, and then right at the barge, where the use key works.
    shot()
    yield AFTER_SHOT_SECONDS
    barge = unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EISkyBarge)[0]
    spot = barge.get_actor_location()
    pawn().set_actor_location(unreal.Vector(spot.x, spot.y - 500, spot.z + STAND_HEIGHT), False, True)
    player().set_control_rotation(unreal.Rotator(0, 0, 90))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS

    # Aboard, and the barge casts off.
    yield from tap("F")
    yield 1.5
    shot()
    yield 2.0

    # The way across, in the dark, and the arrival: the team stands at the edge, and the barge leaves.
    shot()
    yield 3.0
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_invulnerable()
    yield 1.0
    shot()
    yield AFTER_SHOT_SECONDS

    # The sky on the way to the blood moon, and back for the rest of the pictures.
    stand_looking_at((0, 0), (4000, 0, 900))
    for stage in (3, 6):
        player().preview_corruption(stage)
        yield 5.0
        shot()
        yield AFTER_SHOT_SECONDS
    player().preview_corruption(0)
    yield 6.0

    # The overview of the side jobs.
    yield from tap("M")
    yield 0.2
    shot()
    yield AFTER_SHOT_SECONDS
    yield from tap("M")

    # A chest, if this run has one: sealed from outside its circle, with its guards in it, free once they have fallen, and open.
    chest = next((c for c in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EIChest) if c.is_in_use()), None)
    if chest is not None:
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

    # An altar or a shrine, if this run has one, from afar and from inside its zone.
    altar = next((a for a in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EIAltar) if a.is_active()), None)
    if altar is not None:
        spot = altar.get_actor_location()
        stand_looking_at((spot.x + 1800, spot.y + 300), (spot.x, spot.y, 200))
        yield SETTLE_SECONDS
        shot()
        yield AFTER_SHOT_SECONDS
        stand_looking_at((spot.x + 350, spot.y), (spot.x, spot.y, 200))
        yield SETTLE_SECONDS
        shot()
        yield AFTER_SHOT_SECONDS

    # A leader in its camp, if this run has one, from afar.
    leader = next((e for e in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EIEnemy) if e.is_leader()), None)
    if leader is not None:
        spot = leader.get_actor_location()
        stand_looking_at((spot.x + 1600, spot.y), (spot.x, spot.y, 150))
        yield 0.4
        shot()
        yield AFTER_SHOT_SECONDS

    # The fire from afar, and from right beside it.
    fire = next(f for f in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EISignalFire) if f.is_in_use())
    spot = fire.get_actor_location()
    outward = math.hypot(spot.x, spot.y)
    in_x, in_y = -spot.x / outward, -spot.y / outward
    stand_looking_at((spot.x + in_x * 3400, spot.y + in_y * 3400), (spot.x, spot.y, 950))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    behind = (spot.x - in_x * 300, spot.y - in_y * 300)
    stand_looking_at(behind, (spot.x, spot.y, 300))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS

    # Lit: the zone shows, the wave comes and the barge is on its way.
    yield from tap("F")
    stand_looking_at(behind, (0, 0, -300))
    yield SETTLE_SECONDS
    shot()
    yield hold_seconds * 0.6
    target = barge.get_actor_location()
    stand_looking_at(behind, (target.x, target.y, target.z))
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
