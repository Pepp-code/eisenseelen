"""Builds the grey map of a whole run, /Game/Maps/L_ErsterLauf, from Data/laufkarte.json and Data/gelaende.json.

Run without an editor window:
    C:\\Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe Eisenseelen.uproject
        -run=pythonscript -script=tools/make_runmap.py -unattended -nullrhi
"""
import json
import math
import os
import random
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gelaende import Gelaende  # noqa: E402

MAP_PATH = "/Game/Maps/L_ErsterLauf"
MATERIAL_PATH = "/Game/Materials/M_Schachbrett"
TERRAIN_MATERIAL_PATH = "/Game/Materials/M_Gelaende"
WATER_MATERIAL_PATH = "/Game/Materials/M_Wasser"
WOOD_MATERIAL_PATH = "/Game/Materials/M_Holz"
ROCK_MATERIAL_PATH = "/Game/Materials/M_Fels"
FOLIAGE_MATERIAL_PATH = "/Game/Materials/M_Laub"
GLOW_MATERIAL_PATH = "/Game/Materials/M_Glut"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"

CM_PER_M = 100
WATER_SURFACE_M = -0.15
WATER_THICKNESS_M = 0.20
COAST_BORDER_OUTSET_M = 24.0
BRIDGE_TOP_M = 0.14
BRIDGE_THICKNESS_M = 0.24
BRIDGE_POST_SIZE_M = 0.35
BRIDGE_POST_HEIGHT_M = 0.85
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
# Blocks reach this far into the ground, so they do not float on a slope.
COVER_SINK_M = 1
# Cover and chests stay off mesa tops, out of deep hollows and off slopes.
OPEN_GROUND_MAX_HEIGHT_M = 2.5
OPEN_GROUND_MAX_SLOPE_DEG = 20
# A chest needs room for the fight around it, away from the altars and fires, where the fights already are.
CHEST_CLEAR_EDGE_M = 16
CHEST_CLEAR_PLACE_M = 32
CHEST_CLEAR_PAD_M = 18
CHEST_CLEAR_COVER_M = 15
CHEST_CLEAR_CAMP_M = 26
CHEST_CLEAR_CHEST_M = 35
# A new volume's brush is a cube of this size. Paths are only needed near the floor.
VOLUME_BRUSH_M = 2
# Paths are needed from the shallow water and quarry up to the mountain tops.
NAVIGATION_BOTTOM_M = -10
NAVIGATION_TOP_M = 35
# Everything the script builds carries this tag. A rebuild removes only these, so art placed by hand stays.
GENERATED_TAG = "EI_Generiert"

terrain = Gelaende()


def ground_m(x, y):
    return terrain.height_m(x, y)


def is_open_ground(x, y):
    """Level enough, off the roads and tunnel, and on the dry island plain, so a block or chest belongs there."""
    if not terrain.is_dry_land(x, y) or terrain.road_weight(x, y) > 0.05 or terrain.tunnel_cut_weight(x, y) > 0.05:
        return False
    height = ground_m(x, y)
    step = 1.0
    rise = max(abs(ground_m(x + step, y) - height), abs(ground_m(x, y + step) - height)) / step
    return 0.0 <= height <= OPEN_GROUND_MAX_HEIGHT_M and math.degrees(math.atan(rise)) <= OPEN_GROUND_MAX_SLOPE_DEG


def on_ground(x_m, y_m):
    """Location in centimetres for an actor that stands on the terrain."""
    return (x_m * CM_PER_M, y_m * CM_PER_M, ground_m(x_m, y_m) * CM_PER_M)


def spawn(actors, actor_class, location=(0, 0, 0), rotation=(0, 0, 0), label=""):
    actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(*location), unreal.Rotator(*rotation))
    if actor is None:
        raise RuntimeError(f"could not spawn {actor_class}")
    actor.set_actor_label(label or actor_class.__name__)
    actor.set_editor_property("tags", [unreal.Name(GENERATED_TAG)])
    return actor


def clear_generated(actors):
    """Removes what this script built last time. Things placed by hand stay, unless the map is from before the tag."""
    level_actors = actors.get_all_level_actors()
    tagged = [actor for actor in level_actors if actor.actor_has_tag(GENERATED_TAG)]
    for actor in tagged or level_actors:
        actors.destroy_actor(actor)


def add_box(actors, label, center_m, size_m, rotation=(0, 0, 0), material_path=MATERIAL_PATH):
    """The engine's cube is 1 m wide, so its scale is its size in metres."""
    location = [value * CM_PER_M for value in center_m]
    box = spawn(actors, unreal.StaticMeshActor, location=location, rotation=rotation, label=label)
    box.static_mesh_component.set_static_mesh(unreal.load_asset(CUBE_MESH))
    box.static_mesh_component.set_material(0, unreal.load_asset(material_path))
    box.set_actor_scale3d(unreal.Vector(*size_m))
    return box


def load_looks():
    path = os.path.join(unreal.Paths.project_dir(), "Data", "aussehen.json")
    with open(path, encoding="utf-8") as file:
        return json.load(file).get("aussehen", {})


LOOKS = load_looks()


def add_part(actors, part_key, label, center_m, size_m, rotation=(0, 0, 0), material_path=MATERIAL_PATH):
    """Spawns a building block that replaces itself with the 3D model from Data/aussehen.json once a path is entered."""
    entry = LOOKS.get(part_key, {})
    model_path = entry.get("modell", "").strip() if isinstance(entry, dict) else ""
    if model_path:
        asset = unreal.load_asset(model_path)
        if asset is not None:
            offset_cm = entry.get("versatz_cm", [0, 0, 0])
            rot_deg = entry.get("drehung_grad", [0, 0, 0])
            scale_val = entry.get("skalierung", 1.0)
            scale = [scale_val, scale_val, scale_val] if isinstance(scale_val, (int, float)) else list(scale_val)
            loc = [center_m[i] * CM_PER_M + offset_cm[i] for i in range(3)]
            rot = (rotation[0] + rot_deg[0], rotation[1] + rot_deg[1], rotation[2] + rot_deg[2])
            actor = spawn(actors, unreal.StaticMeshActor, location=loc, rotation=rot, label=label)
            actor.static_mesh_component.set_static_mesh(asset)
            actor.set_actor_scale3d(unreal.Vector(*scale))
            return actor
    return add_box(actors, label, center_m, size_m, rotation=rotation, material_path=material_path)


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


def add_sea_border(actors, floor_size):
    """Invisible boundary around the sea at the outer edge of the landscape and along the deep-water coast."""
    half = floor_size / 2
    floor_z = -6.0
    sides = [
        ((0, half), (floor_size + EDGE_THICKNESS_M, EDGE_THICKNESS_M)),
        ((0, -half), (floor_size + EDGE_THICKNESS_M, EDGE_THICKNESS_M)),
        ((half, 0), (EDGE_THICKNESS_M, floor_size + EDGE_THICKNESS_M)),
        ((-half, 0), (EDGE_THICKNESS_M, floor_size + EDGE_THICKNESS_M)),
    ]
    for (x, y), (size_x, size_y) in sides:
        blocker = add_box(actors, "Randsperre", (x, y, floor_z + EDGE_BLOCKER_HEIGHT_M / 2), (size_x, size_y, EDGE_BLOCKER_HEIGHT_M))
        blocker.set_actor_hidden_in_game(True)

    coast_pts = terrain.design.get("kueste_punkte", [])
    ring = []
    for x, y in coast_pts:
        r = math.hypot(x, y)
        scale = (r + COAST_BORDER_OUTSET_M) / max(1.0, r)
        ring.append((x * scale, y * scale))
    for i in range(len(ring)):
        x0, y0 = ring[i]
        x1, y1 = ring[(i + 1) % len(ring)]
        mx, my = (x0 + x1) * 0.5, (y0 + y1) * 0.5
        seg_len = math.hypot(x1 - x0, y1 - y0) + EDGE_THICKNESS_M
        yaw = math.degrees(math.atan2(y1 - y0, x1 - x0))
        blocker = add_box(
            actors,
            "Kuestensperre",
            (mx, my, floor_z + EDGE_BLOCKER_HEIGHT_M / 2),
            (seg_len, EDGE_THICKNESS_M, EDGE_BLOCKER_HEIGHT_M),
            rotation=(0, 0, yaw),
        )
        blocker.set_actor_hidden_in_game(True)


def add_water(actors, floor_size):
    """Visible water surface at Z = -0.15 m across sea, streams, and swamp ponds. Walkable ground stays on the terrain underneath."""
    center_z = WATER_SURFACE_M - WATER_THICKNESS_M * 0.5
    water = add_box(actors, "Wasser", (0, 0, center_z), (floor_size, floor_size, WATER_THICKNESS_M), material_path=WATER_MATERIAL_PATH)
    water.set_actor_enable_collision(False)
    water.static_mesh_component.set_collision_profile_name("NoCollision")
    water.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    water.static_mesh_component.set_cast_shadow(False)


def add_bridges(actors):
    """Wooden bridges and boardwalks over streams, gorges, and swamp ponds."""
    deck_center_z = BRIDGE_TOP_M - BRIDGE_THICKNESS_M * 0.5
    for index, bridge in enumerate(terrain.design.get("bruecken", [])):
        x, y = bridge["x_m"], bridge["y_m"]
        length, width = bridge["laenge_m"], bridge["breite_m"]
        yaw_deg = bridge["drehung_grad"]
        part_key = "bauteil.steg" if bridge["name"].startswith("Steg") else "bauteil.holzbruecke"
        add_part(
            actors,
            part_key,
            f"Bruecke {index + 1}",
            (x, y, deck_center_z),
            (length, width, BRIDGE_THICKNESS_M),
            rotation=(0, 0, yaw_deg),
            material_path=WOOD_MATERIAL_PATH,
        )
        yaw = math.radians(yaw_deg)
        for sx in (-1.0, 1.0):
            for sy in (-1.0, 1.0):
                ox = sx * (length * 0.5 - BRIDGE_POST_SIZE_M * 0.5)
                oy = sy * (width * 0.5 + BRIDGE_POST_SIZE_M * 0.35)
                px = x + ox * math.cos(yaw) - oy * math.sin(yaw)
                py = y + ox * math.sin(yaw) + oy * math.cos(yaw)
                add_box(
                    actors,
                    f"Brueckenpfosten {index + 1}",
                    (px, py, BRIDGE_TOP_M + BRIDGE_POST_HEIGHT_M * 0.5 - 0.15),
                    (BRIDGE_POST_SIZE_M, BRIDGE_POST_SIZE_M, BRIDGE_POST_HEIGHT_M),
                    rotation=(0, 0, yaw_deg),
                    material_path=WOOD_MATERIAL_PATH,
                )


def add_torch(actors, base_m, light_z_m=4.2, light_xy=None):
    """A stone brazier with glowing embers and a warm point light for the tunnel."""
    bx, by, bz = base_m
    stand = add_part(actors, "bauteil.fackel", "Fackel", (bx, by, bz + 0.7), (0.45, 0.45, 1.4), material_path=ROCK_MATERIAL_PATH)
    stand.set_actor_enable_collision(False)
    stand.static_mesh_component.set_collision_profile_name("NoCollision")
    stand.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    ember = add_box(actors, "Fackelglut", (bx, by, bz + 1.55), (0.4, 0.4, 0.35), material_path=GLOW_MATERIAL_PATH)
    ember.set_actor_enable_collision(False)
    ember.static_mesh_component.set_collision_profile_name("NoCollision")
    ember.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    lx, ly = light_xy if light_xy is not None else (bx, by)
    light = spawn(actors, unreal.PointLight, location=(lx * CM_PER_M, ly * CM_PER_M, light_z_m * CM_PER_M), label="Tunnellicht")
    light.point_light_component.set_light_color(unreal.LinearColor(1.0, 0.62, 0.25, 1.0))
    light.point_light_component.set_intensity(6000.0)
    light.point_light_component.set_attenuation_radius(2200.0)


def add_tunnel(actors):
    """The Y-shaped tunnel through Das Eisenjoch with a central cavern, rock roof, side walls, portal arches, and torches."""
    tun = terrain.design.get("tunnel")
    if not tun:
        return
    hm = tun["halle_mitte"]
    hx, hy = hm["x_m"], hm["y_m"]
    hall_ceil = hm.get("decke_hoehe_m", 9.0)
    pass_ceil = tun.get("gang_hoehe_m", 6.0)
    roof_thick = tun.get("dach_dicke_m", 12.0)

    # Central cavern roof and upper peak cap
    add_part(
        actors,
        "bauteil.tunnel_bogen",
        "Tunnelhalle Dach",
        (hx, hy, hall_ceil + roof_thick * 0.5),
        (30.0, 30.0, roof_thick),
        material_path=ROCK_MATERIAL_PATH,
    )
    add_part(
        actors,
        "bauteil.fels_gross",
        "Tunnelhalle Kuppe",
        (hx, hy, hall_ceil + roof_thick + 2.0),
        (20.0, 20.0, 4.0),
        rotation=(0, 0, 18),
        material_path=ROCK_MATERIAL_PATH,
    )

    # Cavern walls closing the sectors between the three tunnel passages
    for ang_deg in (85.0, 213.0, 317.0):
        ang = math.radians(ang_deg)
        wx = hx + 13.5 * math.cos(ang)
        wy = hy + 13.5 * math.sin(ang)
        add_part(
            actors,
            "bauteil.tunnel_bogen",
            "Tunnelhalle Wand",
            (wx, wy, 4.5),
            (3.0, 12.0, 10.0),
            rotation=(0, 0, ang_deg),
            material_path=ROCK_MATERIAL_PATH,
        )

    # Four support pillars and braziers inside the central cavern around the bandit camp
    for ang_deg in tun.get("pfeiler_winkel_grad", [35, 125, 215, 315]):
        ang = math.radians(ang_deg)
        px = hx + 8.5 * math.cos(ang)
        py = hy + 8.5 * math.sin(ang)
        add_part(
            actors,
            "bauteil.fels_mittel",
            "Tunnelpfeiler",
            (px, py, 4.5),
            (2.2, 2.2, 9.5),
            rotation=(0, 0, ang_deg),
            material_path=ROCK_MATERIAL_PATH,
        )
        tx = hx + 6.8 * math.cos(ang)
        ty = hy + 6.8 * math.sin(ang)
        add_torch(actors, (tx, ty, 0.0), light_z_m=5.2, light_xy=(hx + 5.0 * math.cos(ang), hy + 5.0 * math.sin(ang)))

    # Three tunnel passages (North, South, West) with roof slabs, side walls, and stone entrance portals
    for key in ("eingang_nord", "eingang_sued", "eingang_west"):
        ep = tun[key]
        ex, ey = ep["x_m"], ep["y_m"]
        dx, dy = ex - hx, ey - hy
        length = math.hypot(dx, dy)
        yaw_deg = math.degrees(math.atan2(dy, dx))
        ux, uy = dx / length, dy / length
        nx, ny = -uy, ux

        d_roof_0, d_roof_1 = 11.0, length - 1.0
        roof_len = d_roof_1 - d_roof_0
        d_roof_mid = (d_roof_0 + d_roof_1) * 0.5
        rx, ry = hx + ux * d_roof_mid, hy + uy * d_roof_mid
        add_part(
            actors,
            "bauteil.tunnel_bogen",
            f"Tunneldach {key}",
            (rx, ry, pass_ceil + roof_thick * 0.5),
            (roof_len, 14.0, roof_thick),
            rotation=(0, 0, yaw_deg),
            material_path=ROCK_MATERIAL_PATH,
        )

        d_wall_0, d_wall_1 = 12.5, length - 2.0
        wall_len = d_wall_1 - d_wall_0
        d_wall_mid = (d_wall_0 + d_wall_1) * 0.5
        for side in (-1.0, 1.0):
            wx = hx + ux * d_wall_mid + nx * (side * 5.5)
            wy = hy + uy * d_wall_mid + ny * (side * 5.5)
            add_part(
                actors,
                "bauteil.tunnel_bogen",
                f"Tunnelwand {key}",
                (wx, wy, 3.0),
                (wall_len, 3.0, 7.0),
                rotation=(0, 0, yaw_deg),
                material_path=ROCK_MATERIAL_PATH,
            )

        px_base = hx + ux * (length - 1.0)
        py_base = hy + uy * (length - 1.0)
        for side in (-1.0, 1.0):
            ppx = px_base + nx * (side * 5.6)
            ppy = py_base + ny * (side * 5.6)
            add_part(
                actors,
                "bauteil.tunnel_bogen",
                f"Tunnelportal Pfeiler {key}",
                (ppx, ppy, 3.25),
                (3.2, 3.2, 7.5),
                rotation=(0, 0, yaw_deg),
                material_path=ROCK_MATERIAL_PATH,
            )
        add_part(
            actors,
            "bauteil.tunnel_bogen",
            f"Tunnelportal Bogen {key}",
            (px_base, py_base, 7.25),
            (3.6, 14.4, 2.5),
            rotation=(0, 0, yaw_deg),
            material_path=ROCK_MATERIAL_PATH,
        )

        mx = hx + ux * (length * 0.55)
        my = hy + uy * (length * 0.55)
        add_torch(actors, (mx + nx * 3.7, my + ny * 3.7, 0.0), light_z_m=4.2, light_xy=(mx, my))


def add_rocks(actors):
    """Rock cliffs, coastal crags, and low stone walls along the island paths."""
    felsen = terrain.design.get("felsen", {})
    rock_points = []
    for index, cliff in enumerate(felsen.get("klippen", [])):
        x, y = cliff["x_m"], cliff["y_m"]
        sx, sy, sz = cliff["groesse_m"]
        yaw_deg = cliff.get("drehung_grad", 0.0)
        part_key = cliff.get("bauteil", "bauteil.fels_gross")
        bottom = ground_m(x, y) - 1.5
        add_part(
            actors,
            part_key,
            f"Klippe {index + 1}",
            (x, y, bottom + (sz + 1.5) * 0.5),
            (sx, sy, sz + 1.5),
            rotation=(0, 0, yaw_deg),
            material_path=ROCK_MATERIAL_PATH,
        )
        rock_points.append((x, y))

    for index, wall in enumerate(felsen.get("felsmauern", [])):
        x, y = wall["x_m"], wall["y_m"]
        length, width, height = wall["laenge_m"], wall["breite_m"], wall["hoehe_m"]
        yaw_deg = wall.get("drehung_grad", 0.0)
        bottom = ground_m(x, y) - COVER_SINK_M
        add_part(
            actors,
            "bauteil.fels_mauer",
            f"Felsmauer {index + 1}",
            (x, y, bottom + (height + COVER_SINK_M) * 0.5),
            (length, width, height + COVER_SINK_M),
            rotation=(0, 0, yaw_deg),
            material_path=ROCK_MATERIAL_PATH,
        )
        rock_points.append((x, y))
    return rock_points


def _make_transform(center_m, size_m, yaw_deg=0.0):
    loc = unreal.Vector(center_m[0] * CM_PER_M, center_m[1] * CM_PER_M, center_m[2] * CM_PER_M)
    rot = unreal.Rotator(0.0, 0.0, yaw_deg)
    scale = unreal.Vector(size_m[0], size_m[1], size_m[2])
    return unreal.Transform(loc, rot, scale)


def _custom_part_entry(part_key):
    entry = LOOKS.get(part_key, {})
    model_path = entry.get("modell", "").strip() if isinstance(entry, dict) else ""
    if not model_path:
        return None, None
    asset = unreal.load_asset(model_path)
    return (asset, entry) if asset is not None else (None, None)


def _custom_transform(item, entry):
    offset_cm = entry.get("versatz_cm", [0, 0, 0])
    rot_deg = entry.get("drehung_grad", [0, 0, 0])
    scale_val = entry.get("skalierung", 1.0)
    scale = [scale_val, scale_val, scale_val] if isinstance(scale_val, (int, float)) else list(scale_val)
    loc = unreal.Vector(
        item["x_m"] * CM_PER_M + offset_cm[0],
        item["y_m"] * CM_PER_M + offset_cm[1],
        item["z_m"] * CM_PER_M + offset_cm[2],
    )
    rot = unreal.Rotator(rot_deg[0], rot_deg[1], item["drehung_grad"] + rot_deg[2])
    return unreal.Transform(loc, rot, unreal.Vector(*scale))


def add_forest():
    """Instanced pine trees, dead swamp/coastal trees, and non-colliding bushes from tools/gelaende.py."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    cube = unreal.load_asset(CUBE_MESH)
    wood_mat = unreal.load_asset(WOOD_MATERIAL_PATH)
    foliage_mat = unreal.load_asset(FOLIAGE_MATERIAL_PATH)
    bush_cull_cm = int(terrain.design.get("wald", {}).get("busch_sichtweite_cm", 12000))

    forest = terrain.generate_forest()
    pine_trunks = []
    pine_crowns = []
    pine_custom = []
    dead_trees = []
    dead_custom = []
    bushes = []
    bush_custom = []
    tree_points = []

    pine_mesh, pine_entry = _custom_part_entry("bauteil.kiefer")
    dead_mesh, dead_entry = _custom_part_entry("bauteil.baum_tot")
    bush_mesh, bush_entry = _custom_part_entry("bauteil.busch")

    for item in forest:
        x, y, z = item["x_m"], item["y_m"], item["z_m"]
        width = item["breite_m"]
        height = item["hoehe_m"]
        trunk_w = item["stamm_m"]
        yaw = item["drehung_grad"]
        art = item["art"]

        if art == "kiefer":
            tree_points.append((x, y))
            if pine_mesh is not None:
                pine_custom.append(_custom_transform(item, pine_entry))
            else:
                trunk_h = height * 0.78 + 1.0
                pine_trunks.append(_make_transform((x, y, z - 1.0 + trunk_h * 0.5), (trunk_w, trunk_w, trunk_h), yaw))
                low_bottom = z + 4.6
                low_top = z + height * 0.78
                low_h = max(1.8, low_top - low_bottom)
                pine_crowns.append(_make_transform((x, y, low_bottom + low_h * 0.5), (width, width, low_h), yaw))
                high_bottom = z + height * 0.72
                high_top = z + height
                high_h = max(1.6, high_top - high_bottom)
                pine_crowns.append(_make_transform((x, y, high_bottom + high_h * 0.5), (width * 0.68, width * 0.68, high_h), yaw + 45.0))
        elif art == "baum_tot":
            tree_points.append((x, y))
            if dead_mesh is not None:
                dead_custom.append(_custom_transform(item, dead_entry))
            else:
                trunk_h = height + 1.0
                dead_trees.append(_make_transform((x, y, z - 1.0 + trunk_h * 0.5), (trunk_w, trunk_w, trunk_h), yaw))
                branch_z = z + height * 0.72
                branch_w = trunk_w * 0.62
                dead_trees.append(_make_transform((x, y, branch_z), (width, branch_w, branch_w), yaw + 30.0))
        elif art == "busch":
            if bush_mesh is not None:
                bush_custom.append(_custom_transform(item, bush_entry))
            else:
                bush_h = height + 0.3
                bushes.append(_make_transform((x, y, z - 0.3 + bush_h * 0.5), (width, width * 0.88, bush_h), yaw))

    if pine_custom:
        unreal.EIWorldTools.build_instanced_mesh(world, pine_mesh, None, pine_custom, True, 0, "Kiefer", GENERATED_TAG)
    else:
        if pine_trunks:
            unreal.EIWorldTools.build_instanced_mesh(world, cube, wood_mat, pine_trunks, True, 0, "Kiefernstamm", GENERATED_TAG)
        if pine_crowns:
            unreal.EIWorldTools.build_instanced_mesh(world, cube, foliage_mat, pine_crowns, False, 0, "Kiefernkrone", GENERATED_TAG)

    if dead_custom:
        unreal.EIWorldTools.build_instanced_mesh(world, dead_mesh, None, dead_custom, True, 0, "Toter Baum", GENERATED_TAG)
    elif dead_trees:
        unreal.EIWorldTools.build_instanced_mesh(world, cube, wood_mat, dead_trees, True, 0, "Toter Baum", GENERATED_TAG)

    if bush_custom:
        unreal.EIWorldTools.build_instanced_mesh(world, bush_mesh, None, bush_custom, False, bush_cull_cm, "Busch", GENERATED_TAG)
    elif bushes:
        unreal.EIWorldTools.build_instanced_mesh(world, cube, foliage_mat, bushes, False, bush_cull_cm, "Busch", GENERATED_TAG)

    return tree_points


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
        floor_z = ground_m(center_x, center_y)
        add_box(actors, f"Ort {index + 1}", (center_x, center_y, floor_z + PLACE_WALL_HEIGHT_M / 2), size, rotation=(0, 0, math.degrees(yaw) + wall_yaw))


def add_cover(actors, layout, extra_taken=(), tree_points=()):
    """Loose blocks that give cover and break the view. They keep clear of the places, the fires and the middle of the map."""
    rng = random.Random(layout["deckung_zufall"])
    half = layout["boden_m"] / 2 - COVER_CLEAR_M
    taken = [(p["x_m"], p["y_m"]) for p in layout["orte"] + layout["signalfeuer"] + layout["ankunft"] + layout["sprungfelder"]]
    taken += [(b["x_m"], b["y_m"]) for b in terrain.design.get("bruecken", [])]
    taken += list(extra_taken)
    taken.append((0, 0))
    bosses = [(p["x_m"], p["y_m"]) for p in layout["bossplaetze"]]
    placed = 0
    while placed < layout["deckung_anzahl"]:
        x, y = rng.uniform(-half, half), rng.uniform(-half, half)
        if any(math.hypot(x - other_x, y - other_y) < COVER_CLEAR_M for other_x, other_y in taken):
            continue
        if any(math.hypot(x - other_x, y - other_y) < BOSS_CLEAR_M for other_x, other_y in bosses):
            continue
        if any(math.hypot(x - tx, y - ty) < 5.0 for tx, ty in tree_points):
            continue
        if not is_open_ground(x, y):
            continue
        size_x, size_y = rng.uniform(COVER_MIN_SIZE_M, COVER_MAX_SIZE_M), rng.uniform(COVER_MIN_SIZE_M, COVER_MAX_SIZE_M)
        height = rng.uniform(1, COVER_MAX_HEIGHT_M)
        bottom = ground_m(x, y) - COVER_SINK_M
        add_box(actors, "Deckung", (x, y, bottom + (height + COVER_SINK_M) / 2), (size_x, size_y, height + COVER_SINK_M), rotation=(0, 0, rng.uniform(0, 90)))
        taken.append((x, y))
        placed += 1
    return taken


def add_camps(actors, layout):
    """The fixed places where enemies stand watch. The game fills them when a run begins."""
    camp_class = unreal.load_class(None, "/Script/Eisenseelen.EIEnemyCamp")
    for camp in layout["lager"]:
        actor = spawn(actors, camp_class, location=on_ground(camp["x_m"], camp["y_m"]), label="Lager")
        actor.set_editor_property("start_size", camp["groesse"])


def add_chests(actors, layout, cover, tree_points=()):
    """Out in the open, each with room for its fight. A run only uses some of them."""
    chest_class = unreal.load_class(None, "/Script/Eisenseelen.EIChest")
    # A random generator of its own, so that more chests do not move the cover around.
    rng = random.Random(layout["deckung_zufall"] + 1)
    half = layout["boden_m"] / 2 - CHEST_CLEAR_EDGE_M
    keep_clear = [(point, CHEST_CLEAR_COVER_M) for point in cover]
    keep_clear += [((p["x_m"], p["y_m"]), CHEST_CLEAR_PLACE_M) for p in layout["orte"] + layout["signalfeuer"] + layout["ankunft"]]
    keep_clear += [((p["x_m"], p["y_m"]), CHEST_CLEAR_PAD_M) for p in layout["sprungfelder"]]
    keep_clear += [((b["x_m"], b["y_m"]), CHEST_CLEAR_PAD_M) for b in terrain.design.get("bruecken", [])]
    keep_clear += [((p["x_m"], p["y_m"]), BOSS_CLEAR_M) for p in layout["bossplaetze"]]
    keep_clear += [((p["x_m"], p["y_m"]), CHEST_CLEAR_CAMP_M) for p in layout["lager"]]
    keep_clear.append(((0, 0), CHEST_CLEAR_PLACE_M))
    placed = 0
    while placed < layout["truhen"]:
        x, y = rng.uniform(-half, half), rng.uniform(-half, half)
        if any(math.hypot(x - other_x, y - other_y) < clear for (other_x, other_y), clear in keep_clear):
            continue
        if any(math.hypot(x - tx, y - ty) < 5.0 for tx, ty in tree_points):
            continue
        if not is_open_ground(x, y):
            continue
        spawn(actors, chest_class, location=on_ground(x, y), rotation=(0, 0, rng.uniform(0, 360)), label="Truhe")
        keep_clear.append(((x, y), CHEST_CLEAR_CHEST_M))
        placed += 1


def add_launch_pads(actors, pads):
    pad_class = unreal.load_class(None, "/Script/Eisenseelen.EILaunchPad")
    for pad in pads:
        x, y, tower_height = pad["x_m"], pad["y_m"], pad["turm_hoehe_m"]
        spawn(actors, pad_class, location=on_ground(x, y), label="Sprungfeld")
        tower_floor = ground_m(x + TOWER_DISTANCE_M, y)
        add_box(actors, "Turm", (x + TOWER_DISTANCE_M, y, tower_floor + tower_height / 2), (TOWER_SIZE_M, TOWER_SIZE_M, tower_height))


def add_terrain():
    """The landscape. Its heights come from tools/gelaende.py, so the PNG preview shows the same ground."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    components, quads, quad_cm, _ = terrain.resolution()
    material = unreal.load_asset(TERRAIN_MATERIAL_PATH)
    landscape = unreal.EIWorldTools.build_landscape(world, terrain.grid_cm(), components, quads, quad_cm, material, GENERATED_TAG)
    if landscape is None:
        raise RuntimeError("could not build the landscape")


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
        clear_generated(actors)
    elif not levels.new_level(MAP_PATH):
        raise RuntimeError(f"could not create {MAP_PATH}")

    floor_size = layout["boden_m"]
    add_terrain()
    add_water(actors, floor_size)
    add_bridges(actors)
    add_tunnel(actors)
    rock_points = add_rocks(actors)
    tree_points = add_forest()
    add_sea_border(actors, floor_size)
    altar_class = unreal.load_class(None, "/Script/Eisenseelen.EIAltar")
    for index, place in enumerate(layout["orte"]):
        add_place(actors, place, index)
        spawn(actors, altar_class, location=on_ground(place["x_m"], place["y_m"]), label="Altar")
    taken = add_cover(actors, layout, rock_points, tree_points)
    add_camps(actors, layout)
    add_chests(actors, layout, taken, tree_points)
    add_launch_pads(actors, layout["sprungfelder"])

    navigation_middle_cm = (NAVIGATION_BOTTOM_M + NAVIGATION_TOP_M) / 2 * CM_PER_M
    navigation = spawn(actors, unreal.NavMeshBoundsVolume, location=(0, 0, navigation_middle_cm), label="Wegfindung")
    scale = floor_size / VOLUME_BRUSH_M
    navigation.set_actor_scale3d(unreal.Vector(scale, scale, (NAVIGATION_TOP_M - NAVIGATION_BOTTOM_M) / VOLUME_BRUSH_M))

    fire_class = unreal.load_class(None, "/Script/Eisenseelen.EISignalFire")
    for fire in layout["signalfeuer"]:
        spawn(actors, fire_class, location=on_ground(fire["x_m"], fire["y_m"]), label="Signalfeuer")

    add_dock(actors, layout["anlegestelle"])

    sun = spawn(actors, unreal.DirectionalLight, location=(0, 0, 1000), rotation=(0, -50, 30), label="Sonne")
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    # The blood moon changes the light during a run.
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    spawn(actors, unreal.SkyAtmosphere, label="Himmel")
    sky_light = spawn(actors, unreal.SkyLight, location=(0, 0, 1000), label="Himmelslicht")
    sky_light.light_component.set_editor_property("real_time_capture", True)
    spawn(actors, unreal.ExponentialHeightFog, label="Nebel")

    # The game builds the paths itself when the run starts. Paths saved here would only cover what stood when the volume was new.
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.RecastNavMesh):
            actors.destroy_actor(actor)

    if not levels.save_current_level():
        raise RuntimeError(f"could not save {MAP_PATH}")
    unreal.log(f"make_runmap: {MAP_PATH} saved")


main()
