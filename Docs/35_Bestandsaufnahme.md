# 35 – Bestandsaufnahme: visuell und technisch (Vertical Slice)

*2026-09-23, nach dem Visual Reference, Quality & Cleanup Protocol (Doc 00b, Abschnitt 24). Erst prüfen, dann
überarbeiten. Die Referenzen und ihre Analyse stehen in Doc 36.*

Werkzeuge (reproduzierbar, nur lesend):
- `Tools/Audit/audit_content.py`: alle Unreal-Pakete mit Klasse, Größe, Referenzen und Erreichbarkeit von den
  Spielkarten, immer gekochten Ordnern und Code-Pfaden aus (Unreal-Asset-Registry, nicht nach Gefühl).
- `Tools/Audit/Measure-Mood.ps1`: Farbstimmung eines Bildes (Leuchtdichte, warm, kalt, Tiefen, Spitzlichter,
  Sättigung). Dieselben Grenzen für Cover und Szenen.
- Szenenbilder aus der Engine: `Docs/Media/Audit/` (Stand vor der Überarbeitung, für Vorher/Nachher).

## 1. Bestand

| Bereich | Umfang | Anmerkung |
|---|---|---|
| Unreal-Inhalte `/Game/Genesis` | 642 Pakete, davon 600 von den Spielkarten aus erreichbar, 0 Redirectors | Characters 404 Pakete / 2,0 GB (MetaHuman, nicht im Repository) |
| Karten | Eileiter, Gebärmutterhöhle, Fruchthöhle, Kreißsaal, `L_DevSandbox`, `L_GEN_EmbryoLookdev`, Trailer-Bühne | Spielstart: Eileiter (`GameDefaultMap`) |
| C++ | 20 Plugins, ein System je Plugin | keine Platzhalter, TODOs oder Prototyp-Reste im Code gefunden |
| Tests | 153 Automation-Tests | alle grün |
| Blender-Quellen | `ArtSource/Generated` 1,9 GB (lokal, nicht im Repository) | Blender-Skripte in `Tools/Blender` erzeugen alles neu |
| Referenzen | `ArtSource/Reference`: 29 echte Embryo-Referenzen mit Lizenzliste, 6 KI-Zielbilder (kie.ai) | KI-Bilder sind Konzept, keine Realitätsreferenz |
| Repository | 980 MB `Docs/Media` (davon 867 MB drei Trailer-Videos), 508 MB `Genesis/Content` | Git LFS |

## 2. Szenen gegen das Cover – gemessen

![Cover](Media/Cover/GENESIS_Cover.png)

| Szene | Leuchtdichte | warm | kalt | Tiefen | Spitzlichter | Sättigung |
|---|---|---|---|---|---|---|
| **Cover** | 0,28 | 26 % | 33 % | 5,7 % | 3,2 % | 0,25 |
| 1 Rennen im Eileiter | 0,29 | 70 % | 0 % | 14,1 % | 0,1 % | 0,20 |
| 2 An der Eizelle | 0,41 | 72 % | 0 % | 0 % | 0,3 % | 0,16 |
| 3 Furchung | 0,32 | 6 % | 0 % | 20,0 % | 0 % | 0,18 |
| 4 Einnistung | 0,40 | 100 % | 0 % | 0 % | 0 % | **0,46** |
| 5 Fruchthöhle | 0,23 | 89 % | 0 % | 11,6 % | 0 % | **0,63** |
| 6 Geburt (Wehen) | 0,25 | 13 % | 0 % | 4,9 % | 11,3 % | **0,07** |
| 7 Haut an Haut | 0,17 | 28 % | 12 % | 22,2 % | 0,1 % | 0,36 |

**Befund:** Das Cover lebt vom Gegensatz: warmes, von innen leuchtendes Leben vor kühlem, tiefem Dunkel (26 % warm,
33 % kalt), gedämpft gesättigt (0,25). Die Szenen vor der Geburt sind durchweg warm ohne kühlen Gegenpol, zwei davon
deutlich zu bunt (Einnistung 0,46, Fruchthöhle 0,63). Der Kreißsaal ist grau und klinisch (0,07) – die Visual DNA
ordnet die Geburt dem Warmen zu; die echten Geburtsfotos (Doc 36) zeigen genau diesen Warm-kalt-Kontrast: warme Haut
unter hartem Licht vor blaugrünen Tüchern.

| 1 Rennen | 2 Eizelle | 3 Furchung | 4 Einnistung |
|---|---|---|---|
| ![](Media/Audit/01_Rennen.png) | ![](Media/Audit/02_Eizelle.png) | ![](Media/Audit/03_Furchung.png) | ![](Media/Audit/04_Einnistung.png) |

| 5 Fruchthöhle | 6 Geburt | 7 Haut an Haut |
|---|---|---|
| ![](Media/Audit/05_Fruchthoehle.png) | ![](Media/Audit/06_Geburt_Wehen.png) | ![](Media/Audit/07_Haut_an_Haut.png) |

## 3. Klassifizierung

**A KEEP · B IMPROVE · C REBUILD · D DELETE.** Grundlage: Szenenbild, Referenzen (Doc 36), Registry.

| Element | Bewertung | Begründung (gegen Referenz und Cover) |
|---|---|---|
| Startbildschirm, Titelkarte, Vorfilm | **A KEEP** | am Cover gemessen und abgestimmt (Doc 31, 33) |
| Klang, Stimmen, Musik, Körperklang | **A KEEP** | nicht Teil dieser visuellen Prüfung; Hören des Ungeborenen gerade nach Recherche erneuert (Doc 34) |
| Spermien, Schwarm, Steuerung, Ego-Ansicht | **B IMPROVE** | Bewegung gut; Aussehen im Nahbereich prüfen |
| Kumuluszellen um die Eizelle (Szene 1) | **C REBUILD** | glatte braune Ellipsoide wie Bohnen, gleich groß, undurchsichtig. Echt: unregelmäßige, durchscheinende Zellen, locker in gallertiger Matrix (Hyaluronsäure), dazwischen Fasern |
| Eizelle, Zona, Befruchtung (Szene 2) | **B IMPROVE** | Mikroskop-Unschärfe richtig; Zona als leuchtender Ring (Doppelbrechung), Polkörperchen fehlen |
| Furchungskeim (Szene 3) | **C REBUILD** | undurchsichtiger grauer „Ton“, Kerne als eingedrückte Knöpfe außen. Echt (IVF, Hoffman-Kontrast): durchscheinend, fein gekörnt, weiche Kanten, Kerne innen kaum sichtbar, milchiger Zona-Hof |
| Gebärmutterschleimhaut, Einnistungsstelle (Szene 4) | **C REBUILD** | lachsfarbener „Schwamm“ mit gestanzten Löchern, trocken, flach, zu bunt. Echt: nass glänzend, tief rot-orange; aus der Nähe Kopfsteinpflaster gewölbter Zellen (Pinopoden) mit Flaum |
| Fruchthöhle mit Embryo Tag 28 (Szene 5) | **B IMPROVE** | Anatomie jetzt richtig (Doc 32); zu gesättigt (0,63), kein kühler Gegenpol; Amnion ohne glänzende Falten; Gefäßbaum zu regelmäßig; lebendes Gewebe rosiger und durchscheinender (Fetus Woche 10 in Doc 36) |
| Kreißsaal, Licht (Szene 6) | **B IMPROVE** | grau und kalt; echte Geburt: warme Haut, hartes Arbeitslicht, kühle Tücher |
| Hebamme (MetaHuman) | **B IMPROVE** | Gesicht glaubwürdig; Frisur wirkt wie ein Helm, Haut unter kaltem Licht fahl |
| Mutter, Haut an Haut (Szene 7) | **B IMPROVE** | Arme glatt ohne Poren und Fältchen; echte Haut: Poren, Knöchelfalten, feine Härchen, fleckige Rötung |
| Neugeborenes | **B IMPROVE** | (in dieser Aufnahme nicht im Bild) gegen Referenzen: violett-rosa, nass, Käseschmiere, geschwollene Lider |
| `L_GEN_EmbryoLookdev` | **D DELETE** | Prüfkarte, von keiner Karte und keinem Code benutzt; durch die Fruchthöhle ersetzt. Erzeugung in `setup_embryo.py` entfernen |
| `SM_GEN_Debris_05`, `SM_GEN_Debris_06` | **D DELETE** | von nichts referenziert, in keinem Skript |
| `SM_GEN_OocyteMatrix`, `M_GEN_Oocyte_Matrix` | **D DELETE** | werden von `setup_oocyte.py` noch angelegt, aber von keiner Karte benutzt – Skript bereinigen, dann löschen |
| `SM_GEN_OocyteZona`, `SM_GEN_MucosaFolds` | **A KEEP** | im Spiel ungenutzt, aber von den Trailer-Werkzeugen gebraucht (`Tools/Trailer/…`) |
| `L_DevSandbox` | **B IMPROVE** | Startkarte des Editors, wird aber mit ins Spielpaket gebaut – aus dem Paket nehmen |
| Trailer-Inhalte `/Game/Genesis/Trailer` | **A KEEP** (Trailer-Produktion) | nicht Teil des Spiels; Namen `VO_Trailer_Placeholder`, `MRQ_Test_1080p` verstoßen gegen die Namensregel – Sache der Trailer-Produktion |
| Kleidung `DefaultGarment bodyShapeB`, `MHC_Mother_Outfits` (MetaHuman) | **D DELETE**, nach Prüfung | unerreichbar; nicht im Repository. Vorher prüfen, ob der MetaHuman-Aufbau sie verlangt |
| `MHC_Mother` (MetaHuman Character) | **A KEEP** | unerreichbar, aber die Quelle der Mutter – ohne sie keine Änderung am Gesicht |
| Blender-Sicherungen `*.blend1`, `*.bak` (lokal) | **D DELETE** | automatische Kopien, die Originale existieren |
| Alte Lookdev-Bilder `ArtSource/Generated/*` (lokal) | **B** sichten | 103 Bilder allein im Eileiter; Belege liegen in `Docs/Media` |
| `Genesis/Build/` (Cook-Protokoll im Projektordner) | **D** ignorieren | gehört nicht ins Repository → `.gitignore` |
| KI-Zielbilder `ArtSource/Reference/Concept/kie` | **B** umgeordnet | als Konzept gekennzeichnet, von echten Referenzen getrennt |

## 4. Abhängigkeitsprüfung der Löschkandidaten

Geprüft nach Doc 00b Punkt 11: Referenzen in der Asset-Registry (harte und weiche), C++, Konfiguration, Python- und
Blender-Werkzeuge, Doku; Speicherstände verweisen nicht auf Assets (nur Simulationsdaten).

| Kandidat | Registry | C++ / Config | Werkzeuge | Ersatz | Ergebnis |
|---|---|---|---|---|---|
| `L_GEN_EmbryoLookdev` | 0 Verweise | – | `setup_embryo.py` baut sie | `L_GEN_Fruchthoehle` | löschen, Skript anpassen |
| `SM_GEN_Debris_05/06` | 0 | – | – | – | löschen |
| `SM_GEN_OocyteMatrix`, `M_GEN_Oocyte_Matrix` | 0 | – | `setup_oocyte.py`, `build_oocyte.py` | – | Skripte bereinigen, dann löschen |
| `SM_GEN_OocyteZona`, `SM_GEN_MucosaFolds` | 0 | – | **Trailer-Werkzeuge** | – | **nicht löschen** |
| `L_DevSandbox` | 0 | `EditorStartupMap` | `CreateDevSandbox.py`, Trailer-Doku | – | nur aus dem Paket nehmen |
| MetaHuman-Kleidung | 1 (unerreichbare Kette) | – | – | – | nach Prüfung des MetaHuman-Aufbaus |

## 5. Maßnahmen und Reihenfolge (Vorschlag)

1. **Aufräumen, was geprüft ist** (sofort, ohne Risiko): Lookdev-Karte, Debris-Meshes, Eizell-Matrix samt
   Skriptzeilen, `L_DevSandbox` aus dem Paket, `Genesis/Build/` ignorieren, Blender-Sicherungen in den Papierkorb,
   KI-Zielbilder als Konzept ablegen, Referenzarchiv nach Doc 00b ordnen.
2. **Fruchthöhle verbessern** (Grundlage der Schwangerschaft): Sättigung, kühler Gegenpol, lebendes Gewebe, Amnion,
   Gefäßbaum – Vorher/Nachher.
3. **Die Schwangerschaft** (GENESIS-044 Teil 1b und 2, Auftrag des Game Directors): Szenen aus Sicht des Kindes und
   das wachsende Kind – mit den Boards aus Doc 36.
4. **Neu bauen:** Furchungskeim und Gebärmutterschleimhaut (größter Abstand zur Realität).
5. **Neu bauen:** Kumuluszellen im Rennen.
6. **Kreißsaal und Menschen:** Licht wärmer mit kühlem Gegenpol, Hautdetail der Mutter, Frisur der Hebamme,
   Neugeborenes gegen die Referenzen.

Performance wird bei jeder Überarbeitung gemessen (Grafikzeit je Szene) und hier nachgetragen.

## 6. Umgesetzt: Aufräumen (Maßnahme 1)

Nach der Abhängigkeitsprüfung oben, 2026-09-23:
- **Entfernt** (Versionskontrolle hält die Historie): `L_GEN_EmbryoLookdev`, `SM_GEN_Debris_05`, `SM_GEN_Debris_06`,
  `SM_GEN_OocyteMatrix`, `M_GEN_Oocyte_Matrix`. Die Skripte erzeugen sie nicht mehr (`setup_embryo.py` ohne
  Prüfkarte; `setup_oocyte.py` ohne Gallerte; `build_oocyte.py` behält die Gallerte nur für das Cycles-Lookdev).
- `L_DevSandbox` wird nicht mehr ins Spielpaket gebaut (bleibt Startkarte des Editors).
- `Genesis/Build/` in `.gitignore`.
- Fünf Blender-Sicherungen (`*.blend1`, 298 MB) in den Papierkorb – Originale vorhanden. **Nicht** gelöscht: zwei
  `*.wav.bak` ohne Original (möglicherweise einzige Kopie einer Sprachaufnahme).
- Referenzarchiv nach Doc 00b geordnet: `Biology/Embryo` (vorher `Embryo`), KI-Zielbilder als `Concept/kie`
  (vorher `kie`), leere Fächer für die weiteren Boards; `ArtSource/Reference/README.md` beschreibt die Struktur, das
  Archiv selbst bleibt lokal.
- Offen: MetaHuman-Kleidung (erst den Aufbau der Figuren prüfen), Sichtung der alten Lookdev-Bilder.

## 7. Umgesetzt: Fruchthöhle verbessert (Maßnahme 2)

| vorher | nachher |
|---|---|
| ![](Media/Audit/05b_Fruchthoehle_vorher.png) | ![](Media/Audit/05c_Fruchthoehle_nachher.png) |

| | Leuchtdichte | warm | kalt | Tiefen | Sättigung |
|---|---|---|---|---|---|
| vorher | 0,23 | 88 % | 0 % | 11,6 % | 0,63 |
| nachher | 0,23 | 75 % | 0 % | 13,6 % | **0,35** |
| Cover | 0,28 | 26 % | 33 % | 5,7 % | 0,25 |

- **Wand und Dottersack** nach den Referenzen dunkler und weniger bunt (Chorionplatte tiefrot statt hell-rot).
- **Farbabstimmung** wie beim Cover: insgesamt weniger Sättigung, die Tiefen leicht kühl.
- **Amnion** mit Falten (scharfe Knicke, am Nabel dichter) und etwas Glanz; feineres Netz.
- **Gefäße der Chorionplatte** teilen sich jetzt ungleich wie echte Gefäße (Murray), mit Seitenzweigen – vorher
  spiegelbildlich gegabelt wie ein Diagramm.
- **Haut des lebenden Embryos** rosiger und durchscheinender (eigene Instanz `MI_GEN_EmbryoHuelle_Lebend`).
- **Gefunden und behoben:** Eine nassere, glattere Wand (Rauheit 0,32) zeigte das Kugelnetz als gestrichelte
  Glanzstreifen; bei 0,55 verschwunden.
- **Kalt bleibt bei 0 % – bewusst:** Im Mutterleib gibt es kein blaues Licht, und ein aufgesetzter
  Blau-Orange-Look wäre Game-Look. Der Gegenpol zum warmen Leben ist hier das tiefe Dunkel (13,6 %).
- **Performance:** Grafikzeit 3,4 ms bei 1066 × 600, 76 Draw-Calls, 1,16 Mio. Dreiecke.
- **Offen:** Die Wand ist noch zu glatt – die faserige, nasse Feinstruktur der Referenz fehlt; der Dottersack wirkt
  etwas matt. Die Aortenbögen am Hals als roter Knoten (Doc 32).
