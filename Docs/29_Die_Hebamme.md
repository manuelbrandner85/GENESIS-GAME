# 29 – Die Hebamme

*GENESIS-039 (Teil 5). Game Director: „Die Beine der Hebamme sehen sehr künstlich aus, die Arme zu steif – insgesamt viel
realistischer, das Maximum an Fotorealismus. Physik und das Organische müssen 100 % passen."*

Die Hebamme ist der erste Mensch, den das Kind sieht. Sie fängt es auf, hält es vor ihr Gesicht, legt es der Mutter auf die
Brust, rubbelt es dort trocken und deckt es zu. Grundlage ist eine MetaHuman-Figur (im Creator gebaut, nicht im öffentlichen
Repo); alles, was sie tut und trägt, kommt aus diesem Projekt.

## Kleidung: echte Kleidungsstücke statt einer Hülle

Der MetaHuman Creator kennt als Kleidung nur T-Shirt und Shorts. Die erste Hose (Teil 4) war eine im Material aufgeblasene
Hülle auf dem Körper – eine glatte Röhre ohne Gewicht, mit einer Stufe am Saum der Shorts. Das Urteil „künstlich" war richtig.

**Jetzt: Kasackhose und Clogs aus einer Stoffsimulation** (`Tools/Blender/Birth/build_scrub_trousers.py`):

1. Körper und Kleidung der Hebamme aus Unreal nach Blender (`Tools/Unreal/Birth/midwife_export.py`, lokal, nicht im Repo).
2. **Schnitt wie beim Schneider:** zwei Hosenbeine und das Becken als ein Stück (reine Vierecke), jeder Punkt auf der
   gemessenen Oberfläche von Körper und Shorts plus Bewegungszugabe (Strahlen von außen auf den Körper). Oben 2 cm Zugabe,
   darunter gerade weite Beine (Saumweite ~55 cm), der Saum knapp über dem Boden.
3. **Stoffsimulation:** Poly-Baumwoll-Köper, Schwerkraft, Bund gehalten, Kollision mit Beinen und Clogs. Der Stoff fällt,
   legt sich in Falten am Knie und staucht sich über dem Schuh.
4. **Organische Kontrolle:** Kein Punkt des Stoffes liegt näher als 5 mm an der Haut (306 Punkte hatte das Glätten in die
   Oberschenkel gezogen – im Spiel waren das zwei Löcher). Wo der Kasack die Hose überdeckt, liegt sie 4 mm darunter.
5. **Hautgewichte:** an den Beinen exakt die des Körpers – der Stoff geht mit jedem Knie und jeder Hüfte mit wie die Haut
   darunter. Die zuerst übernommenen Gewichte der Shorts ließen die Haut bei der kleinsten Hüftbeugung durchstechen.
6. Als Skeletal Mesh auf ihr Skelett, im Spiel über die Pose des Körpers geführt (`AGenesisMidwifeRig::Dress`).

**Clogs:** Die MetaHuman-Figur war barfuß – im Kreißsaal undenkbar. Jetzt trägt sie geschlossene, glatte Kunststoff-Clogs
(Arbeitsschutz: geschlossene Ferse, keine Löcher), aus der konvexen Hülle ihres Fußes mit hoher Zehenkappe und 28 mm Sohle.
Sie steht entsprechend 28 mm höher.

**Handschuhe** bleiben eine Hülle auf der Haut (0,1 mm Nitril folgt der Hand exakt); der Abstand ist jetzt 1,6 mm, bei
0,7 mm schien an gebeugten Fingern die Haut durch.

| Stoffsimulation (Blender) | im Spiel | Saum und Clogs |
|---|---|---|
| ![](Media/GENESIS-039_Hose_Stoffsimulation.png) | ![](Media/GENESIS-039_Hebamme_Kasackhose.png) | ![](Media/GENESIS-039_Hebamme_Saum_Clogs.png) |

## Arme und Hände: wie man ein Neugeborenes hält

**Befund (gemessen):** Die Hände lagen 57 cm vor ihrer Schulter – länger als ihr Arm. Der Halteort war von ihren Augen aus
gerechnet (38 cm davor); weil sie sich vorbeugte, lagen die Augen schon 28 cm vor den Füßen. Die Arme standen durchgestreckt
nach vorn, die Finger gestreckt und gespreizt – die Referenzpose des Skeletts.

**Jetzt:**
- **Halten:** vom Schulterpunkt aus 30 cm vor und 6 cm unter den Schultern. Das Kind liegt auf den Unterarmen vor der unteren
  Brust, die Ellenbogen gut 80–90° gebeugt und am Körper, ihr Kopf neigt sich hinunter. Von Auge zu Auge rund 25 cm – der
  Abstand, auf den ein Neugeborenes scharf sieht. Abstand Schulter–Hand jetzt 40 cm.
- **Finger:** In allen drei Gelenken gebeugt (entspannt 14/24/10°, beim Halten 32/38/18°), außen etwas mehr, die Finger
  rücken zusammen, der Daumen liegt an. Die Beugeachse wird aus der Handfläche berechnet, für jede Hand.
- **Lebendigkeit:** Niemand steht still wie eine Statue. Das Gewicht wandert langsam zwischen den Füßen (Rauschen statt Sinus),
  die Hände driften um wenige Millimeter, jede für sich. Beim Zusehen liegen die behandschuhten Hände ineinander vor dem Bauch.

| vorher | jetzt |
|---|---|
| ![](Media/GENESIS-039_Hebamme_Arme_vorher.png) | ![](Media/GENESIS-039_Hebamme_Arme.png) |

## Versorgung auf der Brust: abtrocknen, zudecken

Leitlinie (AWMF, WHO): Ein gesundes Kind kommt **sofort** auf die Mutter und wird dort abgetrocknet. Vorher hielt die
Hebamme es zwei Simulationsminuten vor ihr Gesicht – nass in der Luft. Jetzt: eine halbe Minute, wer schreit, früher.

Dann, in dieser Reihenfolge (`GenesisMidwifeLogic::TaskFor`): hinüberreichen (7 s) → **abrubbeln** (14 s: Hände über Rücken
und Kopf, 1,6 Striche je Sekunde, das Kind spürt das rhythmische Hin und Her, Geräusch des Frotteetuchs, „So, du Kleines –
jetzt rubbel ich dich erst mal schön trocken.") → **zudecken** (4 s: ein zweites, trockenes Tuch über Rücken und Hinterkopf;
das Kind sieht dessen Rand oben im Bild, „Und ein warmes Tuch drüber. So liegst du gut.") → neben dem Bett zusehen.

**Die Physik dahinter** (`GenesisEarlyLifeLogic`, Test `DryingAndCoveringKeepWarm`): Nass verliert ein Neugeborenes über die
Verdunstung mehr Wärme als auf jedem anderen Weg. Abgetrocknet kühlt es im Raum halb so schnell aus; auf der Brust bleibt es
nass 0,7 °C, unbedeckt 0,3 °C unter der Haut der Mutter.

| | nach 10 min allein | nach 30 min auf der Brust |
|---|---|---|
| nass / frei | 34,9 °C | 36,3 °C |
| abgetrocknet / zugedeckt | 35,9 °C | 37,05 °C |

![Zugedeckt: der Rand des Tuchs oben, die Hebamme sieht zu](Media/GENESIS-039_Zugedeckt.png)

## Rendering

Lumen-Spiegelungen mit Hit Lighting (Material und Licht am Trefferpunkt statt aus dem Oberflächen-Cache): Augen, Haut, Nitril
und Edelstahl spiegeln die Umgebung richtig. Gemessen: +0,7 ms (~5 %) im Kreißsaal auf einer RTX 5070.

## Nebenbefunde

- **Alle Geräusche waren um mehr als die Hälfte gekürzt:** Unreal las die Suno-MP3 nur zu ~45 % – der Raumklang lief 6,7 statt
  14,7 s, jeder Schrei brach nach 6 s ab. Importiert werden jetzt die WAV-Fassungen (`Tools/Unreal/Audio/import_speech.py`
  meldet die Länge jeder Aufnahme).
- **Geburtstuch:** hing rotbraun genau über ihrem Kopf (siehe Doc 28, Teil 4).

## Offen

- Das Kind selbst ist nicht zu sehen (es ist die Kamera); von außen hält sie „nichts".
- Kasack als T-Shirt-Schnitt mit Rundhals statt V-Ausschnitt; die Shorts liegen unsichtbar unter der Hose.
- Die Hose ist in der Ruhepose simuliert; beim Gehen falten sich die Beine nur über die Hautgewichte (keine Laufzeit-Simulation).
