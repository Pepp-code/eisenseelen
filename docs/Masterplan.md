> Kopie aus dem Masterplan-Dokument (Claude Doc: https://claude.ai/artifact/Syfr3Y3xcV8i1Rhfn2XXty), Stand 5. November 2026.
> Das Original ist das Doc, der Entwickler bearbeitet es auch selbst. Diese Datei ist für Werkzeuge ohne Zugriff darauf.

# Masterplan: Fantasy-Koop-Extraction-Shooter nach dem Vorbild von Far Far West

Nov 2, 2026 · @Phillip

Dieser Plan gilt für ein Hobbyprojekt: eine Person mit KI-Hilfe, 5–6 Stunden pro Woche, wenig Vorerfahrung, ohne Förderung und ohne Testgruppe. Er ersetzt die früheren Fassungen für ein Team und für Vollzeit.

## 1. Kurzfassung

Mit 5–6 Stunden pro Woche ist dieses Spiel als Hobby machbar, wenn der KI-Agent fast alles baut und du entscheidest, spielst und urteilst. Nach etwa 3 Monaten hast du ein spielbares Kampfgefühl, nach etwa 8 Monaten einen vollständigen Lauf zu zweit und nach etwa 2 Jahren ein kleines fertiges Spiel. Das kostet rund 20–100 US-Dollar im Monat für den Agenten und sonst fast nichts.

Der Arbeitstitel bleibt **Eisenseelen**: beseelte Rüstungen, die im Auftrag einer Gilde in verfluchte Länder fliegen, einen Auftrag erledigen und mit der Beute entkommen. Die Rüstungen bestehen aus starren Teilen. Das erspart dir den teuersten und schwierigsten Teil der Figurenarbeit, und du musst dafür nicht modellieren können.

### Die Rechnung, die alles verändert

Du hast etwa 250 Stunden pro Jahr. Der Vollzeit-Plan brauchte rund 4.500 Stunden, das wären 18 Jahre. Deshalb ändert sich nicht nur der Umfang, sondern die ganze Art zu planen.

| Thema | Vollzeit-Fassung | Hobby-Fassung |
| --- | --- | --- |
| Planung | Termine und Prüfpunkte | Stufen, jede endet mit etwas Spielbarem |
| Erfolgsmaß | Verkäufe und Wunschlisten | Es macht dir Spaß, und du wirst fertig |
| Deine Rolle | Leitung, Kunst, Marketing | Richtung vorgeben, spielen, urteilen |
| Inhalt am Ende | 2 Klassen, 2 Karten, 6 Gegner, 1 Boss | 2 Klassen, 1 Karte, 5 Gegner, 1 Boss |
| Kunst | Bezahlte Künstler, keine generative KI | Skripte, freie Pakete, generative KI erlaubt |
| Testen | Feste Gruppe | Du, automatische Tests, zwei Fenster |
| Geld | 115.000 € | 500–2.500 € über 2 Jahre |
| Veröffentlichung | Pflicht | Freiwillig, erst am Ende entscheiden |

### Die Entscheidungen auf einen Blick

| Thema | Entscheidung | Hauptgrund |
| --- | --- | --- |
| Engine | Unreal Engine 5.8 | Netzwerk und Ego-Bewegung sind fertig, das spart dir das Schwierigste |
| Rollen | Die KI baut und prüft, du spielst und entscheidest | Nur so reichen 5–6 Stunden |
| Arbeitsweise | Alles als Text oder Skript, jede Änderung mit automatischem Test | Du kannst den Code nicht prüfen, die Tests schon |
| Netzwerk | Ein Spieler ist Host, keine Server | Keine Kosten, kein Betrieb. Koop bleibt möglich |
| Klassen | Schwertkämpfer und Magier | Die beiden, um die es dir geht |
| Karte | 300 × 300 m, zufällige Orte | Wenig Kunst, kurze Wege, kein Reittier nötig |
| Spielerzahl | Solo muss zuerst gut sein, Koop bis 4 kommt dazu | Du testest allein |
| Kunst | Starre Teile aus Blender-Skripten, flache Farben | Geht ohne Modellierkönnen |

### Die drei größten Risiken

1. **Die Lust geht verloren.** Das ist bei Hobbyprojekten der häufigste Grund zu scheitern. Darum endet jede Woche mit etwas zum Ausprobieren.
2. **Der Umfang wächst, weil die KI alles leicht macht.** Jede neue Idee kostet dich danach Test- und Pflegezeit, und die ist knapp.
3. **Du kannst den Code nicht beurteilen.** Wenn etwas tief bricht, hängst du vom Agenten ab. Tests, kleine Schritte und eine einfache Architektur senken das Risiko, beseitigen es aber nicht.

Eine Einschränkung musst du dazu wissen: Ob sich Koop wirklich gut anfühlt, zeigt sich erst, wenn jemand anderes mitspielt. Automatische Tests prüfen nur, ob es funktioniert. Ein einziger Abend mit einem Freund bei Stufe 3 wäre viel wert, ist aber keine Bedingung.

### So sind die Zahlen zu lesen

Zahlen mit Quellenlink sind recherchiert (Stand 2. November 2026). Alles für dein Spiel sind meine Setzungen. Die Zeiten sind grobe Schätzungen und können leicht um die Hälfte daneben liegen. Das ist bei einem Hobby kein Problem, weil kein Termin daran hängt.

## 2. Das Vorbild: Was Far Far West ist und warum es funktioniert

Far Far West beweist, dass 8 Leute mit einem klaren Aufhänger und einem sauberen Koop-Kreislauf über eine Million Kopien verkaufen können. Der Erfolg kommt nicht aus der Menge der Inhalte, sondern aus Spielgefühl, Tempo und Stimmung. Gerade die Inhaltsmenge wird dem Spiel am häufigsten vorgeworfen.

Für dich heißt das: Acht erfahrene Personen zeigen, wie es sich anfühlen kann, aber nicht, wie viel du bauen kannst. Du übernimmst die Formel aus Tempo, Trefferwirkung und Gier, nicht den Umfang.

### Belegte Fakten

| Merkmal | Wert | Quelle |
| --- | --- | --- |
| Entwickler | Evil Raptor, Lyon, Frankreich; 8 Personen | [Evil Raptor](https://www.evilraptor.com/), [Steam](https://store.steampowered.com/app/3124540) |
| Frühere Spiele | Pumpkin Jack, Akimbot (3D-Action und Plattformer, keine Shooter) | [ScreenRant](https://screenrant.com/far-far-west-developer-interview/) |
| Publisher | Fireshine Games (Teil der EG7-Gruppe) | [EG7](https://www.enadglobal7.com/press-releases/far-far-west-delivers-strong-initial-sales-on-steam-with-over-500000-units-sold-the-first-days-after-launch/) |
| Early Access | Seit 28. April 2026, geplant etwa 12 Monate, Preiserhöhung zur 1.0 angekündigt | [Steam](https://store.steampowered.com/app/3124540) |
| Preis | 19,99 €, keine Mikrotransaktionen geplant | [Steam](https://store.steampowered.com/app/3124540), [ScreenRant](https://screenrant.com/far-far-west-developer-interview/) |
| Wunschlisten zum Start | über 700.000 | [EG7](https://www.enadglobal7.com/press-releases/far-far-west-delivers-strong-initial-sales-on-steam-with-over-500000-units-sold-the-first-days-after-launch/) |
| Verkäufe | über 100.000 in 24 Stunden, über 500.000 am ersten Wochenende, 700.000 in einer Woche, über 1 Mio. bis Mitte Mai 2026 | [EG7](https://www.enadglobal7.com/press-releases/far-far-west-delivers-strong-initial-sales-on-steam-with-over-500000-units-sold-the-first-days-after-launch/), [MMORPG.com](https://www.mmorpg.com/news/futuristic-wild-west-co-op-shooter-far-far-west-gets-first-major-update-and-hits-1-million-in-sales-2000138103) |
| Gleichzeitige Spieler | über 47.000 in den ersten Tagen | [EG7](https://www.enadglobal7.com/press-releases/far-far-west-delivers-strong-initial-sales-on-steam-with-over-500000-units-sold-the-first-days-after-launch/) |
| Bewertungen | 96 % positiv bei 21.257 englischen Rezensionen, zuletzt 93 % | [Steam](https://store.steampowered.com/app/3124540) (Stand 2. Nov. 2026) |
| Mindestanforderung | i5-7600K oder Ryzen 5 1600, 8 GB RAM, GTX 1660 oder RX 590, 10 GB Speicher | [Steam](https://store.steampowered.com/app/3124540) |
| Sprachen | 15, dazu vollständig Englisch; ob die übrigen vertont sind, ist unklar | [Steam](https://store.steampowered.com/app/3124540) |

Offizielle Angaben zur Engine habe ich nicht gefunden. Fan-Wikis nennen [Unreal Engine 5](https://wikily.gg/far-far-west), das ist aber unbestätigt. Auch Entwicklungszeit und Finanzierung sind nicht öffentlich.

### So läuft ein Einsatz im Vorbild

1. **Auftrag:** Im Hub nimmt die Gruppe einen Auftrag beim [Sheriff](https://store.steampowered.com/app/3124540) an.
2. **Absprung:** Ein Schwebezug setzt die Gruppe auf einer [weiten, offenen Karte](https://gamingtrend.com/previews/far-far-west-preview/) ab. Es gibt [kein Zeitlimit](https://game8.co/articles/reviews/far-far-west-review-early-access).
3. **Hauptziel und Boss:** Die Gruppe erfüllt eine Aufgabe, die den Boss herbeiruft, und besiegt ihn.
4. **Flucht:** Sie [läutet die Glocke am Bahnhof](https://gamingtrend.com/previews/far-far-west-preview/), hält eine letzte Welle auf und steigt ein.
5. **Hub:** Mit [Gold und Seelen](https://game8.co/articles/reviews/far-far-west-review-early-access) werden Held, Waffen und dauerhafte Perks verbessert.

Länger bleiben bringt [mehr Beute, aber auch mehr Gefahr](https://game8.co/articles/reviews/far-far-west-review-early-access). Ein Rezensent nennt [über 40 Minuten](https://indiecator.org/2026/06/03/far-far-west-review/) als Laufzeit, die man mit zügigem Spiel vermeidet. Eine typische Länge ist nicht belegt.

### Die Systeme im Einzelnen

- **Ausrüstung:** [Hauptwaffe, Zweitwaffe, bis zu drei Zauber, Pferd und Hilfsmittel](https://game8.co/articles/reviews/far-far-west-review-early-access). Laut einer Vorschau gibt es [mehr als zehn Waffen](https://gamingtrend.com/previews/far-far-west-preview/).
- **Zauber:** [Feuer, Säure, Elektrizität, Voodoo und Kaktus](https://indiecator.org/2026/06/03/far-far-west-review/). Sie werden [durch Benutzung freigeschaltet](https://gamingtrend.com/previews/far-far-west-preview/), nicht gekauft.
- **Joker:** Karten mit Boni. [Temporäre](https://game8.co/articles/reviews/far-far-west-review-early-access) gibt es im Einsatz, sie enden mit ihm. Dauerhafte werden im Hub gekauft.
- **Bewegung:** [Kein Sprint, nur ein kurzer Dash](https://game8.co/articles/reviews/far-far-west-review-early-access). Guides beschreiben [Dash und Sprung als Kombination bis hin zum Bunnyhop](https://ninewiki.com/guide/far-far-west-guide-combos-secrets-and-quests/). Ein Fan-Wiki nennt [550 cm/s als Grundtempo](https://farfarwest.wiki.gg/wiki/Speedster).
- **Reittier:** Gehört zur Grundausrüstung. Laut Guide sind [Reittiere rein kosmetisch verschieden](https://dotesports.com/far-far-west/news/far-far-west-mounts).
- **Gegner:** [Skelette, Marodeure, Geistergeier](https://game8.co/articles/reviews/far-far-west-review-early-access). Der Entwickler plant [eine Alien-Armee](https://screenrant.com/far-far-west-developer-interview/).
- **Bosse:** Unter anderem [ein besessener Saloon, ein Geier und ein Geisterzug](https://game8.co/articles/reviews/far-far-west-review-early-access).
- **Hub:** [Lebendiger Ort](https://screenrant.com/far-far-west-developer-interview/) mit Händlern und [Schießstand](https://gamingtrend.com/previews/far-far-west-preview/).

### Warum es funktioniert

Die Punkte unter dieser Überschrift sind meine Deutung der Quellen.

- **Ein Satz genügt.** Roboter-Cowboys, die auch zaubern können, lassen sich in einem Screenshot erklären. Der Studioleiter begründet die Magie schlicht damit, dass sie [immer cool sei](https://www.pcgamer.com/games/fps/developers-of-steam-next-fests-wildly-popular-robot-wizard-cowboy-shooter-say-its-cowboy-robots-are-also-wizards-because-magic-is-always-cool/).
- **Spielgefühl vor Inhalt.** Rezensenten loben [Gunplay und Politur](https://game8.co/articles/reviews/far-far-west-review-early-access) für den frühen Stand. Bewegung und Trefferrückmeldung tragen viele Stunden, auch wenn sich Inhalte wiederholen.
- **Extraktion ohne Verlustangst.** Das Spiel ist kein hartes Extraction-Spiel. Der Zug entstand laut Entwickler [erst spät als Rückweg in die Lobby](https://screenrant.com/far-far-west-developer-interview/), und daraus wurde das Genre. Das Prinzip bringt Spannung, ohne Gelegenheitsspieler abzuschrecken.
- **Bunte Abgrenzung.** Der Studioleiter wollte [etwas Farbintensives, das den Spielen Leben zurückgibt](https://screenrant.com/far-far-west-developer-interview/). Das hebt sich von graubraunen Shootern ab und wirkt in Clips sofort.
- **Steam Next Fest als Zündung.** Das Spiel fiel [im Februar 2026 beim Next Fest](https://www.enadglobal7.com/press-releases/far-far-west-delivers-strong-initial-sales-on-steam-with-over-500000-units-sold-the-first-days-after-launch/) auf und startete zehn Wochen später. Dieses Muster übernehme ich in den Plan.
- **Albernheit als Marketing.** Spieler fordern laut PC Gamer vor allem [einen Yeehaw-Knopf](https://www.pcgamer.com/games/fps/far-far-west-is-a-surprise-hit-on-steam-but-its-players-have-one-demand-a-yeehaw-button/). Kleine soziale Spielereien kosten wenig und tragen weit.

### Was am Vorbild kritisiert wird und was daraus folgt

| Kritik | Quelle | Was das für dein Hobbyprojekt heißt |
| --- | --- | --- |
| Sehr wenige Gegnertypen, Bosse wiederholen sich | [GamingTrend](https://gamingtrend.com/previews/far-far-west-preview/), [ComicBuzz](https://comicbuzz.com/far-far-west-review/) | Mit 5 Gegnern geht es dir genauso. Für ein Hobby ist das in Ordnung, Gegner-Gruppen holen mehr heraus |
| Solo sehr schwer, keine Anpassung an die Spielerzahl | [Game8](https://game8.co/articles/reviews/far-far-west-review-early-access) | Bei dir der wichtigste Punkt: Du testest allein, also muss Solo von Anfang an gut sein |
| Fortschritt zäh: 50–100 Gold pro Lauf bei Preisen in die Tausende | [GamingTrend](https://gamingtrend.com/previews/far-far-west-preview/) | Preise stehen in einer Textdatei, du stellst sie nach deinem Gefühl ein |
| Grind so groß, dass ein Patch ihn [halbierte](https://www.mmorpg.com/news/futuristic-wild-west-co-op-shooter-far-far-west-gets-first-major-update-and-hits-1-million-in-sales-2000138103) | MMORPG.com | Eine Änderung dauert bei dir nur Minuten |
| Beute wurde unter Mitspielern weggeschnappt, bis Joker [instanziert wurden](https://www.mmorpg.com/news/futuristic-wild-west-co-op-shooter-far-far-west-gets-first-major-update-and-hits-1-million-in-sales-2000138103) | MMORPG.com | Beute von Anfang an je Spieler getrennt, kostet nichts extra |
| Fortschrittsverlust und [beschädigte Speicherstände](https://www.gamewatcher.com/news/far-far-west-patch-notes-hub-and-roadmap-of-updates-for-2026) | [ScreenRant](https://screenrant.com/far-far-west-developer-interview/) | Einfache sichere Speicherung mit Test, gleich beim ersten Speicherstand |
| Leere Kartenbereiche, zu viel Laufen | [Indiecator](https://indiecator.org/2026/06/03/far-far-west-review/) | Kleine Karte, alle 20 Sekunden etwas Lohnendes |
| Pferd bleibt an Bäumen und Kakteen hängen | [Gamereactor](https://www.gamereactor.eu/far-far-west-1714343/) | Kein Reittier |
| Sperrige Nebenquests | [Game8](https://game8.co/articles/reviews/far-far-west-review-early-access) | Keine langen Nebenquests, nur kleine Nebenaufträge vor Ort |

### Vergleichbare Spiele als Maßstab

| Spiel | Team und Engine | Preis | Ergebnis | Was es dir zeigt |
| --- | --- | --- | --- | --- |
| [Lethal Company](https://en.wikipedia.org/wiki/Lethal_Company) | [Solo-Entwickler Zeekerss](https://appmagic.rocks/blog/friendslop-steam-games-2025), Unity | [9,75 €](https://store.steampowered.com/app/1966720/Lethal_Company/) | über 10 Mio. Kopien bis Januar 2024 (Schätzung) | Eine Person kann einen Koop-Hit landen |
| [Schedule I](<https://en.wikipedia.org/wiki/Schedule_I_(video_game)>) | TVGS, Unity, Teamgröße nicht belegt | [19,50 €](https://store.steampowered.com/app/3164500/Schedule_I/) | Spitze der Steam-Charts, [etwa 2 Jahre Early Access](https://store.steampowered.com/app/3164500/Schedule_I/) | Ein langer Early Access ist normal |
| [R.E.P.O.](https://en.wikipedia.org/wiki/R.E.P.O.) | semiwork, Unity | [9,49 €](https://store.steampowered.com/app/3241660/REPO/) | [über 152 Mio. US-Dollar laut AppMagic](https://appmagic.rocks/blog/friendslop-steam-games-2025) | Komik und niedriger Preis verbreiten sich |
| [Peak](<https://en.wikipedia.org/wiki/Peak_(video_game)>) | Aggro Crab und Landfall, Unity | [7,99 US-Dollar](https://appmagic.rocks/blog/friendslop-steam-games-2025) | über 10 Mio. Kopien bis August 2025 | Begann als einmonatiger Game Jam |
| [Content Warning](<https://en.wikipedia.org/wiki/Content_Warning_(video_game)>) | 5 Personen | [7,99 US-Dollar](https://appmagic.rocks/blog/friendslop-steam-games-2025) | über 1 Mio. Kopien im April 2024 | Kleine Idee, in zwei Monaten gebaut |
| [Deep Rock Galactic](https://en.wikipedia.org/wiki/Deep_Rock_Galactic) | Ghost Ship Games, UE4 |  | 8 Mio. bis Januar 2024 | Klassen und lange Pflege tragen Jahre |
| [Risk of Rain 2](https://en.wikipedia.org/wiki/Risk_of_Rain_2) | Hopoo Games, Unity |  | über 1 Mio. im ersten Monat | Schwierigkeit steigt mit der Zeit |
| [Vermintide 2](https://en.wikipedia.org/wiki/Warhammer:_Vermintide_2) | Fatshark, Stingray |  | 500.000 in 4 Tagen | Ego-Nahkampf gegen Horden funktioniert im Koop |

Vermintide 2 ist die wichtigste Referenz für den Schwertkampf. Risk of Rain 2 lieferte das Muster für die Zeit-Eskalation, die am 5. November 2026 dem Blutmond gewichen ist. Der [AI Director aus Left 4 Dead](https://en.wikipedia.org/wiki/Left_4_Dead) liefert das Muster für die Steuerung der Horde.

### Was die Solo- und Kleinteam-Erfolge lehren

Die größten Koop-Hits kleiner Teams sind einfacher als Far Far West. Das ist die wichtigste Einsicht für deinen Plan. Die Punkte hier sind meine Deutung der Quellen.

- **Eine Idee, die man weitererzählt.** GameDiscoverCo nennt als Treiber bei Roadside Research und Super Battle Golf [ein „deliciously subversive“ Format und ein schräges Konzept](https://newsletter.gamediscover.co/p/how-two-friendslop-hits-got-the-audience). Super Battle Golf verkaufte in 9 Tagen 500.000 Einheiten, Roadside Research 396.000 mit 903.000 Demo-Spielern.
- **Streamer als Verstärker.** Laut derselben Quelle kamen Streamer [5- bis 6-mal zu einem Spiel zurück](https://newsletter.gamediscover.co/p/how-two-friendslop-hits-got-the-audience). Dein Spiel braucht also Momente, die man zeigen will.
- **Niedriger Preis.** Die meisten dieser Spiele kosten 5,50–10 US-Dollar. Das Vorbild liegt mit 19,99 € deutlich darüber und bietet mehr.
- **Kurze Lebensdauer ist normal.** AppMagic nennt für das Genre eine [30-Tage-Bindung von rund 3 %](https://appmagic.rocks/blog/friendslop-steam-games-2025). Du brauchst zum Start also keine 100 Stunden Inhalt.
- **Unity ist auffällig häufig.** Das bewerte ich in Abschnitt 8. Bei Far Far West ist die Engine nicht offiziell bestätigt.

Mein Schluss: Du baust keinen Far-Far-West-Klon in voller Größe, sondern ein kleines eigenes Spiel mit seinem Kampfgefühl. Das Rüstungs-Slapstick liefert die Komik, das Schwert den Unterschied, und beides braucht keine große Inhaltsmenge.

## 3. Rechtsrahmen: Was ein Klon darf

Solange du das Spiel nur selbst spielst, ist Recht kaum ein Thema. Sobald du es veröffentlichst, darfst du Spielidee, Regeln und Genre übernehmen, den konkreten Ausdruck aber nicht. Dieser Abschnitt ist keine Rechtsberatung. Vor einem Verkauf solltest du dich kurz beraten lassen.

### Was Gerichte bisher gesagt haben

| Fall | Ergebnis | Lehre |
| --- | --- | --- |
| [Tetris Holding gegen Xio](https://en.wikipedia.org/wiki/Tetris_Holding,_LLC_v._Xio_Interactive,_Inc.) (USA, 2012) | Die Idee fallender Blöcke ist frei. Spielfeldgröße, Vorschau und Darstellungsdetails sind geschützt. Xio verlor. | Wer alle Ausdrucksdetails übernimmt, verliert, auch wenn die Regeln frei sind |
| [Spry Fox gegen Lolapps](https://en.wikipedia.org/wiki/Video_game_clone) (USA, 2012) | Einstweilige Verfügung trotz anderer Grafik, weil das Spiel nebeneinander gleich wirkte. Der Fall endete im Vergleich. | Ein anderes Thema allein schützt nicht, wenn Struktur und Eindruck gleich bleiben |
| [SAS gegen World Programming](https://en.wikipedia.org/wiki/SAS_Institute_Inc_v_World_Programming_Ltd) (EuGH, C-406/10, 2012) | Funktionalität und Programmiersprache einer Software sind nicht urheberrechtlich geschützt. Handbücher können es sein. | Das Beobachten und Nachbauen von Funktionen ist in der EU zulässig, das Kopieren von Text und Daten nicht |
| [Nexon gegen Ironmace](https://en.wikipedia.org/wiki/Dark_and_Darker) (Seoul, Februar 2025) | Keine Urheberrechtsverletzung, aber Verletzung von Betriebsgeheimnissen. Schadensersatz: 8,5 Mrd. Won (etwa 5,9 Mio. US-Dollar). | Das größte Risiko liegt oft nicht im Spielprinzip, sondern in mitgenommenen internen Daten |

Diese Fallbeschreibungen stammen aus Sekundärquellen und sind nicht deutsches Recht im Einzelfall. Sie zeigen die Richtung, ersetzen aber keine Prüfung.

### Unsere Regeln im Projekt

**Frei nutzbar:**

- Das Genre: Koop-Shooter mit Extraktion und Roguelite-Elementen.
- Das Grundmuster: Auftrag, Absprung, Ziel, Boss, Flucht, Hub.
- Allgemeine Mechaniken: Dash statt Sprint, Karten mit Boni, Elementar-Kombinationen, Reittier.
- Das Beobachten des Vorbilds als Käufer: Tempo messen, Abläufe notieren.

**Tabu:**

- Name, Logo, Figuren, Texte, Musik und Grafiken von Far Far West.
- Begriffe, die das Vorbild prägen. Unsere Karten heißen nicht Joker, unser Hub ist kein Saloon, unser Boss kein Geisterzug.
- Eins-zu-eins übernommene Zahlentabellen, Kartenlisten und Kartenlayouts. Zahlen entstehen im eigenen Balancing.
- Werbung mit dem Namen des Vorbilds, etwa auf der Steam-Seite. Spieler dürfen vergleichen, wir tun es nicht.

### Abgrenzungsprüfung

Zu jedem Meilenstein legen wir Screenshots beider Spiele nebeneinander. Die Frage lautet: Würde jemand beim Durchblättern glauben, das sei dasselbe Spiel oder ein offizieller Ableger? Bei Ja wird umgestaltet. Das Schwert, die Rüstungsfiguren, die Himmelsbarke und das Fantasy-Thema helfen dabei stark.

### Weitere rechtliche Aufgaben

| Thema | Aufgabe | Wann |
| --- | --- | --- |
| Eigene Namen und Figuren | Nichts vom Vorbild übernehmen, damit du dir später nichts verbaust | Von Anfang an |
| Fremde Pakete | Lizenz je Paket in eine Liste im Repository schreiben | Bei jeder Nutzung |
| KI-Inhalte | Liste führen, was mit welchem Werkzeug entstanden ist. Steam verlangt die [Angabe](https://partner.steamgames.com/doc/gettingstarted/contentsurvey), [Coding-Helfer im Hintergrund sind ausgenommen](https://www.notebookcheck.net/Steam-updates-AI-disclosure-form-requiring-developers-to-report-visible-and-in-game-AI-but-not-background-tools.1206103.0.html) | Von Anfang an, weil es sich später nur schwer nachholen lässt |
| Name des Spiels | Marke prüfen (DPMA, EUIPO, Steam-Suche) | Vor einer Veröffentlichung |
| Verkauf | Gewerbe und Steuer klären, auch bei kleinen Beträgen | Nur wenn du verkaufen willst |
| Daten von Spielern | Keine sammeln. Dann entfällt der größte Teil des Datenschutzes | Dauerhaft |

### Spielbar nach dem Ende des Betriebs

Die EU-Kommission hat die Bürgerinitiative Stop Killing Games laut Berichten [am 16. Juni 2026 ohne neues Gesetz beantwortet](https://www.galaxus.ch/en/page/the-stop-killing-games-initiative-has-failed-in-the-eu-but-there-is-hope-43037). Der Druck der Spieler und die Debatte um den Digital Fairness Act bleiben aber bestehen. Unser Listen-Server-Modell löst die Frage von selbst: Es gibt keinen eigenen Server, der abgeschaltet werden könnte.

## 4. Vision und Design-Säulen

Eisenseelen soll sich anfühlen wie ein Abend, an dem alle zu gierig werden, auch wenn du allein spielst. Zuerst muss es dir selbst Spaß machen, denn du bist Entwickler, Tester und erstes Publikum zugleich.

### Der Aufhänger

**Leere Rüstungen, gefüllt mit gebundenen Seelen, kämpfen als Söldner gegen verfluchte Länder.**

Ein Satz, ein Bild: eine Ritterrüstung ohne Körper, deren Helm beim Niederschlag davonrollt. Bei wenig Zeit und wenig Kunsterfahrung ist das die beste Wahl, aus sechs Gründen:

- **Starre Teile statt Haut:** Jedes Rüstungsteil hängt an genau einem Knochen. Das spart Gewichte-Malen, Hautverformung und Stoffsimulation. Skelett-Gegner funktionieren genauso. Das ist die größte Einzelersparnis im ganzen Plan.
- **Keine Gesichter:** Keine Lippensynchronität, keine Haare, keine Mimik.
- **Leichte Anpassung:** Helme, Umhänge und Wappen sind einfach zu bauen und ergeben sofort sichtbare Vielfalt.
- **Erklärung für das Spielprinzip:** Eine Seele kann zurückgeholt werden, eine Rüstung lässt sich aufrüsten.
- **Eigener Humor:** Klappernde Gliedmaßen und verlorene Helme tragen die Komik, die das Genre verbreitet.
- **Abgrenzung:** Kein Roboter, kein Cowboy, kein gewöhnlicher Fantasy-Held.

Am 3. November 2026 hast du dich anhand von Probebildern für die leeren Rüstungen entschieden. Ein späterer Wechsel wäre teuer, denn menschliche Figuren bräuchten fertige, gehäutete Modelle aus Paketen.

### Übertragung vom Wilden Westen in die Fantasy

| Vorbild | Eisenseelen | Was sich dabei ändert |
| --- | --- | --- |
| Roboter-Cowboy | Beseelte Rüstung | 2 Klassen statt eines einheitlichen Helden |
| Revolver, Gewehr, Schrotflinte | Schwert, Wurfäxte, Feuerstab, Blitzrute | Der Nahkampf kommt neu dazu und braucht eigene Regeln |
| Zauber in der linken Hand | Zauber bei beiden Klassen, beim Magier stärker | 4 statt 5 Elemente |
| Sheriff und Stadt | Eine kleine Anlegestelle über den Wolken | Eine kleine graue Fläche, keine Stadt |
| Auftrag und Kopfgeld | Jagdbrief | 3 Briefe zur Wahl mit Gebiet, Boss und Bannbruch, die Belohnung bleibt geheim |
| Schwebezug, Glocke am Bahnhof | Himmelsbarke, Signalfeuer entzünden | Die Barke ist nur ein starres Modell |
| Joker | Keine Boni im Lauf | Dein Build entsteht zwischen den Läufen |
| Pferd | Kein Reittier | Kleine Karte und der Sprint ersetzen es |
| Geister, Skelette, Aliens | Aschenheer | 1 Fraktion |
| Yeehaw-Wunsch der Spieler | Helm-Wurf, Schlachtruf-Taste vorerst herausgenommen | Kostet fast nichts, macht auch allein Spaß |

### Die fünf Design-Säulen

Jede Entscheidung wird an diesen Säulen gemessen. Was keine davon stärkt, kommt nicht ins Spiel.

1. **Jede Klasse ist ein eigenes Gefühl.** Das Schwert spielt sich anders als der Stab. Ein Klassenwechsel soll sich anfühlen wie ein neues Spiel im gleichen Rahmen.
2. **Bewegung ist die Verteidigung.** Es gibt keine Deckung und kein Stehenbleiben. Wer sich gut bewegt, überlebt auch mit schlechter Ausrüstung.
3. **Gier gegen Flucht.** Mit jeder Minute steigen Beute und Gefahr. Die spannendste Frage eines Laufs lautet: Noch ein Nebenauftrag oder gleich zum Boss?
4. **Allein gut, zusammen besser.** Solo muss von der ersten Version an Spaß machen. Koop kommt dazu, ohne dass das Spiel davon abhängt.
5. **Laut, bunt, lesbar.** Gegnerische Angriffe sind aus 30 m erkennbar. Jede Aktion hat eine deutliche Rückmeldung. Das Spiel darf albern sein, aber nie unklar.

Dazu kommt eine Regel nur für dich: **Jede Woche muss sich lohnen.** Nach jeder Sitzung soll es etwas geben, das du ausprobieren kannst. Was sich über Wochen nicht sichtbar auszahlt, kommt nicht ins Spiel.

### Was wir bewusst nicht bauen

- **Kein PvP.** Das spart Anti-Cheat, Server und Balancing-Streit.
- **Keine Echtgeld-Käufe, kein Battle Pass, keine Live-Dienste.** Du schuldest niemandem Updates.
- **Keine Story-Kampagne und keine Vertonung.** Geschichte erzählen ein paar Texte und Orte.
- **Kein prozedurales Gelände.** Eine gebaute Karte mit zufälligem Inhalt reicht.
- **Kein Reittier.** Es bräuchte Modell, Animationen, Netzcode und größere Karten.
- **Kein großer Hub.** Eine kleine Anlegestelle reicht, keine Stadt und keine Händlerfiguren.
- **Keine Konsolen und keine eigenen Server.** Beides würde dich dauerhaft binden.
- **Keine großen Systeme „für später“.** Du baust, was du diese Woche ausprobieren kannst.

### Für wen du das baust

| Wer | Was daraus folgt |
| --- | --- |
| Du selbst | Der einzige Maßstab, der bis zum Ende zählt. Baue das, was dir beim Spielen fehlt |
| Ein paar Freunde, falls sie Lust haben | Koop funktioniert, ist aber kein Zwang |
| Vielleicht andere Spieler | Eine Entscheidung ganz am Ende, siehe Abschnitt 14 |

## 5. Spielablauf: Lauf, Extraktion, Tod und Fortschritt

Ein Lauf dauert etwa 10 bis 18 Minuten und endet entweder im Abflug mit der Beute oder im Verlust eines großen Teils davon. Kurze Läufe passen zu deinen kurzen Sitzungen, denn du kannst in einer Stunde mehrere testen. Der Kreislauf bleibt der des Vorbilds, nur kürzer und enger.

&#91;embedded content: Kreislauf eines Laufs · eigene Darstellung\]

Wer scheitert, landet trotzdem in der Abrechnung: 30 % von Gold und Fraktionspunkten aus dem Lauf bleiben, die Zutaten sind verloren, die Erfahrung bleibt ganz, und was du gekauft hast, verlierst du nie.

### Ablauf eines Laufs

| Abschnitt | Minute (Mittel) | Was passiert | Ab Stufe |
| --- | --- | --- | --- |
| Vorbereitung | an der Anlegestelle | Klasse, Waffen und Rune wählen, einen von 3 Jagdbriefen nehmen | 2 |
| Überfahrt | 0:00 | Alle steigen ein, die Barke setzt das Team am Kartenrand ab | 2 |
| Nebenaufträge | 0–7 | 3 zufällige kleine Aufträge, freiwillig. Sie stehen ab der Ankunft auf der Karte | 2 |
| Bannbruch | 7–10 | Die Vorquest des Bosses: ihn befreien, beschwören oder wecken | 2 |
| Bosskampf | 10–13 | An seinem Platz auf der Karte, ohne Arena und Pflicht für die Flucht. Viel Gold und Zutaten aus seinem Beute-Set | 2 |
| Flucht | 13–15 | Signalfeuer entzünden, 60 Sekunden gegen Wellen halten, Barke besteigen | 2 |
| Abrechnung | danach | Gold, Punkte und Zutaten, dann zurück zur Anlegestelle | 2 |

### Blutmond: Gefahr nur bei Aufgaben

**Entschieden am 5. November 2026:** Die steigende Verderbnis ist gestrichen. Es gibt kein Zeitlimit und keine Stufen mehr, und die Gegner werden mit der Zeit nicht stärker. Wie beim Vorbild entsteht die Gefahr durch das, was du gerade tust.

- Sobald du eine Aufgabe hältst, steigt der **Blutmond** auf: Der Himmel färbt sich rot, und Wellen greifen an. Aufgaben sind Schrein oder Altar halten, Ankersteine halten, der Bosskampf, die Flucht nach dem Boss und das Signalfeuer.
- Ist die Aufgabe vorbei, kommen keine Wellen mehr. Der Himmel bleibt noch 8 Sekunden rot.
- Ohne Aufgabe kämpfst du nur gegen die Lager. Ein leeres Lager bleibt leer.
- Belohnungen sind fest, und Gegner und Items bleiben gleich stark, egal wie lange der Lauf dauert.
- Der Kettenkoloss kommt nur in Elite-Gruppen, also am Signalfeuer.

Länger bleiben ist damit nicht gefährlicher, aber jede weitere Aufgabe kostet Leben und Zeit. Welche Welle wann kommt, steht unter „Lager, Gruppen und Wellen“.

### Jagdbrief und Nebenaufträge

**Entschieden am 5. November 2026:** An der Anlegestelle wählst du einen von 3 Jagdbriefen. Er zeigt Gebiet, Boss und Bannbruch. Die Belohnung bleibt geheim, auch welche Fraktionspunkte es auf der Karte gibt. Das Spiel würfelt dafür 3 kleine Nebenaufträge, jeder von einer zufälligen Fraktion: Feuer, Frost, Blitz und später Wind. Sie stehen ab der Ankunft auf der Karte und sind freiwillig. Die Aufgaben sind Anführer jagen, Schrein halten, bewachte Truhe erobern und Altar halten, und jede dauert etwa 1 bis 2 Minuten. Ein Nebenauftrag bringt Gold und Fraktionspunkte.

### Tod und Wiederkehr

| Fall | Regel |
| --- | --- |
| Lebenspunkte auf null, zu zweit oder mehr | Die Rüstung bricht zusammen, der Helm rollt. 30 Sekunden Ausbluten |
| Wiederbeleben | Mitspieler hält 3 Sekunden die Taste am Helm |
| Solo | Eine **Seelenanker-Ladung** pro Lauf, die dich wieder aufstehen lässt |
| Alle gefallen | Lauf gescheitert, 30 % von Gold und Punkten bleiben, die Erfahrung bleibt ganz |
| Host verlässt das Spiel | Lauf endet für alle, keine Strafe |
| Außerhalb eines Kampfes | Wer 6 Sekunden weder getroffen wurde noch selbst getroffen hat, bekommt 10 % seines Lebens pro Sekunde zurück |

### Schwierigkeit und Spielerzahl

| Stufe | Gegnerleben | Bedrohung | Beute |
| --- | --- | --- | --- |
| Lehrling | 75 % | 70 % | 70 % |
| Geselle | 100 % | 100 % | 100 % |
| Meister | 150 % | 130 % | 160 % |

| Spieler | Bedrohungsbudget | Gegnerleben |
| --- | --- | --- |
| 1 | 45 % | 75 % |
| 2 | 70 % | 90 % |
| 3 | 85 % | 100 % |
| 4 | 100 % | 110 % |

Du stellst alles zuerst für einen Spieler ein. Die Zeilen für 2–4 Spieler sind erste Annahmen, die du nur mit Bots und gelegentlichen Mitspielern prüfen kannst. Das Vorbild wird dafür kritisiert, dass es [Solospielern keine Erleichterung gibt](https://game8.co/articles/reviews/far-far-west-review-early-access), du startest von der anderen Seite.

### Anlegestelle und Fortschritt

Die Anlegestelle ist eine kleine begehbare graue Plattform über den Wolken, mit der Barke und dem Brett für die Jagdbriefe. Alles andere bleibt ein Menü.

- **Jagdbrett:** einen von 3 Jagdbriefen wählen, später auch die Schwierigkeit. Seit dem 5. November gebaut: F am Brett nimmt einen anderen Brief, und jeder Brief nennt einen anderen Boss.
- **Ausrüstung und Schmiede:** Das Level öffnet Waffen und Rüstung in der Schmiede, gekauft wird mit Gold, verbessert mit Gold, Fraktionspunkten und Zutaten. Alles auf der Taste I.

Jede Währung hat genau einen Zweck: **Gold** kauft und verbessert, **Fraktionspunkte** braucht jede Verbesserung, **seltene Zutaten** vom Boss die stärksten Verbesserungen, und **Erfahrung** von normalen Gegnern bringt Level, die neue Teile in der Schmiede öffnen.

| Spielzeit | Was erreicht sein soll |
| --- | --- |
| Nach 30 Minuten | Erster Abflug, erste Waffe gekauft |
| Nach 2 Stunden | Beide Klassen ausprobiert, Meister erreichbar |
| Nach 5 Stunden | Alles freigeschaltet |

Das klingt kurz, passt aber zu einem kleinen Spiel, und jede Stunde mehr wäre bezahlte Wiederholung. Du stellst die Kurve nach Gefühl ein, die Preise stehen in einer Textdatei.

### Sessionfluss

- **Beitreten:** Zunächst LAN oder direkte Verbindung. Einladungen über Steam nur, wenn du veröffentlichst.
- **Getrennte Beute:** Jeder sammelt Gold, Punkte, Zutaten und Erfahrung für sich.
- **Kein Eigenbeschuss,** und Beute geht durch Mitspieler nie verloren.
- **Schlachtruf-Taste** ist vorerst herausgenommen und kann später mit eigener Taste zurückkommen.

## 6. Helden und Kampf

Es gibt genau die zwei Klassen, um die es dir geht: Schwertkämpfer und Magier. Das Kampfgefühl ist der einzige Bereich, in dem das Spiel an das Vorbild heranreichen soll, und zugleich der, den du am besten selbst beurteilen kannst. Alle Zahlen sind Startwerte, die du beim Spielen änderst.

### Die Klassen

|  | Klingenwacht (Schwertkämpfer) | Arkanist (Magier) |
| --- | --- | --- |
| Fantasie | Mitten in der Horde stehen und sie zerlegen | Mit einem Spruch ganze Gruppen vernichten |
| Waffen | Langschwert in vier Elementen, Wurfäxte | Feuerstab, Froststab, Blitzrute |
| Klassenfähigkeit | Sturmlauf: 12 m Ansturm, stößt Gegner um, 6 s Abklingzeit | Blinzeln: 10 m Teleport, 5 s, auch in der Luft |
| Passiv | Kills mit dem Schwert geben kurzfristig Rüstung | 20 % kürzere Abklingzeit bei Zaubern |
| Schwäche | Ferne Gegner | Wenig Lebenspunkte |

Beide Klassen nutzen dieselben Zauber, also kann auch eine Klingenwacht Feuerbälle werfen. Das Vorbild macht es ähnlich, dort kann man sich [auf Zauberschaden, Heilung oder Kopfschüsse spezialisieren](https://store.steampowered.com/app/3124540). Eine dritte Klasse ist etwas für die Zeit, nachdem alles andere fertig ist.

### Der Schwertkampf in der Ego-Perspektive

Der Nahkampf ist die größte Design-Aufgabe, und du bist dafür die wichtigste Prüfinstanz. [Vermintide 2](https://en.wikipedia.org/wiki/Warhammer:_Vermintide_2) zeigt, dass [wuchtiger Ego-Nahkampf](https://en.wikipedia.org/wiki/Warhammer:_Vermintide_2) gegen Horden im Koop tragen kann.

**Die Lösung ohne Animationswissen: der Hieb als Kurve.** Panzerhandschuh und Schwert sind starre Teile. In der Ego-Sicht bewegt sie kein Skelett, sondern eine Bewegungskurve aus einer Textdatei. Du sagst dem Agenten „der Hieb soll schneller beginnen und länger ausschwingen“, er ändert die Werte, du probierst es aus. So brauchst du kein Animationsprogramm zu lernen.

| Problem | Antwort |
| --- | --- |
| Nahkämpfer kommen nicht rechtzeitig an den Gegner | Sturmlauf, automatische Vorwärtsbewegung beim Hieb von bis zu 1,5 m, der Sprint bleibt nutzbar |
| Treffer fühlen sich leicht an | Kurzer Treff-Stopp von 40–60 ms, Kamerastoß, breite Klingenspur, deutlicher Schlagton, Knochen splittern |
| Schlechte Übersicht in der Horde | Richtungshinweise am Bildschirmrand, lauter Ton bei Angriffen von hinten |
| Ferne Gegner | Wurfäxte als Zweitwaffe, Sprung-Schlag, Zauber |

| Aktion | Dauer | Schaden | Reichweite und Form | Besonderheit |
| --- | --- | --- | --- | --- |
| Leichter Hieb (immer hin und her) | 0,45 s | 35 | 2,8 m, Bögen von 130° | Trifft bis zu 5 Ziele, je Ziel 15 % weniger Schaden |
| Schwerer Hieb (halten) | 0,6 s Aufladen | 110 | 3,2 m, Abwärtsbogen | Lässt Gegner 1 s taumeln, später aber nicht jede Gegnerart. Durchbricht Schilde |
| Sprung-Schlag | nach Fallhöhe | 80–180 | 4 m Fläche am Boden | Belohnt den Sprung in der Luft |
| Blocken | rechte Maustaste halten | 60 % weniger frontal | 120° Frontbereich | Verlangsamt um 40 %. Parade in den ersten 0,2 s nimmt keinen Schaden und lässt den Gegner 1 s taumeln. Rechtsklick bricht Hieb und Aufladen sofort ab |

Es gibt keine Ausdauerleiste und keinen Eigenbeschuss. Treffer laufen über Abfragen entlang des Klingenwegs, nicht über die Animation. Damit bleibt das Ergebnis vorhersehbar, und der Agent kann es automatisch testen.

### Der Magier als Schießgefühl

Ein Stab muss sich so gut anfühlen wie eine Waffe. Der Arkanist feuert darum sofort, ohne Zauberbalken. Auch der Stab ist ein starres Objekt mit Bewegungskurve.

| Stab | Vergleich | Feuerart | Schaden | Besonderheit |
| --- | --- | --- | --- | --- |
| Feuerstab | Revolver | Einzelschuss bei jedem Klick, keine Wartezeit | 40, kleine Explosion | Kostet 10 Mana. Rechte Maustaste 0,4 s halten für den Feuerball: 120 Schaden, 40 Mana |
| Blitzrute | Maschinenpistole | Dauerstrahl, 10 Treffer pro Sekunde | 8, springt auf 2 Ziele | Überhitzt statt Munition |

**Mana (entschieden am 3. November 2026):** Den Feuerstab begrenzt nicht die Wartezeit, sondern das Mana. Er hat 100 Mana und bekommt 20 pro Sekunde zurück, auch während er schießt. Reicht es nicht, passiert nichts, und der Balken blinkt rot. Die Klingenwacht hat kein Mana. Der Feuerball auf Q braucht kein Aufladen, hat dafür eine Abklingzeit von 6 s, beim Arkanisten 4,8 s, und kostet kein Mana.

### Zauber: Elemente und Reaktionen

Zwei Zauberplätze, vier Elemente, sechs Reaktionen und später eine große zu dritt. Das reicht für Kombinationen und bleibt gut testbar. Jeder Treffer trägt am Ziel einen Zustand auf, und ein anderes Element löst eine Reaktion aus, auch über Spielergrenzen.

**Entschieden am 3. November 2026:** Jeder Held hat genau zwei Zauber.

- **Waffenzauber auf Q:** Jede Waffe und jeder Zauberstab hat genau einen festen Zauber. Ein Feuerschwert gibt zum Beispiel einen Feuerzauber, und verschiedene Stäbe geben verschiedene Zauber.
- **Runenzauber auf E:** Der Zauber hängt an der Rune. Es gibt verschiedene Runen mit verschiedenen Zaubern, und du wählst immer genau eine, und zwar nur vor dem Lauf.
- **Reaktionen allein:** Haben beide Zauber verschiedene Elemente, löst du Reaktionen selbst aus, gleichzeitig oder nacheinander. Nach dem ersten Treffer bleiben dir dafür 2 Sekunden. Du bist dafür nicht auf Mitspieler angewiesen.
- **Reaktionen zu dritt:** Später kommen Reaktionen aus drei verschiedenen Zaubern dazu. Weil jeder Held nur zwei Zauber hat, braucht man dafür immer ein Teammitglied. Nach einer Reaktion trägt das Ziel 2 Sekunden lang eine Markierung. Trifft in dieser Zeit das dritte Element, gibt es den Dreiklang.
- **Builds:** Klasse, Waffe und Rune ergeben zusammen einen Build. Später kannst du Builds zusammenstellen und mit verschiedenen Spezialisierungen experimentieren.

| Element | Zustand am Ziel | Zauber |
| --- | --- | --- |
| Feuer | Brennend | Feuerball auf der Waffe, Flammenwand auf der Flammenrune |
| Eis | Verlangsamt, bei Stapelung gefroren | Eislanze auf der Waffe, Frostnova auf der Frostrune |
| Blitz | Geladen | Kettenblitz auf der Waffe, Sturmfeld auf der Sturmrune |
| Wind | Verweht | Windstoß auf der Waffe, Wirbelsturm auf der Windrune |

| Reaktion | Auslöser | Wirkung |
| --- | --- | --- |
| Dampfexplosion | Feuer auf Eis oder umgekehrt | Flächenschaden, Sichtschutz durch Dampf |
| Splitterbruch | Blitz oder schwerer Hieb auf Gefrorene | Hoher Schaden, Splitter treffen Nachbarn |
| Funkenflug | Feuer auf Geladene oder Blitz auf Brennende | Funken springen auf Nachbarn über und setzen sie in Brand |
| Feuersturm | Wind auf Brennende oder Feuer auf Verwehte | Ein Wirbel zieht Gegner an und setzt sie in Brand |
| Schneesturm | Wind auf Verlangsamte oder Eis auf Verwehte | Ein Wirbel zieht Gegner an und verlangsamt sie, wer drin bleibt, friert ein |
| Gewittersturm | Wind auf Geladene oder Blitz auf Verwehte | Ein Wirbel zieht Gegner an, und Blitze schlagen in alle darin ein |
| Dreiklang | Das dritte Element auf ein markiertes Ziel, nur mit Teammitglied | Sehr hoher Flächenschaden |

Die Klingenwacht kann also selbst einfrieren und dann zerschlagen. Das funktioniert allein genauso wie mit einem Mitspieler, und deshalb kannst du es auch allein beurteilen.

**Reihenfolge:** Die Reaktionen sind das Herz dieses Systems, deshalb kommt die erste früh und wird sofort gespielt. So weißt du schnell, ob die Idee trägt, bevor viele Waffen und Runen entstehen.

| Stufe | Was dazukommt |
| --- | --- |
| 2 | Frostrune mit Frostnova auf E, Zustände am Ziel, das 2-Sekunden-Fenster, Dampfexplosion. Der Feuerball bleibt auf Q |
| 3 | Sturmrune mit Sturmfeld, Funkenflug, Splitterbruch, Reaktionen zwischen zwei Spielern |
| 5 | Waffe und Rune im Ausrüstungsmenü wählen, Builds |
| 6 | Alle Schwerter und Stäbe mit festen Zaubern, Flammenrune, Dreiklang, dazu eine Taste in der Testarena, die das dritte Element auslöst, damit du ihn allein ausprobieren kannst |
| 2 | Das vierte Element Wind: Windrune mit Wirbelsturm, Windstoß auf den Waffen, die drei Wind-Reaktionen und alle Windwaffen |

Das kostet grob 10 Runden zusätzlich. In den Rundenzahlen der Stufen ist das noch nicht enthalten.

**Stand 4. November 2026:** Runen sind Items, ihre Werte stehen in `Data/runen.json`. Frostrune mit Frostnova, Sturmrune mit Sturmfeld und Flammenrune mit Flammenwand sind im Spiel. Du wählst sie im Ausrüstungsmenü auf der Taste I. Zum Start hast du die Frostrune, die anderen kaufst du in der Schmiede, und in der Testarena sind alle frei. Alle Reaktionen sind drin: Dampfexplosion mit Wolke, Funkenflug, Splitterbruch und der Zustand Gefroren. Die Wolke verdeckt vorerst nur dir die Sicht, Gegner sehen hindurch.

**Wind als viertes Element, entschieden am 4. November 2026, noch nicht gebaut:** Wind bewegt Gegner. Feuer macht Schaden über Zeit, Frost hält an, Blitz verteilt, und Wind stößt weg oder zieht zusammen. So hat zu viert jeder eine eigene Aufgabe, und auch allein bringt es etwas, denn es passt zur Säule „Bewegung ist die Verteidigung“. Der Windstoß auf der Waffe stößt Gegner von dir weg, der Wirbelsturm auf der Windrune zieht sie in einen Wirbel. Die drei neuen Reaktionen sind derselbe Wirbel, der das Element des Ziels aufnimmt, das hält den Aufwand klein. Der Kettenkoloss lässt sich nicht bewegen. Dazu kommen die Windwaffen und die Rüstungsart Windwirker, siehe den Reiter Items und Builds. Die Farbe ist ein helles Mintgrün, weil Orange, Hellblau und Gelb vergeben sind. Beim Bau werden Sturmrune, Sturmfeld und Sturmklinge umbenannt, weil „Sturm“ nach Wind klingt. Aufwand: etwa 4 bis 6 Runden, das ist eine Schätzung.

### Keine Omen mehr

**Entschieden am 4. November 2026:** Omen sind gestrichen. Im Lauf wirst du nicht mehr stärker, dein Build steht vor dem Einsteigen fest. Nebenaufträge bringen Gold und Fraktionspunkte, und damit verbesserst du Waffen und Rüstung zwischen den Läufen.

Wie Waffen und Rüstung besser werden, steht hier: Items und Builds

### Bewegung

Die Bewegung kostet keine Kunst und trägt das ganze Spielgefühl. Sie steht deshalb ganz am Anfang.

| Element | Startwert | Hinweis |
| --- | --- | --- |
| Lauftempo | 5,5 m/s | [Wert laut Fan-Wiki beim Vorbild](https://farfarwest.wiki.gg/wiki/Speedster) |
| Sprint | 8 m/s, solange Shift gehalten wird | Deine Entscheidung nach dem ersten Spielen. Das Vorbild hat keinen Sprint |
| Sprung | 1 m Scheitelhöhe, von dir beim Spielen festgelegt | Schwerkraft-Faktor 1,6 für rund 0,7 s Flugzeit |
| Kein Bunnyhop | Ein Sprung nimmt nie mehr Tempo mit als am Boden erlaubt | Deine Entscheidung nach dem ersten Spielen, weil Hüpfen zu viel Vorteil gab. Die Tempo-Kette ist dafür gestrichen |
| Luftkontrolle | 55 % | Richtung bleibt in der Luft änderbar. Oberhalb des Lauftempos dreht die Flugrichtung mit 200 Grad pro Sekunde und behält das Tempo |
| Fallschaden | keiner | Sonst bremst er die Bewegung aus |

Auf der Karte helfen Sprungfelder. Das sind einfache Objekte mit großer Wirkung.

### Tasten

| Taste | Wirkung |
| --- | --- |
| Linke Maustaste | Klingenwacht: Hieb, halten für den schweren Hieb. Arkanist: Schuss |
| Rechte Maustaste | Klingenwacht: blocken, solange gehalten. Arkanist: halten für den Zauber der Waffe |
| Q | Zauber der Waffe, je nach Element Feuerball, Eislanze oder Kettenblitz, für beide Klassen |
| Shift | Sprint, solange gehalten |
| Leertaste | Springen |
| F | Benutzen: in die Barke steigen, Truhen öffnen, Signalfeuer entzünden |
| M | Karte mit Nebenaufträgen, Boss und Signalfeuer |
| E | Zauber der ausgerüsteten Rune |
| I | Ausrüstungsmenü: an der Anlegestelle, in der Testarena und nach dem Lauf |
| C, H, R | Nur in der Testarena: Klasse wechseln, Horde, Neustart |

### Inhaltsmengen

| Inhalt | Nach Stufe 1 | Nach Stufe 3 | Am Ende |
| --- | --- | --- | --- |
| Klassen | 1–2 | 2 | 2 |
| Waffen | 2 | 3 | 7 |
| Zauber | 1 | 3 | 8 |
| Runen | 0 | 2 | 4 |

## 7. Gegner, Bosse und Welt

Am Ende gibt es 6 Gegnertypen, 1 Boss und 1 Karte. Das ist wenig, aber Gegner-Gruppen, Blutmond-Wellen und zufällige Orte holen daraus deutlich mehr heraus. Alle Gegner nutzen dasselbe Skelett und frei verfügbare Animationen.

### Gegner

Skelette bestehen aus starren Knochen, und die [Universal Animation Library von Quaternius](https://quaternius.com/packs/universalanimationlibrary2.html) bietet über 130 Animationen für menschliche Skelette unter CC0-Lizenz. Jeder Gegner stellt dir eine eigene Frage.

| Gegner | Bedrohung | Leben | Frage an dich |
| --- | --- | --- | --- |
| Knochenläufer | 1 | 40 | Schwarm, schnell: Wie überstehst du die Zeit in der Zone? |
| Knochenwerfer | 2 | 60 | Bleibt auf Distanz und weicht zurück: Wann läufst du zu ihm hin? |
| Schildträger | 4 | 220 | Frontalblock: Gehst du außen herum, wartest du auf seinen Hieb oder nimmst du einen Zauber? |
| Glutschädel | 2 | 20 | Explodiert im markierten Kreis: Weichst du aus oder schießt du ihn vorher ab? |
| Kettenkoloss | 10 | 900 | Elite mit markiertem Ansturm, den nichts aufhält: Wohin weichst du aus? |
| Totenrufer | 6 | 180 | Hebt Gefallene auf: Wen tötest du zuerst? |

Jeder Angriff hat eine sichtbare und hörbare Ankündigung von mindestens 0,4 s. Das Vorbild wird wegen [zu schwacher Gegner-Signale](https://themagicrain.com/2026/05/far-far-west-review/) kritisiert. Gegner sind Daten: Werte und Verhalten stehen in Textdateien.

Knochenwerfer, Schildträger und Kettenkoloss sind seit dem 3. November 2026 im Spiel, alle noch als graue Kapseln. Der Koloss stürmt auf einem rot markierten Weg an. Der Totenrufer ist noch nicht gebaut.

**Entschieden am 5. November 2026:** Jeder Gegner bekommt wie beim Vorbild eine klare Rolle:

- **Knochenwerfer:** Kommst du näher als 6 m, weicht er nach dem Wurf zurück.
- **Schildträger:** Der Schild hält von vorn Schüsse und leichte Hiebe. Ein schwerer Hieb geht durch, und Zauber, Runen und Explosionen gehen um den Schild herum. Nach seinem eigenen Hieb senkt er den Schild kurz und ist offen.
- **Glutschädel:** Neu, ein dunkler Körper mit glühendem Kopf. Er läuft auf dich zu, zeigt einen Kreis am Boden und explodiert nach 1 Sekunde für 30 Schaden. Schießt du ihn vorher ab, explodiert er trotzdem, trifft dann aber nur Gegner mit 60 Schaden, und andere Glutschädel in der Nähe gehen mit hoch.
- **Kettenkoloss:** Seinen Ansturm halten weder Parade noch Frost auf.

### Lager, Gruppen und Wellen

**Entschieden am 3. November 2026:** Gegner kommen nicht mehr von überall und immer. Am Anfang eines Laufs stehen sie nur in festen Lagern. **Seit dem 5. November 2026** gibt es keinen Nachschub mehr, Wellen kommen nur noch beim Blutmond.

Alle Gegner kommen in Gruppen. Eine Gruppe ist eine Zeile in `Data/gruppen.json` und fühlt sich wie ein neuer Gegner an. Die Zahlen gelten für einen Spieler, mit mehr Spielern werden die Gruppen größer.

- **Lager:** Auf der Karte stehen 18 feste Lager, jedes mit einer Gruppe. Ihre Gegner stehen Wache und kommen erst, wenn sie dich im Umkreis von 25 m frei sehen oder getroffen werden. Dann kommt die ganze Gruppe mit. Wer sich weiter als 50 m absetzt, wird in Ruhe gelassen, und die Gegner gehen zurück. Ein leeres Lager bleibt leer.
- **Wellen:** Solange du eine Aufgabe hältst, steigt der Blutmond auf, und Wellen greifen an. Die Größe hängt nur von der Aufgabe ab, nicht von der Zeit.
- **Truhe:** Die bewachte Truhe ruft eine mittlere Gruppe als Wächter.
- **Obergrenze:** Sind schon Gegner mit 16 Bedrohung hinter dir her, kommt vorerst keine neue Gruppe.

| Aufgabe | Welle | Alle |
| --- | --- | --- |
| Schrein oder Altar halten | klein | 12 s |
| Ankerstein halten | mittel | 15 s |
| Bosskampf | mittel | 20 s |
| Flucht nach dem Boss | groß | 15 s |
| Signalfeuer | Elite | 20 s |

Zum Start eines Laufs stehen 14 kleine und 4 mittlere Gruppen in den Lagern.

| Größe | Gruppe | Bedrohung | Wer |
| --- | --- | --- | --- |
| klein | Knochentrupp | 3 | 3 Läufer |
| klein | Spähtrupp | 4 | 2 Läufer, 1 Werfer |
| klein | Wurfpaar | 4 | 2 Werfer |
| klein | Einzelwache | 4 | 1 Schildträger |
| klein | Glutläufer | 4 | 2 Läufer, 1 Glutschädel |
| mittel | Knochenrudel | 6 | 6 Läufer |
| mittel | Wurftrupp | 6 | 2 Läufer, 2 Werfer |
| mittel | Schildwache | 7 | 3 Läufer, 1 Schildträger |
| mittel | Schützenreihe | 7 | 1 Läufer, 3 Werfer |
| mittel | Feuerrotte | 7 | 3 Läufer, 2 Glutschädel |
| groß | Knochenmeute | 10 | 10 Läufer |
| groß | Sturmtrupp | 11 | 3 Läufer, 2 Werfer, 1 Schildträger |
| groß | Schildwall | 12 | 2 Werfer, 2 Schildträger |
| groß | Brandhaufen | 14 | 4 Läufer, 2 Werfer, 3 Glutschädel |
| Elite | Koloss mit Gefolge | 14 | 4 Läufer, 1 Koloss |
| Elite | Kriegshaufen | 22 | 2 Läufer, 2 Werfer, 1 Schildträger, 1 Glutschädel, 1 Koloss |

### Der Boss

**Entschieden am 5. November 2026:** Der Boss steht ohne geschlossene Arena an einem festen Bossplatz auf der Karte. Jeder Boss hat einen eigenen Bannbruch, also einen eigenen Weg, wie er befreit, beschworen oder geweckt wird, und ein eigenes Beute-Set mit Chancen. Was er fallen lässt, siehst du erst, wenn er fällt. Nur nach dem Boss kannst du fliehen: Das Signalfeuer steht an einem festen Ort, und nur dort holt dich die Barke ab. Geplant sind zuerst drei graue Bosse: Der Gefesselte, Der Seelenrufer und Die Leere Wacht. Die Wandernde Feste bleibt die spätere, teure Fassung: eine laufende Burg mit zerstörbaren Türmen aus starren Teilen, die sich per Code bewegt. Die Einzelheiten stehen im Reiter Items und Builds. Gebaut sind seit dem 5. November Der Gefesselte und Der Seelenrufer.

### Die Karte

Es gibt eine Karte, die **Aschesteppe**: orange, verbrannte Dörfer, gefallene Riesen. Sie ist 300 × 300 m groß, das Gelände ist fest, der Inhalt zufällig.

- **Gelände:** Höhendaten per Skript aus einer Entwurfsdatei, du beurteilst die Vorschau und sagst, was sich ändern soll.
- **Orte:** 8 Plätze für größere Orte (Ruine, Lager, Friedhof). Jeder Lauf aktiviert 4–5 davon.
- **Bossplätze, Ankunft und Signalfeuer:** je 3 bis 4 feste Orte, das Spiel wählt pro Lauf einen.
- **Dichte:** Alle 20 Sekunden Laufweg etwas Lohnendes.
- **Modifikatoren:** Nacht und Aschesturm. Sie kosten fast nichts, aber der Lauf fühlt sich anders an.

Zwei weitere graue Gebiete sind eingeplant, siehe Reiter Items und Builds. Ein zweites fertig gestaltetes Biom ist erst etwas für die Zeit nach dem Ende des Plans.

## 8. Technologiewahl von Grund auf

Die Engine-Wahl bleibt Unreal Engine 5.8, weil die schwerste Aufgabe des Spiels dort schon gelöst ist. Bei wenig Zeit zählt, was du nicht selbst bauen musst. Das ist vor allem die Netzvorhersage einer schnellen Ego-Bewegung.

### Engine-Vergleich

Die Punkte reichen von 1 bis 5 und sind meine Einschätzung. Geld spielt keine Rolle, denn alle drei Engines sind auf dieser Stufe kostenlos.

| Kriterium | Gewicht | Unreal Engine 5.8 | Unity 6 | Godot 4 |
| --- | --- | --- | --- | --- |
| Vom Agenten steuerbar | 30 % | 4 | 4 | 5 |
| Netz und schnelle Ego-Bewegung fertig dabei | 30 % | 5 | 3 | 2 |
| Schneller Kreislauf: Ändern und Ausprobieren | 15 % | 3 | 4 | 5 |
| Anfängerfreundlich | 15 % | 2 | 3 | 4 |
| Geringe Hardware-Ansprüche | 10 % | 3 | 4 | 3 |
| **Summe** |  | **3,75** | **3,55** | **3,75** |

Unreal und Godot liegen gleichauf, und die Entscheidung fällt am Netzcode. Bei einem Fehler in selbst gebauter Bewegungsvorhersage könntest du kaum helfen, und Koop ist ein Kern des Spiels. Dein Rechner ist stark genug für Unreal, der Kostenpunkt Hardware entfällt also.

### Warum Unreal die Vorhersage schon löst

Der [Character Movement Component](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine) enthält fertige Client-Vorhersage und Korrektur. Sprint und das Tempo beim Blocken kommen als eigene Regeln hinein. Du startest leer und nutzt das [First-Person-Template](https://dev.epicgames.com/documentation/unreal-engine/first-person-template-in-unreal-engine) nur als Nachschlagewerk, weil seine Inhalte auf Blueprints aufbauen.

Du bleibst bis zum Ende auf der Version 5.8. Engine-Updates kosten Zeit und bringen dir nichts Sichtbares.

### Technik-Stack

| Bereich | Wahl | Warum |
| --- | --- | --- |
| Sprache | C++ | Der Agent liest und schreibt Text. Blueprints sind dafür schwer zugreifbar |
| Spielwerte | Textdateien (CSV oder JSON) | Du kannst Zahlen selbst ändern, ohne Code zu verstehen |
| Effekte und Fähigkeiten | Eigenes kleines System | Das Gameplay Ability System wäre für diesen Umfang zu groß und schwer zu lernen |
| Modelle | Blender per Skript, CC0-Pakete | Kostenlos und vom Agenten steuerbar |
| Versionskontrolle | Git mit Git LFS, neues Repository | Jede Sitzung endet mit einem Stand, zu dem du zurückkehren kannst |
| Rechner | Dein vorhandener PC | 14 Kerne, 32 GB RAM und eine RTX 4080 reichen locker. Halte etwa 100 GB frei |
| Mehrspieler-Tests | Zwei Fenster auf einem PC | Kein zweiter Rechner nötig |
| Verbindung | LAN oder direkte IP | Steam kommt erst, falls du veröffentlichst |

### Was der Agent in der Engine steuern kann

| Aufgabe | Weg | Einschätzung |
| --- | --- | --- |
| C++ schreiben und kompilieren | Text, Kommandozeile | Gut |
| Tests im Hintergrund laufen lassen | `UnrealEditor-Cmd` mit Automation | Gut |
| Assets importieren, Karten befüllen | Python im Editor | Gut |
| Blueprints bauen | Kaum per Text möglich | Schlecht, deshalb vermeiden |
| Materialien und Partikel | Per Python mühsam, einfache Vorlagen gehen | Mittel |
| Beurteilen, ob es Spaß macht | Gar nicht | Das bleibt deine Aufgabe |

Der Agent arbeitet also am besten mit wenigen Grafik-Assets, viel Code und Textdaten. Genau darauf ist dieser Plan ausgelegt.

## 9. Technische Architektur

Die Architektur folgt vier Regeln: Der Host entscheidet die Welt, der eigene Client entscheidet das eigene Gefühl, jeder Inhalt ist Text und jedes System hat automatische Tests. Die vierte Regel ist die wichtigste für dich. Ohne sie könntest du die Arbeit des KI-Agenten nicht beurteilen, und die Fehler würden sich unbemerkt stapeln.

&#91;embedded content: Aufgabenteilung im Netz · eigene Darstellung\]

Der Host rechnet alles, was alle betrifft. Der Gast zeigt seine eigenen Aktionen sofort und bekommt das Ergebnis danach bestätigt.

### Module

Wenige, klar getrennte Bereiche machen es dem Agenten leichter, und du behältst den Überblick.

| Modul | Inhalt |
| --- | --- |
| Kern | Lauf-Ablauf, Blutmond, Beute, Spielstand |
| Held | Bewegung, Klassen, Waffen, Zauber, Gravuren |
| Gegner | Gegnertypen, Direktor, Boss |
| Welt | Karte, Orte, Sprungfelder, Nebenaufträge |
| Oberfläche | Anzeigen, Menüs, Taverne |
| Tests | Automatische Prüfungen und Testbots |

### Regeln für den Agenten

- **Eine Aufgabe, ein Commit.** Jeder Commit nennt, was sich ändert.
- **Kein Commit ohne grüne Tests.** Der Agent zeigt dir das Ergebnis im Bericht.
- **Werte stehen nie im Code.** Alles, was du beim Spielen ändern möchtest, steht in Textdateien.
- **Keine Blueprints mit Logik.** Sie dürfen nur Modelle und Werte zuordnen.
- **Der Server entscheidet.** Das gilt von Anfang an, auch allein.
- **Neue Systeme nur auf deinen Wunsch.** Der Agent baut nichts vorsorglich.

### Netz-Rollen

Ein Spieler ist zugleich Host und Server, Mitspieler sind Clients.

| Was | Wer entscheidet | Was der Client vorab zeigt |
| --- | --- | --- |
| Bewegung | Server korrigiert | Sofort eigene Bewegung |
| Schwerthieb, Schuss, Zauber | Server prüft Treffer | Sofort Animation und Ton |
| Lebenspunkte, Tod | Server | Nichts |
| Gegner | Server | Nur Darstellung |
| Gold, Punkte und Zutaten | Server, je Spieler getrennt | Nichts |

Du brauchst dich damit nicht auszukennen. Es ist eine Bauvorgabe für den Agenten, damit Koop in Stufe 3 nicht zum Umbau wird.

### Horde

Gleichzeitig leben höchstens 20 Gegner, in der Spitze 30. Das reicht für das Gefühl einer Horde, wenn sie aus mehreren Richtungen kommt. Du nutzt die normalen Unreal-Bausteine: Pathfinding, wenige Updates pro Sekunde, einfache Ragdolls.

Mass-Entity und ähnliche Systeme für tausende Gegner entfallen. Sie wären für 30 Gegner zu aufwändig.

### Bandbreite

Zwei Regeln genügen. Gegner senden wenige Updates, Effekte laufen beim Empfänger nur als Darstellung. Mehr Optimierung folgt erst, wenn ein Test zu zweit oder zu viert ruckelt. Da du meist allein testest, steht das erst in Stufe 3 an.

### Spielstände

Ein Spielstand ist eine einfache lokale Datei auf dem PC des Spielers. Schreiben in eine Zwischendatei und danach umbenennen schützt vor Absturzschäden. Cloud-Speicher und Kopierschutz entfallen, weil es nichts gibt, was sich zu schützen lohnt.

### Einstellungen

Maus-Empfindlichkeit, Sichtfeld, Lautstärke, Tastenbelegung und Bildschirmmodus reichen. Controller-Unterstützung und weitere Komfort-Funktionen kommen in Stufe 7 dazu, falls du veröffentlichst.

## 10. Content-Pipeline

Grafik und Sound stammen aus drei Quellen in fester Reihenfolge: aus Skripten, aus freien Paketen, und erst dann aus generativen Werkzeugen. So bleibt es billig und einheitlich, auch wenn du nicht zeichnen kannst.

### Reihenfolge der Quellen

| Reihenfolge | Quelle | Wofür | Kosten |
| --- | --- | --- | --- |
| 1 | Blender-Skripte des Agenten | Rüstungen, Waffen, Ruinen, Barke, Boss | 0 € |
| 2 | Freie CC0-Pakete, etwa [Kenney](https://kenney.nl/assets/mini-dungeon) und [Quaternius](https://quaternius.com/) | Requisiten, Gerümpel, Skelette, Animationen | 0 € |
| 3 | Generative Werkzeuge wie [Meshy](https://www.meshy.ai/pricing) | Einzelne Requisiten, Texturen, Probebilder | 0–20 € im Monat, nur bei Bedarf |

Die Quaternius-Seite nennt ihre Modelle ausdrücklich frei für persönliche und kommerzielle Projekte. Bei Kenney habe ich die CC0-Angabe nur auf der verlinkten Paketseite geprüft, du siehst also je Paket nach.

### Generative KI im Hobby

Generative KI ist in einem Hobbyprojekt erlaubt, und du nutzt sie, wo sie dir Arbeit spart. Falls du später verkaufen möchtest, wird es nur eine Offenlegung, denn Steam fragt danach, und [die Daten zeigen schlechtere Verkäufe bei sichtbaren KI-Inhalten](https://fragwyz.substack.com/p/three-years-of-ai-on-steam). Darum führst du von Anfang an eine einfache Liste, welche Assets generiert sind. Die Lizenz hängt vom Tarif ab: Die Gratis-Stufe von Meshy steht unter CC BY 4.0 und verlangt Namensnennung.

### Der Stil

Der Stil ist flach, kräftig und starr. Er ist so gewählt, dass Fehler kaum auffallen.

- **Starre Teile:** Rüstungsteile hängen ohne Hautgewichte am Skelett.
- **Flache Farben:** Wenige Materialien aus einem kleinen Farbsatz statt detaillierter Texturen.
- **Zusammenbruch als Gag:** Beim Tod fallen die Teile per Physik auseinander, dazu rollt der Helm. Das kostet keine Animation.
- **Ego-Sicht:** Hände und Waffe bewegen sich über Kurven, siehe Abschnitt 6.
- **Karte:** Starke Himmelsfarbe und Nebel tragen die Stimmung und schlucken Details.

**Entschieden am 3. November 2026:** Beide Klassen bekommen denselben flachen Stil aus einfarbigen Flächen, ohne Umrisslinien. Die Teile schweben mit sichtbaren Lücken, in der Brust leuchtet ein Kristallherz, und Lichtfäden halten die Teile zusammen. Farbe und Licht zeigen die Klasse: Die Klingenwacht ist stahlblau mit Goldkanten, blauem Licht und einem blauen Umhang, der Arkanist tiefrot mit Goldkanten, orangem Licht und einem Spitzhut. Die Vorlagen liegen im Projekt unter `SourceAssets/Konzepte/Klingenwacht` und `SourceAssets/Konzepte/Arkanist`.

Du beurteilst Vorschaubilder, der Agent ändert die Skripte. Damit bleibt die Entscheidung bei dir, ohne dass du Blender lernen musst.

### Wie Modelle ins Spiel kommen

**Gebaut am 5. November 2026:** Die grauen Klötze bleiben die Wahrheit für Treffer, Wände und Wege. Ein Modell liegt nur darüber und versteckt sie. Welches Modell wo liegt, steht in `Data/aussehen.json`: Jeder Held, jede Hand, jeder Gegner, jeder Boss, jede Waffe, jedes Rüstungsteil und jeder Ort hat dort schon eine leere Zeile. Ein Pfad genügt, und das Modell erscheint beim nächsten Start. Fehlt es, bleiben die Klötze. Die Mitspieler sehen dich jetzt als Körper, du selbst siehst nur deine Hand.

Die Reihenfolge: zuerst Modelle über diese Datei, dann Effekte für Zauber und Treffer, dann die Umgebung von Hand in den Karten. Selbst gesetzte Dinge überstehen jeden Neubau der Karten, denn das Skript ersetzt nur, was es selbst gebaut hat. Alle Regeln zu Maßstab, Richtung, Sockeln und Animationen stehen in `docs/Kunst-Leitfaden.md`.

### Sound und Musik

Sound trägt das Trefferfühl und kostet fast nichts. Die [Sonniss-GDC-Pakete](https://sonniss.com/gameaudiogdc/) sind kostenlos und kommerziell nutzbar, ohne Namensnennung. [Ihre Lizenz](https://sonniss.com/gdc-bundle-license/) verbietet aber KI-Training. Musik kommt als CC0-Stücke oder bleibt am Anfang ganz weg. Vertonung gibt es keine.

### Sprachen

Das Spiel erscheint auf Englisch und Deutsch. Alle Texte stehen von Anfang an in einer Tabelle, damit der Agent sie übersetzen kann.

### Inhaltsaufwand in Runden

Eine Runde ist eine Aufgabe von etwa 2 Stunden deiner Zeit. Das sind Schätzungen.

| Inhalt | Runden |
| --- | --- |
| Rüstung für eine Klasse | 3–4 |
| Waffe mit Hiebkurve und Ton | 2–3 |
| Zauber mit Effekt | 1–2 |
| Gegner mit Skelett und Animationen | 2–4 |
| Ort auf der Karte | 2 |
| Boss | 15–20 |

## 11. Leistungsbudgets, Tests und Qualität

Du kannst fast alles allein testen, wenn der Agent die Prüfungen übernimmt und du dich aufs Spielgefühl konzentrierst. Tests sind in diesem Plan kein Aufwand, sondern die Art, wie du einem Code vertraust, den du nicht liest.

### Leistung

Das Ziel sind 60 Bilder pro Sekunde auf deinem PC, in der lautesten Szene mit 30 Gegnern. Fällt der Wert darunter, sucht der Agent die Ursache, bevor neuer Inhalt dazukommt. Weitere Budgets sind nicht nötig.

### Was der Agent testet

| Prüfung | Wann | Was sie findet |
| --- | --- | --- |
| Kompilieren und schnelle Tests | Vor jedem Commit | Grobe Fehler |
| Automatischer Spielstart ohne Fenster | Vor jedem Commit | Abstürze beim Laden |
| Bewegung wird gemessen | Nach Änderungen daran | Tempo und Sprunghöhe weichen von den Werten ab |
| Testbot spielt einen Lauf | Einmal pro Sitzung | Hänger, Lauf lässt sich nicht beenden |
| Zwei Spieler im selben Editor | Ab Stufe 3, vor jedem Commit | Netzfehler |
| Bildrate in einer festen Szene | Einmal im Monat | Langsam wachsende Leistungsprobleme |

### Wie du allein testest

- **Spielgefühl:** Du spielst nach jeder größeren Änderung ein paar Minuten und schreibst drei Sätze auf, was stört.
- **Koop:** Zwei Fenster auf einem PC zeigen die meisten Fehler. Dazu kommen Bots, die den zweiten Spieler mimen.
- **Echter Test:** Einmal in Stufe 3 sollte ein Freund mitspielen. Falls es niemanden gibt, geht es auch ohne, dann bleibt ein Rest an Unsicherheit.
- **Abstand:** Nach einer Pause von zwei Wochen fühlst du Fehler besser als nach zwei Stunden.

### Wann etwas fertig ist

- Die Tests laufen grün.
- Du hast es selbst gespielt und es fühlt sich richtig an.
- Die Werte stehen in Textdateien.
- Es ist committet, mit einem Satz, was sich ändert.

## 12. Stufenplan statt Terminplan

Es gibt keine Termine, sondern acht Stufen, und jede endet mit etwas, das du spielen kannst. Hörst du nach einer Stufe auf, hast du trotzdem etwas Fertiges. Das ist für ein Hobby wichtiger als jedes Datum.

&#91;embedded content: Zeitplan · eigene Planung, Stand 2. November 2026\]

Die Monate sind grobe Schätzungen mit etwa 8 Runden im Monat, Pausen eingerechnet. Eine Runde ist eine Aufgabe von etwa 2 Stunden deiner Zeit. Insgesamt sind es 184 Runden.

### Die Stufen

| Stufe | Runden | Monat | Was du danach spielen kannst |
| --- | --- | --- | --- |
| 0 Aufsetzen | 4 | 0–1 | Eine leere Karte, auf der du herumläufst. Tests laufen ohne dich |
| 1 Spielgefühl | 20 | 1–3 | Graue Arena mit Bewegung, Schwert, Stab und einem Gegner |
| 2 Erster Lauf | 25 | 3–6,5 | Ein ganzer Lauf, von der Anlegestelle bis zum Abflug, alles grau |
| 3 Zu zweit | 15 | 6,5–8,5 | Derselbe Lauf mit einem zweiten Spieler, inklusive Wiederbeleben |
| 4 Aussehen | 35 | 8,5–13 | Rüstungen, Karte, Effekte und Sound. Es sieht aus wie ein Spiel |
| 5 Fortschritt | 20 | 13–15,5 | Taverne und Schwierigkeitsgrade. Den Grund für den nächsten Lauf geben schon Schmiede und Gravuren |
| 6 Inhalt | 40 | 15,5–21 | Alle Waffen, Zauber und Gegner, dazu der Boss |
| 7 Feinschliff | 25 | 21–24 | Das fertige Spiel. Veröffentlichen bleibt freiwillig |

### Was in jeder Stufe entsteht

- **Stufe 0, Aufsetzen:** Neues Projekt, Regeln für den Agenten, automatische Tests, leere Karte.
- **Stufe 1, Spielgefühl:** Bewegung, Klingenwacht mit Schwert und Block, Arkanist mit Feuerstab und Mana, ein Zauber, Knochenläufer.
- **Stufe 2, Erster Lauf:** Anlegestelle, Überfahrt, Jagdbriefe, Nebenaufträge, Bannbruch, Signalfeuer und Abflug, dazu Blutmond, Lager, Gruppen und Wellen. Dazu Knochenwerfer, Schildträger, Glutschädel und Kettenkoloss, die drei Runen und alle Reaktionen. Dazu Spielstand, Gold, Fraktionspunkte, Schmiede, Gravuren, Meisterstücke, Zutaten, Erfahrung, drei graue Bosse, zwei weitere graue Gebiete und das vierte Element Wind mit Windwaffen.
- **Stufe 3, Zu zweit:** Beitreten über LAN oder IP, Niederschlag und Wiederbeleben, Seelenanker, Tests mit zwei Fenstern.
- **Stufe 4, Aussehen:** Rüstungen und Waffen beider Klassen, die Karte Aschesteppe, Effekte, Sound und Himmel.
- **Stufe 5, Fortschritt:** Taverne als Menü und Schwierigkeitsgrade. Spielstand, Schmiede und Erfahrung sind schon in Stufe 2 dazugekommen.
- **Stufe 6, Inhalt:** Die Rute, weitere Runen und Zauber, die übrigen Rüstungsarten, der Dreiklang, die ersten 6 Herzkristalle, weitere Zutaten, Totenrufer, weitere Gruppen, alle 8 Orte, Modifikatoren und die Wandernde Feste als großer Boss.
- **Stufe 7, Feinschliff:** Balance, Einstellungen, Fehlerliste und Übersetzung, dann die Entscheidung über eine Veröffentlichung.

### Die Arbeitswoche

Drei Sitzungen von je etwa 2 Stunden passen in deine 5–6 Stunden, und der Agent arbeitet dazwischen weiter. Du kannst sie frei verteilen.

| Sitzung | Was du tust | Was der Agent tut |
| --- | --- | --- |
| 1. Spielen und entscheiden | Spielen, drei Sätze aufschreiben, die nächste Aufgabe formulieren | Baut, testet, committet, schreibt einen Bericht |
| 2. Ausprobieren | Bericht lesen, Ergebnis spielen, nachsteuern | Korrigiert, was dir nicht gefällt |
| 3. Feinarbeit | Werte in den Textdateien ändern, bis es sich gut anfühlt | Nur kleine Änderungen |

### Regeln gegen wachsenden Umfang

- **Neue Ideen kommen auf eine Liste.** Du prüfst sie erst am Ende der laufenden Stufe.
- **Eine rein, eine raus.** Wer in einer Stufe etwas ergänzt, streicht etwas Gleichgroßes.
- **Drei Runden sind die Grenze.** Was nach drei Runden nicht läuft, wird vereinfacht oder gestrichen.
- **Grau zuerst.** Grafik kommt erst, wenn das Spiel in Grau Spaß macht.
- **Keine Stufe überspringen.** Sie bauen aufeinander auf.

### Der Lust-Check

Statt Prüfpunkten stellst du dir am Ende jeder Stufe drei Fragen. Sie entscheiden, wie es weitergeht.

1. Würde ich das heute Abend freiwillig spielen?
2. Habe ich mich auf die letzten Sitzungen gefreut?
3. Was nervt mich gerade am meisten?

Bei einem Nein baust du um, verkleinerst das Ziel oder machst Pause. Besonders ernst ist ein Nein zu Frage 1 nach Stufe 1, denn dann hilft auch keine Grafik. Du darfst auch aufhören, das ist bei einem Hobby kein Scheitern.

| Wenn du hier aufhörst | Hast du |
| --- | --- |
| Nach Stufe 1 | Einen Kampf-Prototyp, den du herzeigen kannst |
| Nach Stufe 3 | Ein graues, aber vollständiges Koop-Spiel für Abende mit Freunden |
| Nach Stufe 5 | Ein kleines, schönes Spiel mit Fortschritt |
| Nach Stufe 7 | Das ganze Spiel |

## 13. Du und die KI: Rollen, Zeit und Kosten

Du gibst die Richtung vor, die KI führt aus, und deine Zeit geht vor allem ins Spielen und Urteilen. Du musst nicht programmieren lernen, aber du musst sagen können, was dir fehlt. Das ist eine eigene Fähigkeit, und sie wächst schnell.

### Wer was macht

| Aufgabe | Du | Der Agent |
| --- | --- | --- |
| Was gebaut wird | Entscheidest | Schlägt vor, warnt vor Aufwand |
| Code, Tests, Skripte | Liest nur den Bericht | Schreibt alles |
| Spielgefühl | Beurteilst, änderst Werte in Textdateien | Baut Regler und Messwerte ein |
| Grafik | Beurteilst Vorschaubilder | Schreibt Skripte, bindet Pakete ein |
| Fehler | Beschreibst, was du siehst | Sucht die Ursache |
| Sicherung | Prüfst, dass es geklappt hat | Committet nach jeder Aufgabe |

### Wohin deine 5–6 Stunden gehen

Das ist eine grobe Annahme für eine normale Woche.

| Tätigkeit | Stunden pro Woche |
| --- | --- |
| Spielen und Eindrücke notieren | 2 |
| Aufgaben formulieren, Berichte lesen | 1,5 |
| Werte ändern und ausprobieren | 1,5 |
| Lernen und Puffer | 0,5–1 |

### Eine gute Aufgabe

Die Qualität deiner Aufgaben entscheidet über das Ergebnis. Eine einfache Vorlage reicht:

- **Ziel:** Was soll sich beim Spielen anders anfühlen?
- **Prüfung:** Woran erkennst du, dass es klappt?
- **Grenze:** Was darf sich nicht ändern?

Ein Beispiel: Der schwere Hieb soll wuchtiger wirken. Ich prüfe das an der Trainingspuppe. Das Tempo des leichten Hiebs bleibt gleich.

### Was du lernen solltest

Du lernst nur, was du wirklich brauchst, und der Agent erklärt dir den Rest, wenn du fragst.

- **Editor-Grundlagen:** Projekt öffnen, Karte laden, Play drücken, im Fenster umsehen. Das dauert 2–3 Stunden.
- **Berichte lesen:** Grüne und rote Tests unterscheiden, den letzten guten Stand erkennen.
- **Spiele auseinandernehmen:** Beim Spielen anderer Titel notieren, warum sich etwas gut anfühlt.

### Wo der Agent dir nicht helfen kann

- Er kann nicht fühlen, ob ein Hieb Spaß macht. Das Urteil bleibt immer bei dir.
- Er kann sich sicher anhören und trotzdem falsch liegen. Vertraue dem Test und dem Spielen, nicht dem Bericht.
- Er vergisst Zusammenhänge. Eine kurze Projektdatei mit Regeln und aktuellem Stand gleicht das aus.
- Er baut gern mehr, als du wolltest. Darum gehört die Grenze in jede Aufgabe.

### Kosten

Der Agent ist der einzige nennenswerte Posten, alles andere ist kostenlos oder schon vorhanden.

| Posten | Kosten | Hinweis |
| --- | --- | --- |
| KI-Agent | 20–100 US-Dollar im Monat | [Claude Pro und Max](https://www.claude.com/pricing). Am Anfang reicht vermutlich das kleine Abo |
| Unreal Engine | 0 € | Lizenzgebühren erst ab 1 Mio. US-Dollar Umsatz, [laut Wikipedia](https://en.wikipedia.org/wiki/Unreal_Engine) |
| Blender, Git | 0 € |  |
| Freie Pakete, Sound | 0 € | Abschnitt 10 |
| Generative Werkzeuge | 0–20 € im Monat | Nur bei Bedarf |
| Steam-Gebühr | 100 US-Dollar | Nur wenn du dort veröffentlichst |
| PC | 0 € | Dein vorhandener Rechner reicht |

Über 2 Jahre sind das etwa 500–2.500 €. Der Rahmen ergibt sich aus 24 Monaten mal 20 oder 100 US-Dollar. Das ist kein Investment, das sich lohnen muss, sondern der Preis eines Hobbys.

## 14. Veröffentlichung: freiwillig und am Ende

Du brauchst weder Förderung noch Publisher, und ob du das Spiel je veröffentlichst, entscheidest du erst in Stufe 7. Nichts in diesem Plan hängt davon ab. Das nimmt den Druck aus dem ganzen Projekt.

### Drei Wege

| Weg | Zusatzaufwand | Kosten | Was dazukommt |
| --- | --- | --- | --- |
| Nur für dich und Freunde | Keiner | 0 € | Spielordner weitergeben, LAN oder direkte Verbindung |
| Kostenlos auf itch.io | 1–2 Runden | 0 € | Eine Download-Seite, keine Steam-Anbindung |
| Günstig auf Steam | 15–25 Runden | 100 US-Dollar | Einladungen über Steam, Shopseite, Steuerformulare |

Der erste Weg ist die Vorgabe. Die anderen beiden sind ein Bonus, falls dir das Spiel am Ende gefällt und du es anderen zeigen möchtest.

### Falls du auf Steam gehst

Steam verlangt [100 US-Dollar Gebühr und 21 Tage Wartezeit](https://partner.steamgames.com/doc/gettingstarted/onboarding), bevor ein Spiel erscheinen darf. Bei einem Preis von 6,99 € bleiben dir nach meiner Rechnung etwa 3,40 € pro Kopie:

| Schritt | Annahme | Rest |
| --- | --- | --- |
| Verkaufspreis |  | 6,99 € |
| Ohne Umsatzsteuer | Im Schnitt 15 % | 6,08 € |
| Regionale Preise und Erstattungen | Abzug von 20 % | 4,86 € |
| Anteil von Steam | 30 % | 3,40 € |

Die 100 US-Dollar sind also nach rund 30 Kopien wieder drin. Mehr brauchst du nicht zu erwarten, um es für ein Hobby gut zu finden.

### Was sich beim Verkauf ändert

- **Steuern:** Einnahmen musst du versteuern. Lass dir das vor dem ersten Verkauf von einer Fachperson erklären.
- **Recht:** Abschnitt 3 wird dann ernst. Prüfe Namen, Modelle und Lizenzen aller Pakete.
- **KI-Angabe:** Steam fragt danach, und deine Liste aus Abschnitt 10 beantwortet die Frage.
- **Erwartungen:** Käufer dürfen Fehlerbehebungen erwarten. Du entscheidest vorher, wie viel du leisten willst, und schreibst es auf die Seite.

## 15. Zeigen und bekannt machen, falls du willst

All das ist freiwillig und kostet nur Zeit, die dir sonst beim Spiel fehlt. Zeig das Spiel frühestens, wenn es dir selbst gefällt, also etwa ab Stufe 4.

- **Kurze Clips:** 20–30 Sekunden vom Helm, der davonrollt, oder vom Zusammenbruch. Das ist die billigste Werbung, und sie macht Spaß.
- **Steam-Seite:** Erst ab Stufe 6, wenn du ernsthaft verkaufen willst. Sie braucht Bilder und einen kurzen Trailer.
- **Demo und [Next Fest](https://partner.steamgames.com/doc/marketing/upcoming_events/nextfest):** Jedes Spiel kann nur einmal teilnehmen, also ist das eine Sache für den Schluss.
- **Offen über KI sprechen:** Sag von Anfang an, wie dein Spiel entsteht. Kritik gibt es trotzdem, aber du musst nichts verbergen.
- **Kein Early Access als Pflicht:** Wenn du nicht monatelang Updates liefern willst, erscheint das Spiel einfach fertig.
- **Keine Versprechen:** Du schuldest niemandem Updates, solange du keine ankündigst.

Rechne mit wenigen Verkäufen. Hits wie das Vorbild sind Ausnahmen, und ein Hobbyprojekt mit 5–6 Stunden pro Woche muss das nicht werden. Dein Erfolgsmaß bleibt: Es macht dir Spaß, und du wirst fertig.

## 16. Risikoregister

Die größten Risiken liegen nicht in der Technik, sondern in deiner Lust und im Umfang. Danach folgen die Grenzen der KI. Wahrscheinlichkeit und Wirkung sind meine Einschätzung.

| Risiko | Wahrscheinlichkeit | Wirkung | Frühwarnung | Gegenmaßnahme | Plan B |
| --- | --- | --- | --- | --- | --- |
| Die Lust geht verloren | Hoch | Hoch | Du schiebst Sitzungen auf | Jede Woche etwas Spielbares, Pausen sind erlaubt | Nach Stufe 3 oder 5 enden |
| Der Umfang wächst | Hoch | Hoch | Neue Systeme, bevor eine Stufe fertig ist | Ideenliste, eine rein, eine raus | Streichliste unten |
| Tiefe Fehler, die du nicht beurteilen kannst | Mittel | Hoch | Derselbe Fehler über drei Runden | Kleine Schritte, Tests, zurück zum letzten guten Stand | Funktion einfacher lösen oder streichen |
| Agent macht Fehler, die Tests nicht finden | Mittel | Mittel | Das Spiel fühlt sich nach einer Änderung anders an | Nach jeder Änderung selbst spielen, Bewegung messen | Commit rückgängig machen |
| Koop bleibt ungeprüft | Mittel | Mittel | Fehler, die erst auf zwei PCs auftreten | Zwei Fenster, Bots, ein Abend mit einem Freund | Das Spiel funktioniert auch solo |
| Das Aussehen gefällt dir nicht | Mittel | Mittel | Vorschaubilder überzeugen dich nicht | Stil ist geklärt, Rüstungen früh als Vorschaubild prüfen | Fertige Pakete statt eigener Skripte |
| Datenverlust | Niedrig | Hoch | Lange kein Backup | Nach jeder Sitzung in ein privates Online-Repository sichern | Letzter gesicherter Stand |
| KI-Preise oder -Regeln ändern sich | Mittel | Mittel | Ankündigungen der Anbieter | Alles bleibt normaler, lesbarer Code | Kleineres Abo, langsameres Tempo |
| Rechtliche Probleme | Niedrig | Mittel | Nur bei Verkauf relevant | Eigene Namen und Modelle, Abschnitt 3 | Nicht veröffentlichen |

### Streichliste

Wird es eng, streichst du in dieser Reihenfolge. Das Spiel bleibt dabei spielbar.

1. Die Wandernde Feste, denn sie kostet am meisten. Der graue Boss bleibt.
2. Karten-Modifikatoren und die letzten Orte.
3. Kettenkoloss und Totenrufer.
4. Blitzrute und Wurfäxte.
5. Mehr als 2 Spieler.
6. Die Hälfte der Gravuren und Meisterstücke.

Nicht verhandelbar sind Bewegung, Schwert, Stab und ein Lauf, der mit dem Abflug endet. Ohne sie ist es nicht mehr dieses Spiel.

## 17. Die ersten 12 Wochen

Die ersten 12 Wochen sollen genau eine Frage beantworten: Macht es Spaß, mit Schwert und Stab durch eine graue Arena zu jagen? Sie decken Stufe 0 und Stufe 1 ab. Grafik, Menüs und Netz spielen noch keine Rolle.

### Der Ablauf

| Woche | Was entsteht | Deine Aufgabe | Fertig, wenn |
| --- | --- | --- | --- |
| 1 | Neues Projekt, Regeln für den Agenten, Tests, leere Karte | Projekt einmal öffnen und herumlaufen | Du läufst in der Karte, alle Tests sind grün |
| 2 | Testarena mit Rampen, erste Werte in Textdateien | Einen Wert selbst ändern und die Wirkung spüren | Laufen fühlt sich nach deinem Wert anders an |
| 3–4 | Bewegung: Sprung, Sprint, Sprungfelder | Werte einstellen, bis es sich gut anfühlt | 5 Minuten Bewegen macht ohne Gegner Spaß |
| 5–7 | Klingenwacht: Hiebkurve, Treffer, Treff-Stopp, Blocken, Ton | Auf die Trainingspuppe einschlagen und urteilen | Der Hieb fühlt sich wuchtig an |
| 8–9 | Knochenläufer als graue Kapseln, einfaches Spawnen | Menge und Tempo einstellen | 20 Gegner bringen dich in Bedrängnis |
| 10–11 | Arkanist: Feuerstab, Feuerball, Mana | Mit dem Schwert vergleichen | Beide Klassen fühlen sich verschieden an |
| 12 | Nichts Neues | Eine Stunde spielen, Lust-Check | Du entscheidest: weiter, umbauen oder aufhören |

### Woche 1 im Detail

In der ersten Woche baust du noch nichts Spielbares, sondern die Grundlage für alle weiteren Sitzungen.

1. Der Agent legt ein neues, leeres Repository an, ohne Altlasten aus anderen Projekten.
2. Er richtet ein leeres Unreal-Projekt mit C++ ein, ohne Blueprints mit Logik.
3. Er schreibt die Projektdatei mit den Regeln aus Abschnitt 9 und dem aktuellen Stand.
4. Er baut einen Befehl, der kompiliert und alle Tests ohne Fenster startet.
5. Du öffnest das Projekt, drückst Play und läufst herum.
6. Du richtest ein privates Online-Repository ein, damit die Arbeit gesichert ist.

### Die zwei frühen Entscheidungen

- **Stil (Woche 1–2):** Entschieden am 3. November 2026: leere Rüstungen im flachen Stil für beide Klassen. Die Probebilder hast du selbst erzeugt.
- **Erste Klasse (Woche 5):** Die Klingenwacht kommt zuerst, weil der Nahkampf das größere Risiko ist. Sollte er scheitern, erfährst du es früh.

### Was du in dieser Zeit nicht machst

- Keine Rüstungen, Karten oder Effekte über Probebilder hinaus.
- Keine Menüs außer einem Neustart-Knopf.
- Kein Netz. Der Server entscheidet aber schon von Anfang an, damit Stufe 3 kein Umbau wird.
- Keine Steam-Anbindung, kein Marketing, keine Texte.

### Woran du nach Woche 12 erkennst, dass es sich lohnt

Du spielst länger als geplant, ohne es zu merken. Das ist der einzige Maßstab, und ihn kann dir kein Test abnehmen. Falls das nicht passiert, ist das eine wertvolle Erkenntnis nach nur 60–70 Stunden.

## 18. Quellen und offene Punkte

Die Recherche fand am 2. November 2026 statt. Die Liste nennt nur die Quellen, auf die sich dieser Plan noch stützt. Einschränkungen stehen unten, und alle Werte für dein Spiel sind Setzungen dieses Plans.

### Far Far West

- [Steam-Seite](https://store.steampowered.com/app/3124540): Merkmale, Preis, Systemanforderungen, Sprachen, Bewertungen, Early-Access-Angaben
- [Evil Raptor](https://www.evilraptor.com/): Studio, Standort, Team, frühere Spiele
- [ScreenRant, Interview mit Nicolas Meyssonnier](https://screenrant.com/far-far-west-developer-interview/): Team, Entstehung der Extraktion, Pläne, Kritik
- [EG7, Pressemitteilung vom 4. Mai 2026](https://www.enadglobal7.com/press-releases/far-far-west-delivers-strong-initial-sales-on-steam-with-over-500000-units-sold-the-first-days-after-launch/): Verkäufe, Wunschlisten, gleichzeitige Spieler
- [MMORPG.com, 19. Mai 2026](https://www.mmorpg.com/news/futuristic-wild-west-co-op-shooter-far-far-west-gets-first-major-update-and-hits-1-million-in-sales-2000138103): 1 Mio. Verkäufe, erstes großes Update
- [GameWatcher](https://www.gamewatcher.com/news/far-far-west-patch-notes-hub-and-roadmap-of-updates-for-2026): Patch-Verlauf bis Juni 2026
- Rezensionen und Vorschauen: [Game8](https://game8.co/articles/reviews/far-far-west-review-early-access), [GamingTrend](https://gamingtrend.com/previews/far-far-west-preview/), [Indiecator](https://indiecator.org/2026/06/03/far-far-west-review/), [Gamereactor](https://www.gamereactor.eu/far-far-west-1714343/), [The Magic Rain](https://themagicrain.com/2026/05/far-far-west-review/), [ComicBuzz](https://comicbuzz.com/far-far-west-review/)
- Guides und Fan-Quellen: [Fan-Wiki Speedster](https://farfarwest.wiki.gg/wiki/Speedster), [Wikily](https://wikily.gg/far-far-west), [Ninewiki](https://ninewiki.com/guide/far-far-west-guide-combos-secrets-and-quests/), [Dot Esports zu Reittieren](https://dotesports.com/far-far-west/news/far-far-west-mounts)
- PC Gamer: [Yeehaw-Knopf](https://www.pcgamer.com/games/fps/far-far-west-is-a-surprise-hit-on-steam-but-its-players-have-one-demand-a-yeehaw-button/), [Magie](https://www.pcgamer.com/games/fps/developers-of-steam-next-fests-wildly-popular-robot-wizard-cowboy-shooter-say-its-cowboy-robots-are-also-wizards-because-magic-is-always-cool/)

### Vergleichbare Spiele

- [Lethal Company](https://en.wikipedia.org/wiki/Lethal_Company), [Schedule I](<https://en.wikipedia.org/wiki/Schedule_I_(video_game)>), [Peak](<https://en.wikipedia.org/wiki/Peak_(video_game)>), [Content Warning](<https://en.wikipedia.org/wiki/Content_Warning_(video_game)>), [R.E.P.O.](https://en.wikipedia.org/wiki/R.E.P.O.), [Deep Rock Galactic](https://en.wikipedia.org/wiki/Deep_Rock_Galactic), [Risk of Rain 2](https://en.wikipedia.org/wiki/Risk_of_Rain_2), [Vermintide 2](https://en.wikipedia.org/wiki/Warhammer:_Vermintide_2), [Left 4 Dead](https://en.wikipedia.org/wiki/Left_4_Dead) (alle Wikipedia)
- Steam-Seiten: [Lethal Company](https://store.steampowered.com/app/1966720/Lethal_Company/), [Schedule I](https://store.steampowered.com/app/3164500/Schedule_I/), [R.E.P.O.](https://store.steampowered.com/app/3241660/REPO/)
- [AppMagic zu Koop-Hits](https://appmagic.rocks/blog/friendslop-steam-games-2025), [GameDiscoverCo: Wie zwei Koop-Hits ihr Publikum fanden](https://newsletter.gamediscover.co/p/how-two-friendslop-hits-got-the-audience)

### Freie Grafik und freier Sound

- [Kenney, Mini Dungeon](https://kenney.nl/assets/mini-dungeon), [Quaternius](https://quaternius.com/), [Universal Animation Library 2](https://quaternius.com/packs/universalanimationlibrary2.html)
- [Sonniss GDC-Pakete](https://sonniss.com/gameaudiogdc/), [Sonniss-Lizenz](https://sonniss.com/gdc-bundle-license/)

### KI und Steam

- [Haro: Three years of AI on Steam](https://fragwyz.substack.com/p/three-years-of-ai-on-steam)
- [Notebookcheck zur Steam-KI-Angabe](https://www.notebookcheck.net/Steam-updates-AI-disclosure-form-requiring-developers-to-report-visible-and-in-game-AI-but-not-background-tools.1206103.0.html), [Steam-Inhaltsumfrage](https://partner.steamgames.com/doc/gettingstarted/contentsurvey)
- Preise: [Claude](https://www.claude.com/pricing), [Meshy](https://www.meshy.ai/pricing)

### Recht

- [Video game clone](https://en.wikipedia.org/wiki/Video_game_clone), [Tetris Holding v. Xio](https://en.wikipedia.org/wiki/Tetris_Holding,_LLC_v._Xio_Interactive,_Inc.), [SAS Institute v. World Programming](https://en.wikipedia.org/wiki/SAS_Institute_Inc_v_World_Programming_Ltd), [Dark and Darker](https://en.wikipedia.org/wiki/Dark_and_Darker) (alle Wikipedia, Sekundärquellen)

### Technik und Plattform

- [First-Person-Template](https://dev.epicgames.com/documentation/unreal-engine/first-person-template-in-unreal-engine), [Netzwerk-Bewegung im Character Movement Component](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine), [Unreal Engine](https://en.wikipedia.org/wiki/Unreal_Engine) (Wikipedia)
- Steamworks: [Next Fest](https://partner.steamgames.com/doc/marketing/upcoming_events/nextfest), [Onboarding](https://partner.steamgames.com/doc/gettingstarted/onboarding)

### Einschränkungen

- **Epics Lizenzseite** war für mich gesperrt. Die Bedingungen stammen aus Wikipedia und müssen vor einem Verkauf in der aktuellen Lizenz geprüft werden.
- **Engine des Vorbilds:** Nur Fan-Wikis nennen Unreal Engine 5, offiziell ist das nicht bestätigt.
- **PC Gamer:** Bei mehreren Artikeln konnte ich nur die Überschrift lesen. Daraus stammen die Stichworte Yeehaw-Knopf und Magie.
- **Freie Pakete:** Bei Kenney habe ich nur die verlinkte Seite geprüft, Mixamo gar nicht. Die Lizenz prüfst du je Paket selbst.
- **KI-Daten:** Die Auswertung zeigt Zusammenhänge, keine Ursachen. Auch die Umsatzzahlen von AppMagic sind Schätzungen.
- **Stop Killing Games und Steam-Umsatzstaffel:** Die Angabe zu [Stop Killing Games](https://www.galaxus.ch/en/page/the-stop-killing-games-initiative-has-failed-in-the-eu-but-there-is-hope-43037) kenne ich nur aus Suchergebnissen. Die 30-%-Staffel von Steam ist Allgemeinwissen, nicht frisch geprüft.
- **Engine-Punkte und Rundenzahlen:** Das sind meine Einschätzungen ohne Messung. Die Rundenzahlen prüfst du nach den ersten Wochen an deinem echten Tempo.

### Offene Fragen an dich

- **Mitspieler:** Gibt es jemanden, der in Stufe 3 einen Abend lang mitspielt?
- **Sicherung:** Wo liegt dein privates Online-Repository?
