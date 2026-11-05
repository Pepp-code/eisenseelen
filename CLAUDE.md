# Eisenseelen

Hobbyprojekt: ein Fantasy-Koop-Extraction-Shooter mit beseelten Rüstungen als Helden. Eine Person entwickelt ihn mit KI-Hilfe, 5–6 Stunden pro Woche, mit wenig Vorerfahrung. Der Entwickler spielt und entscheidet, der Agent baut und prüft.

Der Masterplan ist ein Claude Doc: https://claude.ai/artifact/Syfr3Y3xcV8i1Rhfn2XXty
Der Entwickler bearbeitet ihn auch selbst. Lies ihn, bevor du etwas darin änderst.

Eine Kopie des Plans liegt in `docs/Masterplan.md` und `docs/Items-und-Builds.md`, bekannte Fallen und frühere Entscheidungen stehen in `docs/Arbeitsnotizen.md`. So kann auch ein anderes Werkzeug weiterarbeiten. Halte die Kopie und die Notizen aktuell, wenn sich Plan oder Stand ändern, und sichere am Ende eines Arbeitsblocks mit `git push origin main`.

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

Stufe 1 ist fertig, den Lust-Check hat der Entwickler am 3. November 2026 bestanden. Stufe 2 läuft. 49 Tests sind grün, darunter sechs mit echtem Gast im Netz.

- **Bewegung:** Laufen, Sprint auf Shift, Sprung 1 m, Schwerkraft-Faktor 1,6, Luftlenkung, Sprungfelder. Das Ausweichen ist auf Wunsch des Entwicklers ganz gestrichen. Bunnyhop und Tempo-Kette sind auf Wunsch des Entwicklers gestrichen: Ein Sprung nimmt nie mehr Tempo mit als am Boden erlaubt.
- **Leben:** Wer 6 Sekunden weder getroffen wurde noch selbst getroffen hat, bekommt Leben zurück. Werte in `Data/held.json`.
- **Klingenwacht:** Leichte Hiebe immer hin und her, schwerer Hieb, der Gegner taumeln lässt (pro Gegnerart in `Data/gegner.json` einstellbar), Treff-Stopp, Klingenspur. Rechte Maus halten blockt von vorn, in den ersten 0,2 s ist es eine Parade, die den Gegner taumeln lässt. Rechtsklick bricht Hieb und Aufladen sofort ab.
- **Arkanist:** Linke Maus Schuss ohne Wartezeit, rechte Maus halten für den Zauber der Waffe, beides kostet Mana mit Anzeige. Klassenwechsel auf C.
- **Zauber auf Q:** Das Element der Waffe bestimmt ihn und beide Klassen haben ihn mit Abklingzeit: Feuerball, Eislanze (fliegt durch alle hindurch, die zweite friert Verlangsamte ein) und Kettenblitz (springt von Ziel zu Ziel und wird dabei schwächer). Werte in `Data/waffen.json`.
- **Gegner:** Knochenläufer, Knochenwerfer (wirft aus bis zu 14 m), Schildträger (der Schild hält von vorn, ein schwerer Hieb geht durch) und Kettenkoloss (groß, taumelt nicht und stürmt auf einem markierten Weg an), alle als graue Kapseln mit Wegfindung auf der Laufkarte. In der Testarena Horde auf H, Neustart auf R.
- **Lauf:** Graue Laufkarte mit 300 × 300 m. Start an der Anlegestelle, einer kleinen Plattform weit weg von der Karte. Mit F steigen alle in die Barke, sie legt ab, sobald alle an Bord sind, und nach einer schwarzen Überblendung steht das Team gemeinsam an einem der 4 Ankunftsorte am Kartenrand. Das ist ein Teleport und kein echtes Laden. Das Signalfeuer steht an einem von 4 festen Orten, weit weg vom Boss, und F entzündet es erst, wenn der Boss gefallen ist. Dann 60 Sekunden im Kreis halten und mit F in die Barke steigen. Alle 2 Minuten steigt die Verderbnis bis zum Blutmond, Himmel und Nebel färben sich. Die Karte hat 18 feste Lager, deren Gegner erst kommen, wenn sie dich sehen oder getroffen werden. Ab der ersten Verderbnis-Stufe kommt Nachschub aus den Lagern in deiner Nähe, leere Lager füllen sich wieder, und an Altar, Schrein und Signalfeuer greifen Wellen an. Welche Gruppen wann kommen, steht in `Data/gruppen.json`.
- **Boss:** Der Gefesselte steht gefesselt an einem von 4 festen Bossplätzen, ohne geschlossene Arena. Sein Bannbruch: 3 Ankersteine je 20 Sekunden halten, mit jedem fällt eine Kette. Dann kämpft er: Schlag, Ansturm auf markierter Bahn und Stampfen mit Ring am Boden. Weit weg von seinem Platz kehrt er um und erholt sich. Im Kampf kommen Wellen, und nach seinem Fall jagen dich Gegner bis zur Barke. Jeder Spieler würfelt seine eigene Beute aus dem Beute-Set: Gold und Zutaten mit Chancen. Alles in `Data/bosse.json`, die Bossplätze in `Data/laufkarte.json`.
- **Runen:** Items mit einem Zauber auf E: Frostrune (Frostnova), Sturmrune (Sturmfeld), Flammenrune (Flammenwand). Frostnova friert kurz ein. Reaktionen innerhalb von 2 Sekunden: Feuer und Eis ergeben die Dampfexplosion mit Wolke, Feuer und Blitz den Funkenflug, Blitz oder ein schwerer Hieb auf Gefrorene den Splitterbruch. Werte in `Data/runen.json`.
- **Nebenaufträge:** Das Spiel würfelt je Lauf 3 Stück, jeder mit zufälliger Fraktion (Feuer, Frost oder Blitz): Anführer jagen, Schrein halten (15 Sekunden), bewachte Truhe erobern (der Kreis ruft Wächter, erst danach öffnet F die Truhe) und Altar halten (30 Sekunden). Sie sind freiwillig und stehen ab der Ankunft auf der Karte (Taste M) und als Marken in der Welt. Omen gibt es nicht mehr, im Lauf wird man nicht stärker. Werte in `Data/nebenauftraege.json`. Im Code heißen sie noch Contract.
- **Items:** Sieben Plätze: Waffe, Rune, Helm, Brustpanzer, Handschuhe, Schuhe und Herzkristall. Alles steht in `Data/items.json`, die Grundformen der Waffen in `Data/waffen.json`: 15 Waffen aus 5 Grundformen und 3 Elementen, 3 Runen, 5 Rüstungsarten mit je 4 Teilen, 3 Qualitäten. Items, Rüstungs-Boni und Attribute zählen in derselben Summe. Der Spielstand liegt in `Saved/Spielstand.json`, Tests und Vorschaubilder fassen ihn nicht an. Die Testarena hat einen eigenen Teil darin, dort ist alles frei.
- **Ausrüstung:** Die Taste I öffnet das Menü an der Anlegestelle, in der Testarena und nach dem Lauf, aber nicht während eines Laufs. Es zeigt die Plätze, was dazu passt, den Vergleich, die Summe der Werte und die Attribute.
- **Belohnungen:** Ein Nebenauftrag bringt 100 Gold und 25 Punkte seiner Fraktion, je Verderbnis-Stufe 10 % mehr, und zwar jedem Spieler im Umkreis von 40 m. Erst in der Barke ist alles sicher, beim Scheitern bleiben 30 % von Gold und Punkten, die Zutaten sind verloren, die Erfahrung bleibt immer. Gold, Punkte und Zutaten liegen im Spielstand und stehen im Menü, ausgeben kann man sie erst mit Schritt D. Normale Gegner geben Erfahrung und bis Schritt D noch selten ein Item, das wie bisher erst in der Barke sicher ist. Die Abrechnung zeigt alles am Ende.
- **Erfahrung:** Level bis 20, jedes Level gibt einen Punkt für Kraft, Geist, Zähigkeit oder Gewandtheit. Die Punkte verteilt jede Klasse für sich und jederzeit neu, wenn das Menü offen ist. Werte in `Data/items.json`, Erfahrung und Item-Chance je Gegnerart in `Data/gegner.json`.

Die Sprunghöhe von 1 m ist vom Entwickler bestätigt, das schnellere Fallen war ein Wunsch nach dem ersten Spielen. Alles andere wurde noch nicht selbst gespielt.

**Als Nächstes:** Der Umbau nach dem Plan-Reiter „Items und Builds“ läuft, die Schritte heißen dort A bis G. Die Schritte A (Anlegestelle, Überfahrt, Signalfeuer) und B (der erste Boss) sind gebaut. Jetzt spielt der Entwickler den ganzen Lauf. Danach kommt Schritt C: der Seelenrufer, die Jagdbriefe und das Beutebuch.

**Bewusst noch nicht dabei:** Jagdbriefe, Beutebuch, weitere Bosse, Schmiede, Gravuren und Meisterstücke (Schritte C bis G im Plan), das vierte Element Wind mit Windwaffen (Schritt G), Herzkristalle (der Platz ist da, er bleibt leer), die Rute, vier weitere Rüstungsarten, der Auftrag Ruine säubern, Schlachtruf (auf Wunsch vorerst herausgenommen, die Taste E bleibt für den Runenzauber frei), Sprung-Schlag, Hieb-Vorschub, Sturmlauf, Blinzeln, Wurfäxte, Blitzrute, Totenrufer, Klassen-Passive.

**Nicht geprüft:** Töne kann der Agent nicht hören, und die Klingenspur erscheint in keinem Standbild. Der echte Mauszeiger im Menü läuft ohne Fenster nicht, die Tests klicken das Menü direkt an. Viele Items, Erfahrung und Belohnungen sind erste Annahmen und nicht abgestimmt. Ob der Lauf ohne Omen noch schaffbar ist und wie stark der Boss sein darf, zeigt erst das Spielen.

**Stil (entschieden am 3. November 2026):** Leere Rüstungen, beide Klassen flach aus einfarbigen Flächen ohne Umrisslinien. Die Vorlagen des Entwicklers liegen in `SourceAssets/Konzepte/Klingenwacht` und `SourceAssets/Konzepte/Arkanist`. Sie zeigen die Richtung und sind kein Bauplan.
