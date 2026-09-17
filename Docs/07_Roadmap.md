# 07 – Roadmap & Entwicklungsblöcke

Jeder Block wird vollständig abgeschlossen (Definition of Done, siehe `06_Coding_Standards.md`), bevor der nächste beginnt.

Status-Legende: `OFFEN` · `IN ARBEIT` · `DONE`

## Vertical Slice
Spermium → Befruchtung → Embryo → Organentwicklung → Sinne → Geburt → erste Minuten der Kindheit.
Die finale Architektur steht dabei bereits: Soul Seed, DNA, Karma, Memory Graph, Body Simulation, Soundtrack, Decision Engine, Save, MetaHuman-Pipeline, Emotionen.

## Blöcke

| Block | Titel | Inhalt | Status |
|---|---|---|---|
| GENESIS-001 | Project Foundation | Repo, LFS, C++-Projekt, Plugin-Architektur, GenesisCore (Zeit, RNG, Persistenz, Tags, Developer HUD), GenesisSave, Build-/Test-Skripte, Architektur-Doku | IN ARBEIT |
| GENESIS-002 | Soul Engine | Soul Seed, Resonanzen, Echos, Seelenbindungen, Motiv, Inkarnationen, Übertrag zwischen Leben | OFFEN |
| GENESIS-003 | Genetic Code | Genom, Vererbung, Mutation, Ausprägung, Epigenetik, Gen-Katalog-Asset | OFFEN |
| GENESIS-004 | Causal Memory Graph | Kausalgraph, Ursachen-/Folgen-Traversierung, Verdichtung, subjektive Erinnerungen, Verzerrung, Zerfall, Sinnes-Trigger | OFFEN |
| GENESIS-005 | Life Simulation Core | Karma, die 14 Prozessoren, Konsequenz-Netzwerk, Gerüchte, Simulation LOD | OFFEN |
| GENESIS-006 | Decision Engine | Entscheidungssituationen, NPC-Bewertung, Unterbewusstseins-Impulse | OFFEN |
| GENESIS-007 | Body Simulation | Organsysteme, Hormone, Stress, Schlaf, Symptome, Alterung | OFFEN |
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
- Offen: Editor mit C++-Modulen starten, Developer HUD im PIE sichtprüfen, Vorlagen-Content auslagern, Entwickler-Sandbox-Map.
