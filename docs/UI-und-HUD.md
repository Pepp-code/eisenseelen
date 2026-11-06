# UI- und HUD-Plan

Phillips Auftrag vom 6. November 2026: Plane ausführlich, wie UI und HUD gebaut werden, nach dem Vorbild Far Far West. Recherchiere noch einmal und überlege, was fehlt. Dieser Plan ist die Grundlage für die kommenden Arbeitsschritte.

## Was das Vorbild macht

Far Far West (recherchiert am 6. November 2026, wiki.gg und Guides):

- **Die Stadt ist das Menü.** Kein verschachteltes Hauptmenü: Der Saloon ist der Hub, und jede Station ist ein eigener Baukasten — Waffenschmied (Waffen kaufen und verbessern, permanente Joker), Labor (Helden-Joker), Zauberhütte (Zauber wechseln), Kosmetikhütte, Schießstand hinter der Schmiede, Bestiarium (Gegner anschauen und Abschusszahlen verfolgen), Grammophon (Sammel-Musik).
- **Ein Auftrag hat drei Teile:** Hauptziel, Kopfgeldboss und Karte. Am Dock wählt der Trackman die Mission — alle drei Teile sind von Anfang an frei wählbar.
- **Alles Koop zuerst:** Missionsbretter zeigen für jeden Spieler, was er nimmt; die Gruppe reist gemeinsam; im Spiel sieht man immer, wo die anderen sind.
- **Leveln über drei Spuren** (Jäger, Ausrüstung, Zauber), Prestige ab 100 mit Token für zusätzliche Slots.
- **Was dort gut ist:** Der Hub macht jedes Menü zu einem Ort. Nichts ist mehr als zwei Schritte entfernt. Die Kartenansicht ist top-down mit Landmarken und schnellem Reisen.

Bei uns ist die Anlegestelle mit der Barke der Saloon: Jagdbrett (jetzt mit Gebieten), Schmiede, Ausrüstung, Gravuren, Beutebuch. Der Jagdbrief entspricht der dreiteiligen Mission: Boss (Brief), Gebiet (steht jetzt auf dem Brief), Nebenziele (Verträge). Was wir bewusst nicht übernehmen: Prestige, Kosmetik-Händler, Grammophon, schnelles Reisen (unsere Karte ist ein Gebiet pro Lauf, kein offenes Reisen).

## Grundsätze für Eisenseelen

1. **Koop zuerst:** Jede Anzeige, die den eigenen Zustand zeigt, gibt es auch für die Mitspieler. Was einer wählt (Brief, Rune, Waffe), sehen alle.
2. **Ohne Bildschirm testbar:** Menüs bauen nur Listen von Elementen (Text, Balken, Knöpfe mit Klick-Wirkung), das HUD zeichnet sie. Tests klicken die Knöpfe direkt, ohne Fenster. Dieses Muster (Reiter Ausrüstung, Schmiede, Gravuren, Beutebuch) gilt für alles Neue.
3. **Alles datengetrieben:** Jeder Text und jede Zahl, die sich ändern kann, steht in `Data/*.json` — Seiten, Reiter und Kosten ebenso.
4. **Platz für echte Assets:** Jede Anzeige bekommt einen festen „Bild-Platz" (Icon-Slot) mit Namen. Heute zeichnet das HUD dort Text oder einen farbigen Kasten; später legt Phillip an dieselben Stellen 2D-Bilder oder 3D-Modelle, ohne dass Logik oder Tests sich ändern. So wie die Karte schon für echte 3D-Modelle vorbereitet wird, wird es die UI auch.
5. **Lesen statt raten:** Jeder Wert im Spiel (Leben, Schaden, Preise, Boni) muss irgendwo lesbar sein — im Zweifel im Tooltip.

## Was heute gebaut ist

**Im Lauf (HUD):** Steuerungshilfe (oben), Klassen-Zeile mit Level, Trefferblitz, Run-Uhr mit Blutmond-Hinweis, Nebenaufträge-Zähler, Beutebörse (Gold und Punkte des Laufs), Vertragsliste mit Abständen, Boss-Anzeige (Name, Bannbruch-Fortschritt, Leben), F-Hinweise am Zentrum, Treffermarkierungen in der Welt (Verträge, Signalfeuer, Barke, Boss), Richtungsanzeige des letzten eigenen Treffers, Gefahrenliste (Bedrohungen), Fadenkreuz, Parade-/Block-Meldung, Aufladebalken, Leben (mit Erholung grün), Mana, Q-Zauber- und E-Runen-Balken mit Namen, Tempo-Anzeige, Gefallen-Hinweis, Abrechnung nach dem Lauf, Überfahrt-Bild mit Gebietsnamen.

**Karte mit M:** Top-down-Rechteck mit Verträgen (nummeriert), Signalfeuer, Boss und eigenem Standort samt Blickrichtung; daneben die Liste mit Abständen.

**An der Anlegestelle (Menü I und F):** Reiter Ausrüstung (mit Vergleich), Schmiede (Level-gestufte Angebote), Gravuren (Waffe in Stufen mit Auswahl), Beutebuch. Bretter am Ort: Jagdbrett (Briefe mit Gebiet, Boss, Bannbruch; F wechselt), Schmiede, Vertragsbrett (Verträge des Laufs), Truhen. Die Testarena ist unser Schießstand.

## Was fehlt — die vollständige Liste

### A. Im Lauf

| # | Element | Was es tun soll | Warum |
| --- | --- | --- | --- |
| A1 | Mitspieler-Rahmen | Kleine Leisten oben: Name, Klasse, Leben, Mana, Zustand (brennt/verlangsamt), Gefallen | Koop-Kern; heute sieht man von den anderen nichts |
| A2 | Schadenszahlen | Kurze Zahlen über getroffenen Gegnern, nur eigene Treffer (entschieden am 6. November 2026), in den Einstellungen abschaltbar; Reaktionen farbig | Gefühl für Wirkung der Builds |
| A3 | Beute-Meldungen | Eingenommenes Gold/Punkte/Zutaten als kurze Zeile unten, wiederholbar | Heute sieht man Beute erst in der Abrechnung |
| A4 | Blutmond-Uhr | Countdown, wenn der Blutmond noch nicht da ist („Blutmond in 1:20") | Entscheidungsgrund, zu fliehen oder zu bleiben |
| A5 | Vertragsfortschritt | Statt nur „2 von 4": je Vertrag eine Zeile mit Ziel und Abstand im Haupt-HUD (kompakt) | Wegfinden ohne Karte |
| A6 | Boss-Leiste am oberen Rand | Name, Lebensbalken, Bannbruch-Zähler dauerhaft, wenn er wacht | Heute stehen die Zahlen nur als Textzeilen |
| A7 | Zustands-Anzeige am Gegner | Brennend/Verlangsamt/Gefroren als kleines Zeichen über dem Gegner | Reaktionen sehen und planen |
| A8 | Levelaufstieg | Großer kurzer Hinweis mit dem, das neu freigeschaltet wurde | Belohnungsgefühl |
| A9 | Reaktionsauslösung | Kurzes farbiges Aufblitzen, wenn eine Element-Reaktion zündet | Verständnis fürs Kombinieren |
| A10 | Einstellungen im Lauf | Esc öffnet Pause mit Fortsetzen/Neustart/Verlassen | Heute führt R direkt zum Neustart |

### B. Karte (M)

| # | Element | Was es tun soll |
| --- | --- | --- |
| B1 | Legende | Was die Farben und Buchstaben bedeuten, immer sichtbar |
| B2 | Lager und Truhen | Graue Punkte für Lager, aber nur entdeckte (entschieden am 6. November 2026: nur, was man gesehen hat), dazu Truhen der Leeren Wacht |
| B3 | Mitspieler | Standorte der anderen mit Blickrichtung |
| B4 | Barke | Standort, wenn sie da ist |
| B5 | Gebietsgrenzen | Die gebiete aus laufkarte.json als Kreise mit Namen |
| B6 | Karte schwenken | Pfeiltasten/Maus bewegen die Karte, Mausrad zoomt |

### C. An der Anlegestelle (Menüs)

| # | Element | Was es tun soll |
| --- | --- | --- |
| C1 | Stat-Vergleich beim Kaufen | Vorher/nachher bei jedem kaufbaren Teil (Schmiede und Gravuren), wie im Reiter Ausrüstung |
| C2 | Beutebuch vertiefen | Zutaten mit Herkunfts-Boss und Chance in Worten (steht schon in bosse.json), Gegner-Abschüsse als Bestiarium-Teil |
| C3 | Meisterstücke-Ansicht | Später (Schritt G): Slot je Waffe, Zutatenbedarf sichtbar |
| C4 | Steuerungsübersicht | Eine eigene Seite „Hilfe" mit allen Tasten, auch im Menü erreichbar |
| C5 | Profil-Zahlen | Gold, Punkte je Fraktion, Level, Erfahrungsbalken auf einen Blick im Menü |

### D. Hauptmenü und Einstellungen

| # | Element | Was es tun soll |
| --- | --- | --- |
| D1 | Titelbild | Kurzes Titel mit „Start", „Testarena", „Beenden" — heute startet das Spiel direkt |
| D2 | Einstellungen | Lautstärke (Musik/Effekte), Maus-Empfindlichkeit, invertieren, HUD-Größe, HUD an/aus |
| D3 | Pause-Menü | Esc: Fortsetzen, Steuerung anzeigen, Einstellungen, Neustart, Verlassen |
| D4 | Speicherstand | Profil-Anzeige (Name/Level), später mehrere Speicherplätze |
| D5 | Tutorial | Der erste Start zeigt die Steuerung als kurze Liste (die heutige Hilfe-Zeile ausgebaut) |

### E. Koop-Komfort (später, nach dem Netzteil)

| # | Element | Was es tun soll |
| --- | --- | --- |
| E1 | Bereit-Anzeige | Wer an der Barke steht und bereit ist |
| E2 | Team-Marker | Ein Spieler markiert einen Ort, alle sehen die Weltmarkierung |
| E3 | Verbindungsstatus | Anzeige, wenn ein Spieler weg ist oder der Lauf für ihn stockt |

## Stil und Assets

Der heutige Stil ist der „graue Klotz": schwarze halbtransparente Flächen, weiße Schrift, farbige Balken. Der Plan: **erst Funktion, dann Optik.** Alles oben entsteht im Klotz-Stil mit Icon-Platzhaltern (farbige Vierecke mit Kurztext). Wenn die Elemente stehen, folgt die optische Schicht als eigene Aufgabe — als 2D-Assets, die an die vorbereiteten Bild-Plätze gelegt werden. Nichts an der Logik ändert sich dabei.

Das genaue Design („geschmiedetes Eisen" statt Western-Holz, Bildschirm-Anker, der komplette Animations-Katalog, Icon-Platz-System, Umsetzungs-Schritte ST1–ST8) steht ausgearbeitet in `docs/UI-Stil.md`.

## Umsetzung in Schritten

Jeder Schritt ist ein eigener Auftrag mit grünen Tests am Ende. Reihenfolge nach Nutzen:

- **U1 — Pause und Einstellungen (D2, D3, A10, C4):** Esc-Menü, Einstellungen in einer json, Steuerungsseite. Fundament für alles Weitere; ohne Esc landet man heute beim harten Neustart.
- **U2 — Mitspieler-Rahmen (A1):** Der GameState kennt bald alle Helden; die Leisten zeichnen davon ab. Erster Koop-Baustein.
- **U3 — Schadenszahlen und Meldungen (A2, A3, A8, A9):** Zahlen und Beutezeilen als kurze Einblendungen; ein „Meldungs-Dienst" im GameState sammelt sie, das HUD zeigt sie an. Auch als Test prüfbar (Liste statt Fenster).
- **U4 — Run-Anzeigen verdichten (A4, A5, A6):** Blutmond-Countdown, Vertragszeilen, Boss-Leiste.
- **U5 — Karte ausbauen (B1 bis B5):** Legende, Lager, Mitspieler, Gebietsgrenzen aus laufkarte.json.
- **U6 — Menü-Tiefe (C1, C2, C5):** Vergleich beim Kauf, Beutebuch-Boss-Angaben, Profilzahlen.
- **U7 — Titel und Tutorial (D1, D5):** Titelbild und erste Hilfe.
- **U8 — Optik-Schicht:** Bild-Plätze befüllen, Einblendungen, Rahmen. Erst wenn U1 bis U5 stehen.

Tests: Menü-Inhalte nach dem Elementlisten-Muster (Knöpfe klicken, Zustand prüfen); HUD-Werte über den GameState/Meldungs-Dienst; Karten-Inhalte als reine Daten (was gezeigt wird, nicht wie).

## Entscheidungen vom 6. November 2026

- **Schadenszahlen:** Es werden nur eigene Treffer angezeigt, in den Einstellungen abschaltbar (A2).
- **Karte:** Sie zeigt nur entdeckte Lager, nicht alle (B2).
- **Rüstungs-Gravuren** (siehe `docs/Ruestungs-Gravuren.md`): ein Baum je Platz, 2 bis 3 zur Wahl je Stufe, Meisterstücke für Waffe, Brust und Handschuhe, Kosten bleiben hoch.

## Offene Fragen

1. Soll das Titelbild (D1) schon vor dem ersten Spielen da sein, oder erst, wenn Freunde mitspielen können?
2. Maus im Menü: Heute klickt das Menü mit der Maus. Soll alles zusätzlich per Tasten gehen (Pfeile + Enter), damit später Controller möglich bleiben?
