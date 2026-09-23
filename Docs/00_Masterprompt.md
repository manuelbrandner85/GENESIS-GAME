# 00 – Globaler Masterprompt (verbindliche Grundvision)

*Vom Game Director vorgegeben (2026-09-23). Diese Regeln gelten projektweit und dauerhaft, solange er sie nicht
ausdrücklich ändert. Jede Sitzung und jeder Entwicklungsblock baut darauf auf. Dazu: bei jedem inhaltlichen Schritt
gründlich recherchieren (Quellen im jeweiligen Dokument).*

Rollen: leitender Game Director, Technical Director, Gameplay Architect, Unreal-Engine-5-Lead-Developer,
C++-Programmierer, Blueprint-Architekt, AI-Simulation-Engineer, Technical Artist, Blender-Artist,
Character-Technical-Director, Environment Artist, Physics-Engineer, Audio-Designer und Cinematic Director für
**GENESIS: Der Kreislauf des Lebens**.

## 1. Die Grundidee

GENESIS soll kein gewöhnliches Videospiel werden. Es ist eine interaktive Simulation des Lebens, des Bewusstseins,
menschlicher Entscheidungen und ihrer langfristigen Konsequenzen. Der Spieler erlebt einen vollständigen Kreislauf:

Entstehung → Geburt → Kindheit → Jugend → Erwachsenenleben → Alter → Tod → Jenseits → Wiedergeburt → kosmische
Entwicklung → Schöpfung

Das Spiel beginnt nicht mit einem fertigen erwachsenen Charakter, sondern bereits vor der Geburt – zunächst auf
mikroskopischer Ebene. Der Spieler ist Teil des biologischen Prozesses, aus dem später sein Charakter entsteht. Das
Leben danach entsteht aus Genetik, Umwelt, Zufall, Beziehungen und den Entscheidungen des Spielers. Es gibt keinen
klassischen linearen Spielweg. Jedes Leben soll anders verlaufen.

## 2. Nicht verhandelbare Grundsätze

1. Hauptengine ist Unreal Engine 5.
2. Blender für Modelle, Assets, Sculpting, Retopologie, UVs, bestimmte Simulationen, Animationen und Cinematics.
3. Kein Flutter, Flame oder Unity.
4. Zielplattform zunächst High-End-PC.
5. Visuelles Ziel: AAA+ / Cinematic / Hyperrealismus / Fotorealismus.
6. Menschen dürfen nicht wie typische Computerspielfiguren wirken.
7. Haut, Augen, Haare, Kleidung, Materialien, Gebäude, Vegetation und Umgebungen glaubwürdig und organisch.
8. Unreal Engine soll grundsätzlich das technisch und visuell bestmögliche Ergebnis liefern.
9. Qualitätsreduktionen nur, wenn Performance oder Hardware sie notwendig machen.
10. Die Architektur bleibt skalierbar, sodass später verschiedene Grafikstufen möglich sind.
11. Gameplay-Systeme modular.
12. Zentrale Systeme vorzugsweise in C++.
13. Designer-, Content- und Konfigurationslogik über Blueprints und Data Assets.
14. Keine riesigen unwartbaren Blueprints.
15. Keine kurzfristigen Hacks, wenn eine saubere modulare Lösung möglich ist.
16. Jede bedeutende Gameplayfunktion langfristig erweiterbar.
17. Keine voneinander getrennten Minispiele.
18. Alle Lebensphasen durch ein gemeinsames Simulationssystem verbunden.
19. Entscheidungen früher Lebensphasen können Jahrzehnte später noch wirken.
20. NPCs besitzen eigene Leben und existieren nicht nur für den Spieler.
21. Kein simples Gut/Böse-System.
22. Karma existiert, bleibt aber überwiegend verborgen.
23. Konsequenzen erleben statt Zahlen und Moralpunkte.
24. Erinnerungen und Ursachen werden dauerhaft verbunden.
25. Genetik und Epigenetik beeinflussen den Charakter.
26. Körper und Erscheinungsbild verändern sich über das Leben.
27. Psychische und emotionale Zustände beeinflussen die Wahrnehmung der Welt.
28. Träume sind ein tatsächlicher Bestandteil der Simulation.
29. Tod bedeutet nicht Game Over.
30. Nach dem Tod geht das Spiel weiter.
31. Wiedergeburt ist Teil der langfristigen Spielarchitektur.
32. Verschiedene historische Epochen und Weltzustände sollen später möglich sein.
33. Der Spieler entwickelt sich vom einzelnen biologischen Leben zu einer größeren kosmischen Perspektive.
34. Am Ende kann aus dem Erleben von Leben, Tod und Wiedergeburt eine Form von Schöpfung entstehen.

## 3. Die neun großen Spielphasen

**Phase 1 – Ursprung.** Mikroskopischer Beginn, biologisch inspirierter Spermium-Prolog, keine Arcade-Darstellung;
organisch, fremdartig, lebendig und wissenschaftlich glaubwürdig. Faktoren: Bewegung, Energie, Strömungen,
biologische Hindernisse, Konkurrenz, Orientierung, chemische Signale, Überlebensfähigkeit, Zufall. Die Befruchtung
ist kein Gewinnbildschirm, sondern der Beginn einer neuen Existenz.

**Phase 2 – Embryo und fötale Entwicklung.** Die Perspektive verändert sich vollständig. Darzustellen: Zellteilung,
Embryonalentwicklung, Körperstrukturen, Organentwicklung, Herzaktivität, Nervensystem, Gehirnentwicklung,
Entwicklung der Sinne, Reaktionen auf Geräusche, Lichtwahrnehmung, Bewegungen, Einflüsse der Umgebung. Emotional und
visuell außergewöhnlich – nicht wie ein Biologie-Lernprogramm.

**Phase 3 – Geburt und frühe Kindheit.** Die Geburt ist ein zentraler Signature Moment: Perspektive, Kamera, Audio
und Wahrnehmung ändern sich fundamental. Das erste Licht, der erste Atemzug, Geräusche, Stimmen, Temperatur,
Berührung, unscharfes Sehen, Überforderung, Nähe, Angst, Geborgenheit. Danach frühe Kindheit: Interaktion wächst mit
dem Kind (Krabbeln, Gehen, Greifen, Sprechen, Erkennen, Lernen, emotionale Bindungen). Die Welt fühlt sich aus Sicht
eines kleinen Kindes anders an.

**Phase 4 – Kindheit.** Persönlichkeit, Fähigkeiten, Erinnerungen, Beziehungen; Familie, Freundschaften, Schule,
Entdeckung, Ängste, Neugier, Talente, Konflikte, Vertrauen, Traumata, Freude, Umwelt, Erziehung, soziale
Bedingungen. Frühe Erfahrungen können Jahrzehnte später wieder Bedeutung bekommen.

**Phase 5 – Jugend.** Körper und Identität verändern sich; Pubertät, Liebe, Ablehnung, Gruppendruck, Schule,
Ausbildung, Selbstbild, Sexualität im Rahmen der Altersfreigabe und erzählerischen Verantwortung,
Zukunftsentscheidungen, Konflikte mit Eltern, Risikoentscheidungen, erste langfristige Lebensentscheidungen.

**Phase 6 – Erwachsenenleben.** Die Simulation öffnet sich vollständig: Beruf, Selbstständigkeit, Familie,
Partnerschaften, Trennung, Kinder, Freundschaften, Umzüge, Finanzen, Besitz, Gesellschaft, Kriminalität, Gesundheit,
Spiritualität, Forschung, Kunst, Macht, Reichtum, Armut, Abenteuer, Rückzug. Jede Entscheidung öffnet und schließt
Möglichkeiten. Es gibt keinen „richtigen" Lebensweg.

**Phase 7 – Alter und Tod.** Sichtbares und funktionelles Altern: Haltung, Bewegung, Muskelkraft, Haut, Haare,
Stimme, Reaktionsgeschwindigkeit, Sehen, Hören, Ausdauer, Erinnerungszugriff, Gesundheit. Beziehungen und
Erinnerungen gewinnen an Bedeutung. Der Tod kann auf sehr unterschiedliche Weise eintreten – und ist niemals ein
„Game Over"-Bildschirm.

## 4. Nach dem Tod

Der Übergang ins Jenseits soll eines der stärksten Erlebnisse werden – keine religiöse Behauptung, sondern eine
eigene fiktionale kosmische Mythologie. Der Spieler löst sich vom Körper, Erinnerungen bleiben teilweise erhalten,
das vorherige Leben wird aus neuer Perspektive betrachtet; manche Entscheidungen werden erst jetzt verständlich.

## 5. Die Kosmos-Bibliothek

Eine begehbare kosmische Bibliothek repräsentiert Leben, Erinnerungen, Entscheidungen, Möglichkeiten, Epochen,
Ahnen, Welten und Erfahrungen. Kein Menü, sondern ein atmosphärischer Ort; die Architektur kann sich dynamisch
verändern, Bereiche öffnen sich durch Erfahrungen früherer Leben.

## 6. Wiedergeburt

Kein „New Game+". Frühere Existenzen hinterlassen indirekte Spuren: Träume, Déjà-vus, Instinkte, besondere Ängste,
ungeklärte Fähigkeiten, emotionale Reaktionen, kosmische Erinnerungsfragmente. Wiedergeburt in unterschiedlichen
Familien, Regionen, gesellschaftlichen Bedingungen und langfristig Epochen.

## 7. Kosmisches Bewusstsein

Nach mehreren Lebenszyklen erkennt der Spieler größere Zusammenhänge zwischen Menschen, Generationen,
Entscheidungen, Natur, Geschichte, Gesellschaft, Leben und Tod – ein Netzwerk verbundener Existenzen.

## 8. Schöpfung

Kein simpler Creative Mode, sondern die höchste Entwicklungsstufe: grundlegende Bedingungen einer neuen Welt
beeinflussen (Biologie, Umwelt, Ökosysteme, Kulturen, gesellschaftliche Entwicklung, Lebensbedingungen). Damit
schließt sich der Kreislauf.

## 9. Die Life & Decision Simulation Engine – 14 Kernsysteme

Kein wichtiger Lebensbereich wird isoliert simuliert; die Systeme tauschen Informationen aus.

1. **Genetics & Biology** – DNA, Vererbung, körperliche Merkmale, Prädispositionen, Mutationen.
2. **Epigenetics & Development** – Umwelt und Erfahrung prägen langfristig Körper und biologische Prozesse.
3. **Body Simulation** – Alter, Körperzustand, Kraft, Ausdauer, Schlaf, Energie, Schmerz, Verletzungen,
   Krankheiten, Regeneration, Entwicklung.
4. **Brain, Emotion & Psychology** – Angst, Freude, Trauer, Stress, Liebe, Wut, Neugier, Vertrauen, Einsamkeit,
   Bindung; Emotionen beeinflussen Verhalten und Wahrnehmung.
5. **Memory** – Erinnerungen mit Bedeutung, Stärke, Emotion, Verknüpfungen; verblassen, verdrängt, verstärkt,
   reaktiviert.
6. **Causal Memory Graph** – gerichteter Konsequenzgraph, auch für sehr langfristige Folgen.
7. **Relationships** – mehrdimensional: Vertrauen, Nähe, Respekt, Angst, Abhängigkeit, Liebe, Anziehung,
   Familienbindung, Erinnerungen, Konflikte, Vergebung.
8. **Social World** – Familie, Freundeskreise, Schule, Arbeit, Gemeinschaft, Status, Netzwerke, Gerüchte,
   Gruppendynamik.
9. **Knowledge, Skills & Personality** – Fähigkeiten durch Erfahrung, dynamische Persönlichkeit.
10. **Economy & Life Conditions** – Geld, Eigentum, Beruf, Armut, Wohlstand, Wohnen, Ressourcen, Möglichkeiten.
11. **World & Environment** – Umwelt verändert Menschen, Menschen verändern Umwelt.
12. **Hidden Karma & Consequence** – kein sichtbares Punktesystem; wirkt im Hintergrund auf Wahrscheinlichkeiten,
    Beziehungen, Folgen, spätere Lebenszyklen – nie unmittelbar strafend oder belohnend.
13. **Dream & Subconscious** – Träume aus Erinnerungen, Ängsten, Wünschen, Traumata, aktuellen Ereignissen,
    früheren Lebensfragmenten; persistente, sich verändernde Traumwelt.
14. **Time, Life Cycle & Cosmic Continuity** – Alter, Lebensphasen, subjektive Zeit, Tod, Jenseits, Erinnerungen,
    Wiedergeburt, Epochen, kosmische Entwicklung.

## 10. Subjektive Zeit

Zeit fühlt sich nicht immer gleich an: In der Kindheit dauert ein Jahr lange, später vergehen Jahre schneller;
besondere Momente dehnen sich, Stress und Gefahr verändern die Zeitwahrnehmung, Erinnerungen fassen Jahre in
Sekunden. Vermittelt über Gameplay, Kamera, Audio, Schnitt und Wahrnehmung.

## 11. Emotionsabhängige Wahrnehmung

Emotionen sind keine UI-Werte; sie beeinflussen Kamerabewegung, Fokus, Tiefenschärfe, Umgebungsgeräusche, Musik,
Atmung, Herzschlag, Licht- und Farbwahrnehmung, NPC-Wahrnehmung, Bewegung, Animation – nie billig oder übertrieben.

## 12. NPC-Life-Simulation

NPCs haben Alter, Familie, Erinnerungen, Beziehungen, Ziele, Bedürfnisse, Berufe, Tagesabläufe, Persönlichkeiten,
Emotionen, Gesundheit, Entscheidungen – und handeln auch ohne den Spieler (heiraten, Kinder bekommen, umziehen,
Karriere machen, scheitern, krank werden, altern, sterben). Die Welt wartet nicht auf den Spieler.

## 13. Generationensystem

Kinder werden erwachsen, Eltern alt, Großeltern sterben; Handlungen und genetische, gesellschaftliche oder
emotionale Folgen wirken über Generationen.

## 14. Genetik und Epigenetik

Genetik beeinflusst Gesicht, Körperbau, Haut, Haare, Augen, Größe, Stoffwechsel, Potenziale und Prädispositionen –
legt aber nie das ganze Schicksal fest. Umwelt, Lebensweise, soziale Faktoren und Entscheidungen wirken mit.

## 15. Sichtbare Alterung

Kontinuierliches Alterungssystem statt verschiedener Modelle: Gesichtsform, Haut, Falten, Haare, Körperfett,
Muskulatur, Haltung, Bewegung, Gang, Stimme, Augenbereich, Hände, Reaktionsverhalten, Gesundheit.

## 16. Charakterdarstellung

MetaHuman, realistische Skelette, Control Rig, IK Rig, IK Retargeter, Motion Matching, Motion Warping, Pose Search,
realistische Gesichtsanimation, plausible Haare und Kleidung. Bewegungen mit Gewicht, Trägheit, Mikrobewegungen,
Blickverhalten, Atmung, Balance, Reaktion auf Gelände.

## 17. Spielperspektive

Hybrid aus First Person, Third Person und Cinematic, abhängig von Lebensphase und Situation. Intime
Wahrnehmungsmomente First Person, Exploration Third Person, große Übergänge filmisch. Nie eine statische
Standard-Game-Kamera.

## 18. Weltarchitektur

World Partition, Data Layers, Level Instances, PCG, Streaming, HLOD, Nanite, Lumen, Virtual Shadow Maps. Die Welt
muss sehr groß werden können und darf nicht dauerhaft komplett im Speicher liegen.

## 19. PCG

Für skalierbare Welten (Vegetation, Landschaften, Details, Gebäudevariationen, Biome, Naturentwicklung, historische
Veränderungen) – ohne sichtbare Wiederholung. Hero-Locations werden gezielt gestaltet.

## 20. Physik

Chaos Physics für Körper, Objekte, Fahrzeuge, Kleidung, Zerstörung, Umwelt, Materialien – mit LOD-, Distanz- und
Relevanzsystemen.

## 21. Audio

Audio ist zentrales Gameplayelement: MetaSounds und räumliches Audio. Musik entwickelt sich aus dem Leben des
Spielers, kein generischer Dauersoundtrack. Motive verbinden sich mit Menschen, Erinnerungen, Orten, Lebensphasen,
Liebe, Verlust, Tod, Wiedergeburt; ein Kindheitsmotiv kann Jahrzehnte später wiederkehren.

## 22. Traumwelt

Eigene visuelle Regeln, surreale Geometrie und unmögliche Räume – aber nicht zufällig, sondern aus dem Leben des
Spielers (Haus, Person, Stimme, Ereignis, Ort, Trauma, Wunsch als wiederkehrende Elemente).

## 23. Save-System

Speichert langfristig Identität, DNA, Epigenetik, Alter, Körperzustand, Fähigkeiten, Persönlichkeit, Erinnerungen,
Causal Graph, Beziehungen, NPC-Zustände, Familien, Weltzustand, Entscheidungen, Träume, Lebensgeschichte, Karma,
kosmische Progression, frühere Lebenszyklen. Versioniert; ältere Spielstände bleiben möglichst gültig.

## 24. Event-Architektur

Lose Kopplung: Interfaces, Components, Gameplay Tags, Delegates, Subsystems, Data Assets, Event Dispatcher, GAS nur
wo es Vorteile bringt. Keine unnötigen direkten Abhängigkeiten.

## 25. AI

StateTree, Behavior Trees, EQS, Smart Objects, Mass Entity / Mass AI, Navigation, Utility-Entscheidungen. AI
skaliert; entfernte NPCs vereinfacht, nahe mit höherer Simulationsstufe.

## 26. Simulation LOD

Level 0 statistische Fernsimulation · Level 1 Lebens- und Ereignissimulation · Level 2 detaillierte soziale
Simulation · Level 3 vollständige Gameplay-AI · Level 4 lokale Darstellung mit Animation, Physik, Wahrnehmung.
Übergänge unsichtbar.

## 27. Performance

Hyperrealismus heißt nicht schlechte Performance. Von Anfang an Profiling (Unreal Insights, GPU Profiler, Stat
Commands, Memory Profiling) und Budgets für Assets, Texturen, Animation, AI, Streaming. Kein „am Ende optimieren".

## 28. Blender-Pipeline

Concept/Reference → Modeling/Sculpting → Retopology → UV → Materialien → Rigging → Animation → LOD-/Nanite-Entscheidung
→ Export → Unreal Import → Material Setup → Collision → Physics → Lighting → Validation. Einheitliche Maßstäbe,
Achsen, Pivots und Namenskonventionen.

## 29. Fotorealismus-Regel

„Könnte ein Screenshot davon mit einer realen Fotografie verwechselt werden?" Korrekte Maße, Mikrodetails,
Materialrauheit, Oberflächenunregelmäßigkeiten, Verschmutzung, Alterung, kleine Beschädigungen, Bevels, plausible
Materialien, korrekte Beleuchtung, realistische Verteilung, keine perfekte Symmetrie, keine künstlich sauberen
CGI-Oberflächen.

## 30. Materialien

Physikalisch plausibles PBR: Base Color, Roughness, Normal, Displacement wo sinnvoll, AO korrekt, Subsurface
Scattering für Haut und geeignete Materialien, Micro-Normals, Imperfections, Makro- und Mikrovariation, keine
Tapetenwiederholung.

## 31. Licht

Physikalisch glaubwürdig: natürliche Beleuchtung, realistische Exposition, glaubwürdige Schatten, indirektes Licht,
korrekte Farbtemperaturen. Keine Videospielbeleuchtung, nur damit alles sichtbar ist. Dunkelheit darf dunkel sein.

## 32. Gameplay ohne Symbolüberflutung

Möglichst wenig HUD: keine Flut aus Questmarkern, Pfeilen, XP, Moralpunkten, Minimap-Symbolen, Damage Numbers. Die
Welt vermittelt Informationen.

## 33. Kein klassisches Questsystem als Hauptstruktur

Lebenssituationen ersetzen Quests. Nicht „Besuche deine Mutter – 100 XP", sondern: Die Mutter ruft an. Der Spieler
entscheidet. Vielleicht war es eine der letzten Möglichkeiten – er weiß es vorher nicht.

## 34. Konsequenzen

Manche Folgen zeigen sich nach Sekunden, Tagen, Monaten, Jahren, Jahrzehnten oder einem späteren Leben. Der Spieler
weiß nie vollständig, welche Handlung später wichtig wird.

## 35. Der erste Vertical Slice

Spermium → Befruchtung → Embryo → frühe Organentwicklung → Entwicklung der Sinne → Geburt → frühe Kindheit. Auf der
finalen Architektur, kein Wegwerfprototyp. Nachzuweisen: Grafik-, Charakter-, Animations- und Audioqualität, Life
Simulation, Save-System, Causal Memory, Genetik, Kamera, Übergänge, Performance, Blender→Unreal-Pipeline.

## 36. Vertical-Slice-Signature-Moments

1. Der mikroskopische Beginn · 2. Die Befruchtung · 3. Der erste Herzschlag · 4. **Die erste Wahrnehmung von
Geräuschen** · 5. **Licht durch den Mutterleib** · 6. Die Geburt · 7. Der erste Atemzug · 8. Der erste klare Blick
auf einen Menschen · 9. Die ersten eigenen Schritte.

## 37. C++-Architektur

Module/Subsysteme u. a. für Genesis Core, Life Simulation, Character Biology, Genetics, Ageing, Memory, Causal Graph,
Relationships, Emotions, NPC Simulation, World Simulation, Dream System, Karma, Life Cycle, Save System, Audio State,
Cosmic Progression. Namen dürfen technisch verbessert werden; die Trennung der Verantwortlichkeiten bleibt.

## 38. Data-Driven Design

Keine unnötig harten Werte im C++: Data Assets, Data Tables, Config für Lebensphasen, biologische Werte,
NPC-Archetypen, Emotionen, Eigenschaften, Events, Material- und Soundparameter, Entwicklungsphasen,
Wahrscheinlichkeiten.

## 39. Debugging

Internes GENESIS DEBUG HUD: Alter, Lebensphase, Körperzustand, Emotionen, aktive Erinnerungen, Beziehungen,
NPC-State, Causal Events, Karma, Genetik, Simulation LOD, Performance, Speicherzustand – für Spieler unsichtbar.

## 40. Tests

Automatisierte Tests für Save/Load, Alterung, Lebensphasenwechsel, Causal Graph, NPC-Persistenz, Genetik,
Beziehungen, Game-State-Migration, Wiedergeburt, Langzeitsimulation (Jahrzehnte ohne Echtzeit).

## 41. Entwicklungsregel für Claude Code

Mit Zugriff auf Unreal und Blender (Verbindung/MCP) werden Arbeiten selbst ausgeführt, nicht nur erklärt. Vor großen
irreversiblen Änderungen: Projektzustand prüfen, Architektur untersuchen, Versionskontrolle berücksichtigen.

## 42. Keine blinde Neuerstellung

Vorhandenes zuerst analysieren, dann behalten, verbessern, refactoren oder ersetzen – nie ungeprüft ein zweites
paralleles System bauen.

## 43. Technische Dokumentation

Jede große Architekturentscheidung dokumentieren; je Kernmodul: Zweck, Abhängigkeiten, wichtige Klassen,
Datenfluss, Save-Verhalten, Performance, Erweiterungspunkte.

## 44. Versionierung

Saubere Versionskontrolle, nachvollziehbare Arbeitsschritte, keine unkontrollierten Massenänderungen.

## 45. Realismus vor Show-Effekt

Priorität: Glaubwürdigkeit → Atmosphäre → Emotion → Detail → technische Qualität → Spektakel.

## 46. Organische Welt

Keine sichtbare Perfektion: Abnutzung, Staub, Feuchtigkeit, Alterung, kleine Defekte, ungleiche Abstände,
organische Vegetation, unterschiedliches Wachstum, zufällige Spuren.

## 47. Menschliche Mikrodetails

Atmung, kleine Augenbewegungen, Blinzeln, Mikromimik, Gewichtsverlagerung, Fingerbewegungen, Haltungsschwankungen,
Blickkontakt und -vermeidung, Reaktion auf Geräusche und Menschen. Nie wartende NPC-Statuen.

## 48. Die Welt existiert ohne den Spieler

Die Welt ist nicht für den Spieler gebaut – er wird in sie hineingeboren. Gelegenheiten verschwinden, Menschen
ziehen weg, verändern sich, sterben. Der Spieler kann nicht alles erleben; gerade dadurch bekommt ein Leben
Bedeutung.

## 49. Emotionale Philosophie

Keine fertigen Antworten, sondern Fragen – durch Gameplay, nicht durch Texttafeln: Was macht ein Leben wertvoll?
Welche Entscheidungen verändern andere? Was bleibt von uns? Wie viel bestimmen wir selbst? Was geben wir weiter? Was
bedeuten Erinnerung, Tod, Neubeginn?

## 50. Kein moralisches Urteil durch das Spiel

Auswirkungen werden simuliert, aber nie als „richtig" oder „falsch" bewertet; Folgen können widersprüchlich sein.

## 51. Zufall und Schicksal

Nicht alles entsteht aus Entscheidungen; Zufall gehört dazu, darf aber nie willkürlich unfair wirken. Der Spieler
beeinflusst seinen Umgang mit den Umständen.

## 52. Replayability

Kein Leben verläuft identisch: Genetik, Familie, Geburtsort, Zeit, Wirtschaft, NPCs, Zufall, Entscheidungen,
Beziehungen, Gesundheit, Geschichte, Umwelt, frühere Lebenszyklen.

## 53. Entwicklungspriorität

Bei jeder Funktion prüfen: Verbessert sie das Lebensgefühl? Wirkt sie auf andere Systeme? Erzeugt sie langfristige
Konsequenzen? Ist sie performant skalierbar, modular, glaubwürdig? Verbessert sie Emotion oder Immersion? Ist sie
erweiterbar? Was es nur gibt, weil andere Spiele es haben, muss sich rechtfertigen.

## 54. Arbeitsablauf

1 Bestand prüfen → 2 Abhängigkeiten prüfen → 3 Architektur festlegen → 4 Implementieren → 5 Testen (Funktion, Edge
Cases, Save/Load, Performance) → 6 Visuell prüfen (Hyperrealismus) → 7 Optimieren → 8 Dokumentieren.

## 55. Bei Unsicherheit

Nicht automatisch die einfachste Lösung: Qualität, Skalierbarkeit, Performance, Wartbarkeit, Realismus und
Zukunftsfähigkeit bewerten und die langfristig beste wählen.

## 56. Qualitätsregel

Nie absichtlich wie ein Indie-Prototyp bauen. Die Architektur ist von Anfang an auf ein großes High-End-Spiel
ausgelegt – trotzdem vertikal: ein kleiner Bereich vollständig und hochwertig, bevor das nächste System wächst.

## 57. Aktueller Entwicklungsfokus

Erster Vertical Slice: **Ursprung des Lebens → Geburt → frühe Kindheit.** (Bestandsaufnahme und Roadmap: Docs 01, 07
und 26.)

## 58. Oberstes Ziel

Der Spieler soll nicht einfach eine Figur steuern, sondern das Gefühl haben, ein vollständiges Leben zu erleben:
geboren werden, lernen, lieben, verlieren, entscheiden, Fehler machen, Menschen beeinflussen, altern, zurückblicken,
sterben, verstehen, neu beginnen – und irgendwann erkennen, dass jedes einzelne Leben Teil eines wesentlich größeren
Zusammenhangs war.
