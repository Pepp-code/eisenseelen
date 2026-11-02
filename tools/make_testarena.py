"""Builds the grey test arena /Game/Maps/L_Testarena from nothing.

Run without an editor window:
    C:\\Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe Eisenseelen.uproject
        -run=pythonscript -script=tools/make_testarena.py -unattended -nullrhi
"""
import os

import unreal

MAP_PATH = "/Game/Maps/L_Testarena"
FLOOR_SIZE_M = 200
GRID_MATERIAL = "/Engine/EngineMaterials/WorldGridMaterial"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"


def spawn(actors, actor_class, location=(0, 0, 0), rotation=(0, 0, 0), label=""):
    actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(*location), unreal.Rotator(*rotation))
    if actor is None:
        raise RuntimeError(f"could not spawn {actor_class}")
    actor.set_actor_label(label or actor_class.__name__)
    return actor


def main():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # new_level refuses a map the editor already knows, so a rebuild empties the existing one instead.
    map_file = os.path.join(unreal.Paths.project_content_dir(), "Maps", "L_Testarena.umap")
    if os.path.exists(map_file):
        if not levels.load_level(MAP_PATH):
            raise RuntimeError(f"could not load {MAP_PATH}")
        for actor in actors.get_all_level_actors():
            actors.destroy_actor(actor)
    elif not levels.new_level(MAP_PATH):
        raise RuntimeError(f"could not create {MAP_PATH}")

    # The cube is 1 m wide and its top must sit at z = 0.
    floor = spawn(actors, unreal.StaticMeshActor, location=(0, 0, -50), label="Boden")
    floor.static_mesh_component.set_static_mesh(unreal.load_asset(CUBE_MESH))
    floor.static_mesh_component.set_material(0, unreal.load_asset(GRID_MATERIAL))
    floor.set_actor_scale3d(unreal.Vector(FLOOR_SIZE_M, FLOOR_SIZE_M, 1))

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
