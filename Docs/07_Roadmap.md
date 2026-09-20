# 07 – Roadmap & Entwicklungsblöcke

Jeder Block wird vollständig abgeschlossen (Definition of Done, siehe `06_Coding_Standards.md`), bevor der nächste beginnt.

Status-Legende: `OFFEN` · `IN ARBEIT` · `DONE`

## Vertical Slice
Spermium → Befruchtung → Embryo → Organentwicklung → Sinne → Geburt → erste Minuten der Kindheit.
Die finale Architektur steht dabei bereits: Soul Seed, DNA, Karma, Memory Graph, Body Simulation, Soundtrack, Decision Engine, Save, MetaHuman-Pipeline, Emotionen.

## Blöcke

| Block | Titel | Inhalt | Status |
|---|---|---|---|
| GENESIS-001 | Project Foundation | Repo, LFS, C++-Projekt, Plugin-Architektur, GenesisCore (Zeit, RNG, Persistenz, Tags, Developer HUD), GenesisSave, Build-/Test-Skripte, Architektur-Doku | DONE |
| GENESIS-002 | Soul Engine | Soul Seed, Resonanzen, Echos, Seelenbindungen, Motiv, Inkarnationen, Übertrag zwischen Leben | DONE |
| GENESIS-003 | Genetic Code | Genom, Vererbung, Mutation, Ausprägung, Epigenetik, Gen-Katalog-Asset | DONE |
| GENESIS-004 | Causal Memory Graph | Kausalgraph, Ursachen-/Folgen-Traversierung, Verdichtung, subjektive Erinnerungen, Verzerrung, Zerfall, Sinnes-Trigger | DONE |
| GENESIS-005 | Life Simulation Core | Karma, die 14 Prozessoren, Konsequenz-Netzwerk, Gerüchte, Simulation LOD | DONE |
| GENESIS-006 | Decision Engine | Entscheidungssituationen, NPC-Bewertung, Unterbewusstseins-Impulse | DONE |
| GENESIS-007 | Body Simulation | Organsysteme, Hormone, Stress, Schlaf, Symptome, Alterung | DONE |
| GENESIS-008 | Audio Core | Hörwahrnehmung aus dem Körper (Mutterleib, Geburtssprung, Alter, Tunnel, Tinnitus), Körperklang-Parameter, Mix-Engine mit Prioritäten und weichem Ducking | DONE |
| GENESIS-009 | Soul Music | Seelenmotiv, Phasen-Instrumentierung, Charaktermotive, Vererbung, Erinnerungsfragmente, Lebens-Soundtrack, Todeskomposition | DONE |
| GENESIS-010 | Entstehung: Mikrowelt | Eileiter-Ampulle (Blender, Nanite), Spermienzelle, Schwimmmodell (CASA, Rheotaxis, Wandbindung), Schwarm, Schwebeteilchen, Mikroskop-Kamera mit Endoskoplicht | DONE |
| GENESIS-011 | Seelenmusik hörbar | Acht Instrumente als Synthese, Nachhall, Leitmotiv folgt der Lebensphase, Hörproben als WAV | DONE (Music Director offen) |
| GENESIS-012 | Körperklang | Synthese von Herzschlag, Blutstrom, Mutterleib, Atem; Hörwahrnehmung wird hörbar; Hörproben als WAV | DONE (Musik und Weltklang offen) |
| GENESIS-013 | Voice System | Stimmprofil aus dem Körper, Stimmwechsel, Alters- und Krankheitsstimme, Säuglingslaute, Stimme der Mutter durch den Mutterleib | DONE (Sprache offen) |
| GENESIS-014 | Dialogue System | Dialog-Datenbank, Lokalisierung DE/EN/IT, Weltwahrheit, Gerüchte, Untertitel | OFFEN |
| GENESIS-015 | Vertical-Slice-Audio | Klang der Orte (Eileiter, Mutterleib, Kreißsaal), Darmgeräusche, Mutterkuchen, Monitor im Pulstakt – und die Mischung wirkt | DONE (Raumhall und Ortung offen) |
| GENESIS-016 | Mind: Emotion & Thoughts | Emotions-Wahrnehmung, Gedankeninventar, Unterbewusstsein | OFFEN |
| GENESIS-017 | Relationships & NPC Memory | Beziehungsdimensionen, NPC-Erinnerungssätze | OFFEN |
| GENESIS-018 | Entstehung: Feinschliff | Flimmerhärchen mit metachronem Schlag, Akrosomkappe, Epithel-Zellmosaik, durchscheinendes Gewebe | DONE |
| GENESIS-019 | Fertilization | Befruchtung, Eizelle, Corona radiata, Genom-Erzeugung, Übergang | DONE (Bildqualität der Corona offen) |
| GENESIS-020 | Embryo | Furchung, Kompaktierung, Blastozyste, Schlüpfen, Einnistung, Übergabe an den Körper | DONE (Minispiele offen) |
| GENESIS-021 | Birth | Wehen aus Sicht des Kindes, Geburtskanal, erster Atemzug, Übergabe an Körper und Audio | DONE (Kreißsaal offen) |
| GENESIS-022 | Early Childhood | Die erste Stunde: Wärme, Ruhe, Bindung, erstes Anlegen – und die erste Erinnerung eines Lebens | DONE (Kreißsaal und Ton offen) |
| GENESIS-023 | Vertical Slice Polish | Ein Durchlauf von der Befruchtung bis zum ersten Schlaf: Phasenregie, Zeitsprünge, Ortswechsel, Performance gemessen | DONE (Eingabe offen) |
| GENESIS-024 | Befruchtung: Feinschliff | Makro-Schärfentiefe, dichter Cumulus, Zellton je Zelle, Kameraführung der Befruchtung | DONE |

## Protokoll

### GENESIS-001 – Project Foundation
- 2026-09-17: Repository mit GitHub verbunden, Git LFS aktiv, `.gitignore`/`.gitattributes`/`.editorconfig` angelegt.
- 2026-09-17: Bestehendes Unreal-Projekt (First-Person-Vorlage, Blueprint) zu C++-Projekt erweitert. `GenesisCore` und `GenesisSave` implementiert, Tests geschrieben.
- 2026-09-17: Visual Studio 2026 (18.10, MSVC 14.51, Windows SDK 22621) über `Genesis/.vsconfig` installiert.
- 2026-09-17: **Build GenesisEditor Win64 Development: erfolgreich.** 1 Fehler (`FStatId` → `TStatId`) behoben.
- 2026-09-17: **Tests: 9/9 grün** (`Genesis.Core.*`, `Genesis.Save.*`). Log ohne Fehler aus Genesis-Modulen.
- 2026-09-17: Vorlagen-Content (168 MB, First-Person-Template) nach `%LOCALAPPDATA%\GenesisArchive` ausgelagert (nicht gelöscht). Template-Konfiguration entfernt.
- 2026-09-17: Entwickler-Map `/Game/Genesis/Maps/L_DevSandbox` headless per Python erzeugt (`Tools/Unreal/CreateDevSandbox.py`), als Start- und Spiel-Map gesetzt.
- 2026-09-17: Editor lädt die C++-Module ohne Rebuild-Abfrage. **Sichtprüfung im Spiel:** Developer HUD mit Seiten Weltzeit, Save und Soul Engine (`Docs/Media/GENESIS-001_DeveloperHUD.png`).
- 2026-09-17: Performance grob (Code-Review, nicht gemessen): Weltuhr begrenzt auf max. 8 Schritte pro Frame plus einen Nachhol-Schritt, HUD-Seiten nur bei aktivem `showdebug`. Messung mit `stat unit` / Insights ab den ersten sichtbaren Systemen (Body/NPC).
- **Status: DONE.**

![Developer HUD](Media/GENESIS-001_DeveloperHUD.png)

### GENESIS-002 – Soul Engine
- 2026-09-17: `GenesisSoul` implementiert (Soul Seed, Resonanzen, Echos, Bindungen, Motiv, Inkarnationen, Übertrag, Subsystem, Settings, HUD-Seite). Doku `04_Soul_System.md`.
- 2026-09-17: **Build erfolgreich (ohne Fehler). Tests: 5/5 Soul, gesamt 14/14 grün.**
- 2026-09-17: Entwicklerbefehle `genesis.Soul.Create`, `genesis.Soul.SimulateLife`; Seele über mehrere simulierte Leben im HUD geprüft (Echos, Resonanzen, Motiv-Entwicklung).
- **Status: DONE.**

### GENESIS-003 – Genetic Code
- 2026-09-17: `GenesisGenetics` implementiert: Genom mit polygenen und Mendel-Merkmalen, Befruchtung mit Allel-Auswahl und Mutation, Merkmalsausprägung, Epigenetik (Exposition, Rückbildung, transgenerationale Weitergabe), Genom-Pool, Gen-Katalog-Data-Asset, Projekteinstellungen, HUD-Seite, Befehl `genesis.Genetics.SimulateFamily`. Doku `08_Genetik.md`.
- 2026-09-17: **Build erfolgreich. Tests: 5/5 Genetik, gesamt 19/19 grün.**
- 2026-09-17: **Sichtprüfung im Spiel:** Mutter nach 15 Jahren Belastung Stress-Markierung 0,79 / Angstempfindlichkeit 0,48; Kind (Gen. 1) erbt 0,12 (erwartet ½·0,79·0,3 = 0,12). `Docs/Media/GENESIS-003_GeneticsHUD.png`.
- 2026-09-17: Fehler gefunden und behoben: (1) Sandbox-Map enthielt doppelten Himmel (alte SkySphere) → Engine-Warnung im Bild; entfernt per `CreateDevSandbox.py`. (2) Git-LFS `lockable` setzte Maps/Assets schreibgeschützt → Speichern im Editor unmöglich; `lockable` entfernt.
- **Status: DONE.**

### GENESIS-004 – Causal Memory Graph
- 2026-09-17: `GenesisMemory` implementiert: azyklischer Kausalgraph (Sequence-Invariante), Beleg-Kombination, Max-Produkt-Traversierung vorwärts/rückwärts, Ereignissuche, Themen-Wiederholung, Verdichtung mit Kettenerhalt; subjektive Erinnerungsspuren (Intensität, Genauigkeit, Valenz, Verdrängung, Verzerrung, Zerfall, ruhende Spuren, Reiz-Trigger mit Geruchsgewichtung); Memory World, Subsystem mit Zerfall über die Weltuhr, Settings, HUD-Seite, Befehl `genesis.Memory.SimulateLieChain`. Doku `05_Causal_Memory_Graph.md`.
- 2026-09-17: **Build erfolgreich. Tests: 4/4 Memory, gesamt 23/23 grün, 0 Warnungen.** Testskript zählt jetzt auch Tests mit Warnungen.
- 2026-09-17: **Sichtprüfung im Spiel:** Kette Lüge 1408 → Vertrauensverlust → Bindungsangst 1416 → übernommenes Verhalten 1440 → Familienkonflikt 1470 (Pfadstärke 0,34), offene Fäden markiert. `Docs/Media/GENESIS-004_MemoryGraphHUD.png`.
- **Status: DONE.**

### GENESIS-005 – Life Simulation Core
- 2026-09-17: `GenesisLifeSimulation` implementiert: verborgenes 6D-Karma (Sättigung, Gewohnheit), 14 Prozessoren in 4 Stufen mit gemeinsamem Frame, Handlungs-Data-Asset, Lebensprofile, Vertrauensnetz, Ruf, Gerüchte (Verbreitung/Verzerrung/Hörensagen), Zeitgeist, Gruppenzwang, Doppelmoral, verzögerte Konsequenzen, automatische Kausalverknüpfung, Simulation LOD, Engine + Subsystem, Settings, HUD-Seite, Befehl `genesis.Life.SimulateScenario`. Doku `03_Life_Simulation_Core.md`.
- 2026-09-17: Build: 2 Compilerfehler behoben (fehlender Include, Sortier-Prädikat für `TObjectPtr`). Test `Karma.SaturationAndHabit` hatte eine falsche Erwartung (Sättigung dominiert Gewohnheit) → Test korrigiert, Logik unverändert.
- 2026-09-17: **Tests: 9/9 Life, gesamt 32/32 grün, 0 Warnungen.**
- 2026-09-17: **Performance gemessen:** 0,015 ms pro Handlung, 0,106 ms pro Tagesschritt (300 Personen, 3000 Ereignisse).
- 2026-09-17: **Sichtprüfung im Spiel:** Szenario Lüge → Predigt → 2 Jahre: Ehrlichkeit −8,7, Vertrauen Mutter 0,70 → 0,21 (Verrat), Konsequenz ausgelöst, Gerücht an Dritten weitergegeben und verklungen. `Docs/Media/GENESIS-005_LifeSimulationHUD.png`.
- Bekannte Grenze: Gerüchte werden noch nicht je Person/Thema zusammengefasst (geplant mit GenesisSociety).
- **Status: DONE.**

### GENESIS-006 – Decision Engine
- 2026-09-17: `GenesisDecision` implementiert: Entscheidungssituationen, 8 Considerations (Charakter, Motiv, Überzeugung, Kultur, Umfeld, Beziehung, Erfahrung, Unterbewusstsein inkl. Seelen-Echos), Kopf/Bauch/Zeitdruck-Modell, Zögern, innerer Konflikt, Softmax-NPC-Wahl, Spielerwahl mit Timeout → Impuls, prägende Erinnerungen als Kausal-Ursachen, Subsystem, Settings, HUD-Seite, Befehl `genesis.Decision.SimulateDilemma`. Doku `09_Decision_Engine.md`.
- 2026-09-17: Beim Vorab-Nachrechnen der Tests gefunden und behoben: Erfahrung ignorierte die Zugänglichkeit (fast verdrängte Erinnerung prägte so stark wie lebendige); Test hielt ungültige Profil-Referenz; wirkungslose Testzeile entfernt. Build: 1 Fehler (C4458 Variablenüberdeckung) behoben.
- 2026-09-17: **Tests: 7/7 Decision, gesamt 39/39 grün, 0 Warnungen.** **Performance:** 0,071 ms pro Entscheidung mit 5000 Erinnerungen.
- 2026-09-17: **Sichtprüfung im Spiel:** ehrliche Person gesteht (p 0,77); täuschende Person hatte 0,88 für Leugnen und gestand in diesem Lauf trotzdem – gewollt nicht-deterministisch, Aufschlüsselung im HUD korrekt. `Docs/Media/GENESIS-006_DecisionHUD.png`.
- **Status: DONE.**

### GENESIS-007 – Body Simulation
- 2026-09-17: `GenesisBody` implementiert: 9 Organsysteme (Entwicklung, Kapazität, Schaden, Verschleiß), pränataler Zeitplan für Organe und Sinne, Geburt mit Lungenreife, Sinnesreifung und -alterung, Hormone mit Tagesrhythmus, Vitalwerte, Schlaf/Hunger/Durst/Energie, Fitness, Schlafschuld, chronischer Stress, biologisches Alter, Genetik-Anbindung, Verletzungen/Krankheiten/Narben, Vitalversagen, 13 Symptome, Subsystem mit Simulation LOD, HUD-Seite, Entwicklerbefehle. Doku `10_Body_Simulation.md`.
- 2026-09-17: Vor dem Build beim Durchrechnen gefunden: Stress-Herzschaden wurde durch Reparatur sofort neutralisiert (→ dauerhafter Verschleiß eingeführt), Sehschärfe hing fälschlich an Nervensystem-Kapazität, Wachstumskurve zu langsam, Melatonin-Testaussage zu weich.
- 2026-09-17: **Build im ersten Versuch erfolgreich. Tests: 8/8 Body, gesamt 47/47 grün, 0 Warnungen.**
- 2026-09-17: **Gemessen:** biologisches Alter mit 60 bei Dauerstress 67,3 vs. aktiv 53,4; natürliches Lebensende 91,1 Jahre; 0,005 ms Stundenschritt (50 Körper), 0,28 ms Tagesschritt (1000 Körper).
- 2026-09-17: **Sichtprüfung im Spiel:** Befruchtung → 38 Wochen → Geburt: 50 cm, 3,8 kg, Puls 160, Atmung 45, Sehschärfe 0,05, Symptome unscharfes Sehen 0,95 / Herzklopfen / Zittern. `Docs/Media/GENESIS-007_NewbornBodyHUD.png`.
- Persistenz-Doku präzisiert: Körper liegen in der World-Ebene (Ahnen bleiben erhalten).
- **Status: DONE.**

### GENESIS-008 – Audio Core
- 2026-09-17: Audio-/Narrative-Auftrag eingeplant: Audio-Blöcke 008–014 vor Mind/Beziehungen/Vertical Slice eingeschoben (Nummern der folgenden Blöcke verschoben). Audioquelle laut Game Director: Unreal-Engine-Bibliothek → prozedurale MetaSounds aus Engine-Nodes. Freie Fab-Pakete werden bei Bedarf angefragt.
- 2026-09-17: GenesisAudioCore implementiert: Hörwahrnehmung aus der Body Simulation (Mutterleib-Tiefpass nach Gehörreife, Frequenzsprung bei der Geburt, Tunnel-Hören, Fieber, Altersschwerhörigkeit begrenzt auf −12 dB, Tinnitus), Körperklang-Parameter (Mutterherz, eigenes Herz, Atem, Zittern), Mix-Engine (6 Busse, Ducking-Regeln nach Wichtigkeit, Summen-Begrenzung, exponentielle Zeitkonstanten), Subsystem, Settings, HUD-Seite, Befehl genesis.Audio.SimulateDialogue. Doku 11_Audio_Core.md.
- 2026-09-17: Test fand einen Fehler: Der schnelle Geburtsübergang galt nur bis zur halben Strecke, danach zog der Tiefpass träge nach. Behoben durch einen expliziten Übergangszustand bis zum Ankommen (2 %).
- 2026-09-17: Sichtprüfung fand einen Fehler in GenesisBody: Nach einem Zeitsprung blieben die Vitalwerte voll simulierter Körper bis zum nächsten Stundenschritt auf dem alten Stand (Herz 0 bpm im Mutterleib). Behoben: Nach dem Sprung wird einmal die Stunden-Physiologie am neuen Zeitpunkt berechnet.
- 2026-09-17: Neu für automatische Sichtprüfungen: genesis.Debug.After <Sekunden> <Befehl> und Capture-GameScreenshot.ps1 -ShotDelaySeconds. Screenshots zeigen damit eingeschwungene Zustände statt den ersten Frame.
- 2026-09-17: **Build erfolgreich. Tests: 3/3 Audio, gesamt 50/50 grün, 0 Warnungen.**
- 2026-09-17: **Sichtprüfung im Spiel:** 30. Woche: Mutterleib, Tiefpass 803 Hz, Mutterherz 72 bpm, eigenes Herz 140 bpm, Körper hörbar 1,0, Musik −2 dB. Neugeborenes mit Dialog: Luft, Tiefpass 16,8 kHz, Tunnel 0,38, Herz 160 bpm, Atem 45/min; Musik −7,9 dB / Ambient −4,9 dB / Foley −3,0 dB bei laufendem Dialog. Die grünen senkrechten Linien im Bild sind die Kollisionskapsel des Pawns aus showdebug (Engine). Docs/Media/GENESIS-008_AudioWomb.png, Docs/Media/GENESIS-008_AudioNewborn.png.
- 2026-09-17: **Performance gemessen:** Audio-Core-Tick 0,004–0,013 ms pro Frame.
- Offen und bewusst verschoben: **noch kein hörbarer Klang.** Die Parameter steuern ab GENESIS-011 Submixes und MetaSounds. Räumliche Prüfung und Mix-Abhören folgen dort.
- **Status: DONE** (Umfang: Audio-Kernlogik und Integration, ohne Wiedergabe).

![Audio im Mutterleib](Media/GENESIS-008_AudioWomb.png)
![Audio nach der Geburt](Media/GENESIS-008_AudioNewborn.png)

### GENESIS-009 – Soul Music
- 2026-09-17: GenesisSoulMusic implementiert: Leitmotiv je Person (Seelenmotiv der Spielerinkarnation, sonst Charaktermotiv), Instrumentierung für alle 13 Lebensphasen (Spieluhr → Klavier/Gitarre → Streicher → reduziertes Klavier → Orchester → Chor/kosmisch → kaum hörbare Spieluhr), Modus-Raster mit Konturschutz, Motiv-Ähnlichkeit, Familienmotive mit Vererbung, Verschmelzen und Trennen mit Narbe, Erinnerungsfragmente mit emotionaler Färbung und fehlbarem Erinnern, Lebens-Soundtrack mit Verdichtung, Todeskomposition, Seelen-Archiv (Soul-Ebene), Subsystem, Settings, HUD-Seite, Befehl genesis.Music.SimulateLife. GenesisSoul: GenerateMotifFromSeed öffentlich. Doku 12_Soul_Music.md.
- 2026-09-17: Testerwartungen vor dem Build von Hand nachgerechnet. **Build im ersten Versuch erfolgreich. Tests: 6/6 Soul Music, gesamt 56/56 grün, 0 Warnungen.**
- 2026-09-17: **Gemessen:** Ähnlichkeit Kind–Eltern 0,70 / Kind–Fremde 0,43 / Enkel–Großeltern 0,56; ungenaues Erinnern (Genauigkeit 0,2) verändert 50/50 Fragmente; 0,01 ms pro Todeskomposition mit 64 Fragmenten.
- 2026-09-17: Sichtprüfung fand einen musikalischen Fehler: Beim Einrasten in den Modus fielen Halbtonschritte auf denselben Ton („G#5 G#5 G#5“), und die Kontur des Motivs ging verloren. Behoben durch Konturschutz (nächster Modus-Ton in Schrittrichtung), abgesichert durch einen Test über 200 Motive in allen Modi.
- 2026-09-17: **Sichtprüfung im Spiel:** Seelenmotiv in 8 Phasen (Kindheit „C6 A#5 G5 G#5 A#5 G#5“, Alter reduziert auf 4 Noten bei gleicher Länge), Familienmotiv ~ Mutter 0,77 / Vater 0,79 / Seele 0,47, Bindung Fusion 0,9 → Narbe 0,31 nach dem Tod des Partners, 5 Fragmente (schwache Erinnerung ohne Musik), Erinnern genau „C6 B5 G5 A5“ vs. ungenau „C6 B5 F#5 A5“, Todeskomposition 9 Abschnitte / 68 s. Docs/Media/GENESIS-009_SoulMusicHUD.png.
- Offen und bewusst verschoben: hörbare Wiedergabe (GENESIS-011), Schichtung und Emotion → Musik (GENESIS-010), Verschmelzen von Motiven im Jenseits (Afterlife-Audio).
- **Status: DONE** (Umfang: musikalische Logik, Persistenz, Integration, ohne Wiedergabe).

![Soul Music HUD](Media/GENESIS-009_SoulMusicHUD.png)

### TRAILER – Cinematic Reveal Trailer (parallel zu den Blöcken)
- 2026-09-17: Pre-Production nach Trailer-Briefing: Asset-Analyse, Lückenliste, 120-s-Timeline mit 65 Shots (`Docs/Trailer/ShotList.json` als einzige Datenquelle, Tabelle `Docs/Trailer/01_ShotList.md`), Voice-over-Timing, Musikdramaturgie. Doku `Docs/Trailer/00_Trailer_Produktion.md`.
- 2026-09-17: Soul Theme (eigene Komposition, 60 BPM, D-Moll → D-Dur) mit Unreal-Harmonix-Instrumenten; Stimmen über kie.ai (Gemini 3.1 Flash TTS, 2 Takes je Zeile; ElevenLabs über kie.ai war gestört); prozedurales Sound Design; Mix mit gemessener absoluter Stille bei 27,5 und 88–90 s.
- 2026-09-17: **Animatic v1** (Blender-VSE, 1080p, 120 s, mit Ton) und **Unreal-Sequencer**: `L_Trailer_Stage`, `LS_GENESIS_REVEAL_TRAILER` (24 fps, 2880 Frames), 12 Sub-Sequences, 65 CineCameras mit Brennweite und Bewegungs-Blocking, 3 Audio-Tracks, Fade-Track, 65 Marker. Zurückgelesen und geprüft; nur API-Veraltet-Warnungen.
- **Status: ANIMATIC.** Noch kein finales Bildmaterial, keine MetaHumans, Stimmen und Sound als PLACEHOLDER.

### GENESIS-010 – Entstehung: Mikrowelt
- 2026-09-20: Auf Wunsch des Game Directors vorgezogen (vorher GENESIS-017), damit endlich etwas Sichtbares entsteht. Audio-Blöcke rücken nach hinten.
- 2026-09-20: Maßstab festgelegt: **1 µm = 1 Unreal-Einheit**. Begründung und Folgen (Licht, Schärfentiefe) in `13_Entstehung_Mikrowelt.md`.
- 2026-09-20: Blender-Pipeline headless (kein MCP nötig): `build_sperm_cell.py` (Zelle nach WHO-Maßen, 21.600 Flächen, Farbattribut für Zonen) und `build_oviduct_wall.py` (Ampullen-Abschnitt 1.500 µm, 9 Primärfalten mit Sekundärfalten, Vereinigung über OpenVDB, nahtlos kachelbar, 2,7 Mio. Flächen, Nanite).
- 2026-09-20: Plugin `GenesisConception`: Schwimmmodell (progressiv/hyperaktiviert/träge, Rotationsdiffusion, Rheotaxis, Wandbindung), `AGenesisSpermSwarm` (Instanzen, Geißelschlag im Material), `AGenesisFluidParticles`, `AGenesisMicroscopeCameraRig` (Endoskoplicht, Schärfenachführung). **Tests: 4/4 grün.**
- 2026-09-20: **Gemessen:** progressiv VSL 42,5 µm/s, VCL 129 µm/s; hyperaktiviert VCL 255 µm/s, LIN 0,04; Wandansammlung 98 % vs. 23 % bei Gleichverteilung; Rheotaxis +471 µm in 30 s; 187 ns je Zellschritt.
- 2026-09-20: Beim Sichtprüfen gefunden und behoben: (1) FBX-Import verwarf die Geißel als „entartet“ (Einheiten), (2) Kamera erzwang 3:2 mit schwarzen Balken, (3) Belichtung und Lichtstärke passten nicht zusammen (gemessen statt geschätzt: −6,5 EV bei 3.000 cd), (4) Cordsamt-Rippen aus Loft und Voxelraster, (5) offene Schnittflächen zwischen den Wandabschnitten, (6) zu starke Flüssigkeitsstreuung (graue Suppe), (7) Facetten durch zu harte Reduktion.
- 2026-09-20: **Sichtprüfung im Spiel:** `Docs/Media/GENESIS-010_OviductScene.png` – Gewebetunnel mit Falten, zwei Spermien mit sichtbarer Geißelwelle, indirektes Licht in den Falten, Belichtung gemessen (Median 0,29, nichts ausgebrannt).
- Offen (siehe GENESIS-018): Flimmerhärchen, Schleimfilm-Glanz, feines Epithel-Zellmosaik, sichtbare Schwebeteilchen.
- **Status: DONE** (Umfang: erste sichtbare Szene mit Simulation; Feinschliff bewusst als eigener Block).

![Eileiter-Szene](Media/GENESIS-010_OviductScene.png)

### GENESIS-018 – Entstehung: Feinschliff
- 2026-09-20: Nach Kritik des Game Directors („Spermium nicht realistisch, zu flach, Artefakte unrealistisch") direkt im Anschluss an GENESIS-010 gemacht.
- 2026-09-20: **Flimmerhärchen**: 81.456 Halme je Abschnitt auf den zum Kanal zeigenden Flächen (dicht an den Faltenkanten, locker in der Fläche), metachroner Schlag im Material (25 µm Wellenlänge, 8 Hz, schneller Arbeitsschlag Richtung Gebärmutter).
- 2026-09-20: **Spermienkopf**: Akrosomkappe mit Äquatorialfurche in der Geometrie; Material mit Randstreuung und Dichteverlauf statt gleichmäßigem Weiß.
- 2026-09-20: **Epithel**: Zellmosaik mit wechselnden Zellgrößen und Flecken aus Flimmer- und Drüsenzellen; Schleimfilm (glatter an umströmten Spitzen).
- 2026-09-20: **Gemessen im Spiel:** 5,53 ms je Bild (≈ 180 fps), GPU 3,71 ms, 71 Draw Calls; Belichtung gemessen, nichts ausgebrannt.
- 2026-09-20: **Sichtprüfung:** `Docs/Media/GENESIS-018_CiliaEpithelium.png`.
- Offen: Epithel wirkt aus mittlerer Entfernung noch textil; Eizelle und Corona radiata (GENESIS-019).
- **Status: DONE.**

![Zilien und Epithel](Media/GENESIS-018_CiliaEpithelium.png)

### GENESIS-024 – Befruchtung: Feinschliff
- 2026-09-20: **Makro-Optik**: Sensor und Brennweite werden gemeinsam um `MacroScale` (18) vergrößert – gleicher Bildwinkel, aber die hauchdünne Schärfeebene einer Mikroskopaufnahme. Vorher war im Mikrometerraum alles scharf, und die Szene wirkte wie ein Kunststoffmodell.
- 2026-09-20: **Corona radiata neu gebaut**: 2.100 Zellen in fünf überlappenden Lagen (vorher 950 in einer Schale), rundlicher statt radial gestreckt, Ausrichtungsstreuung wächst nach außen, Zufallston je Zelle im Farbattribut, zweiseitiges Laubmodell mit Durchleuchtung, Eigenverschattung an.
- 2026-09-20: **Kameraführung der Befruchtung**: Bindung → Kamera wechselt auf die erfolgreiche Zelle; Verschmelzung → Schnitt auf die ganze Eizelle mit der Cortikalreaktion. Eine Nahaufnahme der Bindungsstelle von außen ist anatomisch unmöglich (der Cumulus ist dicht) – zwei Versuche sind dokumentiert.
- 2026-09-20: **Gemessen:** Zellansicht Median 0,29 / P99 0,81; Eizelle nach der Verschmelzung Median 0,33 / P99 0,76; nichts ausgebrannt. 63 von 63 Tests bestanden.
- 2026-09-20: Neue Entwicklerbefehle für Abstand, Bildwinkel, Blende, Makro-Faktor; automatische Lichtregelung eingebaut (standardmäßig aus, begründet).
- Offen: Cumulus-Gallerte als Streuvolumen kaum sichtbar; Geißel flimmert an den dünnsten Stellen; Ooplasma-Körnung in den Lücken noch zu gleichmäßig.
- **Status: DONE.**

![Cumulus-Oozyten-Komplex](Media/GENESIS-024_Cumulus.png)

### GENESIS-020 – Embryo: die erste Woche
- 2026-09-20: Plugin `GenesisEmbryo`: Furchungsteilungen mit eigener Uhr je Zelle (nicht synchron), Kompaktierung ab 8 Zellen, Blastozyste mit Embryoblast und Trophoblast, Ausdehnung, Schlüpfen aus der Zona, Einnistung.
- 2026-09-20: Der Keim wächst während der Furchung **nicht** – aus einer Zelle von 110 µm werden viele kleine im selben Raum (im Test geprüft: Volumenverhältnis 1,00).
- 2026-09-20: **Übergabe an die Körpersimulation**: Die Entwicklungsqualität der ersten Woche prägt die Organanlagen; mit der Einnistung erklingt die Lebensphase „Embryo".
- 2026-09-20: **Darstellung**: Zellen als Instanzen der Eizell-Kugel, Farbton je Zelle aus der Simulation; Trophoblastzellen liegen flach an der Hülle, damit das Deckgewebe dicht ist. Zellkranz, Polkörper und Zona verschwinden nacheinander.
- 2026-09-20: **Gemessen:** 34 h → 2 Zellen, Tag 3 → 11 Zellen (Morula), Tag 5 → 66 Zellen (Schlüpfen, Embryoblast 22), Tag 10 → eingenistet. 38 % der fremden Keime bleiben stehen (biologisch etwa die Hälfte); der Keim des Spielers nie. 68 von 68 Tests bestanden.
- Offen: Minispiele der Embryonalphase; Zelltrümmer werden gezählt, aber nicht dargestellt; Gebärmutterschleimhaut als Ort fehlt; Zellen wirken noch porzellanartig hell.
- **Status: DONE** (Umfang: Simulation der ersten Woche und ihre Darstellung; Minispiele als eigener Block).

![Blastozyste](Media/GENESIS-020_Blastocyst.png)

### GENESIS-021 – Die Geburt
- 2026-09-20: Plugin `GenesisBirth`: Wehenzyklen mit asymmetrischem Verlauf, Eröffnung in vier Abschnitten, Tiefertreten und Drehung, Sauerstoffeinbrüche unter jeder Wehe mit Erholung dazwischen, Herzschlag, der dem Sauerstoff folgt, erster Atemzug und erstes Zustandsbild.
- 2026-09-20: **Alles aus der Sicht des Kindes**: Druck, Enge, Licht, Kälte, Dumpfheit und Sehschärfe sind Ausgaben der Simulation, keine Effekte der Kamera.
- 2026-09-20: **Übergabe**: Mit der Geburt wird der Körper geboren – dadurch hört das Kind ab sofort in Luft statt in Fruchtwasser; die Musik wechselt in die Lebensphase „Geburt".
- 2026-09-20: **Geburtskanal** in Blender (140 mm lang, engste Stelle 104 mm, Quer- und Längsfalten); Material als zweiseitig durchscheinendes Gewebe, von draußen rot durchleuchtet. Kamera mit Makro-Schärfentiefe: Ein Neugeborenes sieht nur auf Armlänge scharf.
- 2026-09-20: **Gemessen im Spiel:** Geburt nach 9,2 h, 94 Wehen, 42 min Sauerstoffmangel; Sauerstoff schwankt in der aktiven Phase zwischen 0,70 und 1,00, Herzschlag bis 106/min. 73 von 73 Tests bestanden.
- 2026-09-20: Zwei Fehler durch Messung gefunden: Der Geburtsfortschritt lief zwischen den Wehen weiter (26 statt 9 Stunden), und die Testabtastung im Minutentakt traf bei vier Minuten Wehenabstand nie eine Wehe (Aliasing).
- Offen: Kreißsaal und Gestalt sind Platzhalter; Ton der Geburt fehlt (MetaSounds, GENESIS-012); Nabelschnur und Nachgeburt fehlen; Apgar nach einer und fünf Minuten fehlt als eigener Wert.
- **Status: DONE** (Umfang: Simulation und Wahrnehmung der Geburt; die Welt draußen als eigener Block).

![Blick aus dem Geburtskanal](Media/GENESIS-021_BirthCanal.png)

### GENESIS-012 – Körperklang: die hörbare Simulation
- 2026-09-20: Plugin `GenesisSound`: Herzschlag (zwei gedämpfte Schwingungen je Schlag), Herzschlag der Mutter, Blutstrom im Takt des Auswurfs, braunes Grundrauschen des Mutterleibs, Atem und Ohrenrauschen bei Sauerstoffmangel – alles als Synthese aus den Werten der Simulation, keine Klangdateien.
- 2026-09-20: Die Hörwahrnehmung des Audio Core (Tiefpass, Lautheiten) wirkt jetzt hörbar; der Druck einer Wehe macht den Klang lauter und dumpfer.
- 2026-09-20: **Gemessen:** 120 Schläge in 60 s bei 120/min; Energie über 2 kHz an Luft 10,8-mal stärker als im Mutterleib; bei der Geburt springen die Höhen um den Faktor 7,6; Spitzenwert 0,689 (kein Übersteuern); gleicher Seed → Unterschied 0,00000000. 78 von 78 Tests bestanden.
- 2026-09-20: `genesis.Sound.RenderWav <mutterleib|wehen|geburt|neugeboren> [s]` schreibt Hörproben; zwei liegen als Beleg unter `Docs/Media/Audio/`.
- 2026-09-20: Abweichung vom Plan, bewusst: statt MetaSounds-Graphen eigene Synthese in C++ – sie ist kopflos baubar, reproduzierbar und im Test messbar. MetaSounds bleiben für die Musik vorgesehen, wo ein bearbeitbarer Graph den Unterschied macht.
- Offen: Musik wird noch nicht gespielt (GENESIS-011/012 MetaSounds); Welt draußen klingt nicht; Synthese ist mono; Darmgeräusche der Mutter fehlen. **Nachgetragen mit GENESIS-015: Welt und Darmgeräusche sind da.**
- **Status: DONE** (Umfang: Körperklang; Musik und Weltklang als eigene Blöcke).

### GENESIS-011 – Die Seelenmusik klingt
- 2026-09-20: `FGenesisMusicSynth`: acht Instrumente aus Obertonstrukturen, Hüllkurven und (Gitarre) einer schwingenden Saite; Nachhall aus vier Kammfiltern und zwei Allpässen, gesteuert von `Space` und `Presence` der Phrase.
- 2026-09-20: `AGenesisMusicActor` spielt das Leitmotiv der hörenden Person und holt es beim Wechsel der Lebensphase sofort neu – dieselbe Melodie, andere Instrumentierung. Zwischen den Wiederholungen sechs Sekunden Stille.
- 2026-09-20: **Gemessen:** Tonhöhe auf 0,26 % genau (MIDI 69 → 440,4 Hz); Kindheit 8 Noten bei 84 BPM mit Höhenanteil 0,495 gegen Alter 4 Noten bei 58 BPM mit 0,175; Ausklang von 0,029 auf 0,007; Spitzenwert 0,237; zweimal gerendert identisch. 83 von 83 Tests bestanden.
- 2026-09-20: `genesis.Music.RenderWav <lebensphase> [s]` schreibt Hörproben; zwei liegen unter `Docs/Media/Audio/`.
- 2026-09-20: Durch die Messung gefunden: Die Hörprobe wiederholte die Phrase stillschweigend und ließ den Nachhall anschwellen statt verklingen. Wiederholen ist jetzt ein ausdrücklicher Schalter.
- Offen: kein Music Director (Schichten, Emotion → Parameter, subjektive Zeit); keine Überblendung zwischen Phrasen; mono; Erinnerungsfragmente und Todeskomposition werden noch nicht gespielt.
- **Status: DONE** (Umfang: Seelenmusik hörbar; Regie der Musik als eigener Block).

### GENESIS-022 – Die ersten Minuten
- 2026-09-20: Plugin `GenesisEarlyLife`: Stufen der ersten Stunde (erste Atemzüge, ruhige Wachheit, Haut an Haut, erstes Anlegen, erster Schlaf, Unterkühlung), Wärmehaushalt nach dem Newtonschen Abkühlungsgesetz, Ruhe aus Wärme und Stimme, Hunger, Schreien, Bindung mit Sättigung.
- 2026-09-20: **Anschluss ohne Gameplay-Code**: Mit der Geburt beginnt die erste Stunde von selbst; mit dem ersten Anlegen wechselt die Lebensphase auf `LifePhase.Childhood`.
- 2026-09-20: **Die erste Erinnerung eines Lebens** wird im Causal Memory Graph abgelegt – aus der Perspektive dessen, dem etwas geschieht, mit den Sinnesankern Tasten, Riechen und Hören, ohne Bild und ohne Sprache. Die Seele verstärkt ihre Resonanz auf `Theme.Care`.
- 2026-09-20: **Gemessen im Spiel** (Zeugung → Termin → Geburt → erste Stunde in einem Durchlauf): geboren nach 9,2 h; erstes Anlegen nach 32 min bei 37,0 °C und Ruhe 1,00; nach 54 min Bindung 0,78, vertraute Stimme 0,90, erste Erinnerung abgelegt. 88 von 88 Tests bestanden.
- 2026-09-20: **Gemessen im Test:** nach 20 min allein 33,0 °C (unterkühlt) gegen 37,1 °C auf der Haut; nach 45 min Bindung 0,71 mit Haut und Stimme gegen 0,00 allein (dort 45 min geschrien); erste Sekunde Blendung 1,00 bei Sehschärfe 0,040.
- 2026-09-20: Durch die Messung gefunden: Bindung erreichte in einer halben Stunde 1,00 und machte den Wert bedeutungslos – sie wächst jetzt mit Sättigung (eine perfekte erste Stunde legt eine Bindung an, vollendet sie aber nicht).
- Offen: Kreißsaal, Mutter und Gestalt bleiben Platzhalter – die Kamera zeigt die Wahrnehmung, aber es gibt noch nichts zu sehen; Ton der ersten Stunde (Stimme, Raum, Schreien) fehlt; Nabelschnur, Nachgeburt und Apgar nach einer und fünf Minuten fehlen.
- **Status: DONE** (Umfang: Simulation und Wahrnehmung der ersten Stunde; Gestalt und Ton als eigene Blöcke).

![Die erste Stunde](Media/GENESIS-022_FirstHour.png)

### GENESIS-013 – Die Stimme
- 2026-09-20: Plugin `GenesisVoice`: Stimmprofil aus Alter, Geschlecht, Körpergröße, Lungengesundheit, Krankheit, Erschöpfung und Erregung – Grundfrequenz aus den Stimmlippen, Klangfarbe aus der Länge des Ansatzrohrs.
- 2026-09-20: **Synthese nach dem Quelle-Filter-Modell**: Rosenberg-Luftstoß mit Jitter und Shimmer, vier Formantresonanzen, Nasenresonanz, Abstrahlung an den Lippen. Zehn nicht-sprachliche Laute (Schreien, Quengeln, Gurren, Lallen, Lachen, Seufzen, Summen, Beruhigen, Sprechen ohne Worte, Rufen) mit Altersgrenzen: Ein Neugeborenes kann nicht lachen.
- 2026-09-20: **Biologisches Geschlecht im Genom** (Voraussetzung für den Stimmwechsel): Die Eizelle gibt ein X, das Spermium X oder Y – hälftig und deterministisch. Körperliche Angabe; Geschlechtsidentität wird bewusst nicht mitmodelliert.
- 2026-09-20: **Gemessen:** Profil 112 Hz → synthetisiert 112 Hz (0,0 % Abweichung), Frau 196 → 196 Hz (0,2 %); Neugeborenes 450 Hz gegen Kind 265 Hz; Stimmwechsel senkt die Männerstimme um den Faktor 2,1, die Frauenstimme um 1,2; mit 80 Jahren Mann 135 Hz und Frau 175 Hz (Annäherung von 85 auf 40 Hz); Kinderstimme über 1,5 kHz 0,445 gegen 0,277 beim Mann; Schrei 0,263 gegen Sprechen 0,108 mit 25 Atempausen; Stimme der Mutter im Mutterleib über 2 kHz um den Faktor 10,7 gedämpft, Tonhöhe unverändert 203 Hz. 95 von 95 Tests bestanden.
- 2026-09-20: **Im Spiel geprüft** (Geburt im Zeitraffer, HUD-Seite „Stimmen"): Kind 0,0 Jahre, Säuglingsklang 499 Hz, Behauchtheit 0,34, Rauigkeit 0,30; Mutter (Platzhalter) heller Klang 206 Hz; zuletzt „Summen" – die Mutter summt dem schreienden Kind zu. Zwei Stimm-Actors in `L_GEN_Birth`.
- 2026-09-20: `genesis.Voice.RenderWav <neugeboren|saeugling|kind|frau|mann|alt|mutter|heiser>` schreibt Hörproben; vier liegen unter `Docs/Media/Audio/`.
- 2026-09-20: Durch die Messung gefunden: Die Resonanzkette lief dauerhaft in die Begrenzung – jede Stimme war gleich laut und gleich hart, ein Schrei nicht lauter als ruhiges Sprechen. Nach der Pegelkorrektur stimmen die Verhältnisse. Außerdem fehlte der Heiserkeit das Rauschen der unvollständig schließenden Stimmritze.
- Offen: keine Sprache (Worte kommen mit GENESIS-014); die Mutter ist noch keine Person der Simulation; mono, ohne Raum und Ortung; Husten, Niesen, Gähnen fehlen; der Mix im Spiel ist noch nicht gemessen, nur die Hörproben.
- **Status: DONE** (Umfang: Stimme als Körper und nicht-sprachliche Laute; Sprache als eigener Block).

![Stimmen in der Geburtsszene](Media/GENESIS-013_Voices.png)

### GENESIS-015 – Der Klang der Orte
- 2026-09-20: Plugin `GenesisWorldSound`: Grundton aus drei Rauschbändern plus Ereignisse aus einem Poisson-Prozess. Drei Orte des Vertical Slice – Eileiter-Ampulle, Mutterleib von innen, Kreißsaal – jeweils mit eigener Bandgrenze, die auch für jedes einzelne Ereignis gilt.
- 2026-09-20: **Darmgeräusche der Mutter** (in GENESIS-012 als fehlend notiert) sind da: 5 bis 30 je Minute wie beim Menschen. Dazu das Rauschen des Mutterkuchens und der Monitor im Kreißsaal – beide im Takt des mütterlichen Herzens.
- 2026-09-20: **Die Mischung wirkt.** Die Mix-Engine gab es seit GENESIS-008, angewendet hat sie niemand: Körperklang, Musik, Stimmen und Orte holen sich jetzt ihren Bus-Pegel. Wer spricht, meldet sich beim Mix an; ein Schrei läuft über den Bus für lebenswichtige Signale und schneidet schneller durch.
- 2026-09-20: Der Filter der Hörwahrnehmung liegt jetzt in `GenesisAudioCore` (`FGenesisHearingFilter`) und gilt für Stimmen wie für Orte – es ist dasselbe Ohr. In der Geburtsszene stehen zwei Orte gleichzeitig: der Mutterleib von innen und der Kreißsaal von außen, der mit dem ersten Atemzug klar wird.
- 2026-09-20: **Gemessen:** Höhenanteil über 2 kHz – Mutterleib 0,0925, Eileiter 0,3510, Kreißsaal 0,5455; Darmgeräusche 21,0/min bei arbeitender gegen 4/min bei ruhender Verdauung; Mutterkuchen 79,5/min bei Puls 78; Kreißsaal ruhig 2 Instrumente und 10 Schritte gegen 17 und 47 unter der Austreibung; beim Sprechen tritt die Umgebung auf −5,0 dB und die Musik auf −8,0 dB zurück, eine halbe Sekunde später steht die Umgebung bei −3,6 dB. 100 von 100 Tests bestanden.
- 2026-09-20: **Im Spiel geprüft** (HUD-Seite „Klang der Orte", Geburt im Zeitraffer): Mutterleib von innen und Kreißsaal von außen, Puls der Mutter 110/min und Betrieb 0,70 mitten in der Eröffnung.
- 2026-09-20: Durch die Messung gefunden: Die Ereignisse liefen an der Bandgrenze des Ortes vorbei – der Mutterleib klang dumpf, aber jedes Gluckern darin hell. Jetzt filtert der Ort alles, was in ihm geschieht.
- Offen: mono, kein Raumhall, keine Ortung einzelner Ereignisse; die Stimmen im Gang sind Rauschen mit Sprechrhythmus; Türen, Alarme, Wasser und die Atem- und Pressgeräusche der Mutter fehlen.
- **Status: DONE** (Umfang: Klang der Orte des Vertical Slice und wirksame Mischung; Raumakustik als eigener Block).

![Klang der Orte](Media/GENESIS-015_Places.png)

### GENESIS-023 – Der Durchlauf
- 2026-09-20: Plugin `GenesisSlice`: Phasenregie von der Befruchtung über die erste Woche und die Schwangerschaft bis zur Geburt und zur ersten Stunde. Die Regie liest nur, was die Systeme melden, und entscheidet, wann gewartet, gesprungen und der Ort gewechselt wird.
- 2026-09-20: **Ein Leben am Stück, gemessen:** Befruchtung nach 26,3 s, Einnistung 34,2 s, Termin (39,1 Wochen) 47,7 s mit Ortswechsel nach `L_GEN_Birth`, geboren 53,9 s, erstes Anlegen und erste Erinnerung um 70 s, erster Schlaf nach 86,0 s bei 37,0 °C, Ruhe 1,00, Bindung 0,83. Keine Fehler und keine Warnungen aus Genesis-Modulen. 104 von 104 Tests bestanden.
- 2026-09-20: **Performance gemessen** (1137×600): Eileiter 178 FPS (Frame 5,62 ms, Spiel-Thread 2,73 ms, GPU 3,83 ms, 72 Draws, 5,33 M Dreiecke), Geburt 193 FPS (5,18 / 2,01 / 3,19 ms, 75 Draws). Die Simulation ist nicht der Engpass. Offene Engine-Warnung: Ray-Tracing-Geometrie belegt 83 von 400 MiB des Budgets.
- 2026-09-20: **Ehrliche Enden:** „Das Kind schläft" gibt es nur, wenn es eingeschlafen ist; sonst heißt es „Die erste Stunde ist vorbei – das Kind ist nicht zur Ruhe gekommen". Ein abgestorbener Keim und ein Kind, das die Geburt nicht überlebt, beenden den Durchlauf mit ihrem jeweiligen Grund.
- 2026-09-20: Die Versorgung nach der Geburt (Haut an Haut, Ansprache) übernimmt die Welt, nicht der Spieler – bis es eine Eingabe gibt. Mit `genesis.Newborn.SkinToSkin 0` lässt sich das Gegenteil erzwingen.
- 2026-09-20: Zwei Fehler durch den ersten Durchlauf gefunden: (1) Weltuhr und Phasenzeitraffer liefen übereinander, die erste Stunde war nach sieben Sekunden vorbei – die Uhr wird jetzt je Phase gestellt; (2) der Durchlauf meldete „Das Kind schläft", obwohl das Kind unterkühlt und wach war. Außerdem überleben verzögerte Entwicklerbefehle (`genesis.Debug.After`) jetzt einen Ortswechsel und laufen über den PlayerController – ohne diese Korrektur hätte es von diesem Block kein Bild gegeben.
- Offen: keine Eingabe – der Spieler kann noch nichts entscheiden; der Ortswechsel ist ein harter Ladevorgang; zwischen Einnistung und Geburt gibt es nichts zu sehen; Kreißsaal, Mutter und Gestalt bleiben Platzhalter.
- **Status: DONE** (Umfang: der Slice als ein Durchlauf mit gemessener Performance; Eingabe und Gestalt als eigene Blöcke).

![Der Durchlauf](Media/GENESIS-023_Run.png)
