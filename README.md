# GENESIS: Der Kreislauf des Lebens

Hyperrealistische Cinematic-Lebenssimulation auf Basis von **Unreal Engine 5.8 (C++)**, Blender und MetaHuman.

> **GENESIS erinnert sich.** Eine kleine Handlung aus dem ersten Leben kann viele Stunden
> und mehrere Inkarnationen später wieder Auswirkungen haben.

## Repository-Struktur

| Ordner | Inhalt |
|---|---|
| `Genesis/` | Unreal-Engine-Projekt (`Genesis.uproject`, C++ Source, Plugins, Content, Config) |
| `Genesis/Plugins/Genesis*` | Modulare Spielsysteme – jedes System ist ein eigenes Plugin |
| `Docs/` | Architektur, Datenmodell, Persistenz, Systemdesign, Coding Standards, Roadmap |
| `ArtSource/` | Blender-Quelldateien (`.blend`) und Export-Vorstufen (Git LFS) |
| `Tools/` | Pipeline-Skripte (Blender → Unreal, Build, Tests) |

## Dokumentation

1. [Technische Architektur & Modulstruktur](Docs/01_Architektur.md)
2. [Datenarchitektur & Save-/Persistence-Konzept](Docs/02_Daten_und_Persistenz.md)
3. [Life Simulation Core](Docs/03_Life_Simulation_Core.md)
4. [Soul-System](Docs/04_Soul_System.md)
5. [Causal Memory Graph](Docs/05_Causal_Memory_Graph.md)
6. [Coding Standards](Docs/06_Coding_Standards.md)
7. [Roadmap & Entwicklungsblöcke](Docs/07_Roadmap.md)
8. [Genetik & Epigenetik](Docs/08_Genetik.md)
9. [Decision Engine](Docs/09_Decision_Engine.md)
10. [Body Simulation](Docs/10_Body_Simulation.md)
11. [Audio Core](Docs/11_Audio_Core.md)
12. [Soul Music](Docs/12_Soul_Music.md)
13. [Entstehung: Mikrowelt](Docs/13_Entstehung_Mikrowelt.md)
14. [Embryo: die erste Woche](Docs/14_Embryo.md)
15. [Die Geburt](Docs/15_Geburt.md)
16. [Körperklang](Docs/16_Koerperklang.md)
17. [Die ersten Minuten](Docs/17_Die_ersten_Minuten.md)
18. [Die Stimme](Docs/18_Stimme.md)
19. [Der Klang der Orte](Docs/19_Klang_der_Orte.md)
20. [Der Durchlauf](Docs/20_Der_Durchlauf.md)
21. [Die erste Entscheidung](Docs/21_Die_erste_Entscheidung.md)
22. [Startbildschirm, Einstellungen und Controller](Docs/22_Startbildschirm_und_Steuerung.md)
23. [Der Spielstart](Docs/23_Der_Spielstart.md)
24. [Der Kreißsaal](Docs/24_Der_Kreisssaal.md)
25. [Die Mutter und der erste Blick](Docs/25_Die_Mutter.md)
26. [Recherche und Plan zur AAA-Qualität](Docs/26_Recherche_und_Plan_AAA.md)
27. [Setup: Entwicklungsumgebung](Docs/Setup/01_Entwicklungsumgebung.md)

## Voraussetzungen

- Unreal Engine **5.8.2**
- Visual Studio 2026 (18.x, MSVC 14.50.35723+) **oder** Visual Studio 2022 17.14 (MSVC 14.44.35211+)
- Blender 5.2
- Git + Git LFS
