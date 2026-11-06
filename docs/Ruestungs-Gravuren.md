# Rüstungs-Gravuren nach dem Deep-Rock-Galactic-Prinzip

Phillip hat am 6. November entschieden: Das Verbessern von Waffen und Rüstung soll so funktionieren wie bei Deep Rock Galactic. Für die Waffen ist das gebaut (siehe Items-und-Builds.md, „Gravuren und Meisterstücke"). Dieser Plan holt die Rüstung auf dasselbe Prinzip und ersetzt den kurzen Absatz aus dem alten Entwurf („24 Zeilen, 3 Stufen, je Platz").

## Das Prinzip bei Deep Rock Galactic

So macht es das Vorbild, recherchiert am 6. November 2026:

- **5 Stufen je Waffe**, je 2 bis 3 Verbesserungen zur Wahl. Pro Stufe ist genau eine aktiv.
- **Die nächste Stufe öffnet sich erst**, wenn in der vorherigen etwas gekauft wurde. Man steigt also Stufe für Stufe auf, nichts kann man überspringen.
- **Gekauft ist gekauft:** Zwischen den Einsätzen wechselt man kostenlos zwischen allen gekauften Optionen. Kein Respec, keine Strafe fürs Ausprobieren.
- **Zwei Zahlungsmittel:** Credits (bei uns Gold) und Mineralien (bei uns Fraktionspunkte). Die Kosten steigen mit jeder Stufe, von früh (~480 Credits, 20–40 Mineralien) bis spät (über 2.000 Credits, über 120 Mineralien).
- **Überclocks als Krönung über den 5 Stufen:** ein sechster, besonderer Slot mit drei Sorten — Sauber (nur Bonus), Ausgewogen (größerer Bonus mit Nachteil), Riskant (großer Bonus mit großem Nachteil). Sie kosten eine seltene, langsam verdiente Ressource und sind dauerhaft.

Bei uns heißt das: Gravuren (Stufen 1–5), Meisterstücke (der Krönungs-Slot), Gold und Fraktionspunkte (die Zahlungsmittel), seltene Zutaten von Bossen (die seltene Ressource). Die Schmiede an der Anlegestelle ist unsere Fassung der DRG-Schmiede.

## Der eine Unterschied: Wem gehört der Baum?

Bei DRG gehört der Ausbau jeder einzelnen Waffe. Bei uns haben Waffen eine Zwischenebene: Alle Exemplare einer Grundform teilen sich die Gravuren (Langschwert-Gravuren gelten für jedes Langschwert), gekauft wird pro Exemplar.

Für die Rüstung gibt es drei Möglichkeiten:

1. **Ein Baum je Platz, gehört dem Helden** (empfohlen): Helm, Brustpanzer, Handschuhe und Schuhe haben je einen eigenen Baum mit 5 Stufen. Was du kaufst, gilt für jedes Teil, das du an diesem Platz trägst — Eisenwacht-Helm oder Frostwacht-Helm. Die Set-Boni der Art bleiben trotzdem der Grund, die Art zu wechseln; die Gravuren machen dich unabhängig davon stärker.
2. Ein Baum je Art (10 Bäume): alle vier Teile einer Art zusammen. Wer die Art wechselt, verliert den Fortschritt — hart, aber am nächsten an „pro Waffe kaufen" der Einzelteile.
3. Ein Baum je einzelner Teil (40 Bäume): am nächsten an DRG, aber unbezahlbar — ein Vollausbau käme auf rund 260.000 Gold.

**Empfehlung ist Möglichkeit 1**, aus drei Gründen: Die Gesamtkosten bleiben im Bereich von vier Waffen (spielbar), der Spieler muss nicht denselben Baum vierzigmal bezahlen, und die Baustelle im Menü bleibt übersichtlich (vier Bäume, nicht vierzig). Bei den Waffen gilt dasselbe Prinzip schon: Der Baum gehört der Grundform, nicht dem einzelnen gekauften Exemplar — nur dass dort pro Exemplar gekauft wird, weil man nur eine Waffe trägt.

Kosten der Punkte: Rüstungs-Gravuren kosten Gold und Punkte der Fraktion, zu der das Teil gehört, das du beim Kauf an dem Platz trägst. Wer also in der Frostwacht kauft, zahlt Frostpunkte — genau wie Feuerwaffen Feuerpunkte kosten.

## Die vier Bäume

Jeder Baum hat wie die Waffen 5 Stufen mit je 2 (selten 3) Gravuren, von denen eine aktiv ist. Die Stufen 1 bis 4 verbessern vorhandene Werte, die Stufe 5 ändert wie bei den Waffen eine Regel. Alle Zahlen sind erste Annahmen und stehen dann in `Data/gravuren.json` zum Ändern.

### Helm — Zauber und Sinne

| Stufe | Thema | Zur Wahl | Kosten |
| --- | --- | --- | --- |
| 1 | Zauber | Runenkranz: E +15 % Schaden, oder Klarer Kranz: E 15 % schneller bereit | 200 Gold, 10 Punkte |
| 2 | Mana | Kristallkranz: +25 Mana, oder Sparsamer Kranz: −10 % Manakosten | 350 Gold, 15 Punkte |
| 3 | Zustände | Tiefer Kranz: Zustände 25 % länger, oder Weißer Kranz: +15 % Zaubermacht | 500 Gold, 20 Punkte |
| 4 | Reaktion | Zeitkranz: 0,5 s mehr Reaktionsfenster, oder Funkelkranz: +20 % Reaktionsschaden | 700 Gold, 30 Punkte |
| 5 | Neue Regel | Klare Stimme: Q wirkt bei vollem Mana 25 % stärker, oder Zweitgedanke: Q 20 % schneller bereit, dafür +10 % Manakosten aller Zauber | 1.000 Gold, 40 Punkte |

### Brustpanzer — Leben und Rüstung

| Stufe | Thema | Zur Wahl | Kosten |
| --- | --- | --- | --- |
| 1 | Leben | Gefüttert: +40 Leben, oder Gehärtet: +5 % Rüstung | 200 Gold, 10 Punkte |
| 2 | Erholung | Ruhige Brust: Leben-Erholung setzt 1 s früher ein, oder Dickes Fell: +8 % Rüstung | 350 Gold, 15 Punkte |
| 3 | Widerstand | Brennfeste Platte: −25 % Zustandsschaden, oder Stoßdämpfer: −20 % Schaden von schweren Hieben | 500 Gold, 20 Punkte |
| 4 | Standhaft | Fester Stand: weniger Rückstoß durch Treffer, oder Zäher Kern: unter 30 % Leben 20 % weniger Schaden | 700 Gold, 30 Punkte |
| 5 | Neue Regel | Letzter Atem: einmal je Lauf tödlicher Schaden lässt dich mit 25 % Leben weiterkämpfen, oder Adlerbrust: +15 % Leben, aber 1 s länger keine Leben-Erholung | 1.000 Gold, 40 Punkte |

### Handschuhe — Angriff und Block

| Stufe | Thema | Zur Wahl | Kosten |
| --- | --- | --- | --- |
| 1 | Angriff | Feste Hand: +8 % Angriffskraft, oder Schnelle Hand: +8 % Angriffstempo | 200 Gold, 10 Punkte |
| 2 | Block | Breiter Griff: +10 % Blockstärke, oder Wuchtiger Griff: schwerer Hieb lädt 20 % schneller | 350 Gold, 15 Punkte |
| 3 | Treffer | Scharfer Daumen: +25 % Schaden gegen taumelnde Gegner, oder Langer Griff: +10 % Angriffsreichweite | 500 Gold, 20 Punkte |
| 4 | Zauber | Zaubertatze: Q +15 % Schaden, oder Flinke Tatze: Q 15 % schneller bereit | 700 Gold, 30 Punkte |
| 5 | Neue Regel | Doppelschwung: der schwere Hieb trifft in einem kleinen Kreis statt nur vor dir, oder Eiserne Faust: Parieren macht den nächsten Hieb sofort bereit, aber −10 % Angriffstempo | 1.000 Gold, 40 Punkte |

### Schuhe — Tempo und Bewegung

| Stufe | Thema | Zur Wahl | Kosten |
| --- | --- | --- | --- |
| 1 | Tempo | Leichte Sohlen: +5 % Tempo, oder Feste Sohlen: +2 % Rüstung | 200 Gold, 10 Punkte |
| 2 | Ausdauer | Läufersohlen: Sprünge weiter, oder Stille Sohlen: Gegner bemerken dich 20 % später | 350 Gold, 15 Punkte |
| 3 | Trittfest | Rutschfeste Sohlen: weniger Rutschen auf Eis und im Wind, oder Sprintsohlen: nach 2 s Laufen +10 % Tempo | 500 Gold, 20 Punkte |
| 4 | Reaktion | Flinke Sohlen: nach einer Parade 1 s +20 % Tempo, oder Flinker Rückzug: nach einem Treffer kurz +15 % Tempo | 700 Gold, 30 Punkte |
| 5 | Neue Regel | Windläufer: Wind und Rückstoß stoßen dich nicht mehr zurück, dafür −10 Leben, oder Doppelschritt: einmal je 12 s ein kurzer Sprung nach vorn in Blickrichtung | 1.000 Gold, 40 Punkte |

Alle vier Bäume zusammen kosten so etwa 26.000 Gold und 460 Punkte — wie vier Waffen im Vollausbau. Ein voll ausgebauter Held (Waffe + Rüstung) investiert damit in etwa 20 bis 25 gute Läufe, das ist bewusst ein langfristiges Ziel. Die Preise stehen in `Data/gravuren.json` und lassen sich jederzeit senken.

## Meisterstücke für die Rüstung

Bei den Waffen trägt jedes Exemplar ein Meisterstück (Sauber / Ausgewogen / Riskant, 1.500 Gold, 80 Punkte, eine seltene Zutat). Für die Rüstung empfiehlt dieser Plan: **nur der Brustpanzer bekommt ein Meisterstück**, das „Herzstück" der Rüstung. Vier Meisterstück-Slots je Held würde die seltene Zutat entwerten, und die Brust ist der Platz der großen Regel (Stufe 5). Beispiele:

- **Sauber:** Herz aus Eisen — +10 % Rüstung, kein Nachteil.
- **Ausgewogen:** glühendes Herz — +20 % Leben, aber 1 s später beginnt die Leben-Erholung.
- **Riskant:** Herz der Leere — +30 % Schaden, aber −25 % Leben.

Ob Handschuhe, Helm und Schuhe später auch Meisterstücke bekommen, bleibt offen (siehe unten).

## Umsetzung in Schritten

- **R1 — Daten:** `Data/gravuren.json` bekommt neben den Waffen-Grundformen einen Abschnitt `ruestung` mit den vier Bäumen. Struktur wie bei den Waffen, damit der vorhandene Parser fast unverändert bleibt.
- **R2 — Fortschritt:** Das Profil speichert Gravuren-Fortschritt je Platz (bisher je Waffe). Die Menü-Seite „Gravuren" zeigt oben: getragene Waffe und die vier Plätze; Auswahl schaltet um, darunter der bekannte Stufen-Baum.
- **R3 — Wirkung:** Die Gravuren wirken über die vorhandenen Werte (Leben, Rüstung, Tempo, Manakosten, Reaktionsfenster ...). Neue Regeln der Stufe 5 brauchen je ein paar Zeilen Code, wie bei den Waffen-Regeln der Stufe 5.
- **R4 — Meisterwerk:** Zusammen mit den Waffen-Meisterstücken (Schritt G), derselbe Ablauf in der Schmiede.
- **R5 — Tests:** Kauf, Wechseln, Wirkung je Baum nach dem Muster der Waffen-Gravuren-Tests; die Bäume sind reine Daten, also laufen die Structure-Checks über JSON.

Zeitlich passt R1 bis R3 in einen Arbeitsschritt, R4 liegt bei Schritt G (Meisterstücke). Der alte Entwurf (3 Stufen, 24 Zeilen) wird damit ersetzt.

## Offene Fragen

1. Bäume je Platz und Held (Empfehlung), je Art oder je einzelner Teil?
2. Reichen zwei Auswahlmöglichkeiten je Stufe, oder sollen einzelne Stufen drei haben wie bei den Waffen?
3. Soll es außer dem Brustpanzer später Meisterstücke für weitere Plätze geben?
4. Passt die Gesamtrechnung (rund 26.000 Gold für die Rüstung, etwa wie vier Waffen)?
