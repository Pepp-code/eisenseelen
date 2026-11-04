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

Stufe 1 ist fertig, den Lust-Check hat der Entwickler am 3. November 2026 bestanden. Stufe 2 läuft. 46 Tests sind grün, darunter fünf mit echtem Gast im Netz.

- **Bewegung:** Laufen, Sprint auf Shift, Sprung 1 m, Schwerkraft-Faktor 1,6, Luftlenkung, Sprungfelder. Das Ausweichen ist auf Wunsch des Entwicklers ganz gestrichen. Bunnyhop und Tempo-Kette sind auf Wunsch des Entwicklers gestrichen: Ein Sprung nimmt nie mehr Tempo mit als am Boden erlaubt.
- **Leben:** Wer 6 Sekunden weder getroffen wurde noch selbst getroffen hat, bekommt Leben zurück. Werte in `Data/held.json`.
- **Klingenwacht:** Leichte Hiebe immer hin und her, schwerer Hieb, der Gegner taumeln lässt (pro Gegnerart in `Data/gegner.json` einstellbar), Treff-Stopp, Klingenspur. Rechte Maus halten blockt von vorn, in den ersten 0,2 s ist es eine Parade, die den Gegner taumeln lässt. Rechtsklick bricht Hieb und Aufladen sofort ab.
- **Arkanist:** Linke Maus Schuss ohne Wartezeit, rechte Maus halten für den Zauber der Waffe, beides kostet Mana mit Anzeige. Klassenwechsel auf C.
- **Zauber auf Q:** Das Element der Waffe bestimmt ihn und beide Klassen haben ihn mit Abklingzeit: Feuerball, Eislanze (fliegt durch alle hindurch, die zweite friert Verlangsamte ein) und Kettenblitz (springt von Ziel zu Ziel und wird dabei schwächer). Werte in `Data/waffen.json`.
- **Gegner:** Knochenläufer, Knochenwerfer (wirft aus bis zu 14 m), Schildträger (der Schild hält von vorn, ein schwerer Hieb geht durch) und Kettenkoloss (groß, taumelt nicht und stürmt auf einem markierten Weg an), alle als graue Kapseln mit Wegfindung auf der Laufkarte. In der Testarena Horde auf H, Neustart auf R.
- **Lauf:** Graue Laufkarte mit 300 × 300 m. Start auf der Himmelsbarke, Sturzflug, Ritual stören (drei Altare je 30 Sekunden halten), dann ein Leuchtfeuer entzünden, 60 Sekunden halten und mit F in die Barke steigen. Alle 2 Minuten steigt die Verderbnis bis zum Blutmond, Himmel und Nebel färben sich. Die Karte hat 18 feste Lager, deren Gegner erst kommen, wenn sie dich sehen oder getroffen werden. Ab der ersten Verderbnis-Stufe kommt Nachschub aus den Lagern in deiner Nähe, leere Lager füllen sich wieder, und an Altar und Leuchtfeuer greifen Wellen an. Welche Gruppen wann kommen, steht in `Data/gruppen.json`.
- **Runen:** Items mit einem Zauber auf E: Frostrune (Frostnova), Sturmrune (Sturmfeld), Flammenrune (Flammenwand). Frostnova friert kurz ein. Reaktionen innerhalb von 2 Sekunden: Feuer und Eis ergeben die Dampfexplosion mit Wolke, Feuer und Blitz den Funkenflug, Blitz oder ein schwerer Hieb auf Gefrorene den Splitterbruch. Werte in `Data/runen.json`.
- **Truhen und Omen:** Die Karte hat 14 Truhen, jeder Lauf nutzt 5 davon. Jede steht versiegelt in einem Kreis. Wer ihn betritt, ruft 8 Wächter, mit Verderbnis und mehr Spielern auch mehr. Erst wenn alle gefallen sind, öffnet F die Truhe und gibt ein zufälliges Omen, das zur Klasse passt. Jeder Spieler öffnet für sich. Die Omen enden mit dem Lauf und stehen als Zeilen in `Data/omen.json`.
- **Items:** Sieben Plätze: Waffe, Rune, Helm, Brustpanzer, Handschuhe, Schuhe und Herzkristall. Alles steht in `Data/items.json`, die Grundformen der Waffen in `Data/waffen.json`: 15 Waffen aus 5 Grundformen und 3 Elementen, 3 Runen, 5 Rüstungsarten mit je 4 Teilen, 3 Qualitäten. Items, Rüstungs-Boni, Attribute und Omen zählen in derselben Summe. Der Spielstand liegt in `Saved/Spielstand.json`, Tests und Vorschaubilder fassen ihn nicht an. Die Testarena hat einen eigenen Teil darin, dort ist alles frei.
- **Ausrüstung:** Die Taste I öffnet das Menü auf der Barke, in der Testarena und nach dem Lauf, aber nicht während eines Laufs. Es zeigt die Plätze, was dazu passt, den Vergleich, die Summe der Werte und die Attribute.
- **Belohnungen:** Das Ritual gibt eine Waffe oder Rune, Anführer (zwei Lager je Lauf) und Schreine (zwei freie Altar-Orte je Lauf, 15 Sekunden halten) geben ein Rüstungsteil, normale Gegner selten ein Item. Jeder Spieler bekommt sein Eigenes, gewählt vom Server. Erst in der Barke ist die Beute sicher, ein gescheiterter Lauf verliert sie, die Erfahrung bleibt immer. Die Abrechnung zeigt alles am Ende. Ein zweiter Fund hebt die Qualität, danach gibt es Erfahrung.
- **Erfahrung:** Level bis 20, jedes Level gibt einen Punkt für Kraft, Geist, Zähigkeit oder Gewandtheit. Die Punkte verteilt jede Klasse für sich und jederzeit neu, wenn das Menü offen ist. Werte in `Data/items.json`, Erfahrung und Item-Chance je Gegnerart in `Data/gegner.json`.

Die Sprunghöhe von 1 m ist vom Entwickler bestätigt, das schnellere Fallen war ein Wunsch nach dem ersten Spielen. Alles andere wurde noch nicht selbst gespielt.

**Als Nächstes:** Der Spieltest des Entwicklers für alles seit dem 3. November steht aus, danach kommt Stufe 3. Die Items sind nach den Empfehlungen im Plan-Reiter „Items und Builds“ gebaut, der Entwickler kann jede davon noch ändern.

**Bewusst noch nicht dabei:** Herzkristalle (der Platz ist da, er bleibt leer), die Rute, vier weitere Rüstungsarten, der Auftrag Ruine säubern, Schlachtruf (auf Wunsch vorerst herausgenommen, die Taste E bleibt für den Runenzauber frei), Sprung-Schlag, Hieb-Vorschub, Sturmlauf, Blinzeln, Wurfäxte, Blitzrute, Totenrufer, Klassen-Passive.

**Nicht geprüft:** Töne kann der Agent nicht hören, und die Klingenspur erscheint in keinem Standbild. Der echte Mauszeiger im Menü läuft ohne Fenster nicht, die Tests klicken das Menü direkt an. Viele Items, Erfahrung und Belohnungen sind erste Annahmen und nicht abgestimmt.

**Stil (entschieden am 3. November 2026):** Leere Rüstungen, beide Klassen flach aus einfarbigen Flächen ohne Umrisslinien. Die Vorlagen des Entwicklers liegen in `SourceAssets/Konzepte/Klingenwacht` und `SourceAssets/Konzepte/Arkanist`. Sie zeigen die Richtung und sind kein Bauplan.
