# 02 – Datenarchitektur & Save-/Persistence-Konzept

Status: **verbindlich** · Implementiert in: `GenesisCore` (Interface, Registry, Serialisierung), `GenesisSave` (Dateien)

---

## 1. Drei Datenkategorien

| Kategorie | Beispiele | Form | Gespeichert? |
|---|---|---|---|
| **Authoring-Daten** (statisch, Designer) | Handlungs-Definitionen, Kulturen, Gen-Katalog, Epochen, Berufe | `UPrimaryDataAsset` (`DA_`), `UDataTable` (`DT_`), Gameplay Tags | Nein, Teil des Builds |
| **Laufzeitzustand** (dynamisch, persistent) | Soul Seed, Genome, Kausalgraph, Erinnerungen, Karma, Vertrauen, Gerüchte | `USTRUCT`s mit `FGuid`-IDs, gehalten von Subsystemen | Ja |
| **Abgeleitete Daten** (flüchtig) | Such-Indizes, Adjazenzlisten, Caches, Actors | Nicht-`UPROPERTY` Member / Actors | Nein, wird nach dem Laden neu aufgebaut |

Regeln:
- Persistente Structs enthalten **keine Objektzeiger** auf Laufzeitobjekte, nur IDs. Verweise auf Authoring-Daten sind Asset-Pfade (werden als String serialisiert).
- Jedes `UPROPERTY` eines Zustands-Structs wird gespeichert. Laufzeit-Caches sind deshalb **kein** `UPROPERTY` (oder `Transient`).
- Wertebereiche: Karma −100…+100; alle anderen Simulationsgrößen normiert auf 0…1 bzw. −1…+1; Zeit als int64-Sekunden.

## 2. Identitätsmodell

```
Seele (SoulId) ──1:n──► Inkarnation ──1:1──► Person (EntityId) ──1:1──► Genom (GenomeId)
                                                  │
                                                  ├──n:m──► Ereignis (EventId)  ◄── Kausale Links
                                                  ├──1:n──► Erinnerungsspur (TraceId → EventId)
                                                  └──n:m──► Beziehung / Vertrauen / Ruf
Ort (LocationId) ──1:n──► Ereignis        Gegenstand (ItemId) ──1:n──► Ereignis
```

- **SoulId** bleibt über alle Leben gleich.
- **EntityId** steht für genau ein Leben (eine Person in einer Welt).
- **GenomeId** ist die DNA dieses Körpers. Seele und Genom kennen sich nicht direkt.

## 3. Persistenz-Ebenen

| Ebene | Datei (Slot) | Lebensdauer | Inhalt |
|---|---|---|---|
| **Soul** | `Genesis_<Profil>_Soul` | Alle Leben, alle Welten | Soul Seed, Echos, Seelenbindungen, Motiv, Inkarnationshistorie, Vermächtnis-Briefe, letzte Worte, Meta-Muster (Spieler-Ebene), Startmenü-Reflexion |
| **World** | `Genesis_<Profil>_World` | Generationen einer Welt | Weltuhr, Kausalgraph, Erinnerungen, Profile, Körper und Genome aller simulierten Personen (auch Verstorbener – Ahnen), Vertrauen, Gerüchte, Zeitgeist, Orte, Gegenstände, Familien |
| **Life** | `Genesis_<Profil>_Life_<n>` | Aktuelle Inkarnation | Wahrnehmungs- und Präsentationszustand des Spielers (Emotion, Gedankeninventar, laufende Situationen, Position, Streaming) |

**Tod als atomarer Übergang:** Lebensrückblick → Karma-Gericht → Soul-Ebene aktualisieren (synchron speichern) → Kausalgraph des Lebens verdichten (World) → Life-Slot abschließen. Bricht der Vorgang ab, bleibt der letzte gültige Stand erhalten (Backup).

## 4. Dateiformat

```
UGenesisSaveGame (USaveGame)
├── Header
│   ├── FormatVersion        Version des Containers
│   ├── Scope                Soul | World | Life
│   ├── ProfileId, SlotName
│   ├── SavedAtUtc, BuildVersion
│   └── RecordsChecksum      CRC32 über alle Records
└── Records[]                ein Record pro persistentem System
    ├── SystemId             stabil, z. B. "Genesis.WorldClock"
    ├── SchemaVersion        Version des System-Zustands
    └── Payload              Tagged Property Serialization des Zustands-Structs
```

**Warum Tagged Property Serialization?** Neue Felder erhalten beim Laden alter Stände ihren Default-Wert, entfernte Felder werden übersprungen. Die meisten Datenänderungen brauchen dadurch keine Migration. Echte Umbauten erhöhen `SchemaVersion`, und `LoadState` migriert dann gezielt.

## 5. Ablauf

**Speichern (`SaveScope`):**
1. Registry liefert alle Systeme des Scopes (sortiert nach ID → deterministische Dateien).
2. Jedes System schreibt seinen Record (`SaveState`).
3. Prüfsumme wird berechnet.
4. Der bisherige Stand wird in den Slot `<Slot>_Backup` rotiert.
5. Der neue Stand wird geschrieben.

**Asynchron (`SaveScopeAsync`):** Schritte 1–3 laufen auf dem Game Thread, weil der Zustand dort konsistent ist. Nur die Dateioperation läuft im Thread-Pool. Pro Slot ist höchstens ein Schreibvorgang gleichzeitig aktiv.

**Laden (`LoadScope`):**
1. Primärdatei lesen → Format und Prüfsumme validieren. Bei Beschädigung wird automatisch das Backup verwendet.
2. **Versionsschutz:** Hat ein Record ein neueres Schema als der Build, wird nichts angewendet (`NewerVersion`).
3. Snapshot des aktuellen Zustands aller betroffenen Systeme.
4. Anwenden. Systeme ohne Record (neu hinzugekommen) werden auf den Neuzustand gesetzt. Records ohne System werden mit Warnung ignoriert.
5. Schlägt ein System fehl, werden **alle** Systeme des Scopes auf den Snapshot zurückgerollt (`ApplyFailed`). Halb geladene Welten gibt es nicht.

## 6. Wann gespeichert wird

Ereignisbasiert, **nie pro Frame**:
- Übergänge zwischen Lebensphasen
- Schlaf / Tageswechsel (Autosave, asynchron)
- Nach bedeutsamen Ereignissen (Magnitude-Schwelle im Kausalgraph)
- Vor dem Tod und nach dem Karma-Gericht (synchron)
- Manuell über das Menü

## 7. Große Datenmengen

- **Kausalgraph-Verdichtung:** Nach jedem Leben werden unbedeutende Ereignisse entfernt. Ihre Ursache-Wirkungs-Ketten bleiben erhalten: Gibt es A → x → B und x wird entfernt, entsteht A → B mit multiplizierter Stärke.
- **Erinnerungszerfall** entfernt verblasste Spuren. Hochemotionale Spuren bleiben als ruhende Spuren erhalten.
- **Simulation LOD:** L3-Bevölkerung erzeugt keine Einzeldatensätze.
- **Kompression:** Oodle-Kompression der Payloads ist vorgesehen, sobald Welt-Dateien > 5 MB erreichen. Das Format erlaubt das ohne Bruch über `FormatVersion`.

## 8. Tests

`Genesis.Save.*`:
- Rundlauf über den Engine-Serialisierungspfad (`SaveGameToMemory` / `LoadGameFromMemory`)
- Erkennung manipulierter Daten (Prüfsumme)
- Versionsschutz ohne Teiländerungen
- Rollback bei Ladefehler
- Neuzustand für Systeme ohne Record
- Slot-Benennung und Bereinigung von Profil-IDs

`Genesis.Core.Persistence.*`: Struct-Rundlauf inklusive exakter Fortsetzung eines Zufallsstroms.
