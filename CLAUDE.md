# Eisenseelen

Hobbyprojekt: ein Fantasy-Koop-Extraction-Shooter mit beseelten Rüstungen als Helden. Eine Person entwickelt ihn mit KI-Hilfe, 5–6 Stunden pro Woche, mit wenig Vorerfahrung. Der Entwickler spielt und entscheidet, der Agent baut und prüft.

Der Masterplan ist ein Claude Doc: https://claude.ai/artifact/Syfr3Y3xcV8i1Rhfn2XXty
Der Entwickler bearbeitet ihn auch selbst. Lies ihn, bevor du etwas darin änderst.

## So arbeiten wir

- Antworte auf Deutsch und in einfachen Worten. Der Entwickler liest keinen Code.
- Code und Kommentare sind Englisch, alles für den Entwickler Sichtbare ist Deutsch.
- Jede Aufgabe endet mit einem Bericht: was sich geändert hat, Testergebnis, was der Entwickler jetzt ausprobieren soll.
- Baue nur, was die aktuelle Aufgabe verlangt. Neue Ideen kommen in die Ideenliste, nicht ins Spiel.
- Was nach drei Runden nicht läuft, wird vereinfacht oder gestrichen.

## Regeln für den Code

- Eine Aufgabe, ein Commit. Kein Commit ohne grüne Tests.
- Werte stehen nie im Code, sondern in `Data/*.json`, damit der Entwickler sie selbst ändern kann.
- Keine Blueprints mit Logik. Sie dürfen höchstens Modelle und Werte zuordnen.
- Der Server entscheidet von Anfang an: Treffer, Schaden und Spawns nur mit `HasAuthority()`.
- Eigene Namen im Spiel, nie Begriffe des Vorbilds Far Far West.
- Klassen-Präfix ist `EI`, Tests heißen `Eisenseelen.*`.

## Befehle

Kompilieren und alle Tests ohne Fenster:

```
pwsh tools/check.ps1
```

Nur die Tests:

```
pwsh tools/check.ps1 -NoBuild
```

Materialien, Töne und beide Karten neu erzeugen:

```
pwsh tools/rebuild_content.ps1
```

Der Aufbau der Karten steht in `Data/testarena.json` und `Data/laufkarte.json`. Nach einer Änderung dort muss die Karte neu erzeugt werden. Alle anderen Dateien in `Data/` liest das Spiel beim Start und nach der Taste R.

Läuft noch ein Editor, kann der Linker die DLL nicht schreiben. Prüfe dann, wem der Prozess gehört, bevor du etwas beendest.

Vorschaubilder ohne sichtbares Fenster, sie landen in `Saved\Screenshots\WindowsEditor`:

```
pwsh tools/preview.ps1
pwsh tools/preview.ps1 -Szene lauf
```

Schau dir die Bilder nach jeder sichtbaren Änderung selbst an, denn Tests sehen keine Grafikfehler.

Der Entwickler startet den Editor mit `Editor_starten.bat` (Laufkarte) oder `Testarena_starten.bat`. Öffne selbst kein Editor-Fenster ohne Rückfrage, alles läuft sonst mit `-nullrhi` oder `-RenderOffscreen`.

## Aktueller Stand

Stufe 1 ist fertig, den Lust-Check hat der Entwickler am 3. November 2026 bestanden. Stufe 2 läuft. 31 Tests sind grün, darunter vier mit echtem Gast im Netz.

- **Bewegung:** Laufen, Sprint auf Shift, Sprung 1 m, Schwerkraft-Faktor 1,6, Ausweichen auf V, Luftlenkung, Sprungfelder. Bunnyhop und Tempo-Kette sind auf Wunsch des Entwicklers gestrichen: Ein Sprung nimmt nie mehr Tempo mit als am Boden erlaubt.
- **Klingenwacht:** Leichte Hiebe immer hin und her, schwerer Hieb, der Gegner taumeln lässt (pro Gegnerart in `Data/gegner.json` einstellbar), Treff-Stopp, Klingenspur. Rechte Maus halten blockt von vorn, in den ersten 0,2 s ist es eine Parade, die den Gegner taumeln lässt. Rechtsklick bricht Hieb und Aufladen sofort ab.
- **Arkanist:** Linke Maus Feuerstab ohne Wartezeit, rechte Maus halten für den Feuerball, beides kostet Mana mit Anzeige. Feuerball auch auf Q mit Abklingzeit. Klassenwechsel auf C.
- **Gegner:** Knochenläufer als graue Kapseln mit Wegfindung auf der Laufkarte, in der Testarena Horde auf H, Neustart auf R.
- **Lauf:** Graue Laufkarte mit 300 × 300 m. Start auf der Himmelsbarke, Sturzflug, Ritual stören (drei Altare je 30 Sekunden halten), dann ein Leuchtfeuer entzünden, 60 Sekunden halten und mit F in die Barke steigen. Alle 2 Minuten steigt die Verderbnis bis zum Blutmond, Himmel und Nebel färben sich. Der Direktor bringt Gegner außerhalb der Sicht, je nach Phase und Verderbnis.
- **Truhen und Omen:** Die Karte hat 14 Truhen, jeder Lauf nutzt 5 davon. Jede steht versiegelt in einem Kreis. Wer ihn betritt, ruft 8 Wächter, mit Verderbnis und mehr Spielern auch mehr. Erst wenn alle gefallen sind, öffnet F die Truhe und gibt ein zufälliges Omen, das zur Klasse passt. Jeder Spieler öffnet für sich. Die Omen enden mit dem Lauf und stehen als Zeilen in `Data/omen.json`.

Die Sprunghöhe von 1 m ist vom Entwickler bestätigt, das schnellere Fallen war ein Wunsch nach dem ersten Spielen. Alles andere wurde noch nicht selbst gespielt.

**Als Nächstes in Stufe 2, in dieser Reihenfolge:** Knochenwerfer und Schildträger, Frostrune und Dampfexplosion. Der Vorschlag für dauerhafte Items und Builds steht im Plan im Reiter „Items und Builds“. Der Entwickler entscheidet noch darüber, und danach kann sich die Reihenfolge ändern.

**Bewusst noch nicht dabei:** Waffenzauber und Runen (Plan, Abschnitt zu den Zaubern), Schlachtruf (auf Wunsch vorerst herausgenommen, die Taste E bleibt für den Runenzauber frei), Sprung-Schlag, Hieb-Vorschub, Sturmlauf, Blinzeln, Wurfäxte, Blitzrute, Wegfindung der Gegner, Klassen-Passive.

**Nicht geprüft:** Töne kann der Agent nicht hören, und die Klingenspur erscheint in keinem Standbild.

**Stil (entschieden am 3. November 2026):** Leere Rüstungen, beide Klassen flach aus einfarbigen Flächen ohne Umrisslinien. Die Vorlagen des Entwicklers liegen in `SourceAssets/Konzepte/Klingenwacht` und `SourceAssets/Konzepte/Arkanist`. Sie zeigen die Richtung und sind kein Bauplan.
