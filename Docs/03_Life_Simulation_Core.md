# 03 – Life Simulation Core

Modul: `GenesisLifeSimulation` · Persistenz-Ebene: **World** · Block: GENESIS-005

> Jede relevante Handlung läuft durch 14 miteinander verbundene Systeme. Keines rechnet isoliert.

---

## 1. Ablauf einer Handlung

```
Spieler / NPC / Director
   │  FGenesisLifeAction (Definition-Asset + Handelnder, Ziele, Zeugen, Gruppen, Ursache, Einflüsse, Intensität)
   ▼
UGenesisLifeSimulationSubsystem::ReportAction  (Zeitpunkt = Weltuhr)
   ▼
UGenesisLifeSimulationEngine::ProcessAction
   ├─ Frame anlegen: Tragweite, tatsächliche Absicht, Karma-Impuls, Beteiligte (Ziele vor Zeugen)
   ├─ WAHRNEHMUNG   Kulturelle Identität → Missverständnis
   ├─ INNERES       Indoktrination → Glaube → Propaganda → Zeitgeist → Gruppenzwang → Egoismus/Altruismus → Doppelmoral
   ├─ SOZIAL        Vertrauen → Ruf & Gerüchte
   └─ INTEGRATION   Karma → Genetik/Epigenetik → Konsequenz-Netzwerk
   ▼
Kausalgraph-Ereignis + subjektive Erinnerungen + geplante Folgen
```

Das **Frame** (`FGenesisLifeSimulationFrame`) ist das gemeinsame Blackboard der 14 Systeme. Jedes System liest die Ergebnisse der vorherigen und schreibt eigene. Beispiele:
- Missverständnis nutzt die kulturelle Distanz aus System 9.
- Vertrauen folgt der **wahrgenommenen** Absicht, nicht der tatsächlichen.
- Karma integriert den Impuls erst, nachdem Gruppenzwang, Opferbereitschaft und Doppelmoral ihn verändert haben.
- Das Konsequenz-Netzwerk speichert alle Systemergebnisse als „Warum“ am Ereignis.

## 2. Die 14 Systeme

| # | System | Stufe | Liest | Schreibt |
|---|---|---|---|---|
| 9 | **Kulturelle Identität** | Wahrnehmung | Tabus/Werte von Handelndem und Beobachtern | Tabubruch, Scham-Stress, kulturelle Distanz, Tabu-Kränkung, Bindung an die Kultur |
| 13 | **Missverständnis** | Wahrnehmung | Sichtbarkeit, Mehrdeutigkeit, Distanz, Stress, Vertrautheit | bemerkt?, fehlgedeutet?, wahrgenommene Absicht und Themen, Erregung |
| 3 | **Indoktrination** | Inneres | verinnerlichte Ideologien | Vertiefung bzw. Erosion, Dissonanz |
| 10 | **Glaube** | Inneres | Gewissheit, Offenheit, Tradition | Verschiebung, Konsistenz, neutraler Haltungswechsel |
| 11 | **Propaganda** | Inneres | Einflüsse, Vertrauen in die Quelle, Weisheit, Stress | Ursachen-Einflüsse, unbemerkte Ideologie-Exposition bei Beobachtern |
| 8 | **Zeitgeist** | Inneres | dominante Werte der Epoche | Ausrichtung, Widerstands-Stress, gesellschaftlicher Druck (jährlich eingearbeitet) |
| 7 | **Gruppenzwang** | Inneres | anwesende Gruppen, Erwartungen, Ablehnungen | Gruppendruck, Mut bei Widerstand, „mitgemacht gegen Überzeugung“ |
| 4 | **Egoismus/Altruismus** | Inneres | Eigen-/Fremdnutzen, Kosten | Spektrum, Opfer verstärken Großzügigkeit |
| 12 | **Doppelmoral-Detektor** | Inneres | öffentlich vertretene Werte, eigene Vergangenheit im Kausalgraph | Heuchelei, Ehrlichkeits-Impuls, wahrgenommene Heuchelei je Beobachter |
| 5 | **Vertrauen** | Sozial | wahrgenommene Absicht, Heuchelei, Tabu, Gruppenhaltung | gerichtetes Vertrauen, Vertrautheit, Verrat (Negativitätsverzerrung) |
| 6 | **Ruf & Gerüchte** | Sozial | ausgedrückte/verletzte Werte, Fehldeutung, Zeitgeist | Ruf je Eigenschaft und Person; Gerüchte, die sich täglich über das Vertrauensnetz verbreiten und verzerren |
| 1 | **Karma** | Integration | modulierter Impuls | 6 Dimensionen mit Sättigung und Gewohnheit |
| 14 | **Genetik & Epigenetik** | Integration | Angstempfindlichkeit aus dem Genom | Stress, Dissonanz, epigenetische Dosis (jährlich ins Genom übertragen) |
| 2 | **Konsequenz-Netzwerk** | Integration | alles | Kausalgraph-Ereignis, direkte/beitragende/automatische Verknüpfungen, Erinnerungen, verzögerte Folgen |

## 3. Karma

- Sechs Dimensionen: Mitgefühl, Ehrlichkeit, Mut, Großzügigkeit, Weisheit, Liebe. Wertebereich jeweils −100 … +100.
- **Sättigung:** Nahe an den Extremen wirken Impulse schwächer. Extreme entstehen nur durch viele Handlungen, ein gefestigter Charakter kippt nicht durch eine Tat.
- **Gewohnheit:** Wiederholte gleichgerichtete Handlungen prägen stärker (Charakterbildung).
- **Absicht statt Wahrnehmung:** Karma folgt der Handlung selbst. Wird Hilfe missverstanden, steigt das Mitgefühl trotzdem, nur Vertrauen und Ruf leiden.
- **Verborgen:** nicht in Blueprints lesbar, nie in der Spiel-UI. Nur im Developer HUD (`genesis.Debug.Page Life`).

## 4. Handlungen modellieren (Designer)

`UGenesisLifeActionDefinition` (Data Asset, `DA_Action_*`) beschreibt, **was eine Handlung ausdrückt**, nicht, was sie wert ist:

| Bereich | Felder |
|---|---|
| Identität | ActionType, Themes, BaseMagnitude, Valence, offener Faden |
| Karma | Karma-Impuls je Dimension (verborgen) |
| Motiv | SelfBenefit, OthersBenefit, CostToSelf |
| Sozial | TrustImpact, Ambiguity, Visibility, ExpressesValues, ViolatesValues, bIsAdvocacy |
| Ideologie/Glaube | IdeologiesAligned/Contradicted, Glaubens-Impulse, PropagatedIdeology, ManipulationStrength |
| Körper | StressLoad, epigenetische Exposition |
| Folgen | verzögerte Konsequenzen: Typ, Themen, Betroffene, Wahrscheinlichkeit, Verzögerung, Tragweite, offener Faden |

Der Kontext kommt zur Laufzeit dazu: Ziele, Zeugen, anwesende Gruppen, direkte Ursache, Einflüsse, Intensität.

## 5. Konsequenzen über Zeit

- **Direkt:** `CausedByEventId` erzeugt die Kante *Direct*.
- **Einflüsse:** Propaganda, Rat und Vorbild werden zu *Contributing*-Kanten.
- **Erlerntes Verhalten:** Thematisch verwandte Ereignisse der letzten 5 Jahre, an denen die Person beteiligt war (auch als Opfer), werden automatisch verknüpft. Wer belogen wurde, lügt eher, und der Graph weiß das.
- **Verzögert:** Geplante Folgen treten Jahre später ein, werden als Ereignis aufgezeichnet, mit der Ursache verknüpft und über `OnConsequenceTriggered` an den Living World Director gemeldet.

## 6. Simulation Level of Detail

| Level | Verarbeitung |
|---|---|
| L1 Full | alle Systeme, jede bemerkte Handlung wird erinnert |
| L2 Reduced | alle Systeme, Erinnerungen nur ab Tragweite 0,35; Gerüchte als Hörensagen |
| L3 Statistical | keine Einzelprofile (Bevölkerung wird in GenesisSociety statistisch simuliert) |

Umschalten: `SetSimulationLevel(EntityId, Level)`. Die plausible Rekonstruktion beim Aufstufen folgt mit GenesisNPC.

## 7. Zeitdynamik

Die Weltuhr ruft `AdvanceTime` pro Simulationsschritt auf:
- **Täglich:** Gerüchte verbreiten sich (Wahrscheinlichkeit aus Reiz, Vertrautheit und Zeit), verzerren sich und verklingen. Stress und Dissonanz klingen ab.
- **Jährlich:** Zeitgeist-Druck wird eingearbeitet, epigenetische Dosis geht ins Genom über.
- **Bei jedem Schritt:** Fällige Konsequenzen werden ausgelöst.
- Zeitsprünge werden in einem Schritt aggregiert berechnet.

## 8. Performance (gemessen)

`Genesis.Life.Performance.Throughput`, Development-Editor-Build, Ryzen 7 5700X:

| Messung | Ergebnis | Budget |
|---|---|---|
| Handlung durch alle 14 Systeme (300 Personen, 2 Zeugen) | **0,015 ms** | < 1 ms |
| Tagesschritt (3000 Ereignisse, 8480 Vertrauenskanten, 2973 aktive Gerüchte) | **0,106 ms** | < 5 ms |

**Bekannte Grenze:** Jede bezeugte, erzählenswerte Handlung erzeugt ein eigenes Gerücht. Bei sehr vielen NPC-Handlungen sollten Gerüchte je Person und Thema zusammengefasst werden. Geplant mit GenesisSociety.

## 9. Tests (`Genesis.Life.*`)

| Test | Prüft |
|---|---|
| `Karma.SaturationAndHabit` | Sättigung, Gewohnheit, Grenzen, gefestigter Charakter |
| `Pipeline.AllFourteenSystems` | 14 unterschiedliche Systeme, Reihenfolge, keine Duplikate |
| `Consequences.LieAcrossTime` | Karma, Egoismus, Verrat, Ruf beim Zeugen, Kausalgraph, Erinnerungen (inkl. L2), verzögerte Folge, erlerntes Verhalten |
| `Misunderstanding.SubjectivePerception` | Fehldeutungen, Karma folgt der Absicht, verzerrte Erinnerung, Determinismus |
| `Social.GroupPressureAndDoubleStandard` | Mut gegen die Gruppe, Heuchelei (Predigen nach Lüge, Lüge gegen erklärten Wert), stärkerer Rufschaden |
| `Social.RumorsSpreadAndDistort` | Entstehung, Verbreitung über Vertrautheit, Ruf beim Dritten, Hörensagen-Erinnerung, Verklingen |
| `Epigenetics.StressShapesGenome` | Stress → Dosis → jährliche Markierung, Erholung |
| `Persistence` | Rundlauf inkl. verborgener Werte und Zufallsstrom |
| `Performance.Throughput` | Durchsatz-Budget |
