"""Berechnet und zeichnet die geplante Waldinsel aus Data/waldinsel.json.

Reines Python ohne Unreal:
- `py -3.12 tools/plan_waldinsel.py` erzeugt:
  - `Saved/waldinsel_draufsicht.png` (Einzel-Draufsicht des Plans)
  - `Saved/Referenz/plan_vs_vorlage.png` (Vorlage und Plan direkt nebeneinander)

Achsen wie in Unreal (make_runmap.py und EIHud.cpp):
- +X zeigt nach Norden (oben im Bild), -X nach Süden (unten).
- +Y zeigt nach Osten (rechts im Bild), -Y nach Westen (links).
"""

import json
import math
import os
import random
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _load(name="waldinsel.json"):
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


def _polyline_distance(px, py, points):
    return min(_segment_distance(px, py, points[i][0], points[i][1], points[i + 1][0], points[i + 1][1]) for i in range(len(points) - 1))


class Waldinsel:
    def __init__(self, design=None):
        self.design = design or _load("waldinsel.json")
        self.size_m = float(self.design["boden_m"])
        self.half_m = self.size_m * 0.5
        rng = random.Random(self.design.get("zufall", 13))
        wave_length = self.design["hoehen_m"].get("wellen_laenge", 65.0)
        self.waves = []
        for factor in (1.0, 0.58, 1.65):
            angle = rng.uniform(0, math.pi)
            k = 2 * math.pi / (wave_length * factor)
            self.waves.append((math.cos(angle) * k, math.sin(angle) * k, rng.uniform(0, 2 * math.pi), 1.0 / factor))
        weight_sum = sum(w[3] for w in self.waves)
        self.wave_scale = self.design["hoehen_m"].get("wellen_hoehe", 2.2) * 0.5 / weight_sum

        # Precompute polar coastline radii (every 1 degree) from polygon vertices
        pts = self.design["kueste_punkte"]
        self.coast_angles = []
        for x, y in pts:
            ang = math.atan2(y, x)  # 0 = North (+X), pi/2 = East (+Y)
            r = math.hypot(x, y)
            self.coast_angles.append((ang, r))
        self.coast_angles.sort(key=lambda item: item[0])

        self.flat_spots = self._collect_flat_spots()
        self.trees = self._generate_trees()

    def _collect_flat_spots(self):
        radii = self.design["eben_um"]
        spots = []
        for s in self.design.get("sandplaetze", []):
            spots.append((s["x_m"], s["y_m"], s.get("radius_m", radii["sandplaetze_m"])))
        for key in ("bossplaetze", "signalfeuer", "ankunft", "lager", "sprungfelder"):
            rad = radii[key + "_m"]
            for s in self.design.get(key, []):
                spots.append((s["x_m"], s["y_m"], rad))
        return spots

    def coast_radius(self, x, y):
        """Interpolated coast radius in the direction of (x, y) using Catmull-Rom / cosine smoothing."""
        if x == 0 and y == 0:
            return self.design.get("insel_radius_m", 240.0)
        ang = math.atan2(y, x)
        n = len(self.coast_angles)
        for i in range(n):
            a0, r0 = self.coast_angles[i]
            a1, r1 = self.coast_angles[(i + 1) % n]
            if i == n - 1:
                a1 += 2 * math.pi
            check_ang = ang + (2 * math.pi if ang < a0 else 0.0)
            if a0 <= check_ang <= a1:
                t = (check_ang - a0) / max(1e-6, a1 - a0)
                s = 0.5 - 0.5 * math.cos(t * math.pi)
                return r0 + (r1 - r0) * s
        return self.design.get("insel_radius_m", 240.0)

    def flat_weight(self, x, y):
        blend = self.design["eben_um"]["uebergang_m"]
        best = 0.0
        for sx, sy, radius in self.flat_spots:
            d = math.hypot(x - sx, y - sy)
            if d < radius + blend:
                best = max(best, 1.0 - _smooth(radius, radius + blend, d))
                if best >= 1.0:
                    return 1.0
        return best

    def road_weight(self, x, y):
        """1.0 on a road or sand circle, 0.0 outside."""
        for s in self.design.get("sandplaetze", []):
            d = math.hypot(x - s["x_m"], y - s["y_m"])
            r = s["radius_m"]
            if d < r + 2.0:
                return max(0.0, 1.0 - _smooth(r - 1.0, r + 2.0, d))
        best = 0.0
        for line in self.design["wege"]["polylinien"]:
            half_w = line["breite_m"] * 0.5
            d = _polyline_distance(x, y, line["punkte"])
            if d < half_w + 2.5:
                best = max(best, 1.0 - _smooth(half_w - 0.5, half_w + 2.0, d))
        return best

    def stream_depth(self, x, y):
        depth = 0.0
        for b in self.design.get("baeche", []):
            d = _polyline_distance(x, y, b["punkte"])
            half_s = b["sohle_m"] * 0.5
            if d < half_s + b["ufer_m"]:
                depth = max(depth, b["tiefe_m"] * (1.0 - _smooth(half_s, half_s + b["ufer_m"], d)))
        # Swamp ponds in Southeast and Northwest marsh
        for sx, sy, sr, st in ((-130, 120, 48, 0.7), (135, -115, 42, 0.55)):
            d = math.hypot(x - sx, y - sy)
            if d < sr:
                ripple = math.sin(x * 0.22 + y * 0.17) * math.cos(x * 0.19 - y * 0.25)
                if ripple > 0.05:
                    depth = max(depth, st * (1.0 - _smooth(sr * 0.65, sr, d)) * min(1.0, (ripple - 0.05) * 3.0))
        return depth

    def elevation_hills(self, x, y):
        top = 0.0
        for h in self.design.get("erhebungen", []):
            dx, dy = x - h["x_m"], y - h["y_m"]
            rx, ry = h["radius_x_m"], h["radius_y_m"]
            # Normalized elliptical distance with slight organic rock noise
            norm_d = math.hypot(dx / rx, dy / ry)
            if norm_d > 1.8:
                continue
            rock_mod = 0.08 * math.sin(x * 0.12 + y * 0.09) + 0.05 * math.cos(x * 0.23 - y * 0.19)
            eff_d = max(0.0, norm_d + rock_mod)
            steil = h.get("steilheit", 0.45)
            val = h["hoehe_m"] * (1.0 - _smooth(1.0 - steil, 1.0 + steil * 0.5, eff_d))
            top = max(top, val)

        # Ring canyon walls around Schlucht-Plateau (65, -125)
        cd = math.hypot(x - 65.0, y - (-125.0))
        if 29.0 <= cd <= 41.0:
            # Leave bridge gaps
            ang = math.degrees(math.atan2(y - (-125.0), x - 65.0))
            if not (-15 <= ang <= 15 or 65 <= ang <= 95 or -145 <= ang <= -115):
                wall = 5.5 * (1.0 - abs(cd - 35.0) / 6.0)
                top = max(top, wall)
        return top

    def height_m(self, x, y):
        r = math.hypot(x, y)
        coast_r = self.coast_radius(x, y)
        # Small organic coastline wobble
        coast_r += 6.0 * math.sin(x * 0.05 + y * 0.04) + 4.0 * math.cos(x * 0.09 - y * 0.07)

        if r > coast_r + 22.0:
            return self.design["hoehen_m"]["meeresboden"]
        if r > coast_r:
            t = _smooth(coast_r, coast_r + 22.0, r)
            return -0.2 * (1.0 - t) + self.design["hoehen_m"]["meeresboden"] * t

        # On the island
        coast_blend = _smooth(coast_r - 18.0, coast_r, r)
        waves = self.wave_scale * sum(w * math.sin(kx * x + ky * y + phase) for kx, ky, phase, w in self.waves)
        hills = self.elevation_hills(x, y)
        sinks = 0.0
        for s in self.design.get("senken", []):
            d = math.hypot(x - s["x_m"], y - s["y_m"])
            if d < s["radius_m"] + 10.0:
                sinks += s["tiefe_m"] * (1.0 - _smooth(s["radius_m"] * 0.65, s["radius_m"] + 8.0, d))

        streams = self.stream_depth(x, y)
        flat = self.flat_weight(x, y)
        road = self.road_weight(x, y)

        # Keep roads and flat spots level, and cut streams only outside flat spots/roads
        land_h = (waves + hills - sinks) * (1.0 - max(flat, road * 0.85)) - streams * (1.0 - max(flat, road * 0.9))
        beach_h = 0.25 * (1.0 - coast_blend)
        return land_h * (1.0 - coast_blend) + beach_h * coast_blend

    def forest_density(self, x, y):
        """0.0 to 1.0 density for tree placement."""
        r = math.hypot(x, y)
        coast_r = self.coast_radius(x, y)
        if r > coast_r - 8.0:
            return 0.0
        if self.flat_weight(x, y) > 0.15 or self.road_weight(x, y) > 0.15:
            return 0.0
        if self.stream_depth(x, y) > 0.2:
            return 0.0
        # Open regions: Bauernhof (North), Steinbruch (NE)
        if x > 115 and -95 < y < 165:
            return 0.08
        # Dense core forest around central massif and west mountain
        d_core = math.hypot((x - 15) / 140.0, (y - 10) / 170.0)
        if d_core < 1.0:
            return 0.85
        return 0.35

    def _generate_trees(self):
        rng = random.Random(self.design.get("zufall", 13) + 99)
        trees = []
        step = 9.0
        half = self.half_m - 25.0
        x = -half
        while x <= half:
            y = -half
            while y <= half:
                jx = x + rng.uniform(-3.2, 3.2)
                jy = y + rng.uniform(-3.2, 3.2)
                dens = self.forest_density(jx, jy)
                if dens > 0.0 and rng.random() < dens:
                    trees.append((jx, jy, rng.uniform(3.2, 5.2)))
                y += step
            x += step
        # Add shallow water trees in SW peninsula and NE coast
        for bx, by in ((-225, -165), (-235, -140), (-210, -185), (-240, -160), (225, 45), (215, 60), (235, 55)):
            for _ in range(6):
                trees.append((bx + rng.uniform(-18, 18), by + rng.uniform(-18, 18), rng.uniform(3.5, 5.0)))
        return trees


def render_plan_image(island, px_size=1024):
    from PIL import Image, ImageDraw, ImageFont

    img = Image.new("RGB", (px_size, px_size))
    pixels = img.load()
    half_m = island.half_m

    # Precompute height grid for shading
    grid = [[0.0] * px_size for _ in range(px_size)]
    road_grid = [[0.0] * px_size for _ in range(px_size)]
    stream_grid = [[0.0] * px_size for _ in range(px_size)]

    step_m = island.size_m / px_size
    for py in range(px_size):
        # Top of image is North (+X), bottom is South (-X)
        x_m = half_m - py * step_m
        for px in range(px_size):
            # Left is West (-Y), right is East (+Y)
            y_m = -half_m + px * step_m
            grid[py][px] = island.height_m(x_m, y_m)
            road_grid[py][px] = island.road_weight(x_m, y_m)
            stream_grid[py][px] = island.stream_depth(x_m, y_m)

    for py in range(px_size):
        x_m = half_m - py * step_m
        for px in range(px_size):
            y_m = -half_m + px * step_m
            h = grid[py][px]
            h_right = grid[py][min(px + 1, px_size - 1)]
            h_up = grid[max(py - 1, 0)][px]
            dx_h = (h_right - h) / step_m
            dy_h = (h_up - h) / step_m
            slope = math.degrees(math.atan(math.hypot(dx_h, dy_h)))
            light = 0.68 + 0.32 * max(-1.0, min(1.0, (-dx_h + dy_h) * 0.45))

            if h < -1.2:
                # Deep sea
                col = (34, 64, 96)
                light = 1.0
            elif h < -0.05:
                # Shallow water / stream / marsh pond
                t = max(0.0, min(1.0, (h + 1.2) / 1.15))
                col = (int(55 + 45 * t), int(88 + 45 * t), int(115 + 30 * t))
                light = 0.95
            else:
                # Land layers: grass, sand/road, rock
                grass = (92, 102, 58)
                # Darker forest moss in core forest
                if math.hypot((x_m - 15) / 150.0, (y_m - 10) / 170.0) < 1.0:
                    grass = (72, 82, 46)
                sand = (188, 175, 145)
                rock = (188, 192, 198)

                rw = road_grid[py][px]
                # Beach near water
                if h < 0.45 and stream_grid[py][px] < 0.05:
                    bw = 1.0 - _smooth(0.1, 0.45, h)
                    rw = max(rw, bw * 0.9)

                col = (
                    grass[0] * (1.0 - rw) + sand[0] * rw,
                    grass[1] * (1.0 - rw) + sand[1] * rw,
                    grass[2] * (1.0 - rw) + sand[2] * rw,
                )
                # Rock on steep slopes or high mountain peaks
                if slope > 18.0 or h > 4.2:
                    rk_w = min(1.0, max((slope - 18.0) / 14.0, (h - 4.2) / 6.0))
                    col = (
                        col[0] * (1.0 - rk_w) + rock[0] * rk_w,
                        col[1] * (1.0 - rk_w) + rock[1] * rk_w,
                        col[2] * (1.0 - rk_w) + rock[2] * rk_w,
                    )

            pixels[px, py] = tuple(int(max(0, min(255, c * light))) for c in col)

    draw = ImageDraw.Draw(img)

    def to_px(x_m, y_m):
        return (int((y_m + half_m) / island.size_m * px_size), int((half_m - x_m) / island.size_m * px_size))

    # Draw farm field rows in North (Brachfelder)
    for row_i in range(-3, 4):
        p1 = to_px(165 + row_i * 4, -80)
        p2 = to_px(150 + row_i * 4, -35)
        draw.line([p1, p2], fill=(155, 145, 112), width=2)
    for row_i in range(-3, 4):
        p1 = to_px(185 + row_i * 4, -20)
        p2 = to_px(198 + row_i * 4, 5)
        draw.line([p1, p2], fill=(155, 145, 112), width=2)

    # Draw low rock walls (Felsmauern)
    for wall in island.design.get("felsmauern", []):
        pts = [to_px(p[0], p[1]) for p in wall["punkte"]]
        draw.line(pts, fill=(165, 168, 172), width=4)

    # Draw trees with drop shadows
    for tx, ty, tr in island.trees:
        cx, cy = to_px(tx, ty)
        r_px = max(2, int(tr / island.size_m * px_size))
        draw.ellipse([cx - r_px + 2, cy - r_px + 2, cx + r_px + 2, cy + r_px + 2], fill=(28, 34, 20))
        draw.ellipse([cx - r_px, cy - r_px, cx + r_px, cy + r_px], fill=(54, 68, 36), outline=(42, 54, 28))

    # Draw wooden bridges and docks
    for br in island.design.get("bruecken", []):
        bx, by = br["x_m"], br["y_m"]
        ang = math.radians(br["drehung_grad"])
        hl = br["laenge_m"] * 0.5
        p1 = to_px(bx - hl * math.cos(ang), by - hl * math.sin(ang))
        p2 = to_px(bx + hl * math.cos(ang), by + hl * math.sin(ang))
        draw.line([p1, p2], fill=(148, 104, 62), width=5)

    # Draw wooden scaffolds (Holzgerüste) north of massif
    for sc in island.design.get("holzgerueste", []):
        p1 = to_px(*sc["von"])
        p2 = to_px(*sc["bis"])
        draw.line([p1, p2], fill=(162, 114, 68), width=4)

    # Draw tunnel dashed line through massif
    tun = island.design["tunnel"]
    tn = to_px(tun["eingang_nord"]["x_m"], tun["eingang_nord"]["y_m"])
    th = to_px(tun["halle_mitte"]["x_m"], tun["halle_mitte"]["y_m"])
    ts = to_px(tun["eingang_sued"]["x_m"], tun["eingang_sued"]["y_m"])
    tw = to_px(tun["eingang_west"]["x_m"], tun["eingang_west"]["y_m"])
    draw.line([tn, th, ts], fill=(255, 190, 60), width=3)
    draw.line([tw, th], fill=(255, 190, 60), width=3)
    draw.ellipse([th[0] - 8, th[1] - 8, th[0] + 8, th[1] + 8], outline=(255, 190, 60), width=2)

    # Draw meter grid every 100 m
    for m in range(-200, 250, 100):
        gx, _ = to_px(0, m)
        _, gy = to_px(m, 0)
        col = (255, 255, 255) if m == 0 else (180, 195, 210)
        draw.line([(gx, 0), (gx, px_size)], fill=col, width=1)
        draw.line([(0, gy), (px_size, gy)], fill=col, width=1)
        draw.text((gx + 4, 6), f"Y={m}m", fill=(220, 230, 240))
        draw.text((6, gy + 4), f"X={m}m", fill=(220, 230, 240))

    # Region labels
    for reg in island.design["regionen"]:
        rx, ry = reg["x_m"], reg["y_m"]
        if reg["id"] == "kernwald":
            rx, ry = 55, -20
        elif reg["id"] == "steinbruch_turm":
            rx, ry = 135, 130
        elif reg["id"] == "ostgrat":
            rx, ry = 5, 195
        px, py = to_px(rx, ry)
        label = reg["name"]
        w_box = max(52, int(len(label) * 3.2))
        draw.rectangle([px - w_box, py - 9, px + w_box, py + 9], fill=(20, 25, 30), outline=(90, 105, 115))
        draw.text((px - w_box + 5, py - 6), label, fill=(255, 245, 200))

    def draw_tag(px, py, text, color):
        tw = int(len(text) * 6.0) + 8
        tx = px - tw - 10 if px > px_size - 165 else px + 10
        ty = py - 7
        draw.rectangle([tx - 2, ty - 1, tx + tw, ty + 13], fill=(16, 22, 28))
        draw.text((tx + 2, ty), text, fill=color)

    # Draw markers for gameplay places
    # Arrivals (cyan circles)
    for a in island.design["ankunft"]:
        px, py = to_px(a["x_m"], a["y_m"])
        draw.ellipse([px - 7, py - 7, px + 7, py + 7], fill=(60, 200, 255), outline=(255, 255, 255), width=2)
        draw_tag(px, py, a["name"], (160, 235, 255))

    # Boss places (red circles)
    for b in island.design["bossplaetze"]:
        px, py = to_px(b["x_m"], b["y_m"])
        draw.ellipse([px - 8, py - 8, px + 8, py + 8], fill=(230, 55, 55), outline=(255, 255, 255), width=2)
        draw_tag(px, py, f"Boss: {b['name']}", (255, 165, 165))

    # Signal fires (orange circles)
    for f in island.design["signalfeuer"]:
        px, py = to_px(f["x_m"], f["y_m"])
        draw.ellipse([px - 7, py - 7, px + 7, py + 7], fill=(255, 145, 35), outline=(255, 255, 255), width=2)
        draw_tag(px, py, f["name"], (255, 210, 130))

    # Camps (yellow dots)
    for c in island.design["lager"]:
        px, py = to_px(c["x_m"], c["y_m"])
        r = 5 if c["groesse"] == "mittel" else 3
        draw.ellipse([px - r, py - r, px + r, py + r], fill=(250, 220, 60), outline=(40, 40, 40), width=1)

    # Legend in top-left
    draw.rectangle([10, 24, 345, 132], fill=(18, 24, 32), outline=(120, 140, 160))
    draw.text((18, 30), f"Plan: {island.design['arbeitsname']} ({int(island.size_m)}x{int(island.size_m)} m)", fill=(255, 255, 255))
    draw.text((18, 48), "Blau = Ankunft (4) | Rot = Bossplatz (4)", fill=(180, 225, 255))
    draw.text((18, 66), "Orange = Signalfeuer (4) | Gelb = Lager (18)", fill=(255, 215, 140))
    draw.text((18, 84), "Gelbe Linie = Tunnel durch das Eisenjoch", fill=(255, 200, 90))
    draw.text((18, 102), "Achsen: +X = Nord (oben), +Y = Ost (rechts)", fill=(200, 210, 220))

    return img


def main():
    from PIL import Image, ImageDraw

    island = Waldinsel()
    plan_img = render_plan_image(island, px_size=1024)

    single_path = os.path.join(ROOT, "Saved", "waldinsel_draufsicht.png")
    plan_img.save(single_path)

    ref_path = os.path.join(ROOT, "Saved", "Referenz", "waldkarte_draufsicht.png")
    if os.path.exists(ref_path):
        ref_img = Image.open(ref_path).convert("RGB").resize((1024, 1024), Image.LANCZOS)
        comp = Image.new("RGB", (2048, 1064), (18, 22, 28))
        comp.paste(ref_img, (0, 40))
        comp.paste(plan_img, (1024, 40))
        draw = ImageDraw.Draw(comp)
        draw.text((20, 12), "VORLAGE: Woodlands (Far Far West) - 2048px Draufsicht", fill=(255, 255, 255))
        draw.text((1044, 12), "EIGENER PLAN: Aschenwerder / Waldinsel (600 x 600 m, berechnet aus Data/waldinsel.json)", fill=(255, 235, 160))
        comp_path = os.path.join(ROOT, "Saved", "Referenz", "plan_vs_vorlage.png")
        comp.save(comp_path)
        print(comp_path)
    else:
        print(single_path)


if __name__ == "__main__":
    main()
