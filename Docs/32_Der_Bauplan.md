# 32 – Der Bauplan: die dritte und vierte Woche

*GENESIS-041. Carnegie-Stadien 7 bis 13, Tag 15 bis 28 nach der Befruchtung.*

Bis zur zweiten Woche ist der Keim eine Scheibe aus zwei Blättern (Docs/30). In der dritten Woche bekommt er einen
Bauplan: vorn und hinten, links und rechts, oben und unten. In der vierten schlägt sein Herz.

| Bereich | Stand |
|---|---|
| Simulation der dritten und vierten Woche (`GenesisEmbryogenesisLogic`) | **PRODUCTION READY**: gegen Lehrbuch und Ultraschalldaten geprüft |
| Anzeige im Durchlauf (Tag, Länge, Somiten, Herzschlag), Entwicklerseite | **BETA** |
| Der Körper an Tag 28, nach Referenzen in Blender (Form, innere Organe, Gewebe) | **ALPHA** – siehe „Der Körper" |
| Der Körper in Unreal (bis zur Fruchthöhle in der Prüfkarte `L_GEN_EmbryoLookdev`, seit Doc 35 entfernt) | **ALPHA** – siehe „In Unreal" |
| Die Fruchthöhle im Durchlauf (`L_GEN_Fruchthoehle`): Amnion, Dottersack, Dottergang, Haftstiel, das schlagende Herz | **ALPHA** – siehe „Die Fruchthöhle" |
| Die Formen der Tage 15–25 (Keimscheibe, Neuralrinne, erste Somiten) | **PLAN** |

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

## Der Körper (Tag 28) – nach Referenzen, in Blender

Game Director: „Baue das in Blender live, recherchiere vorher alle Bilder und nutze diese für das 3D-Rendering."

![Tag 28, seitlich](Media/GENESIS-041_Embryo_Tag28_Seite.png)

![Tag 28, von vorn schräg](Media/GENESIS-041_Embryo_Tag28_Dreiviertel.png)

![Tag 28, Rücken: Somiten beiderseits des Neuralrohrs](Media/GENESIS-041_Embryo_Tag28_Ruecken.png)

**Referenzen** (25 Bilder, Wikimedia Commons, Lizenz je Bild in `ArtSource/Reference/Biology/Embryo/QUELLEN.md` – nur lokal,
nicht im Repository): vor allem die Blechschmidt-Rekonstruktionsmodelle echter Embryonen mit 2,5 / 3,4 / 4,2 / 6,3 mm
Länge (Form in 3D) und ein Präparat mit 4–5 Wochen (Oberfläche). Aus dem Präparat sind die Landmarken im Raster
abgelesen (Kopfkuppe, Rückenlinie, Herz, Kiemenbögen, Knospen, Schwanz; 6,7 µm je Bildpunkt).

**Aufbau** (`Tools/Blender/Embryogenesis/build_embryo_day28.py`, Lookdev `lookdev_embryo.py`):
- Die Form entsteht aus weich verschmelzenden Grundformen (Metaballs): der Rumpf in 14 gemessenen Querschnitten
  zwischen Rücken- und Bauchlinie, Rückenwülste entlang der Somiten, der Kopf aus drei Hirnbläschen, Kiemenbögen als
  Leisten, Herzwölbung, Leberwulst, Arm- und Beinknospe. Danach gleichmäßig neu vernetzt (9 µm, 700 000 Punkte).
- Feinformen als Verschiebung: 30 Somitenfurchen (nach hinten kleiner), die kaum erhabene Mittellinie, Ohr- und
  Linsengrübchen.
- **Innen liegen die Organe als eigene Körper**, weil der Embryo mit vier Wochen fast durchsichtig ist: Herzschlauch
  mit Blut (S-Schleife vom Venensinus zum Truncus), Aortenbögen durch die Kiemenbögen, Rückenaorten, Hirnwand um die
  flüssigkeitsgefüllten Ventrikel, Neuralrohr mit Lumen, 30 Somitenpaare, Leberanlage.
- **Gewebe nach der Physik im Fruchtwasser:** Die Hülle ist überwiegend durchlässig; der Brechzahlsprung zwischen
  Gewebe und Fruchtwasser ist winzig (1,38 zu 1,335), deshalb fast kein Glanz. Darunter ein schwach streuendes
  Mesenchym (mittlere freie Weglänge 1,7 mm) – dunstig, die Organe bleiben sichtbar.
- Größte Länge 4,6 mm, Kopfbreite 1,6 mm. Belichtung gemessen (Motiv-Median 0,42–0,46, nichts ausgebrannt).

**Eigene Fehler auf dem Weg** (in dieser Reihenfolge gefunden und behoben):
1. Nur das Neuralrohr als Körper genommen – ein Wurm. Das „C" ist innen gefüllt (Schlund, Herz, Leber).
2. Kopf aus Füllmasse – ein Kasten. Jetzt drei Hirnbläschen als Kuppen.
3. Somitenfurchen dreimal zu tief und die Mittellinie als Grat – wieder der „Drachenkamm" des Prototyps.
4. Glanz und Wachs: gegen Luft gerechnet statt gegen Fruchtwasser, dazu eine undurchsichtige Haut.
5. Somiten als Perlenkette außerhalb des Körpers; in der Rückenansicht standen sie beiderseits heraus, weil der
   Querschnitt zum Rücken hin spitz zulief. Jetzt Rückenwülste in voller Körperbreite.
6. Ein Loch unter dem Kopf (Schlundboden fehlte), Kiemenbögen wie Zähne, das Neuralrohr am Nacken außerhalb der Haut.
7. Messwerkzeug: 16-Bit-Bilder wurden linear gemessen und wirkten zu dunkel (behoben in `genesis_blender_common`).

Der frühere Prototyp (`build_embryo_body.py`, Röhre mit Ringen) ist damit ersetzt und entfernt.

### In Unreal

![Tag 28 in Unreal, Prüfkarte mit dem Licht des Blender-Lookdevs](Media/GENESIS-041_Embryo_Tag28_Unreal.png)

Weg: `build_embryo_day28.py` exportiert (`export_all`) Hülle und Organe als FBX (1 µm = 1 Einheit),
`Tools/Unreal/Embryogenesis/setup_embryo.py` importiert, baut die Materialien und die Prüfkarte
`/Game/Genesis/Embryogenesis/Maps/L_GEN_EmbryoLookdev` mit Kamera und Licht wie in Blender.

- **Organe** undurchsichtig mit Streuung (Subsurface), über Nanite in voller Auflösung (Herz, Gefäße, Neuralrohr,
  Somiten, Leberanlage – je eine Materialinstanz von `M_GEN_EmbryoOrgan`).
- **Hülle** durchscheinend (kein Nanite möglich, deshalb in Blender auf 30 % der Dreiecke reduziert – glatt, ohne
  sichtbaren Verlust). Blender rechnet das Mesenchym als Streuvolumen; in Echtzeit übernimmt das die Hülle selbst:
  - **Beer-Lambert:** Deckung = 1 − e^(−d/0,7 mm), d = Gewebe zwischen Haut und dem Organ dahinter (Szenentiefe minus
    Hauttiefe). Ein Organ dicht unter der Haut bleibt klar, ein tiefes wird milchig.
  - **Tiefenbewusste Streuunschärfe:** Das Bild hinter der Haut wird mit 13 Abtastpunkten geholt, Radius mit d
    wachsend; ein Punkt zählt nur, wenn hinter ihm Körper liegt.
- **Gemessen:** Grafikzeit 2,7 ms bei 1066 × 600 (1,27 Mio. Dreiecke, 83 Zeichenaufrufe). Farben an denselben
  Bildstellen gegen Blender abgeglichen.

**Gefunden und behoben:**
1. Alles blau statt gold: `unreal.Color` liegt als B, G, R im Speicher – jetzt mit benannten Kanälen.
2. Organe wie in Glas: feste Deckung der Hülle; jetzt Tiefe nach Beer-Lambert und Unschärfe.
3. Dunkle Ringe um Organkanten durch die Unschärfe (Abtastung traf den schwarzen Hintergrund) – jetzt tiefenbewusst.
4. Durchscheinende Flächen beleuchtet Unreal nur mit **einem** gerichteten Licht: Gegenlicht und Aufhellung sind
   jetzt Scheinwerfer (Stärke aus I = E · d²).
5. Orange Haut: Unreals Filmic-Tonemapper behält Sättigung, die Blenders AgX nimmt – Licht auf das Gold des Covers
   (#C6A28F) gedämpft.
6. `SceneColor` hat in 5.8 nur noch einen Eingang (Modus „Versatz"); Verbindungen im Materialgraphen werden jetzt
   geprüft statt still übergangen.

**Noch offen in Unreal:** feines Flimmern an den Knospen (Abtastung am Rand), der Hirnrand ist unten etwas bläulich
(Aufhellung von unten).

## Die Fruchthöhle (Teil 5)

![Die Fruchthöhle an Tag 28, im Spiel aufgenommen](Media/GENESIS-041_Fruchthoehle.png)

Der Embryo in seiner Umgebung, so wie ihn ein Embryoskop sähe:
- **Amnion:** eine Zellschicht, eng um den Embryo, am Nabel angesetzt. Im Fruchtwasser fast unsichtbar; man sieht es
  nur, wo der Blick streifend durch die Haut läuft (Deckung 2,5 % in der Fläche, 45 % am Saum).
- **Dottersack**, 3,5 mm (Ultraschall 6. SSW: 3–5 mm), außerhalb des Amnions. Auf ihm das erste Blut: Blutinseln am
  fernen Pol, ein Netz von Dottergefäßen, das am Stiel zusammenläuft.
- **Dottergang** vom Nabel zum Dottersack, Dotterarterie und -vene in seiner Wand.
- **Haftstiel – die eigentliche Lebensleitung.** Kurz und kräftig, mit den Nabelgefäßen (zwei Arterien, zwei Venen),
  vom Schwanzende des Nabels zur **Chorionplatte**: der Wand der Fruchtblase dort, wo sie in der Gebärmutterwand
  (Decidua basalis) verankert ist. Hier wächst die Plazenta; von der Ansatzstelle verzweigen sich die Gefäße über die
  Platte. Aus Haftstiel und Dottergang wird ab Woche 4–8 die Nabelschnur, wenn das Amnion beide umwächst.
- Die **Chorionhöhle** (22 mm) ringsum, rötlich, im Hintergrund.

**Recherche zur Frage des Game Directors** („Sollte da nicht die Gebärmutter dran sein und kein schwebender Ball?"):
Beides stimmt. Der Dottersack schwebt tatsächlich frei in der Chorionhöhle und hängt nur am Dottergang, man sieht ihn
im frühen Ultraschall als Ring. Die Verbindung zur Mutter aber ist der Haftstiel zur Chorionplatte an der
Gebärmutterwand. In der ersten Fassung lief er ins Dunkel, und das Kind schien am Dottersack zu hängen. Quellen:
[embryology.ch – Nabelschnur](https://embryology.ch/en/embryogenese/fetal-membranes-and-placenta/umbilical-cord/development.html),
[StatPearls – Umbilical Cord](https://www.ncbi.nlm.nih.gov/books/NBK557490/),
[ScienceDirect – Connecting Stalk](https://www.sciencedirect.com/topics/neuroscience/connecting-stalk).
- Die Flüssigkeit der Chorionhöhle ist leicht trüb: Das Licht des Embryoskops steht als Hauch im Raum.

Weg: `Tools/Blender/Embryogenesis/build_fruchthoehle.py` baut und exportiert die Umgebung im Koordinatensystem des
Embryos; `Tools/Unreal/Embryogenesis/setup_fruchthoehle.py` importiert, legt die Materialien an und baut die Karte.

**Der Szenen-Actor** `AGenesisEmbryoScene` (Plugin GenesisEmbryo) zeigt, was die Simulation rechnet:
- Der Körper hat die simulierte Länge (Tag 26 kleiner als Tag 28; das Modell steht für 4,6 mm).
- **Das Herz schlägt in Echtzeit** im simulierten Takt (111/min an Tag 28), auch während der Zeitraffer läuft – rasch
  zusammen, langsam erschlaffen, Ruhe (Test `Genesis.Embryo.SceneView`).
- Kamera wie ein Embryoskop: 18 mm Brennweite, nah und weitwinklig, sanftes Schwenken; das Licht am Lichtleiter regelt
  nach, damit der Embryo bei jedem Abstand gleich hell ist. Bildaufbau vorher gegen die Geometrie durchgerechnet (die
  Kamera muss in der 22-mm-Höhle bleiben).

**Im Durchlauf:** Ab Tag 26 wechselt die Phase „Die ersten vier Wochen" aus der Gebärmutterhöhle in die Fruchthöhle
(`MapForEmbryo`, Test `Genesis.Slice.EmbryoSceneMap`). Steht der Bauplan, hält die Zeit 8 Sekunden an – nur das Herz
schlägt weiter –, dann beginnt die Schwangerschaft. Dort springt die Zeit in Wochen; ab Tag 29 blendet die Kamera ab,
statt einen Embryo vom Ende der vierten Woche in der achten zu zeigen.

**Gefunden und behoben:**
1. Der Nabel lag in der Lücke zwischen Bauch und eingerolltem Schwanz, außerhalb der Haut – der Dottergang endete als
   offenes Rohr im Wasser. Jetzt per Strahl gegen die Hülle gemessen und geschlossen in der Bauchwand angesetzt.
2. Dottergang wie ein Strohhalm mit zwei knallroten Schläuchen obenauf; jetzt liegen die Gefäße in der Wand und
   schimmern gedämpft durch.
3. Der Dottersack beherrschte angeschnitten den unteren Bildrand; jetzt neben dem Embryo, ganz im Bild.
4. Die Dottergefäße liefen als lange Bögen einmal um die Kugel – ein Ball mit Nähten; dazu zog der Hauptast seine
   Länge bei jedem Schritt neu (Zickzack). Jetzt verschieden lange, geschlängelte Äste, die am Stiel zusammenlaufen.
5. Der Haftstiel endete frei im Wasser und stand dann stirnseitig hinter dem Schwanz wie eine Fahne; jetzt läuft er
   nach hinten in die Chorionwand.
6. Der Hintergrund war reines Digitalschwarz (0/0/0 gemessen). Mit leicht streuender Flüssigkeit (Dichte 0,012;
   0,03 nahm dem Embryo den Kontrast) steht der Lichtkegel als Hauch im Raum.
7. **Im gebauten Spiel gefunden:** Die Kamera blendete in der Schwangerschaft nie ab. Steht der Bauplan, rechnet der
   Embryo nicht weiter, und seine eigene Uhr blieb bei Tag 28 stehen. Die Szene liest das Alter jetzt aus der
   Weltuhr, die in Wochen weiterspringt.
8. Der Haftstiel lief ins Dunkel, die Chorionplatte fehlte (siehe Recherche oben). Jetzt steht die Platte hinter dem
   Embryo; der Ansatz liegt auf dem Sichtstrahl der Kamera rechts unter ihm (eine erste Rechnung setzte den Versatz ohne
   Perspektive an, der Ansatz landete hinter dem Embryo).
9. **Alle selbst gebauten Kugeln und Röhren hatten ihre Flächen nach innen gedreht.** Unreal zeichnet nur
   Vorderseiten: Vom Dottersack, den Stielen und Gefäßen sah man die Innenseite der Rückwand (es fiel kaum auf), die
   Chorionwand dagegen war unsichtbar – deshalb lag der Hintergrund bisher im Schwarz. Jetzt im Skript geprüft:
   Kugeln und Röhren nach außen, die Höhle nach innen.
10. Die Gefäße der Platte kamen erst aus dem Gefäßbaum des Dottersacks und ringelten sich um den Ansatz wie
    Stacheldraht; jetzt ein eigener, flacher Baum mit gabeligen Ästen.

## Offen

- Der Körper steht für **Tag 28**. Die Tage davor (Keimscheibe, Neuralrinne, erste Somiten) brauchen eigene Formen
  derselben Bauart, damit die Wochen 3–4 sichtbar werden – dann als Übergänge zwischen den Tagen.
- In der Fruchthöhle: Die Aortenbögen zeichnen sich am Hals als kräftig roter Knoten ab – im Leben schimmern sie
  durch, aber zarter. Der Gefäßbaum der Chorionplatte ist noch recht regelmäßig gegabelt; die Wand hat keine
  Feinstruktur (Zotten und Bluträume dahinter nur als Farbe).
- Feinheiten: Gefäßgeflecht am Kopf, Kardinalvenen, feinere Kiemenfurchen, Nasenplakoden (erst ab Tag 32).
- Der Herzschlag soll hörbar werden (der Klang liegt im Körperklang-System bereit) und den Spieler von hier an
  begleiten.
- Die Ernährung der Mutter steht noch als Wert in der Regie (`MotherNutrition`), weil die Mutter noch keine eigene
  Person mit Körper ist.
