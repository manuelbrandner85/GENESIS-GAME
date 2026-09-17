# 12 – Soul Music

Plugin `GenesisSoulMusic` (GENESIS-009). Die Seele, die Familie, Beziehungen und Erinnerungen werden als Musik geführt.
Es gibt keine festen Tracks. Ergebnis sind **Phrasen aus Noten** (Tonhöhe, Einsatz, Dauer, Lautstärke, Instrument), die der Music Director (GENESIS-010) schichtet und MetaSounds/Quartz (GENESIS-011) hörbar machen.

> Stand GENESIS-009: Alle musikalischen Entscheidungen werden berechnet, getestet und im Developer HUD angezeigt. **Noch kein hörbarer Klang.**
> Alle Motive sind prozedural erzeugt, es werden keine bestehenden Werke verwendet oder nachgebildet.

## Motiv-Format

`FGenesisSoulMotif` aus GenesisSoul ist das gemeinsame Format für Seelen-, Charakter-, Beziehungs- und Erinnerungsmotive:

- Intervalle in Halbtönen (±1…7), Dauern in Sechzehnteln, Modus (Ionisch … Lokrisch), Spannung, Wärme.
- Der **Identitätskern** (die ersten zwei Intervalle) des Seelenmotivs ändert sich nie (Soul Engine).
- **Einrasten in den Modus mit Konturschutz:** Tonhöhen werden auf Töne des Modus gerundet. Ein Schritt darf dabei weder verschwinden noch die Richtung wechseln; sonst wird der nächste Modus-Ton in Schrittrichtung genommen. Die Kontur ist das Wiedererkennungsmerkmal.

## Leitmotiv einer Person

| Person | Leitmotiv |
|---|---|
| Aktuelle Inkarnation der Spielerseele | **Seelenmotiv** (aus dem Soul Seed, unabhängig von Familie und DNA) |
| Alle anderen | **Charaktermotiv** (Familienmotiv, aus den Motiven der Eltern vererbt oder aus der ID abgeleitet) |

Eine wiedergeborene Seele klingt in jeder Familie nach sich selbst. Die Familie ist trotzdem hörbar: Auch der Spielercharakter besitzt ein Familienmotiv.

## Instrumentierung je Lebensphase

| Phase | Instrumente | Tempo | Dichte | Präsenz |
|---|---|---|---|---|
| Zeugung | Spieluhr | 56 | 0,5 | 0,08 |
| Embryo | Spieluhr | 60 | 0,6 | 0,15 |
| Geburt | Spieluhr + Klavier | 64 | 1,0 | 0,35 |
| Kindheit | Spieluhr + Klavier | 84 | 1,0 | 0,50 |
| Jugend | Klavier + Gitarre | 96 | 1,0 | 0,60 |
| Erwachsen | Streicher + Klavier | 80 | 1,0 | 0,65 |
| Alter | reduziertes Klavier | 58 | 0,6 | 0,45 |
| Tod | Orchester + Streicher + Chor | 52 | 1,0 | 0,90 |
| Geist | Chor + kosmische Fläche | 46 | 0,7 | 0,40 |
| Jenseits | Chor + kosmische Fläche | 44 | 1,0 | 0,60 |
| Wiedergeburt | Spieluhr, kaum hörbar | 56 | 0,5 | 0,05 |
| Kosmisches Bewusstsein | kosmische Fläche + Chor | 40 | 0,8 | 0,50 |
| Schöpfung | Orchester + Chor + kosmische Fläche | 60 | 1,0 | 0,80 |

- **Dichte < 1:** Identitätskern und Schlussnote bleiben. Ausgelassene Noten klingen in der vorherigen weiter, deshalb bleibt die Phrasenlänge gleich (das Alter spielt weniger, aber nicht kürzer).
- Einzelne Phasen lassen sich unter *Project Settings → Genesis → Soul Music → Arrangement Overrides* ersetzen.

## Familienmotive (Vererbung)

Melodie aus zusammenhängenden Abschnitten beider Eltern (ein Kreuzungspunkt), Rhythmus eines Elternteils, Modus eines Elternteils, 15 % Mutation je Intervall.
Gemessen über 300 Familien: Ähnlichkeit Kind–Eltern **0,70**, Kind–Fremde **0,43**, Enkel–Großeltern **0,56**. Die Ähnlichkeit verblasst über Generationen, bleibt aber erkennbar.

Ähnlichkeit (`MotifSimilarity`, 0..1): Kontur und Intervalle 60 %, Rhythmus 30 %, Modus 10 %.

## Beziehungen: verschmelzen und trennen

- `FuseMotifs(Own, Other, Fusion)`: Jede Seite bewegt sich bis zur Mitte. Bei **Fusion 1 klingt das Motiv aus beiden Perspektiven identisch** (zwei Stimmen sind eine geworden). Der gemeinsame Modus ist der des wärmeren Motivs.
- Ab Fusion 0,6 wird die Bindung ein Soundtrack-Moment.
- **Trennung** (Bruch oder Tod): Das eigene Motiv kehrt zurück, eine **Narbe** bleibt (Fusion × 0,35). Eine neue Nähe beginnt wieder, der Datensatz behält die Narbe.

## Erinnerungsfragmente

- Erinnerungen ab Intensität 0,6 erhalten ein Fragment: 2–4 Noten des Leitmotivs. Ab 0,85 beginnt es beim Identitätskern.
- Das Fragment hält den Klang **des Moments** fest. Spätere Entwicklungen des Motivs verändern es nicht.
- Starke Gefühle färben den Modus: Schmerz macht helle Modi äolisch, Zärtlichkeit hellt dunkle Modi auf (Farbe, keine Bewertung).
- **Erinnern** spielt das Fragment in der Instrumentierung seiner Lebensphase: Eine Kindheitserinnerung klingt auch im Alter nach Spieluhr.
- Geringe Genauigkeit (aus der Gedächtnisspur) erzeugt falsche Töne, Lücken, Zeitversatz und mehr Raum. Der erste Ton bleibt als Anker, und die Erinnerung ist nie ganz leer.
- Höchstens 64 Fragmente je Person. Darüber verliert die am wenigsten bedeutsame Erinnerung ihre Musik.

## Lebens-Soundtrack und Todeskomposition

- Soundtrack-Momente: Phasenwechsel, Verschmelzung, Trennung, Erinnerungsfragment, Todeskomposition. Sie werden chronologisch gespeichert, höchstens 256. Bei Platzmangel fällt der unwichtigste Moment, Phasenwechsel und Tod bleiben immer.
- **Todeskomposition** (Abschnitte mit Startzeit in Sekunden, weil jeder Abschnitt ein eigenes Tempo hat):
  1. Seelenmotiv, wie es zu Beginn klang (Spieluhr)
  2. die 5 bedeutsamsten Erinnerungen in der Reihenfolge des Lebens, jeweils im Klang ihrer Phase
  3. die wichtigste Bindung in voller Verschmelzung
  4. Seelenmotiv im Orchester
  5. Übergang: Chor und kosmische Flächen
- `ArchiveLife`: Soundtrack, Schlüsselfragmente und das Seelenmotiv im Moment des Todes gehen an das **Seelen-Archiv** (Soul-Ebene, für Soundtrack-Export und Jenseits). Charaktermotive und Beziehungen bleiben in der Welt, damit Nachkommen sie erben können.

## Persistenz

| System | Ebene | Inhalt |
|---|---|---|
| `Genesis.SoulMusic` | World | Charaktermotive, Beziehungsthemen, Fragmente, laufende Soundtracks |
| `Genesis.SoulMusic.Archive` | Soul | Musik abgeschlossener Leben der Spielerseele |

Das Seelenmotiv selbst speichert weiterhin GenesisSoul.

## Tests

| Test | Prüft |
|---|---|
| `PhaseArrangements` | Instrumente je Phase, reduziertes Alter mit gleicher Länge und erhaltenem Kern, Oktavlage, Modus-Raster, Konturschutz über 200 Motive in allen Modi, Einstellungen |
| `InheritanceAndSimilarity` | Ähnlichkeit, Vererbung statistisch über 300 Familien, Verblassen über Generationen, Determinismus |
| `FusionAndSeparation` | Fusion 0/0,5/1, identische Perspektiven bei voller Verschmelzung, Narbe |
| `MemoryFragmentsAndSoundtrack` | Schwelle, Identitätskern, Farbe, genaues und ungenaues Erinnern, Soundtrack-Verdichtung |
| `DeathComposition` | Aufbau, Auswahl, Chronologie, Instrumente, Dauer, Determinismus, Performance |
| `PersistenceRoundTrip` | Welt- und Archivzustand |

Performance: **0,01 ms** pro Todeskomposition (64 Fragmente). Das Entwickler-Szenario (ganzes Leben) braucht 0,16 ms.

## Entwicklerbefehle

- `genesis.Music.SimulateLife`: spielt ein ganzes Leben musikalisch durch.
- `genesis.Debug.Page SoulMusic`: zeigt das Ergebnis im HUD.
