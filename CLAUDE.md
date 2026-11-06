# Eisenseelen

Hobbyprojekt: ein Fantasy-Koop-Extraction-Shooter mit beseelten Rüstungen als Helden. Eine Person entwickelt ihn mit KI-Hilfe, 5–6 Stunden pro Woche, mit wenig Vorerfahrung. Der Entwickler spielt und entscheidet, der Agent baut und prüft.

Der Masterplan ist ein Claude Doc: https://claude.ai/artifact/Syfr3Y3xcV8i1Rhfn2XXty
Der Entwickler bearbeitet ihn auch selbst. Lies ihn, bevor du etwas darin änderst.

Eine Kopie des Plans liegt in `docs/Masterplan.md` und `docs/Items-und-Builds.md`, bekannte Fallen und frühere Entscheidungen stehen in `docs/Arbeitsnotizen.md`, die Regeln für 3D-Modelle in `docs/Kunst-Leitfaden.md`, der Plan für Rüstungs-Gravuren in `docs/Ruestungs-Gravuren.md` und der Plan für UI und HUD in `docs/UI-und-HUD.md`. So kann auch ein anderes Werkzeug weiterarbeiten. Halte die Kopie und die Notizen aktuell, wenn sich Plan oder Stand ändern, und sichere am Ende eines Arbeitsblocks mit `git push origin main`.

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
- Was man sieht, hängt an `UEILookComponent`. Neue Figuren, Waffen und Orte bekommen eine Zeile in `Data/aussehen.json`, ihre Klötze bleiben für Treffer und Wände zuständig.
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

Der Aufbau der Karten steht in `Data/testarena.json` und `Data/laufkarte.json`, die Form des Geländes der Laufkarte in `Data/gelaende.json` (Draufsicht: `py -3.12 tools/gelaende.py`). Nach einer Änderung dort muss die Karte neu erzeugt werden. Der Neubau ersetzt nur Dinge mit dem Tag `EI_Generiert`, von Hand gesetzte Dinge bleiben. Alle anderen Dateien in `Data/` liest das Spiel beim Start und nach der Taste R.

Läuft noch ein Editor, kann der Linker die DLL nicht schreiben. Prüfe dann, wem der Prozess gehört, bevor du etwas beendest.

Vorschaubilder ohne sichtbares Fenster, sie landen in `Saved\Screenshots\WindowsEditor`:

```
pwsh tools/preview.ps1
pwsh tools/preview.ps1 -Szene lauf
```

Schau dir die Bilder nach jeder sichtbaren Änderung selbst an, denn Tests sehen keine Grafikfehler.

Der Entwickler startet den Editor mit `Editor_starten.bat` (Laufkarte) oder `Testarena_starten.bat`. Öffne selbst kein Editor-Fenster ohne Rückfrage, alles läuft sonst mit `-nullrhi` oder `-RenderOffscreen`.

## Aktueller Stand

Stufe 1 ist fertig, den Lust-Check hat der Entwickler am 3. November 2026 bestanden. Stufe 2 läuft. 64 Tests sind grün, darunter sechs mit echtem Gast im Netz.

- **Bewegung:** Laufen, Sprint auf Shift, Sprung 1 m, Schwerkraft-Faktor 1,6, Luftlenkung, Sprungfelder. Das Ausweichen ist auf Wunsch des Entwicklers ganz gestrichen. Bunnyhop und Tempo-Kette sind auf Wunsch des Entwicklers gestrichen: Ein Sprung nimmt nie mehr Tempo mit als am Boden erlaubt.
- **Leben:** Wer 6 Sekunden weder getroffen wurde noch selbst getroffen hat, bekommt Leben zurück. Werte in `Data/held.json`.
- **Klingenwacht:** Leichte Hiebe immer hin und her, schwerer Hieb, der Gegner taumeln lässt (pro Gegnerart in `Data/gegner.json` einstellbar), Treff-Stopp, Klingenspur. Rechte Maus halten blockt von vorn, in den ersten 0,2 s ist es eine Parade, die den Gegner taumeln lässt. Rechtsklick bricht Hieb und Aufladen sofort ab.
- **Arkanist:** Linke Maus Schuss ohne Wartezeit, rechte Maus halten für den Zauber der Waffe, beides kostet Mana mit Anzeige. Klassenwechsel auf C.
- **Zauber auf Q:** Das Element der Waffe bestimmt ihn und beide Klassen haben ihn mit Abklingzeit: Feuerball, Eislanze (fliegt durch alle hindurch, die zweite friert Verlangsamte ein) und Kettenblitz (springt von Ziel zu Ziel und wird dabei schwächer). Werte in `Data/waffen.json`.
- **Gegner:** Jeder hat eine klare Rolle. Knochenläufer, Knochenwerfer (wirft aus bis zu 14 m und weicht zurück, wenn du näher als 6 m kommst), Schildträger (der Schild hält von vorn Schüsse und leichte Hiebe, ein schwerer Hieb, Zauber, Runen und Explosionen gehen durch, nach seinem Hieb senkt er den Schild kurz), Glutschädel (dunkler Körper mit glühendem Kopf, läuft auf dich zu und explodiert im markierten Kreis, abgeschossen trifft er nur Gegner und reißt andere Glutschädel mit) und Kettenkoloss (groß, taumelt nicht und stürmt auf einem markierten Weg an, den weder Parade noch Frost aufhalten). Alle als graue Kapseln mit Wegfindung auf der Laufkarte. Im Code heißt der Zauber-Schaden `UEISpellDamage`. In der Testarena Horde auf H, Neustart auf R.
- **Lauf:** Laufkarte mit 300 × 300 m auf einer echten Landschaft: Wellen, fünf Tafelberge (drei mit Rampe), ein trockenes Flussbett und steile Hänge am Rand. Jeder Spielort liegt eben auf Höhe 0. Die Welt wächst in den Schritten W1 bis W6, siehe Masterplan unter „Die Karte“; W1 Gelände ist gebaut. Start an der Anlegestelle, einer kleinen Plattform weit weg von der Karte. Am Jagdbrett nimmt F einen anderen Jagdbrief. Jeder Brief zeigt Gebiet, Boss und Bannbruch, nie die Beute, und jeder nennt einen anderen Boss, darum hängen bei 2 Bossen nur 2 Briefe (höchstens `jagdbriefe` in `Data/bosse.json`). Mit F steigen alle in die Barke, sie legt ab, sobald alle an Bord sind, und nach einer schwarzen Überblendung steht das Team gemeinsam an einem der 4 Ankunftsorte am Kartenrand. Das ist ein Teleport und kein echtes Laden. Das Signalfeuer steht an einem von 4 festen Orten, weit weg vom Boss, und F entzündet es erst, wenn der Boss gefallen ist. Dann 60 Sekunden im Kreis halten und mit F in die Barke steigen. Es gibt keine steigende Verderbnis und keinen Nachschub mehr. Solange eine Aufgabe gehalten wird (Schrein, Altar, Ankerstein, Bosskampf, Flucht, Signalfeuer), steigt der Blutmond auf: Der Himmel wird rot, und Wellen greifen an, danach bleibt er noch 8 Sekunden. Die Karte hat 18 feste Lager, deren Gegner erst kommen, wenn sie dich sehen oder getroffen werden. Ein leeres Lager bleibt leer. Welche Gruppen kommen und welche Welle zu welcher Aufgabe gehört, steht in `Data/gruppen.json`.
- **Bosse:** Der Jagdbrief bestimmt den Boss. Er steht an einem von 4 festen Bossplätzen, ohne geschlossene Arena, und ist unverwundbar, bis sein Bannbruch erfüllt ist. Weit weg von seinem Platz kehrt er um und erholt sich. Im Kampf kommen Wellen, und nach seinem Fall jagen dich Gegner bis zur Barke. Jeder Spieler würfelt seine eigene Beute aus dem Beute-Set des Bosses: Gold und Zutaten mit Chancen. Alles in `Data/bosse.json`, die Bossplätze in `Data/laufkarte.json`.
  - **Der Gefesselte:** Bannbruch: 3 Ankersteine je 20 Sekunden halten, mit jedem fällt eine Kette. Dann kämpft er: Schlag, Ansturm auf markierter Bahn und Stampfen mit Ring am Boden.
  - **Der Seelenrufer:** Bannbruch: Wer im Kreis um seinen Kessel Gegner besiegt, füllt ihn mit Seelen (25). Ist er voll, steigt der Boss aus dem Grab daneben. Er schlägt zu, ruft alle 18 Sekunden eine kleine Gruppe und ist durch eine Schutzblase unverwundbar, solange sie steht, höchstens 20 Sekunden. Alle 14 Sekunden verschwindet er und taucht 10 bis 20 m weiter wieder auf.
  - **Die Leere Wacht:** Bannbruch: Sie schläft versteckt in einer leeren Riesenrüstung. Um sie stehen im Ring bewachte Truhen mit Seelensteinen: Wer in den Truhenkreis tritt, ruft die Wachen, erst dann lässt sich mit F ein Stein nehmen (nur einer je Truhe und Held). F an der Rüstung setzt ihn ein, mit jedem Stein leuchtet der Kern heller, beim letzten wacht der Boss auf und Rüstung und Truhen verschwinden. Stirbt ein Träger, wandert der Stein zurück in seine Truhe. Im Kampf hält sie wie ein Schildträger von vorn (schwerer Hieb und Zauber gehen durch).
- **Runen:** Items mit einem Zauber auf E: Frostrune (Frostnova), Donnerrune (Gewitterfeld), Flammenrune (Flammenwand), Windrune (Windhose, zieht Gegner zusammen). Frostnova friert kurz ein. Reaktionen innerhalb von 2 Sekunden: Feuer und Eis ergeben die Dampfexplosion mit Wolke, Feuer und Blitz den Funkenflug, Blitz oder ein schwerer Hieb auf Gefrorene den Splitterbruch, Wind mit einem anderen Element einen Feuer-, Frost- oder Gewitterwirbel. Windwaffen wirken auf Q den Windstoß, der Gegner wegschleudert. Kettenkoloss und Bosse sind windfest. Werte in `Data/runen.json` und `Data/waffen.json`.
- **Nebenaufträge:** Das Spiel würfelt je Lauf 3 Stück, jeder mit zufälliger Fraktion (Feuer, Frost oder Blitz): Anführer jagen, Schrein halten (15 Sekunden), bewachte Truhe erobern (der Kreis ruft Wächter, erst danach öffnet F die Truhe) und Altar halten (30 Sekunden). Sie sind freiwillig und stehen ab der Ankunft auf der Karte (Taste M) und als Marken in der Welt. Omen gibt es nicht mehr, im Lauf wird man nicht stärker. Werte in `Data/nebenauftraege.json`. Im Code heißen sie noch Contract.
- **Items:** Sieben Plätze: Waffe, Rune, Helm, Brustpanzer, Handschuhe, Schuhe und Herzkristall. Alles steht in `Data/items.json`, die Grundformen der Waffen in `Data/waffen.json`: 15 Waffen aus 5 Grundformen und 3 Elementen, 3 Runen, 5 Rüstungsarten mit je 4 Teilen. Jedes Teil hat einen Preis (`preis_gold`) und ein Level, ab dem es in der Schmiede steht (`ab_level`). Items und Rüstungs-Boni zählen in derselben Summe. Der Spielstand liegt in `Saved/Spielstand.json`, Tests und Vorschaubilder fassen ihn nicht an. Die Testarena hat einen eigenen Teil darin, dort ist alles frei.
- **Ausrüstung:** Die Taste I öffnet das Menü an der Anlegestelle, in der Testarena und nach dem Lauf, aber nicht während eines Laufs. Oben hat es drei Reiter. „Ausrüstung“ zeigt die Plätze, was du davon gekauft hast, den Vergleich und die Summe der Werte. „Schmiede“ zeigt je Platz alle Teile für die Klasse: gekauft, kaufbar, zu wenig Gold oder ab welchem Level. Was du kaufst, trägst du sofort. Kaufen geht nur an der Anlegestelle, dort steht auch die Schmiede selbst, F davor öffnet direkt diesen Reiter. „Beutebuch“ zeigt je Boss seinen Bannbruch und seine Zutaten, noch nicht gefundene als „???“, die Chance nur in Worten (`chance_worte` in `Data/bosse.json`). Eine Zutat gilt als gefunden, sobald sie fällt, das steht im Spielstand.
- **Belohnungen:** Ein Nebenauftrag bringt 100 Gold und 25 Punkte seiner Fraktion, und zwar jedem Spieler im Umkreis von 40 m. Erst in der Barke ist alles sicher, beim Scheitern bleiben 30 % von Gold und Punkten, die Zutaten sind verloren, die Erfahrung bleibt immer. Gold, Punkte und Zutaten liegen im Spielstand und stehen im Menü, Gold gibt man in der Schmiede aus. Normale Gegner geben nur Erfahrung und nie ein Item. Die Abrechnung zeigt alles am Ende.
- **Aussehen:** Alles ist noch grau, aber für echte 3D-Modelle vorbereitet. `Data/aussehen.json` hat eine leere Zeile für jeden Helden, jede Hand, jeden Gegner, jeden Boss, jede Waffe, jedes Rüstungsteil und jeden Ort. Ein Pfad dort legt ein Modell über die Klötze, Animationen und Warnfarben inklusive. Zauber, Reaktionen, Zustände und Elementmale zeigen leuchtende Ersatz-Effekte, die Zeilen `effekt.` tauschen sie gegen Niagara-Effekte. Die Mitspieler sehen einander als Körper aus Klötzen, man selbst sieht nur seine Hand. Wie es geht, steht in `docs/Kunst-Leitfaden.md`.
- **Erfahrung:** Level bis 20, für beide Klassen gemeinsam. Ein Level macht nicht stärker, es öffnet neue Teile in der Schmiede, und eine Meldung sagt, welche. Attribute und Qualitäten sind auf Wunsch des Entwicklers gestrichen, alte Spielstände werden beim Laden umgeschrieben. Werte in `Data/items.json`, Erfahrung je Gegnerart in `Data/gegner.json`.

Die Sprunghöhe von 1 m ist vom Entwickler bestätigt, das schnellere Fallen war ein Wunsch nach dem ersten Spielen. Alles andere wurde noch nicht selbst gespielt.

**Als Nächstes:** Der Umbau nach dem Plan-Reiter „Items und Builds“ läuft, die Schritte heißen dort A bis G. Die Schritte A (Anlegestelle, Überfahrt, Signalfeuer), B (der erste Boss) C (Seelenrufer, Jagdbriefe, Beutebuch) D (Schmiede) und E (Gravuren für Langschwert und Stab, `Data/gravuren.json`, Reiter „Gravuren“ im Menü) sind gebaut. Aus Schritt F ist Die Leere Wacht gebaut, ihre Seelenstein-Welle steht in `Data/gruppen.json` unter `seelensteine`. Die Jagdbriefe sind seit dem 6. November gebietsgebunden: Jeder Boss hat ein `gebiet` in `Data/bosse.json`, die Gebiete stehen in `Data/laufkarte.json` unter `gebiete`, und Brief, Ankunft und Signalfeuer wählen danach ihre Plätze. Rüstungs-Gravuren (Schritt G) sind in `docs/Ruestungs-Gravuren.md` geplant, UI und HUD in `docs/UI-und-HUD.md` mit den Schritten U1 bis U8. Jetzt spielt der Entwickler alles zusammen. Die zwei weiteren grauen Gebiete aus Schritt F warten auf die neue Karte.

**Bewusst noch nicht dabei:** Echte Modelle und Niagara-Effekte (nur die Anschlüsse sind gebaut), weitere Bosse und die zwei grauen Gebiete aus Schritt F, Meisterstücke, Gravuren für andere Waffen und für Rüstung (Schritt G im Plan), Herzkristalle (der Platz ist da, er bleibt leer), die Rute, vier weitere Rüstungsarten, der Auftrag Ruine säubern, Schlachtruf (auf Wunsch vorerst herausgenommen, die Taste E bleibt für den Runenzauber frei), Sprung-Schlag, Hieb-Vorschub, Sturmlauf, Blinzeln, Wurfäxte, Blitzrute, Totenrufer, Klassen-Passive.

**Nicht geprüft:** Töne kann der Agent nicht hören, und die Klingenspur erscheint in keinem Standbild. Der echte Mauszeiger im Menü läuft ohne Fenster nicht, die Tests klicken das Menü direkt an. Viele Items, Erfahrung und Belohnungen sind erste Annahmen und nicht abgestimmt. Ob der Lauf ohne Omen noch schaffbar ist und wie stark der Boss sein darf, zeigt erst das Spielen.

**Stil (entschieden am 3. November 2026):** Leere Rüstungen, beide Klassen flach aus einfarbigen Flächen ohne Umrisslinien. Die Vorlagen des Entwicklers liegen in `SourceAssets/Konzepte/Klingenwacht` und `SourceAssets/Konzepte/Arkanist`. Sie zeigen die Richtung und sind kein Bauplan.
