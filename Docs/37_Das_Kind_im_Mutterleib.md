# 37 – Das Kind im Mutterleib: sein Körper, seine Handlungen, seine Sinne

*Auftrag des Game Directors (2026-09-23, während GENESIS-044 Teil 1b):* „Der Spieler soll auch kleine
Mikro-Interaktionen machen können, z. B. Hand bewegen oder sich insgesamt leicht bewegen, pupsen usw. Vor allem werden
nicht alle 9 Monate korrekt dargestellt – man sieht nur einen Teil der Entwicklung bis zum Embryo und dann das Kind.
Es soll immer das Kind gezeigt werden, dann in die Ich-Perspektive. Der Spieler muss mit allem alles wahrnehmen können.“

Grundlage für GENESIS-044 Teil 2. Recherche vor dem Bauen (Doc 00, Doc 00b), Wochentafel und Sinne in Doc 34.

## Was der Auftrag heißt

1. **Die Entwicklung lückenlos zeigen.** Heute springt das Bild vom Embryo (Tag 28, Fruchthöhle) direkt in die
   Ich-Sicht ab SSW 19 – der Fetus selbst ist nie zu sehen. Künftig zeigt jeder Moment zuerst **das Kind von außen**, so
   wie es in dieser Woche wirklich aussieht, und dann fährt die Kamera **in seine Augen**: Ich-Perspektive. Dazu die
   Wochen, die bisher fehlen (SSW 8, 10, 12, 16): Dort nimmt das Kind noch nichts bewusst wahr, also bleibt die Kamera
   draußen.
2. **Der Spieler handelt mit dem Körper, den das Kind in dieser Woche hat.** Nur was der Körper schon kann, geht –
   eine Hand zum Gesicht mit SSW 10, Augen öffnen erst mit SSW 28.
3. **Jede Handlung kommt in allen Sinnen an, die schon da sind:** man sieht die eigene Hand (wenn die Lider offen
   sind und Licht da ist), spürt sie (Rumble am Gamepad, Kamera), hört das eigene Schlucken und das Herz der Mutter,
   schmeckt das Fruchtwasser – und die Mutter antwortet, wenn sie den Tritt spürt.

## Was ein Fetus wirklich tut (recherchiert)

| Handlung | ab SSW (ca.) | Quelle | Im Spiel |
|---|---|---|---|
| Ganzkörperbewegung, Strecken, Schreck | 9–10 | de Vries et al. 1982 (Doc 34) | sich bewegen, strecken |
| Arm-, Beinbewegungen, Kopf drehen | 10–11 | de Vries 1982 | Hand bewegen, strampeln, umschauen |
| Hand zum Gesicht | 10–12 | de Vries 1982 | Hand zum Gesicht, Daumen |
| Atembewegungen, Gähnen, Schlucken | 10–13 | de Vries 1982 | schlucken (schmecken), gähnen |
| Schluckauf | 9–10 (in Serien) | de Vries 1982 | geschieht dem Kind – nicht steuerbar |
| **Wasserlassen** – die Niere arbeitet, der Urin wird Teil des Fruchtwassers | früh; gemessen 12 ml/h in SSW 25, 91 ml/h in SSW 40 | Maged et al. 2014 | ja – unauffällig, als Körpergefühl |
| Tritte, die die Mutter spürt | 18–22 | Doc 34 | treten → sie antwortet |
| Die Mutter berührt den Bauch → das Kind berührt sich weniger selbst, saugt mehr | 3. Trimenon | Nagy et al. 2021 | ihre Hand auf dem Bauch als Ereignis |
| Nabelschnur greifen (Greifreflex) | 3. Trimenon | Jakobovits 2007 | Nabelschnur greifen |
| Augen öffnen, Blick zum Licht | 26–28 | Doc 34 | Augen öffnen/schließen |

**„Pupsen“ gibt es im Mutterleib nicht.** Im Darm eines Ungeborenen ist Flüssigkeit, keine Luft – Gas kommt erst nach der
Geburt hinein, mit der ersten geschluckten Luft und den Bakterien. Auch Stuhl (Mekonium) bleibt normalerweise im Darm: Abgang vor dem Termin
ist selten, am ehesten am oder nach dem Termin und oft ein Stresszeichen (Yurdakök 2011). Das **Wasserlassen** dagegen
ist echt und dauernd: Ein großer Teil des Fruchtwassers ist Urin des Kindes, den es wieder schluckt. Das kommt ins
Spiel – ohne Klamauk, als warmes Körpergefühl, wie alles andere.

## Die Sinne und wie der Spieler sie erlebt

| Sinn | ab SSW | Darstellung |
|---|---|---|
| Tasten (Mund, dann Hände, dann Körper) | Mund ~8, Hände ~11, fast ganzer Körper ~17 (Humphrey; noch mit Quelle belegen) | Kamera/Hand, die anstößt; Rumble; die Wand gibt nach |
| Gleichgewicht, Lage | Organ ~14 angelegt, Reaktionen später (noch zu belegen) | Wiegen, wenn sie geht; Drehen, wenn das Kind sich dreht |
| Schmecken | 12–14 | nach ihrer Mahlzeit schmeckt das Fruchtwasser anders; süß → das Kind schluckt öfter |
| Hören | 19 | Teil 1b (Band wächst mit der Woche) |
| Sehen | 26–28 | Teil 1b (Rot, Lider, Pupille), dazu die eigene Hand vor dem Gesicht |

## Plan GENESIS-044 Teil 2

- **2a – Handlungen (Code zuerst):** Eingaben im Mutterleib, jede nach Woche freigeschaltet und im Körpermodell des
  Kindes gerechnet (`GenesisFetalLogic`): Hand bewegen / zum Gesicht / Daumen, strampeln/treten, strecken, Kopf drehen,
  schlucken, gähnen, Augen öffnen, Nabelschnur greifen. Rückmeldung in jedem schon vorhandenen Sinn; die Mutter
  reagiert auf gespürte Tritte (Hand auf dem Bauch: Druck, Schatten im Licht, ihre Stimme). Tests: nichts vor der
  Woche, in der es der Körper kann.
- **2b – Der Fetus in jeder Woche (Blender → Unreal):** ein parametrisches Fetusmodell nach Referenzen (Board B „Fetus
  Woche 8–40“, Doc 36: Carnegie-/Kyoto-Sammlung, 3D-Ultraschall): Proportionen je Woche (Kopf fast die Hälfte in SSW
  10, am Termin ein Viertel), durchscheinende Haut bis ~24, Lanugo 20–32, Käseschmiere, Fett ab 28, verklebte Lider bis
  26. Mit Skelett für Hände, Arme, Beine, Kopf – damit der Spieler seine eigene Hand sieht.
- **2c – Außen, dann innen:** Jeder Moment beginnt mit dem Kind von außen (dokumentarisch, das rote Licht der Szene),
  dann fährt die Kamera in seine Augen. Neue Momente SSW 8, 10, 12, 16 nur von außen (noch kein bewusstes Erleben).

## Quellen (neu, über PubMed)

- Maged AM et al. 2014, *Measuring the rate of fetal urine production using three-dimensional ultrasound*, J Matern
  Fetal Neonatal Med 27:1790–1794 ([DOI](https://doi.org/10.3109/14767058.2013.879709)).
- Yurdakök M 2011, *Meconium aspiration syndrome: do we know?*, Turk J Pediatr 53:121–129 (PMID 21853647).
- Nagy E et al. 2021, *Do foetuses communicate? Foetal responses to interactive versus non-interactive maternal voice
  and touch*, Infant Behav Dev 63:101562 ([DOI](https://doi.org/10.1016/j.infbeh.2021.101562)).
- Jakobovits A 2007, *Grasping activity is a part of fetal ethology*, Orv Hetil 148:1673–1675
  ([DOI](https://doi.org/10.1556/OH.2007.28089)).
- de Vries JIP, Visser GHA, Prechtl HFR 1982, *The emergence of fetal behaviour I* (Doc 34).

## Teil 2a – die Handlungen (umgesetzt)

**Was der Spieler tun kann** – jede Handlung erst ab der Woche, in der der Körper sie kann
(`GenesisFetalLogic::CanPerform`, Meilensteine nach de Vries 1982, Augen Doc 34, Greifen 3. Trimenon):

| Taste (Tastatur / Controller) | Handlung | ab SSW |
|---|---|---|
| WASD / linker Stick | sich bewegen (spät im letzten Drittel wird es eng: `RoomToMove`) | 8,5 |
| Maus / rechter Stick | den Kopf drehen (kehrt von selbst langsam zurück) | 10 |
| Umschalt / LT halten + bewegen | die eigene Hand bewegen | 9,5 |
| Leertaste / A | strampeln | 9,5 |
| Q / X | strecken; halten: gähnen (die Lider schließen sich dabei) | 8,5 / 11 |
| E / B | halten: Daumen zum Mund; tippen: schlucken – und schmecken | 10 / 12 |
| F / RT | greifen – um die Nabelschnur, wenn die Hand dort ist | 28 |
| R / Y | Augen auf und zu | 26 |

**Was davon in den Sinnen ankommt**
- **Sehen:** die eigene Hand vor dem Gesicht, im Gegenlicht rot durchscheinend (`SK_GEN_FetalHand`, Blender
  `build_fetal_hand.py`, Skelett mit 17 Knochen; Größe nach Woche: 0,83 × Fußlänge, Näherung). Lider zu: nur rotes Leuchten.
- **Tasten:** Vibration am Controller, nur so stark, wie das Kind schon tastet (`FetalView.Touch`), und nur mit
  eingeschalteter Vibration: Tritt, Strecken, Daumen an den Lippen, Schlucken, ihre Hand; ihr Puls als feines Klopfen;
  die Nabelschnur in der Hand pulsiert im Takt des eigenen Herzens.
- **Schmecken:** Nach ihren Mahlzeiten schmeckt das Fruchtwasser anders – deutlich ab ~45 min (Mennella 1995), süßlich,
  Knoblauch oder Karotte (`GenesisMotherDay`, Test `People.MotherFlavor`). Ohne Aroma: „warm, ein wenig salzig“.
- **Gleichgewicht:** Wiegen, wenn sie geht; Strecken und Drehen bewegen den Blick.
- **Die Mutter antwortet:** Ein Tritt, den sie spürt (ab SSW ~20), und sie legt nach 1,5–4 s die Hand auf den Bauch –
  ein weicher Schatten im roten Licht, das Kind wird ein wenig weggedrückt, oft spricht sie.
- **Wasserlassen:** etwa alle 20–40 Minuten (Näherung), als Körpergefühl – „Warm. Das Kind lässt Wasser.“

Eine Zeile unten im Bild sagt, was das Kind gerade spürt oder schmeckt; links stehen die Handlungen dieser Woche, schon
benutzte treten zurück. Entwickler: `genesis.Womb.Do kick|stretch|yawn|thumb|swallow|grasp|eyes|touch|hand <x> <y>`.

**Geprüft** im gebauten Spiel (`L_GEN_Mutterleib`, SSW 31): Hand im Bild, Tritt → ihre Hand und ihr Satz
(„Mutterleib: Sie spürt den Tritt und legt die Hand auf den Bauch, spricht VO_G_M_Bauch_01“), Schlucken → Knoblauch,
Daumen → Hand zum Mund, Greifen an der Nabelschnur („Das Kind greift die Nabelschnur“). GPU 4,1 ms. Tests
`Body.Fetal.Actions`, `People.MotherFlavor`; 158 von 158.

**Unterwegs gefunden:** Die erste Hand (eine Röhre je Knochen) wirkte wie ein Handschuh, die zweite war ein Paddel
(Metaball-Größe doppelt gerechnet); jetzt weich verschmolzene Körper in cm gebaut. In Unreal fehlte zuerst das Skelett
(Absturz beim Laden), dann lag die Hand grau im Standardmaterial (drei Materialplätze nach dem Neuimport).

## Teil 2a, Nachtrag – Hand und Nabelschnur nach Recherche

Game Director: „Die Hand des Babys ist zu unorganisch, auch die Physik stimmt nicht, viel zu steif, und kann durch die
Nabelschnur hindurch statt sie zu greifen. Recherchiere … alles sehr realistisch.“

**Recherchiert** (PubMed, Belege je Zahl):
- Handlänge am Termin 64 ± 3 mm (Halder 1999, PMID 10320927; Honoré 2016, DOI 10.1016/j.dib.2016.03.089); Fußlänge
  FL = −14,02 + 2,361 × SSW mm (Hebbar 2013, DOI 10.4038/sljog.v35i2.6169) → Hand ≈ 0,8 × Fuß: SSW 28 ≈ 4,2 cm.
- Beugefurchen fertig ab SSW ~13 (Kimura 1991), Nägel an der Kuppe erst ~SSW 34; Fett ab SSW 28 (Haut vorher dünn und
  durchscheinend), Käseschmiere im letzten Drittel (Nishijima 2019, DOI 10.1111/jog.14103).
- Am Termin liegt bei 62,5 % der Daumen eingeschlagen in der Faust (Jaffe 2000, DOI 10.1542/peds.105.3.e41);
  dauerhaft geballte Fäuste mit überkreuzten Fingern sind ein Krankheitszeichen – also vermeiden.
- Bewegung: flüssig, an- und abschwellend, mit Drehung um die Gliedmaßenachse (Einspieler & Prechtl 2005,
  DOI 10.1002/mrdd.20051); Hand zum Mund 1,6–2 s, gut zwei Drittel davon Abbremsen (Zoia 2013,
  DOI 10.1371/journal.pone.0080876). Die Langsamkeit kommt aus dem Nervensystem, nicht aus dem Fruchtwasser (Viskosität
  ≈ 1,0–1,2 cP, fast wie Wasser; Rosati 1991).
- Greifen der Nabelschnur: beobachtet ab SSW 32, als kurzes Greifen normal, anhaltendes Festhalten mit gestörtem
  Blutfluss (Habek 2002, DOI 10.1007/s00404-002-0375-7; Jakobovits 2007; Heyl & Rath 1996).
- Nabelschnur: ~1–1,5 cm dick, 0,17–0,21 Windungen/cm (Strong 1994; de Laat 2005), in Längsrichtung kaum dehnbar
  (Pennati 2001, > 10 MPa bei starker Dehnung), unter Druck wie ein nasser Schwamm, federt über Sekunden zurück
  (Gervaso 2014, DOI 10.1016/j.jmbbm.2014.03.016).

**Umgesetzt:**
- **Die Hand war starr:** Blenders automatische Gewichtung hatte bei der kleinen Hand still versagt (kein Punkt hing an
  einem Knochen). Jetzt eigene Gewichte nach Abstand zu den Knochen – die Finger beugen sich wirklich.
- **Form:** Beugefalten als weiche Rinnen mit Hautwulst, Fünffinger- und Daumenfurche, Knöchelgrübchen, Speckfalte am
  Handgelenk, flache Fingerbeeren, Nägel mit Nagelwall; Haut nach Woche (dünn und rötlich → Fett → Käseschmiere in den
  Falten ab SSW 36); Größe nach Hebbar.
- **Greifen wie eine echte Hand:** Jedes Fingerglied ist eine Kapsel; jedes Gelenk beugt sich nur, bis sein Glied die
  Schnur berührt, dann beugen sich die äußeren weiter – die Finger legen sich um die Schnur, nie hindurch.
- **Die Nabelschnur ist weich:** Skelettnetz mit 37 Knochen, eigener Löser (Länge und Biegung bleiben, Fruchtwasser
  bremst, Enden fest, kehrt über Sekunden in die Form zurück). Hand und Finger schieben sie weg; wo sie drücken, gibt
  die Sulze nach (Delle im Material, klingt über Sekunden ab). Hält die Hand sie, zieht sie das Stück mit.
- **Bewegung:** Hand als Körper im Wasser (gedämpfte Feder), Drehung um den Unterarm, Eigenbewegung nach
  Schlaf- und Wachzustand, Hand zum Mund mit langem Abbremsen, Finger außen weicher als innen, Daumen am Termin öfter
  eingeschlagen. Nach 3–7 s Greifen lässt die Hand von selbst los.
- Geprüft im gebauten Spiel (GPU 4,2 ms): Greifen mit 4 Fingern an der Schnur, Loslassen nach einigen Sekunden.

![Die Hand greift die Nabelschnur](Media/GENESIS-044_2a_Greifen.png)
![Die Hand aus Blender: Falten, Nägel, Grübchen](Media/GENESIS-044_2a_Hand_Blender.png)

**Offen (Teil 2b):** Die Fingerkuppen wirken noch knopfartig; Nägel, Hautfalten und Lanugo fehlen – kommt mit dem
vollständigen Fetusmodell. Die Mutter reagiert noch nicht auf das Daumenlutschen oder Schlucken (Nagy 2021: Berührung
der Mutter → das Kind saugt mehr).

![Die eigene Hand, SSW 31 – das Fruchtwasser schmeckt nach Knoblauch](Media/GENESIS-044_2a_Hand_Geschmack.png)
![Ihre Hand auf dem Bauch – ein Schatten im Licht](Media/GENESIS-044_2a_IhreHand.png)

## Teil 2b – das Kind in jeder Woche, von außen, dann in seine Augen (umgesetzt, ALPHA)

**Grundmodell:** Ein anatomisch sauberer Säugling aus MPFB2 (MakeHuman; Blender-Erweiterung von extensions.blender.org,
erzeugte Menschen CC0 – vom Game Director freigegeben). `Tools/Blender/Gestation/build_fetus.py` passt ihn je SSW an:
- **Maße** (50. Perzentile): Scheitel-Steiß-Länge (Robinson & Fleming 1975; ab 16 SSW Archie 2006), Kopf-, Bauchumfang
  und Femur (INTERGROWTH-21st, Papageorghiou 2014), Humerus und Fuß (Chitty & Altman 2002), Hand ≈ 0,8 × Fuß. Kopf zu
  Bauch 1,22 (SSW 12) → 0,95 (Termin) ergibt sich daraus.
- **Fett** nach Woche (Makro „weight“), **Lider geschlossen**, Finger locker gebeugt.
- **Haltung** (Williams Obstetrics): Rücken gerundet, Kinn zur Brust, eine Hand am Kinn, eine an der Wange, Oberschenkel
  seitlich am Bauch, Knie gebeugt, Unterschenkel gekreuzt. Wo sich Haut überschneidet, wird sie per Volumen-Neuvernetzung
  zu einer geschlossenen Falte.
- Je Woche (12–40) ein Netz und eine Tabelle mit Körpermitte, Nabel und 96 Hüllpunkten.

**Im Spiel:** Jeder Moment beginnt mit dem Kind von außen (die Kamera sieht es wie im 3D-Ultraschall durch die leuchtende
Wand), nach 9 s fährt sie in seine Augen. Das Kind liegt längs in der Gebärmutter, Gesicht zum Bauch; ab SSW ~32 dreht es
sich mit dem Kopf nach unten (Beckenendlage bei SSW 28 ~20 %, am Termin 3–4 %, RCOG 2017). Die Nabelschnur endet an
seinem Nabel.

**Fehler gefunden (Game Director: „die Füße hängen aus der Fruchtblase“):**
- Die Höhle war zu klein: SSW 28 nur 23 cm lang. Richtig ~28 cm (Symphysen-Fundus-Abstand ≈ SSW in cm), am Termin ~36 cm
  innen – das gebeugte Kind (längste Ausdehnung ≈ Scheitel-Steiß-Länge) füllt sie dann fast aus.
- Die Höhle war vorn–hinten am längsten statt von oben nach unten.
- Das Kind war am Blick statt an seiner Körperachse ausgerichtet. Jetzt liegt die Hauptachse der Hülle längs in der Höhle;
  eine Einpassung schiebt es, bis kein Hüllpunkt mehr herausragt (am Termin bleiben ~6 % – es ist wirklich eng).
- Die Beine steckten im Bauch (Skinning kennt keine Kollision): Haltung „Froschbeine“ und geschlossene Haut.

**Geprüft** im gebauten Spiel (SSW 36): außen, dann Ich-Sicht mit eigener Hand; GPU 2,8–4,3 ms. 158 von 158 Tests.

**Offen:** Knie von vorn noch etwas flach; Lanugo, Käseschmiere und Haut je Woche auf dem ganzen Körper; SSW 8–10 (der
Embryo, bisher nur in der Fruchthöhle); Gesicht mit offenen Augen ab SSW 28.

![Das Kind von außen, SSW 36, Kopf unten](Media/GENESIS-044_2b_Kind_aussen.png)
![Das Kind aus Blender, SSW 36](Media/GENESIS-044_2b_Fetus_Blender.png)

## Teil 2c – die frühen Wochen von außen (2026-09-24, ALPHA)

**Lücke:** Nach der Fruchthöhle (Tag 28 nach der Befruchtung = SSW 6) lief die Schwangerschaft bis SSW 19 als Zeitraffer
hinter geschlossenen Lidern – die Wochen 6–18 waren nicht zu sehen.

**Neue Momente, nur von außen** (das Kind nimmt vor SSW 19 nichts bewusst wahr; `bOnlyFromOutside`, die Kamera kreist
über den ganzen Moment und fährt nicht in die Augen):

| SSW | Zeile | Grundlage |
|---|---|---|
| 10 | „Es bewegt sich – und niemand spürt es“ | allgemeine Bewegungen und Schreck ab ~8, Arme/Beine einzeln 9–10 (de Vries, Visser & Prechtl 1982, DOI 10.1016/0378-3782(82)90033-0); die Mutter spürt erst ab 18–20 |
| 12 | „Es gähnt, schluckt, legt die Hand ans Gesicht“ | Hand zum Gesicht 10–12, Gähnen ab 11, Saugen und Schlucken 12–13 (de Vries 1982) |
| 16 | „Sie spricht mit ihm – hören kann es sie noch nicht“ | Sie spricht ab SSW 16 abends mit dem Bauch (Doc 34); Hören ab 19 |

**Referenzen** (Wikimedia Commons, lokal in `ArtSource/Reference/Fetus`, nicht im Repository, Lizenzen in QUELLEN.md;
Download mit Freigabe des Game Directors): Fetus SSW 10 mit und ohne Fruchtblase (drsuparna, CC BY-SA 2.0), Fetus
10–12 Wochen (lunar caustic, CC BY 2.0), Embryo 9. Woche und 7. Woche (Ed Uthman, CC BY 2.0). Merkmale SSW 9–10: Kopf
etwa die halbe Länge, die Augen als dunkle Pigmentflecke durch dünne Lider, schlanke, deutlich getrennte Gliedmaßen,
eine Hand am Mund, durchscheinend rosige Haut ohne Fett.

**Abgleich gegen die Referenzen – Fehler im Modell gefunden** (auch in SSW 12–16, die schon im Spiel waren):
- Die Arme liefen hinter bzw. durch den Rumpf: Der Ellbogen-Zielpunkt der IK hing am Kopf-Rahmen – bei gebeugtem Kopf
  zeigte „unten“ nach hinten. Jetzt im Rahmen des Körpers (unten-außen-vorn): Hände vor Mund und Kinn.
- Der Kopf war so stark gebeugt, dass man von vorn den Scheitel sah; Hals und Kopf jetzt weniger (Gesicht sichtbar).
- Das Kind hockte: Hüfte 95° gegen einen um ~50° vorgebeugten Oberkörper. Jetzt 115° (Knie am Bauch) und Knie 138°
  (Unterschenkel an den Oberschenkeln) – die fetale Haltung als Walze.

**Nachtrag – die Augen:** Das Pigment der Netzhaut bildet sich ab Woche ~6 nach der Befruchtung; die Lider verkleben um
SSW 10–12 und öffnen sich erst um SSW 26–28 (Moore, *The Developing Human*). Durch die dünnen Lider scheinen die Augen
als dunkle Flecke durch – so auf allen Referenzen von SSW 9–12. Im Material jetzt ein weicher dunkler Fleck an der Lage
der Augen (halber Augenabstand je Woche aus Blender, `fetus_eyes.py`), voll bis SSW 12, verblassend bis SSW 22.
Die Kamera kreist in den frühen Momenten von der anderen Seite – die Schlinge der Nabelschnur verdeckte sonst das Gesicht.

![SSW 12 von außen, im Kaltlicht – gebautes Spiel](Media/GENESIS-044_2c_SSW12_aussen.png)

### SSW 8 – der Embryo (Nachtrag 2026-09-24, ALPHA)

Das MakeHuman-Baby taugt für SSW 8 (6 Wochen nach der Befruchtung, Carnegie 17–18) nicht: In dieser Woche hat das Kind
einen Kopf fast so groß wie der Rumpf, seitlich stehende Augen mit dunklem Pigment und noch ohne Lider, Ohrhöcker statt
Ohrmuscheln, Hand- und Fußplatten mit Fingerstrahlen, einen Schwanzrest und eine große Herz-Leber-Wölbung; der Darm
tritt in den Nabelstrang aus (Moore, *The Developing Human*, Kap. 5). Eigenes Modell: `Tools/Blender/Gestation/build_embryo_w08.py`
(Metaballs, groß gebaut und auf 1,55 cm verkleinert, Voxel-Remesh, auf 200 000 Dreiecke ausgedünnt).

**Nachgezeichnet und deckungsgleich geprüft:** Landmarken aus der Seitenansicht „Human Embryo – approximately 8 weeks“
(lunar caustic, CC BY 2.0; lokal in `ArtSource/Reference/Fetus`). Blender rendert das Modell orthografisch im Pixelraum
des Fotos und legt beide übereinander (Umriss in Cyan). Gefunden und behoben: Kopf oben und vorn ~20 px zu groß,
Herz-Leber-Wölbung ~50 px zu hoch, Nacken mit Stufe, Schwanzrest zu lang. Danach liegt der Umriss auf ~10 px (0,2 mm).
Von vorn standen Arme und Beine erst als flache Flügel ab, dann verschwanden die Hände im Rumpf – jetzt liegen die
Handplatten flach an der Flanke vor der Brust, die Fußsohlen einander zugewandt.

**Im Spiel:** Moment „Woche 8 – Anderthalb Zentimeter, die Finger zeichnen sich ab“, nur von außen. Das Augenpigment hat
je Alter eine eigene Blickachse (`EyeSideways`): beim Embryo seitlich, kleiner und scharf begrenzt. Die Außenkamera
schließt die Blende mit der Größe des Kindes und wirkt draußen bleibend wie ein Fetoskop (kleiner Sensor, große
Schärfentiefe) – vorher war beim Embryo nur das Auge scharf.

**Herz und Leber scheinen durch (Nachtrag):** Frisch ist der Embryo durchscheinend – beim Embryo der 7. Woche liegt eine
dunkle Masse unter der Herz-Leber-Wölbung, in der 9. Woche ist der Bauch rosig-rot, Kopf und Glieder bleiben blass
(Referenzen Ed Uthman, CC BY 2.0). Im Material jetzt je Alter Lage und Radius von Herz und Leber (aus dem Modell,
`organs` in fetus_weeks.json → `HeartCm`/`LiverCm`): dunkelrot in der Grundfarbe, und Blut schluckt auch das
Durchlicht und das im Gewebe gestreute Licht. Drei Messfehler auf dem Weg: (1) Die Streufarbe des Subsurface-Modells
leuchtet unabhängig von der Grundfarbe und hatte die Organe fast ganz überdeckt. (2) Ein beleuchteter Maskentest
täuscht: Bei hellem Kaltlicht erscheinen schon 10 % Maske als volles Rot – die Maske erreichte die Haut kaum. Die Organe
liegen jetzt mitten unter den Wölbungen, voll bis 0,9 × Radius. (3) Die kleine Höhle (1,5 cm) wirft das Kaltlicht wie
eine Ulbrichtkugel zurück: Haut-Median 0,86 statt 0,80–0,83 (SSW 12, Referenzfotos) – die Belichtung nimmt dort bis
0,4 EV zurück. Die Kamera kreist beim Embryo von der Seite (wie auf den Referenzen); von vorn verdeckte der Nabelstrang
die Wölbung.

**Die Gefäße der Nabelschnur von vorn (Nachtrag):** Frisch (Referenz SSW 10, drsuparna, CC BY-SA 2.0) liegen die zwei
Arterien und die Vene als rötliche Schlingen in der durchscheinenden Wharton-Sulze. Bisher schluckten sie nur das
Durchlicht von hinten (Ich-Sicht); unter Licht von vorn – Kaltlicht in der Außenansicht – war die Schnur deckend weiß.
Jetzt färben sie auch Grund- und Streufarbe, weich und gedämpft violett-rot (tief in der Gallerte), die Sulze etwas
weniger weiß. Ein erster Versuch mit voller Stärke sah aus wie eine Zuckerstange – zurückgenommen.

**Noch offen:** Gefäße am Kopf. Physiologischer Nabelbruch bei SSW 10–12 und der Dottersack fehlen. Das Kind ist
starr: Bewegungen zeigt es von außen nur als Ganzkörper-Ruck. Frisch ist das frühe Kind deutlich rosiger als unsere
Haut (Referenz SSW 10 frisch gegen die Präparate in Alkohol).

![SSW 8 von außen, im Kaltlicht – gebautes Spiel](Media/GENESIS-044_2c_SSW08_aussen.png)
