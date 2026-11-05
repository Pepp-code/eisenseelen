"""Builds the grey map of a whole run, /Game/Maps/L_ErsterLauf, from Data/laufkarte.json.

Run without an editor window:
    C:\\Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe Eisenseelen.uproject
        -run=pythonscript -script=tools/make_runmap.py -unattended -nullrhi
"""
import json
import math
import os
import random

import unreal

MAP_PATH = "/Game/Maps/L_ErsterLauf"
MATERIAL_PATH = "/Game/Materials/M_Schachbrett"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"

CM_PER_M = 100
EDGE_THICKNESS_M = 2
# High enough that nobody leaves the map from a launch pad.
EDGE_BLOCKER_HEIGHT_M = 300
DOCK_RAILING_HEIGHT_M = 1
DOCK_BLOCKER_HEIGHT_M = 20
DOCK_RAILING_THICKNESS_M = 0.5
BARGE_HALF_WIDTH_M = 3.5
PLAYER_STARTS = 4
PLAYER_START_SPACING_M = 3
PLAYER_START_ABOVE_FLOOR_CM = 120
TOWER_SIZE_M = 4
TOWER_DISTANCE_M = 3.5
PLACE_WALL_LENGTH_M = 10
PLACE_WALL_THICKNESS_M = 1
PLACE_WALL_HEIGHT_M = 3
PLACE_SIZE_M = 7
COVER_CLEAR_M = 22
# A boss needs room to fight in.
BOSS_CLEAR_M = 30
COVER_MIN_SIZE_M = 2
COVER_MAX_SIZE_M = 5
COVER_MAX_HEIGHT_M = 3
# A chest needs room for the fight around it, away from the altars and fires, where the fights already are.
CHEST_CLEAR_EDGE_M = 16
CHEST_CLEAR_PLACE_M = 32
CHEST_CLEAR_PAD_M = 18
CHEST_CLEAR_COVER_M = 15
CHEST_CLEAR_CAMP_M = 26
CHEST_CLEAR_CHEST_M = 35
# A new volume's brush is a cube of this size. Paths are only needed near the floor.
VOLUME_BRUSH_M = 2
NAVIGATION_HEIGHT_M = 12


def spawn(actors, actor_class, location=(0, 0, 0), rotation=(0, 0, 0), label=""):
    actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(*location), unreal.Rotator(*rotation))
    if actor is None:
        raise RuntimeError(f"could not spawn {actor_class}")
    actor.set_actor_label(label or actor_class.__name__)
    return actor


def add_box(actors, label, center_m, size_m, rotation=(0, 0, 0)):
    """The engine's cube is 1 m wide, so its scale is its size in metres."""
    location = [value * CM_PER_M for value in center_m]
    box = spawn(actors, unreal.StaticMeshActor, location=location, rotation=rotation, label=label)
    box.static_mesh_component.set_static_mesh(unreal.load_asset(CUBE_MESH))
    box.static_mesh_component.set_material(0, unreal.load_asset(MATERIAL_PATH))
    box.set_actor_scale3d(unreal.Vector(*size_m))
    return box


def add_walls(actors, label, center, length, width, floor_z, height, thickness, blocker_height):
    """A low visible wall around a rectangle, and an invisible one above it."""
    center_x, center_y = center
    half_x, half_y = length / 2, width / 2
    sides = [
        ((center_x, center_y + half_y), (length + thickness, thickness)),
        ((center_x, center_y - half_y), (length + thickness, thickness)),
        ((center_x + half_x, center_y), (thickness, width + thickness)),
        ((center_x - half_x, center_y), (thickness, width + thickness)),
    ]
    for (x, y), (size_x, size_y) in sides:
        add_box(actors, label, (x, y, floor_z + height / 2), (size_x, size_y, height))
        blocker = add_box(actors, f"{label}sperre", (x, y, floor_z + blocker_height / 2), (size_x, size_y, blocker_height))
        blocker.set_actor_hidden_in_game(True)


def add_dock(actors, dock):
    """The platform above the clouds where a run starts, with the barge moored at its long side."""
    x, y, height = dock["x_m"], dock["y_m"], dock["hoehe_m"]
    length, width = dock["laenge_m"], dock["breite_m"]
    add_box(actors, "Anlegestelle", (x, y, height - 0.5), (length, width, 1))
    add_walls(actors, "Geländer", (x, y), length, width, height, DOCK_RAILING_HEIGHT_M, DOCK_RAILING_THICKNESS_M, DOCK_BLOCKER_HEIGHT_M)
    barge_location = (x * CM_PER_M, (y + width / 2 + BARGE_HALF_WIDTH_M) * CM_PER_M, height * CM_PER_M)
    spawn(actors, unreal.load_class(None, "/Script/Eisenseelen.EISkyBarge"), location=barge_location, label="Himmelsbarke")
    for index in range(PLAYER_STARTS):
        start_x = x + (index - (PLAYER_STARTS - 1) / 2) * PLAYER_START_SPACING_M
        location = (start_x * CM_PER_M, y * CM_PER_M, height * CM_PER_M + PLAYER_START_ABOVE_FLOOR_CM)
        spawn(actors, unreal.PlayerStart, location=location, rotation=(0, 0, 90), label="Start")


def add_place(actors, place, index):
    """A grey ruin: three walls with gaps, turned differently at every place so they do not look stamped."""
    x, y = place["x_m"], place["y_m"]
    yaw = math.radians(index * 53)
    walls = [(0, PLACE_SIZE_M, 0), (PLACE_SIZE_M, 0, 90), (-PLACE_SIZE_M, -2, 90)]
    for offset_x, offset_y, wall_yaw in walls:
        center_x = x + offset_x * math.cos(yaw) - offset_y * math.sin(yaw)
        center_y = y + offset_x * math.sin(yaw) + offset_y * math.cos(yaw)
        size = (PLACE_WALL_LENGTH_M, PLACE_WALL_THICKNESS_M, PLACE_WALL_HEIGHT_M)
        add_box(actors, f"Ort {index + 1}", (center_x, center_y, PLACE_WALL_HEIGHT_M / 2), size, rotation=(0, 0, math.degrees(yaw) + wall_yaw))


def add_cover(actors, layout):
    """Loose blocks that give cover and break the view. They keep clear of the places, the fires and the middle of the map."""
    rng = random.Random(layout["deckung_zufall"])
    half = layout["boden_m"] / 2 - COVER_CLEAR_M
    taken = [(p["x_m"], p["y_m"]) for p in layout["orte"] + layout["signalfeuer"] + layout["ankunft"] + layout["sprungfelder"]]
    taken.append((0, 0))
    bosses = [(p["x_m"], p["y_m"]) for p in layout["bossplaetze"]]
    placed = 0
    while placed < layout["deckung_anzahl"]:
        x, y = rng.uniform(-half, half), rng.uniform(-half, half)
        if any(math.hypot(x - other_x, y - other_y) < COVER_CLEAR_M for other_x, other_y in taken):
            continue
        if any(math.hypot(x - other_x, y - other_y) < BOSS_CLEAR_M for other_x, other_y in bosses):
            continue
        size_x, size_y = rng.uniform(COVER_MIN_SIZE_M, COVER_MAX_SIZE_M), rng.uniform(COVER_MIN_SIZE_M, COVER_MAX_SIZE_M)
        height = rng.uniform(1, COVER_MAX_HEIGHT_M)
        add_box(actors, "Deckung", (x, y, height / 2), (size_x, size_y, height), rotation=(0, 0, rng.uniform(0, 90)))
        taken.append((x, y))
        placed += 1
    return taken


def add_camps(actors, layout):
    """The fixed places where enemies stand watch. The game fills them when a run begins."""
    camp_class = unreal.load_class(None, "/Script/Eisenseelen.EIEnemyCamp")
    for camp in layout["lager"]:
        actor = spawn(actors, camp_class, location=(camp["x_m"] * CM_PER_M, camp["y_m"] * CM_PER_M, 0), label="Lager")
        actor.set_editor_property("start_size", camp["groesse"])


def add_chests(actors, layout, cover):
    """Out in the open, each with room for its fight. A run only uses some of them."""
    chest_class = unreal.load_class(None, "/Script/Eisenseelen.EIChest")
    # A random generator of its own, so that more chests do not move the cover around.
    rng = random.Random(layout["deckung_zufall"] + 1)
    half = layout["boden_m"] / 2 - CHEST_CLEAR_EDGE_M
    keep_clear = [(point, CHEST_CLEAR_COVER_M) for point in cover]
    keep_clear += [((p["x_m"], p["y_m"]), CHEST_CLEAR_PLACE_M) for p in layout["orte"] + layout["signalfeuer"] + layout["ankunft"]]
    keep_clear += [((p["x_m"], p["y_m"]), CHEST_CLEAR_PAD_M) for p in layout["sprungfelder"]]
    keep_clear += [((p["x_m"], p["y_m"]), BOSS_CLEAR_M) for p in layout["bossplaetze"]]
    keep_clear += [((p["x_m"], p["y_m"]), CHEST_CLEAR_CAMP_M) for p in layout["lager"]]
    keep_clear.append(((0, 0), CHEST_CLEAR_PLACE_M))
    placed = 0
    while placed < layout["truhen"]:
        x, y = rng.uniform(-half, half), rng.uniform(-half, half)
        if any(math.hypot(x - other_x, y - other_y) < clear for (other_x, other_y), clear in keep_clear):
            continue
        spawn(actors, chest_class, location=(x * CM_PER_M, y * CM_PER_M, 0), rotation=(0, 0, rng.uniform(0, 360)), label="Truhe")
        keep_clear.append(((x, y), CHEST_CLEAR_CHEST_M))
        placed += 1


def add_launch_pads(actors, pads):
    pad_class = unreal.load_class(None, "/Script/Eisenseelen.EILaunchPad")
    for pad in pads:
        x, y, tower_height = pad["x_m"], pad["y_m"], pad["turm_hoehe_m"]
        spawn(actors, pad_class, location=(x * CM_PER_M, y * CM_PER_M, 0), label="Sprungfeld")
        add_box(actors, "Turm", (x + TOWER_DISTANCE_M, y, tower_height / 2), (TOWER_SIZE_M, TOWER_SIZE_M, tower_height))


def main():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    with open(os.path.join(unreal.Paths.project_dir(), "Data", "laufkarte.json"), encoding="utf-8") as file:
        layout = json.load(file)

    # new_level refuses a map the editor already knows, so a rebuild empties the existing one instead.
    map_file = os.path.join(unreal.Paths.project_content_dir(), "Maps", "L_ErsterLauf.umap")
    if os.path.exists(map_file):
        if not levels.load_level(MAP_PATH):
            raise RuntimeError(f"could not load {MAP_PATH}")
        for actor in actors.get_all_level_actors():
            actors.destroy_actor(actor)
    elif not levels.new_level(MAP_PATH):
        raise RuntimeError(f"could not create {MAP_PATH}")

    floor_size = layout["boden_m"]
    add_box(actors, "Boden", (0, 0, -0.5), (floor_size, floor_size, 1))
    add_walls(actors, "Rand", (0, 0), floor_size, floor_size, 0, layout["rand_hoehe_m"], EDGE_THICKNESS_M, EDGE_BLOCKER_HEIGHT_M)
    altar_class = unreal.load_class(None, "/Script/Eisenseelen.EIAltar")
    for index, place in enumerate(layout["orte"]):
        add_place(actors, place, index)
        spawn(actors, altar_class, location=(place["x_m"] * CM_PER_M, place["y_m"] * CM_PER_M, 0), label="Altar")
    taken = add_cover(actors, layout)
    add_camps(actors, layout)
    add_chests(actors, layout, taken)
    add_launch_pads(actors, layout["sprungfelder"])

    navigation = spawn(actors, unreal.NavMeshBoundsVolume, label="Wegfindung")
    scale = floor_size / VOLUME_BRUSH_M
    navigation.set_actor_scale3d(unreal.Vector(scale, scale, NAVIGATION_HEIGHT_M / VOLUME_BRUSH_M))

    fire_class = unreal.load_class(None, "/Script/Eisenseelen.EISignalFire")
    for fire in layout["signalfeuer"]:
        spawn(actors, fire_class, location=(fire["x_m"] * CM_PER_M, fire["y_m"] * CM_PER_M, 0), label="Signalfeuer")

    add_dock(actors, layout["anlegestelle"])

    sun = spawn(actors, unreal.DirectionalLight, location=(0, 0, 1000), rotation=(0, -50, 30), label="Sonne")
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    # The corruption changes the light during a run.
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    spawn(actors, unreal.SkyAtmosphere, label="Himmel")
    sky_light = spawn(actors, unreal.SkyLight, location=(0, 0, 1000), label="Himmelslicht")
    sky_light.light_component.set_editor_property("real_time_capture", True)
    spawn(actors, unreal.ExponentialHeightFog, label="Nebel")

    if not levels.save_current_level():
        raise RuntimeError(f"could not save {MAP_PATH}")
    unreal.log(f"make_runmap: {MAP_PATH} saved")


main()
