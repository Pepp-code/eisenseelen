"""Takes preview pictures of the running game. Started by tools/preview.ps1, which also ends the game.

Runs inside `-game`, where only the engine's runtime is up, so it steers the player directly. The first shot waits
until shaders had time to compile.
"""
import unreal

WORLD_PATH = "/Game/Maps/L_Testarena.L_Testarena"
SHADER_WAIT_SECONDS = 25.0
SECONDS_BETWEEN_SHOTS = 4.0
RESOLUTION = "1600x900"

# (x, y, z in cm), (pitch, yaw in degrees)
VIEWS = [
    ((0, 0, 100), (0, 0)),
    ((500, -1800, 1500), (-30, 45)),
    ((1800, 1800, 500), (-10, -10)),
]

state = {"elapsed": 0.0, "next_view": 0, "shot_taken": False}


def tick(delta_seconds):
    state["elapsed"] += delta_seconds
    index = state["next_view"]
    due = SHADER_WAIT_SECONDS + index * SECONDS_BETWEEN_SHOTS
    world = unreal.find_object(None, WORLD_PATH)
    if world is None:
        return
    if index >= len(VIEWS):
        # End the game from inside too, so it can never keep running when the starting script dies.
        if state["elapsed"] >= due:
            unreal.SystemLibrary.execute_console_command(world, "Quit")
        return
    if state["elapsed"] >= due - SECONDS_BETWEEN_SHOTS / 2 and not state["shot_taken"]:
        # Move first and shoot two seconds later, so the frame has settled.
        location, (pitch, yaw) = VIEWS[index]
        pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
        pawn.set_actor_location(unreal.Vector(*location), False, True)
        unreal.GameplayStatics.get_player_controller(world, 0).set_control_rotation(unreal.Rotator(0, pitch, yaw))
        pawn.get_component_by_class(unreal.CharacterMovementComponent).set_movement_mode(unreal.MovementMode.MOVE_FLYING)
        state["shot_taken"] = True
    if state["elapsed"] >= due:
        unreal.SystemLibrary.execute_console_command(world, f"HighResShot {RESOLUTION}")
        state["next_view"] = index + 1
        state["shot_taken"] = False


unreal.register_slate_post_tick_callback(tick)
