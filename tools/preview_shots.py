"""Takes preview pictures of the running game. Started by tools/preview.ps1. Ends the game after the last picture.

Runs inside `-game`, where only the engine's runtime is up, so it steers the player directly. The first shot waits
until shaders had time to compile.
"""
import unreal

WORLD_PATH = "/Game/Maps/L_Testarena.L_Testarena"
SHADER_WAIT_SECONDS = 25.0
SECONDS_BETWEEN_SHOTS = 3.0
SETTLE_SECONDS = 1.5
RESOLUTION = "1600x900"

BEFORE_DUMMIES = (620, 1020, 100)

# location in cm, (pitch, yaw) in degrees, and optionally a hand pose as (curve, position from 0 to 1)
VIEWS = [
    ((0, 0, 100), (0, 0), None),
    ((500, -1800, 1500), (-30, 45), None),
    (BEFORE_DUMMIES, (0, 0), None),
    (BEFORE_DUMMIES, (0, 0), ("leicht_1", 0.2)),
    (BEFORE_DUMMIES, (0, 0), ("leicht_1", 0.42)),
    (BEFORE_DUMMIES, (0, 0), ("leicht_1", 0.65)),
    (BEFORE_DUMMIES, (0, 0), ("leicht_3", 0.42)),
    (BEFORE_DUMMIES, (0, 0), ("schwer_laden", 1.0)),
    (BEFORE_DUMMIES, (0, 0), ("schwer", 0.45)),
]

state = {"elapsed": 0.0, "next_view": 0, "moved": False}


def tick(delta_seconds):
    state["elapsed"] += delta_seconds
    world = unreal.find_object(None, WORLD_PATH)
    if world is None:
        return
    index = state["next_view"]
    due = SHADER_WAIT_SECONDS + index * SECONDS_BETWEEN_SHOTS
    if index >= len(VIEWS):
        if state["elapsed"] >= due:
            unreal.SystemLibrary.execute_console_command(world, "Quit")
        return

    if state["elapsed"] >= due - SETTLE_SECONDS and not state["moved"]:
        location, (pitch, yaw), pose = VIEWS[index]
        pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
        pawn.get_component_by_class(unreal.CharacterMovementComponent).set_movement_mode(unreal.MovementMode.MOVE_FLYING)
        pawn.set_actor_location(unreal.Vector(*location), False, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(unreal.Rotator(0, pitch, yaw))
        curve, position = pose if pose else ("None", 0.0)
        pawn.get_component_by_class(unreal.EIViewModelComponent).preview_pose(curve, position)
        state["moved"] = True
    if state["elapsed"] >= due:
        unreal.SystemLibrary.execute_console_command(world, f"HighResShot {RESOLUTION}")
        state["next_view"] = index + 1
        state["moved"] = False


unreal.register_slate_post_tick_callback(tick)
