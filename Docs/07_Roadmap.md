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
| GENESIS-008 | Mind: Emotion & Thoughts | Emotions-Wahrnehmung, Gedankeninventar, Unterbewusstsein | OFFEN |
| GENESIS-009 | Relationships & NPC Memory | Beziehungsdimensionen, NPC-Erinnerungssätze | OFFEN |
| GENESIS-010 | Sperm Environment | Blender-Assets, Niagara-Schwarm, Flüssigkeit, Strömung | OFFEN |
| GENESIS-011 | Fertilization | Befruchtung, Genom-Erzeugung, Übergang | OFFEN |
| GENESIS-012 | Embryo | Zellteilung, Organe, Minispiele | OFFEN |
| GENESIS-013 | Birth | Geburts-Cinematic, First-Person-Sequenz | OFFEN |
| GENESIS-014 | Early Childhood | Unscharfe Wahrnehmung, erste Minuten | OFFEN |
| GENESIS-015 | Vertical Slice Polish | Performance, Audio, Übergänge | OFFEN |

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
