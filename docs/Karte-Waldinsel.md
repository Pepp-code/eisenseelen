# Karte: Waldinsel (Arbeitsname „Nebelholm“)

Stand: 6. November 2026 · Schritte K0 (Bestandsaufnahme) und K1 (Detailplan)

Dieser Plan ersetzt die bisherige Wüsten-Laufkarte (`300 × 300 m` mit Randhängen aus W1) durch eine bewaldete Insel im Meer nach dem Vorbild „Woodlands“ aus Far Far West. Sie ist ein Vorbild, keine Kopie: Alle Namen sind eigene deutsche Begriffe passend zu Eisenseelen (leere Rüstungen, Seelen, Asche, Moor), und die genaue Lage der Wege, Hügel und Orte ist für unseren Spielablauf mit der Himmelsbarke berechnet.

Die Geometrie der Insel steht vollständig in [`Data/waldinsel.json`](file:///C:/Users/Phillip/dev/game-project/Data/waldinsel.json). Das Skript [`tools/plan_waldinsel.py`](file:///C:/Users/Phillip/dev/game-project/tools/plan_waldinsel.py) berechnet daraus ohne Unreal die Höhen, Küsten, Bäche, Wege, Walddichte und Spielorte und legt das Vergleichsbild unter `Saved/Referenz/plan_vs_vorlage.png` ab.

---

## Teil 1: K0 Bestandsaufnahme (ohne Änderungen am Spiel)

### 1.1 Was an der Vorlage beobachtet und an den Bildern geprüft wurde

Anhand von `Saved/Referenz/waldkarte_draufsicht.png` (`2048 × 2048 px`, Norden oben), den vier Vierteln (`waldkarte_nw.png`, `_ne`, `_sw`, `_se`) sowie den noch schärferen Kacheln der Zoomstufe `z=2` (8 × 8 Kacheln zu je `512 × 512 px`, also `4096 × 4096 px` Gesamtauflösung – geprüft: `z=2` liefert volle Details) ergibt sich folgendes Bild:

1. **Insel im Meer:** Rundlich-eckiger Umriss, umgeben von einem seichten Uferstreifen (hellblau/türkis mit Sandbänken) und tiefem, dunkelblauem Meer. Im Vorbild legt man per Boot an; bei uns landet die Himmelsbarke an 4 Küstenstellen dicht am Strand.
2. **Zentrales Felsmassiv (Mitte bis leicht östlich):** Ein langgezogener, schroffer Felskamm von West nach Ost, auf dem Kiefern wachsen. Mitten hindurch führt ein verzweigtes Höhlen- und Tunnelsystem mit einem Banditenlager im Inneren, das Nord- und Südhälfte direkt verbindet.
3. **Zweiter Felsberg (westlich der Mitte):** Ein runder, steiler Felsstock, ebenfalls mit Kiefern bewachsen, umgeben von schmalen Schluchtwegen und kleinen Holzbrücken.
4. **Kiefernwald um beide Felsen:** Dichter Nadelwald im gesamten Inselinneren, der Sichtlinien bricht. Nach außen öffnet er sich zu Wiesen, Marschland, Strand und Ackerflächen.
5. **Norden (Bauernhof):** Geordnete Ackerreihen, ein großes Hofgebäude (Scheune) mit Holzzäunen, Kisten und Heuballen, umgeben von offenen Wiesen mit vielen kleinen Sandflecken und einer runden Sandzunge ganz im Norden.
6. **Nordosten (Steinbruch, Hütten, Metallturm):** Offene Wiesen mit Findlingen, eine große abgesenkte Grube (Steinbruch/Schürfgrube), einzelne Holzhütten, schroffe Küstenfelsen, abgestorbene Bäume im Flachwasser und ein hoher Metallturm als weithin sichtbares Wahrzeichen.
7. **Nordwesten (Kap, Marsch, Felsring):**
   - Ein steiles, helles Felskap mit Klippen direkt am Nordwestmeer und einem vorgelagerten Felsen im Wasser.
   - Dahinter nasses Marschland mit verzweigten Bächen, Sandinseln und Holzbrücken.
   - Südlich davon eine ringförmige Felsschlucht mit Wassergraben, die ein erhöhtes Sandplateau mit Lager umschließt; vier Holzbrücken führen über den Graben hinein.
8. **Westen (Sandküste mit Wrack):** Ein langer, heller Sandstrand, vor dem ein zerbrochenes Holzschiff mit Masten im flachen Wasser liegt, begleitet von langgezogenen Felsmauern parallel zum Ufer.
9. **Südwesten (Halbinsel mit Leuchtturm):** Eine felsig-sandige Landzunge, auf der ein Leuchtturm mit Holzsteg steht, umgeben von vielen einzelnen Bäumen direkt im seichten Meerwasser.
10. **Süden (Stegdorf):** Ein Fischer- und Stegdorf auf Holzpfählen mit langen Plankenwegen übers Wasser, kleinen Booten und einem gesunkenen/ankernden Schiff weiter draußen vor der Südküste.
11. **Südosten (Sumpf und weiße Felsplatten):** Ein weites Sumpfbecken voller kleiner Tümpel und Grasinseln, gesäumt von hellweißen, geschichteten Küstenfelsen und langen Reihen aus Holzpfählen im Meer.
12. **Osten (Felsgrat und Rinnen):** Ein Ausläufer des Hauptmassivs reicht bis an die Ostküste; südlich davon schneidet eine tiefe Wasserrinne ins Land, die von einer langen Holzbrücke überspannt wird.
13. **Große Sandplätze (Aufgabenringe):** In der Draufsicht sind 12 bis 13 kreisrunde, völlig ebene Sandflächen zu erkennen (Durchmesser jeweils etwa 1/12 der Inselbreite). Sie liegen im Ring um die Mitte und an den Küstenbuchten und dienen als freie Kampfflächen für Bosse und Aufträge.
14. **Wegenetz und Felsmauern:** Ein breiter Sandweg läuft als Ring um das Massiv und die Westkuppe; davon zweigen Speichen zu allen Küsten ab. Niedrige, langgezogene Felsmauern fassen viele Wege wie natürliche Schützengräben und Gassen ein.
15. **Holzgerüste nördlich des Massivs (geprüft auf Zoomstufe `z=2`):** Es sind zwei lange, gerade Holzstege auf hohen Stützpfeilern (Laufbrücken/Kranbahnen der Banditen), die aus dem nördlichen Waldlager direkt an die steile Nordwand des Felsmassivs heranführen. Dort oben liegt u. a. eines der versteckten Gräber.
16. **Sammelkram und Geheimnis (nur als Idee für später notiert, wird nicht umgesetzt):** Im Vorbild gibt es 10 versteckte Gräber, 3 Schallplatten-Teile, ein violettes X-Symbol und die geheime Pilzhöhle (7 Pilze in der Reihenfolge eines Pilzsteins essen).

---

### 1.2 Was im Code und in den Daten an der Karte hängt

Die folgende Tabelle zeigt alle Dateien, Werkzeuge, C++-Klassen und Tests, die die Laufkarte lesen oder verändern:

| Datei / Klasse / Test | Was liest die Karte? | Was setzt feste Koordinaten voraus? | Was bricht bei einer neuen Inselform? |
| --- | --- | --- | --- |
| [`Data/laufkarte.json`](file:///C:/Users/Phillip/dev/game-project/Data/laufkarte.json) | `boden_m` (300), `rand_hoehe_m` (3), `gebiet` („Aschesteppe“), `anlegestelle`, `signalfeuer` (4), `bossplaetze` (4), `ankunft` (4), `orte` (8), `sprungfelder` (3), `lager` (18), `truhen` (14), `deckung_anzahl` (30) | Alle Koordinaten liegen im Quadrat `-138 m` bis `+138 m`. Die Anlegestelle liegt bei `(0, -6000, 150)`. | Wenn `boden_m` auf `550–600 m` wächst, müssen alle 4 Ankünfte, 4 Bossplätze, 4 Feuer, 18 Lager, Orte und Sprungfelder auf die neuen Insel-Koordinaten umgezogen werden. |
| [`Data/gelaende.json`](file:///C:/Users/Phillip/dev/game-project/Data/gelaende.json) & [`tools/gelaende.py`](file:///C:/Users/Phillip/dev/game-project/tools/gelaende.py) | `wellen_hoehe_m`, `rand` (18 m hohe Randhänge), `eben_um`, `tafelberge` (5), `flussbett`, `aufloesung` (`5 × 63`, 100 cm). | `KARTE_HALB_M = 150.0` ist in `gelaende.py` fest eingetragen; `_rim()` zieht am quadratischen Rand bei `±150 m` steile Wände hoch. | Eine Insel hat **keine Hänge am Rand**, sondern fällt ins Meer ab (`-4 m`). Zudem braucht eine doppelt so große Karte `10 × 63` Komponenten statt `5 × 63`. |
| [`tools/make_runmap.py`](file:///C:/Users/Phillip/dev/game-project/tools/make_runmap.py) | Baut `/Game/Maps/L_ErsterLauf` aus `laufkarte.json` und `Gelaende`: Landschaft, Randmauern (`add_walls`), Ruinen-Orte, Deckung, Lager, Truhen, Sprungfelder, `NavMeshBoundsVolume`, Signalfeuer, Anlegestelle, Licht/Nebel. | Zufällige Truhen (`add_chests`) und Deckung (`add_cover`) würfeln im vollen Quadrat `[-half, +half]`. `is_open_ground` prüft `abs(height) <= 2.5 m`. `NAVIGATION_BOTTOM_M = -10`, `NAVIGATION_TOP_M = 30`. | Im Quadrat `[-half, +half]` liegen die Ecken jetzt **im Meer**! `is_open_ground` muss zusätzlich prüfen, dass der Punkt auf dem Land (nicht im Wasser/Strand unter `0,3 m`) und außerhalb des Tunnels liegt. Statt sichtbarer Randmauern (`rand_hoehe_m: 3`) braucht der Rand im Meer unsichtbare Sperrwände. |
| [`tools/make_assets.py`](file:///C:/Users/Phillip/dev/game-project/tools/make_assets.py) | Erzeugt `M_Gelaende` (Steppen-Orange `TERRAIN_ASH` auf flachem Boden, dunkler Fels `TERRAIN_ROCK` an Hängen) sowie `M_Schachbrett`, `M_Farbe`, `M_Glut`, `M_Ueberzug`. | Keine Koordinaten, aber feste Wüstenfarben (`0.32, 0.22, 0.13`). | Für die Waldinsel braucht `M_Gelaende` Schichten für **Gras/Waldboden, Sand/Weg, Fels, Schlamm und Strand** sowie ein Wassermaterial `M_Wasser`. |
| [`tools/preview_run.py`](file:///C:/Users/Phillip/dev/game-project/tools/preview_run.py) | Macht die Vorschaubilder für `tools/preview.ps1 -Szene lauf`. | **Zeile 207:** `(0, 0)` für das Blutmond-Bild. **Zeile 216:** feste Kamerapunkte `(0, -12000, 4500)` und `(-11000, -11000, 4500)` für die Gelände-Übersicht. | Bei `(0, 0)` steht auf der Waldinsel das Felsmassiv; und `±120 m` ist bei einer `600 m`-Karte mitten im Wald statt am Südrand. Die Übersichtspunkte müssen aus `boden_m` berechnet werden. |
| [`UEIWorldTools::BuildLandscape`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/EIWorldTools.cpp#L18-L55) | Baut den `ALandscape`-Actor zentriert um `(0, 0, 0)` aus `HeightsCm`, `ComponentsPerSide`, `QuadsPerComponent`, `QuadSizeCm`. | Keine festen Koordinaten; Höhen von `-256 m` bis `+256 m` passen in `uint16`. | Bricht **nicht** durch Größe oder Form. Für Bemalungsschichten (Gras, Weg, Fels, Schlamm) oder ein Sichtbarkeits-Loch (Variante B beim Tunnel) müsste `Import()` um `PaintPerLayer` ergänzt werden. |
| [`UEIHordeDirector`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/EIHordeDirector.cpp#L123-L236) | Spawnt Gegner-Gruppen in Lagern und Blutmond-Wellen im Umkreis (`MinDistanceMeters` 18 m bis `MaxDistanceMeters` 38 m) außerhalb des Sichtfelds. | `FindStandingPlace` macht einen LineTrace (`+100 m` bis `-50 m`). Auf `ALandscapeProxy` wird **jede** Höhe akzeptiert; auf StaticMeshes gilt `Floor.ImpactPoint.Z <= 50 cm` (`MaxGroundHeightCm`). | **Zwei wichtige Fallen:** 1. Am Ufer ist auch der Meeresboden Teil des `ALandscapeProxy` – ohne Höhenprüfung (`Z >= 0`) würden Wellen-Gegner im tiefen Meer spawnen! 2. Auf Brücken, Stegen oder erhöhten Plateaus aus StaticMeshes über `0,5 m` Höhe lehnt `MaxGroundHeightCm` Spawns ab, wenn man Brücken nicht als gültigen Boden kennzeichnet. |
| [`UEIRunDirector`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/EIRunDirector.cpp#L636-L1005) | Liest `laufkarte.json` (`gebiet`, `anlegestelle`, `ankunft`, `bossplaetze`). Setzt Helden bei `Arrival` ab und lässt die Barke am `ActiveFire` landen. | **Zeile 1001:** Die Barke schwebt beim Signalfeuer auf `FireLocation - FireLocation.GetSafeNormal2D() * 12 m` (also 12 m Richtung Kartenmitte) auf `Z = FireLocation.Z + 1,4 m` ein. | Rund um jedes Signalfeuer muss Richtung Kartenmitte `(0, 0)` mindestens `18 m` weit **frei von hohen Bäumen und Felsen** sein, damit die Barke beim Landen nicht in Baumkronen steckt. |
| [`AEIHud::DrawContractBoard`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/EIHud.cpp#L367-L437) & [`EIContracts.cpp`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/EIContracts.cpp#L197-L202) | Liest `boden_m` in `Table.MapMeters` und zeichnet die Übersichts-Karte auf Taste `M`. | Rechnet `Scale = BoardMapSize / (Table.MapMeters * 100.0f)`. Oben auf der Karte ist `+X` (Norden), rechts ist `+Y` (Osten). | Skaliert automatisch mit `boden_m`, bricht im Spiel nicht. |
| [`FEIContractDataTest`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/Tests/EIContractTest.cpp#L27) (`Data.Contracts`) | Prüft die Werte aus `nebenauftraege.json` und `laufkarte.json`. | **Zeile 27:** `TestEqual("The overview knows the map", Table.MapMeters, 300.0f);` | **Bricht sofort**, sobald `boden_m` in `laufkarte.json` geändert wird! Muss auf die neue Größe (oder `Table.MapMeters > 100.0f`) angepasst werden. |
| [`FEIRunCompleteTest`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/Tests/EIRunTest.cpp#L219-L235) (`Play.Run.Complete`) | Prüft Anlegestelle, Überfahrt, Ankunft, Signalfeuer und Abflug. | **Zeile 223 & 228:** `FromEdge = Half - Max(Abs(X), Abs(Y))` und `TestTrue("The arrival lies at the edge", FromEdge < 2500.0f)`. | **Bricht bei einer runden Insel!** Bei einer Insel im Meer liegt die Küste etwa bei Radius `210–250 m`, während `Half` (der Kartenrand des Meeres) bei `300 m` liegt (`FromEdge` wäre `50–90 m` statt `< 25 m`). Außerdem wird in Zeile 308 die Barke auf die andere Seite des Feuers gespiegelt (`Fire * 2 - Barge`). |
| [`FEIPathfindingTest`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/Tests/EIRunTest.cpp#L1205-L1256) (`Play.Run.Pathfinding`) | Prüft, ob ein Gegner um eine Mauer am ersten Ort (`orte[0]`) herum zum Helden läuft. | **Zeile 1208–1209:** Feste Koordinaten `HeroSpot(0, 12300, 92)` und `EnemySpot(0, 11100, 92)`, passend zur Mauer von `orte[0]` bei `(0, 110 m)`. | **Bricht sofort**, wenn `orte[0]` nicht mehr bei `(0, 110 m)` steht oder eine andere Mauerform hat. |
| [`FEIRunTerrainTest`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/Tests/EIRunTest.cpp#L1260-L1359) (`Play.Run.Terrain`) | Prüft Höhenspanne der Landschaft, ob alle Spielorte eben auf `Z = 0` (`±30 cm`) liegen und von `Arrivals[0]` zu Fuß erreichbar sind. | Abtastung `-14000` bis `+14000 cm`; `FMath::Abs(Height) > 30.0f` verlangt, dass **jeder** Spielort exakt auf Höhe `0 m` liegt. | Wenn alle Spielorte weiter auf `Z = 0 m` liegen (oder der Test Ebenheit relativ zur Platzhöhe prüft), bleibt die Logik erhalten; der Abtastbereich sollte `boden_m` nutzen. |
| Übrige Tests in [`EIRunTest.cpp`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/Tests/EIRunTest.cpp) & [`EIBossTest.cpp`](file:///C:/Users/Phillip/dev/game-project/Source/Eisenseelen/Private/Tests/EIBossTest.cpp) | `Director`, `Camps`, `Chests`, `Contracts`, `Boss`, `BossWaves`, `SoulCaller`. | Nutzen fast überall `Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f)` (`x = -15 m, y = 0 m`) als sicheren Parkplatz weit weg von Lagern und Aufgaben, sowie `OnGround(Hero + (9000, 0, 0))` in Zeile 557. | Bei `(-15 m, 0 m)` (knapp südwestlich des Felsmassivs) muss auf der neuen Karte **ebener Boden auf Höhe 0 ohne Lager im 25-m-Sichtkreis** liegen, damit kein Test dort in einem Felsen steckt oder versehentlich ein Lager weckt! |

---

### 1.3 Gemessene Geschwindigkeiten und Laufzeiten

Aus [`Data/held.json`](file:///C:/Users/Phillip/dev/game-project/Data/held.json), [`Data/welt.json`](file:///C:/Users/Phillip/dev/game-project/Data/welt.json) und [`Source/Eisenseelen/Private/EILaunchPad.cpp`](file:///C:/Users/Phillip/dev/game-project/Source/EILaunchPad.cpp):

| Bewegungsart | Wert aus den Daten | Reale Wirkung im Spiel |
| --- | --- | --- |
| **Laufen (Grundtempo)** | `lauftempo_m_pro_s = 5.5` | `5,5 m/s` (`330 m` pro Minute). |
| **Sprinten (Shift halten)** | `sprint_tempo_m_pro_s = 8.0` | `8,0 m/s` (`480 m` pro Minute, keine Ausdauerleiste). Mit Tempo-Schuhen bis zu `+30 %` (`10,4 m/s`). |
| **Blocken (Klingenwacht)** | `blocken_tempo_m_pro_s = 3.3` | `3,3 m/s` während der Schild oben ist. |
| **Normaler Sprung** | `sprunghoehe_m = 1.0`, `schwerkraft_faktor = 1.6` | Scheitelhöhe `1,0 m`, Flugzeit rund `0,71 s`. Nimmt kein Extra-Tempo mit (kein Bunnyhop). |
| **Sprungfeld (`AEILaunchPad`)** | `sprungfeld_hoehe_m = 6.0` | Vertikaler Katapult-Stoß auf `6,0 m` Scheitelhöhe (`LaunchSpeed = sqrt(2 * g * 600 cm) ≈ 13,7 m/s`, Flugzeit `~1,75 s`). Horizontal behält man das Lauf-/Sprinttempo (`5,5–8,0 m/s`) und überwindet so Klippen und Schluchten von `5–6 m` Höhe. |

**Wie lange läuft man über die jetzige Karte (`300 × 300 m`, nutzbare Fläche `276 × 276 m`)?**

- **Einmal gerade quer über die Karte (`276 m` von Rand zu Rand):**
  - Zu Fuß (`5,5 m/s`): **50 Sekunden** (`0,83 Minuten`).
  - Im Sprint (`8,0 m/s`): **34,5 Sekunden** (`0,58 Minuten`).
- **Einmal diagonal (`390 m`):**
  - Zu Fuß: **71 Sekunden** (`1,18 Minuten`).
  - Im Sprint: **49 Sekunden** (`0,81 Minuten`).
- **Typischer Lauf (Ankunft → Bossplatz → Signalfeuer):**
  - Aus den 4 Ankünften, 4 Bossplätzen und 4 Feuern in `Data/laufkarte.json` (unter Beachtung von `signalfeuer_mindestabstand_m = 100 m`) beträgt die Luftlinie im Mittel **357,5 m** (Minimum `170 m`, Maximum `514,5 m`).
  - Reine Gehzeit auf der heutigen Karte: im Schnitt nur **65 Sekunden zu Fuß** bzw. **45 Sekunden im Sprint**.

---

## Teil 2: K1 Detailplan der Waldinsel

### 1. Größe und Maßstab

Damit die gewünschten Richtwerte erreicht werden (**einmal quer über die Insel: 2–3 Minuten zu Fuß**, **Ankunft → Boss → Signalfeuer: 4–6 Minuten**), ist die heutige `300 × 300 m`-Karte deutlich zu klein.

- **Rechnung über die Geschwindigkeit:**
  - Bei `5,5 m/s` Lauftempo entsprechen `2 Minuten` einem Weg von `660 m` (auf gewundenen Wegen mit `1,3×` Umwegfaktor einer Luftlinie von **rund `500 m` von Küste zu Küste**).
  - Im Wechsel aus Laufen und Sprinten (`Ø 6,8 m/s`) dauert die Inselquerung auf einem `650 m` langen Weg durch das Wegenetz rund **1,6 bis 2,0 Minuten**, reines Gehen (`5,5 m/s`) **2,0 bis 2,3 Minuten**.
  - Für die Strecke **Ankunft → Nebenauftrag → Boss (mit Bannbruch) → Signalfeuer** ergibt sich auf einer Insel mit **500 m Land-Durchmesser** eine Wegstrecke von rund `850–1.100 m` (etwa `2,5–3,3 Minuten` reine Laufzeit plus `2–3 Minuten` Kämpfe und Haltezeiten = **4,5 bis 6 Minuten** bis zum Signalfeuer).
- **Vorschlag für die Kartengröße:**
  - **Gesamtgröße der Landschaft (`boden_m`):** **`600 × 600 m`** (Koordinaten von `-300 m` bis `+300 m`).
  - **Größe der Insel selbst (Landmasse):** rund **`490 × 470 m`** (Radius vom Zentrum `210 m` bis `265 m` an den Halbinseln), ringsum gesäumt von `35–80 m` Flachwasser und offenem Meer bis zum Kartenrand.
  - **Proportionen wie im Vorbild:** Die 12 großen Sandkreise haben einen Durchmesser von `44 m` (`Radius 22 m`), das ist genau `1/11,5` der Inselbreite – exakt wie die `1/12` in der Vorlage.
- **Technische Grenzen (warum `600 × 600 m` der optimale Punkt ist):**
  - **Landschafts-Auflösung:** `teile_pro_seite = 10`, `quadrate_pro_teil = 63`, `quadrat_cm = 100`. Das ergibt `631 × 631 = 398.161` Eckpunkte bei `1 m` Rasterabstand. `UEIWorldTools::BuildLandscape` importiert diese Größe in weniger als 2 Sekunden.
  - **Wegenetz (`RecastNavMesh`):** Da das Wegenetz nur auf dem Land (und nicht auf dem tiefen Meeresboden) gebraucht wird, baut Unreal das NavMesh beim Spielstart in etwa `3–5 Sekunden` (weit unter dem `30 s`-Limit der Tests).
  - **Leistung:** 4-fache Fläche gegenüber `300 × 300 m`, aber etwa `45 %` davon sind Meer, Strand und offene Felder. Mit `UInstancedStaticMeshComponent` (HISM) für den Wald bleiben wir sicher über `60 FPS`.

---

### 2. Raster und Achsen

In Unreal Engine (`make_runmap.py` und `AEIHud::DrawContractBoard` in `EIHud.cpp`, Zeile 381) gilt:

- **Mitte der Insel:** `(0, 0)` auf Höhe `Z = 0 m`.
- **Norden (oben im Bild und auf der M-Karte):** **`+X`** (`x_m` von `-300` im Süden bis `+300` im Norden).
- **Süden (unten im Bild):** **`-X`**.
- **Osten (rechts im Bild):** **`+Y`** (`y_m` von `-300` im Westen bis `+300` im Osten).
- **Westen (links im Bild):** **`-Y`**.
- **Blickrichtung (`blick_grad` / Yaw):** `0°` schaut nach Norden (`+X`), `90°` nach Osten (`+Y`), `180°` nach Süden (`-X`), `270°` nach Westen (`-Y`).

---

### 3. Regionen-Tabelle (10 Regionen)

Jede Region trägt einen eigenen deutschen Namen, der zur Welt von *Eisenseelen* passt:

| Nr. | Name | Lage (`x_m` Nord, `y_m` Ost) & Fläche | Untergrund & Höhe | Dichte | Stimmung | Rolle im Spiel & Gefahrstufe |
| --- | --- | --- | --- | --- | --- | --- |
| **1** | **Das Eisenjoch** *(Zentrales Felsmassiv)* | Mitte-Ost (`x: -20..+60`, `y: -10..+190`), `210 × 110 m` | Schroffer Fels, Kiefernnadeln, Höhlenboden; **`0 m` (Tunnel) bis `+26 m` (Gipfel)** | dicht | Dunkler Felskamm mit Kiefern obenauf, darunter Fackelschein im Höhlentunnel | Großes Sicht- und Wegehindernis in der Inselmitte; der Tunnel bietet die schnellste Nord-Süd-Abkürzung, wird aber von einem mittleren Lager gehalten. **Gefahr: 3 (Hoch)** |
| **2** | **Die Wächterkuppe** *(Westlicher Felsberg)* | Mitte-West (`x: -70..+20`, `y: -150..-60`), `95 × 95 m` | Fels, Waldboden; **`0 m` bis `+18 m`** | dicht | Runder, bewaldeter Felsstock mit Holzbrücke über die südliche Schlucht | Zweiter Höhenpunkt, teilt die westlichen Waldwege; über eine Rampe im Südosten oder per Sprungfeld ersteigbar. **Gefahr: 2 (Mittel)** |
| **3** | **Der Seelenforst** *(Dichter Kiefernwald)* | Ring um Mitte (`x: -110..+110`, `y: -150..+140`), `280 × 220 m` | Dunkles Moos, Waldboden, Felsmauern; **`0 m` bis `+5 m`** | dicht | Hohe Kiefern, tiefhängender Walddunst, zwei lange Holzgerüste vor der Nordwand des Massivs | Hauptkampfgebiet im Inneren; niedrige Felsmauern bilden Gassen; 2 Bossplätze (`S6`, `S9`) und mehrere Lager. **Gefahr: 3 (Hoch)** |
| **4** | **Schluchtring & Nebelmarsch** *(Nordwesten)* | Nordwest (`x: +15..+185`, `y: -245..-80`), `170 × 165 m` | Schlamm, Bachkiesel, Sandplateau, weiße Klippen; **`-1,2 m` (Bach) bis `+22 m` (Kap)** | halboffen | Helles Klippenkap am Westmeer, dahinter verzweigte Bäche und ein Felsring mit 4 Holzbrücken | Das erhöhte Plateau im Felsring (`+4,5 m`) ist eine natürliche kleine Festung für ein mittleres Lager (Anführer-Ziel); außen am Kap steht ein Signalfeuer (`S3`). **Gefahr: 2 (Mittel)** |
| **5** | **Die Brachfelder** *(Bauernhof im Norden)* | Norden (`x: +115..+255`, `y: -95..+35`), `140 × 130 m` | Ackerfurchen, Wiese, Sandflecken; **`0 m` bis `+2 m`** | offen | Verlassener Hof mit Scheune, Holzzäunen und Feldreihen bis zur runden Sandzunge im Norden | Weite Sichtlinien für Arkanisten und Knochenwerfer; Ankunft Nord (`x=248, y=-55`) und Bossplatz `S4` am südlichen Hofrand. **Gefahr: 1 (Niedrig am Rand, 2 am Hof)** |
| **6** | **Steinbruch & Eisenturm** *(Nordosten)* | Nordost (`x: +70..+220`, `y: +35..+195`), `150 × 160 m` | Wiese mit Findlingen, heller Grubensand, Küstenfelsen; **`-3,5 m` (Grube) bis `+3 m`** | offen | Offene Hügelfelder, eine tiefe runde Schürfgrube, ein rostiger Eisenturm und tote Bäume im Uferwasser | Der Eisenturm dient als Nordost-Wahrzeichen; in der Grube steht ein mittleres Lager, an der Küste Signalfeuer `S2` und Auftragsplatz `S5`. **Gefahr: 2 (Mittel)** |
| **7** | **Die Ostklamm** *(Ostküste)* | Osten (`x: -70..+65`, `y: +160..+265`), `135 × 105 m` | Felsgrat, Gras, Wasserrinne; **`-1,4 m` bis `+15 m`** | halboffen | Schroffer Küstengrat am Meer, südlich davon eine tiefe Wasserrinne mit langer Holzbrücke | Verbindet Nordosten und Südosten über die Ostbrücke; Ankunft Ost (`x=8, y=238`) und Bossplatz `S8`. **Gefahr: 2 (Mittel)** |
| **8** | **Das Bleiche Moor** *(Südost-Sumpf)* | Südost (`x: -195..-70`, `y: +55..+215`), `125 × 160 m` | Moorwasser, Schlamminseln, weiße Kalkplatten; **`-0,7 m` bis `+1 m`** | halboffen | Viele flache Tümpel, bodennaher Nebel, Holzpfähle im Meer und weiße Felsplatten am Ufer | Flaches Wasser und schmale Landbrücken zwingen zu überlegter Bewegung; Signalfeuer `S11` am Südrand. **Gefahr: 2 (Mittel)** |
| **9** | **Pfahlbucht & Leuchtturm** *(Süden & Südwesten)* | Süd & Südwest (`x: -245..-130`, `y: -195..+45`), `115 × 240 m` | Holzplanken, Sandstrand, seichtes Wasser; **`-0,5 m` bis `+5 m` (Leuchtturm)** | halboffen | Hütten auf Holzpfählen, lange Stege ins Meer, ein Schiff vor der Küste und der Leuchtturm im Südwesten | Ankunft Süd (`x=-192, y=-35`) direkt am Hauptsteg; Signalfeuer `S12` vor der Leuchtturm-Halbinsel. **Gefahr: 1 (Niedrig)** |
| **10** | **Die Wrackküste** *(Weststrand)* | Westen (`x: -120..+60`, `y: -235..-165`), `180 × 70 m` | Breiter Sandstrand, Dünengras, Felsmauern; **`0 m` bis `+3,5 m`** | offen | Langer Weststrand mit gestrandetem Schiffswrack im Flachwasser und Felsgassen landeinwärts | Ankunft West (`x=-65, y=-212`), schnelle Nord-Süd-Küstenroute und Auftragsplatz `S7`. **Gefahr: 1 (Niedrig)** |

---

### 4. Höhenplan

Alle Höhen werden in `tools/plan_waldinsel.py` (und ab K2 in `tools/gelaende.py`) mathematisch berechnet, sodass die Python-Draufsicht und die Unreal-Landschaft exakt übereinstimmen:

- **Tiefes Meer (`-4,0 m`):** Alles weiter als `22 m` außerhalb der Küstenlinie liegt auf `-4,0 m`.
- **Flachwasser (`-0,6 m` bis `-0,1 m`):** Ein `15–25 m` breiter Ufergürtel rund um die Insel sowie die großen Sumpftümpel.
- **Strand und Ufer (`0,0 m` bis `+0,4 m`):** Sanfter Übergang vom Wasser auf die Küstenwege.
- **Ebene und alle Spielorte (`0,0 m`):** Alle 12 großen Sandkreise (`Radius 22 m`), alle 4 Ankünfte, 4 Bossplätze, 4 Signalfeuer, 18 Lager und 5 Sprungfelder werden über `flat_weight(x, y)` exakt auf **`Z = 0,0 m`** geglättet (mit `12 m` weichem Übergang ins Gelände). Das garantiert, dass Bosse, Ankersteine, Seelenkessel, Truhen und die landende Barke nie schräg stehen oder im Boden versinken.
- **Sanfte Geländewellen (`-1,1 m` bis `+1,1 m`):** Überlagerung aus drei Sinuswellen (`wellen_laenge = 65 m`) auf den Wiesen und im Wald außerhalb der ebenen Plätze.
- **Bäche und Sumpfsenken (`-0,6 m` bis `-1,4 m`):**
  - Die beiden **Schluchtbäche** im Nordwesten (`-1,2 m` tief, `7–8 m` Sohle) umschließen den Felsring und münden nach Nordwesten ins Meer.
  - Die **Ostklamm-Rinne** (`-1,4 m` tief) schneidet südlich des Ostgrats zum Meer durch.
  - Das **Bleiche Moor** im Südosten und die **Nebelmarsch** im Nordwesten liegen als flache Senken (`-0,6 m` bis `-0,7 m`) mit kleinen trockenen Grasinseln (`+0,2 m`) darin.
  - **Steinbruch im Nordosten (`x=110, y=100`):** Eine runde Grube mit `26 m` Radius, die auf **`-3,5 m`** abfällt (mit flacher Sohle für ein mittleres Lager und zwei breiten Rampen nach Westen und Süden).
- **Erhebungen und Felsen:**
  - **Das Eisenjoch (Zentrales Felsmassiv):** Drei verbundene Felsrücken von West nach Ost (`+26 m` im Westen, `+22 m` in der Mitte, `+18 m` im Osten). Die Flanken sind über `45°` steil (unbegehbarer Fels), aber von Südwesten (`rampe_grad = 200°`), von Nordosten (`rampe_grad = 20°`), über die beiden Holzgerüste im Norden sowie über 2 Sprungfelder sind die oberen Kuppen erreichbar.
  - **Die Wächterkuppe (`x=-25, y=-105`):** Runder Felsberg mit **`+18 m`** Höhe, steilen Wänden und einer begehbaren Rampe nach Südosten sowie einem Sprungfeld im Nordosten.
  - **Nordwest-Kap (`x=145, y=-225`):** Steile Küstenklippe bis **`+22 m`** Höhe als dramatische Kulisse zum Meer.
  - **Felsring-Plateau (`x=65, y=-125`):** Ein rundes Plateau (`Radius 34 m`) auf **`+4,5 m`** Höhe, umschlossen von einem `5,5 m` hohen Felsring und den beiden Schluchtbächen. Vier Holzbrücken und eine Rampe führen hinein.
  - **Leuchtturm-Kuppe (`x=-225, y=-155`):** Kleiner Küstenhügel auf **`+5,0 m`** Höhe auf der Südwest-Halbinsel.

---

### 5. Höhle und Tunnel durch das Felsmassiv

Eine Unreal-Landschaft (`ALandscape`) ist ein Höhenfeld – sie hat pro `(X, Y)`-Punkt genau eine Höhe und kann von sich aus **keine Überhänge oder Höhlendecken** darstellen. Dafür gibt es zwei saubere Bauweisen:

#### Variante A (Empfohlen): Eingeschnittene Schlucht auf Höhe `0 m` mit Felsdach aus Bauteilen
- **So funktioniert es:** Entlang der Tunnelachse schneidet `gelaende.py` die Landschaft auf `8 m` Breite glatt auf **`Z = 0,0 m`** herunter (genau wie ein ebener Weg). Darüber setzt `make_runmap.py` ab `Z = 6,0 m` bis `Z = 24,0 m` große Felsblöcke und ein Höhlendach aus Bauteilen (`bauteil.tunnel_bogen`, `bauteil.fels_gross`), die die Lücke im Massiv optisch und physisch wieder komplett schließen.
- **Warum das die beste Wahl ist:**
  1. **Wegfindung (`RecastNavMesh`) funktioniert sofort und fehlerfrei:** Gegner und Helden laufen im Tunnel auf der ganz normalen Landschaft (`ALandscapeProxy` auf `Z = 0 m`), ohne dass ein Loch-Material oder eine Visibility-Maske gebacken werden muss.
  2. **`OnGround` und `UEIHordeDirector::FindStandingPlace` brauchen keine Sonderfälle:** Ein LineTrace von oben innerhalb des Tunnels (oder die Höhenabfrage der Landschaft) findet auf `Z = 0 m` immer gültigen Boden; für Spawns von oben blockiert das Felsdach mit `ECC_WorldStatic`, sodass keine Wellen-Gegner mitten auf dem Dach über dem Spieler spawnen (`MaxGroundHeightCm = 50 cm` verhindert Spawns auf dem `24 m` hohen Felsdach automatisch!).
  3. **Planbild gleich Karte:** Die Python-Draufsicht zeigt den Boden des Tunnels exakt so, wie er im Spiel begehbar ist.

#### Variante B (Alternative): Sichtbarkeits-Maske (Landscape Visibility Hole) + komplettes Tunnel-Rohr aus Bauteilen
- **So funktioniert es:** Die Landschaft bleibt oben auf `+26 m`, bekommt über einen `LandscapeVisibilityMask`-Layer an den 3 Eingängen ein Loch, und der gesamte Tunnel (Boden, Wände, Decke) besteht aus StaticMesh-Bauteilen.
- **Nachteil:** Erfordert ein spezielles Opacity-Mask-Material auf der Landschaft, und `ALandscapeProxy::GetHeightAtLocation` im Test `Run.Terrain` liefert im Tunnel weiterhin `+26 m` (die Höhe der ausgeblendeten Landschaft) statt `0 m`.

#### Grundriss und Details des Tunnels („Eisenjoch-Durchbruch“)
- **Drei Eingänge (Y-Form):**
  1. **Nord-Eingang (`x = 58 m, y = 65 m`):** Direkt hinter dem Gerüstlager im nördlichen Seelenforst.
  2. **Süd-Eingang (`x = -28 m, y = 72 m`):** Öffnet sich direkt zum Boss-/Auftragsplatz `S9` an der Südwand des Massivs.
  3. **West-Stollen (`x = 8 m, y = -8 m`):** Ein Seitenausgang Richtung Inselmitte (`Forstherz S6`).
- **Länge und Maße:** Nord–Süd-Hauptgang **`86 m` lang**, West-Stollen **`66 m` lang**. Die Gänge sind **`8 m` breit** und **`6 m` hoch** (genug Platz, damit die Kamera nie an der Decke streift und man auch einem Schildträger ausweichen kann).
- **Zentrale Kaverne (`x = 14 m, y = 58 m`):** Eine runde Halle in der Mitte (`Durchmesser 36 m`, **`9 m` Deckenhöhe**) mit felsigen Stützpfeilern als Deckung.
- **Wer darin haust:** Eines der 4 mittleren Lager (`lager[15]`) steht fest in der zentralen Kaverne. Wenn ein Nebenauftrag „Anführer jagen“ dieses Lager auswählt, wartet der Anführer mitten im Berg.
- **Licht im Tunnel:** Warme Punktlichter (`PointLight`, orangefarben wie Fackeln/Glutbecken, Schlüssel `bauteil.fackel`) alle `18 m` an den Tunnelwänden und in der Mittelhalle, damit die Höhle ohne Tageslicht klar lesbar ist.

---

### 6. Wasser, Bäche, Sumpf und Brücken

- **Wasserspiegel:** Überall auf der Karte liegt die Wasseroberfläche bei **`Z = -0,15 m`**.
- **Bauweise (Einfache Wasserfläche statt Unreal Water-Plugin):**
  - **Empfehlung:** Eine große, flache Wasser-Ebene (`StaticMeshActor` mit skaliertem Plane/Cube auf `Z = -0,15 m` und eigenem Material `M_Wasser` in `make_assets.py`: tiefes Blaugrün, leicht spiegelnd mit `Roughness = 0.15`).
  - **Begründung:** Das Unreal-Water-Plugin verändert die Landschaft über eigene Blueprints und Edit-Layers, verträgt sich schlecht mit unserem C++-Import (`BuildLandscape`) und bringt im Headless-Test (`-nullrhi`) keinen Nutzen. Eine einfache Fläche ist in 10 Zeilen gebaut, kostet fast keine Leistung und ist zu 100 % stabil.
- **Wie das Meer den Kartenrand bildet (Vorschlag zur Auswahl):**
  - **Flaches Uferwasser (`Z = -0,6 m` bis `0,0 m`, bis etwa `12 m` vor den Strand):** **Begehbar!** Man steht bis zu den Knien/Hüften im Wasser (etwa bei den Bäumen vor der Leuchtturm-Halbinsel, am Schiffswrack im Westen oder im Bleichen Moor).
  - **Tiefes Meer (`Z < -0,7 m`):** Entlang der Tiefwassergrenze (rund `15 m` außerhalb der Küstenlinie) zieht `make_runmap.py` einen Ring aus **unsichtbaren Sperrwänden** (`CollisionProfile = BlockAll`, `HiddenInGame = True`, `NavModifier = Hindernis`). So sieht man bis zum Horizont freies Meer ohne hässliche graue Mauer, kann aber weder ins tiefe Meer hinauslaufen noch von einem Sprungfeld aus über das Ufer hinausfliegen.
- **Bäche und Sumpftümpel:**
  - Die Sumpftümpel im Südosten (`-0,6 m`) sind direkt durchwatbar.
  - Die beiden Schluchtbäche im Nordwesten (`-1,2 m`) und die Ostklamm-Rinne (`-1,4 m`) haben steilere Uferböschungen oder Felsen am Rand, sodass man die **9 Holzbrücken und Stege** (`bauteil.holzbruecke`, `bauteil.steg`) als natürliche Übergänge nutzt, aber an flachen Furten auch durch das Wasser kommt.
  - **Verlangsamen im Wasser?** Im ersten Schritt (K4) empfehlen wir **kein Verlangsamen**, weil langsame Bewegung im Kampf gegen Fernkämpfer schnell frustriert (Design-Säule 2: *„Bewegung ist die Verteidigung“*). Falls gewünscht, lässt sich ein Faktor `wasser_tempo_faktor: 0.85` in `Data/welt.json` leicht ergänzen.

---

### 7. Wegenetz

- **Verlauf (aus [`Data/waldinsel.json`](file:///C:/Users/Phillip/dev/game-project/Data/waldinsel.json)):**
  - **Innerer Hauptring (`7,0 m` breit):** Verbindet alle Regionen einmal rund um das Eisenjoch und die Wächterkuppe (`Länge ~920 m`).
  - **8 Küsten-Speichen (`6,0–6,5 m` breit):** Führen vom Ring zu allen 4 Ankünften, allen 4 Signalfeuern und den äußeren Sandkreisen.
  - **2 Querverbindungen (`5,5 m` breit):** Der **Tunnelweg** mitten durch das Eisenjoch und der **Herzpfad** zwischen Eisenjoch und Wächterkuppe.
- **Wie die Wege sichtbar werden:**
  - Direkt über das **Landschaftsmaterial `M_Gelaende`** (und im Planbild über `road_weight(x, y)`): Entlang der Polylinien aus `waldinsel.json` wird die Bodenschicht **Sand/Weg** gemalt, und `height_m(x, y)` glättet Unebenheiten auf dem Weg um `85 %`. Außerdem hält die Walddichte-Funktion alle Bäume, Büsche und Zufallstruhen mindestens `4,5 m` von der Wegmitte fern.
  - **Vorteil:** Keine schwebenden Streifen-Meshes an Hängen, kein Z-Fighting, und Wege sind schon aus der Ferne und von erhöhten Punkten klar als helle Sandbänder erkennbar.
- **Wirkung im Spiel:**
  - Reine **Orientierung und freie Sicht-/Laufbahn** (kein Hindernis, kein Baum im Weg), **ohne künstlichen Tempo-Bonus**, damit man im Kampf nicht bestraft wird, wenn man einem Angriff seitlich ins Gras ausweicht.

---

### 8. Spielorte neu verteilen (Koordinaten und Regeln)

Alle Regeln aus `Data/bosse.json` (`signalfeuer_mindestabstand_m = 100 m`), `Data/nebenauftraege.json` (`mindestabstand_m = 40 m`) und `Data/lauf.json` werden strikt eingehalten:

#### A. Anlegestelle (über den Wolken, unverändert weit weg)
- `x_m = 0`, `y_m = -6000`, `hoehe_m = 150` (Abstand zur Insel `6.000 m`, also weit über den `1.000 m` aus `Run.Complete`).

#### B. Die 4 Ankünfte an der Küste (`ankunft`)
Das Team landet mit der Himmelsbarke am Strand und blickt direkt ins Inselinnere auf ein Wahrzeichen:

| Nr. | Name | `x_m` (Nord) | `y_m` (Ost) | `blick_grad` | Wo man landet & was man zuerst sieht |
| --- | --- | --- | --- | --- | --- |
| **1** | **Ankunft Nord (Sandzunge)** | `248` | `-55` | `180°` (Süd) | Auf der runden Sandhalbinsel im Norden; Blick über den Sandkreis `S1` und den Brachhof direkt auf das hohe **Eisenjoch**. |
| **2** | **Ankunft Ost (Ostbucht)** | `8` | `238` | `270°` (West) | Am Strand unterhalb des Ostgrats; Blick auf die Ostklamm-Klippen und den **Eisenturm** rechts. |
| **3** | **Ankunft Süd (Pfahlbucht)** | `-192` | `-35` | `0°` (Nord) | Direkt am breiten Hauptsteg des Pfahlbucht-Dorfs; Blick durch die Steghütten auf die Südwand des **Eisenjochs**. |
| **4** | **Ankunft West (Wrackstrand)** | `-65` | `-212` | `90°` (Ost) | Am breiten Sandstrand neben dem Schiffswrack; Blick zwischen Felsmauern auf die **Wächterkuppe**. |

#### C. Die 12 großen Sandkreise (`sandplaetze`, je `Radius 22 m`, eben auf `0,0 m`)
Wie in der Vorlage sind die großen runden Sandplätze die Bühnen für Bosse, Signalfeuer und Nebenaufträge (Schreine/Altäre):

| Kreis | Name | `x_m` | `y_m` |feste Zuordnung in `laufkarte.json` |
| --- | --- | --- | --- | --- |
| **S1** | Nordkap-Kreis | `235` | `-55` | Nebenauftrag (`orte[0]` – Altar/Schrein am Nordstrand) |
| **S2** | Nordostbucht-Kreis | `205` | `110` | **Signalfeuer 1** (Nordost-Küste) |
| **S3** | Klippen-Kreis | `160` | `-180` | **Signalfeuer 2** (Nordwest-Klippenkap) |
| **S4** | Hof-Kreis | `140` | `20` | **Bossplatz 1** (Offene Lichtung zwischen Hof und Wald) |
| **S5** | Steinbruch-Kreis | `95` | `160` | Nebenauftrag (`orte[1]` – Altar/Schrein östlich des Steinbruchs) |
| **S6** | Forstherz-Kreis | `25` | `-55` | **Bossplatz 2** (Zentrale Waldlichtung zwischen beiden Bergen) |
| **S7** | Westwald-Kreis | `20` | `-180` | Nebenauftrag (`orte[2]` – Altar/Schrein an der Westküste) |
| **S8** | Osthang-Kreis | `-30` | `185` | **Bossplatz 3** (Hinter der Ostklamm-Brücke) |
| **S9** | Südfels-Kreis | `-45` | `65` | **Bossplatz 4** (Vor dem Süd-Ausgang des Tunnels) |
| **S10** | Südwald-Kreis | `-95` | `-75` | Nebenauftrag (`orte[3]` – Altar/Schrein südlich der Wächterkuppe) |
| **S11** | Moor-Kreis | `-155` | `115` | **Signalfeuer 3** (Am Rand des Bleichen Moors) |
| **S12** | Bucht-Kreis | `-165` | `-135` | **Signalfeuer 4** (Zwischen Stegdorf und Leuchtturm) |

*(Zusätzlich gibt es 4 weitere Nebenauftrags-Orte `orte[4..7]` außerhalb der Sandkreise: am Brachhof `(155, -50)`, in der Felsring-Feste `(65, -125)`, im Steinbruch `(110, 100)` und am Stegdorf `(-175, -25)`, sodass wie bisher **8 Altar-/Schrein-Orte** zur Auswahl stehen.)*

**Prüfung des Mindestabstands Boss ↔ Signalfeuer (`signalfeuer_mindestabstand_m = 100 m`):**
- Der kleinste Abstand zwischen irgendeinem der 4 Bossplätze (`S4, S6, S8, S9`) und irgendeinem der 4 Signalfeuer (`S2, S3, S11, S12`) liegt zwischen `S4 (140, 20)` und `S2 (205, 110)` und beträgt **`111,0 m`** (`> 100 m`!). Alle 4 Signalfeuer sind also für **jeden** der 4 Bossplätze gültig; im Mittel liegt das Signalfeuer **`258 m`** vom Boss entfernt (eine spannende Flucht von rund `35–45 Sekunden` unter Blutmond-Verfolgung!).

#### D. Die 18 Lager (`lager`: 14 klein, 4 mittel – Gefahr steigt zur Mitte)
- **4 mittlere Lager (schwerer, im Inneren und an Schlüsselorten):**
  1. `(`14`, `58`)` – **Tunnel-Halle** mitten im Eisenjoch.
  2. `(`70`, `55`)` – **Gerüstlager** vor dem Nord-Eingang des Tunnels.
  3. `(`65`, `-125`)` – **Felsring-Feste** auf dem umschlossenen Plateau im Nordwesten.
  4. `(`110`, `100`)` – **Steinbruch-Sohle** im Nordosten.
- **14 kleine Lager (verteilt an Wegen, Höfen und Küsten):**
  - Brachfelder Nord `(195, -35)`, Brachhof Scheune `(150, -55)`, Eisenturm-Wiese `(170, 65)`, Nordost-Küste `(125, 180)`, Ostgrat `(25, 215)`, Moor-Nordrand `(-90, 165)`, Moor-Steg `(-130, 90)`, Pfahlbucht-Dorf `(-175, -25)`, Buchtweg Südwest `(-195, -115)`, Wrackstrand `(-85, -185)`, Westdüne `(-15, -195)`, Nebelmarsch-Bach `(125, -125)`, Südlicher Seelenforst `(-25, -15)`, Kreuzung Südwald `(-75, 20)`.
- **Sicherer Test-Punkt `(-15 m, 0 m)`:** Der am nächsten gelegene Lagerpunkt ist `(-25, -15)` in `18 m` Abstand, aber für die Tests halten wir um `(-15, 0)` einen `30 m`-Radius komplett lagerfrei (wir schieben `(-25, -15)` in K3 auf `(-35, -25)` = `32 m` Abstand, außerhalb des `25 m`-Sichtkreises!).

#### E. Sprungfelder (`sprungfelder`, 5 Stück) und Truhen (`truhen`, 14 Stück)
- **5 Sprungfelder:** Je eines am Nord-Aufstieg zum Eisenjoch `(75, 82)`, am Süd-Aufstieg zum Eisenjoch `(-35, 35)`, an der Wächterkuppe `(-5, -75)`, vor der Felsring-Feste `(40, -100)` und am Ostgrat `(15, 205)`.
- **14 Truhen:** Werden mit festem Zufalls-Seed auf ebenem Land (`Z` zwischen `0,3 m` und `2,5 m`, außerhalb von Wasser, Tunnel und Wegen) verteilt.

---

### 9. Orte mit Charakter (eigene Versionen mit `bauteil.*`-Schlüsseln)

Jeder Ort wird in Schritt K7 aus grauen Grundformen gebaut, die eine `UEILookComponent` mit einem eigenen Schlüssel `bauteil.*` in [`Data/aussehen.json`](file:///C:/Users/Phillip/dev/game-project/Data/aussehen.json) tragen. Sobald Phillip später ein 3D-Modell einträgt, ersetzt es sofort jede Instanz dieses Bauteils auf der ganzen Karte:

#### 9.1 Der Brachhof (Bauernhof im Norden, `x=150, y=-50`)
- **Bauteile (`Data/aussehen.json`):** `bauteil.scheune` (`14 × 9 × 7 m`), `bauteil.holzzaun` (`4 × 0.3 × 1.4 m`), `bauteil.ackerfurche` (`18 × 2 × 0.3 m`), `bauteil.strohballen` (`2 × 2 × 1.8 m`), `bauteil.holzkiste` (`1.5 × 1.5 × 1.5 m`).
- **Grundriss-Skizze:**
  ```text
       [Acker-Reihen West]       (Nordweg zu S1)      [Acker-Reihen Ost]
              |                         |                     |
      +---[holzzaun]---+         +--------------+      +---[holzzaun]---+
      |  [strohballen] |         |   SCHEUNE    |      |  [strohballen] |
      |                +---------+ (Durchgang)  +------+                |
      |     Hofplatz (Altar/Lager)      |              [holzkiste]      |
      +---[holzzaun]--------------------+-------------------------------+
                                        |
                              (Südweg zum Bossplatz S4)
  ```
- **Deckung & Wege:** Die Scheune hat offene Tore nach Norden und Süden; Zäune und Strohballen bieten halbhohe Deckung gegen Knochenwerfer. Drei Wege führen hinein (von Nordankunft `S1`, von der Nebelmarsch im Westen und vom Bossplatz `S4` im Süden).
- **Spielfunktion:** Kleines Lager im Hof und einer der 8 Altar-/Schrein-Plätze.

#### 9.2 Pfahlbucht-Dorf & Leuchtturm (Süden/Südwesten, `x=-180, y=-30` & `x=-225, y=-155`)
- **Bauteile:** `bauteil.pfahlhuette` (`7 × 6 × 5 m`), `bauteil.steg` (`10 × 4 × 0.5 m` auf Pfählen), `bauteil.leuchtturm` (`6 × 6 × 18 m` mit Leuchtfeuer oben), `bauteil.holzboot` (`5 × 2 × 1.2 m`), `bauteil.schiffswrack` (`22 × 7 × 8 m`).
- **Grundriss-Skizze:**
  ```text
         (Weg zum Südwald)                     (Weg zum Bucht-Kreis S12)
                 |                                        |
         [pfahlhuette]---[steg]---[pfahlhuette]      [Leuchtturm-Kuppe +5m]
                 |                   |                    |
  Meer ~~~ [Hauptsteg 55m (Ankunft Süd)] ~~~        [LEUCHTTURM 18m]
                 |                                        |
            [holzboot]                              [Steg ins Meer]
  ```
- **Deckung & Wege:** Hüttenwände und Stapel auf den breiten Holzstegen. Der Hauptsteg ist `4 m` breit und mit dem Ufer bündig, sodass das NavMesh nahtlos darüberläuft.
- **Spielfunktion:** Ankunft Süd am Hauptsteg, Signalfeuer `S12` vor der Leuchtturm-Halbinsel, kleines Lager zwischen den Hütten.

#### 9.3 Schiffswrack & Steinbruch (Weststrand `x=-75, y=-215` & Nordost `x=110, y=100`)
- **Bauteile:** `bauteil.schiffswrack` (schräg im Sand/Flachwasser liegender Rumpf mit Mast), `bauteil.felsblock` (`4 × 4 × 3 m`), `bauteil.stuetzmauer` (`8 × 1.5 × 3 m`), `bauteil.holzgeruest` (`12 × 3 × 6 m`).
- **Grundriss-Skizze (Steinbruch):**
  ```text
              [Felsblock]        (Nordweg zum Eisenturm)
          +-----------------------------------------------+
          |      Obere Kante (Z = 0 m)                    |
   Rampe  |   +---------------------------------------+   |
   West ===>  |   Grubensohle (Z = -3.5 m)            |   |
          |   |   [Lager mittel]  [Altar]  [Felsblock]|   |
          |   +---------------------------------------+   |
          +-----------------------||----------------------+
                               Rampe Süd
  ```
- **Spielfunktion:** Das Wrack markiert die Ankunft West; der Steinbruch bildet eine natürliche Arena (`-3,5 m`) für ein mittleres Lager und einen Auftragsplatz.

#### 9.4 Felsring-Feste (Banditenlager im Schluchtring, `x=65, y=-125`)
- **Bauteile:** `bauteil.holzbruecke` (`16 × 4 × 1 m`), `bauteil.palisade` (`6 × 0.6 × 3 m`), `bauteil.wachturm_holz` (`4 × 4 × 7 m`), `bauteil.zelt` (`5 × 4 × 3.5 m`).
- **Grundriss-Skizze:**
  ```text
                     ~~~ Schluchtbach ~~~
                            || (Brücke NW)
               +-----===Felsring-Wand===-----+
               |  [zelt]            [wachturm]|
  (Brücke SW) ===     Plateau (Z = +4.5 m)   === (Brücke NO)
               |      [Mittleres Lager]       |
               +-----===Felsring-Wand===-----+
                            || (Brücke SO)
  ```
- **Deckung & Wege:** Vier Holzbrücken überqueren den Bachgraben durch Lücken im Felsring. Palisaden am Plateaurand schützen die Verteidiger vor Fernbeschuss von außen.
- **Spielfunktion:** Mittleres Lager (bevorzugter Ort für „Anführer jagen“) und Altarplatz; daneben ein Sprungfeld für den schnellen Angriff über die Felswand.

#### 9.5 Eisenturm, Waldhütten und Hochgerüste (`x=155, y=75` & `x=70, y=55`)
- **Bauteile:** `bauteil.eisenturm` (`7 × 7 × 22 m`, offenes Fachwerk mit Plattform), `bauteil.waldhuette` (`6 × 5 × 4 m`), `bauteil.hochgeruest` (`20 × 3 × 7 m`).
- **Spielfunktion:** Der `22 m` hohe Eisenturm im Nordosten ist von fast überall sichtbar; die beiden langen Holzgerüste nördlich des Eisenjochs führen vom mittleren Waldlager (`x=70, y=55`) an die Felswand heran (mit Sprungfeld hinauf).

---

### 10. Wald und Felsen

- **Dichtekarte (`forest_density(x, y)` in `plan_waldinsel.py`):**
  - **Dichter Wald (`85 %` Dichte, Abstand `7–10 m`):** Im gesamten **Seelenforst** rund um das Eisenjoch und die Wächterkuppe.
  - **Lichter Wald (`35 %` Dichte):** An den Hängen zum Küstenring, in der Nebelmarsch und am Rand des Bleichen Moors.
  - **Offene Flächen (`0–8 %` Dichte):** Auf den **Brachfeldern** im Norden, rund um den **Steinbruch/Eisenturm** im Nordosten und am **Weststrand**.
  - **Garantiert baumfrei (`0 %` Dichte):** Alle 12 Sandkreise (`Radius 22 m + 6 m Puffer`), alle Wege (`Breite + 4 m`), alle 4 Ankünfte, 4 Signalfeuer (inklusive `18 m` Landezone für die Barke!), 18 Lager und Bachläufe.
- **Baumarten als Platzhalter (Schlüssel in `Data/aussehen.json`):**
  1. `bauteil.kiefer` (hoher Stamm `0,8 m` dick, `10–14 m` hoch, grüne Krone oben ab `4 m` Höhe, sodass man unter den Ästen hindurchsieht!).
  2. `bauteil.toter_baum` (kahler Stamm `8–11 m` hoch, im Bleichen Moor, im Nordost-Flachwasser und rund um die Südwest-Halbinsel).
  3. `bauteil.busch` (`1,5–2,5 m` breit, `1,2 m` hoch, **ohne Kollision**, nur Sichtauflockerung).
  4. `bauteil.felsmauer` (`8 × 2,5 × 3,5 m`, bildet die langgezogenen Felsgassen entlang der Wege).
- **Verteilung (Python mit festem Zufalls-Seed statt PCG-Graph):**
  - **Entscheidung:** Wir berechnen alle Baum- und Felspositionen deterministisch in Python (`zufall: 13`) aus `waldinsel.json` und erzeugen sie in `make_runmap.py`.
  - **Begründung:** Dann zeigt das Python-Planbild **exakt dieselben Bäume und Lichtungen** wie das Spiel, es gibt keine PCG-Ladezeiten beim Spielstart, und wir können im automatischen Test prüfen, dass kein einziger Baum einen Weg versperrt.
- **Leistung (Instanzen & Ziel-FPS):**
  - Insgesamt stehen etwa **450 bis 550 Bäume und Sträucher** auf der Insel.
  - Statt 500 einzelner Actors packt `make_runmap.py` (bzw. ein kleiner Helfer in `UEIWorldTools` oder ein Sammel-Actor mit `UHierarchicalInstancedStaticMeshComponent` / `UEILookComponent`) gleiche Baumarten in **Instanced Static Meshes** (mit `CullDistance` für Büsche ab `120 m`). Damit kostet der gesamte Wald nur 3 bis 4 Draw Calls und läuft stabil mit **über 60 FPS**.

---

### 11. Orientierung und Sichtachsen

Vier markante **Wahrzeichen (Landmarks)** überragen die Baumkronen (`12–14 m`) und zeigen dem Spieler jederzeit, wo auf der Insel er steht:

1. **Das Eisenjoch (Mitte-Ost, `+26 m` hoch):** Der helle, gezackte Felsrücken in der Mitte ist von jeder Region aus zu sehen und zeigt sofort, wo das Zentrum liegt.
2. **Der Eisenturm (Nordosten, `x=155, y=75`, Spitze auf `+24 m`):** Ein schlanker, rostiger Metallturm auf den offenen Nordost-Feldern.
3. **Der Leuchtturm (Südwesten, `x=-225, y=-155`, Spitze auf `+23 m`):** Steht frei auf der Südwest-Halbinsel mit einem warm leuchtenden Feuer an der Spitze.
4. **Das Nordwest-Klippenkap (`x=145, y=-225`, `+22 m` hoch) & der Große Bleiche Baum im Moor (`x=-130, y=120`, `18 m` hoch):** Markieren Nordwest- und Südost-Ecke.

**Sichtachsen von den 4 Ankünften:**
- **Von Ankunft Nord (`blick_grad = 180°`):** Geradeaus über den Hof-Sandkreis erhebt sich das **Eisenjoch**, links davon glänzt der **Eisenturm**, rechts das **Nordwest-Kap**.
- **Von Ankunft Ost (`blick_grad = 270°`):** Geradeaus die Ostspitze des **Eisenjochs**, rechts der **Eisenturm**, links die Brücke über die Ostklamm zum **Bleichen Moor**.
- **Von Ankunft Süd (`blick_grad = 0°`):** Über den langen Hauptsteg durch das Pfahlbucht-Dorf direkt auf die steile Südwand des **Eisenjochs** (mit dem Tunneleingang davor), links hinten der **Leuchtturm**.
- **Von Ankunft West (`blick_grad = 90°`):** Am Schiffswrack vorbei direkt auf die runde **Wächterkuppe**, dahinter das höhere **Eisenjoch**.

---

### 12. Stimmung: Licht, Nebel, Himmel und Blutmond

- **Tageslicht und Himmel (Standard-Zustand):**
  - Kühleres, klares Küsten- und Waldlicht statt der bisherigen Wüstenglut: Sonne (`DirectionalLight`) mit leicht goldenem Schräglicht (`Rotation: Pitch -42°, Yaw 35°`), `SkyAtmosphere` mit tiefblauem Zenit und hellem Küstenhorizont.
- **Nebel (`ExponentialHeightFog`):**
  - Leichter, blaugrauer Küsten- und Walddunst in der Ferne. In den Senken (`Z < 0 m`, also im **Bleichen Moor**, in der **Nebelmarsch** und im **Steinbruch**) sammelt sich dichterer Bodennebel.
- **Wie der Blutmond auf der Waldinsel aussieht:**
  - Sobald eine Aufgabe gehalten wird (`State->IsBloodMoon()`), färbt sich der Himmel über dem Meer und zwischen den schwarzen Silhouetten der Kiefern tief karminrot; der Bodennebel im Wald und Sumpf nimmt einen unheimlichen rötlichen Schimmer an. Gerade im dunklen Kiefernwald wirkt der Kontrast zwischen schwarzen Stämmen und rotem Himmel noch stärker als in der offenen Steppe.
- **Ideen für später (Modifikator Nacht):** Dunkelblauer Nachthimmel, bei dem der **Leuchtturm** im Südwesten, die **Signalfeuer**, die **Fackeln im Tunnel** und die leuchtenden Augen/Köpfe der Glutschädel die wichtigsten Lichtquellen sind.

---

### 13. Vorbereitung für echte 3D-Modelle und Handarbeit

- **Alle neuen Bauteile in `Data/aussehen.json`:**
  - In Schritt K7 ergänzen wir in `Data/aussehen.json` den Abschnitt `"_bauteile"` mit leeren `"modell": ""`-Einträgen für:
    - Natur: `bauteil.kiefer`, `bauteil.toter_baum`, `bauteil.busch`, `bauteil.fels_gross`, `bauteil.felsmauer`
    - Wege & Wasser: `bauteil.holzbruecke`, `bauteil.steg`, `bauteil.hochgeruest`, `bauteil.tunnel_bogen`, `bauteil.fackel`
    - Gebäude & Requisiten: `bauteil.scheune`, `bauteil.holzzaun`, `bauteil.strohballen`, `bauteil.holzkiste`, `bauteil.pfahlhuette`, `bauteil.leuchtturm`, `bauteil.holzboot`, `bauteil.schiffswrack`, `bauteil.eisenturm`, `bauteil.waldhuette`, `bauteil.palisade`, `bauteil.wachturm_holz`, `bauteil.zelt`
  - Solange `"modell": ""` leer ist, baut das Spiel graue/farbige Klötze in den Originalmaßen (für Kollision und Sichtschutz). Sobald Phillip ein FBX/GLB importiert und den Pfad einträgt, liegt das echte Modell über dem Klotz.
- **Landschaftsmaterial `M_Gelaende` mit Schichten (ab K2/K4):**
  - Wir erweitern `make_terrain()` in `tools/make_assets.py` so, dass das Material fünf klar unterscheidbare Farben mit dem 1-Meter-Schachbrett-Raster mischt:
    1. **Gras / Waldboden:** sattes Moos-/Kieferngrün (`0.14, 0.19, 0.09`).
    2. **Sand / Weg / Sandplätze:** heller Küstensand (`0.42, 0.36, 0.25`).
    3. **Fels (an Hängen `> 24°` und über `+7 m`):** kühles Schiefergrau (`0.22, 0.23, 0.25`).
    4. **Schlamm / Moor (`Z` zwischen `-0,8 m` und `0,0 m` im Sumpf):** dunkles Moorbraun (`0.12, 0.11, 0.08`).
    5. **Strand / Meeresboden (`Z < 0,3 m` am Ufer):** nasser Küstensand (`0.28, 0.25, 0.18`).
- **Was Phillip jederzeit von Hand nachbearbeiten kann:**
  - Wie im Kunst-Leitfaden festgelegt, löscht `clear_generated()` in `make_runmap.py` nur Actors mit dem Tag **`EI_Generiert`**.
  - Wenn Phillip eigene Felsen, Bäume, Lichter oder Deko im Editor in `L_ErsterLauf` platziert (ohne das Tag `EI_Generiert`), bleiben sie bei jedem `rebuild_content.ps1` unangetastet stehen.
  - Wenn Phillip später die Landschaft selbst im Landscape-Modus von Hand nachmodellieren oder bemalen möchte, genügt ein kurzer Hinweis (oder ein Schalter `"landschaft_behalten": true` in `waldinsel.json`), dann entfernt das Skript das Tag `EI_Generiert` von `Gelaende` und überschreibt die Landschaft nicht mehr.

---

### 14. Das neue Datenformat (`Data/waldinsel.json`)

Die gesamte Karte wird durch [`Data/waldinsel.json`](file:///C:/Users/Phillip/dev/game-project/Data/waldinsel.json) beschrieben (und speist ab K2/K3 auch `laufkarte.json` und `gelaende.json`). Hier ist ein Ausschnitt der Struktur:

```json
{
  "arbeitsname": "Aschenwerder",
  "boden_m": 600,
  "insel_radius_m": 255,
  "aufloesung": { "teile_pro_seite": 10, "quadrate_pro_teil": 63, "quadrat_cm": 100 },
  "hoehen_m": { "meeresboden": -4.0, "flachwasser": -0.6, "strand": 0.4, "ebene": 0.0, "wellen_hoehe": 2.2, "wellen_laenge": 65.0 },
  "eben_um": { "sandplaetze_m": 22, "bossplaetze_m": 26, "signalfeuer_m": 14, "ankunft_m": 12, "lager_m": 9, "sprungfelder_m": 8, "uebergang_m": 12 },
  "kueste_punkte": [[255, -55], [225, -10], [205, 25], "..."],
  "erhebungen": [
    { "name": "Eisenjoch-West", "x_m": 5, "y_m": 30, "radius_x_m": 55, "radius_y_m": 50, "hoehe_m": 26, "steilheit": 0.45, "rampe_grad": 200 }
  ],
  "senken": [
    { "name": "Steinbruch", "x_m": 110, "y_m": 100, "radius_m": 26, "tiefe_m": 3.5 }
  ],
  "baeche": [
    { "name": "Schluchtbach-West", "punkte": [[175, -165], [130, -155], "..."], "sohle_m": 7, "ufer_m": 4, "tiefe_m": 1.2 }
  ],
  "tunnel": {
    "eingang_nord": { "x_m": 58, "y_m": 65 },
    "eingang_sued": { "x_m": -28, "y_m": 72 },
    "eingang_west": { "x_m": 8, "y_m": -8 },
    "halle_mitte": { "x_m": 14, "y_m": 58, "radius_m": 18, "decke_hoehe_m": 9 },
    "gang_breite_m": 8,
    "gang_hoehe_m": 6
  },
  "wege": {
    "polylinien": [
      { "name": "Innerer Ring", "breite_m": 7.0, "punkte": [[135, -35], [140, 20], "..."] }
    ]
  },
  "sandplaetze": [
    { "id": "S1", "name": "Nordkap-Kreis", "x_m": 235, "y_m": -55, "radius_m": 22, "nutzung": "Ankunft Nord / Nebenauftrag" }
  ]
}
```

---

### 15. Geplante automatische Tests (K2 bis K9)

In `Source/Eisenseelen/Private/Tests/EIRunTest.cpp` erweitern wir `Eisenseelen.Play.Run.Terrain` (und ergänzen gezielte Prüfungen in den jeweiligen Schritten):

1. **Alle Spielorte eben und auf Land (`K2/K3`):**
   - Prüft für alle 4 Ankünfte, 4 Bossplätze, 4 Signalfeuer, 8 Altar-Orte, 18 Lager, 5 Sprungfelder und 14 Truhen: Die Bodenhöhe liegt über dem Wasserspiegel (`Z >= -5 cm`) und ist im `5 m`-Umkreis eben (`±30 cm`).
2. **Kein Spielort und kein Wellen-Spawn im tiefen Wasser (`K4`):**
   - Prüft, dass kein Spielort unter `Z = -0,15 m` liegt und dass `UEIHordeDirector::FindStandingPlace` Punkte unterhalb des Meeresspiegels (`Z < -0,5 m`) ablehnt.
3. **Vollständige Route Ankunft → Boss → Signalfeuer → Barke zu Fuß erreichbar (`K3/K4`):**
   - Prüft über `UNavigationSystemV1::FindPathToLocationSynchronously` für **alle 4 Ankünfte**, dass von dort jeder Bossplatz, von jedem Bossplatz jedes gültige Signalfeuer und von dort jedes Lager, jeder Altar und jede Truhe über das Wegenetz (inklusive Brücken) erreichbar ist.
4. **Der Tunnel durch das Massiv ist durchgehend begehbar (`K5`):**
   - Prüft einen NavMesh-Pfad vom Nord-Eingang `(58, 65)` durch die Tunnel-Halle `(14, 58)` zum Süd-Eingang `(-28, 72)` sowie zum West-Stollen `(8, -8)`: Der Pfad muss durch die Halle führen (also kürzer als `130 m` sein und darf nicht `350 m` weit außen um das Massiv herumlaufen) und über der Tunnel-Halle muss ein Dach (`LineTrace` nach oben trifft das Felsdach) liegen.
5. **Der Wald blockiert keine Wege (`K6`):**
   - Prüft entlang aller Weg-Polylinien alle `5 m` mit einer Kapsel-Abfrage (`Sweep` / `Overlap`), dass kein Baumstamm und kein Felsen auf der Fahrbahn steht.
6. **Leistung und Instanzen (`K6`):**
   - Prüft, dass die Bäume als Instanzen (`UInstancedStaticMeshComponent`) zusammengefasst sind (wenige Actor-Container statt > 500 Einzel-Actors) und misst die Frame-Zeit im Lauf.
7. **Messung der Laufzeiten (`K3`):**
   - Rechnet im Test aus den echten NavMesh-Pfadlängen (`Path->GetLength()`) zwischen Ankunft, Bossplatz und Signalfeuer die genaue Lauf- und Sprintdauer in Sekunden aus und gibt sie im Testbericht (`AddInfo`) aus.

---

### 16. Risiken und offene Fragen (Entscheidungen vor Schritt K2)

| Risiko | Gegenmaßnahme im Plan |
| --- | --- |
| **Gegner spawnen bei Wellen im tiefen Meer**, weil der Meeresboden zur `ALandscapeProxy` gehört | In `UEIHordeDirector::FindStandingPlace` eine Zeile ergänzen: Auf `ALandscapeProxy` gilt ein Platz nur, wenn `Floor.ImpactPoint.Z >= -40.0f` (also nicht im tiefen Meer). |
| **Die Barke bleibt beim Landen am Signalfeuer in Bäumen hängen** | Alle 4 Signalfeuer liegen auf den `44 m` breiten, baumfreien Sandkreisen (`S2, S3, S11, S12`), sodass die Barke `12 m` landeinwärts völlig frei einschweben kann. |
| **Truhen oder Deckung werden im Meer gewürfelt** | `is_open_ground(x, y)` in `make_runmap.py` prüft `0.25 <= ground_m(x, y) <= 2.5` und schließt damit Meer, Bäche, Steinbruchsohle und Berggipfel sauber aus. |
| **Bestehende Tests mit festen Koordinaten (`EIContractTest`, `Run.Complete`, `Run.Pathfinding`) brechen** | In Abschnitt 1.2 sind alle drei Stellen genau benannt und werden in K2/K3 zusammen mit den Daten angepasst. |

#### Bestätigte Entscheidungen von Phillip (vor dem Start von K2):
1. **Name der Karte** (für die Jagdbriefe unter `"gebiet"` in `Data/laufkarte.json`):
   - **Bestätigt: `Aschenwerder`**
2. **Größe der Karte:**
   - **Bestätigt: `600 × 600 m` Gesamtlandschaft mit `~490 m` Insel-Durchmesser** (2–3 Min. Querung zu Fuß, 4–6 Min. Ankunft → Boss → Signalfeuer)
3. **Wie soll sich das Wasser verhalten?**
   - **Bestätigt: Flaches Wasser (Strand/Bäche/Sumpf) frei begehbar mit `85 %` Tempo, tiefes Meer blockiert durch eine unsichtbare Küstenwand**
4. **Bauweise des Tunnels durch das Felsmassiv:**
   - **Bestätigt: Variante A — Eingeschnittene Schlucht auf Höhe `0 m` mit Felsdach und Seitenwänden aus Bauteilen** (zuverlässigste Wegfindung für Feinde)
5. **Was passiert mit der alten Wüstenkarte (`Aschesteppe` aus W1)?**
   - **Bestätigt: Alte Wüstendaten komplett löschen und `Data/gelaende.json` sowie `Data/laufkarte.json` ab K2/K3 direkt überschreiben**

---

## 3. Umsetzungsstand der Schritte

- **K0 Bestandsaufnahme:** erledigt
- **K1 Detailplan & Planbild:** erledigt
- **K2 Grundform (`600 × 600 m` Insel-Landschaft, Höhenplan, geschichtetes `M_Gelaende`, `Run.Terrain`):** erledigt (58/58 Tests grün)
- **K3 Spielorte neu verteilen:** erledigt (4 Küsten-Ankünfte, 4 Bossplätze, 4 Signalfeuer, 8 Altäre/Schreine, 18 Lager, 5 Sprungfelder, 14 Truhen; Bannbruch erst nach Abschluss aller 3 Nebenaufträge)
- **K4 Wasser und Wege:** erledigt (Wasserspiegel `M_Wasser` auf `-0,15 m`, unsichtbare Tiefwasser-Küstensperre, `85 %` Tempo im Flachwasser, 9 Holzbrücken/Stege mit `M_Holz`, alle Wege als Sandschicht in `M_Gelaende`)
- **K5 Felsmassiv und Tunnel:** erledigt (Y-förmiger Tunnel durch das Eisenjoch mit Mittelhalle, Banditenlager `lager[0]`, 4 Stützpfeilern, Felsdach, 3 Portalen und Fackelbeleuchtung; 25 Klippen/Schären und 8 niedrige Felsmauern aus `bauteil.*` mit `M_Fels`)
- **K6 Wald:** als Nächstes
- **K7 Orte mit Charakter:** offen
- **K8 Orientierung und Stimmung:** offen
- **K9 Feinschliff & Doku:** offen
