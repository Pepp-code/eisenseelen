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
CHECKER_LIGHT = 0.42
CHECKER_DARK = 0.30
CHECKER_ROUGHNESS = 0.85

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


def make_checker():
    """A checker fixed in the world: it does not stretch on scaled cubes, and its 1 m cells show size and speed."""
    graph = Graph("M_Schachbrett")
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
    is_odd = graph.node(unreal.MaterialExpressionFmod, ("A", positive_sum), ("B", graph.number(2)))
    color = graph.node(
        unreal.MaterialExpressionLinearInterpolate,
        ("A", graph.grey(CHECKER_LIGHT)),
        ("B", graph.grey(CHECKER_DARK)),
        ("Alpha", is_odd),
    )
    graph.output(color, unreal.MaterialProperty.MP_BASE_COLOR)
    graph.output(graph.number(CHECKER_ROUGHNESS), unreal.MaterialProperty.MP_ROUGHNESS)
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
    vertex_alpha = graph.node(
        unreal.MaterialExpressionComponentMask, r=False, g=False, b=False, a=True,
    )
    library.connect_material_expressions(vertex_color, "", vertex_alpha, "")
    fade = graph.multiply(opacity, vertex_alpha)
    graph.output(graph.multiply(graph.multiply(color, strength), fade), unreal.MaterialProperty.MP_EMISSIVE_COLOR)
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
    make_color()
    make_glow()
    import_sounds()
    unreal.log("make_assets: done")


main()
