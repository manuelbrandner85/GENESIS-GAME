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
