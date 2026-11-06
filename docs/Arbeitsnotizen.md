# Arbeitsnotizen

Was hier steht, steht nicht in `CLAUDE.md`, und jeder Punkt hat schon einmal Zeit gekostet. Die Datei ist für jedes Werkzeug gedacht, das an Eisenseelen weiterarbeitet. Stand: 5. November 2026.

## Wo was liegt

| Was | Wo |
| --- | --- |
| Regeln, Befehle, aktueller Stand | `CLAUDE.md` |
| Plan: Stufen, Reihenfolge, Ideen | Claude Doc, Kopie in `docs/Masterplan.md` und `docs/Items-und-Builds.md` |
| Code | `Source/Eisenseelen`, Tests in `Private/Tests` |
| Werte, die der Entwickler selbst ändert | `Data/*.json` |
| Welches 3D-Modell wo liegt, Regeln dafür | `Data/aussehen.json`, `docs/Kunst-Leitfaden.md` |
| Sicherung | privates Repo `Pepp-code/eisenseelen` auf GitHub, Zweig `main` |
| Spielstand des Entwicklers | `Saved/Spielstand.json`, liegt nicht im Repo |

Assets und Bilder liegen in Git LFS (`*.uasset`, `*.umap`, `*.png` und andere). Auf einem neuen Rechner also zuerst `git lfs install`, dann klonen.

`Binaries`, `Intermediate`, `Saved` und `DerivedDataCache` sind nicht im Repo. Nach dem Klonen muss einmal kompiliert werden.

Das Claude Doc ist das Original des Plans, und der Entwickler bearbeitet es auch selbst. Lies es, bevor du etwas darin änderst. Stelle nichts wieder her, was der Entwickler gestrichen hat. Halte beide Reiter und die Kopie in `docs/` gleich. Ohne Zugriff auf das Doc: Änderungen in die Kopie schreiben und dem Entwickler sagen, dass das Original noch fehlt.

## So arbeitet der Entwickler

- Eine Person, 5 bis 6 Stunden pro Woche, wenig Vorerfahrung, testet allein. Ziel ist, dass es Spaß macht und fertig wird, nicht der Verkauf.
- Vereinbarte Punkte werden am Stück gebaut, jeder mit eigenen grünen Tests. Der Entwickler testet gebündelt am Ende. Angehalten wird nur, wo sein Urteil gebraucht wird.
- Er gibt kurze, klare Rückmeldungen zum Spielgefühl. Nimm sie wörtlich, und erkläre nicht, warum die alte Zahl gut war. Alle Zahlen sind erste Annahmen.
- Eigene Namen im Spiel. Begriffe des Vorbilds Far Far West (zum Beispiel Sturm, Kopfgeld, Boss Spawner) dürfen nur im Gespräch fallen, nie im Spiel. Deshalb heißen die Blitz-Items Donnerklinge, Donnerrune und Gewitterfeld. Sturmläufer, Sturmlauf und der Ansturm des Kolosses bleiben, weil sie nichts mit dem Wetter zu tun haben.

## Bauen und testen

- Die Engine ist `C:\Games\UE_5.8`. Der Quellcode-Ordner unter `C:\Users\Phillip\Games` hat keinen fertigen Editor.
- Kompilieren allein:
  `C:\Games\UE_5.8\Engine\Build\BatchFiles\Build.bat EisenseelenEditor Win64 Development "-Project=C:/Users/Phillip/dev/game-project/Eisenseelen.uproject" -WaitMutex -NoHotReload`
- Einzelne Tests: `pwsh tools/check.ps1 -NoBuild -Filter Eisenseelen.Play.Run`. Alle Tests dauern etwa 7 Minuten. Die letzte Zeile heißt `GRUEN: n von n Tests` oder beginnt mit `ROT`.
- Gib die Ausgabe von `check.ps1` nie direkt in einen verketteten `git commit`. Schreib sie in eine Datei und committe nur, wenn dort `GRUEN` steht. Einmal ging so ein roter Stand durch.
- `Kein Testbericht entstanden` heißt: Ein Test ist abgestürzt. Die Ursache steht in `Saved/Logs/Eisenseelen.log`, suche nach `Critical error` und der Zeile mit `Eisenseelen.dll`.
- In Test-Lambdas nie `->` auf einen `TWeakObjectPtr` ohne `IsValid`-Prüfung. Ein weggeschleuderter oder gestorbener Gegner reißt sonst den ganzen Editor mit (Zugriffsfehler bei 0x70).
- Der Test-Held ist ein Klingenwächter. Stäbe und Zepter lassen sich in Spieltests nicht anlegen, nimm Schwerter.
- Arbeitet ein zweiter Agent im Hauptordner, baue in einem eigenen Worktree (`git worktree add ..\game-project-<thema> -b <thema>`), rebase am Ende auf `origin/main` und pushe mit `git push origin <thema>:main`.
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
- Ein Test, der Schaden am Helden misst, muss die Blutmond-Wellen der Aufgabe leeren (zum Beispiel `StoneWave.Size` und `FightWave.Size`). Sonst trifft ein zufälliger Werfer, und der Test wird nur manchmal rot.
- Tests und Vorschaubilder dürfen `Saved/Spielstand.json` nie anfassen (`GIsAutomationTesting`, `-KeinSpielstand`).
- Ein Test mit Gast braucht `ULevelEditorPlaySettings` mit `AddToRoot`, sonst räumt die Engine sie während des Ladens weg.
- `Landed` kommt vor dem Wechsel auf Laufen, und der löscht einen dort gedrückten Sprung. Springe aus `OnMovementModeChanged`.
- Modelle im Test: `FEILookTable::SetTestOverride` gilt nur, solange etwas gebaut wird. Setze die Tabelle, spawne und setze sie im selben Schritt zurück. `UEILookComponent::Setup` baut nur neu, wenn sich die Schlüssel ändern, also wirkt eine Tabelle nur auf neue Dinge oder nach einem Wechsel von Klasse oder Waffe.
- Ein fehlendes Modell meldet das Spiel nur einmal je Pfad und Editor-Sitzung. Ein Test, der die Meldung erwartet, braucht deshalb jedes Mal einen neuen Pfad (`FGuid`).

**Netzwerk**

- Beim Gast kommt `OnRep` vor `BeginPlay`, also muss `BeginPlay` das Aussehen noch einmal setzen.
- Werte, die beim Spawn schon stimmen müssen: `SpawnActorDeferred`, Setter, dann `FinishSpawning`.
- Der Server entscheidet alles. Das Aussehen hängt an replizierten Werten, und `OnRep` wird auf dem Server von Hand aufgerufen.

**Wegfindung**

- Ein gescheitertes `MoveToActor` hält die Figur auf der Stelle an. Prüfe zuerst, ob sie auf dem Wegenetz steht, und warte vor dem nächsten Versuch.
- Bewegliche Dinge brauchen `SetCanEverAffectNavigation(false)`, sonst wird das Wegenetz ständig neu gebaut.
- `RuntimeGeneration=Dynamic`: Die Wege entstehen beim Spielstart, auch unter `-nullrhi`, und das dauert einen Moment.
- Ein gespeichertes `RecastNavMesh` in der Karte gewinnt beim Start: Es hatte nur die Wege um den Kartenmittelpunkt, weil der Volumen-Klotz beim Bau erst 2 m groß war. `make_runmap.py` löscht es deshalb vor dem Speichern, das Spiel baut die Wege dann selbst über die ganze Landschaft.
- Altäre und Feuer sind fest. Ein Weg dorthin endet davor, also gilt „erreichbar“ ab 4 m Abstand.

**Vorschaubilder**

- `tools/preview.ps1` nutzt `-game -RenderOffscreen` und braucht kein Fenster. Ein Durchlauf dauert etwa 5 Minuten.
- Python kann nur Funktionen mit `BlueprintPure` oder `BlueprintCallable` aufrufen.
- Der Lauf hat nicht immer Truhe, Altar und Anführer, weil die Aufträge gewürfelt werden. Darum zählen die Bilder ab Bild 16 vom Ende her.
- `-Szene seelenrufer` spielt denselben Lauf mit dem Seelenrufer (`EI_VORSCHAU_BOSS`). Ein Boss ist unverwundbar, solange er gefesselt ist oder seine Schutzblase steht. `PreviewDefeatBoss` besiegt deshalb zuerst die gerufenen Gegner (`AEIBoss::DefeatSummons`), sonst hängt der Durchlauf.
- Die Bilder 4 und 5 im Lauf sind schwarz. Das ist die Überblendung der Überfahrt und gewollt.

**Karten und Skripte**

- `levels.new_level()` verweigert eine Karte, die schon bekannt ist. Zum Neubau: laden und alles löschen, was das Tag `EI_Generiert` trägt (`clear_generated` in `tools/make_testarena.py`). Was der Entwickler selbst in die Karte stellt, bleibt so stehen. Eine alte Karte ganz ohne Tags wird einmal ganz geleert.
- `save_current_level()` scheitert still, wenn die Datei schreibgeschützt ist. Prüfe den Rückgabewert.
- Wenn ein `AStaticMeshActor` keine Kollision haben soll (zum Beispiel die Wasserfläche), reicht `set_collision_enabled(NO_COLLISION)` allein nicht: Beim Laden der Karte stellt Unreal das Profil `BlockAll` wieder her. Setze immer auch `actor.set_actor_enable_collision(False)` und `set_collision_profile_name("NoCollision")`.
- Ein Material auf einer `UInstancedStaticMeshComponent` / `UHierarchicalInstancedStaticMeshComponent` (zum Beispiel `M_Laub` und `M_Holz` für den Wald) braucht in `tools/make_assets.py` zwingend `material.set_editor_property("used_with_instanced_static_meshes", True)`, sonst zeichnet Unreal im Spiel (`-game`) nur das graue Ersatzgitter (`WorldGridMaterial`).
- `AndroidFileServer` ist abgeschaltet, weil es sonst bei jedem Lauf etwas in `DefaultEngine.ini` schreibt.
- `tools/preview_run.py` und die Karten-Skripte laufen mit Python in Unreal, sie brauchen kein eigenes Python.
- Die Landschaft baut `UEIWorldTools::BuildLandscape` in C++, denn Python kann keine Höhen importieren. In UE 5.8 sind Edit Layers immer an, die Meldung „Automatically enabling edit layers“ ist harmlos. Die Höhen rechnet `tools/gelaende.py` ohne Unreal, so zeigt die Draufsicht genau den Boden der Karte.
- Ein Test, der etwas an eine feste Stelle setzt, muss die Bodenhöhe dort nehmen (`OnGround` in `EIRunTest.cpp`). Auf Höhe 0 steckt man sonst in einem Hügel, und das Spawnen scheitert still.
- `unreal.log` aus einem Commandlet landet in `Saved/Logs/Eisenseelen.log`, nicht in der umgeleiteten Ausgabe.

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
- **5. November 2026, später:** Copilot hat übernommen, als Claude Code am Limit war. Die steigende Verderbnis ist gestrichen. Gefahr kommt nur noch durch Aufgaben: Beim Halten steigt der Blutmond mit Wellen, sonst gibt es nur die Lager, und die füllen sich nicht wieder. Gegner, Belohnungen und Items werden mit der Zeit nicht stärker. Danach bekam jeder Gegner eine klare Rolle wie beim Vorbild, und der Glutschädel kam neu dazu.

- **5. November 2026, abends:** Das Level erhöht keine Werte mehr, die Attribute sind gestrichen, denn das passte nicht zum Prinzip. Jedes Level öffnet stattdessen neue Teile in der Schmiede, gekauft wird mit Gold. Qualitäten und Item-Funde von Gegnern sind mit der Schmiede weggefallen. Bewusst noch leer: Herzkristalle.
- **5. November 2026, nachts:** Alles ist für echte 3D-Modelle vorbereitet. Die Klötze bleiben für Treffer und Wände zuständig, ein Modell aus `Data/aussehen.json` liegt nur darüber. Ein Modell ohne eigene Animationen zerfällt beim Tod wie bisher in Knochen. Runen haben keine Zeile, denn sie sieht man nur als Zauber. Effekte (Niagara) und Umgebung kommen danach.
- **5. November 2026, spät:** Was man kauft, trägt man sofort. Die Welt wächst in den Schritten W1 bis W6 (Masterplan, „Die Karte“). W1 ist gebaut: echte Landschaft aus `Data/gelaende.json`, alle Spielorte eben auf Höhe 0, damit Aufgaben, Lager und Tests unverändert gelten.
- **6. November 2026:** Die Wüstenkarte aus W1 wird durch die bewaldete Küsteninsel **Aschenwerder** (`600 × 600 m` Landschaft, `~490 m` Insel-Durchmesser, Plan in `docs/Karte-Waldinsel.md`) in den Schritten K0 bis K9 ersetzt. K0, K1, K2 (Grundform), K3 (52 Spielorte neu verteilt), K4 (Wasserspiegel `M_Wasser` auf `-0,15 m`, unsichtbare Küstensperre zum tiefen Meer, `85 %` Tempo im Flachwasser aus `welt.json`, 9 Holzbrücken/Stege mit `M_Holz`, alle Wege als Sandschicht in `M_Gelaende`), K5 (Y-förmiger Tunnel durch das Eisenjoch mit Mittelhalle, Banditenlager, 4 Stützpfeilern, Felsdach, 3 Portalen und Fackelbeleuchtung; 25 Klippen/Schären und 10 niedrige Felsmauern aus `bauteil.*` mit `M_Fels`) und K6 (Wald aus 306 Kiefern `bauteil.kiefer`, 58 toten Bäumen `bauteil.baum_tot` und 112 kollisionsfreien Büschen `bauteil.busch` mit `M_Laub`, gebündelt in 4 `AEIInstancedMeshActor`-Containern aus `UEIWorldTools::BuildInstancedMesh`) sind fertig. Außerdem: Der Bannbruch des Bosses ist erst möglich, wenn alle 3 Nebenaufträge abgeschlossen sind, und Projektile (Feuerball/Stab) treffen bei Bodentreffern zuverlässig alle Gegner im Explosionsradius.
- **6. November 2026, später:** Alle Zauber, Runen, Reaktionen, Zustände und Elementmale haben sichtbare Ersatz-Effekte, die sich über `effekt.`-Zeilen in `Data/aussehen.json` gegen Niagara-Effekte tauschen lassen. Gebaut in einem eigenen Arbeitsbaum, während Gemini die Karte neu baut.
- **6. November 2026, abends:** Schritt E: Gravuren für Langschwert und Stab aus `Data/gravuren.json`. Sie gelten je Waffe, kosten Gold und die Punkte der Fraktion des Waffen-Elements und stehen im Spielstand unter `gravuren` (Version 3, alte Dateien laden weiter). Waffen ohne Gravuren-Liste zeigen „kommen später“.
- **6. November 2026, nachts:** Schritt F, erster Teil: Die Leere Wacht, der dritte graue Boss. Bannbruch `seelensteine` in `Data/bosse.json`: Sie schläft versteckt in einer leeren Riesenrüstung (Kern leuchtet je eingesetztem Stein heller), um sie stehen bewachte Truhen mit Seelensteinen im Ring. Wer in den Kreis tritt, ruft die Wachen (Gruppe `truhe`, Welle `seelensteine` in `Data/gruppen.json`), F nimmt einen Stein, F an der Rüstung setzt ihn ein. Stirbt ein Träger, geht der Stein zurück in die Truhe. Sie hält von vorn wie ein Schildträger.
