# 00b – Visual Reference, Quality & Cleanup Protocol

*Verbindlicher Bestandteil des Masterplans (Doc 00), vom Game Director vorgegeben am 2026-09-23. Gilt für jede
wichtige Szene und jedes Hero Asset.*

## Das Cover als Visual North Star

Das Cover von *GENESIS – Der Kreislauf des Lebens* (Docs/31) ist das übergeordnete Leitbild: filmische Größe,
emotionales Licht, Menschlichkeit, Lebenszyklus, der Übergang vom biologischen zum kosmischen Leben, organische
Formen, Realismus, epische Landschaften, Alterung, Geburt, Tod, Wiedergeburt, kosmische Dimension, Unendlichkeit.
**Nicht 1:1 kopieren** – jeder Bestandteil wird für Gameplay, Cinematics, Figuren, Umgebungen und Simulationen
eigenständig und glaubwürdig umgesetzt.

**Visuelle DNA:** WARM = Geburt, Leben, Kindheit, Sonne, Nähe, Erinnerung. KALT = Alter, Tod, Kosmos,
Unendlichkeit, Jenseits. Dazwischen kein harter Schnitt, sondern ein fließender Übergang:
Leben → Erinnerung → Vergänglichkeit → Tod → Transformation → Kosmos → Neubeginn.

## Referenzen

1. **Recherche ist Pflicht** vor jeder wichtigen Szene oder grundlegenden Überarbeitung: hochwertige Fotografie,
   Makro- und wissenschaftliche Fotografie, Anatomie, Natur, Architektur, Landschaft, Dokumentarfoto, Film-Stills
   (Licht und Kamera), reale Oberflächen und Materialien, reale Menschen aller Altersstufen, historische Quellen,
   NASA/ESA für kosmische Größen, medizinisch belastbare Quellen für Biologie. Bevorzugt hochauflösend.
2. **Keine blinde Bildsuche:** Jedes Bild wird analysiert – warum funktioniert es? Lichtquelle, -temperatur,
   Kontrast, Schatten, Materialreaktion, Kamera, Brennweite, Perspektive, Komposition, Maßstab, Tiefe, atmosphärische
   Perspektive, Farbe, Oberfläche, Imperfektionen, Haltung, Mimik, Bewegung, Umgebungsdetails – und in technische
   Entscheidungen für Blender und Unreal übersetzt.
3. **Reference Boards** mindestens für: A Human Life (Neugeborene bis sehr alte Menschen: Haut, Haare, Augen,
   Körperform, Hände, Falten, Haltung, Gang, Gesichtsanatomie, Mikromimik) · B Embryo & Biologie (Spermien bis
   Hautentwicklung – wissenschaftliche Glaubwürdigkeit hat Vorrang) · C Natur · D Menschliche Umgebung (bewohnte
   reale Welt) · E Alterung und Vergänglichkeit (auch die Welt altert) · F Kosmos (reale Aufnahmen als Grundlage auch
   für Fantastisches) · G Cinematography (Kamera, Brennweite, Licht, Blocking, Silhouetten, Farbdramaturgie,
   Bewegung).
4. **Reihenfolge:** Referenz → Analyse → Planung → Modellierung. Nicht umgekehrt.
5. **Referenzarchiv** getrennt von den Spiel-Assets (Struktur: Human/Baby, Child, Adult, Elderly, Biology, Nature,
   Architecture, Materials, Cinematography, Cosmos, Historical, Lighting, Animation, Weather). **Quellen
   dokumentieren**, besonders bei Wissenschaft, Medizin, Geschichte, Anatomie, Astronomie.
6. **Keine 1:1-Kopie fremder Werke** – Referenzen dienen Analyse, Realismus, Material, Anatomie, Licht,
   Komposition, Atmosphäre. GENESIS braucht eine eigene Bildsprache. Kein generischer Asset-Look (Marketplace-Demo,
   Asset-Pack, Standard-MetaHuman, Unreal-Template, Blender-Tutorial, generische Fantasy).
7. **Epochen:** Architektur, Kleidung, Werkzeuge, Transport, Landwirtschaft, Straßen, Innenräume, Materialien,
   Vegetation, Gesellschaft, Alltagsgegenstände und Lichtquellen passend zur Zeit – nie ohne Referenzen.

## Bestand, Audit und sauberes Projekt

8. **Erst prüfen, was da ist** (Blender-Dateien, Maps, Blueprints, C++, Materialien, Texturen, Meshes, Skeletal
   Meshes, Animationen, Niagara, Sounds, MetaSounds, Figuren, MetaHumans, Landschaften, PCG, Shader, VFX, Kameras,
   Licht, Post-Process, Plugins, Testassets, alte Prototypen). Nichts ist gut, nur weil es existiert.
9. **Asset-Audit:** A KEEP (entspricht dem Standard) · B IMPROVE (gute Grundlage, überarbeiten statt neu bauen) ·
   C REBUILD (Idee richtig, Umsetzung nicht – neu bauen, dann ersetzen) · D DELETE (unbrauchbar, veraltet, doppelt,
   fehlerhaft, Testmüll, inkompatibel).
10. **Clean Project Policy:** Das Projekt wird mit jeder Phase sauberer, nicht größer. Alte Tests, Temp-Dateien,
    ungenutzte Materialien, doppelte Texturen, unbenutzte Meshes, alte Blueprints, Testmaps, Prototyp-Code, unnötige
    Plugins, Duplikate, alte Exporte, ungültige Redirectors, veraltete Versionen werden entfernt. Keine
    Daten-Friedhöfe (OLD, TEST, COPY, BACKUP, FINALFINAL) – die Versionskontrolle hält die Historie.
11. **Niemals blind löschen:** 1 Abhängigkeiten, 2 Referenzen, 3 Blueprints, 4 Maps, 5 C++, 6 Save-System prüfen,
    7 Versionskontrolle sichern, 8 Ersatz verifizieren, 9 testen, 10 erst dann löschen.
12. **Duplikate** regelmäßig suchen (identische Texturen, fast gleiche Materialien, doppelte Meshes, Mehrfachimporte),
    eine Version wählen, Abhängigkeiten umstellen, Rest entfernen.
13. **Vergleichsrender** Vorher/Nachher bei wichtigen Überarbeitungen – eine neue Version zählt nur, wenn sie
    wirklich besser ist.
14. **Referenz-Matching:** Render und Referenz nebeneinander – warum sieht die Referenz real aus, warum unser Bild
    künstlich (Maßstab, zu perfekte Kanten, Roughness, gleichmäßige Textur, Licht, Mikrodetails, Augen, Haut,
    Haltung, zu saubere Umgebung)?
15. **Reality Check:** Könnte ein Betrachter, der GENESIS nicht kennt, es für echtes Filmmaterial oder hochwertige
    Fotografie halten? Wenn klar nein: Ursache, verbessern, neu rendern, neu vergleichen.

## Qualitätsgate für Hero Assets und wichtige Szenen

REFERENCES · SCALE · GEOMETRY · MATERIALS · LIGHT · DETAIL (Makro, Meso, Mikro) · IMPERFECTIONS · ANIMATION ·
CAMERA · SOUND · PERFORMANCE · STYLE (Visual DNA) – alle geprüft, sonst nicht abgeschlossen.

## Arbeitsablauf für jede große Szene

1 Szene und Assets analysieren → 2 Unbrauchbares identifizieren → 3 Referenzen recherchieren → 4 Reference Board →
5 Abstand zum Ziel definieren → 6 Vorhandenes verbessern → 7 nur Ungeeignetes neu bauen → 8 Altes nach
Abhängigkeitsprüfung entfernen → 9 Blender und Unreal umsetzen → 10 gegen Referenzen vergleichen → 11 Realismus
verbessern → 12 Performance messen → 13 Projekt bereinigen → 14 dokumentieren.

## Oberste visuelle Regel

Wir modellieren nicht danach, wie wir glauben, dass etwas aussieht. Wir untersuchen zuerst, wie es in der Realität
aussieht, und rekonstruieren dann Form, Material, Licht, Alterung, Bewegung, Atmosphäre und Verhalten. Auch das
Fantastische steht auf einem Fundament glaubwürdiger Realität.
