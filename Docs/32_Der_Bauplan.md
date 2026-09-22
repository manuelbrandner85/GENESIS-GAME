# 32 – Der Bauplan: die dritte und vierte Woche

*GENESIS-041. Carnegie-Stadien 7 bis 13, Tag 15 bis 28 nach der Befruchtung.*

Bis zur zweiten Woche ist der Keim eine Scheibe aus zwei Blättern (Docs/30). In der dritten Woche bekommt er einen
Bauplan: vorn und hinten, links und rechts, oben und unten. In der vierten schlägt sein Herz.

| Bereich | Stand |
|---|---|
| Simulation der dritten und vierten Woche (`GenesisEmbryogenesisLogic`) | **PRODUCTION READY**: gegen Lehrbuch und Ultraschalldaten geprüft |
| Anzeige im Durchlauf (Tag, Länge, Somiten, Herzschlag), Entwicklerseite | **BETA** |
| Szene: die Keimscheibe, das Neuralrohr, das schlagende Herz | **PLAN** (Teil 2) |

## Die Stufen

| Stufe | Tag | Was geschieht |
|---|---|---|
| Primitivstreifen | 15 | Am hinteren Rand der Scheibe zieht sich eine Furche. Von jetzt an hat der Keim vorn und hinten, links und rechts. |
| Gastrulation | 16–17 | Zellen wandern durch den Streifen nach innen. Aus zwei Blättern werden drei: Entoderm (Darm, Lunge), Mesoderm (Muskeln, Knochen, Herz, Nieren), Ektoderm (Haut, Nerven). |
| Chorda | 17–19 | Die Chorda spannt sich als Achse durch die Scheibe; über ihr verdickt sich das Ektoderm zur Neuralplatte. |
| Neuralplatte | 19–20 | Die Ränder der Platte heben sich zur Neuralrinne. |
| Somiten | 20–21 | Rechts und links der Achse erscheinen Somitenpaare – eines alle sieben Stunden. Aus ihnen werden Wirbel, Rippen, Muskeln und Lederhaut. |
| **Der erste Herzschlag** | **22** | Aus zwei Herzschläuchen ist einer geworden, und er zieht sich zusammen. |
| Neuralrohr geschlossen | 24–28 | Das Rohr schließt in der Mitte zuerst und läuft wie ein Reißverschluss nach vorn und hinten. Vorderer Neuroporus zu an Tag 25, hinterer an Tag 27. |
| Ende der vierten Woche | 28 | Ein gekrümmter Embryo von 4–5 mm mit rund 30 Somitenpaaren, vier Kiemenbögen, Augenbläschen und Extremitätenknospen. |

Alle Größen rechnen sich aus dem Tag nach der Befruchtung, nicht Schritt für Schritt – dasselbe Ergebnis bei jeder
Schrittweite und nach dem Laden (Test `BodyPlanStepIndependent`).

## Gemessen im Test

| | Tag 21 | Tag 25 | Tag 28 |
|---|---|---|---|
| Länge (Scheitel-Steiß) | 2,1 mm | 3,2 mm | 4,6 mm |
| Somitenpaare | 4 | 17 | 28 |
| Neuralrohr | beginnt zu schließen | vorn zu, hinten offen | geschlossen |
| Herz | noch still | 93/min | 111/min |
| Kiemenbögen | – | 1 | 4 |

Klinische Vergleichswerte: Tag 21 vier bis sechs Somitenpaare, Tag 25 rund 20, Ende der vierten Woche 26 bis 30;
Länge Ende Woche 4 vier bis fünf Millimeter. Die Herzfrequenz beginnt bei 70–80/min und steigt bis Woche 9 auf etwa
170/min (Ultraschall: Woche 5 ~100, Woche 6 ~120, Woche 7 ~150).

## Das Neuralrohr – und was die Mutter damit zu tun hat

Schließt sich ein Neuroporus nicht, bleibt er offen: vorn bedeutet das Anenzephalie (nicht mit dem Leben vereinbar),
hinten einen offenen Rücken (Spina bifida). Das betrifft rund eine von tausend Schwangerschaften. **Folat senkt das
Risiko um bis zu 70 Prozent, ein Mangel hebt es.** Deshalb hängt das Risiko im Spiel an der Ernährung der Mutter:

| Ernährung der Mutter | Risiko |
|---|---|
| schlecht (0,0) | 0,40 % |
| mittel (0,7) | 0,10 % |
| gut (1,0) | 0,03 % |

Gemessen über 4000 Keime bei schlechter Ernährung: 0,33 % mit offenem Neuralrohr (Risiko 0,36 %). Gewürfelt wird
einmal, wenn sich das Rohr zu schließen beginnt (Tag 21,5); was offen bleibt, bleibt offen. **Der Keim des Spielers
schließt sein Neuralrohr immer** – geprüft über 500 Durchläufe bei schlechtester Ernährung. Ob ein Leben mit offenem
Rücken spielbar sein soll, ist eine Entscheidung für später; die Simulation kann es bereits.

Das ist die erste Kette dieser Art im Spiel: **Wie die Mutter lebt, entscheidet mit, wie der Körper des Kindes wird.**

## Im Durchlauf

- Die Phase heißt jetzt „Die ersten vier Wochen" und endet nicht mehr mit der Einnistung, sondern wenn der Bauplan
  steht (`Signals.bBodyPlanDone`). Erst dann übernimmt die Körpersimulation die Schwangerschaft.
- Zeitraffer: 12 Stunden je Sekunde. Gemessen liegt der erste Herzschlag 18 Sekunden nach dem Beginn der dritten Woche,
  der fertige Bauplan nach 30 Sekunden (Test `Genesis.Slice.BodyPlanPace`).
- Die Anzeige zeigt Tag, Länge in Millimetern, Somitenpaare und – ab Tag 22 – den Herzschlag mit seiner Frequenz.
- Entwicklerseite „Embryo": Körperbau, Neuralrohr mit beiden Neuroporen, Herz, Kiemenbögen, Augen, Knospen,
  Defektrisiko.

## Tests

- `Genesis.Embryo.BodyPlanTimeline`: jede Stufe an ihrem Tag, Somiten im klinischen Takt, der erste Herzschlag an Tag 22.
- `Genesis.Embryo.NeuralTube`: das Risiko folgt der Ernährung, der Anteil über 4000 Keime folgt dem Risiko, der Keim des
  Spielers bekommt keinen Defekt, und was offen bleibt, schließt sich nicht nachträglich.
- `Genesis.Embryo.BodyPlanStepIndependent`: gleiches Ergebnis bei jeder Schrittweite.
- `Genesis.Slice.BodyPlanPace`: Tempo im Durchlauf.

## Offen

- **Die Szene fehlt** (Teil 2): Keimscheibe mit Primitivstreifen, die Neuralrinne, die sich schließt, Somitenpaare, die
  wie Perlen erscheinen, und das Herz, das zu schlagen beginnt. Bis dahin bleibt die Kamera auf der Einnistungsstelle.
  Ein erster Anlauf für den Körper steht als **Prototyp** in `Tools/Blender/Embryogenesis/build_embryo_body.py`: Die
  Maße stimmen (0,7 mm an Tag 16 bis 4,6 mm an Tag 28, Somitenzahl, Krümmung zur C-Form, Formschlüssel je Tag), die
  Form überzeugt aber nicht – die Somiten lesen sich als gezackter Kamm, der Kopf bleibt ein glatter Klumpen. Der
  nächste Weg steht im Kopf der Datei: die Anatomie aus impliziten Körpern aufbauen und die Topologie danach
  vereinheitlichen.
- Der Herzschlag soll hörbar werden (der Klang liegt im Körperklang-System bereit) und den Spieler von hier an
  begleiten.
- Die Ernährung der Mutter steht noch als Wert in der Regie (`MotherNutrition`), weil die Mutter noch keine eigene
  Person mit Körper ist.
