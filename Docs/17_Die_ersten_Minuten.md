# 17 – Die ersten Minuten

Mit dem ersten Atemzug ist die Geburt vorbei, aber das Leben fängt nicht bei null an.
Ein Neugeborenes ist weder blind noch hilflos: Es sieht auf Armlänge scharf, es erkennt die Stimme,
die es neun Monate lang gehört hat, und es findet die Brust von selbst, wenn man es lässt.
Was es nicht kann, ist seine Wärme halten – das ist in der ersten Stunde die eigentliche Gefahr.

In dieser Stunde entsteht außerdem etwas, das ein Leben lang trägt: **die erste Erinnerung.**
Kein Bild und kein Satz, sondern Wärme, ein Herzschlag und eine vertraute Stimme.

Plugin: `GenesisEarlyLife` (Logik ohne Welt, Subsystem auf der Weltuhr, Anschluss an Geburt, Gedächtnis und Seele).

## Die Stufen

| Stufe | Wann | Was geschieht |
|---|---|---|
| erste Atemzüge | 0–2 min | Der Atem kommt in Gang, alles ist zu hell und zu laut |
| ruhige Wachheit | bis ~55 min | Das Kind ist so aufmerksam wie danach lange nicht mehr |
| Haut an Haut | sobald es auf der Mutter liegt | Wärme, Herzschlag, Geruch – die Ruhe steigt |
| erstes Anlegen | ab ~32 min, wenn es ruhig ist | Der Brustkrabbelgang: Das Kind findet die Brust allein |
| erster Schlaf | wenn satt, warm und müde | Die erste Stunde endet |
| unterkühlt | unter 35,5 °C | Ohne Wärme von außen endet die erste Stunde gefährlich |

## Wärme entscheidet

Ein Neugeborenes hat viel Oberfläche und wenig Masse, es ist nass und kann nicht zittern.
Die Temperatur folgt dem **Newtonschen Abkühlungsgesetz**: Die Änderung richtet sich nach dem Gefälle,
nicht nach der Uhr. Deshalb fällt sie anfangs steil und flacht dann ab – und das Aufwärmen dauert
umso länger, je näher das Kind der Hauttemperatur der Mutter kommt.

**Gemessen** (Kreißsaal 24 °C, Start 37,2 °C):

| Nach 20 Minuten | allein auf dem Wickeltisch | auf der Haut der Mutter |
|---|---|---|
| Körpertemperatur | 33,0 °C (unterkühlt) | 37,1 °C |
| Ruhe | fällt gegen 0 | 1,00 |

Wird ein ausgekühltes Kind zurück auf die Haut gelegt, steht es nach 45 Minuten wieder bei 36,5 °C.
Das ist kein Schalter, sondern eine Kurve – und genau deshalb ist die Entscheidung, das Kind hinzulegen
oder es liegen zu lassen, im Spiel etwas wert.

## Bindung entsteht aus dem, was geschieht

Bindung wächst nicht dadurch, dass Zeit vergeht, sondern aus Hautkontakt, Stimme und Sattsein –
und mit Sättigung: Eine perfekte erste Stunde **legt** eine Bindung an, sie vollendet sie nicht.
Ohne diese Bremse stünde nach einer halben Stunde 1,00, und der Rest des Lebens könnte nichts mehr beitragen.

**Gemessen nach 45 Minuten:**

| | Haut und Stimme | allein |
|---|---|---|
| Bindung | 0,71 | 0,00 |
| geschrien | 1 min | 45 min |

## Was das Kind wahrnimmt

`FGenesisNewbornPerception` geht an Kamera, Nachbearbeitung und Ton – nichts davon ist ein Kameraeffekt,
alles kommt aus der Simulation:

- **Schärfe auf 250 mm**: genau die Entfernung zu einem Gesicht auf dem Arm.
- **Sehschärfe 0,04–0,05**: entspricht etwa 20/400. Alles außerhalb einer Armlänge verschwimmt.
- **Blendung**: In der ersten Minute 1,00, nach sechs Minuten 0,15. Das Auge gewöhnt sich schnell.
- **Vertraute Stimme**: Die Stimme der Mutter ist von Anfang an bei 0,55 – sie ist aus dem Mutterleib
  bekannt – und wächst mit der Bindung.

## Die erste Erinnerung

Mit dem ersten Anlegen legt das Subsystem eine Spur im Causal Memory Graph an:

- Ereignis mit den Themen `Theme.Care` und `Theme.Belonging`, Tragweite 0,85.
- Kodierung aus der Perspektive dessen, **dem** etwas geschieht (`Target`), mit den Sinnesankern
  `Sense.Touch`, `Sense.Smell`, `Sense.Hearing` – kein Sehen, keine Sprache.
- Aufmerksamkeit und Stimmung folgen Ruhe und Bindung: Ein Kind, das in dieser Stunde allein liegt,
  legt eine andere erste Erinnerung ab als eines, das auf der Haut liegt.
- Die Seele verstärkt daraufhin ihre Resonanz auf `Theme.Care` – der Anfang eines Musters,
  das über Inkarnationen hinweg zieht.

Danach wechselt die Lebensphase auf `LifePhase.Childhood`; die Seelenmusik wechselt hörbar die Instrumentierung.

## Bedienung (Entwickler)

```
genesis.Body.Conceive 0.8         # Körper zeugen
genesis.Clock.SkipDays 273        # bis zum Termin
genesis.Birth.Start               # Wehen beginnen
genesis.Birth.Speed 900           # Geburt im Zeitraffer
genesis.Newborn.SkinToSkin 1      # Kind auf die Haut legen
genesis.Newborn.Voice 1           # mit dem Kind sprechen
genesis.Newborn.Speed 2           # 2 Simulationsminuten je Sekunde
genesis.Debug.Page Newborn        # HUD-Seite „Die ersten Minuten"
```

![Die erste Stunde](Media/GENESIS-022_FirstHour.png)

## Offen

- Kreißsaal, Mutter und Gestalt sind weiterhin Platzhalter – die Kamera zeigt die Wahrnehmung,
  aber es gibt noch nichts zu sehen (eigener Block).
- Ton der ersten Stunde: Stimme, Raumklang, Schreien fehlen (GENESIS-013/015).
- Nabelschnur, Nachgeburt, Apgar nach einer und fünf Minuten.
- Erstes Wiegen und Messen, Vitamin K, Wärmebett als Alternative zur Haut.
