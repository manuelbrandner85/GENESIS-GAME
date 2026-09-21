# 14 – Embryo: die erste Woche

Von der Verschmelzung bis zur Einnistung vergehen etwa neun Tage. In dieser Woche entscheidet sich,
ob aus dem Keim ein Mensch wird – und mit welchen Voraussetzungen. GENESIS simuliert sie in echten Zeiten
und echten Größen, nicht als Zwischensequenz.

Plugin: `GenesisEmbryo` (Logik ohne Welt, Subsystem auf der Weltuhr, Darstellung als Actor).

## Der Ablauf

| Zeit | Stufe | Was geschieht |
|---|---|---|
| 24–30 h | Furchung | Die erste Teilung. Kräftige Zellen teilen sich früher – ein früher erster Schnitt gilt auch in der Medizin als gutes Zeichen. |
| 34–72 h | Furchung | Alle 11–17 h teilt sich jede Zelle. **Der Keim wächst dabei nicht**: Aus einer Zelle von 110 µm werden viele kleine im selben Raum. |
| ab 8 Zellen | Morula | Die Zellen verzahnen sich (Kompaktierung), aus dem Haufen wird ein Verband. |
| ab 16 Zellen | Blastozyste | Flüssigkeit sammelt sich im Inneren. Die Zellen trennen sich in **Embryoblast** (daraus wird der Mensch) und **Trophoblast** (daraus werden Mutterkuchen und Eihäute). |
| Tag 5 | Blastozyste | Der Keim dehnt sich um etwa ein Drittel aus – zum ersten Mal überhaupt wächst er. Die Zona wird dünner. |
| Tag 5–6 | Schlüpfen | Die Zona reißt auf, der Keim schlüpft heraus. |
| Tag 6–9 | Einnistung | Der Trophoblast dringt in die Gebärmutterschleimhaut ein. |

**Gemessen im Test** (Genom-Seed 11, Lebenskraft 0,8):

| Zeit | Zellen | Stufe |
|---|---|---|
| 34 h | 2 | Furchung |
| 72 h (Tag 3) | 11 | Morula, Kompaktierung 79 % |
| 120 h (Tag 5) | 66 | Schlüpfen, Embryoblast 22 Zellen |
| 144 h (Tag 6) | – | Einnistung, Zona aufgebraucht |
| 240 h (Tag 10) | – | eingenistet, Qualität 0,92 |

Zum Vergleich: Eine menschliche Blastozyste hat an Tag 5 etwa 60–120 Zellen.

## Was die erste Woche für das Leben bedeutet

- **Entwicklungsqualität**: Unsaubere Teilungen hinterlassen Zelltrümmer (Fragmentierung). Die Qualität am Ende
  der Woche geht in die **Organanlagen** des Körpers ein (`FGenesisOrganState::DevelopmentQuality`) – ein Mensch
  startet also nicht bei null, sondern mit dem, was diese Woche ergeben hat.
- **Lebenskraft der erfolgreichen Samenzelle** entscheidet mit: kräftig → 3 % Fragmentierung, Qualität 0,98;
  schwach → 50 % Fragmentierung, Qualität 0,65 (gemessen).
- **Stillstand**: Biologisch kommt nur etwa die Hälfte aller befruchteten Eizellen bis zur Einnistung. Das Risiko
  liegt in den ersten Tagen, wenn das eigene Erbgut die Entwicklung übernimmt (Genomaktivierung im 4- bis 8-Zell-Stadium).
  Gemessen bleiben 38 % der fremden Keime stehen. **Der Keim des Spielers ist davon ausgenommen** – sonst gäbe es
  kein Leben zu spielen. Das ist eine bewusste Entscheidung, keine vergessene Regel.

## Darstellung

Die Zellen der Simulation werden als Instanzen der Eizell-Kugel dargestellt (55 µm Radius, Skalierung = Radius/55).
Zwei Dinge machen aus Kugeln einen Keim:

- **Kompaktierung**: Die Zellen dürfen sich überlappen und werden leicht flachgedrückt.
- **Trophoblast**: In der Blastozyste liegen die äußeren Zellen flach an der Hülle (Höhe 40 %, Breite entsprechend
  größer, Volumen bleibt). Als Kugeln blieben Lücken, durch die man in den Keim sähe – ein Deckgewebe hat keine Lücken.

Die Eizelle selbst verschwindet Stück für Stück: Der Zellkranz löst sich in den ersten 20 Stunden auf,
der Polkörper zerfällt in den ersten drei Tagen, die Zona bleibt bis zum Schlüpfen.

![Morula in der Zona](Media/GENESIS-020_Morula.png)
![Blastozyste nach dem Schlüpfen](Media/GENESIS-020_Blastocyst.png)

## Entwicklerbefehle

- `genesis.Embryo.Advance <Stunden>` – führt den Keim weiter (die Weltuhr wäre sonst 24 Minuten je Tag).
- `genesis.Embryo.Start [Lebenskraft]` – legt einen Keim ohne Zeugung an.
- `genesis.Debug.Page Embryo` – Stufe, Zellzahl, Kompaktierung, Hohlraum, Zona, Einnistung, Qualität.

## Offen (bewusst, nicht versteckt)

- Die Zellen wirken noch **porzellanartig hell**; echte Blastomeren sind grauer und unregelmäßiger geformt.
- **Zelltrümmer** werden als Zahl geführt, aber noch nicht dargestellt.
- Die **Einnistung** läuft als Fortschrittswert – die Gebärmutterschleimhaut als Ort gibt es noch nicht.
- Die Entwicklung nach der Einnistung übernimmt die Körpersimulation in Wochenschritten; die Organbildung
  im Detail (GENESIS-021 und später) ist noch nicht ausgearbeitet.

## GENESIS-038 (Teil 1) – Die erste Woche wird sichtbar

**Befund:** Im Level der Befruchtung gab es **keinen Keim-Actor**. Beim letzten Neuaufbau der Szene war er
verloren gegangen (das Szenenskript rief das Keim-Skript nicht mehr auf). Die erste Woche lief seitdem
unsichtbar: Man sah die unbefruchtete Eizelle samt Cumulus, zehn Tage lang, in zehn Sekunden.
Dazu kam ein Material, das nicht kompilierte (Drehmatrix fehlte) – das Spiel zeigte das Ersatzraster.

![Vorher: Cumulus bei 35 hpi, der Keim unsichtbar](Media/GENESIS-038_Vorher_Cumulus.png)

![Zwischenstand: Keim da, Material defekt (Raster)](Media/GENESIS-038_Vorher_Raster.png)

![Jetzt: Morula an Tag 3 mit Laborbeschriftung](Media/GENESIS-038_Morula.png)

Behoben und neu:
- Keim-Actor wieder im Level; `setup_oocyte.py` setzt ihn jetzt immer mit.
- Material kompiliert und ist für Instanzen freigegeben.
- **Zeitraffer wie im EmbryoScope:** Zygote 3 h/s, Teilungen bis zum Schlüpfen 1,6 h/s, Einnistung 10 h/s –
  bis zur Einnistung 75 s statt 10 s (Test `Genesis.Slice.EmbryoTimeLapse`).
- **Beschriftung** wie im Brutschrank: „70,5 hpi · Tag 3 · 14 Zellen" und ein Satz zum Geschehen.
- **Zelltrümmer** sichtbar (bis 60 Fragmente, 1,5–5 µm, im Spalt unter der Zona).
- Der ganze Cumulus (Zellkranz, Gallerte, Fäden) löst sich in 20 h auf; die übrigen Spermien sind nach
  30 h fort.
- Kamera auf den Keim (260 µm, Makro ×3, f/22 – der ganze Keim scharf; vorher 18 µm Schärfentiefe).

**Noch nicht AAA (Teil 2):** Die Zellen sind glatt und rosa wie Kunststoff. Echte Keime sind im
Hoffman-Kontrast grau-beige, durchscheinend, körnig, mit sichtbaren Kernen, Vorkernen und Polkörper.
Die Zeiten der Simulation weichen von den Klinikdaten ab (erste Teilung 24 h statt Median 25,8 h, 8 Zellen
bei 56 h statt 58,7 h, Morula mit 8 statt nach Kompaktierung bei ~89 h, Blastozyste bei 74 statt ~100–110 h).
Beides ist der nächste Schritt – Zahlen siehe [26 – Recherche und Plan](26_Recherche_und_Plan_AAA.md).