# Kunst-Leitfaden

So kommen echte 3D-Modelle ins Spiel, ohne dass sich am Spielen etwas ändert. Stand: 5. November 2026.

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

Eine Waffe mit eigenem `item.`-Eintrag nutzt diesen. Ist er leer, gilt das Modell ihrer Grundform. So reicht für den Anfang ein Modell je Grundform.

Runen haben keine Zeile. Man sieht sie nur als Zauber, und die Zauber kommen später (Phase 2).

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

## Was bleibt, wie es ist

- **Treffer und Wände:** Die Kapseln und Klötze darunter entscheiden. Ein großes Modell trifft nicht weiter, und ein kleines ist nicht schwerer zu treffen.
- **Warnfarben:** Ausholen, Treffer, Verlangsamt und Taumeln liegen als farbiger Schimmer über dem Modell (Material `M_Ueberzug`). Dafür ist nichts zu tun.
- **Anzeigen:** Kreise und Bahnen am Boden, Marken, Lebensbalken und Schrift bleiben einfache Formen. Sie sollen klar zu lesen sein.

## Die Karten

`tools/rebuild_content.ps1` baut die Karten aus `Data/testarena.json` und `Data/laufkarte.json` neu. Dabei ersetzt es nur, was es selbst gebaut hat. Diese Dinge tragen das Tag `EI_Generiert`. Was du selbst in eine Karte stellst, zum Beispiel Bäume, Felsen oder Licht, bleibt beim Neubau stehen. Kopiere also nie ein erzeugtes Ding, um es zu verschieben, denn die Kopie trägt das Tag mit und verschwindet beim nächsten Neubau.

## Die Reihenfolge

1. **Modelle** über `Data/aussehen.json`. Das geht ab jetzt, Stück für Stück. Am meisten bringt zuerst ein Gegner, der Heldenkörper und eine Grundform jeder Waffe.
2. **Effekte** für Zauber, Runen, Explosionen und Treffer als Niagara-Effekte. Dafür baut der Agent später die Anschlüsse, ähnlich wie hier.
3. **Umgebung** von Hand in die Karten: Boden, Bäume, Felsen, Licht. Das geht ab jetzt, wegen des Tags `EI_Generiert` gefahrlos.

Der Stil steht in `CLAUDE.md`: leere Rüstungen, flach aus einfarbigen Flächen. Die Vorlagen liegen in `SourceAssets/Konzepte`.
