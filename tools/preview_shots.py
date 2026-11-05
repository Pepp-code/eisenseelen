"""Takes preview pictures of the running game. Started by tools/preview.ps1. Ends the game after the last picture.

Runs inside `-game`, where only the engine's runtime is up. The scenario below is a generator: it does something,
then yields how many seconds to wait. Keys go through the real controls.
"""
import unreal

WORLD_PATH = "/Game/Maps/L_Testarena.L_Testarena"
SHADER_WAIT_SECONDS = 25.0
SETTLE_SECONDS = 1.5
AFTER_SHOT_SECONDS = 1.0
RESOLUTION = "1600x900"

# Before the dummies: hand poses are easiest to judge against something.
BEFORE_DUMMIES = (620, 1020, 100)
TEN_METRES_BEFORE_DUMMIES = (-200, 1020, 92)
AIM_AT_DUMMY_PITCH = -3.7


def world():
    return unreal.find_object(None, WORLD_PATH)


def player():
    return unreal.GameplayStatics.get_player_controller(world(), 0)


def pawn():
    return unreal.GameplayStatics.get_player_pawn(world(), 0)


def movement():
    return pawn().get_component_by_class(unreal.CharacterMovementComponent)


def fly_to(location, pitch=0, yaw=0):
    """Hangs in the air for a picture."""
    movement().set_movement_mode(unreal.MovementMode.MOVE_FLYING)
    pawn().set_actor_location(unreal.Vector(*location), False, True)
    player().set_control_rotation(unreal.Rotator(0, pitch, yaw))


def stand_at(location, pitch=0, yaw=0):
    pawn().set_actor_location(unreal.Vector(*location), False, True)
    movement().set_movement_mode(unreal.MovementMode.MOVE_WALKING)
    player().set_control_rotation(unreal.Rotator(0, pitch, yaw))


def pose(curve, position):
    pawn().get_component_by_class(unreal.EIViewModelComponent).preview_pose(curve, position)


def shot():
    unreal.SystemLibrary.execute_console_command(world(), f"HighResShot {RESOLUTION}")


def tap(key):
    player().preview_key(key, True)
    yield 0.05
    player().preview_key(key, False)


def scenario():
    yield SHADER_WAIT_SECONDS

    # The arena as the player first sees it, and from above.
    fly_to((0, 0, 100))
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    fly_to((500, -1800, 1500), -30, 45)
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS

    # The sword's poses.
    fly_to(BEFORE_DUMMIES)
    for curve, position in [
        ("None", 0.0),
        ("leicht_1", 0.2),
        ("leicht_1", 0.42),
        ("leicht_1", 0.65),
        ("schwer_laden", 1.0),
        ("schwer", 0.45),
        ("blocken", 1.0),
    ]:
        pose(curve, position)
        yield SETTLE_SECONDS
        shot()
        yield AFTER_SHOT_SECONDS
    pose("None", 0.0)

    # The arcanist, and a fireball at the dummies: flying, then bursting.
    yield from tap("C")
    stand_at(TEN_METRES_BEFORE_DUMMIES, AIM_AT_DUMMY_PITCH)
    yield SETTLE_SECONDS
    shot()
    yield AFTER_SHOT_SECONDS
    yield from tap("Q")
    yield 0.05
    shot()
    yield 0.28
    shot()
    yield AFTER_SHOT_SECONDS

    # The runes: the frost around the hero, then the field and the wall where the hero looks.
    yield from tap("E")
    yield 0.15
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_equip("Sturmrune")
    yield 0.1
    yield from tap("E")
    yield 0.6
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_equip("Flammenrune")
    yield 0.1
    yield from tap("E")
    yield 0.6
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_equip("Frostrune")

    # The other spells of the staff.
    player().preview_equip("Funkenstab")
    yield 0.2
    yield 6.5
    yield from tap("Q")
    yield 0.08
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_equip("Froststab")
    yield 0.2
    yield 6.5
    yield from tap("Q")
    yield 0.15
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_equip("Feuerstab")
    yield 0.1

    # The kinds of enemies, all in one group.
    yield from tap("C")
    stand_at((-1500, 0, 92))
    player().preview_invulnerable()
    yield 0.5

    # The gear page, with a slot full of choices and with armour on.
    player().preview_equip("Brustpanzer der Eisenwacht")
    player().preview_equip("Helm der Eisenwacht")
    yield 0.1
    yield from tap("I")
    yield 0.2
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_menu_slot(3)
    yield 0.2
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_loot_book(True)
    yield 0.2
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_loot_book(False)
    yield from tap("I")
    yield 0.1

    # How other swords lie in the hand.
    player().preview_equip("Donnerbrecher")
    yield 0.3
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_equip("Reifsäbel")
    yield 0.3
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_equip("Flammenschwert")
    yield 0.1

    player().preview_spawn_group("Kriegshaufen")
    yield 0.4
    shot()
    yield AFTER_SHOT_SECONDS
    yield from tap("R")
    yield SETTLE_SECONDS

    # The horde reaches the hero.
    stand_at((0, 0, 92))
    yield from tap("H")
    yield 6.0
    shot()
    yield 1.5
    shot()
    yield AFTER_SHOT_SECONDS

    # The block against the horde, and the runners staggering.
    player().preview_key("RightMouseButton", True)
    yield 1.5
    shot()
    yield AFTER_SHOT_SECONDS
    for enemy in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.EIEnemy):
        enemy.stagger(5.0)
    yield 0.5
    shot()
    yield AFTER_SHOT_SECONDS
    player().preview_key("RightMouseButton", False)

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
