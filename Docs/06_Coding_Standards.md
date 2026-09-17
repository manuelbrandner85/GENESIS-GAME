# 06 – Coding Standards

## Allgemein
- Grundlage ist der **Unreal Engine Coding Standard**: Tabs, Allman-Klammern, Präfixe `U` `A` `F` `E` `I` `T`.
- **Klassen-, Funktions- und Variablennamen auf Englisch. Kommentare auf Deutsch.**
- Alle reflektierten Typen tragen das Präfix `Genesis`, z. B. `FGenesisSoulSeed`. So gibt es keine Namenskollisionen mit Engine oder Plugins.
- Keine Fake-Funktionen. Temporäre Stubs werden ausdrücklich markiert:
  `// TEMPORARY(GENESIS-0XX): <Grund> – ersetzt durch <Plan>`
- Dieselben Marker gelten für Features: `PROTOTYPE`, `TEMPORARY`, `PLACEHOLDER`, `EXPERIMENTAL`.

## Architektur
- Muster: **Zustands-Struct + zustandslose Logik + dünnes Subsystem**. Logik muss ohne World und GameInstance testbar sein.
- Abhängigkeiten nur zu tieferen Schichten (siehe `01_Architektur.md`). Nach oben wird über Delegates kommuniziert.
- Kein Zufall ohne `FGenesisRandomStream`. Kein `FMath::Rand`, kein `FGuid::NewGuid` in der Simulation.
- Keine Simulationslogik im Actor-Tick. Simulation läuft über `UGenesisWorldClockSubsystem::OnSimulationStep`.
- Persistenter Zustand implementiert `IGenesisPersistentSystem` und registriert sich bei `UGenesisPersistenceRegistry`.
- Verborgene Werte (Karma, Resonanzen, Vertrauen …) werden **nicht** als `BlueprintReadable` exponiert.

## Performance
- Keine Allokationen in heißen Schleifen. `Reserve` nutzen, `TInlineAllocator` wo sinnvoll.
- Daten als zusammenhängende `TArray`s speichern, Indizes zur Laufzeit als `TMap` aufbauen.
- Teure Abfragen (Graph-Traversierung, Gerüchteverbreitung) laufen zeitgesteuert über Simulationsschritte, nicht pro Frame.
- `TRACE_CPUPROFILER_EVENT_SCOPE` in allen Simulationsschritten, die mehr als trivial sind.

## Tests
- Automation-Tests unter `Private/Tests/`, Namensraum `Genesis.<Modul>.<Thema>`.
- Jede Kernregel hat einen Test. Jeder behobene Fehler bekommt einen Regressionstest.
- Ausführung: `Tools/Build/Run-Tests.ps1` (headless).

## Git
- Ein Entwicklungsblock = mindestens ein Commit mit der Nummer: `GENESIS-00X: <Beschreibung im Imperativ>`.
- Binärdateien über Git LFS (`.gitattributes`). Nie versioniert werden `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`.
- Vor dem Commit: Build grün, Tests grün, Log ohne kritische Fehler.

## Definition of Done (pro Block)
- [ ] Code fertig
- [ ] Blender-Assets fertig (falls erforderlich)
- [ ] Unreal-Integration fertig
- [ ] Projekt kompiliert
- [ ] Keine kritischen Fehler im Log
- [ ] Funktion getestet (Automation-Tests grün)
- [ ] Visuell geprüft (falls sichtbar)
- [ ] Performance grob geprüft
- [ ] Dokumentiert
- [ ] Git-Commit erstellt und gepusht
