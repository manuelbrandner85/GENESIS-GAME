# 01 – Technische Architektur

Status: **verbindlich** · Engine: Unreal Engine 5.8.2 · Stand: GENESIS-001

---

## 1. Leitprinzipien

| Prinzip | Bedeutung für den Code |
|---|---|
| **GENESIS erinnert sich** | Jede relevante Handlung wird als Ursache-Wirkungs-Knoten gespeichert (Causal Memory Graph). Nichts Wichtiges wird „weggerechnet“. |
| **Simulation ist Wahrheit, Actors sind Darstellung** | Lebens-, Seelen- und Weltzustand liegen in C++-Structs mit IDs. Actors, MetaHumans, Niagara und Sequencer lesen diesen Zustand nur und stellen ihn dar. Gespeichert wird nur die Simulation. |
| **Verborgene Werte** | Karma, Resonanzen, Vertrauen usw. sind nicht in der Spiel-UI oder in Blueprints lesbar. Einzige Ausnahme ist das Developer HUD (nicht in Shipping). |
| **Keine Moralwertung** | Kein System vergibt Belohnung oder Strafe. Alle Systeme erzeugen Konsequenzen und Wahrscheinlichkeiten. |
| **Determinismus** | Zufall läuft ausschließlich über `FGenesisRandomStream` (serialisierbar, ableitbar). Gleicher Seed + gleiche Entscheidungen = gleiche Welt. Das ist Voraussetzung für Tests, Debugging, Zeitlinien-Visionen und späteres Multiplayer. |
| **Daten vor Code** | Handlungen, Kulturen, Gene, Epochen sind Data Assets / Data Tables. C++ implementiert Regeln, nicht Inhalte. |
| **Skalierung durch Simulation LOD** | Level 1 voll, Level 2 reduziert, Level 3 statistisch. |
| **Keine KI-Pflicht** | Generative KI ist optionaler Adapter (World Truth System), nie Voraussetzung. |
| **Stabil vor experimentell** | Experimentelle Engine-Features nur mit stabilem Fallback. |

## 2. Schichtenmodell

Abhängigkeiten zeigen **nur nach unten**. Nach oben wird ausschließlich über Delegates/Events kommuniziert.

```
 ┌───────────────────────────────────────────────────────────────────────────┐
 │ L6  Präsentation   GenesisUI · GenesisCinematics · GenesisAudio*           │
 ├───────────────────────────────────────────────────────────────────────────┤
 │ L5  Kreislauf      GenesisDeath · GenesisAfterlife · GenesisReincarnation  │
 │                    GenesisCreation                                         │
 ├───────────────────────────────────────────────────────────────────────────┤
 │ L4  Welt           GenesisWorld · GenesisSociety · GenesisEconomy          │
 │                    GenesisHistory                                          │
 ├───────────────────────────────────────────────────────────────────────────┤
 │ L3  Leben          GenesisLifeSimulation · GenesisDecision · GenesisBody   │
 │                    GenesisMind · GenesisRelationship · GenesisNPC          │
 │                    GenesisAI · GenesisDream                                │
 ├───────────────────────────────────────────────────────────────────────────┤
 │ L2  Identität      GenesisSoul · GenesisGenetics · GenesisMemory           │
 ├───────────────────────────────────────────────────────────────────────────┤
 │ L1  Dienste        GenesisSave · GenesisPerformance                        │
 ├───────────────────────────────────────────────────────────────────────────┤
 │ L0  Fundament      GenesisCore                                             │
 └───────────────────────────────────────────────────────────────────────────┘
   Querschnitt: GenesisMultiplayer (später), Developer HUD (in GenesisCore, nicht Shipping)
```

Wichtige Trennungen:
- **Soul ≠ DNA.** `GenesisSoul` und `GenesisGenetics` kennen sich nicht. Die Verbindung ist nur eine ID im Inkarnations-Datensatz.
- **Wahrheit ≠ Erinnerung.** `GenesisMemory` speichert das objektive Ereignis (Kausalgraph) getrennt von subjektiven, fehlbaren Erinnerungsspuren.
- **Simulation ≠ Entscheidung.** `GenesisDecision` erzeugt Handlungen, `GenesisLifeSimulation` berechnet ihre Folgen.

## 3. Laufzeit-Architektur

### 3.1 Subsystem-Lebensdauer

| Scope | Unreal-Klasse | Verwendet für |
|---|---|---|
| Spielsitzung (über Level-Wechsel) | `UGameInstanceSubsystem` | Weltuhr, Persistenz, Soul, Memory Graph, Life Simulation, Genetik |
| Welt/Level | `UWorldSubsystem` | Simulation LOD (räumlich), NPC-Repräsentation, PCG, Streaming |
| Lokaler Spieler | `ULocalPlayerSubsystem` | Wahrnehmung, Emotions-Postprocess, UI-Phasen |

Lebenssimulation läuft **nicht** im Actor-Tick. Grund: Level-Streaming (World Partition) darf keine Lebensdaten zerstören.

### 3.2 Zeitmodell

- `FGenesisTimestamp` sind int64-Sekunden seit dem Epochenursprung. Der Kalender hat 365 Tage.
- `UGenesisWorldClockSubsystem` sammelt Echtzeit × `TimeScale` und löst **feste Simulationsschritte** aus (Standard: 1 Weltstunde).
- Kann ein Frame nicht alle Schritte abarbeiten, werden sie zu einem Nachhol-Schritt zusammengefasst. So entsteht keine Frame-Spirale.
- **Zeitsprünge** (`SkipTime`, z. B. Jahre zwischen Lebensabschnitten) erzeugen einen Schritt mit `bIsTimeSkip = true`. Systeme rechnen dann aggregiert statt Schritt für Schritt.
- **Subjektive Zeit** entsteht über `TimeScale`. Der Living World Director stellt ihn je nach Lebensphase und Situation ein. Die Darstellung (Musik, Kamera, Übergänge) liest denselben Wert.

### 3.3 Determinismus

- Ein `WorldSeed` erzeugt über `Derive(Salt)` unabhängige Teilströme pro System und Person. Die Reihenfolge der Aufrufe ist dabei egal.
- Seed-Hashes (`GenesisHash`) sind plattformstabil. `GetTypeHash` wird nie für persistente Daten verwendet.
- Persistente IDs sind `FGuid`. Sie werden in der Simulation deterministisch aus Zufallsströmen erzeugt.

### 3.4 Kommunikation zwischen Modulen

1. **Direkte Aufrufe nach unten** (z. B. LifeSimulation → Memory).
2. **Multicast-Delegates nach oben** (z. B. `OnConsequenceTriggered` → Living World Director, Audio, Cinematics).
3. **Gameplay Tags als gemeinsames Vokabular.** Native Tags stehen in C++ für Tags, die Logik direkt nutzt. Inhalts-Tags stehen in `Config/Tags/*.ini`.
4. **Keine zyklischen Modulabhängigkeiten.** Das erzwingen die `Build.cs`-Dateien.

## 4. Unreal-Technologie-Einsatz

| Technologie | Einsatz in GENESIS | Produktionsweg / Entscheidung |
|---|---|---|
| **C++** | Alle Kernsysteme | Stabil |
| **Blueprints** | Level-Gameplay, Interaktion, Sequenzen, Designer-Konfiguration | Kernlogik bleibt in C++ |
| **Gameplay Ability System** | Moment-zu-Moment-Fähigkeiten von Level-1-Charakteren (Greifen, Krabbeln, Laufen, Sprechen), Effekte auf Körper-Attribute | *Nicht* für Langzeitsimulation (Karma, Beziehungen); dafür wäre GAS zu schwer und replikationsorientiert |
| **StateTree** | NPC-Verhalten (Level 1), Situations-Logik des Living World Director | Stabil |
| **MassEntity** | Nur visuelle Crowds (Level-3-Darstellung) | Level-3-Simulation ist **statistisch ohne Entities**. Mass ist dadurch keine Kernabhängigkeit. |
| **Motion Matching (PoseSearch)** | Fortbewegung, Hinsetzen, Aufstehen, Stolpern | Stabil ab 5.4, Chooser für Alters- und Zustands-Datenbanken |
| **MetaHuman** | Wichtige Menschen, Alterung über parametrische Varianten | MetaHuman Creator in-engine |
| **Nanite / Lumen / VSM** | Standard-Rendering (Hardware-Raytracing-Lumen ist aktiv) | Skalierbarkeits-Profile (Abschnitt 7) |
| **Niagara** | Spermien-Schwarm, Zellumgebung, Partikel, Jenseits | GPU-Sim mit CPU-Fallback |
| **Chaos Physics** | Flüssigkeiten und Gewebe nur, wo spielrelevant | Visuelle Flüssigkeiten bevorzugt über Niagara Fluids und Shader |
| **World Partition** | Offene Welten ohne Ladebildschirm | One File Per Actor, Data Layers für Epochen |
| **PCG** | Vegetation, Landschaft, Siedlungsvariation | Storyorte **nie** vollständig prozedural |
| **MetaSounds** | Adaptiver Soundtrack (Soul-Motiv, Emotion, Epoche) | Stabil |
| **Data Assets / Data Tables** | Handlungen, Kulturen, Gene, Epochen, Berufe | Asset Manager Primary Asset Types |
| **SaveGame** | Container für Speicherdateien | Eigenes Record-Format (siehe 02) |
| **Substrate** | Haut, Augen, organische Materialien | Wird in Phase 8 evaluiert, bis dahin Standard-Materialmodell |

## 5. Projektstruktur

```
GENESIS-GAME/                       Git-Repository (GitHub, LFS)
├── Genesis/                        Unreal-Projekt
│   ├── Genesis.uproject
│   ├── .vsconfig                   Visual-Studio-Komponenten für UE 5.8
│   ├── Config/
│   │   └── Tags/                   Inhalts-Gameplay-Tags (Designer)
│   ├── Source/
│   │   ├── Genesis.Target.cs / GenesisEditor.Target.cs
│   │   └── Genesis/                Primäres Spielmodul (schlank: Verdrahtung)
│   ├── Plugins/
│   │   ├── GenesisCore/            je Plugin: .uplugin, Source/<Modul>/{Public,Private,Private/Tests}
│   │   ├── GenesisSave/
│   │   └── …                       weitere Plugins je Entwicklungsblock
│   └── Content/
│       └── Genesis/                Nur eigene Inhalte (Vorlagen-Content wird entfernt)
│           ├── Data/               DA_/DT_ Handlungen, Kulturen, Genetik, Epochen
│           ├── Characters/         MetaHumans, Skelette, Animation
│           ├── Environments/       Prolog, Embryo, Geburt, Kindheit, …
│           ├── Materials/          Master (M_), Functions (MF_), Instances (MI_)
│           ├── FX/                 Niagara (NS_)
│           ├── Audio/              MetaSounds (MS_), Motive
│           ├── Cinematics/         Level Sequences (LS_)
│           ├── UI/                 Widgets (WBP_)
│           └── Maps/               L_ Karten
├── ArtSource/                      Blender-Quellen (.blend, LFS) + Export-Vorstufen
├── Tools/
│   ├── Build/                      Build-Editor.ps1, Run-Tests.ps1
│   └── Blender/                    Export-/Validierungsskripte
└── Docs/
```

**Asset-Namenspräfixe:** `BP_` Blueprint · `ABP_` Animation Blueprint · `SK_` Skeletal Mesh · `SM_` Static Mesh · `M_` / `MI_` / `MF_` Material · `T_` Textur · `NS_` Niagara · `MS_` MetaSound · `LS_` Level Sequence · `DA_` Data Asset · `DT_` Data Table · `ST_` StateTree · `WBP_` Widget · `L_` Map · `PCG_` PCG Graph.

## 6. Modulstruktur

Jedes System ist ein **eigenes Plugin mit einem Runtime-Modul**. Projekt-Plugins stehen auf `EnabledByDefault: false` und werden in `Genesis.uproject` ausdrücklich aktiviert. Dadurch sind sie einzeln abschaltbar und isoliert testbar.

Aufbau jedes Moduls:
```
Plugins/GenesisX/
├── GenesisX.uplugin                (Abhängigkeiten zu anderen Genesis-Plugins)
└── Source/GenesisX/
    ├── GenesisX.Build.cs
    ├── Public/                     API: USTRUCTs, Subsystems, Interfaces
    └── Private/
        ├── *.cpp
        └── Tests/                  Automation-Tests (Genesis.X.*)
```

Muster für testbare Systeme:
**Zustands-Struct + zustandslose Logik + dünnes Subsystem.** Die Logik arbeitet auf Structs und braucht weder GameInstance noch World. Das Subsystem verbindet Weltuhr, Persistenz und Delegates.

| Modul | Schicht | Verantwortung | Abhängig von | Block |
|---|---|---|---|---|
| GenesisCore | L0 | Weltzeit, deterministischer Zufall, Hashes, Persistenz-Interface & Registry, native Tags, Math, Developer HUD | Engine | 001 ✔ Code |
| GenesisSave | L1 | Speicherdateien Soul/World/Life, Versionen, Prüfsumme, Backup, Rollback, Async-Write | Core | 001 ✔ Code |
| GenesisSoul | L2 | Soul Seed, Resonanzen, Echos, Seelenbindungen, Motiv, Inkarnationen | Core | 002 |
| GenesisGenetics | L2 | Genom, Vererbung, Mutation, Merkmalsausprägung, Epigenetik | Core | 003 |
| GenesisMemory | L2 | Causal Memory Graph, subjektive Erinnerungsspuren, Verzerrung, Zerfall, Trigger | Core | 004 |
| GenesisLifeSimulation | L3 | Karma, die 14 verbundenen Systeme, Konsequenz-Netzwerk, Simulation LOD | Core, Memory, Genetics | 005 |
| GenesisDecision | L3 | Entscheidungssituationen, NPC-Nutzenbewertung, Unterbewusstseins-Impulse | Core, LifeSimulation, Mind | 006 |
| GenesisBody | L3 | Organe, Hormone, Schlaf, Verletzung, Krankheit, Alterung, Symptome | Core, Genetics | 007 |
| GenesisMind | L3 | Emotionen, Gedankeninventar, Unterbewusstsein, Sucht, Trauma | Core, Memory | 008 |
| GenesisRelationship | L3 | Beziehungsdimensionen, Nähe, nonverbale Kommunikation, Lügen, Geheimnisse | Core, Memory, LifeSimulation | später |
| GenesisNPC | L3 | NPC-Leben, Bedürfnisse, Ziele, LOD-Rekonstruktion | Relationship, Body, Mind | später |
| GenesisAI | L3 | StateTree-Tasks, Wahrnehmung, Körpersprache-Erkennung, World-Truth-Adapter | NPC | später |
| GenesisDream | L3 | Traumgenerierung, persistente Traumwelt, innere Dämonen | Mind, Memory, Soul | später |
| GenesisWorld | L4 | Epochen, Orte mit Geschichte, Gegenstands-Biografien, dynamische Städte | Core, Memory | später |
| GenesisSociety | L4 | Zeitgeist, Kultur, Politik, Propaganda-Quellen, Migration | World, LifeSimulation | später |
| GenesisEconomy | L4 | Angebot/Nachfrage, Preise, Berufe (abstrahiert) | World | später |
| GenesisHistory | L4 | Emergente Weltereignisse, Butterfly Engine, Emergent Story Director | World, Memory | später |
| GenesisDeath | L5 | Todesarten, Übergänge, Geistermodus, letzte Worte | Body, Memory | später |
| GenesisAfterlife | L5 | Lebensrückblick, Karma-Gericht, Jenseitsbereiche, Kosmische Bibliothek | Memory, Soul, LifeSimulation | später |
| GenesisReincarnation | L5 | Neuer Körper/Kultur/Epoche, Echo-Entscheidungen, Seelenbegegnungen | Soul, Genetics, World | später |
| GenesisCreation | L5 | Schöpfungsmodus, Welt-Parameter, Beobachtung | World, Society | Endgame |
| GenesisAudioCore | L6 | Hörwahrnehmung aus dem Körper, Körperklang-Parameter, Mix-Engine (Prioritäten, Ducking) | Core, Body | 008 |
| GenesisSoulMusic | L6 | Leitmotive, Phasen-Instrumentierung, Familienmotive, Beziehungsmotive, Erinnerungsfragmente, Todeskomposition | Core, Soul, Memory | 009 |
| GenesisMusicDirector | L6 | Musikschichten, Emotion → Musik, Stille, subjektive Zeit | AudioCore, SoulMusic | 010 |
| GenesisVoiceSystem · GenesisDialogueSystem | L6 | Stimmprofile, Dialog-Datenbank, Lokalisierung, Weltwahrheit, Untertitel | AudioCore, Memory, LifeSimulation | 012–013 |
| GenesisCinematics | L6 | Adaptive Cinematic Director, Kamera-Hybrid FP/TP | Mind, World | später |
| GenesisUI | L6 | Phasenabhängige UI, Startmenü-Reflexion | Soul, Mind | später |
| GenesisPerformance | L1 | Skalierbarkeitsprofile, Budgets, Simulation-LOD-Budgets | Core | später |
| GenesisMultiplayer | quer | Seelen fremder Spieler, Griefing-Schutz | Soul, Save | später |

`GenesisMind` ist eine Ergänzung zur ursprünglichen Modulliste. Emotionen, Gedankeninventar und Unterbewusstsein sind eine eigene Domäne. Sie gehören weder in Body noch in LifeSimulation.

### Zuordnung der 14 Life-Simulation-Systeme

Alle 14 Systeme laufen als **Prozessoren in einer gemeinsamen Pipeline** in `GenesisLifeSimulation`. Ein gemeinsamer Frame dient als Blackboard, deshalb rechnet nie ein System isoliert. Details folgen in `03_Life_Simulation_Core.md` (GENESIS-005).

| Stufe | Systeme |
|---|---|
| Wahrnehmung | Kulturelle Identität → Missverständnis |
| Inneres | Indoktrination → Glaube → Propaganda → Zeitgeist → Gruppenzwang → Egoismus/Altruismus → Doppelmoral |
| Sozial | Vertrauen → Ruf & Gerüchte |
| Integration | Karma → Genetik/Epigenetik → Konsequenz-Netzwerk (schreibt in den Causal Memory Graph) |

## 7. Simulation Level of Detail

| Stufe | Wer | Was wird simuliert | Kosten |
|---|---|---|---|
| **L1 Voll** | Personen in Spielernähe / im Gespräch | Alle 14 Systeme, Körper, Emotion, Gedanken, jede Erinnerung, Körpersprache, StateTree | hoch, wenige Dutzend |
| **L2 Reduziert** | Wichtige entfernte NPCs (Familie, Freunde, Rivalen) | Pipeline mit reduzierter Erinnerungscodierung (nur bedeutsame Ereignisse), tägliche statt stündliche Schritte, keine Actors | mittel, Hunderte |
| **L3 Statistisch** | Bevölkerung | Keine Einzelpersonen: Verteilungen, Kohorten, Zeitgeist-Druck, Wirtschaft | sehr gering |

**Aufstufung L2→L1 bzw. L3→L2:** Wird eine Person wieder relevant, rekonstruiert das System ihr Leben plausibel. Grundlage sind der gespeicherte Zustand, die seitdem vergangene Zeit, Kohorten-Statistik und deterministische Ableitung aus dem Welt-Seed. Rekonstruierte Ereignisse werden als Kausalknoten mit reduzierter Detailtiefe eingefügt.

## 8. Skalierbarkeit (Grafik)

| Profil | Rendering | Simulation |
|---|---|---|
| ULTRA | HW-Lumen, VSM hoch, volle Crowds, volumetrische Effekte | identisch |
| HIGH | HW-Lumen, leicht reduzierte Schatten und Volumetrics | identisch |
| MEDIUM | SW-Lumen, weniger Crowds und Partikel | identisch |
| LOW | Reduzierte GI, niedrige Crowd-Dichte | L2-Budget reduziert, **Gameplay identisch** |

Zielhardware für Entwicklung: RTX 5070 (12 GB), Ryzen 7 5700X, 32 GB RAM.

## 9. Developer HUD

- Konsole: `showdebug Genesis` zeigt alle Seiten, `genesis.Debug.Page <Id>` nur eine Seite.
- Jedes System registriert eine Seite über `GenesisDebug::RegisterPage`.
- Nicht in Shipping-Builds. Hier (und nur hier) werden verborgene Werte sichtbar.

## 10. Multiplayer-Vorbereitung

Multiplayer ist noch nicht implementiert. Heute gilt schon:
- Alle persistenten Daten sind ID-basiert und enthalten keine Zeiger.
- Die Simulation ist deterministisch und serverautoritativ auslegbar.
- Seelen sind von Körpern getrennt, deshalb kann eine reale Spielerseele später in einem fremden Leben existieren.
