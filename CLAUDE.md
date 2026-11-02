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

Testkarte neu erzeugen:

```
C:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe Eisenseelen.uproject -run=pythonscript -script=C:/Users/Phillip/dev/game-project/tools/make_testarena.py -unattended -nullrhi
```

Der Aufbau der Testkarte steht in `Data/testarena.json`. Nach einer Änderung dort muss die Karte neu erzeugt werden.

Vorschaubilder ohne sichtbares Fenster, sie landen in `Saved\Screenshots\WindowsEditor`:

```
pwsh tools/preview.ps1
```

Schau dir die Bilder nach jeder sichtbaren Änderung selbst an, denn Tests sehen keine Grafikfehler.

Der Entwickler startet den Editor mit `Editor_starten.bat`. Öffne selbst kein Editor-Fenster ohne Rückfrage, alles läuft sonst mit `-nullrhi` oder `-RenderOffscreen`.

## Aktueller Stand

Stufe 0 ist technisch fertig: Laufen und Springen in einer Testkarte mit Rampen und Sprungklötzen, Werte aus `Data/held.json`, vier grüne Tests. Der Entwickler hat die Sprunghöhe auf 1,0 m gesetzt.

Die Sicherung liegt im privaten Repository https://github.com/Pepp-code/eisenseelen. Pushe am Ende jeder Sitzung dorthin.

Offen in Stufe 0: Der Entwickler probiert es einmal aus.

Als Nächstes laut Plan, Abschnitt 17: Dash, Tempo-Kette und Sprungfelder (Woche 3–4).

## Das alte Projekt

Das verworfene Spiel Hexcaliber liegt vollständig unter `C:\Users\Phillip\dev\game-project-ALT-hexcaliber`. Es gehört nicht zu diesem Projekt. Übernimm daraus nichts ohne ausdrücklichen Auftrag.
