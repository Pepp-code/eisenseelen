# Kunst-Leitfaden

So kommen echte 3D-Modelle und Effekte ins Spiel, ohne dass sich am Spielen etwas ändert. Stand: 6. November 2026.

## Das Prinzip

Alles im Spiel besteht heute aus grauen Klötzen. Diese Klötze bleiben die Wahrheit: Getroffen, geblockt und angestoßen wird immer an ihnen. Ein Modell liegt nur darüber und versteckt sie.

Welches Modell wo liegt, steht in `Data/aussehen.json`. Jede Figur, jede Waffe, jedes Rüstungsteil und jeder Ort hat dort schon eine Zeile mit leerem `"modell": ""`. Leer heißt: Die Klötze bleiben. Trägst du einen Pfad ein, erscheint das Modell beim nächsten Spielstart, und die Klötze darunter verschwinden. Ein falscher Pfad zeigt eine rote Meldung, und die Klötze bleiben stehen. Kaputt geht dabei nichts.

## Ein Modell einsetzen

1. Lege das Modell im Content Browser in den passenden Ordner unter `Kunst` (siehe unten). Ziehe dazu die Datei (FBX oder GLB) einfach hinein.
2. Rechtsklick auf das Modell, dann „Copy Reference“.
3. In `Data/aussehen.json` die Zeile suchen, zum Beispiel `gegner.knochenlaeufer`, und den kopierten Text bei `modell` einfügen.
4. Speichern, das Spiel stoppen und neu starten.
5. Passt Größe oder Richtung nicht, ändere `skalierung`, `versatz_cm` oder `drehung_grad` und starte neu.

Ein volles Beispiel mit allen Werten steht oben in der Datei unter `_beispiel`.

## Die Ordner

| Was | Ordner | Zeile in `aussehen.json` |
| --- | --- | --- |
| Helden: Körper für die anderen | `Kunst/Helden` | `held.klingenwacht`, `held.arkanist` |
| Helden: deine Hand vor der Kamera | `Kunst/Helden` | `hand.klingenwacht`, `hand.arkanist` |
| Gegner | `Kunst/Gegner` | `gegner.knochenlaeufer` und so weiter |
| Bosse | `Kunst/Bosse` | `boss.Der Gefesselte`, `boss.Der Seelenrufer` |
| Waffen | `Kunst/Waffen` | `form.langschwert` für alle Langschwerter, `item.Frostklinge` nur für diese eine |
| Rüstung | `Kunst/Ruestung` | `item.` und der Name des Teils |
| Orte | `Kunst/Orte` | `ort.truhe`, `ort.barke` und so weiter |
| Effekte | `Kunst/Effekte` | `effekt.feuerball` und so weiter, siehe unten |

Eine Waffe mit eigenem `item.`-Eintrag nutzt diesen. Ist er leer, gilt das Modell ihrer Grundform. So reicht für den Anfang ein Modell je Grundform.

Runen haben keine Zeile. Man sieht sie nur als Zauber, und deren Aussehen steht unter `effekt.` (siehe „Effekte“).

## Regeln für jedes Modell

- **Maßstab:** 1 Einheit sind 1 cm. Der Held ist etwa 180 cm groß. Am einfachsten vergleichst du in der Testarena mit den Klötzen.
- **Nullpunkt:** an den Füßen oder auf der Standfläche, nicht in der Mitte.
- **Vorn** ist die rote X-Achse. Figuren aus dem Unreal-Mannequin und von Mixamo schauen zur Seite, sie brauchen `"drehung_grad": [0, 0, -90]`.
- **Drehung:** `drehung_grad` hat drei Zahlen in der Reihenfolge X, Y, Z wie im Details-Fenster des Editors.
- **Figuren** (Helden, Gegner, Bosse) sollten ein Skelett haben, sonst stehen sie starr. Orte und Waffen brauchen keins.
- **Waffen:** Nullpunkt am Griff, die Klinge oder der Stab zeigt nach vorn (X). Die Heldenhand und der Heldenkörper brauchen am Skelett einen Sockel namens `waffe_r` in der rechten Hand. Ein anderer Name geht über `sockel`.
- **Rüstung** mit Skelett muss dasselbe Skelett haben wie der Heldenkörper. Dann bewegt sie sich mit ihm. Rüstung ohne Skelett hängt an dem Sockel, der bei `sockel` steht, zum Beispiel ein Helm an einem Sockel am Kopf.
- **Leuchtteile:** Kristalle, Glut, Siegel und die Elementfarbe an der Klinge sind eigene Teile. Sie bleiben über dem Modell sichtbar, außer bei `"leuchtteile_behalten": false`. Bei Waffen und Helden steht das schon auf `false`, weil ihre Leuchtteile mit dem Klotz zusammenhängen.

## Animationen

Eine Figur mit Skelett kann auf zwei Arten Animationen bekommen.

**Einfach:** Unter `animationen` stehen einzelne Animationen je Moment. Fehlt eine, nimmt das Spiel die nächstbeste:

| Moment | Wann | fehlt sie, gilt |
| --- | --- | --- |
| `stehen` | steht still | nichts |
| `laufen` | geht | `stehen` |
| `rennen` | ist schnell | `laufen` |
| `springen` | ist in der Luft | die Bewegung am Boden |
| `ausholen` | holt aus, Warnung vor dem Angriff | nichts |
| `schlag` | schlägt zu | nichts |
| `wurf` | wirft | `schlag` |
| `ansturm` | stürmt los | nichts |
| `stampfen` | stampft | `schlag` |
| `beschwoeren` | ruft Gegner | `ausholen` |
| `verschwinden` | verschwindet und taucht woanders auf | `ausholen` |
| `treffer` | wird getroffen | nichts |
| `taumeln` | taumelt | nichts |
| `bemerkt` | sieht dich zum ersten Mal | nichts |
| `tod` | fällt und bleibt ein paar Sekunden liegen. Ohne sie zerfällt die Figur in Knochen wie heute. | nichts |
| `hieb`, `schwerer_hieb`, `blocken`, `schuss`, `zauber`, `rune` | was der Held tut | `schwerer_hieb` nimmt `hieb` |
| `aktiv` | ein Ort ist an: die Truhe offen, der Altar und das Signalfeuer brennen, der Ankerstein ist gebrochen, die Barke fliegt, Kessel und Grab sind voll | nichts |

**Fortgeschritten:** Unter `anim_blueprint` steht ein Animation Blueprint. Das Spiel schreibt dann zwei Werte hinein: `Tempo` (Zahl, cm pro Sekunde) und `InDerLuft` (ja oder nein). Die Momente aus der Tabelle spielt es über den Slot `DefaultSlot` ab. Der Blueprint darf nur Animationen auswählen und keine Spiellogik enthalten.

## Effekte

Jeder Zauber, jede Reaktion und jeder Zustand hat schon einen sichtbaren Effekt aus leuchtenden Ersatzteilen: Kugeln, Würfel, Ringe, Blitze und ein Licht in der Farbe des Elements. Jeder Rechner zeigt seine eigenen, der Server spielt keine. Die Effekte sind nur fürs Auge, Schaden und Reichweite ändern sie nicht.

Ein echter Niagara-Effekt kommt genauso ins Spiel wie ein Modell: In `Data/aussehen.json` steht für jeden Effekt eine Zeile `effekt.<name>` mit leerem `"modell": ""`. Dort kommt der Pfad des Niagara-Systems hin (Rechtsklick, „Copy Reference“). Dann spielt der Niagara-Effekt statt der Ersatzteile. Mit `"leuchtteile_behalten": true` bleiben die Ersatzteile zusätzlich sichtbar. Ein falscher Pfad oder etwas, das kein Niagara-System ist, zeigt eine rote Meldung, und die Ersatzteile bleiben.

| Gruppe | Zeilen |
| --- | --- |
| Zauber | `feuerball` (Flug), `feuerball_explosion`, `eislanze` (Flug), `eislanze_treffer`, `bolzen` (Flug des Stabschusses), `bolzen_treffer`, `kettenblitz` (ein Sprung von Ziel zu Ziel), `windstoss` |
| Runen | `frostnova`, `gewitterfeld`, `flammenwand`, `windhose` |
| Reaktionen | `feuerwirbel`, `frostwirbel`, `gewitterwirbel`, `dampfexplosion` (mit Wolke), `funkenflug` (ein Funke zu einem Ziel), `splitterbruch`, `blitzschlag` (Blitz aus Gewitterfeld und Gewitterwirbel) |
| Zustände am Gegner | `zustand_brennend`, `zustand_verlangsamt`, `zustand_gefroren` |
| Elementmal am Gegner | `mal_feuer`, `mal_eis`, `mal_blitz`, `mal_wind`: zeigt, welches Element schon auf ihm liegt, damit man die Reaktion planen kann |
| Sonstiges | `glutschaedel_explosion`, `truhe_siegel` (das Siegel der Truhe bricht) |

Das Spiel gibt jedem Niagara-System diese Werte mit. Lege sie im System als User Parameter an, wenn du sie brauchst:

| Name | Art | Bedeutung |
| --- | --- | --- |
| `Farbe` | Linear Color | Farbe des Elements |
| `Radius` | Float | Größe in cm, zum Beispiel der Kreis der Frostnova oder die halbe Länge der Flammenwand |
| `Dauer` | Float | Sekunden, die ein Feld oder eine Wolke bleibt |
| `Winkel` | Float | Breite des Windstoßes in Grad |
| `Ziel` | Vector | Ende eines Blitzes oder Funkens, vom Startpunkt aus gemessen |

Regeln für Effekte:

- **Nullpunkt:** Kreise und Wirbel stehen mit der Mitte am Boden. Flüge, Zustände und Male hängen an ihrem Ding und bewegen sich mit. Zustände sitzen in der Körpermitte.
- **Richtung:** Vorn (X) ist die Flugrichtung oder die Richtung des Windstoßes. Die Flammenwand steht quer dazu.
- **Länge:** Flüge, Zustände und Male laufen, bis das Spiel sie beendet. Dann deaktiviert es das System, und was schon fliegt, darf noch auslaufen. Lege solche Effekte darum als Schleife an. Felder und Wolken bekommen ihre Länge über `Dauer`, alles andere ist ein einmaliger Ausbruch und endet von selbst.

## Was bleibt, wie es ist

- **Treffer und Wände:** Die Kapseln und Klötze darunter entscheiden. Ein großes Modell trifft nicht weiter, und ein kleines ist nicht schwerer zu treffen.
- **Warnfarben:** Ausholen, Treffer, Verlangsamt und Taumeln liegen als farbiger Schimmer über dem Modell (Material `M_Ueberzug`). Dafür ist nichts zu tun.
- **Anzeigen:** Kreise und Bahnen am Boden, Marken, Lebensbalken und Schrift bleiben einfache Formen. Sie sollen klar zu lesen sein.

## Die Karten

`tools/rebuild_content.ps1` baut die Karten aus `Data/testarena.json` und `Data/laufkarte.json` neu. Dabei ersetzt es nur, was es selbst gebaut hat. Diese Dinge tragen das Tag `EI_Generiert`. Was du selbst in eine Karte stellst, zum Beispiel Bäume, Felsen oder Licht, bleibt beim Neubau stehen. Kopiere also nie ein erzeugtes Ding, um es zu verschieben, denn die Kopie trägt das Tag mit und verschwindet beim nächsten Neubau.

Der Boden der Laufkarte ist eine Landschaft namens „Gelaende“, ihre Form steht in `Data/gelaende.json`. Auch sie trägt das Tag und entsteht bei jedem Neubau neu. Ihr Material `M_Gelaende` ist ein Ersatz: Steppenfarbe mit Metergitter, an steilen Hängen Fels. Das echte Material mit Schichten für Asche, Fels, Sand und Pfad kommt in Schritt W6. Willst du die Landschaft von Hand formen oder bemalen, sag es vorher. Dann bekommt sie das Tag nicht mehr, und der Neubau lässt sie stehen.

## Die Reihenfolge

1. **Modelle** über `Data/aussehen.json`. Das geht ab jetzt, Stück für Stück. Am meisten bringt zuerst ein Gegner, der Heldenkörper und eine Grundform jeder Waffe.
2. **Effekte** für Zauber, Runen, Reaktionen und Zustände als Niagara-Effekte über die Zeilen `effekt.` in `Data/aussehen.json`. Die Anschlüsse sind gebaut, siehe „Effekte“.
3. **Umgebung** von Hand in die Karten: Boden, Bäume, Felsen, Licht. Das geht ab jetzt, wegen des Tags `EI_Generiert` gefahrlos.

Der Stil steht in `CLAUDE.md`: leere Rüstungen, flach aus einfarbigen Flächen. Die Vorlagen liegen in `SourceAssets/Konzepte`.
