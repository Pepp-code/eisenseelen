"""Höhen und Grundform der Waldinsel Aschenwerder aus Data/gelaende.json und Data/laufkarte.json.

Reines Python ohne Unreal: make_runmap.py baut damit die Landschaft (600 x 600 m),
und `py -3.12 tools/gelaende.py [bild.png]` zeichnet eine Draufsicht zum Prüfen.
Achsen: +X ist Norden (oben), -X ist Süden (unten), +Y ist Osten (rechts), -Y ist Westen (links).
Jeder Spielort aus laufkarte.json und jeder große Sandplatz bleibt eben auf Höhe 0.
"""

import json
import math
import os
import random
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


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


def _polyline_distance(px, py, points):
    return min(_segment_distance(px, py, *points[i], *points[i + 1]) for i in range(len(points) - 1))


class Gelaende:
    def __init__(self, design=None, karte=None):
        self.design = design or _load("gelaende.json")
        self.karte = karte or _load("laufkarte.json")
        self.size_m = float(self.design.get("boden_m", 600.0))
        self.half_m = self.size_m * 0.5
        rng = random.Random(self.design.get("zufall", 13))
        heights_cfg = self.design.get("hoehen_m", {})
        wave_length = heights_cfg.get("wellen_laenge", 65.0)
        self.waves = []
        for factor in (1.0, 0.61, 1.73):
            angle = rng.uniform(0, math.pi)
            k = 2 * math.pi / (wave_length * factor)
            self.waves.append((math.cos(angle) * k, math.sin(angle) * k, rng.uniform(0, 2 * math.pi), 1.0 / factor))
        weight_sum = sum(w[3] for w in self.waves)
        self.wave_scale = heights_cfg.get("wellen_hoehe", 2.2) * 0.5 / weight_sum
        self.coast_angles = self._prepare_coast()
        self.flat_spots = self._collect_flat_spots()

    def _prepare_coast(self):
        pts = self.design.get("kueste_punkte", [])
        samples = []
        for x, y in pts:
            ang = math.atan2(y, x)
            r = math.hypot(x, y)
            samples.append((ang, r))
        samples.sort(key=lambda item: item[0])
        return samples

    def coast_radius(self, x, y):
        """Küstenradius in Richtung (x, y), weich zwischen den Stützpunkten interpoliert."""
        if x == 0 and y == 0:
            return self.design.get("insel_radius_m", 255.0)
        ang = math.atan2(y, x)
        samples = self.coast_angles
        n = len(samples)
        for i in range(n):
            a0, r0 = samples[i]
            a1, r1 = samples[(i + 1) % n]
            if i == n - 1:
                a1 += 2 * math.pi
                if ang < a0:
                    ang += 2 * math.pi
            if a0 <= ang <= a1:
                t = (ang - a0) / max(1e-6, a1 - a0)
                t_smooth = t * t * (3.0 - 2.0 * t)
                return r0 + (r1 - r0) * t_smooth
        return self.design.get("insel_radius_m", 255.0)

    def _collect_flat_spots(self):
        radii = self.design["eben_um"]
        spots = [(0.0, 0.0, radii.get("mitte_m", 16.0))]
        for s in self.design.get("sandplaetze", []):
            spots.append((s["x_m"], s["y_m"], s.get("radius_m", radii["sandplaetze_m"])))
        tun = self.design.get("tunnel")
        if tun and "halle_mitte" in tun:
            hm = tun["halle_mitte"]
            spots.append((hm["x_m"], hm["y_m"], hm.get("radius_m", 18.0)))
        for key in ("orte", "bossplaetze", "signalfeuer", "ankunft", "lager", "sprungfelder"):
            radius = radii.get(key + "_m", 10.0)
            for spot in self.karte.get(key, []):
                spots.append((spot["x_m"], spot["y_m"], radius))
        return spots

    def flat_weight(self, x, y, blend_override=None):
        """1.0 auf einem Spielort oder Sandplatz, 0.0 im freien Gelände, dazwischen weich."""
        blend = blend_override if blend_override is not None else self.design["eben_um"]["uebergang_m"]
        best = 0.0
        for sx, sy, radius in self.flat_spots:
            d = math.hypot(x - sx, y - sy)
            if d < radius + blend:
                best = max(best, 1.0 - _smooth(radius, radius + blend, d))
                if best >= 1.0:
                    return 1.0
        return best

    def road_weight(self, x, y):
        """1.0 auf einem Weg, einer Brücke oder im Tunnel-Einschnitt, 0.0 abseits davon."""
        best = 0.0
        for road in self.design.get("wege", {}).get("polylinien", []):
            half_w = road["breite_m"] * 0.5
            d = _polyline_distance(x, y, road["punkte"])
            if d < half_w + 4.0:
                best = max(best, 1.0 - _smooth(half_w, half_w + 4.0, d))
                if best >= 1.0:
                    return 1.0
        for b in self.design.get("bruecken", []):
            ang = math.radians(b["drehung_grad"])
            hl = b["laenge_m"] * 0.5 + 3.0
            p0 = (b["x_m"] - hl * math.cos(ang), b["y_m"] - hl * math.sin(ang))
            p1 = (b["x_m"] + hl * math.cos(ang), b["y_m"] + hl * math.sin(ang))
            half_w = b["breite_m"] * 0.5 + 2.0
            d = _segment_distance(x, y, p0[0], p0[1], p1[0], p1[1])
            if d < half_w + 6.0:
                best = max(best, 1.0 - _smooth(half_w, half_w + 6.0, d))
                if best >= 1.0:
                    return 1.0
        return best

    def bridge_bank_weight(self, x, y):
        """Hält die beiden Ufer-Aufgänge jeder Brücke auf Höhe 0, während unter der Brückenmitte das Wasser fließt."""
        best = 0.0
        for b in self.design.get("bruecken", []):
            ang = math.radians(b["drehung_grad"])
            hl = b["laenge_m"] * 0.5
            for sign in (-1.0, 1.0):
                bx = b["x_m"] + sign * (hl + 1.0) * math.cos(ang)
                by = b["y_m"] + sign * (hl + 1.0) * math.sin(ang)
                if math.hypot(bx, by) > self.coast_radius(bx, by):
                    continue
                d = math.hypot(x - bx, y - by)
                if d < 6.5:
                    best = max(best, 1.0 - _smooth(3.5, 6.5, d))
        return best

    def tunnel_cut_weight(self, x, y):
        """Hält die eingeschnittene Schlucht des Tunnels durch das Felsmassiv auf Höhe 0."""
        tun = self.design.get("tunnel")
        if not tun:
            return 0.0
        hm = tun["halle_mitte"]
        d_hall = math.hypot(x - hm["x_m"], y - hm["y_m"])
        best = 1.0 - _smooth(hm["radius_m"], hm["radius_m"] + 16.0, d_hall)
        half_w = tun.get("gang_breite_m", 10.0) * 0.5
        for key in ("eingang_nord", "eingang_sued", "eingang_west"):
            ep = tun[key]
            d_seg = _segment_distance(x, y, ep["x_m"], ep["y_m"], hm["x_m"], hm["y_m"])
            if d_seg < half_w + 16.0:
                best = max(best, 1.0 - _smooth(half_w, half_w + 16.0, d_seg))
        return best

    def stream_depth(self, x, y):
        """Positive Tiefe (in m) für Bäche und Sumpftümpel."""
        depth = 0.0
        for stream in self.design.get("baeche", []):
            half_w = stream["sohle_m"] * 0.5
            bank = stream["ufer_m"]
            d = _polyline_distance(x, y, stream["punkte"])
            if d < half_w + bank:
                depth = max(depth, stream["tiefe_m"] * (1.0 - _smooth(half_w, half_w + bank, d)))
        for px, py, pr in self.design.get("sumpf_tuempel", []):
            d = math.hypot(x - px, y - py)
            if d < pr + 4.0:
                depth = max(depth, 0.55 * (1.0 - _smooth(pr * 0.6, pr + 4.0, d)))
        return depth

    def elevation_hills(self, x, y):
        """Felsmassiv (Eisenjoch), Westberg (Wächterkuppe), Nordwest-Kap, Ostgrat und Schluchtring."""
        top = 0.0
        for h in self.design.get("erhebungen", []):
            if "innen_radius_m" in h:
                dx, dy = x - h["x_m"], y - h["y_m"]
                d = math.hypot(dx, dy)
                r_in, r_out = h["innen_radius_m"], h["aussen_radius_m"]
                if r_in - 6.0 < d < r_out + 12.0:
                    ring_val = (1.0 - _smooth(r_out, r_out + 10.0, d)) * _smooth(r_in - 4.0, r_in + 4.0, d)
                    ang_deg = math.degrees(math.atan2(dy, dx)) % 360.0
                    for gap_deg in h.get("luecke_grad", []):
                        diff = abs((ang_deg - gap_deg + 180.0) % 360.0 - 180.0)
                        if diff < 28.0:
                            ring_val *= _smooth(12.0, 28.0, diff)
                    top = max(top, h["hoehe_m"] * ring_val)
                continue

            dx, dy = x - h["x_m"], y - h["y_m"]
            rx, ry = h["radius_x_m"], h["radius_y_m"]
            norm_d = math.hypot(dx / rx, dy / ry)
            if norm_d < 1.65:
                falloff = 1.0 - _smooth(0.45, 1.45, norm_d)
                val = h["hoehe_m"] * falloff
                if "rampe_grad" in h:
                    ang = math.radians(h["rampe_grad"])
                    along = dx * math.cos(ang) + dy * math.sin(ang)
                    side = abs(-dx * math.sin(ang) + dy * math.cos(ang))
                    if along > 0 and side < 14.0:
                        ramp_len = max(rx, ry) * 1.45
                        ramp_h = h["hoehe_m"] * (1.0 - _smooth(0.0, ramp_len, along))
                        edge = 1.0 - _smooth(8.0, 14.0, side)
                        val = max(val, ramp_h * edge)
                top = max(top, val)
        return top

    def is_dry_land(self, x, y):
        """True, wenn der Punkt sicher auf dem trockenen Inselinneren liegt (für Truhen und Deckung)."""
        r = math.hypot(x, y)
        if r > self.coast_radius(x, y) - 18.0:
            return False
        if self.stream_depth(x, y) > 0.1:
            return False
        for s in self.design.get("senken", []):
            if math.hypot(x - s["x_m"], y - s["y_m"]) < s["radius_m"] + 8.0:
                return False
        return True

    def height_m(self, x, y):
        """Geländehöhe in Metern an der Stelle (x = Nord, y = Ost)."""
        r = math.hypot(x, y)
        coast_r = self.coast_radius(x, y)
        heights_cfg = self.design["hoehen_m"]

        # Außerhalb der Küstenlinie: Strand -> Flachwasser -> Meeresboden (-4 m)
        if r > coast_r:
            over = r - coast_r
            if over < 16.0:
                t = _smooth(0.0, 16.0, over)
                return heights_cfg["strand"] * (1.0 - t) + heights_cfg["flachwasser"] * t
            t = _smooth(16.0, 52.0, over)
            return heights_cfg["flachwasser"] * (1.0 - t) + heights_cfg["meeresboden"] * t

        # Auf der Insel: Wiesen und Wälder liegen ab +0.38 m (+38 cm), Wege und Plätze auf 0.0 m (Sandschicht)
        coast_blend = _smooth(coast_r - 18.0, coast_r, r)
        waves = 0.38 + self.wave_scale * sum(w * (1.0 + math.sin(kx * x + ky * y + phase)) for kx, ky, phase, w in self.waves)

        # Berge weichen um Spielorte, Wege und den Tunnel-Einschnitt sanft zurück
        hill_Blend = self.design["eben_um"].get("uebergang_berg_m", 16.0)
        hill_suppress = max(
            self.flat_weight(x, y, blend_override=hill_Blend),
            self.tunnel_cut_weight(x, y),
            self.road_weight(x, y),
        )
        hills = self.elevation_hills(x, y) * (1.0 - hill_suppress)

        sinks = 0.0
        for s in self.design.get("senken", []):
            d = math.hypot(x - s["x_m"], y - s["y_m"])
            if d < s["radius_m"] + 12.0:
                sinks += s["tiefe_m"] * (1.0 - _smooth(s["radius_m"] * 0.65, s["radius_m"] + 10.0, d))

        streams = self.stream_depth(x, y)
        flat = max(self.flat_weight(x, y), self.tunnel_cut_weight(x, y), self.bridge_bank_weight(x, y))
        road = self.road_weight(x, y)
        pave = max(flat, road)

        # Spielorte und Wege liegen auf Höhe 0 (Sandschicht in M_Gelaende); Bäche fließen unter den Brücken durch
        dry_h = (waves + hills - sinks) * (1.0 - pave)
        beach_h = heights_cfg["strand"] * (1.0 - coast_blend)
        land_h = dry_h * (1.0 - coast_blend) + beach_h * coast_blend
        return land_h * (1.0 - pave) - streams * (1.0 - flat)

    def slope_deg(self, x, y, step_m=1.0):
        """Geländeneigung in Grad an der Stelle (x, y)."""
        h = self.height_m(x, y)
        dx = (self.height_m(x + step_m, y) - h) / step_m
        dy = (self.height_m(x, y + step_m) - h) / step_m
        return math.degrees(math.atan(math.hypot(dx, dy)))

    def is_forest_clear(self, x, y, allow_shallow_water=False):
        """Prüft, dass ein Baum oder Busch keinen Weg, Platz, Tunnel, Felsen oder Boss-Korridor versperrt."""
        wald_cfg = self.design.get("wald", {})
        road_pad = wald_cfg.get("weg_puffer_m", 5.0)
        spot_pad = wald_cfg.get("platz_puffer_m", 5.0)

        r = math.hypot(x, y)
        coast_r = self.coast_radius(x, y)
        if allow_shallow_water:
            if r > coast_r + 12.0:
                return False
        else:
            if r > coast_r - 10.0 or self.stream_depth(x, y) > 0.05:
                return False

        if self.flat_weight(x, y, blend_override=spot_pad) > 0.01:
            return False
        if self.tunnel_cut_weight(x, y) > 0.01 or self.bridge_bank_weight(x, y) > 0.01:
            return False

        for road in self.design.get("wege", {}).get("polylinien", []):
            if _polyline_distance(x, y, road["punkte"]) < road["breite_m"] * 0.5 + road_pad:
                return False

        for b in self.design.get("bruecken", []):
            ang = math.radians(b["drehung_grad"])
            hl = b["laenge_m"] * 0.5 + 3.0
            p0 = (b["x_m"] - hl * math.cos(ang), b["y_m"] - hl * math.sin(ang))
            p1 = (b["x_m"] + hl * math.cos(ang), b["y_m"] + hl * math.sin(ang))
            if _segment_distance(x, y, p0[0], p0[1], p1[0], p1[1]) < b["breite_m"] * 0.5 + 6.0:
                return False

        # Boss-Rücklaufkorridor (48 m nach Norden für FEIBossRunTest) freihalten
        for bp in self.karte.get("bossplaetze", []):
            if _segment_distance(x, y, bp["x_m"], bp["y_m"], bp["x_m"] + 54.0, bp["y_m"]) < 7.0:
                return False

        # Sprungfelder samt Landeturm (+3.5 m Nord) freihalten
        for pad in self.karte.get("sprungfelder", []):
            if math.hypot(x - (pad["x_m"] + 3.5), y - pad["y_m"]) < 12.0:
                return False

        # Felsen und Felsmauern freihalten
        felsen = self.design.get("felsen", {})
        for kl in felsen.get("klippen", []):
            rad = max(kl["groesse_m"][0], kl["groesse_m"][1]) * 0.5 + 3.0
            if math.hypot(x - kl["x_m"], y - kl["y_m"]) < rad:
                return False
        for mw in felsen.get("felsmauern", []):
            ang = math.radians(mw.get("drehung_grad", 0.0))
            hl = mw["laenge_m"] * 0.5
            p0 = (mw["x_m"] - hl * math.cos(ang), mw["y_m"] - hl * math.sin(ang))
            p1 = (mw["x_m"] + hl * math.cos(ang), mw["y_m"] + hl * math.sin(ang))
            if _segment_distance(x, y, p0[0], p0[1], p1[0], p1[1]) < mw["breite_m"] * 0.5 + 3.5:
                return False

        for c in wald_cfg.get("freihalten_kreise", []):
            if math.hypot(x - c["x_m"], y - c["y_m"]) < c["radius_m"]:
                return False

        h = self.height_m(x, y)
        if allow_shallow_water:
            if not (-0.85 <= h <= 1.2):
                return False
        else:
            if not (0.15 <= h <= 17.5):
                return False

        if self.slope_deg(x, y) > 32.0:
            return False
        return True

    def forest_density(self, x, y):
        """Walddichte (0.0 bis 0.85) an der Stelle (x, y)."""
        if not self.is_forest_clear(x, y, allow_shallow_water=False):
            return 0.0
        dichte = self.design.get("wald", {}).get("dichte", {})
        offen = dichte.get("offen", 0.08)
        kern = dichte.get("kernwald", 0.85)
        uebergang = dichte.get("uebergang", 0.36)

        # Offene Regionen: Brachfelder (Nord), Steinbruch & Eisenturm (Nordost), Wrackküste (West)
        if x > 115.0 and -105.0 < y < 45.0:
            return offen
        if x > 75.0 and y > 45.0 and math.hypot(x - 135.0, y - 120.0) < 95.0:
            return offen
        if y < -175.0 and -125.0 < x < 35.0:
            return offen

        # Dichter Kernwald rund um Eisenjoch, Wächterkuppe und Seelenforst
        d_core = math.hypot((x - 10.0) / 135.0, (y - (-10.0)) / 155.0)
        d_south = math.hypot((x - (-65.0)) / 80.0, (y - (-20.0)) / 105.0)
        if d_core < 1.0 or d_south < 1.0:
            return kern
        return uebergang

    def generate_forest(self):
        """Erzeugt deterministisch Kiefern, tote Bäume und Büsche für Planbild und Laufkarte."""
        wald_cfg = self.design.get("wald", {})
        rng = random.Random(self.design.get("zufall", 13) + wald_cfg.get("zufall_versatz", 99))
        step = float(wald_cfg.get("raster_m", 10.0))
        jitter = float(wald_cfg.get("streuung_m", 3.4))
        half = self.half_m - 25.0

        items = []
        trunks = []

        def trunk_free(px, py, min_dist=4.5):
            return all(math.hypot(px - tx, py - ty) >= min_dist for tx, ty in trunks)

        # 1. Kiefern und Sumpfbäume über das Inselraster
        x = -half
        while x <= half:
            y = -half
            while y <= half:
                jx = x + rng.uniform(-jitter, jitter)
                jy = y + rng.uniform(-jitter, jitter)
                dens = self.forest_density(jx, jy)
                if dens > 0.0 and rng.random() < dens and trunk_free(jx, jy, 4.8):
                    z = self.height_m(jx, jy)
                    in_moor = jx < -65.0 and jy > 70.0
                    in_marsch = jx > 40.0 and jy < -95.0 and z < 0.55
                    if (in_moor and rng.random() < 0.65) or (in_marsch and rng.random() < 0.35):
                        items.append({
                            "art": "baum_tot",
                            "x_m": round(jx, 2),
                            "y_m": round(jy, 2),
                            "z_m": round(z, 2),
                            "breite_m": round(rng.uniform(1.8, 2.6), 2),
                            "hoehe_m": round(rng.uniform(7.5, 10.8), 2),
                            "stamm_m": round(rng.uniform(0.58, 0.76), 2),
                            "drehung_grad": round(rng.uniform(0.0, 360.0), 1),
                        })
                    else:
                        items.append({
                            "art": "kiefer",
                            "x_m": round(jx, 2),
                            "y_m": round(jy, 2),
                            "z_m": round(z, 2),
                            "breite_m": round(rng.uniform(3.8, 5.2), 2),
                            "hoehe_m": round(rng.uniform(10.8, 14.2), 2),
                            "stamm_m": round(rng.uniform(0.72, 0.90), 2),
                            "drehung_grad": round(rng.uniform(0.0, 360.0), 1),
                        })
                    trunks.append((jx, jy))

                # Unterholz-Busch versetzt prüfen (ohne Kollision)
                bx = x + rng.uniform(-jitter, jitter)
                by = y + rng.uniform(-jitter, jitter)
                b_dens = self.forest_density(bx, by)
                if b_dens > 0.0 and rng.random() < b_dens * 0.42 and trunk_free(bx, by, 2.4):
                    bz = self.height_m(bx, by)
                    items.append({
                        "art": "busch",
                        "x_m": round(bx, 2),
                        "y_m": round(by, 2),
                        "z_m": round(bz, 2),
                        "breite_m": round(rng.uniform(1.6, 2.5), 2),
                        "hoehe_m": round(rng.uniform(1.0, 1.45), 2),
                        "stamm_m": 0.0,
                        "drehung_grad": round(rng.uniform(0.0, 360.0), 1),
                    })
                y += step
            x += step

        # 2. Gezielte Gruppen toter Bäume im Bleichen Moor, in der Nebelmarsch und im Küsten-Flachwasser
        for grp in wald_cfg.get("tote_baeume_gruppen", []):
            gx, gy, grad = grp["x_m"], grp["y_m"], grp["radius_m"]
            placed = 0
            attempts = 0
            while placed < grp["anzahl"] and attempts < grp["anzahl"] * 12:
                attempts += 1
                ang = rng.uniform(0.0, 2.0 * math.pi)
                dist = grad * math.sqrt(rng.random())
                tx = gx + dist * math.cos(ang)
                ty = gy + dist * math.sin(ang)
                if not self.is_forest_clear(tx, ty, allow_shallow_water=True) or not trunk_free(tx, ty, 4.2):
                    continue
                tz = self.height_m(tx, ty)
                items.append({
                    "art": "baum_tot",
                    "x_m": round(tx, 2),
                    "y_m": round(ty, 2),
                    "z_m": round(tz, 2),
                    "breite_m": round(rng.uniform(1.8, 2.5), 2),
                    "hoehe_m": round(rng.uniform(7.5, 11.0), 2),
                    "stamm_m": round(rng.uniform(0.58, 0.76), 2),
                    "drehung_grad": round(rng.uniform(0.0, 360.0), 1),
                })
                trunks.append((tx, ty))
                placed += 1

        return items

    def resolution(self):
        res = self.design["aufloesung"]
        side = res["teile_pro_seite"] * res["quadrate_pro_teil"] + 1
        return res["teile_pro_seite"], res["quadrate_pro_teil"], res["quadrat_cm"], side

    def grid_cm(self):
        """Höhen aller Eckpunkte in Zentimetern, Zeile für Zeile (Y außen, X innen für Unreal Landscape)."""
        _, _, quad_cm, side = self.resolution()
        half_m = (side - 1) * quad_cm / 200.0
        step_m = quad_cm / 100.0
        heights = []
        for row in range(side):
            y = -half_m + row * step_m
            for column in range(side):
                x = -half_m + column * step_m
                heights.append(self.height_m(x, y) * 100.0)
        return heights


def draw_preview(path):
    from PIL import Image, ImageDraw

    terrain = Gelaende()
    _, _, quad_cm, side = terrain.resolution()
    heights = terrain.grid_cm()
    low, high = min(heights), max(heights)
    scale = 2
    image = Image.new("RGB", (side, side))
    pixels = image.load()
    half_m = (side - 1) * quad_cm / 200.0
    step_m = quad_cm / 100.0

    # In der Draufsicht zeigt +X nach Norden (oben) und +Y nach Osten (rechts),
    # passend zur Auftragskarte im Spiel (AEIHud::DrawContractBoard).
    for row_y in range(side):
        y_m = -half_m + row_y * step_m
        for col_x in range(side):
            x_m = -half_m + col_x * step_m
            h = heights[row_y * side + col_x]
            h_east = heights[min(row_y + 1, side - 1) * side + col_x]
            h_north = heights[row_y * side + min(col_x + 1, side - 1)]
            slope = math.degrees(math.atan(math.hypot(h_east - h, h_north - h) / quad_cm))
            light = 0.65 + 0.35 * max(-1.0, min(1.0, ((h - h_east) + (h_north - h)) / quad_cm * 1.2))

            if h < -120.0:
                color = (34, 64, 96)
                light = 1.0
            elif h < -10.0:
                t = max(0.0, min(1.0, (h + 120.0) / 110.0))
                color = (55 + 45 * t, 88 + 45 * t, 115 + 30 * t)
                light = 0.95
            else:
                rw = terrain.road_weight(x_m, y_m)
                if h < 40.0 and terrain.stream_depth(x_m, y_m) < 0.05:
                    rw = max(rw, 1.0 - _smooth(10.0, 40.0, h))
                grass = (88, 102, 56)
                sand = (188, 175, 145)
                color = (
                    grass[0] * (1.0 - rw) + sand[0] * rw,
                    grass[1] * (1.0 - rw) + sand[1] * rw,
                    grass[2] * (1.0 - rw) + sand[2] * rw,
                )
                if slope > 22.0 or h > 450.0:
                    rk = min(1.0, max((slope - 22.0) / 16.0, (h - 450.0) / 600.0))
                    rock = (185, 190, 196)
                    color = (
                        color[0] * (1.0 - rk) + rock[0] * rk,
                        color[1] * (1.0 - rk) + rock[1] * rk,
                        color[2] * (1.0 - rk) + rock[2] * rk,
                    )
                if slope > 45.0:
                    color = (210, 65, 55)

            px = row_y
            py = side - 1 - col_x
            pixels[px, py] = tuple(int(max(0, min(255, c * light))) for c in color)

    image = image.resize((side * scale, side * scale), Image.NEAREST)
    draw = ImageDraw.Draw(image)
    half = (side - 1) / 2.0

    def to_pixel(x_m, y_m):
        return ((y_m + half) * scale, (half - x_m) * scale)

    for x_m, y_m, radius in terrain.flat_spots:
        px, py = to_pixel(x_m, y_m)
        draw.ellipse([px - radius * scale, py - radius * scale, px + radius * scale, py + radius * scale], outline=(255, 255, 255), width=1)
    for b in terrain.design.get("bruecken", []):
        ang = math.radians(b["drehung_grad"])
        hl = b["laenge_m"] * 0.5
        p0 = to_pixel(b["x_m"] - hl * math.cos(ang), b["y_m"] - hl * math.sin(ang))
        p1 = to_pixel(b["x_m"] + hl * math.cos(ang), b["y_m"] + hl * math.sin(ang))
        draw.line([p0, p1], fill=(165, 110, 55), width=int(b["breite_m"] * scale))
    felsen = terrain.design.get("felsen", {})
    for kl in felsen.get("klippen", []):
        px, py = to_pixel(kl["x_m"], kl["y_m"])
        rx, ry = kl["groesse_m"][1] * 0.5 * scale, kl["groesse_m"][0] * 0.5 * scale
        draw.ellipse([px - rx, py - ry, px + rx, py + ry], fill=(120, 126, 134), outline=(70, 74, 80), width=1)
    for mw in felsen.get("felsmauern", []):
        ang = math.radians(mw["drehung_grad"])
        hl = mw["laenge_m"] * 0.5
        p0 = to_pixel(mw["x_m"] - hl * math.cos(ang), mw["y_m"] - hl * math.sin(ang))
        p1 = to_pixel(mw["x_m"] + hl * math.cos(ang), mw["y_m"] + hl * math.sin(ang))
        draw.line([p0, p1], fill=(105, 110, 118), width=max(2, int(mw["breite_m"] * scale)))
    forest = terrain.generate_forest()
    for item in forest:
        px, py = to_pixel(item["x_m"], item["y_m"])
        rad = max(2.0, item["breite_m"] * 0.5 * scale)
        if item["art"] == "busch":
            draw.ellipse([px - rad, py - rad, px + rad, py + rad], fill=(56, 88, 38))
        elif item["art"] == "baum_tot":
            draw.ellipse([px - rad, py - rad, px + rad, py + rad], fill=(142, 128, 108), outline=(78, 68, 56), width=1)
        else:
            draw.ellipse([px - rad, py - rad, px + rad, py + rad], fill=(32, 64, 24), outline=(18, 42, 14), width=1)
    tun = terrain.design.get("tunnel")
    if tun:
        hm = tun["halle_mitte"]
        hx, hy = to_pixel(hm["x_m"], hm["y_m"])
        hr = hm["radius_m"] * scale
        draw.ellipse([hx - hr, hy - hr, hx + hr, hy + hr], outline=(240, 160, 60), width=2)
        for key in ("eingang_nord", "eingang_sued", "eingang_west"):
            ep = tun[key]
            ex, ey = to_pixel(ep["x_m"], ep["y_m"])
            draw.line([(hx, hy), (ex, ey)], fill=(240, 160, 60), width=2)
            draw.ellipse([ex - 5, ey - 5, ex + 5, ey + 5], fill=(240, 160, 60))
    for reg in terrain.design.get("regionen", []):
        px, py = to_pixel(reg["x_m"], reg["y_m"])
        draw.rectangle([px - 52, py - 8, px + 52, py + 8], fill=(20, 25, 30))
        draw.text((px - 48, py - 6), reg["name"], fill=(255, 245, 200))
    n_pines = sum(1 for item in forest if item["art"] == "kiefer")
    n_dead = sum(1 for item in forest if item["art"] == "baum_tot")
    n_bushes = sum(1 for item in forest if item["art"] == "busch")
    draw.text((12, 12), f"Aschenwerder ({int(terrain.size_m)}x{int(terrain.size_m)} m): Höhe {low / 100:.1f} bis {high / 100:.1f} m | Wald: {n_pines} Kiefern, {n_dead} tote Bäume, {n_bushes} Büsche", fill=(255, 255, 255))
    image.save(path)
    return path


if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "Saved", "gelaende_draufsicht.png")
    print(draw_preview(target))
