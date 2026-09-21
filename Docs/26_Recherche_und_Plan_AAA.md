# 26 – Recherche und Plan zur AAA-Qualität

Auftrag des Game Directors (2026-09-21): „Die Spermien wackeln komisch hin und her. Die eigene Zelle
halb aus Ego-Perspektive. Schau im ganzen Spiel, wo kie.ai Grafik, Bilder oder Videos liefern kann, die
die Engine hyperrealistisch weiterverarbeitet. Recherchiere vorher Anatomie, Bewegung, Technik – es soll
ein AAA+-Spiel werden."

Drei Recherchen, jede mit Quellen. Diese Seite ist die Arbeitsgrundlage für die nächsten Blöcke.

## 1. Wie Spermien sich wirklich bewegen – und warum unsere wackeln

**Befund:** Echte Spermien wackeln nicht als starrer Körper. Eine Biegewelle läuft vom Hals zur Spitze,
ihre Auslenkung **wächst zur Spitze hin**. Der Kopf wird von der Geißel nur leicht zur Seite gestoßen,
als Gegenbewegung, nicht als eigene Pendelbewegung. Unser Modell dreht die ganze Zelle im Takt
(`ComputeVisualTransform`: Gieren um ±Amplitude mit dem Schlag) – genau das sieht falsch aus.

| Größe | progressiv, zähe Eileiterflüssigkeit | hyperaktiviert |
|---|---|---|
| Geißellänge, Mittelstück | 50 µm, 5 µm | 50 µm, 5 µm |
| Schlagfrequenz | 10 Hz (8–12) | 11 Hz, unregelmäßig |
| Wellenlänge (Bogen) | 17 µm (≈ 3 Wellen) | 35–45 µm (1–1,5 Wellen) |
| Auslenkung Mittelstück → Spitze | 0,1 → 0,8 rad | 0,5 → 1,3 rad |
| einseitige Grundkrümmung | 0 | 0,3–0,5 rad |
| Oberwelle (2. Harmonische) | 0,05 | 0,2–0,3 |
| Kopfgieren | ±3–6° | ±25–40° |
| seitliche Kopfauslenkung (ALH, Spitze–Spitze) | 1–2 µm | 7–12 µm |
| Rollen um die Längsachse | 0–1,5 Hz (an Wänden fast flach) | 0–4 Hz, unregelmäßig |
| Bahngeschwindigkeit | 25–40 µm/s, fast gerade | gering; Kreise, Achten, „Star-Spin" |

Modell für den Shader: Tangentenwinkel entlang der Bogenlänge s,
ψ(s,t) = ψ₀(s) + A(s)·[sin(ks − ωt + φ) + ε·sin(2(ks − ωt) + φ₂)], A(s) wächst vom Mittelstück zur
Spitze; die Mittellinie ist das Integral davon (Länge bleibt erhalten). Der Kopf gleicht das Drehmoment
der Geißel aus: Drehung um −β·ψ̄ (β ≈ 0,3–0,5), Verschiebung um −γ·ȳ (γ ≈ 0,5–0,7).

**Fünf Merkmale, die echt wirken:** (1) Welle läuft sichtbar nach hinten und wird zur Spitze größer;
(2) der Kopf bewegt sich kaum, etwa zehnmal weniger als die Spitze; (3) die mittlere Bahn ist glatt;
(4) der flache Kopf rollt und blitzt dabei hell–dunkel; (5) in zäher Flüssigkeit kurze Wellen, scharfe
Biegungen, ruhiges Mittelstück, langsamer Schlag.

**Achtung:** Unser Modell nutzt 16–24 Hz und 31 µm Wellenlänge – das sind Werte in **dünner**
Flüssigkeit. Im Eileiter (zäher) sind 10 Hz und 17 µm realistisch. Das ändert auch die Zeitlupe
(GENESIS-030/037): Bei 10 Hz genügt eine schwächere Zeitlupe.

Quellen: Smith et al. 2009 (Cell Motil Cytoskeleton 66:220), Gallagher et al. 2019 (Hum Reprod 34:1173,
PMC6613345), Saggiorato et al. 2017 (Nat Commun 8:1415), Ishimoto et al. 2018 (J Theor Biol 446:1),
Bukatin et al. 2015 (PNAS 112:15904), Woolley 2003 (Reproduction 126:259), Ooi et al. 2014
(R Soc Open Sci 1:140230), Zhong et al. 2022 (PMC9790903), Corkidi et al. 2023 (PMC10729817),
Kantsler et al. 2014 (eLife 3:e02403). Hinweis: Gadêlha et al. 2020 (Sci Adv) wurde 2021 zurückgezogen –
nur qualitativ verwendbar.

## 2. Die Entwicklung – Zahlen und Aussehen je Kapitel

**Zwei Zeitrechnungen trennen:** Embryologie zählt ab Befruchtung, Frauenärzte und Ultraschall ab der
letzten Periode (plus zwei Wochen). IVF-Zeitraffer zählt in Stunden nach der Befruchtung (hpi).

### Erste Woche (Zeitraffer im Brutschrank, Median aus 340 Embryonen, HROpen 2024)

| Ereignis | hpi (Median, Bereich) | Aussehen im Mikroskop |
|---|---|---|
| 2. Polkörper | 3,5 (2,1–4,9) | Kügelchen 10–15 µm unter der Zona |
| Vorkerne erscheinen | 8,3 (6,4–10,2) | zwei klare Blasen, wandern zur Mitte |
| Vorkerne lösen sich auf | 23,3 (21,0–25,7) | Zytoplasma kurz gleichmäßig |
| 2 Zellen | 25,8 | erste Furche |
| 3 / 4 Zellen | 36,9 / 38,3 | oft tetraedrisch; 3. und 4. fast gleichzeitig |
| 8 Zellen | 58,7 | Tag 3 |
| Kompaktierung / Morula | 80,2 / 88,9 | Zellgrenzen verschwinden, glatte Kugel |
| Blastulation beginnt / volle Blastozyste | 99,0 / 109,9 | Hohlraum; Embryoblast und Trophoblast |
| ausgedehnt / schlüpfend | 111–121 / ab 116 | Zona dünn (16 → 7–8 µm), Keim wölbt sich durch einen Riss |

Tag 5: 58 ± 8 Zellen (38 Trophoblast, 20 Embryoblast); Tag 6: 84 ± 6 (Hardy 1989). Durchmesser der
Blastozyste wächst von ~130 auf ~175 µm. Das Aussehen: **Hoffman-Modulationskontrast, grau-beige,
reliefartig von einer Seite beleuchtet, heller Saum an Kanten** – nicht rosa, nicht glatt.

### Woche 2–8 (Carnegie-Stadien) und Fetalzeit

Stadientabelle (Tag, Scheitel-Steiß-Länge, Merkmale) in der Recherche; Kernpunkte: Herzschlauch schlägt
ab Tag 21–22 (2 mm), Neuralrohr schließt Tag 25–28, Arm-/Beinknospen Stadium 12–13, Finger frei Stadium 21,
Ende Woche 8: 27–31 mm, alle Organe angelegt. Herzfrequenz: 90–110 in Woche 5–6, Spitze ~170 in Woche
9–10, am Termin 110–160. Erste Bewegungen 7,5 Wochen (Gestationsalter), Schluckauf 9, Gähnen/Saugen
11–12, Mutter spürt Bewegungen 18–20, Reaktion auf Töne ab 19, Augen öffnen 26–28, Schädellage meist
32–36 Wochen. Gewicht (50. Perzentile): 146 g in Woche 16, 670 g in 24, 1953 g in 32, 3619 g in 40.

### Die Verbindungen (Nervensystem)

Erste Synapsen im Körper: Rückenmark, ~5,5 Wochen nach Befruchtung (Okado). Erste in der Hirnrinde
~Woche 8 (Kostović). Die Fasern vom Thalamus warten ab Woche 20–24 unter der Rinde und wachsen um
Woche 24–26 hinein – vermutlich die Grundlage der ersten bewussten Reize (Schmerz, Klang).
„250 000 Neurone pro Minute" ist ein **Durchschnitt**, keine Spitze. Myelin: Rückenmark ab Woche 20,
Großhirn überwiegend erst nach der Geburt.

### Bildsprache je Kapitel

| Kapitel | echtes Vorbild |
|---|---|
| Tag 0–6 | Hoffman-Kontrast, 200–400×, Zeitraffer mit hpi-Stempel (EmbryoScope) |
| Einnistung (Stadium 5–9) | keine Aufnahmen am Lebenden – nur Gewebeschnitte und Elektronenmikroskop |
| Stadium 10–23 | Stereomikroskop-Fotos (Carnegie, Kyoto, EHD); Nilsson-Bilder: warmes, durchscheinendes Gewebe, Gefäße, Fruchtblase |
| Fetalzeit | Ultraschall 2D (Körnung), 3D/4D „HDlive" (bernsteinfarbene Haut), MRT (Hirnfurchen) |

Quellen: Ciray 2014 (Hum Reprod 29:2650), HROpen 2024 (PMC11540439), Meseguer 2011, Hardy 1989,
Endowment for Human Development, UNSW Carnegie-Tabelle, de Vries 1982, Hepper & Shahidullah 1994,
WHO-Wachstumskurven (Kiserud 2017), Okado 1979–81, Kostović 2010/2021.

## 3. kie.ai – was es kann und wofür wir es nutzen

**Kann:** Bilder bis 4K (Nano Banana Pro, GPT Image 2), Hochrechnen bis ~16K (Topaz), Videos bis 15 s
und 4K (Kling 3.0, Seedance 2.0, Veo 3.1), Bewegungsübertragung (Kling Motion Control), Musik/Klänge
(Suno), Sprache (ElevenLabs Turbo 2.5 mit `language_code: "de"`, Gemini TTS).
**Kann nicht:** 3D-Modelle, echte HDR-Himmel, fertige PBR-Materialien. Guthaben per
`GET /api/v1/chat/credit`; 1 Credit = 0,005 $; Dateien nach 14 Tagen weg.
**Grenzen:** Nacktheit und Geburt in Nahaufnahme werden von den Anbietern abgelehnt; klinische
Mikroskop-Motive meist erlaubt. Kommerzielle Nutzungsrechte vor Veröffentlichung schriftlich klären.

**Wofür, in dieser Reihenfolge:**
1. **Look-Dev-Referenztafeln** (Nano Banana Pro 4K): Hoffman-Kontrast-Keim, Zona, Eileiterschleimhaut,
   Fruchtblase, Plazenta – als Zielbild für Blender/Unreal, nie direkt ins Spiel.
2. **Kachelbare Albedo-Texturen** (GPT Image 2 / Nano Banana Pro 4K) → Topaz ×4 → daraus Normal-,
   Rauheits-, AO-Karten: Schleimhaut, Gebärmutterwand, Eihäute, Kreißsaal-Boden und -Wände.
3. **Bewegungsreferenz** (Seedance 2.0 mit echten Mikroskopie-Clips als Vorlage): Geißelschlag,
   Zilienschlag, Zellteilung – für Timing und Kurven, nicht als Bild.
4. **Mimik der Mutter** (Kling 3.0, Bild → Video): Schmerz, Erleichterung, erster Blick – Vorlage für die
   MetaHuman-Animation.
5. **Trailer-Übergänge** (Veo 3.1, Start- und Endbild aus Unreal).
6. **Fensterblick/Himmel-Hintergrund** des Kreißsaals (GPT Image 2 im Format 2:1, nur als Kulisse).
7. **Stimmen:** ElevenLabs Turbo 2.5 auf Deutsch (der bisherige Fehler betraf ein anderes Modell).
8. **Klangbetten:** Suno „sounds" für Herzschlag und Mutterleib als Schleifen.

Jeder bezahlte Aufruf wird vorher angekündigt, Guthaben geprüft, Kosten genannt.

Quellen: docs.kie.ai (llms.txt, market/quickstart, common-api/get-account-credits, Modellseiten).

## 4. Erste Zielbilder (kie.ai, Nano Banana Pro 4K) – erzeugt am 2026-09-21

Sechs Bilder, 5504 × 3072, 144 Credits (≈ 0,72 $). Erzeugt mit `Tools/Reference/kie_reference_images.py`
(Schlüssel nur aus der Umgebungsvariable `KIE_API_KEY`). Sie liegen **nur lokal** in
`ArtSource/Reference/kie/`: Die Nutzungsrechte sind nicht geklärt, das Repository ist öffentlich, und die
Bilder sind Zielbilder für Blender/Unreal, keine Spielgrafik.

| Bild | Brauchbar für | Fachliche Prüfung |
|---|---|---|
| Spermien am Eileiterepithel (DIC) | Licht, Randsaum, Tiefenschärfe | Köpfe und Mittelstück stimmig; Schleimhaut wirkt gerippt statt gefaltet, keine erkennbare Geißelwelle – nur für Licht und Look |
| Keim mit 4 Zellen (Hoffman) | **Hauptvorlage für 038 Teil 3** | Sehr nah am echten Bild: grau-beige, körnig, Relieflicht, Kerne angedeutet, Polkörper, Fragmente. Zona etwas zu breit und zu hell gesäumt |
| Schlüpfende Blastozyste | Vorlage Tag 5–6 | Trophektoderm, Embryoblast, Blastocoel, Durchbruch durch die Zona – stimmig |
| Gebärmutterschleimhaut (Hysteroskopie) | Vorlage GENESIS-039 | Drüsenöffnungen, Gefäßnetz, feuchter Glanz – stimmig |
| Embryo in der Fruchtblase | Vorlage GENESIS-041 | Eher 8–9 Wochen als die verlangten 7 (Finger schon frei); Look (warm, durchscheinend, Gefäße) sehr gut |
| Kreißsaal in der Dämmerung | Abgleich mit GENESIS-035 | Bett, CTG, Wärmebett, Tuch zum Hochziehen, Gymnastikball, Jalousie mit Abendlicht – sehr brauchbar |

Nächste Schritte damit: das Material der Furchungszellen auf das Hoffman-Bild ziehen (Teil 3), die
Coronazellen ebenso (Kerne, Zellgrenzen, Randsaum statt glatter Eier).