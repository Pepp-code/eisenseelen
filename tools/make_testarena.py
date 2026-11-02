"""Builds the grey test arena /Game/Maps/L_Testarena from Data/testarena.json.

Run without an editor window:
    C:\\Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe Eisenseelen.uproject
        -run=pythonscript -script=tools/make_testarena.py -unattended -nullrhi
"""
import json
import math
import os

import unreal

MAP_PATH = "/Game/Maps/L_Testarena"
MATERIAL_PATH = "/Game/Materials/M_Schachbrett"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"

CM_PER_M = 100
PLANK_THICKNESS_M = 0.5
RUN_UP_M = 3
LABEL_HEIGHT_M = 3.8
LABEL_SIZE_CM = 90
# Just above the floor for a capsule with a half height of 88 cm.
MARKER_HEIGHT_CM = 92
TOWER_SIZE_M = 4
TOWER_DISTANCE_M = 3.5


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


def add_label(actors, text, x_m, y_m):
    """A sign above the object. Text faces +X, so it turns around to face the start."""
    location = (x_m * CM_PER_M, y_m * CM_PER_M, LABEL_HEIGHT_M * CM_PER_M)
    sign = spawn(actors, unreal.TextRenderActor, location=location, rotation=(0, 0, 180), label=f"Schild {text}")
    component = sign.get_component_by_class(unreal.TextRenderComponent)
    component.set_text(text)
    component.set_world_size(LABEL_SIZE_CM)
    component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)


def add_course_marker(actors, kind, x_m, y_m, height_m, extra_tags=()):
    """Tells the test where a course starts and what to expect. Tag numbers are centimetres or degrees."""
    location = ((x_m - RUN_UP_M) * CM_PER_M, y_m * CM_PER_M, MARKER_HEIGHT_CM)
    marker = spawn(actors, unreal.TargetPoint, location=location, label=f"Parcours {kind} {height_m}")
    tags = ["Parcours", kind, f"Hoehe={round(height_m * CM_PER_M)}", f"Anlauf={RUN_UP_M * CM_PER_M}", *extra_tags]
    marker.set_editor_property("tags", [unreal.Name(tag) for tag in tags])


def add_ramp(actors, ramps, angle_deg, front_x, y):
    """A plank from the floor up to a platform. The plank's top surface starts at the floor at front_x."""
    height = ramps["hoehe_m"]
    width = ramps["breite_m"]
    depth = ramps["plattform_tiefe_m"]
    angle = math.radians(angle_deg)
    run = height / math.tan(angle)
    center = (
        front_x + run / 2 + PLANK_THICKNESS_M / 2 * math.sin(angle),
        y,
        height / 2 - PLANK_THICKNESS_M / 2 * math.cos(angle),
    )
    size = (height / math.sin(angle), width, PLANK_THICKNESS_M)
    add_box(actors, f"Rampe {angle_deg}", center, size, rotation=(0, angle_deg, 0))
    add_box(actors, f"Plattform {angle_deg}", (front_x + run + depth / 2, y, height / 2), (depth, width, height))
    add_label(actors, f"{angle_deg}°", front_x, y)
    add_course_marker(actors, "Rampe", front_x, y, height, [f"Winkel={angle_deg}"])


def add_jump_block(actors, blocks, height, front_x, y):
    depth = blocks["tiefe_m"]
    add_box(actors, f"Klotz {height}", (front_x + depth / 2, y, height / 2), (depth, blocks["breite_m"], height))
    add_label(actors, f"{height:g} m".replace(".", ","), front_x, y)
    add_course_marker(actors, "Klotz", front_x, y, height)


def add_launch_pads(actors, pads):
    """Launch pads, each with a tower that can be reached from it."""
    pad_class = unreal.load_class(None, "/Script/Eisenseelen.EILaunchPad")
    for pad in pads:
        x, y, tower_height = pad["x_m"], pad["y_m"], pad["turm_hoehe_m"]
        spawn(actors, pad_class, location=(x * CM_PER_M, y * CM_PER_M, 0), label="Sprungfeld")
        tower_x = x + TOWER_DISTANCE_M
        add_box(actors, "Turm", (tower_x, y, tower_height / 2), (TOWER_SIZE_M, TOWER_SIZE_M, tower_height))
        add_label(actors, "Sprungfeld", x, y)


def add_courses(actors, layout):
    """Lays all objects in one row in front of the start, ramps on the left and blocks on the right."""
    ramps = layout["rampen"]
    blocks = layout["sprungkloetze"]
    gap = layout["luecke_zwischen_objekten_m"]
    front_x = layout["abstand_vom_start_m"]
    objects = [(ramps["breite_m"], add_ramp, ramps, angle) for angle in ramps["winkel_grad"]]
    objects += [(blocks["breite_m"], add_jump_block, blocks, height) for height in blocks["hoehen_m"]]

    left_edge = -(sum(width for width, *_ in objects) + gap * (len(objects) - 1)) / 2
    for width, add_object, settings, value in objects:
        add_object(actors, settings, value, front_x, left_edge + width / 2)
        left_edge += width + gap


def main():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    with open(os.path.join(unreal.Paths.project_dir(), "Data", "testarena.json"), encoding="utf-8") as file:
        layout = json.load(file)

    # new_level refuses a map the editor already knows, so a rebuild empties the existing one instead.
    map_file = os.path.join(unreal.Paths.project_content_dir(), "Maps", "L_Testarena.umap")
    if os.path.exists(map_file):
        if not levels.load_level(MAP_PATH):
            raise RuntimeError(f"could not load {MAP_PATH}")
        for actor in actors.get_all_level_actors():
            actors.destroy_actor(actor)
    elif not levels.new_level(MAP_PATH):
        raise RuntimeError(f"could not create {MAP_PATH}")

    # The floor's top must sit at z = 0, the course test measures heights from there.
    floor_size = layout["boden_m"]
    add_box(actors, "Boden", (0, 0, -0.5), (floor_size, floor_size, 1))
    add_courses(actors, layout)
    add_launch_pads(actors, layout["sprungfelder"])

    sun = spawn(actors, unreal.DirectionalLight, location=(0, 0, 1000), rotation=(0, -50, 30), label="Sonne")
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    spawn(actors, unreal.SkyAtmosphere, label="Himmel")
    sky_light = spawn(actors, unreal.SkyLight, location=(0, 0, 1000), label="Himmelslicht")
    sky_light.light_component.set_editor_property("real_time_capture", True)
    spawn(actors, unreal.ExponentialHeightFog, label="Nebel")
    spawn(actors, unreal.PlayerStart, location=(0, 0, 100), label="Start")

    if not levels.save_current_level():
        raise RuntimeError(f"could not save {MAP_PATH}")
    unreal.log(f"make_testarena: {MAP_PATH} saved")


main()
