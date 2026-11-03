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
# High enough that nobody leaves the map during the jump from the barge.
EDGE_BLOCKER_HEIGHT_M = 300
TOWER_SIZE_M = 4
TOWER_DISTANCE_M = 3.5
PLACE_WALL_LENGTH_M = 10
PLACE_WALL_THICKNESS_M = 1
PLACE_WALL_HEIGHT_M = 3
PLACE_SIZE_M = 7
# Just above the deck for a capsule with a half height of 88 cm.
START_ABOVE_DECK_CM = 120
COVER_CLEAR_M = 22
COVER_MIN_SIZE_M = 2
COVER_MAX_SIZE_M = 5
COVER_MAX_HEIGHT_M = 3
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


def add_edge(actors, floor_size, height):
    """A low visible wall around the map, and an invisible one above it."""
    half = floor_size / 2
    sides = [
        ((0, half), (floor_size + EDGE_THICKNESS_M, EDGE_THICKNESS_M)),
        ((0, -half), (floor_size + EDGE_THICKNESS_M, EDGE_THICKNESS_M)),
        ((half, 0), (EDGE_THICKNESS_M, floor_size + EDGE_THICKNESS_M)),
        ((-half, 0), (EDGE_THICKNESS_M, floor_size + EDGE_THICKNESS_M)),
    ]
    for (x, y), (size_x, size_y) in sides:
        add_box(actors, "Rand", (x, y, height / 2), (size_x, size_y, height))
        blocker = add_box(actors, "Randsperre", (x, y, EDGE_BLOCKER_HEIGHT_M / 2), (size_x, size_y, EDGE_BLOCKER_HEIGHT_M))
        blocker.set_actor_hidden_in_game(True)


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
    """Loose blocks that give cover and break the view. They keep clear of the places and the beacons."""
    rng = random.Random(layout["deckung_zufall"])
    half = layout["boden_m"] / 2 - COVER_CLEAR_M
    taken = [(p["x_m"], p["y_m"]) for p in layout["orte"] + layout["leuchtfeuer"] + layout["sprungfelder"]]
    placed = 0
    while placed < layout["deckung_anzahl"]:
        x, y = rng.uniform(-half, half), rng.uniform(-half, half)
        if any(math.hypot(x - other_x, y - other_y) < COVER_CLEAR_M for other_x, other_y in taken):
            continue
        size_x, size_y = rng.uniform(COVER_MIN_SIZE_M, COVER_MAX_SIZE_M), rng.uniform(COVER_MIN_SIZE_M, COVER_MAX_SIZE_M)
        height = rng.uniform(1, COVER_MAX_HEIGHT_M)
        add_box(actors, "Deckung", (x, y, height / 2), (size_x, size_y, height), rotation=(0, 0, rng.uniform(0, 90)))
        taken.append((x, y))
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
    add_edge(actors, floor_size, layout["rand_hoehe_m"])
    altar_class = unreal.load_class(None, "/Script/Eisenseelen.EIAltar")
    for index, place in enumerate(layout["orte"]):
        add_place(actors, place, index)
        spawn(actors, altar_class, location=(place["x_m"] * CM_PER_M, place["y_m"] * CM_PER_M, 0), label="Altar")
    add_cover(actors, layout)
    add_launch_pads(actors, layout["sprungfelder"])

    navigation = spawn(actors, unreal.NavMeshBoundsVolume, label="Wegfindung")
    scale = floor_size / VOLUME_BRUSH_M
    navigation.set_actor_scale3d(unreal.Vector(scale, scale, NAVIGATION_HEIGHT_M / VOLUME_BRUSH_M))

    beacon_class = unreal.load_class(None, "/Script/Eisenseelen.EIBeacon")
    for beacon in layout["leuchtfeuer"]:
        spawn(actors, beacon_class, location=(beacon["x_m"] * CM_PER_M, beacon["y_m"] * CM_PER_M, 0), label="Leuchtfeuer")

    barge = layout["barke"]
    barge_location = [barge["x_m"] * CM_PER_M, barge["y_m"] * CM_PER_M, barge["hoehe_m"] * CM_PER_M]
    spawn(actors, unreal.load_class(None, "/Script/Eisenseelen.EISkyBarge"), location=barge_location, label="Himmelsbarke")
    start = (barge_location[0], barge_location[1], barge_location[2] + START_ABOVE_DECK_CM)
    spawn(actors, unreal.PlayerStart, location=start, label="Start")

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
