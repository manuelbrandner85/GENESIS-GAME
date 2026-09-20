# 13 – Entstehung: Mikrowelt (Eileiter, Spermien)

Erste sichtbare Szene von GENESIS. Plugin `GenesisConception`, Blender-Assets in `Tools/Blender/Conception`, Unreal-Aufbau über `Tools/Unreal/Conception/setup_conception_scene.py`.

## Maßstab

**1 µm = 1 Unreal-Einheit (cm) = 0,01 m in Blender (Faktor 10.000).**

Unreal rechnet in Zentimetern mit einfacher Gleitkommagenauigkeit; eine 60 µm lange Zelle in echten Metern wäre 0,00006 m und würde an Near-Clip, Lumen, Schattenkarten und Tiefenschärfe scheitern. Alle **Proportionen bleiben real**, nur die Einheit ist verschoben. Folgen, die bewusst mitgerechnet werden:

| Größe | Umrechnung |
|---|---|
| Längen | × 10.000 |
| Beleuchtungsstärke bei gleicher Lampe | ÷ 10⁸ (Abstandsquadrat) → Lichtleistung entsprechend hochskalieren |
| Tiefenschärfe | Bei realer Brennweite/Blende entspricht die Schärfentiefe im Bild der eines Makro-/Mikroskopobjektivs |

## Anatomische Referenz

| Objekt | Maße (Mensch) | Umsetzung |
|---|---|---|
| Spermienkopf | 4,6 × 2,9 × 1,4 µm, birnenförmig im Profil, Akrosom über ~55 % | `SM_GEN_SpermCell`, 21.600 Flächen |
| Hals, Mittelstück | ~1 µm, ~5 µm mit spiraliger Mitochondrienscheide (~12 Windungen), Anulus | Geometrie mit unregelmäßigen Einzel-Mitochondrien |
| Haupt- und Endstück | ~45 µm bzw. ~5 µm, Ø 0,6 → 0,06 µm | Verjüngung, Gesamtlänge 60,6 µm |
| Ampulla-Lumen | mehrere mm, fast ausgefüllt von Schleimhautfalten | Abschnitt 1.500 µm, Wandradius 1.150 µm, freier Kanal 450 µm |
| Primärfalten | 8–10, bis ~0,6 mm hoch, blattartig 50–120 µm dick | 9 Falten, wellig, geneigt, mit Sekundärfalten |
| Epithel | einschichtig, Zellen ~10–12 µm, Flimmer- und sekretorische Zellen | Zellmosaik im Material (Relief + Helligkeit) |

## Schwimmmodell (`GenesisSpermSwimLogic`)

Deterministisch, zustandslos, in µm und Sekunden. Bewegungsarten nach CASA-Kategorien:

| | Vortrieb | Schlag | Kopfauslenkung (ALH) | Rotationsdiffusion |
|---|---|---|---|---|
| progressiv | 30–55 µm/s | 12–18 Hz | 2,5–5 µm | 0,08 rad²/s |
| hyperaktiviert | 8–20 µm/s | 7–10 Hz | 9–14 µm | 1,5 rad²/s |
| träge (geringe Vitalität) | 3–12 µm/s | 3–7 Hz | klein | 0,16 rad²/s |

Verhalten:
- **Rotationsdiffusion:** keine geraden Bahnen.
- **Rheotaxis:** Ausrichtung gegen den Zilienstrom (der Richtung Gebärmutter fließt) – der Strom weist den Weg zum Eierstock. Er ist an der Wand am stärksten, steht aber auch in der Mitte des Lumens nicht still (45 % des Wandstroms). Das ist entscheidend: Ohne Strömung in der Mitte hätten die Zellen dort **keinen Hinweis, wohin**, und die Hälfte schwämme rückwärts (korrigiert in GENESIS-025).
- **Wandbindung:** Zellen richten sich an Oberflächen parallel aus und schwimmen leicht zur Wand geneigt (3,5°); dadurch sammeln sie sich dort. Hyperaktivierte lösen sich leichter (Faktor 0,35) – so kommen sie von der Schleimhaut wieder frei.
- **Hyperaktivierung** wechselt zufällig (Raten je Sekunde, 0,05 hin / 0,045 zurück – in der Ampulle zum Eisprung ist gut die Hälfte der Zellen kapazitiert, sie haben sich aus dem Reservoir im Isthmus gelöst).

**Gemessen (Automationstests):**

| Messwert | Ergebnis | Referenz |
|---|---|---|
| Progressiv VSL | 43,2 µm/s | WHO: schnell progressiv ≥ 25 µm/s |
| Progressiv VCL | 121 µm/s | CASA typisch 60–150 µm/s |
| Hyperaktiviert VSL | 23,6 µm/s | peitschend, aber mit Vortrieb |
| Hyperaktiviert VCL | 225 µm/s | CASA-Kriterium ≥ 150 µm/s |
| Hyperaktiviert LIN | 0,11 | CASA-Kriterium < 0,5 |
| Wandnähe nach 40 s | 98 % progressiv, 32 % hyperaktiviert | Gleichverteilung wäre 23 % |
| Rheotaxis an der Wand | Ausrichtung 1,00 stromaufwärts, +487 µm in 30 s | ohne Rheotaxis −796 µm (Abtrift) |
| **Mitte des Lumens** | Ausrichtung 0,99, +773 µm in 30 s, **0 von 200 rückwärts** | vorher: rund die Hälfte rückwärts |
| Performance | 187 ns je Zellschritt → 1.000 Zellen ≈ 0,19 ms/Frame | |

## Darstellung

- `AGenesisSpermSwarm`: Simulation in festen Schritten (1/240 s Simulationszeit), Darstellung über Instanzen mit Per-Instance-Daten (Schlagphase, Amplitude, Asymmetrie, Wellenlänge). Der Geißelschlag entsteht im Material (World Position Offset), nicht in der Geometrie.
- **Zeitlupe 1/4 als Standard:** Ein Schlag von 15 Hz wäre bei 60 Bildern/s nur 4-fach abgetastet und würde stroboskopisch flackern. Reale Spermien werden ebenfalls mit Hochgeschwindigkeitskameras (100–500 fps) gefilmt; die Szene entspricht einer solchen Aufnahme.
- `AGenesisMicroscopeCameraRig`: Vollformat-Kamera (36 mm breit, 16:9-Ausschnitt), 85 mm, f/8, weich gedämpfte Nachführung (Kamera mit Masse), Schärfenachführung wie ein Fokus-Assistent.
- **Licht:** Im Körper gibt es kein Licht. Einzige Quelle ist ein Endoskop-Licht an der Optik (5.600 K, 3.000 cd), dadurch natürlicher Abfall in die Tiefe.

## Assets

| Asset | Herkunft | Inhalt |
|---|---|---|
| `SM_GEN_SpermCell` | `build_sperm_cell.py` | Zelle mit Farbattribut „Zones“ (Akrosom, Mittelstück, Kopf, Position) |
| `SM_GEN_OviductWall` | `build_oviduct_wall.py` | Kachelbarer Abschnitt 1.500 µm, 3,86 Mio. Flächen, Nanite, Farbattribut „Tissue“ (Spalttiefe, Faltenhöhe, Variation) |
| `M_GEN_SpermCell` | Unreal-Skript | Durchscheinende Zelle, optische Dichte je Region, Geißelschlag als WPO |
| `M_GEN_OviductMucosa` | Unreal-Skript | Subsurface-Gewebe, Kapillarnetz, Epithel-Relief |
| `SM_GEN_Debris_*`, `SM_GEN_MucosaFolds`, Eizelle | Trailer-Session (gemeinsam genutzt) | Schwebeteilchen, flache Faltenplatte, Eizelle |

Erzeugte Zwischendateien (FBX, Look-Dev-Renders, .blend) liegen unter `ArtSource/Generated/` und sind **nicht** im Repository – sie entstehen deterministisch aus den Skripten.

## Physikalische Besonderheit: Zellen in Flüssigkeit

Zytoplasma (n ≈ 1,38) und Eileiterflüssigkeit (n ≈ 1,335) brechen Licht fast gleich. Der relative Brechungsindex liegt bei ~1,04, die Fresnel-Reflexion damit unter 0,1 %. **Zellen glänzen nicht**, sie sind glasig und werden nur durch Streuung im Inneren sichtbar (dichtes Chromatin im Kopf am stärksten). Gleiches gilt für die Schleimhaut: kein „nasser Glanz“ wie an Luft, deshalb Specular 0,02.

## Entwicklerbefehle

- `genesis.Debug.Page Conception` – Zustand des Schwarms (Bewegungsarten, Wandnähe, Ausrichtung, CPU).
- Szene starten: `Tools\Build\Capture-GameScreenshot.ps1 -Map "/Game/Genesis/Conception/Maps/L_GEN_OviductAmpulla"`.

## Flimmerhärchen (Kinozilien)

Real stehen Zilien etwa 0,3 µm auseinander – als Geometrie wären das Millionen Halme je Abschnitt. Sichtbar sind sie an zwei Stellen:
auf der **Silhouette** (Faltenkanten) als Saum und auf zugewandten Flächen als feines Flimmern. Deshalb:

- Halme 10 µm lang, 0,4 µm Fußdurchmesser, zur Spitze verjüngt, leicht geneigt – Abstand 2,5 µm nahe den Faltenspitzen, 6 µm in der Fläche.
- 81.456 Halme je Abschnitt (1,3 Mio. Flächen), nur auf Flächen, die in den freien Kanal zeigen.
- **Metachroner Schlag im Material:** Die Zilien schlagen nicht gleichzeitig, sondern als Welle über das Epithel (Wellenlänge 25 µm, 8 Schläge/s). Der Arbeitsschlag Richtung Gebärmutter ist schnell, die Rückholbewegung weich – daher die gerichtete Strömung, gegen die die Spermien anschwimmen.
- Kein Raytracing und keine Schatten für die Halme (bei 0,2 µm Dicke ohne sichtbaren Unterschied, spart Speicher).

**Gemessen im Spiel:** 5,53 ms je Bild (≈ 180 fps), GPU 3,71 ms, 71 Draw Calls, 10,5 Mio. Dreiecke sichtbar (RTX 5070, 1600 × 900).

## Gefundene und behobene Fehler (Sichtprüfung)

| Fehler | Ursache | Behebung |
|---|---|---|
| Geißel blieb starr | Positionsknoten liefern bei Instanzen die Lage im ganzen Kanal, nicht in der Zelle → die Welle verschob die Zelle nur | Position entlang der Zelle aus der UV-Koordinate |
| Zelle wie weißer Kunststoff | Zu heller Grundton, beidseitiges Rendern der dünnen Röhre, kein Randsaum | Einseitig, dunklerer Grundton, Randstreuung und Dichteverlauf nach der Blender-Referenz |
| Falten als schwarze Platten | Dünnes Gewebe wurde undurchsichtig gerechnet | Material für beidseitig durchscheinendes Gewebe (Durchleuchtung rötlich) |
| „Cordsamt“ auf dem Gewebe | Loft-Rippen und Voxel-Treppen | Feineres Raster, 6× Glättung, organische Störung |
| Harte Schnittflächen zwischen Abschnitten | Glättung verschob die Ränder | Randzone eingefroren, Störung läuft zu den Enden auf null |
| Graue Suppe im Bild | Flüssigkeitsstreuung viel zu stark | Nebel fast aus; Flüssigkeit zeigt sich über Schwebeteilchen |
| Graue Kugel in der Szene | Standard-Pawn der Engine | Pawn wird beim Start ausgeblendet |
| Kamera im Gewebe, Kopf hinter der Kamera | Kamera folgte der rollenden Schlagebene und wurde herumgeschleudert | Geglättete Schlagebene, langsamere Eigenrotation, Ausweichen ohne Abstandsverlust |

## Offen (bewusst, nicht versteckt)

- Das **Epithel-Mosaik** wirkt aus mittlerer Entfernung noch etwas textil; Zellgrößen und Flecken variieren zwar, aber echte Epithelien sind unregelmäßiger.
- **Zilien nur im Kanalbereich**: In tiefen Spalten fehlen sie (dort sieht man sie nicht, spart aber Geometrie).
- **Schwebeteilchen** sind sehr dezent (real sind sie es auch, aber sie tragen wenig zur Lesbarkeit der Flüssigkeit bei).
- Das Kapillarnetz im Blender-Referenzmaterial ist noch zu regelmäßig (wabenartig).
- Eizelle und Corona radiata stammen aus der Trailer-Session und sind für Nahaufnahmen noch nicht realistisch genug (glatte, gleichförmige Zellen ohne Cumulus-Matrix).

## Befruchtung (GENESIS-019)

### Die Eizelle

Maße aus der Anatomie einer reifen Eizelle (Metaphase II), Aufbau in `Tools/Blender/Conception/build_oocyte.py`:

| Teil | Maß | Bedeutung im Spiel |
|---|---|---|
| Ooplasma (Zellleib) | Radius 55 µm | Träger des mütterlichen Erbguts; körnig von Organellen |
| Perivitelliner Spalt | 3 µm | Darin der erste Polkörper (10 µm) – Zeuge der Reifeteilung |
| Zona pellucida | 58–72 µm (14 µm dick) | Hier binden die Spermien, hier wird gebohrt, hier schließt sich die Tür |
| Corona radiata | 950 Zellen bis 118 µm | Nährzellen; bremsen die Spermien und schirmen die Zona ab |

### Der Weg einer Zelle (Logik, ohne Zufall im Ablauf)

1. **Lockwirkung** – der Progesteron-Gradient aus dem Cumulus zieht im Umkreis von 220 µm: hyperaktivierte Zellen voll, progressive zu 45 %, träge zu 20 %. Eindringen kann trotzdem nur eine kapazitierte Zelle.
2. **Cumulus** – in der Gallerte fällt die Geschwindigkeit auf 55 %; die Zellen müssen sich hindurcharbeiten.
3. **Bindung** – an der Zona binden ausschließlich kapazitierte Zellen (0,85/s). Ohne Kapazitation gibt es keine Akrosomreaktion, also auch keine Befruchtung.
4. **Akrosomreaktion** – 2–6 s, bis die Kappe aufplatzt und die Enzyme frei sind.
5. **Durchdringung** – 0,35–1,2 µm/s durch 14 µm Zona, abhängig von Vitalität und Schlagkraft; 2 % je Sekunde bleiben stecken.
6. **Verschmelzung** – die erste Zelle, die durchkommt, verschmilzt. Genau eine.
7. **Cortikalreaktion** – die Zona härtet in 12 s aus; ab 15 % Fortschritt bindet keine Zelle mehr (Polyspermie-Block). Sichtbar als Farb- und Dichteänderung der Zona (Materialparameter `CorticalReaction`).

### Aus der Verschmelzung wird ein Mensch

`UGenesisConceptionSubsystem::Conceive` verbindet den Mikrokosmos mit der Lebenssimulation – genau einmal je Welt:

- zwei Elterngenome (Gründer) → Kindgenom über `UGenesisGeneticsSubsystem::ConceiveChild`
- Personen-ID deterministisch aus dem Genom (gleiche Welt → gleicher Mensch)
- erster Körper über `CreateBodyAtConception`: **Lebenskraft** aus der erfolgreichen Zelle, **Widerstandskraft** aus dem geerbten Genom (0,6 × geringes Herz-Kreislauf-Risiko + 0,4 × Stoffwechsel)
- Inkarnation der Spielerseele beginnt; fehlt die Seele, entsteht sie deterministisch aus dem Genom
- Leitmotiv erbt von beiden Eltern, Lebensphase `LifePhase.Conception` wird zum Soundtrack-Moment

**Im Spiel gemessen:** Zelle 179 verschmolzen nach 104,4 s Simulationszeit, Vitalität 0,73, 2 Mitbewerber an der Zona, 0 abgewiesen → Person `duTFBuywA9qu…`, Genom `gpwNSBjsoUX0…`, Lebenskraft 0,73, Widerstandskraft 0,64.

![Befruchtung mit Entwickleranzeige](Media/GENESIS-019_Fertilization.png)

### Entwicklerbefehle

- `genesis.Conception.WatchOocyte 1|0` – Kamera auf die Eizelle oder zurück auf eine Zelle.
- `genesis.Conception.Cam <Abstand µm> [Brennweite mm] [Blende]` – Bildausschnitt für Messreihen.
- `genesis.Conception.Light <cd>` – Endoskoplicht, `genesis.Conception.Exposure <EV>` – Belichtung.
- `genesis.Conception.TimeScale <x>` – Zeitraffer (Standard 0,25; die Befruchtung braucht sonst Minuten).
- `genesis.Debug.Page Oocyte` – Zustand der Eizelle, `genesis.Debug.Page Conceived` – das gezeugte Leben.
- `Tools\Build\Measure-Image.ps1 -Path <png>` – misst Leuchtdichte-Perzentile und ausgefressene Flächen eines Bildes.

### Was die Bildmessung gelehrt hat

| Beobachtung | Ursache | Behebung |
|---|---|---|
| Alles weiß, Materialänderungen ohne Wirkung | Die Belichtung der Kamera hing an der physikalischen Blende; ohne sie fehlten ~12 Blendenstufen | Kamera belichtet selbst (`ExposureBias`), physikalische Belichtung bleibt aktiv |
| Grün eingefärbte Probe kam weißlich an | Streulicht im Lichtkegel direkt vor der Optik (Nebeldichte 0,02, Streuung 2,5) | Nebel 0,006, Streuung 0,35, indirektes Licht 1,0, Licht 150 cd |
| Materialänderungen blieben unsichtbar | Das Skript löschte das Material und legte es neu an – das Level behielt die alte Fassung | Knoten im bestehenden Asset leeren (`delete_all_material_expressions`) |
| Corona wie Popcorn | Gleichmäßige Kugelschale, 80 Flächen je Zelle, ein Farbton für alle | Vier Lagen mit Ausdünnung nach außen, 320 Flächen je Zelle, Zufallston je Zelle im Farbattribut |
| Harte Silhouetten der Zellen | Undurchsichtiges Streumodell | Zweiseitiges Laubmodell: Licht tritt durch die 12 µm dünnen Zellen |
| Cumulus-Gallerte wie Plastikschale | Kugel mit harter Silhouette | Vorerst nicht gesetzt; der Cumulus entsteht aus den Zellen selbst |

![Eizelle im Eileiter](Media/GENESIS-019_Oocyte.png)

### Offen (bewusst, nicht versteckt)

- Die **Coronazellen wirken noch wie feste Körper**, nicht wie lebende, durchscheinende Zellen. Nächster Schritt: weichere Silhouetten (dichtere, überlappende Lagen), Unschärfe der äußeren Lage, Gallerte als volumetrisches Medium statt als Hülle.
- Das **Ooplasma** ist aus mittlerer Entfernung noch eine helle Fläche; die Granulation braucht größere Strukturen (Schlieren) und weniger Eigenhelligkeit.
- Die **entscheidende Nahaufnahme fehlt**: Gebundene Spermien stecken anatomisch richtig unter dem Zellkranz – dafür muss die Kamera *in* den Cumulus, zwischen die Zellen an die Zona.
- Die Hyaluronsäure-**Gallerte** ist gebaut (`SM_GEN_OocyteMatrix`), aber nicht in der Szene.

## Feinschliff der Eizelle (GENESIS-024)

### Die Optik war das eigentliche Problem

Im Mikrometerraum ist mit einer normalen Kameralinse **alles scharf**: Bei 430 µm Motivabstand und 50 mm Brennweite
liegt der Zerstreuungskreis bei Bruchteilen eines Bildpunkts. Genau daran erkennt das Auge sofort ein Modell –
jede echte Mikroskopaufnahme hat eine hauchdünne Schärfeebene.

Echte Makro-Optik (hier etwa 15:1) lässt sich mit einer dünnen Linse in Weltmaßstab nicht nachbilden. Der Ausweg:
**Sensor und Brennweite gemeinsam um den Faktor `MacroScale` vergrößern.** Das Verhältnis beider bestimmt den
Bildwinkel – der bleibt also gleich –, aber der Zerstreuungskreis wächst mit dem Quadrat der Brennweite und
wird nur linear auf den größeren Sensor bezogen. Unterm Strich: **Unschärfe × MacroScale bei identischem Bildausschnitt.**

| Einstellung | Wert | Begründung |
|---|---|---|
| Bildwinkel | wie 50 mm (Kleinbild) | Zelle bei 150 µm bildfüllend, Eizell-Komplex bei 430 µm zu 70 % |
| Makro-Faktor | 18 | Gewebe im Hintergrund löst sich auf, die Schärfeebene liegt auf dem Motiv |
| Blende | f/16 | wirkt über die physikalische Kamerabelichtung auch auf die Helligkeit |
| Belichtungskorrektur | 13 EV | gleicht den Maßstabssprung aus (im Mikrometerraum nur wenige Lux) |

### Corona radiata: von Popcorn zu Zellmasse

| Vorher | Nachher | Wirkung |
|---|---|---|
| 950 Zellen, eine Schale, Abstand 8,6 µm | 2.100 Zellen, fünf überlappende Lagen, Abstand 6,2 µm | zusammenhängende Masse statt einzelner Körner |
| Länge 10–17 µm, radial ausgerichtet (Streuung 0,35) | 8,5–13 µm, Streuung wächst mit der Lage (0,35 → 1,7) | kein Strahlenkranz mehr; nur die innerste Lage zeigt zur Zona |
| ein Farbton für alle | Zufallston je Zelle im Farbattribut (R-Kanal) | jede Zelle wirkt einzeln, die Masse bekommt Tiefe |
| undurchsichtiges Streumodell | zweiseitiges Laubmodell mit rötlicher Durchleuchtung | Licht geht durch die ~12 µm dünnen Zellen |
| keine Eigenverschattung | Zellen werfen Schatten | erst dadurch wirkt die Wolke räumlich |

### Was nicht geht – und warum es so bleibt

Eine Nahaufnahme der Bindungsstelle **von außen ist anatomisch unmöglich**: Der Cumulus ist dicht, gebundene Zellen
stecken darunter. Zwei Versuche (Kamera seitlich neben der Zelle, Kamera radial außerhalb mit Tele) endeten beide
mitten im Zellkranz, mit unscharfen Nachbarzellen im Bild. Echte Befruchtungsaufnahmen entstehen an Eizellen,
denen der Cumulus vorher enzymatisch entfernt wurde.

Deshalb die ehrliche Dramaturgie:

1. Die Kamera begleitet eine Zelle im Kanal.
2. Sobald eine Zelle an der Zona bindet, **wechselt sie auf diese Zelle** – man folgt ihr zwischen die Coronazellen.
3. Mit der Verschmelzung **schneidet sie auf die ganze Eizelle**: Dort läuft die Cortikalreaktion sichtbar ab.

### Gemessen

| Bild | Median | P99 | Reinweiß | Schwarz |
|---|---|---|---|---|
| Zellansicht (150 µm) | 0,29 | 0,81 | 0 % | 0 % |
| Eizelle nach der Verschmelzung | 0,33 | 0,76 | 0 % | 22 % (Kanal im Dunkeln) |

![Cumulus-Oozyten-Komplex](Media/GENESIS-024_Cumulus.png)
![Spermienzelle mit Makro-Schärfentiefe](Media/GENESIS-024_CellMacro.png)

### Offen (bewusst, nicht versteckt)

- Die **Cumulus-Gallerte** wurde als lokales Nebelvolumen getestet und wieder entfernt: zwischen Extinktion 3,5
  und 45 lag im gemessenen Bild kein Unterschied (Median 0,310 gegen 0,309). Ein wirksames Streumedium bräuchte
  ein eigenes Volumenmaterial.
- Die **Geißel** zerfiel an den dünnsten Stellen zu einer Punktreihe, weil das Endstück mit 0,03 µm unter einem
  Bildpunkt lag. Jetzt läuft es auf 0,12 µm aus (anatomisch weiterhin korrekt) und bleibt eine durchgehende Linie.
- Das **Ooplasma** ist nur durch die Lücken im Zellkranz zu sehen; seine Körnung ist dort noch zu gleichmäßig.
- Die automatische **Lichtregelung** ist eingebaut, aber aus: Zellansicht und Eizelle sind bei derselben
  Lichtstärke richtig belichtet; mit Regelung säuft die Umgebung ab.

## GENESIS-025 – Richtung und Dichte des Schwarms

Gemeldet vom Game Director: *„Die Spermien schwimmen unkontrolliert und teilweise rückwärts, es sollten
auch viel mehr sein."* Beides war berechtigt, und beides hatte eine Ursache im Modell, nicht in der Anzeige.

**1. Kein Hinweis, wohin.** Die Strömung floss nur an der Wand; in der Mitte des Lumens stand sie still.
Rheotaxis wirkt aber nur dort, wo etwas fließt – also hatten die Zellen mitten im Kanal überhaupt keinen
Bezugspunkt und behielten ihre zufällige Startrichtung. Die Hälfte schwamm damit zur Gebärmutter statt
zum Eierstock. Jetzt fließt die Flüssigkeit im ganzen Lumen (45 % des Wandstroms in der Mitte),
und die Zellen richten sich überall gegen den Strom aus.

**2. Zu viel Taumeln.** Die Rotationsdiffusion progressiver Zellen lag bei 0,08 rad²/s – das sind über
20° Richtungsänderung je Sekunde, ein Schwarm ohne Kurs. Gemessen liegt sie bei wenigen Hundertsteln;
jetzt 0,035 rad²/s.

**3. Hyperaktivierte standen fast still.** Mit 8–20 µm/s Vortrieb und einer Rotationsdiffusion von
1,5 rad²/s zappelten sie auf der Stelle (LIN 0,04). Gemessen erreichen hyperaktivierte Zellen 20–35 µm/s
bei LIN um 0,1–0,3 – sie peitschen, aber sie kommen voran.

**4. Die Lockwirkung galt nur für Hyperaktivierte.** Jetzt reagieren auch progressive Zellen auf den
Progesteron-Gradienten, nur träger (45 %). Eindringen kann weiterhin nur eine kapazitierte Zelle.

**5. Zu wenige.** 300 Zellen sahen nach Einzelgängern aus, nicht nach Schwarm. Jetzt 6 000 –
gemessen 97 FPS (Frame 10,3 ms, Spiel-Thread 7,5 ms, GPU 9,8 ms); 4 500 Zellen liefen mit 138 FPS,
8 000 mit 72 FPS, dort ist der Spiel-Thread der Engpass.

**6. Alle auf einmal verteilt.** Die Zellen starteten gleichmäßig über den ganzen simulierten
Abschnitt von 3 mm – dadurch war überall wenig los. Sie kommen aber nicht verteilt an, sondern als
**Pulk von der Gebärmutter her**: Der Zug, der es bis in die Ampulle geschafft hat, zieht gemeinsam
flussaufwärts. Jetzt starten sie 700 µm unterhalb der Eizelle mit 450 µm Streuung und schwimmen
gemeinsam auf sie zu.

**Im Spiel gemessen** (4 500 Zellen, nach 68,8 s Simulationszeit):
Ø Ausrichtung gegen den Strom **+0,79**, **rückwärts 5 %**, an der Wand 14 %, Ø Vortrieb 31,0 µm/s;
2 116 progressiv, 2 355 hyperaktiviert, 29 träge; Eizelle befruchtet nach 21,3 s.

![Schwarm nach der Korrektur](Media/GENESIS-025_SwarmHUD.png)

![Der Pulk an der Eizelle](Media/GENESIS-025_Cohort.png)

Offen: Reduktionsstufen (LODs) für die Zelle – die Geißel ist am Ende 0,12 µm dünn, eine automatische
Reduktion würde genau diese Fläche zerlegen. Ohne sie ist bei etwa 8 000 Zellen Schluss.

## GENESIS-026 – Licht, das alle Abstände trägt

Zwei Beobachtungen aus dem Schwarm-Block: Eine weite Einstellung wurde schwarz, und die Corona
brannte in Nahaufnahmen weiß aus. Beides hatte dieselbe Ursache – **ein Licht für alle Fälle**.

Das Endoskoplicht sitzt an der Optik, seine Beleuchtungsstärke fällt mit dem Quadrat des Abstands.
Bei fester Stärke stimmt damit genau **ein** Arbeitsabstand. Bei 110 µm trifft die Zelle 15-mal so
viel Licht wie bei 430 µm; bei 1.100 µm bleibt ein Sechstel übrig.

Die Lösung besteht aus zwei Teilen:

**1. Geregeltes Licht, in zwei Bereichen.** Unterhalb des Bezugsabstands (430 µm) folgt die Regelung
dem Abstandsquadrat – dort füllt das Motiv den Ausschnitt, und die Physik stimmt genau. Oberhalb
wächst das Licht nur noch linear, denn dann sieht die Kamera vor allem die **nahen Falten** der
Schleimhaut; würde es weiter quadratisch steigen, überstrahlten sie das ganze Bild.

**2. Streulicht der Umgebung** (18 cd, 3.200 K, ohne Schatten). Im Gewebe und in der Flüssigkeit wird
Licht gestreut – neben dem Lichtkegel ist es deshalb nie völlig schwarz. Ohne diese Schicht war die
Regelung unbrauchbar: In Nahaufnahmen bekam die Umgebung gar kein Licht mehr ab (73 % der Fläche
nahezu schwarz), weshalb sie bis hierher ausgeschaltet blieb.

**Gemessen** (Luminanz des fertigen Bildes, 1137×600):

| Einstellung | Median | P99 | reinweiß | fast schwarz |
|---|---|---|---|---|
| Zelle, 110 µm | 0,411 | 0,79 | 0 % | 11,5 % |
| Eizelle, 430 µm | 0,302 | 0,74 | 0 % | 0,1 % |
| Weite Einstellung, 1.100 µm | 0,462 | 0,80 | 0 % | 1,1 % |
| Corona in Nahaufnahme, 260 µm | 0,356 | 0,72 | 0 % | 5,6 % |

Vorher war die weite Einstellung praktisch schwarz und die Corona reinweiß. Jetzt gibt es in keiner
Einstellung ausgebrannte Flächen, und in jeder ist Zeichnung vorhanden.

Kosten: 130 FPS statt 138 (Frame 7,66 ms, GPU 5,38 ms) – das Streulicht wirft keine Schatten.

![Corona ohne Überstrahlung](Media/GENESIS-026_Corona.png)

![Weite Einstellung im Eileiter](Media/GENESIS-026_Wide.png)

Offen: Die Coronazellen sehen aus der Nähe weiterhin wie weiche Klumpen aus – Form und Oberfläche der
Zellen sind ein eigener Block; die Belichtung macht das jetzt nur besser sichtbar.

## GENESIS-027 – Die Coronazellen: gepacktes Gewebe statt Popcorn

Mit der richtigen Belichtung wurde sichtbar, was vorher im Weiß unterging: Die Zellen der Corona sahen
aus wie lose Kugeln, die aneinandergelegt wurden. Lebendes Gewebe sieht anders aus – **Zellen drücken
sich gegenseitig platt.** Wo zwei aneinander liegen, entsteht eine ebene Berührungsfläche, wie in einem
Schaum. Erst das macht aus einem Haufen Kugeln ein Gewebe.

**Im Aufbau:** Jede Zelle wird an der Mittelebene zu jedem Nachbarn abgeschnitten, radiusgewichtet
(die größere Zelle drückt die kleinere stärker). Der Schnitt ist weich: Eine Zellmembran knickt nicht,
sie wölbt sich – der Übergang bekommt einen Radius von etwa einem Mikrometer. Ohne diesen Radius sehen
die Zellen aus wie geschliffene Steine.

Dazu 2 600 statt 2 100 Zellen, enger gepackt (Mindestabstand 5,4 statt 6,2 µm) und mit größerer
Streuung: 7,5–15,5 µm lange Achse, Querachsen 62–92 % davon. Im Gewebe ist keine Zelle wie die nächste.

**Gemessen im Aufbau:** Berührungstiefe P10 = 0,709, Median = 0,912; 46,1 % der Punkte sind deutlich
gedrückt (unter 0,90). 849 275 Flächen.

**Im Material** trägt die Vertexfarbe diese Berührungstiefe (Kanal B). Wo Zellen aneinander liegen,
kommt kein Licht hin: Grundfarbe auf ein Viertel, Durchleuchtung auf ein Achtel. Ohne diese dunklen
Fugen verschmelzen die Zellen im Endoskoplicht zu einer hellen Masse. Dazu schwankt die Rauheit mit dem
Zellkorn (0,34–0,56) – die Mikrovilli einer lebenden Zelle sind viel zu klein, um sichtbar zu sein,
aber sie streuen das Licht.

Der Tonumfang zwischen den Zellen wurde zurückgenommen (dunkelster Ton 0,018 → 0,055): Vorher wirkten
einzelne Zellen fast schwarz, was nach Schatten aussah statt nach Zelle.

![Coronazellen in der Nahaufnahme](Media/GENESIS-027_CoronaSharp.png)

![Der Komplex aus 260 µm](Media/GENESIS-027_Corona.png)

Kosten: unverändert (Frame 7,76 ms, GPU 4,78 ms) – die Corona ist ein Nanite-Mesh.

Offen: Die Oberfläche ist noch glatt; echte Mikrovilli-Zotteligkeit bräuchte eine Verdrängung im
Material. Und der Komplex hat keine sichtbare Matrix zwischen den Zellen – die Gallerte ist bisher nur
eine Hülle um das Ganze.

## GENESIS-028 – Die Matrix zwischen den Zellen

Beim Eisprung geben die Cumuluszellen Hyaluronsäure ab. Sie bindet Wasser, quillt auf und drückt die
Zellen auseinander – der Komplex **expandiert**. Was danach zwischen den Zellen steht, ist kein leerer
Raum, sondern ein zähes, fast klares Gel, das an beiden Zellen hängt und beim Auseinanderdriften Fäden
zieht. Genau diese Fäden lassen den Komplex im Mikroskop wie eine Wolke aussehen – und sie sind der
Grund, warum die Spermien kurz vor der Zona langsamer werden.

**Zwei Änderungen im Aufbau:**

1. **Innen dicht, außen locker.** Die Corona radiata liegt der Zona weiterhin dicht an; nach außen
   wächst der Mindestabstand der Zellen (5,4 µm innen bis 10,0 µm in der fünften Lage). Vorher war
   der Komplex überall gleich dicht gepackt – das ist der Zustand *vor* dem Eisprung, nicht danach.
2. **Fäden in den Lücken.** Zwischen Zellen, deren Oberflächen 0,5–7 µm auseinanderliegen, spannt sich
   ein Faden: an den Enden breiter (dort haftet das Gel an der Zelle), in der Mitte am dünnsten
   (0,09–0,22 µm), mit Durchhang quer zur Verbindung. Nach außen zieht die Gallerte mehr Fäden.

**Gemessen im Aufbau:** 2 687 Fäden, 2 299 von 2 600 Zellen hängen an mindestens einem; 67 175 Flächen
(die Corona selbst hat 852 825).

**Im Material:** fast klares Gel – Grundfarbe 0,52, Deckkraft 0,02 plus 0,16 über den Blickwinkel.
Ein Faden aus Wasser mit Zuckerketten hat einen Brechungsindex von etwa 1,34, kaum anders als die
Eileiterflüssigkeit: Er glänzt nicht, er wirft keinen Schatten, er schimmert nur.

Der erste Versuch war zu grob: Mit 0,22–1,43 µm dicken Fäden und einer Deckkraft von 0,42 sahen sie
aus wie weiße Papierschnipsel zwischen den Zellen. Erst dünn und fast durchsichtig sehen sie aus wie
Gel.

Kosten: Frame 7,34 ms, GPU 4,66 ms – unverändert.

![Fäden zwischen den Zellen](Media/GENESIS-028_Strands.png)

![Der expandierte Komplex](Media/GENESIS-028_Cumulus.png)

Offen: Die Fäden sind starr. Ein Spermium, das sich hindurchwindet, verformt sie nicht – dafür bräuchte
es eine Simulation der Gallerte. Und die Gallerte selbst (als Volumen) ist weiterhin nicht sichtbar;
sie wirkt nur über die Zellen, die Fäden und die Bremswirkung auf die Spermien.

## GENESIS-030 – Die Spermien springen nicht mehr: Bewegung gegen die Messliteratur geprüft

Der Game Director hat gemeldet, die Spermien „springen unrealistisch rum und haben keine natürliche
Bewegung". Das war kein Geschmacksurteil, sondern ein Befund – und die Ursache war ein Fehler, den ich
selbst eingebaut hatte.

### Ursache 1: Zeitliche Unterabtastung (mein Fehler)

Für die spielbare Fassung hatte ich `ConceptionTimeScale` von 0,3 auf **1,0** gesetzt, damit der
Spieler nicht so lange wartet. Eine Geißel schlägt aber mit 16–24 Hz. Bei 60 Bildern je Sekunde
bleiben davon **knapp drei Bilder je Schlagzyklus**. Was dann auf dem Bildschirm ankommt, ist kein
Schwimmen mehr, sondern ein Aliasing-Artefakt: Die Zelle zuckt von Kopfposition zu Kopfposition.

Das ist derselbe Effekt, der Wagenräder im Film rückwärts laufen lässt, und er ist nicht durch
Glättung zu heilen. Die Mikrowelt läuft deshalb wieder in **Zeitlupe (`ConceptionTimeScale = 0,3`)**.
Das ist hier keine Stilentscheidung, sondern Physik: Auch echte Aufnahmen von Spermien sind
Hochgeschwindigkeitsaufnahmen, die verlangsamt abgespielt werden – anders kann ein menschliches Auge
einen Geißelschlag gar nicht sehen. Damit die Befruchtung trotzdem zügig kommt, startet die Kohorte
jetzt bei **320 µm** statt 700 µm vor der Eizelle (Streuung 220 µm).

### Ursache 2: Zwei Zahlen im Modell waren falsch herum gedacht

Beim Abgleich mit der Literatur fielen zwei echte Modellfehler auf:

| Größe | vorher | jetzt | Grund |
|---|---|---|---|
| Wellenlänge hyperaktiviert | 45 µm | **17 µm** | Hyperaktivierung heißt *größere Amplitude bei kürzerer Welle*, nicht eine lange, flache Welle. Der alte Wert war die Bewegung genau verkehrt herum. |
| Schlagfrequenz progressiv | 12–18 Hz | **16–24 Hz** | Gemessener Median aktivierter Zellen: 19 Hz; CASA-Referenz BCF 23,6 ± 5,0 Hz. |
| Schlagfrequenz hyperaktiviert | 7–10 Hz | **9–15 Hz** | Median 10 Hz, frei schwimmend im Mittel 14,6 Hz, nach Reizung Median 11,7 Hz. |
| Kopfauslenkung hyperaktiviert | 9–14 µm | **7,5–11,5 µm** | Kriterium ALH ≥ 7 µm; gemessen 5,7–11,4 µm. |
| Rollen um die Längsachse | 0,02/Schlag (≈0,3 Hz) | **0,3/Schlag (≈6 Hz)** | Menschliche Spermien rollen mit 4–8 Hz (Mittel 6,0 ± 2,1). Ohne dieses Rollen gibt es keine Rheotaxis – die Zelle könnte die Strömung gar nicht „schmecken". |

### Ursache 3: Wir haben etwas anderes gemessen als die Literatur

Unsere Tests maßen die Kopfbahn mit 240 Hz **im Raum**. Ein CASA-Gerät misst mit 60 Hz **in der
Ebene**, weil es durch ein Mikroskop auf eine flache Kammer schaut. Beides macht einen großen
Unterschied:

- **Abtastrate:** Die Bahngeschwindigkeit VCL ist die Summe der Abstände zwischen Abtastpunkten. Sie
  wächst mit der Abtastrate – mit 240 Hz kam bei derselben Zelle eine um die Hälfte höhere VCL heraus.
- **Projektion:** Die Zelle rollt, ihr Kopf beschreibt eine Schraube. Deren Länge im Raum ist größer
  als der Schatten, den das Mikroskop sieht.

Der Test bildet jetzt beides nach: 60 Hz, und die Bahn wird in die Ebene projiziert, die das Mikroskop
sieht (Schwimmrichtung + Hauptachse der seitlichen Auslenkung, per Potenzmethode bestimmt).

### Gemessen gegen die Referenz

| Kenngröße | GENESIS | Referenz (CASA, 66 Spender) | |
|---|---|---|---|
| VSL progressiv | 43,2 µm/s | 46,1 ± 9,7 µm/s | innerhalb |
| VCL progressiv | 92,4 µm/s | 82,5 ± 15,7 µm/s | innerhalb |
| LIN progressiv | 0,47 | 0,56 | plausibel |
| Schlagfrequenz progressiv | 20,4 Hz | 23,6 ± 5,0 Hz (Median aktiviert 19 Hz) | innerhalb |
| VCL hyperaktiviert | 156,6 µm/s | Kriterium ≥ 150 µm/s | erfüllt |
| LIN hyperaktiviert | 0,15 | Kriterium < 0,5 | erfüllt |
| ALH hyperaktiviert | 7,5–11,5 µm | Kriterium ≥ 7 µm | erfüllt |
| Schlagfrequenz hyperaktiviert | 11,7 Hz | Median 10–11,7 Hz | innerhalb |

Alle drei Mortimer-Kriterien der Hyperaktivierung (VCL ≥ 150, LIN < 0,5, ALH ≥ 7) sind damit zugleich
erfüllt – vorher war es keines vollständig. Die Testschranken sind jetzt die Streubreite der
Veröffentlichung (66,8–98,2 µm/s) statt eines bequemen weiten Bereichs: Fällt eine Änderung wieder
heraus, schlägt der Test an.

### Quellen

- [Rapid sperm capture: high-throughput flagellar waveform analysis](https://academic.oup.com/humrep/article/34/7/1173/5510488)
- [Accuracy of sperm velocity assessment using the Sperm Quality Analyzer V](https://pmc.ncbi.nlm.nih.gov/articles/PMC5907002/)
- [Influence of image sampling frequency on perceived movement characteristics](https://onlinelibrary.wiley.com/doi/10.1002/mrd.1120200307)
- [Hyperactivation of sperm – HT CASA (Mortimer-Kriterien)](https://www.micropticsl.com/hyperactivation-of-sperm/)
- [The mechanics of hyperactivation in adhered human sperm](https://royalsocietypublishing.org/doi/10.1098/rsos.140230)
- [Quantitative observations of flagellar motility of capacitating human spermatozoa](https://pubmed.ncbi.nlm.nih.gov/9194655/)
- [Rheotaxis guides mammalian sperm](https://www.cell.com/current-biology/fulltext/S0960-9822(13)00148-6)
- [Chirality and frequency measurement of longitudinal rolling of human sperm](https://pmc.ncbi.nlm.nih.gov/articles/PMC9790903/)
- [Human sperm rotate with a conserved direction during free swimming in four dimensions](https://pmc.ncbi.nlm.nih.gov/articles/PMC10729817/)

### Nebenbefund: die rote Zeile im Bild

Bei der Sichtprüfung stand eine rote Warnung im Bild: „RAY TRACING GEOMETRY – ALWAYS RESIDENT MEMORY
EXCEEDS 20% OF THE BUDGET (87 MiB / 400 MiB)". Die Mikrowelt ist ein dichter Raum – 6 000 Zellen,
2 600 Coronazellen, 2 687 Matrixfäden –, und deren Strahlengeometrie liegt dauerhaft im Speicher. Der
Vorrat steht jetzt auf 768 MiB (`r.RayTracing.ResidentGeometryMemoryPoolSizeInMB`), ein für
Desktop-Grafikkarten üblicher Wert. Abgeschaltet wurde die Warnung **nicht**: Sie soll weiter
anschlagen, wenn eine Szene wirklich zu schwer wird.

![Der Schwarm vor dem Cumulus](Media/GENESIS-030_Schwarm.png)

### Offen

- **Sehr nahe Zellen brennen aus.** Auf dem Bild sind einzelne Spermien direkt vor der Linse reine
  weiße Streifen. Das Licht aus GENESIS-026 trägt über alle Arbeitsabstände, aber die
  Belichtungsautomatik kommt mit dem Abstand < 60 µm nicht mit. Gehört in den Fotorealismus-Block.
- **Ein Standbild kann Bewegung nicht belegen.** Dass es nicht mehr springt, steht in den Messwerten;
  dass es *aussieht* wie Schwimmen, muss ein Mensch einmal bestätigen.

## GENESIS-032 – Durchscheinendes gehört in die Schärfentiefe

Bei der Sichtprüfung der spielbaren Fassung stand eine Zelle dicht vor der Linse als **gestochen
scharfer weißer Fleck** im Bild, während alles dahinter weich war. Das war der Rest von „sehen
teilweise nicht realistisch aus" – und diesmal lag es nicht an der Bewegung, sondern an einem
Schalter im Material.

### Die Ursache

Unreal zeichnet durchscheinende Flächen in einem eigenen Durchgang. Der steht in der Voreinstellung
**nach** der Schärfentiefe (`Render After DOF`). Das ist für ein Interface sinnvoll und für alles
andere falsch: Eine durchscheinende Fläche bekommt dann *nie* Unschärfe, egal wo sie steht. Unsere
Spermien, die Zona, die Matrix und die Fäden sind alle durchscheinend – sie blieben also scharf,
während die Umgebung weich wurde. Genau daran erkennt ein Auge ein Bild als gerechnet, auch ohne
sagen zu können, warum.

Alle vier Materialien stehen jetzt auf `Before DOF`. Eine Zelle direkt vor der Linse ist damit das,
was sie in einer echten Makroaufnahme wäre: ein weicher heller Schemen.

![Nahe Zellen sind jetzt weiche Schemen statt scharfer Flecken](Media/GENESIS-032_Schaerfentiefe.png)

### Was dabei auffiel und ehrlich bleiben muss

- **Frühere Bilder in dieser Dokumentation waren zu scharf.** Jedes Bild mit gut erkennbaren
  Spermien im Vorder- oder Mittelgrund zeigte einen Zustand, den es physikalisch nicht gibt.
- **Die Unschärfe hat ein Geometrieproblem verdeckt.** Mit einem kleineren Makro-Faktor (7 statt 18,
  also mehr Tiefenschärfe) sieht man, dass die Coronazellen an den Rändern facettiert wirken – wie
  geschnittener Stein, nicht wie Gewebe. Der weiche Schnitt aus GENESIS-027 (0,9 µm) ist zu klein,
  um scharf gezeichnet als Zellrand durchzugehen. Das gehört in den Fotorealismus-Block und steht
  unten unter „Offen".

### Nebenbefund: mein eigener Fehler von GENESIS-030

Der größere Vorrat für Strahlengeometrie (768 statt 400 MiB) hat die rote Warnung beseitigt – und
auf der Testkarte (RTX 5070, 12 GB) dafür gesorgt, dass der Grafikspeicher um **329 MB überläuft**:
fast genau der Betrag, den der größere Vorrat belegt. Im Bild stand dann „Video memory has been
exhausted. Expect extremely poor performance." Eine Warnung verschwinden zu lassen, indem man ihr
mehr Speicher gibt, verschiebt das Problem nur.

Der Vorrat steht wieder auf dem Vorgabewert. Stattdessen ist der Schwarm aus der Strahlenszene
heraus (`SetVisibleInRayTracing(false)`): Sechstausend Zellen, deren Geißel im Shader schlägt,
müssten jedes Bild neu in die Strahlenszene gebaut werden – und eine 5 µm große, fast durchsichtige
Zelle spiegelt sich in nichts und erhellt nichts. Gesehen, beleuchtet und beschattet wird sie
weiterhin. Der Grafikspeicher läuft damit nicht mehr über.

### Offen

- **Die Warnung zur Strahlengeometrie steht wieder im Bild** (83–87 MiB von 400). Der Schwarm war
  nur 3,5 MiB davon; der Rest ist der Eizellkomplex (852 825 Flächen Corona, 2 687 Fäden). Das ist
  ein Fall für den Fotorealismus-Block, nicht für einen größeren Vorrat. In einer
  Veröffentlichungsfassung erscheint die Warnung ohnehin nicht – sie ist eine Entwicklerhilfe.
- **Die Coronazellen wirken scharf gezeichnet facettiert.** Sichtbar, sobald die Tiefenschärfe
  größer wird.
