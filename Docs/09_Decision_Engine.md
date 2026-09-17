# 09 – Decision Engine

Modul: `GenesisDecision` · Block: GENESIS-006 · Abhängig von: LifeSimulation, Memory, Soul

> Die Engine bildet ab, wie **diese Person** entscheiden würde, nicht, was „richtig“ ist.

---

## 1. Entscheidungssituation

```
FGenesisDecisionSituation
├── DeciderId
├── Options[]          je Option: Handlungsdefinition (DA_Action_*), Ziele, Intensität, neutraler Anzeigetext
├── WitnessIds, Groups Wer sieht zu, welche Gruppen sind anwesend
├── SituationThemes    Themen der Situation (Erinnerungs- und Echo-Resonanz)
├── CausedByEventId    Worauf reagiert wird
├── TimePressure 0..1  Wie schnell reagiert werden muss
└── Stakes 0..1        Tragweite (hoch = überlegter)
```

Spieler und NPCs nutzen dieselbe Struktur.

## 2. Abwägung

Jede Option wird aus mehreren Blickwinkeln bewertet (Utility-AI-*Considerations*, jeweils −1 … +1, gewichtet):

| Consideration | Art | Frage |
|---|---|---|
| **Character** (×1,5) | bewusst | Passt die Handlung zu dem, was die Person durch ihr Handeln geworden ist (Karma + Gewohnheit)? |
| **Motive** | bewusst | Eigennutz vs. Nutzen für andere, gewichtet nach Egoismus/Altruismus; Kosten schrecken Egoisten stärker ab |
| **Conviction** | bewusst | Verinnerlichte Ideologien, Glaubenstradition und Gewissheit |
| **Culture** | bewusst | Tabus vs. hochgehaltene Werte, gewichtet mit der Bindung an die Kultur |
| **Social** | bewusst | Erwartungen anwesender Gruppen, Zeitgeist, Sorge um den eigenen Ruf vor Zeugen |
| **Relationship** (×1,2) | bewusst | Vertrauen zu den Betroffenen: Vertrauten tut man eher Gutes, Misstrauen erleichtert Verletzendes |
| **Experience** | bewusst | Wie fühlten sich thematisch ähnliche Erinnerungen an? Skaliert mit ihrer Zugänglichkeit. |
| **Subconscious** | unterbewusst | Verdrängte schmerzhafte Erinnerungen (Vermeidung), Stress (Selbstschutz), **offene Seelen-Echos (Sog)** |

Gewichte lassen sich unter *Project Settings → Genesis → Decision → Consideration Weights* anpassen. Weitere Module ergänzen eigene Considerations, etwa GenesisMind das Gedankeninventar.

## 3. Kopf, Bauch, Zeitdruck

```
Kopf      = gewichteter Mittelwert der bewussten Considerations
Unterbewusst = gewichteter Mittelwert der unterbewussten Considerations
Bauch     = lerp(Unterbewusst, Charakter, 0,4)
Überlegung = clamp(1 − 0,7·Zeitdruck − 0,3·Stress, 0,2, 1)
Gesamt    = Überlegung·Kopf + (1 − Überlegung)·Bauch + 0,2·Unterbewusst
```

- Mit Zeit entscheidet überwiegend der Kopf, das Unterbewusstsein wirkt aber immer mit.
- Unter Zeitdruck und Stress setzt sich der Impuls durch.
- **Zögern** = 1 − Abstand der zwei wahrscheinlichsten Optionen. Das steuert Körpersprache, Kamera und Audio.
- **Innerer Konflikt** entsteht, wenn Kopf und Bauch auf verschiedene Optionen zeigen.

## 4. Wahl

| Wer | Wie |
|---|---|
| **NPC** | Softmax-Wahrscheinlichkeiten. Die Temperatur steigt mit Stress und sinkt bei hoher Tragweite. Deterministisch über den Welt-Zufallsstrom. NPCs sind **keine** starren Automaten: Auch ein Mensch mit Neigung zur Lüge gesteht manchmal. |
| **Spieler** | Wählt frei. Die Spiel-UI zeigt **keine** Bewertung. Läuft die Zeit ab, entscheidet der Impuls. `bFollowedImpulse` hält fest, ob gegen das Bauchgefühl gehandelt wurde. |

NPCs können **Nein sagen**: Ablehnen ist eine normale Option, deren Bewertung von Vertrauen, Motiv und Kosten abhängt.

## 5. Ausführung und Kausalität

`BuildAction` erzeugt aus der gewählten Option eine `FGenesisLifeAction` und reicht sie an die Life Simulation weiter. Die **Erinnerungen, die die Wahl geprägt haben** (Experience, verdrängte Spuren), werden als `InfluenceEventIds` mitgegeben und im Kausalgraph zu Ursachen der neuen Handlung.

Beispiel: Wer als Kind schmerzhaft belogen wurde und später selbst lügt, hat im Graph eine Kante von der alten Erfahrung zur neuen Lüge. Der Lebensrückblick kann das zeigen.

## 6. API

| Aufruf | Zweck |
|---|---|
| `EvaluateSituation(Situation)` | Bewertung ohne Wahl (Spieler-Zögern live anzeigen) |
| `DecideForNpc(Situation)` | bewerten, wählen, ausführen |
| `ResolvePlayerChoice(Situation, Index)` | Spielerwahl ausführen; `-1` = Zeit abgelaufen, der Impuls entscheidet |
| `OnDecisionMade` | Delegate für Director, Cinematics, Audio |

## 7. Performance (gemessen)

| Messung | Ergebnis | Budget |
|---|---|---|
| Entscheidung mit 2 Optionen, 8 Considerations, **5000 Erinnerungen** | **0,071 ms** | < 2 ms |

## 8. Tests (`Genesis.Decision.*`)

| Test | Prüft |
|---|---|
| `CharacterShapesChoice` | gegensätzliche Charaktere, Wahrscheinlichkeiten, statistische NPC-Wahl über 200 Läufe |
| `NpcCanRefuse` | misstrauischer Egoist lehnt ab, vertrauter Altruist hilft |
| `ExperienceAndCausality` | schmerzhafte Erfahrung spricht gegen Lügen und wird im Kausalgraph zur Ursache |
| `SubconsciousAndTimePressure` | verdrängtes Trauma: kaum bewusst, stark unterbewusst; Kopf vs. Bauch; Zeitdruck; Timeout → Impuls |
| `SoulEchoPull` | offenes Seelenthema zieht unterbewusst an, bewusste Abwägung unberührt |
| `Hesitation` | gleichwertige Optionen → Zögern; ungültige Optionen werden nie gewählt |
| `Performance.LongLifeMemory` | Budget mit 5000 Erinnerungen |
