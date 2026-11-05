# Arbeitsnotizen

Was hier steht, steht nicht in `CLAUDE.md`, und jeder Punkt hat schon einmal Zeit gekostet. Die Datei ist für jedes Werkzeug gedacht, das an Eisenseelen weiterarbeitet. Stand: 5. November 2026.

## Wo was liegt

| Was | Wo |
| --- | --- |
| Regeln, Befehle, aktueller Stand | `CLAUDE.md` |
| Plan: Stufen, Reihenfolge, Ideen | Claude Doc, Kopie in `docs/Masterplan.md` und `docs/Items-und-Builds.md` |
| Code | `Source/Eisenseelen`, Tests in `Private/Tests` |
| Werte, die der Entwickler selbst ändert | `Data/*.json` |
| Sicherung | privates Repo `Pepp-code/eisenseelen` auf GitHub, Zweig `main` |
| Spielstand des Entwicklers | `Saved/Spielstand.json`, liegt nicht im Repo |

Assets und Bilder liegen in Git LFS (`*.uasset`, `*.umap`, `*.png` und andere). Auf einem neuen Rechner also zuerst `git lfs install`, dann klonen.

`Binaries`, `Intermediate`, `Saved` und `DerivedDataCache` sind nicht im Repo. Nach dem Klonen muss einmal kompiliert werden.

Das Claude Doc ist das Original des Plans, und der Entwickler bearbeitet es auch selbst. Lies es, bevor du etwas darin änderst. Stelle nichts wieder her, was der Entwickler gestrichen hat. Halte beide Reiter und die Kopie in `docs/` gleich. Ohne Zugriff auf das Doc: Änderungen in die Kopie schreiben und dem Entwickler sagen, dass das Original noch fehlt.

## So arbeitet der Entwickler

- Eine Person, 5 bis 6 Stunden pro Woche, wenig Vorerfahrung, testet allein. Ziel ist, dass es Spaß macht und fertig wird, nicht der Verkauf.
- Vereinbarte Punkte werden am Stück gebaut, jeder mit eigenen grünen Tests. Der Entwickler testet gebündelt am Ende. Angehalten wird nur, wo sein Urteil gebraucht wird.
- Er gibt kurze, klare Rückmeldungen zum Spielgefühl. Nimm sie wörtlich, und erkläre nicht, warum die alte Zahl gut war. Alle Zahlen sind erste Annahmen.
- Eigene Namen im Spiel. Begriffe des Vorbilds Far Far West (zum Beispiel Sturm, Kopfgeld, Boss Spawner) dürfen nur im Gespräch fallen, nie im Spiel.

## Bauen und testen

- Die Engine ist `C:\Games\UE_5.8`. Der Quellcode-Ordner unter `C:\Users\Phillip\Games` hat keinen fertigen Editor.
- Kompilieren allein:
  `C:\Games\UE_5.8\Engine\Build\BatchFiles\Build.bat EisenseelenEditor Win64 Development "-Project=C:/Users/Phillip/dev/game-project/Eisenseelen.uproject" -WaitMutex -NoHotReload`
- Einzelne Tests: `pwsh tools/check.ps1 -NoBuild -Filter Eisenseelen.Play.Run`. Alle Tests dauern etwa 7 Minuten. Die letzte Zeile heißt `GRUEN: n von n Tests` oder beginnt mit `ROT`.
- Gib die Ausgabe von `check.ps1` nie direkt in einen verketteten `git commit`. Schreib sie in eine Datei und committe nur, wenn dort `GRUEN` steht. Einmal ging so ein roter Stand durch.
- `Kein Testbericht entstanden` heißt: Ein Test ist abgestürzt. Die Ursache steht in `Saved/Logs/Eisenseelen.log`, suche nach `Critical error` und der Zeile mit `Eisenseelen.dll`.
- Nur die Laufkarte neu erzeugen (nach einer Änderung an `Data/laufkarte.json`):
  `UnrealEditor-Cmd.exe Eisenseelen.uproject -run=pythonscript -script=<voller Pfad>/tools/make_runmap.py -unattended -nullrhi -nosplash`

### Der offene Editor des Entwicklers

Prüfe vor jedem Bauen und vor jeder Änderung in `Data/`, ob ein Editor läuft. Gestartet über `Editor_starten.bat` heißt: Die Befehlszeile hat weder `-nullrhi` noch `-RenderOffscreen`. Dieser Editor gehört dem Entwickler, beende ihn nie.

- Er blockiert jedes Kompilieren (`Unable to build while Live Coding is active`). Die Prüfung gilt je Engine, also auch für eine zweite Kopie des Projekts.
- Er liest `Data/` beim Spielstart und auf R. Halb fertige Dateien zeigen ihm rote Fehler und leere Gegnergruppen. Das ist schon einmal passiert.
- Vorgehen: Arbeit beiseitelegen (`git stash push -u`), ihm Bescheid sagen und warten, bis der Editor zu ist.

## Bekannte Fallen

**Tests im Spiel**

- Echte Tasten im Test: `Player->InputKey(FInputKeyEventArgs(...))` läuft auch unter `-nullrhi` durch Enhanced Input.
- Drücke und löse einen Sprung nie im selben Frame. `StopJumping` löscht ihn, bevor die Bewegung ihn liest.
- Ein besiegter Gegner verschwindet 0,2 Sekunden später. Sichere Listen und Zeiger in Testschritten ab, sonst stürzt der ganze Lauf ab.
- Ein Test, der die Gegner einer Karte zählt, muss auch die Anführer und den Boss abschalten. Dafür gibt es Helfer in `EIRunTest.cpp`.
- Tests und Vorschaubilder dürfen `Saved/Spielstand.json` nie anfassen (`GIsAutomationTesting`, `-KeinSpielstand`).
- Ein Test mit Gast braucht `ULevelEditorPlaySettings` mit `AddToRoot`, sonst räumt die Engine sie während des Ladens weg.
- `Landed` kommt vor dem Wechsel auf Laufen, und der löscht einen dort gedrückten Sprung. Springe aus `OnMovementModeChanged`.

**Netzwerk**

- Beim Gast kommt `OnRep` vor `BeginPlay`, also muss `BeginPlay` das Aussehen noch einmal setzen.
- Werte, die beim Spawn schon stimmen müssen: `SpawnActorDeferred`, Setter, dann `FinishSpawning`.
- Der Server entscheidet alles. Das Aussehen hängt an replizierten Werten, und `OnRep` wird auf dem Server von Hand aufgerufen.

**Wegfindung**

- Ein gescheitertes `MoveToActor` hält die Figur auf der Stelle an. Prüfe zuerst, ob sie auf dem Wegenetz steht, und warte vor dem nächsten Versuch.
- Bewegliche Dinge brauchen `SetCanEverAffectNavigation(false)`, sonst wird das Wegenetz ständig neu gebaut.
- `RuntimeGeneration=Dynamic`: Die Wege entstehen beim Spielstart, auch unter `-nullrhi`, und das dauert einen Moment.

**Vorschaubilder**

- `tools/preview.ps1` nutzt `-game -RenderOffscreen` und braucht kein Fenster. Ein Durchlauf dauert etwa 5 Minuten.
- Python kann nur Funktionen mit `BlueprintPure` oder `BlueprintCallable` aufrufen.
- Der Lauf hat nicht immer Truhe, Altar und Anführer, weil die Aufträge gewürfelt werden. Darum zählen die Bilder ab Bild 16 vom Ende her.

**Karten und Skripte**

- `levels.new_level()` verweigert eine Karte, die schon bekannt ist. Zum Neubau: laden und alle Dinge darin löschen (siehe `tools/make_testarena.py`).
- `save_current_level()` scheitert still, wenn die Datei schreibgeschützt ist. Prüfe den Rückgabewert.
- `AndroidFileServer` ist abgeschaltet, weil es sonst bei jedem Lauf etwas in `DefaultEngine.ini` schreibt.
- `tools/preview_run.py` und die Karten-Skripte laufen mit Python in Unreal, sie brauchen kein eigenes Python.

**Windows und die Kommandozeile**

- Das Python auf `PATH` ist `python` (3.10). `python3` gibt es nicht.
- In Bash-Heredocs werden `\\` zu `\`, und Umlaute kommen falsch an. Schreibe Hilfsskripte als Datei und führe die Datei aus.
- Unter Git Bash wird `/Game/...` als Argument zu einem Windows-Pfad umgeschrieben. Setze `MSYS_NO_PATHCONV=1`, oder lass das Skript die Karte selbst laden.

## Was bisher entschieden wurde

Die Einzelheiten stehen im Plan. Hier steht der Weg dorthin, weil er erklärt, warum etwas so ist:

- **2. November 2026:** Das alte Spiel wurde verworfen, Eisenseelen ist neu. Der alte Ordner ist gelöscht, also nicht danach suchen.
- **3. November 2026:** Stil: leere Rüstungen, flach aus einfarbigen Flächen. Stufe 1 bestanden.
- **4. November 2026:** Neues Prinzip: Dein Build entsteht zwischen den Läufen. Omen sind weg, im Lauf wird man nicht stärker. Nebenaufträge bringen nur Gold und Fraktionspunkte, nur der Boss bringt seltene Zutaten.
- **5. November 2026:** Der Ablauf rückt näher ans Vorbild: Anlegestelle, gemeinsame Ankunft am Kartenrand, 3 zufällige Nebenaufträge, ein Bannbruch je Boss, keine geschlossene Arena, festes Signalfeuer erst nach dem Boss.

Die gebauten Item-Regeln (sieben Plätze, Qualitäten, seltene Items von Gegnern) bleiben bis zur Schmiede in Schritt D. Danach fallen Qualitäten und Item-Funde weg. Bewusst noch leer: Herzkristalle.
