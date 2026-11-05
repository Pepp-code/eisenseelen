"""Höhen der Aschesteppe aus Data/gelaende.json und Data/laufkarte.json.

Reines Python ohne Unreal: make_runmap.py baut damit die Landschaft, und
`py -3.12 tools/gelaende.py [bild.png]` zeichnet eine Draufsicht zum Prüfen.
Jeder Spielort aus laufkarte.json bleibt eben auf Höhe 0.
"""

import json
import math
import os
import random
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KARTE_HALB_M = 150.0


def _load(name):
    with open(os.path.join(ROOT, "Data", name), encoding="utf-8") as f:
        return json.load(f)


def _smooth(edge0, edge1, x):
    if edge0 == edge1:
        return 0.0 if x < edge0 else 1.0
    t = max(0.0, min(1.0, (x - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


def _segment_distance(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    length_sq = dx * dx + dy * dy
    t = 0.0 if length_sq == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / length_sq))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


class Gelaende:
    def __init__(self, design=None, karte=None):
        self.design = design or _load("gelaende.json")
        self.karte = karte or _load("laufkarte.json")
        rng = random.Random(self.design.get("zufall", 7))
        wave_length = self.design.get("wellen_laenge_m", 55.0)
        self.waves = []
        for factor in (1.0, 0.61, 1.73):
            angle = rng.uniform(0, math.pi)
            k = 2 * math.pi / (wave_length * factor)
            self.waves.append((math.cos(angle) * k, math.sin(angle) * k, rng.uniform(0, 2 * math.pi), 1.0 / factor))
        weight_sum = sum(w[3] for w in self.waves)
        self.wave_scale = self.design.get("wellen_hoehe_m", 3.0) * 0.5 / weight_sum
        self.flat_spots = self._collect_flat_spots()

    def _collect_flat_spots(self):
        radii = self.design["eben_um"]
        spots = [(0.0, 0.0, radii["mitte_m"])]
        for key in ("orte", "bossplaetze", "signalfeuer", "ankunft", "lager", "sprungfelder"):
            radius = radii[key + "_m"]
            for spot in self.karte.get(key, []):
                spots.append((spot["x_m"], spot["y_m"], radius))
        return spots

    def flat_weight(self, x, y):
        """1 auf einem Spielort, 0 im freien Gelände, dazwischen weich."""
        blend = self.design["eben_um"]["uebergang_m"]
        best = 0.0
        for sx, sy, radius in self.flat_spots:
            distance = math.hypot(x - sx, y - sy)
            if distance < radius + blend:
                best = max(best, 1.0 - _smooth(radius, radius + blend, distance))
                if best >= 1.0:
                    return 1.0
        return best

    def _waves(self, x, y):
        return self.wave_scale * sum(w * math.sin(kx * x + ky * y + phase) for kx, ky, phase, w in self.waves)

    def _mesas(self, x, y):
        ramp = self.design.get("rampe", {})
        ramp_half = ramp.get("breite_m", 6.0) * 0.5
        ramp_slope = math.tan(math.radians(ramp.get("steigung_grad", 22.0)))
        top = 0.0
        for mesa in self.design.get("tafelberge", []):
            dx, dy = x - mesa["x_m"], y - mesa["y_m"]
            radius, height = mesa["radius_m"], mesa["hoehe_m"]
            cliff = max(1.5, height * 0.35)
            distance = math.hypot(dx, dy)
            if distance > radius + height / ramp_slope + cliff + 2:
                continue
            value = height * (1.0 - _smooth(radius, radius + cliff, distance))
            if "rampe_grad" in mesa:
                angle = math.radians(mesa["rampe_grad"])
                along = dx * math.cos(angle) + dy * math.sin(angle)
                side = abs(-dx * math.sin(angle) + dy * math.cos(angle))
                if along > 0 and side < ramp_half + 1.5:
                    ramp_height = height - max(0.0, along - radius) * ramp_slope
                    edge = 1.0 - _smooth(ramp_half, ramp_half + 1.5, side)
                    value = max(value, max(0.0, ramp_height) * edge)
            top = max(top, value)
        return top

    def _riverbed(self, x, y):
        river = self.design.get("flussbett")
        if not river:
            return 0.0
        points = river["punkte"]
        distance = min(_segment_distance(x, y, *points[i], *points[i + 1]) for i in range(len(points) - 1))
        bottom_half = river["sohle_m"] * 0.5
        return -river["tiefe_m"] * (1.0 - _smooth(bottom_half, bottom_half + river["ufer_m"], distance))

    def _rim(self, x, y):
        rim = self.design["rand"]
        to_edge = KARTE_HALB_M - max(abs(x), abs(y))
        return rim["hoehe_m"] * (1.0 - _smooth(0.0, rim["breite_m"], to_edge))

    def height_m(self, x, y):
        raw = self._waves(x, y) + self._mesas(x, y) + self._riverbed(x, y) + self._rim(x, y)
        return raw * (1.0 - self.flat_weight(x, y))

    def resolution(self):
        res = self.design["aufloesung"]
        side = res["teile_pro_seite"] * res["quadrate_pro_teil"] + 1
        return res["teile_pro_seite"], res["quadrate_pro_teil"], res["quadrat_cm"], side

    def grid_cm(self):
        """Höhen aller Eckpunkte in Zentimetern, Zeile für Zeile, X läuft am schnellsten."""
        _, _, quad_cm, side = self.resolution()
        half_m = (side - 1) * quad_cm / 200.0
        step_m = quad_cm / 100.0
        heights = []
        for row in range(side):
            y = -half_m + row * step_m
            for column in range(side):
                heights.append(self.height_m(-half_m + column * step_m, y) * 100.0)
        return heights


def draw_preview(path):
    from PIL import Image, ImageDraw

    terrain = Gelaende()
    _, _, quad_cm, side = terrain.resolution()
    heights = terrain.grid_cm()
    low, high = min(heights), max(heights)
    scale = 3
    image = Image.new("RGB", (side, side))
    pixels = image.load()
    for row in range(side):
        for column in range(side):
            h = heights[row * side + column]
            right = heights[row * side + min(column + 1, side - 1)]
            up = heights[min(row + 1, side - 1) * side + column]
            slope = math.degrees(math.atan(math.hypot(right - h, up - h) / quad_cm))
            light = 0.55 + 0.45 * max(-1.0, min(1.0, ((h - right) + (up - h)) / quad_cm * 1.5))
            level = (h - low) / max(1.0, high - low)
            color = (120 + 110 * level, 95 + 80 * level, 70 + 40 * level)
            if h < -150:
                color = (95, 80, 110)
            if slope > 45:
                color = (200, 60, 50)
            pixels[column, side - 1 - row] = tuple(int(max(0, min(255, c * light))) for c in color)
    image = image.resize((side * scale, side * scale), Image.NEAREST)
    draw = ImageDraw.Draw(image)
    half = (side - 1) / 2.0

    def to_pixel(x, y):
        return ((x + half) * scale, (half - y) * scale)

    for x, y, radius in terrain.flat_spots:
        px, py = to_pixel(x, y)
        draw.ellipse([px - radius * scale, py - radius * scale, px + radius * scale, py + radius * scale], outline=(255, 255, 255), width=1)
    for mesa in terrain.design.get("tafelberge", []):
        px, py = to_pixel(mesa["x_m"], mesa["y_m"])
        draw.text((px - 20, py - 5), mesa["name"], fill=(255, 255, 255))
    draw.text((8, 8), f"Höhe {low / 100:.1f} bis {high / 100:.1f} m, rot = zu steil zum Gehen", fill=(255, 255, 255))
    image.save(path)
    return path


if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "Saved", "gelaende_draufsicht.png")
    print(draw_preview(target))
