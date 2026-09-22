# 30 – Einnistung und Keimscheibe (die zweite Woche)

*GENESIS-040. Carnegie-Stadien 5 und 6, Tag 6 bis 13 nach der Befruchtung.*

Die erste Woche endet damit, dass der Keim aus der Zona schlüpft. Vorher hieß „Einnistung" im Spiel nur, dass eine
Zahl in 48 Stunden von 0 auf 1 stieg. Jetzt durchläuft der Keim die zweite Woche so, wie sie die Embryologie
beschreibt: Er versinkt in der Schleimhaut, erreicht das Blut der Mutter und legt die zweiblättrige Keimscheibe an.
Aus der Keimscheibe entsteht der ganze Mensch.

| Bereich | Stand |
|---|---|
| Simulation der zweiten Woche (`GenesisEmbryoLogic`) | **PRODUCTION READY**: getestet gegen Lehrbuch und klinische Daten |
| Anzeige im Durchlauf (Uhr, Größe, hCG, Satz je Stufe), Entwicklerseite | **PRODUCTION READY** |
| Szene: Gebärmutterschleimhaut, Einsinken, Pfropf, Blut (Teil 2) | **BETA**: im Bild geprüft; offene Punkte unten |

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


## Die Szene: die Schleimhaut der Gebärmutter

Nach dem Schlüpfen wechselt der Ort. Die erste Woche zeigt das Spiel wie einen Zeitraffer aus dem Brutschrank
(Hoffman-Kontrast, optischer Schnitt); die zweite Woche spielt **im Körper**: Blick wie durch ein Hysteroskop auf die
Schleimhaut, achsnahes Kaltlicht, weiche Schatten, alles scharf (`Tools/Unreal/Implantation/setup_uterus_scene.py`,
Karte `L_GEN_UterineCavity`). Die Regie wechselt den Ort, sobald der Keim geschlüpft ist
(`GenesisSliceLogic::MapForEmbryoStage`, Test `Genesis.Slice.EmbryoMap`).

**Die Schleimhaut** (`Tools/Blender/Implantation/build_endometrium.py`, 1,75 Mio. Punkte, Nanite) entsteht aus
anatomischen Maßen der Sekretionsphase (Zyklustag 20–24, Implantationsfenster):

| | Maß | Herkunft |
|---|---|---|
| Polster des ödematösen Stromas | 0,4–0,9 mm breit, 20–45 µm hoch | Histologie der Sekretionsphase |
| Furchen dazwischen | 45–80 µm breit, bis 24 µm tief, geschwungen | Hysteroskopie |
| Drüsenöffnungen | 22–56 µm weit, 45–80 µm tiefer Trichter, im Mittel 175 µm auseinander, in dichteren und lichteren Feldern | SEM-Aufnahmen des Endometriums |
| Epithelzellen | 7 µm, gewölbte Kuppen, Zellgrenzen | einschichtiges Säulenepithel |
| Pinopoden | bis 4 µm hohe glatte Vorwölbungen, feldweise | Kennzeichen der Empfängnisbereitschaft |
| Kapillarnetz | gewundene Röhrchen ~50 µm unter der Oberfläche | subepitheliales Kapillarnetz |

Das Gitter ist in der Bildmitte 4 µm fein und wird zum Rand auf 16 µm gröber – Nanite zeigt in der Ferne ohnehin
weniger. Epithelzellen, Pinopoden, Flimmerzellen und Gefäße rechnet das Material (HLSL), nicht die Geometrie; das
Zellrelief blendet aus, sobald eine Zelle nur noch wenige Pixel groß ist, sonst flimmert das Bild.

**Der Keim von außen.** Unter dem Mikroskop zeigt das Spiel jede Furchungszelle einzeln; im Körper wäre das falsch –
eine geschlüpfte Blastozyste ist eine dünne, klare Hülle aus flachen, vieleckigen Zellen, gefüllt mit Flüssigkeit. Man
sieht hindurch: die Schleimhaut darunter, die Rückwand der Hülle, den Embryoblasten als trüben Knoten. Deshalb ist der
Keim hier eine durchscheinende Kugel mit Zellmuster (`M_GEN_TrophoblastShell`), der Embryoblast ein eigener, dichterer
Körper (`M_GEN_InnerCellMass`) – beim Anlagern dreht er sich sichtbar nach unten zur Schleimhaut. Die erste Fassung mit
den Einzelzellen der Simulation sah aus wie Popcorn.

**Was die Einnistung im Bild macht** (`AGenesisImplantationSite`, Werte aus der Simulation über eine
Material-Parameter-Sammlung):
- Der Keim sinkt ein: Was unter der Oberfläche liegt, verdeckt das Gewebe. Ein Wulst aus Epithel umschließt ihn.
- Im Wasserlinienkreis liegt nicht die Schleimhaut, sondern die untere Wand des Keims.
- Ab Tag 10 liegt er ganz darunter, ein Fibrinpfropf schließt die Stelle: gelblich-graues Gerinnsel aus Fasern mit
  roten Blutkörperchen, unregelmäßiger Rand.
- Bis Tag 12 wächst das Epithel darüber zu, die Stelle rötet sich (Deziduareaktion: weitere, dichtere Kapillaren), das
  Blut in den Lakunen scheint dunkelrot durch, die Stelle wölbt sich leicht vor.
- Die Kamera weicht mit dem wachsenden Keim zurück (gedämpft, nie springend), das Licht regelt mit dem Abstand nach,
  damit die Helligkeit gleich bleibt – wie an einem echten Endoskop.

| Tag 6: angelagert | Tag 7,7: sinkt ein | Tag 9,8: Fibrinpfropf | Tag 12,5: die Stelle |
|---|---|---|---|
| ![](Media/GENESIS-040_Anlagerung.png) | ![](Media/GENESIS-040_Einsinken.png) | ![](Media/GENESIS-040_Fibrinpfropf.png) | ![](Media/GENESIS-040_Stelle_Tag12.png) |

## Eigene Fehler auf dem Weg (im Bild gefunden)

- **Vertexfarben kommen in Unreal nicht an.** Die Maske der Drüsenöffnungen blieb wirkungslos. Gemessen:
  `has_vertex_colors = False` – auch an der Eileiterwand der ersten Woche, dort unbemerkt seit GENESIS-036. Jetzt trägt
  ein Datenbild (2048², 3,9 µm je Texel) die Masken; es liegt über der lokalen Lage und gilt für beide Wände.
- **Unreal überschreibt bei Alpha 0 die Farbe** durchsichtiger Pixel mit Nachbarfarben (PNG-Infill). Der Kennwert der
  Drüse liegt deshalb als 0,5…1 im Alpha.
- **Format „Vector Displacement Map" vertauscht Rot und Blau** (gemessen: Polster statt Drüse im Rotkanal). Jetzt
  unkomprimiert als HDR.
- **`pow()` mit negativer Basis ist in HLSL undefiniert.** Das ergab NaN in der Formverschiebung.
- **Weiße Säulen ab Tag 12:** Die Kamera war beim Zurückweichen über die gegenüberliegende Gebärmutterwand gestiegen und
  sah deren Drüsenschläuche von außen. Die Wand liegt jetzt 3,2 mm über der Einnistungsstelle, die Kamera bleibt darunter.
- **Alles wirkte blass und flach:** 10 EV Belichtung legten die Flächen in die Schulter des Tonemappers. Bei 8,5 EV
  bekommt das Gewebe Tiefe, und die Drüsengänge werden dunkel.
- **Streifen auf den Trichterwänden:** Das Zellmuster liegt in der Draufsicht; an steilen Wänden wurde es zu Streifen
  gezogen. Es blendet dort jetzt aus.
- **Die glasige Hülle zeichnete auch ihre versunkene Hälfte** – eine helle Schale mitten im Gewebe. Sie wird jetzt an
  der Oberfläche der Schleimhaut abgeschnitten.
- **Gefäße wie rote Blitze und Furchen wie gesprungene Farbe:** beides waren Vieleckkanten (Voronoi). Jetzt sind es
  Höhenlinien eines verwundenen Rauschens – sie laufen geschwungen und verzweigt.

## Qualitätsprüfung (Teil 2)

| | |
|---|---|
| VISUAL | Bild für Bild geprüft von Tag 6 bis 14; Befunde oben |
| MATERIAL | Gewebe mit Streuung, Schleimfilm glänzt, im Drüsenschlauch kein Glanz und kein Streulicht |
| SCALE | Keim 0,2 → 1,2 mm gegen Drüsenöffnungen von 22–56 µm und Epithelzellen von 7 µm |
| LIGHTING | eine Quelle (Kaltlicht am Hysteroskop, 5600 K), physikalisch geregelt über den Abstand, Belichtung gemessen |
| ANIMATION | Der Keim dreht sich beim Anlagern, sinkt ein; die Kamera folgt gedämpft |
| PHYSICS | Lage, Tiefe und Größe kommen aus der Simulation, nichts ist von Hand gesetzt |
| PERFORMANCE | gemessen im fertigen Spiel, Full HD nativ: 15,4 ms je Bild (65 fps), GPU 9,7 ms, 78 Draw Calls, 2,1 GB RAM, 4,0 GB VRAM |
| ORIGINALITY | eigene Geometrie und eigene Materialien, keine gekauften Assets |

## Offen

- Biologisch erreicht der Keim die Gebärmutter schon als Morula (Tag 4); im Spiel wechselt der Ort erst beim Schlüpfen
  (Tag 5–6). Die erste Woche bleibt so ganz beim Bild aus dem Brutschrank.
- Die leere Zona, aus der der Keim geschlüpft ist, fehlt noch: gestaucht wirkte sie wie eine Kontaktlinse.
- Der Bildaufbau hängt am Renderthread (15,4 ms gegen 9,7 ms GPU); das ist Luft für später.
- Das Bild ist noch nicht auf die Bildsprache des Covers gezogen (Gold gegen kalte Tiefe).
- Der Durchlauf hält bei gescheiterter Einnistung nur an. Eine eigene Szene dafür fehlt, und für den Spieler kommt sie
  nicht vor.
