# 05 – Causal Memory Graph

Modul: `GenesisMemory` · Persistenz-Ebene: **World** · Block: GENESIS-004

> Kein Quest-Log. Ein Ursache-Wirkungs-Graph, in dem jede Ursache Jahrzehnte oder Generationen später wieder relevant werden kann.

---

## 1. Zwei Ebenen der Wahrheit

| Ebene | Struktur | Bedeutung |
|---|---|---|
| **Objektiv** | `FGenesisCausalGraph` aus `FGenesisCausalEvent` und `FGenesisCausalLink` | Was tatsächlich geschah und was es auslöste. Im Jenseits und in der Kosmischen Bibliothek sichtbar. |
| **Subjektiv** | `FGenesisMemoryStore` pro Person aus `FGenesisMemoryTrace` | Wie sich eine Person erinnert: intensiv oder blass, genau oder verzerrt, verdrängt, umgefärbt. |

Zwei Personen können dasselbe Ereignis unterschiedlich speichern. Die Wahrheit bleibt im Graphen erhalten. `CompareWithTruth` misst die Abweichung.

## 2. Ereignisse und Kanten

**Ereignis:** Typ (`Genesis.Action.*`, `Genesis.Consequence.*`), Themen (`Genesis.Theme.*`), ausgedrückte und verletzte Werte, Handelnder, Ziele, Zeugen, Ort, Seele des Handelnden, Tragweite (0..1), Valenz (−1..1), offener Faden ja/nein, Kennwerte (`Facts`).

**Kantentypen:** `Direct` · `Contributing` · `Enabling` · `Inhibiting` · `Inherited` (Generationen) · `SoulEcho` (Inkarnationen)

**Invarianten:**
- **Azyklisch:** Jedes Ereignis erhält eine streng steigende `Sequence`. Kanten dürfen nur von kleinerer zu größerer Sequence zeigen. Kausalität kann nicht rückwärts laufen.
- **Belege kombinieren:** Wird dieselbe Beziehung erneut festgestellt, gilt `s = 1 − (1 − a)(1 − b)`. Es entstehen keine doppelten Kanten.

## 3. Abfragen

| Abfrage | Zweck |
|---|---|
| `TraceConsequences(Root, Tiefe, MinStärke)` | Butterfly Engine, Kosmische Bibliothek: Was hat diese Handlung ausgelöst? |
| `TraceCauses(Root, …)` | Warum ist das passiert? (Lebensrückblick, Karma-Gericht) |
| `FindEvents(Query)` | Beteiligte, Typ, Themen, verletzte Werte, offene Fäden, Tragweite, Zeitraum |
| `CountThemeRecurrence(Actor, Theme)` | Karma-Gericht: Wiederholung |

Die Traversierung folgt dem **stärksten Pfad** (Produkt der Kantenstärken) in einer Dijkstra-artigen Max-Produkt-Suche.

## 4. Beispiel: eine Lüge über Generationen

```
Kind lügt ──0.9──► Eltern verlieren Vertrauen ──0.7──► Kind entwickelt Strategie
   ──0.6──► Bindungsangst ──0.6──► spätere Partnerschaft ──0.8 (Inherited)──► Kind übernimmt Verhalten
   ──0.9──► Jahrzehnte später Familienkonflikt ──(SoulEcho)──► Thema im nächsten Leben
```

Genau diese Kette prüft der Test `Genesis.Memory.Graph.CausalChainAcrossGenerations`.

## 5. Verdichtung – begrenzter Speicher ohne Vergessen

`Compact(MaxMagnitude, MinBridgedStrength)` entfernt Ereignisse, die
- abgeschlossen sind **und**
- eine Tragweite ≤ `MaxMagnitude` haben **und**
- von keiner lebenden Erinnerungsspur referenziert werden.

**Die Kausalketten bleiben erhalten:** Aus A → x → B wird A → B mit Stärke `s(A,x) · s(x,B)`. Mehrere entfernte Ereignisse hintereinander werden in Sequence-Reihenfolge überbrückt. Die Pfadstärke der Gesamtkette ändert sich dadurch nicht.

Zeitpunkt: beim Tod bzw. Generationswechsel. So bleibt die Welt-Datei trotz Jahrhunderten Spielzeit begrenzt.

## 6. Subjektive Erinnerungen

| Eigenschaft | Entstehung | Veränderung |
|---|---|---|
| **Intensität** | Tragweite, Erregung, Aufmerksamkeit, Perspektive; mittlerer Stress verstärkt (Yerkes-Dodson) | exponentieller Zerfall; Halbwertszeit = Basis + Emotion² · Bonus + log₂(1 + Abrufe) · Bonus |
| **Genauigkeit** | Perspektive (Handelnder 1,0 … Familienerzählung 0,45), Aufmerksamkeit, hoher Stress senkt | Drift pro Jahr; jeder Abruf rekonstruiert und verfälscht minimal |
| **Gefühl (Valenz)** | Ereignisvalenz + Stimmung | Abruf färbt Richtung aktueller Stimmung |
| **Wahrnehmung** | Themen und Handelnder, bei Ungenauigkeit verzerrt, oder vorgegeben durch das Missverständnis-System | – |
| **Verdrängung** | – | `AdjustRepression`: + verdrängen, − aufarbeiten (Therapie, Meditation, Krise, Wahrheit, Jenseits) |
| **Vergessen** | – | unter der Schwelle entfernt; hoch emotionale Spuren **ruhen** nur und können zurückkehren |

## 7. Reize wecken Erinnerungen

`FindByCues(SensoryCues, Themes, …)`:
- Sinnesreize (`Genesis.Sense.*`, Inhalte darunter z. B. `Genesis.Sense.Smell.Rain`) und Themen werten Spuren auf.
- **Geruch zählt doppelt** (Proust-Effekt, `SmellCueWeight`).
- Verdrängte Spuren sind schwer zugänglich.
- `bFlashback`: Reiz plus hohe Aktivierung plus hoher Score. GenesisCinematics kann daraus einen First-Person-Flashback auslösen.

Beispiel: Ein Parfüm in Jahr 60 weckt die Erinnerung an die Mutter.

## 8. NPC-Erinnerungen

NPCs speichern nicht nur `Trust = 65`, sondern konkrete Spuren mit einem **Erinnerungssatz** (`Narrative`), z. B. „Er blieb bei mir, als meine Mutter starb.“ Beziehungs- und Dialogsysteme lesen die Spuren **aus Sicht des NPCs**. Die Wahrnehmung darf dabei falsch sein. Das ist auch die Grundlage des World Truth Systems: Ein NPC weiß nur, was in seinem Gedächtnis steht.

## 9. Simulation LOD

- **L1:** jede relevante Wahrnehmung wird eingeprägt.
- **L2:** nur Ereignisse ab einer Tragweiten-Schwelle (gesteuert von GenesisLifeSimulation).
- **L3:** keine Einzelgedächtnisse; Ereignisse auf Bevölkerungsebene nur aggregiert.
- Zerfall läuft in Tagesintervallen über die Weltuhr, bei Zeitsprüngen in einem Schritt.

## 10. Tests (`Genesis.Memory.*`)

| Test | Prüft |
|---|---|
| `Graph.CausalChainAcrossGenerations` | 6-stufige Kette, Pfadstärke, Rückverfolgung, Tiefenlimit, Azyklizität, Verdichtung mit Kettenerhalt |
| `Graph.QueriesAndEvidence` | Beteiligung, offene Fäden, Themen-Wiederholung, Beleg-Kombination |
| `Traces.SubjectiveAndFallible` | unterschiedliche Erinnerung desselben Ereignisses, Verzerrung, Vergessen vs. Ruhen, Geruchs-Trigger, Verdrängung |
| `Persistence` | Rundlauf, Indizes nach Laden, Sequenzen setzen korrekt fort |
