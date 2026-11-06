"""Builds the project's materials from nothing and imports the sounds from SourceAssets/Audio.

Run without an editor window through tools/rebuild_content.ps1.
"""
import glob
import os

import unreal

MATERIAL_FOLDER = "/Game/Materials"
AUDIO_FOLDER = "/Game/Audio"

CHECKER_CELL_CM = 100
# Surfaces lie on whole and half metres. This shift keeps them off the cell borders, where the pattern would flicker.
CHECKER_SHIFT_CELLS = 0.13
CHECKER_LIGHT = 0.2
CHECKER_DARK = 0.13
CHECKER_ROUGHNESS = 0.85
# The shimmer over a model is this strong in its middle and grows towards its edges, so the shape stays readable.
OVERLAY_MIDDLE = 0.35
# The island landscape layers: deep sea, mud/shallows, sand/clearings, forest grass, and granite rock on slopes/peaks.
TERRAIN_SEA = unreal.LinearColor(0.04, 0.10, 0.16, 1)
TERRAIN_MUD = unreal.LinearColor(0.12, 0.11, 0.08, 1)
TERRAIN_SAND = unreal.LinearColor(0.42, 0.36, 0.24, 1)
TERRAIN_GRASS = unreal.LinearColor(0.14, 0.21, 0.09, 1)
TERRAIN_ROCK = unreal.LinearColor(0.28, 0.30, 0.32, 1)
TERRAIN_DARK_CELL = 0.82
TERRAIN_ROCK_BELOW_UP = 0.88
TERRAIN_ROCK_SHARPNESS = 6.0
TERRAIN_ROUGHNESS = 0.95
WATER_COLOR = unreal.LinearColor(0.03, 0.13, 0.19, 1)
WATER_DARK_CELL = 0.90
WATER_ROUGHNESS = 0.15
WOOD_COLOR = unreal.LinearColor(0.24, 0.15, 0.08, 1)
WOOD_DARK_CELL = 0.84
WOOD_ROUGHNESS = 0.85

library = unreal.MaterialEditingLibrary


class Graph:
    """Creates a material, or empties the existing one, and builds its nodes."""

    def __init__(self, name):
        self.path = f"{MATERIAL_FOLDER}/{name}"
        material_file = os.path.join(unreal.Paths.project_content_dir(), "Materials", f"{name}.uasset")
        if os.path.exists(material_file):
            self.material = unreal.load_asset(self.path)
            library.delete_all_material_expressions(self.material)
        else:
            tools = unreal.AssetToolsHelpers.get_asset_tools()
            self.material = tools.create_asset(name, MATERIAL_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
        if self.material is None:
            raise RuntimeError(f"could not create {self.path}")

    def node(self, expression_class, *inputs, **properties):
        """Makes a node. Inputs are (input name, source node) pairs."""
        expression = library.create_material_expression(self.material, expression_class)
        for key, value in properties.items():
            expression.set_editor_property(key, value)
        for input_name, source in inputs:
            library.connect_material_expressions(source, "", expression, input_name)
        return expression

    def number(self, value):
        return self.node(unreal.MaterialExpressionConstant, r=value)

    def grey(self, value):
        return self.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(value, value, value, 1))

    def color_parameter(self, name, default):
        return self.node(unreal.MaterialExpressionVectorParameter, parameter_name=name, default_value=default)

    def number_parameter(self, name, default):
        return self.node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=default)

    def multiply(self, a, b):
        return self.node(unreal.MaterialExpressionMultiply, ("A", a), ("B", b))

    def output(self, source, material_property):
        library.connect_material_property(source, "", material_property)

    def finish(self):
        library.recompile_material(self.material)
        if not unreal.EditorAssetLibrary.save_asset(self.path):
            raise RuntimeError(f"could not save {self.path}")


def checker_is_odd(graph):
    """0 or 1 in 1 m cells fixed in the world."""
    cells = graph.node(
        unreal.MaterialExpressionDivide,
        ("A", graph.node(unreal.MaterialExpressionWorldPosition)),
        ("B", graph.number(CHECKER_CELL_CM)),
    )
    shifted = graph.node(unreal.MaterialExpressionAdd, ("A", cells), ("B", graph.number(CHECKER_SHIFT_CELLS)))
    whole_cells = graph.node(unreal.MaterialExpressionFloor, ("", shifted))
    # The sum of the cell numbers alternates between even and odd from one cell to the next, on every axis.
    cell_sum = graph.node(unreal.MaterialExpressionDotProduct, ("A", whole_cells), ("B", graph.grey(1)))
    positive_sum = graph.node(unreal.MaterialExpressionAbs, ("", cell_sum))
    return graph.node(unreal.MaterialExpressionFmod, ("A", positive_sum), ("B", graph.number(2)))


def make_checker():
    """A checker fixed in the world: it does not stretch on scaled cubes, and its 1 m cells show size and speed."""
    graph = Graph("M_Schachbrett")
    is_odd = checker_is_odd(graph)
    color = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", graph.grey(CHECKER_LIGHT)),
        ("B", graph.grey(CHECKER_DARK)),
        ("Alpha", is_odd),
    )
    graph.output(color, unreal.MaterialProperty.MP_BASE_COLOR)
    graph.output(graph.number(CHECKER_ROUGHNESS), unreal.MaterialProperty.MP_ROUGHNESS)
    graph.finish()


def height_band(graph, z_cm, low_cm, high_cm):
    """0 below low_cm, 1 above high_cm, linear between."""
    shifted = graph.node(unreal.MaterialExpressionSubtract, ("A", z_cm), ("B", graph.number(low_cm)))
    scaled = graph.multiply(shifted, graph.number(1.0 / (high_cm - low_cm)))
    return graph.node(unreal.MaterialExpressionSaturate, ("", scaled))


def make_terrain():
    """The layered stand-in for the island landscape: sea, mud, sand/clearings, forest grass, and rock on slopes/peaks."""
    graph = Graph("M_Gelaende")
    cell_shade = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", graph.number(1.0)),
        ("B", graph.number(TERRAIN_DARK_CELL)),
        ("Alpha", checker_is_odd(graph)),
    )
    world_z = graph.node(
        unreal.MaterialExpressionComponentMask,
        ("", graph.node(unreal.MaterialExpressionWorldPosition)),
        r=False, g=False, b=True, a=False,
    )
    up = graph.node(
        unreal.MaterialExpressionComponentMask,
        ("", graph.node(unreal.MaterialExpressionVertexNormalWS)),
        r=False, g=False, b=True, a=False,
    )

    sea_or_mud = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", graph.color_parameter("Meer", TERRAIN_SEA)),
        ("B", graph.color_parameter("Schlamm", TERRAIN_MUD)),
        ("Alpha", height_band(graph, world_z, -160.0, -80.0)),
    )
    with_sand = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", sea_or_mud),
        ("B", graph.color_parameter("Strand", TERRAIN_SAND)),
        ("Alpha", height_band(graph, world_z, -25.0, -4.0)),
    )
    with_grass = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", with_sand),
        ("B", graph.color_parameter("Gras", TERRAIN_GRASS)),
        ("Alpha", height_band(graph, world_z, 12.0, 34.0)),
    )

    slope_rock = graph.node(
        unreal.MaterialExpressionSaturate,
        ("", graph.multiply(
            graph.node(unreal.MaterialExpressionSubtract, ("A", graph.number(TERRAIN_ROCK_BELOW_UP)), ("B", up)),
            graph.number(TERRAIN_ROCK_SHARPNESS),
        )),
    )
    peak_rock = height_band(graph, world_z, 450.0, 750.0)
    rock_alpha = graph.node(unreal.MaterialExpressionMax, ("A", slope_rock), ("B", peak_rock))
    surface = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", with_grass),
        ("B", graph.color_parameter("Fels", TERRAIN_ROCK)),
        ("Alpha", rock_alpha),
    )
    graph.output(graph.multiply(surface, cell_shade), unreal.MaterialProperty.MP_BASE_COLOR)
    graph.output(graph.number(TERRAIN_ROUGHNESS), unreal.MaterialProperty.MP_ROUGHNESS)
    graph.finish()


def make_water():
    """The sea, streams, and swamp water surface at Z = -0.15 m."""
    graph = Graph("M_Wasser")
    cell_shade = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", graph.number(1.0)),
        ("B", graph.number(WATER_DARK_CELL)),
        ("Alpha", checker_is_odd(graph)),
    )
    color = graph.color_parameter("Farbe", WATER_COLOR)
    graph.output(graph.multiply(color, cell_shade), unreal.MaterialProperty.MP_BASE_COLOR)
    graph.output(graph.number(WATER_ROUGHNESS), unreal.MaterialProperty.MP_ROUGHNESS)
    graph.output(graph.number(0.7), unreal.MaterialProperty.MP_SPECULAR)
    graph.finish()


def make_wood():
    """Wooden planks and posts for bridges, piers, and scaffolds."""
    graph = Graph("M_Holz")
    cell_shade = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", graph.number(1.0)),
        ("B", graph.number(WOOD_DARK_CELL)),
        ("Alpha", checker_is_odd(graph)),
    )
    color = graph.color_parameter("Farbe", WOOD_COLOR)
    graph.output(graph.multiply(color, cell_shade), unreal.MaterialProperty.MP_BASE_COLOR)
    graph.output(graph.number(WOOD_ROUGHNESS), unreal.MaterialProperty.MP_ROUGHNESS)
    graph.finish()


def make_color():
    """The plain material of all grey-box parts. Code sets its color and glow per part."""
    graph = Graph("M_Farbe")
    color = graph.color_parameter("Farbe", unreal.LinearColor(0.5, 0.5, 0.5, 1))
    glow = graph.number_parameter("Leuchten", 0.0)
    graph.output(color, unreal.MaterialProperty.MP_BASE_COLOR)
    graph.output(graph.multiply(color, glow), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    graph.output(graph.number(0.55), unreal.MaterialProperty.MP_ROUGHNESS)
    graph.finish()


def make_glow():
    """Additive light for fire and blade trails. Vertex alpha and the parameter Deckkraft fade it out."""
    graph = Graph("M_Glut")
    graph.material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    graph.material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    graph.material.set_editor_property("two_sided", True)
    color = graph.color_parameter("Farbe", unreal.LinearColor(1.0, 0.45, 0.1, 1))
    strength = graph.number_parameter("Leuchten", 4.0)
    opacity = graph.number_parameter("Deckkraft", 1.0)
    vertex_color = graph.node(unreal.MaterialExpressionVertexColor)
    fade = graph.node(unreal.MaterialExpressionMultiply, ("A", opacity))
    library.connect_material_expressions(vertex_color, "A", fade, "B")
    graph.output(graph.multiply(graph.multiply(color, strength), fade), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    graph.finish()


def make_overlay():
    """A colored shimmer laid over a whole 3D model, such as the red before a strike. Code sets Farbe and Staerke."""
    graph = Graph("M_Ueberzug")
    graph.material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    graph.material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = graph.color_parameter("Farbe", unreal.LinearColor(1.0, 0.1, 0.05, 1))
    strength = graph.number_parameter("Staerke", 0.0)
    edges = graph.node(
        unreal.MaterialExpressionAdd,
        ("A", graph.node(unreal.MaterialExpressionFresnel)),
        ("B", graph.number(OVERLAY_MIDDLE)),
    )
    graph.output(graph.multiply(graph.multiply(color, strength), edges), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    graph.finish()


def import_sounds():
    source_folder = os.path.join(unreal.Paths.project_dir(), "SourceAssets", "Audio")
    tasks = []
    for wav in sorted(glob.glob(os.path.join(source_folder, "*.wav"))):
        task = unreal.AssetImportTask()
        task.filename = os.path.abspath(wav)
        task.destination_path = AUDIO_FOLDER
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        for task in tasks:
            if not task.imported_object_paths:
                raise RuntimeError(f"could not import {task.filename}")


def main():
    make_checker()
    make_terrain()
    make_water()
    make_wood()
    make_color()
    make_glow()
    make_overlay()
    import_sounds()
    unreal.log("make_assets: done")


main()
