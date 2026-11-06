# UI-Stil: Design und Animationen

Phillips Auftrag vom 6. November 2026: Schau dir Far Far West (UI/HUD) ganz genau an — Layouts, Animationen, Designdetails — und plane, wie wir das für Eisenseelen schön und passend umsetzen. Dieses Doc ist der Design-Teil; der Funktions-Umbau steht in `docs/UI-und-HUD.md` (Schritte U1–U8). Hier steht, **wie alles aussieht und sich bewegt**, nicht was es tut.

## 1. Was Far Far West konkret macht (Recherchestand 6. November 2026)

Quellen: Steam-Diskussionen, farfarwestguide.com, wikily.gg, Try Hard Guides/NGOHQ-Reviews.

**Stil und Material**

- Handgezeichnete, kräftige Optik (Vergleich Sea of Thieves): dicke Umrisse, gesättigte Farben, verspielter „Weird West"-Charakter (Robo-Cowboys, Skelett-Banditen, Magie).
- Die UI trägt denselben Stil: Rahmen, Knöpfe, Icons wirken wie Steckbriefe („Wanted"-Poster) oder Dampfmaschinen-Bauteile. Menüs und Listen sehen aus wie auf raues Papier geschrieben — Kanten eingerissen, gealtert, an Holz festgetackert.
- Typografie: Slab-Serifen und „gestempelte", aufgeschürfte Schriften wie auf alten Western-Plakaten. Trotz Stil gut lesbar.
- Reviews heben hervor: Die visuelle Identität ist eine der größten Stärken;combat-Feedback (Treffer, Elementarwaffen) gilt als sehr befriedigend. Kritikpunkt: In koop-chaotischen Momenten kann das Bild voll wirken — Gegenmittel ist klare Hierarchie und wenige, große Anker.

**Hub und Menüs**

- Der Saloon ist das Menü: Waffenschmied, Zauberhütte, Labor, Kosmetik, Schießstand, Bestiarium — jede Station ein Ort mit NPCs, keine verschachtelten Fenster (Details in `docs/UI-und-HUD.md`).
- Die „Papier"-Ästhetik zieht sich durch alle Menüs: Shop-Angebote, Upgrade-Bäume und Quest-Karten sind Plakate/Notizen auf Holz.

**HUD im Lauf**

- Koop: Portraits, Leben und Zustand aller Teammitglieder sind dauerhaft sichtbar; Missionsziel und Timer bleiben auch im Boss-Chaos lesbar.
- Schadenszahlen erscheinen animiert am Einschlagort (Pop → Schrumpfen/Verschwinden); besondere Treffer farbig/größer.
- Boss-Kämpfe: breite Leiste oben, Name und Fortschritt; Phasenwechsel werden deutlich inszeniert (Effekte auf der Leiste, Geräusche).

**Karte**

- Weltkarte top-down mit Landmarken; interaktive Filter je Kategorie (POI-Toggles: Gräber, Sammelscheiben, Fast-Travel, NPCs) — man blendet ein, was einen interessiert.
- Fortschritts-Tracker je Region („x/y gefunden") und Schwierigkeits-Anzeige.

**Der wichtigste Negative-Befund:** FFW hat einen echten UI-Fehler, über den sich Spieler beschweren — die eigene Lebensleiste springt zwischen Positionen (solo unten Mitte, in der Gruppe oben links). Spieler sterben, weil sie ihre Leiste nicht finden. **Lehre für uns: Die eigene Lebensleiste steht IMMER am selben Ort, egal ob solo oder zu viert.** Die Mitspieler sind zusätzliche, klar getrennte Anzeigen, nie ein Ersatz.

## 2. Übersetzung nach Eisenseelen: „Geschmiedetes Eisen"

FFW ist Western: Holz, Leder, Seile, Sepia, Sonnenglut. Eisenseelen ist düstere Fantasy: Asche, Frost, Eisen, ein totes Land unter grauem Himmel. Wir übernehmen die **Struktur** (Papier-Plakat-Menüs, klare Anker, handgemachter Charakter), nicht die Western-Farben. Unser Gegenstück:

| FFW | Eisenseelen |
| --- | --- |
| Holz, Leder, Seil, warmes Sepia | Geschmiedetes, stellenweise verrostetes Eisen; gebrannte Platten, genietete Ränder |
| „Wanted"-Poster auf Papier | Geritzte Eisen-Plakate / Runen-Tafeln: dunkles Metall, in das Zeichen eingraviert sind; wo FFW Tacker und Falten zeigt, zeigen wir Nieten und Risse |
| Gestempelte Slab-Serifen | Einfache, breite Gravur-Schrift (siehe Typografie unten) — wirkt gemeißelt statt gedruckt |
| Knallige Cartoon-Farben auf Papierweiß | Gedeckte Farben auf dunklem Grund: Aschegrau, Eisenblau, kaltes Runenleuchten als einzige „warmen" Akzente (Frost-Blau, Feuer-Orange, Glut-Rot) |
| Saloon-Hub | Anlegestelle mit Barke (steht schon so im Plan) |

**Farbpalette (Arbeitswerte, später in `Data/ui.json`):**

- Hintergrund-Flächen: Aschegrau, halbtransparent (heutiges PanelColor bleibt erstmal).
- Normal-Text: helles Grau-Weiß wie heute (TextColor).
- Akzente wie heute, aber als festes System: Leben rot, Erholung grün, Mana blau, Runen hellblau, Aufladung orange, Feuer orange, Frost eisblau, Gold/Beute warmes Gelb (RewardColor).
- EIN Leucht-Akzent pro Anzeige: Was wichtig ist, bekommt einen kalten Runen-Schein (später als Glow-Asset), alles andere bleibt matt. Das ist unser Gegenstück zu FFWs Sonnen-Glut und beugt dem Chaos-Eindruck vor, den FFW-Kritiker erwähnen.

**Typografie:** Solange alles Code-gezeichnet ist, bleibt die Engine-Schrift (GetMediumFont). Der Wechsel auf eine lizenzierte Gravur-Schrift ist Teil der Optik-Schicht (U8): eine Schrift, zwei Größen (grob „Überschrift" ×1.6 und „Text" ×1.0), mehr nicht. Eine kostenlose Slab-Serif mit ruhigen Formen (kein Cartoon) ist das Ziel — Phillip sucht sie aus, wir bereiten einen einzelnen Font-Ladeplatz vor.

## 3. Layout-Grundsätze: Bildschirm-Anker

Der HUD-Bildschirm hat feste Anker-Zonen. Jede neue Anzeige (aus `docs/UI-und-HUD.md` A1–A10) bekommt genau einen Anker zugewiesen — nichts Schwimmendes. Das ist die Antwort auf FFWs „im Chaos zu voll"-Problem und auf den FFW-Positionier-Fehler.

```
+--------------------------------------------------------------+
| [Mitspieler A][Mitspieler B]        RUN-UHR / BLUTMOND       |
|                                    BOSS-LEISTE (breit)       |
|                                                              |
|                                                              |
|                          FADENKREUZ                          |
|                    (F-Hinweis, Parade-Meldung)               |
|                                                              |
| ZIEL-LEISTE (Verträge)                                         |
|                                    BEUTE-MELDUNGEN (Toasts)  |
|  LEBEN | MANA | Q | E            (Tempo, Gefallen)            |
+--------------------------------------------------------------+
```

- **Unten Mitte: die eigene „Platte".** Leben, Mana, Zauber/Runen-Slots (Q/E) als eine zusammenhängende geschmiedete Platte. Diese Zone wechselt NIE (die FFW-Lehre). Bei Erholung leuchtet der Lebensbalken grün (existiert schon).
- **Oben Mitte:** Run-Uhr und Blutmond-Hinweis (heute Steuerungshilfe + Uhr), darunter die große Boss-Leiste (A6), sobald der Boss wacht. Breit wie die heutige BossBarWidth (620 px), aber mit Name und Bannbruch-Zähler in der Leiste statt darunter.
- **Oben links:** Mitspieler-Rahmen (A1) — kleine Reihen: Name, Klasse, Leben, Mana, Zustand. Erst wenn Koop-Daten da sind (U2).
- **Unten links:** Vertrags-Fortschritt kompakt (A5): eine Zeile je Vertrag, Ziel + Abstand.
- **Unten rechts:** Beute-Meldungen (A3) und Schadenszahlen-Regeln gelten nur für eigene Treffer (A2) — Zahlen in der Welt am Gegner, Beutezeilen am Bildschirmrand.
- **Mitte:** nur Flüchtiges — Fadenkreuz, F-Hinweise, Aufladebalken, Reaktions-Aufblitzen (A9), Levelaufstieg (A8) als kurzer großer Schriftzug, der durchschwingt.
- **Alles andere (Bedrohungs-Marken, Treffer-Pfeil) bleibt wie heute am Rand.**

Größen-Hierarchie: Wichtiger = größer und heller; unwichtige Infos werden kleiner und dunkler, bevor sie ganz verschwinden. Genau eine Sache darf gleichzeitig „groß" sein (Boss-Leiste im Kampf, Levelaufstieg im Moment des Aufsteigens).

## 4. Animationen — der Katalog

Alles hier ist mit dem heutigen Canvas-HUD machbar: Wir haben schon ein Animations-Muster im Code (`CrossingFade` mit `FInterpConstantTo` und `RenderDelta`). Wir bauen daraus eine kleine Helfer-Bibliothek (`EIHudDraw`-Hilfen in der HUD-Klasse), alle Zeiten aus `Data/ui.json`.

| # | Name | Was es tut | Wo | Parameter (Startwerte) |
| --- | --- | --- | --- | --- |
| S1 | Weiches Ein-/Ausblenden | Alpha läuft mit konstanter Geschwindigkeit gegen 0 oder 1 | Karten-Overlay (M), Menü-Reiter, Pause-Menü | 0.25 s |
| S2 | Balken-Nachlauf („Geist") | Hinter jedem Wert-Balken läuft ein zweiter, hellerer Balken langsamer nach — wie bei Prügelspielen: Man sieht, wie viel gerade abgebaut wurde | Leben, Mana, Boss-Leiste, Bannbruch | Nachlauf 0.6 s verzögert, dreifache Balkendauer |
| S3 | Pop-Schadenszahl | Zahl erscheint am Gegner, steigt leicht auf, skaliert kurz hoch (1.0 → 1.35 → 1.0), verblasst | Eigene Treffer (A2) | Lebensdauer 0.8 s, Aufstieg 40 px, Reaktionen in Reaktionsfarbe |
| S4 | Toast-Zeile | Beute-Zeile schiebt sich von rechts ein, steht, verblasst nach oben | Beute-Meldungen (A3) | Einschub 0.2 s, Stand 2.5 s |
| S5 | Puls | Sinus-Atmung von Größe/Alpha | Was Aufmerksamkeit will: volle Aufladung, bereitender Boss-Bannbruch, Blutmond-Uhr in letzter Minute | Periode 1.0 s, ±15 % |
| S6 | Treffer-Vignette | Der heutige HurtColor-Schleier bekommt einen kurzen harten Aufschlag, der abklingt | Eigener Treffer | Aufschlag 0.15 s, Abklingen 0.4 s (heutiges HurtColor als Basis) |
| S7 | Boss-Erschütterung | Beim Phasenwechsel/Bannbruch-Stufe ruckt die Boss-Leiste kurz horizontal | Boss-Leiste (A6) | Amplitude 8 px, Abklingzeit 0.5 s |
| S8 | Durchschwingen | Große Text-Zeilen (Gebietsname bei der Überfahrt, Levelaufstieg A8, „LAUF GESCHEITERT") skalen von klein auf groß und leicht über das Ziel hinaus zurück | Überfahrt, Abrechnung, Levelauf | 0.4 s |
| S9 | Karten-Aufbau | Die M-Karte baut sich in zwei Schritten auf: erst Fläche + Grenzen, dann die Marken nacheinander (30 ms Versatz je Marke) | M-Karte | 0.3 s gesamt |
| S10 | Slot-Blinken | Q/E-Slots blinken kurz auf, wenn bereit/nicht mehr bereit (vor allem: Mana reicht wieder) | Zauber/Runen-Slots | Aufblinken 0.2 s |

Regeln dazu:

- **Alles abklingend, nichts dauerhaft nervig:** Jede Animation endet in einem Ruhezustand. Dauerschleifen gibt es nur als Puls (S5) und nur an den zwei genannten Stellen.
- **Lesbarkeit schlägt Schönheit:** Schadenszahlen pro Gegner max. ~4 gleichzeitig (älteste verschwinden zuerst); Toasts max. 5 Zeilen.
- **Alles abschaltbar:** HUD-Größe und -Detail regelt später das Einstellungen-Menü (D2). Die Schadenszahlen-Entscheidung (nur eigene) steht schon.
- **Zeiten in Daten:** Jede Dauer/Auslenkung aus S1–S10 landet in `Data/ui.json`, damit Phillip ohne Neubau JUSTIEREN kann (Hausregel).

## 5. Die einzelnen Elemente im Design

**Eigene Platte (unten Mitte):** Ein breiter, flacher „Amboss"-Block: Lebensbalken (mit Geist-Nachlauf S2, grün bei Erholung), darunter Mana, daneben zwei quadratische Slots Q (Zauber) und E (Rune) mit eigenem kleinen Füll-Balken (Abklingzeit) und Name darunter. Slots sind die ersten Icon-Plätze (siehe Abschnitt 6): heutefarbige Kacheln mit Kurztext (z. B. „FROST"), später laufen dort die 2D-Icons der Zauber/Runen.

**Boss-Leiste (oben Mitte, A6):** Dünner Stein/Riss-Rahmen (später Asset), Name links in der Leiste, Leben füllt nach rechts, Bannbruch-Zähler rechts („Bann 2/4"). Geist-Nachlauf (S2), Erschütterung (S7) bei jeder Stufe. Verschwindet weich (S1), wenn der Boss schläft/erstirbt.

**Mitspieler-Rahmen (oben links, A1):** Pro Mitspieler eine kleine Tafel: Name + Klassen-Kürzel, zwei schmale Balken (Leben rot, Mana blau), Zustands-Kürzel als kleine farbige Punkte (brennt orange, gefroren eisblau, verlangsamt blass). Geht mit S1 weich an und aus, wenn jemand beitritt/verlässt. Eigene Platte bleibt davon unberührt (FFW-Lehre).

**Schadenszahlen (A2):** Weiße Grundzahl, Reaktions-Treffer in der Reaktionsfarbe (Dampfexplosion blass-weiß, Brand orange, Frost eisblau), Kraft-/slows-Kombinationen größer. Nur eigene Treffer.

**Beute-Toasts (A3):** Zeilen unten rechts: „+12 Gold" in RewardColor, „+1 Knochensplitter" mit Zutaten-Kürzel. Wiederholtes derselben Beute aktualisiert die Zeile („+24 Gold") statt neue Zeilen zu schieben.

**M-Karte:** Aufbau wie heute (Rechteck + Liste), ergänzt um Legende (B1), entdeckte Lager als graue Punkte (B2), Mitspieler (B3), Gebietsgrenzen als Kreise (B5). Aufbau-Animation S9. POI-Filter wie bei FFW: Taste zum Durchschalten (alle / nur Verträge / nur Feuer) — ein Satz Tasten reicht, kein Maus-Menü im Canvas.

**Menüs (Anlegestelle):** Reiter bleiben Elementlisten (Test-Muster!), aber jede Liste bekommt das Plakat-Layout: Titel-Zeile in Gravur-Großschrift, Einträge als Zeilen mit Icon-Platz links (farbige Kachel + Kürzel heute), Text + Zahlen rechts. Auswahl nicht invertiert-weiß, sondern „Niet-Rahmen": hellerer Doppelrahmen um die gewählte Zeile — schöner als Schwarz/Weiss-Kippen und ohne Asset machbar.

**Abrechnung + Überfahrt:** Beide bekommen den Durchschwing-Schriftzug (S8); die Abrechnung ihre Zeilen nacheinander (30 ms Versatz, wie S9) — das macht das Ende eines Laufs zu einem Moment, ohne dass etwas Neues entsteht.

**Titelbild (D1, offen):** Wenn es kommt: schwarzer Grund, kalter Runen-Schein auf dem Schriftzug, „Start / Testarena / Beenden" als drei breite Niet-Platten. Keine 3D-Szene nötig — erst Funktionalität.

## 6. Icon-Platz-System (der 3D-/2D-Asset-Pfad)

Wie die Karte schon Orte für echte Modelle vorbereitet, bereitet die UI Bild-Plätze vor. Jeder Icon-Platz ist ein benannter Slot:

- **Name:** z. B. `hud.slot.q`, `hud.slot.e`, `menu.item.<id>`, `boss.portrait`, `settlement.header`.
- **Heute:** farbige Kachel + Kurzkürzel aus `Data/ui.json`.
- **Später:** Phillip legt an denselben Platz ein 2D-Bild (Icon) oder gerendertes 3D-Asset — der Platz kennt Größe und Ausrichtung, die Logik bleibt gleich. Wo FFW ein Poster andrawt, haben wir eine „Tafel", die einmal ein geritztes Eisen-Bild aufnehmen kann.
- Umsetzung: eine zentrale Stelle im HUD, die alle Icon-Plätze zeichnet (heute Kachel, später Textur-Lookup nach Name). Einmal bauen, überall nutzen.

## 7. Umsetzung in Schritten

Die Optik-Schicht baut auf U1–U8 aus `docs/UI-und-HUD.md` auf; die Reihenfolge dort bleibt. Diese Schritte hier füllen U3/U8 und die Optik aus:

- **ST1 — Animations-Basis:** Helfer für S1, S2, S5, S8 (die vier universellen) + `Data/ui.json` mit allen Zeiten. Basis für alles Folgende; unabhängig von neuen Anzeigen einbaubar (erst an Überfahrt/Abrechnung und den bestehenden Balken zeigen).
- **ST2 — Schadenszahlen + Toasts (S3, S4, S6):** zusammen mit U3 (Meldungs-Dienst). Der Meldungs-Dienst liefert die Daten, die Animation nur die Darstellung.
- **ST3 — Anker-Ordnung:** Bestehende Anzeigen auf die Anker-Zonen verteilen (Boss-Leiste nach oben, Vertragszeilen nach unten links) — reines Umstellen im Zeichencode, keine Logikänderung.
- **ST4 — Icon-Platz-System:** zentrale Zeichenstelle + erste Slots (Q/E, Menü-Einträge).
- **ST5 — Karte verfeinern:** B1–B5 + S9 + POI-Umschaltung (mit U5).
- **ST6 — Boss-Leiste voll (A6 + S7):** mit U4.
- **ST7 — Schrift & Rahmen (Schrift-Ladeplatz, Niet-Rahmen für Auswahl, Plakat-Titel in Menüs).**
- **ST8 — Voll-Ausstattung der Icon-Plätze**, sobald Phillips erste Assets da sind (der eigentliche U8).

Jeder Schritt endet mit grünen Tests; die Animationen selbst sind visuell (nicht testbar), aber ihre **Zustände** sind es: Der Meldungs-Dienst hat eine Liste (Tests prüfen Inhalte und Reihenfolge), Balken haben Werte (Tests prüfen Werte, nicht Pixel), Icon-Plätze haben Namen (Tests prüfen, dass jeder benannte Slot existiert).

## 8. Offene Fragen

1. **Schrift:** Soll Phillip eine konkrete freie Schrift aussuchen (z. B. eine ruhige Slab-Serif), oder bleiben wir solange bei der Engine-Schrift, bis die Asset-Schicht kommt?
2. **Titelbild-Timing:** (bleibt offen aus `docs/UI-und-HUD.md`) früh oder erst mit Koop?
3. **Tastensteuerung der Menüs:** (bleibt offen) — beeinflusst, ob die „Niet-Platten"-Optik der Knöpfe von Anfang an Tastatur-freundlich gebaut wird.
