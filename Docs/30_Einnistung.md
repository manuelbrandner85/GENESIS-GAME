# 30 – Einnistung und Keimscheibe (die zweite Woche)

*GENESIS-040. Carnegie-Stadien 5 und 6, Tag 6 bis 13 nach der Befruchtung.*

Die erste Woche endet damit, dass der Keim aus der Zona schlüpft. Vorher hieß „Einnistung" im Spiel nur, dass eine
Zahl in 48 Stunden von 0 auf 1 stieg. Jetzt durchläuft der Keim die zweite Woche so, wie sie die Embryologie
beschreibt: Er versinkt in der Schleimhaut, erreicht das Blut der Mutter und legt die zweiblättrige Keimscheibe an.
Aus der Keimscheibe entsteht der ganze Mensch.

| Bereich | Stand |
|---|---|
| Simulation der zweiten Woche (`GenesisEmbryoLogic`) | **PRODUCTION READY**: getestet gegen Lehrbuch und klinische Daten |
| Anzeige im Durchlauf (Uhr, Größe, hCG, Satz je Stufe), Entwicklerseite | **BETA**: noch zu der Szene der ersten Woche |
| Szene: Schleimhaut, Einsinken, Lakunen mit Blut | **PLAN** (Teil 2) |

## Die Stufen

| Stufe | Tag (mittlerer Keim) | Was geschieht |
|---|---|---|
| Anlagerung | 6 | Der geschlüpfte Keim (0,2 mm) legt sich mit dem Pol des Embryoblasten an die Schleimhaut. |
| Anheftung | 6,5 | Haftmoleküle halten ihn fest (L-Selektin, Integrine), er rollt nicht mehr ab. |
| Invasion | 7–8 | Der Trophoblast teilt sich in den Zytotrophoblasten (innen, teilt sich) und den Synzytiotrophoblasten (außen, vielkernig, ohne Zellgrenzen). Das Synzytium löst sich einen Weg ins Stroma. Innen trennen sich Epiblast und Hypoblast, die zweiblättrige Keimscheibe. An Tag 8 öffnet sich die Amnionhöhle. Das Synzytium bildet hCG. |
| Lakunenstadium | 9 | Im Synzytium öffnen sich Lakunen. Primärer Dottersack mit Heuser-Membran. Der Keim ist fast ganz versunken, ein Fibrinpfropf verschließt die Eintrittsstelle. |
| ganz eingebettet | 10 | Der Keim liegt ganz in der Schleimhaut, die Oberfläche wächst über ihm wieder zu (bis Tag 12). |
| uteroplazentarer Kreislauf | 11–12 | Das Synzytium öffnet die erweiterten Kapillaren der Mutter. Ihr Blut fließt in die Lakunen und durch sie hindurch: der erste gemeinsame Kreislauf. Extraembryonales Mesoderm, darin öffnet sich die Chorionhöhle. |
| Primärzotten | 13 | Säulen aus Zytotrophoblast wachsen ins Synzytium. Der sekundäre Dottersack ersetzt den primären. Der Keim ist eingenistet, und die Körpersimulation übernimmt. |

Quellen der Zeiten: Langman „Medizinische Embryologie", Moore „The Developing Human", die Hertig-Rock-Präparate
(Carnegie-Sammlung).

**Jeder Keim hat seine eigene Uhr.** Die Stufen folgen der Uhr des mittleren Keims, verschoben um den Zeitpunkt, zu dem
sich dieser Keim tatsächlich anlegte. Er treibt nach dem Schlüpfen 4 h frei. Wer später schlüpft, ist an jeder Stufe
später dran. Alle Größen rechnen sich aus dieser Uhr, nicht Schritt für Schritt. Damit ist das Ergebnis dasselbe, ob in
einem Rutsch, in krummen Schritten oder nach dem Laden gerechnet wird (Test `SecondWeekStepIndependent`).

## Die Größen

| | Tag 7,5 | Tag 9 | Tag 10 | Tag 12 | Tag 13 |
|---|---|---|---|---|---|
| Keim (Durchmesser) | 0,26 mm | 0,33 mm | 0,45 mm | 0,9 mm | 1,2 mm |
| versunken | 20 % | 80 % | 100 % | 100 % | 100 % |
| Synzytium am Embryonalpol | 20 µm | 60 µm | 75 µm | 105 µm | 120 µm |
| Lakunen / mit Blut | – | 4 / nein | 26 / nein | 40 / ja | 40 / ja |
| Keimscheibe | 80 µm | 110 µm | 130 µm | 175 µm | 200 µm |
| hCG im Blut der Mutter | 1,7 mIU/ml | 3,8 | 6,5 | 19 | 33 |

Die Keimscheibe hat an Tag 13 776 Epiblast- und 272 Hypoblastzellen (Keim 21 im Test). Die Zellen teilen sich wieder
etwa einmal am Tag. Aus dem Epiblasten wird alles, was später der Mensch ist, einschließlich der Keimzellen.

## hCG: das erste Signal an die Mutter

Das Synzytium bildet humanes Choriongonadotropin. hCG hält den Gelbkörper am Leben, sonst bliebe die Regel nicht aus.
Im Modell ist hCG ab Tag 9,5 im Blut messbar (5 mIU/ml, Labortest). Es verdoppelt sich alle 31 Stunden, in 48 Stunden
×2,9. Barnhart (2004) nennt für eine intakte frühe Schwangerschaft mindestens +53 % in zwei Tagen, typisch ist eine
Verdopplung in 1,3–2 Tagen. Ein Urintest (25 mIU/ml) schlägt um Tag 12,5 an, rund zum Termin der ausgebliebenen Regel.
Nach der Einnistung steigt hCG weiter.

## Das Risiko: der Zeitpunkt entscheidet

Wilcox, Baird und Weinberg (1999, NEJM) haben bei 189 Schwangerschaften täglich hCG gemessen. Gemessen am Tag der
Einnistung nach dem Eisprung endeten früh:

| Einnistung | bis Tag 9 | Tag 10 | Tag 11 | später |
|---|---|---|---|---|
| frühes Ende | 13 % | 26 % | 52 % | 82 % |

Ein spät ankommender Keim trifft auf eine Schleimhaut, deren Empfänglichkeitsfenster sich schließt. Im Spiel wird beim
Anheften einmal gewürfelt. Das Risiko ergibt sich aus dem Tag nach Wilcox, verschoben durch die Entwicklungsqualität des
Keims (×1,4 bei schlechter, ×0,7 bei bester Qualität). Wer scheitert, scheitert dort, wo das Synzytium mütterliches Blut
erreichen müsste (Tag 11–12). Das hCG fällt dann auf null. Der mittlere Keim nistet an Tag 9 nach dem Eisprung ein, dem
Median bei Wilcox.

Gemessen im Test (je 1000 Keime): Legt sich der Keim an Tag 6 an, scheitern 13,8 % (Risiko 14,5 %). Zwei Tage später
scheitern 58,6 % (Risiko 57,9 %). **Der Keim des Spielers scheitert nie.**

## Im Durchlauf

- Ab dem Schlüpfen zeigt die Uhr nicht mehr die Zellen, sondern Tag, Größe des Keims und hCG, dazu einen Satz je Stufe
  („Tag 11–12 – das Synzytium öffnet die Kapillaren der Mutter …").
- Die Phase im Menü heißt jetzt „Die ersten zwei Wochen".
- Der Zeitraffer bleibt bei 10 h je Sekunde: Die zweite Woche dauert 17 s. Der ganze Abschnitt vom Zygoten bis zum
  eingenisteten Keim dauert 100 s (Test `EmbryoTimeLapse`, höchstens 120 s).
- Entwicklerseite „Embryo" (`genesis.Debug`): Stufe, Tiefe, Größe, Synzytium, Lakunen, Blut, Keimscheibe, Höhlen, Zotten,
  hCG, Risiko. Jeder Stufenwechsel steht im Log.

## Tests

- `Genesis.Embryo.SecondWeek`: jede Stufe an ihrem Tag, Höhlen in der richtigen Reihenfolge, der Anstieg von hCG,
  Größen nach Carnegie 6.
- `Genesis.Embryo.SecondWeekStepIndependent`: dasselbe Ergebnis bei jeder Schrittweite; ein später Keim ist später dran.
- `Genesis.Embryo.ImplantationRisk`: die Tabelle von Wilcox, 1000 Keime je Zeitpunkt; der Keim des Spielers scheitert nie.
- `Genesis.Embryo.Timeline`: Tag 10 ganz versunken, Tag 14 eingenistet. Vorher hieß es „Tag 10 eingenistet", nach der
  alten 48-h-Rechnung.

## Qualitätsprüfung (Teil 1)

| | |
|---|---|
| SCALE | Größen nach Hertig-Rock und Carnegie; 1 µm = 1 Einheit wie in der ganzen Mikrowelt |
| PHYSICS / Biologie | Zeiten nach Lehrbuch, hCG nach Barnhart, Risiko nach Wilcox, jeweils im Test geprüft |
| GAMEPLAY | Der Tag der Einnistung folgt aus der ersten Woche: Wie gut sich der Keim teilte, bestimmt, wann er ankommt und ob er bleibt |
| PERFORMANCE | Rechnet jede Größe direkt aus der Uhr; in der zweiten Woche teilen sich keine einzelnen Zellen mehr |
| VISUAL | offen, Szene folgt in Teil 2 |

## Offen

- Szene der zweiten Woche: Die Schleimhaut der Gebärmutter fehlt. Der Keim ist noch im Eileiter zu sehen. Biologisch
  gelangt er schon als Morula (Tag 4) in die Gebärmutter.
- Der Durchlauf hält bei gescheiterter Einnistung nur an. Eine eigene Szene dafür fehlt, und für den Spieler kommt sie
  nicht vor.
