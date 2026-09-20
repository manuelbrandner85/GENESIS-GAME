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

**5. Zu wenige.** 300 Zellen sahen nach Einzelgängern aus, nicht nach Schwarm. Jetzt 4 500 –
gemessen 138 FPS (Frame 7,2 ms, Spiel-Thread 5,6 ms, GPU 5,4 ms). Bei 8 000 Zellen wird der
Spiel-Thread mit 10,4 ms zum Engpass; dort liegt die Grenze ohne weitere Arbeit.

**Im Spiel gemessen** (4 500 Zellen, nach 68,8 s Simulationszeit):
Ø Ausrichtung gegen den Strom **+0,79**, **rückwärts 5 %**, an der Wand 14 %, Ø Vortrieb 31,0 µm/s;
2 116 progressiv, 2 355 hyperaktiviert, 29 träge; Eizelle befruchtet nach 21,3 s.

![Schwarm nach der Korrektur](Media/GENESIS-025_SwarmHUD.png)

Offen: Reduktionsstufen (LODs) für die Zelle – die Geißel ist am Ende 0,12 µm dünn, eine automatische
Reduktion würde genau diese Fläche zerlegen. Ohne sie ist bei etwa 8 000 Zellen Schluss.
