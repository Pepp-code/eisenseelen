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
MATERIAL_FOLDER = "/Game/Materials"
MATERIAL_NAME = "M_Schachbrett"
MATERIAL_PATH = f"{MATERIAL_FOLDER}/{MATERIAL_NAME}"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"

CHECKER_CELL_CM = 100
# Surfaces lie on whole and half metres. This shift keeps them off the cell borders, where the pattern would flicker.
CHECKER_SHIFT_CELLS = 0.13
CHECKER_LIGHT = 0.42
CHECKER_DARK = 0.30
CHECKER_ROUGHNESS = 0.85

CM_PER_M = 100
PLANK_THICKNESS_M = 0.5
RUN_UP_M = 3
LABEL_HEIGHT_M = 3.8
LABEL_SIZE_CM = 90
# Just above the floor for a capsule with a half height of 88 cm.
MARKER_HEIGHT_CM = 92


def spawn(actors, actor_class, location=(0, 0, 0), rotation=(0, 0, 0), label=""):
    actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(*location), unreal.Rotator(*rotation))
    if actor is None:
        raise RuntimeError(f"could not spawn {actor_class}")
    actor.set_actor_label(label or actor_class.__name__)
    return actor


def make_checker_material():
    """A checker fixed in the world: it does not stretch on scaled cubes, and its 1 m cells show size and speed."""
    library = unreal.MaterialEditingLibrary
    material_file = os.path.join(unreal.Paths.project_content_dir(), "Materials", f"{MATERIAL_NAME}.uasset")
    if os.path.exists(material_file):
        material = unreal.load_asset(MATERIAL_PATH)
        library.delete_all_material_expressions(material)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = tools.create_asset(MATERIAL_NAME, MATERIAL_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError(f"could not create {MATERIAL_PATH}")

    def node(expression_class, *inputs, **properties):
        """Makes a material node. Inputs are (input name, source node) pairs."""
        expression = library.create_material_expression(material, expression_class)
        for key, value in properties.items():
            expression.set_editor_property(key, value)
        for input_name, source in inputs:
            library.connect_material_expressions(source, "", expression, input_name)
        return expression

    def number(value):
        return node(unreal.MaterialExpressionConstant, r=value)

    def grey(value):
        return node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(value, value, value, 1))

    cells = node(
        unreal.MaterialExpressionDivide,
        ("A", node(unreal.MaterialExpressionWorldPosition)),
        ("B", number(CHECKER_CELL_CM)),
    )
    shifted = node(unreal.MaterialExpressionAdd, ("A", cells), ("B", number(CHECKER_SHIFT_CELLS)))
    whole_cells = node(unreal.MaterialExpressionFloor, ("", shifted))
    # The sum of the cell numbers alternates between even and odd from one cell to the next, on every axis.
    cell_sum = node(unreal.MaterialExpressionDotProduct, ("A", whole_cells), ("B", grey(1)))
    positive_sum = node(unreal.MaterialExpressionAbs, ("", cell_sum))
    is_odd = node(unreal.MaterialExpressionFmod, ("A", positive_sum), ("B", number(2)))
    color = node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", grey(CHECKER_LIGHT)),
        ("B", grey(CHECKER_DARK)),
        ("Alpha", is_odd),
    )
    library.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(number(CHECKER_ROUGHNESS), "", unreal.MaterialProperty.MP_ROUGHNESS)
    library.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError(f"could not save {MATERIAL_PATH}")


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
    make_checker_material()

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
