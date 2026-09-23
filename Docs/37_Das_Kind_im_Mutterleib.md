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
