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
- **Rheotaxis:** Ausrichtung gegen den Zilienstrom (der Richtung Gebärmutter fließt) – der Strom weist den Weg zum Eierstock. Wirkt dort am stärksten, wo die Strömung am stärksten ist, also an der Wand.
- **Wandbindung:** Zellen richten sich an Oberflächen parallel aus und schwimmen leicht zur Wand geneigt (3,5°); dadurch sammeln sie sich dort. Hyperaktivierte lösen sich leichter (Faktor 0,35) – so kommen sie von der Schleimhaut wieder frei.
- **Hyperaktivierung** wechselt zufällig (Raten je Sekunde, Standard 0,02 hin / 0,05 zurück).

**Gemessen (Automationstests):**

| Messwert | Ergebnis | Referenz |
|---|---|---|
| Progressiv VSL | 42,5 µm/s | WHO: schnell progressiv ≥ 25 µm/s |
| Progressiv VCL | 129 µm/s | CASA typisch 60–150 µm/s |
| Hyperaktiviert VCL | 255 µm/s | CASA-Kriterium ≥ 150 µm/s |
| Hyperaktiviert LIN | 0,04 | CASA-Kriterium < 0,5 |
| Wandnähe nach 40 s | 98 % progressiv, 26 % hyperaktiviert | Gleichverteilung wäre 23 % |
| Rheotaxis | Ausrichtung 0,99 stromaufwärts, +471 µm in 30 s | ohne Rheotaxis −750 µm (Abtrift) |
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

## Offen (bewusst, nicht versteckt)

- **Flimmerhärchen** (Kinozilien, ~10 µm, schlagend) fehlen noch; sie sind für Nahaufnahmen der Schleimhaut nötig.
- **Schwebeteilchen ruhen** noch, statt mit der Strömung zu driften.
- Das Kapillarnetz im Blender-Referenzmaterial ist noch zu regelmäßig (wabenartig).
- Eizelle und Corona radiata stammen aus der Trailer-Session und sind für Nahaufnahmen noch nicht realistisch genug (glatte, gleichförmige Zellen ohne Cumulus-Matrix).
