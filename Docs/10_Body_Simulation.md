# 10 – Body Simulation

Modul: `GenesisBody` · Persistenz-Ebene: **World** · Block: GENESIS-007

> Der Spieler sieht keine medizinische Tabelle. Er erlebt Symptome.

---

## 1. Modell

```
FGenesisBodyState
├── Zeitachse     Befruchtung, Geburt (Schwangerschaftswoche), Tod
├── Anlagen       aus dem Genom (Größe, Muskelanlage, Herz-Risiko, Stoffwechsel, Geruch) + Vitalität aus dem Spermium-Prolog
├── Organe[9]     Herz, Lunge, Leber, Gehirn, Immunsystem, Muskulatur, Skelett, Nervensystem, Stoffwechsel
│                 je: Entwicklung · Kapazität · Schaden (heilbar) · Verschleiß (dauerhaft) · Entwicklungsqualität
├── Sinne[5]      Sehen, Hören, Riechen, Schmecken, Tasten: Entwicklung + Schärfe
├── Hormone       Cortisol, Adrenalin, Oxytocin, Melatonin, Wachstum, Sexualhormone
├── Vitalwerte    Puls, Atmung, Sauerstoff, Temperatur, Energie, Schlafdruck, Hunger, Flüssigkeit
├── Lebensstil    Fitness, Trainingslast, Ernährung, Schlafschuld, chronischer Stress, biologisches Alter
└── Zustände      Verletzungen, Infektionen, chronische Erkrankungen; Narben
```

Die Werte sind physiologisch **plausibel angenähert**, also Größenordnungen und Verläufe, bewusst keine medizinische Simulation.

## 2. Vor der Geburt

| Organ | Ausbildung (Wochen seit Befruchtung) | Sinn | Ausbildung |
|---|---|---|---|
| Herz | 3–8 (Herzschlag ab ~5) | Tasten | 7–18 |
| Nervensystem | 3–25 | Schmecken | 10–18 |
| Gehirn | 3–38 (reift danach weiter) | Hören | 15–25 |
| Leber | 4–30 | Riechen | 18–28 |
| Stoffwechsel | 4–32 | Sehen | 20–32 |
| Lunge | 4–36 (reift zuletzt) | | |
| Muskulatur, Immunsystem, Skelett | 5/6–36/38 | | |

- **Entwicklungsqualität** (0..1) begrenzt die erreichbare Ausbildung eines Organs. Hier setzt das Embryo-Minispiel (GENESIS-012) an.
- Im Mutterleib sind die Sinne gedämpft: Hören 30 % → Symptom *gedämpftes Hören*.
- Fetaler Herzschlag 110–160, sobald das Herz ausgebildet ist.

## 3. Geburt

`Birth()`: erster Atemzug, Adrenalin- und Cortisolschub, Kälte.
- **Lungenreife < 85 %** (Frühgeburt): Zustand *Atemnot*. Das Kind atmet schneller und hat weniger Sauerstoff. Die Lunge reift nach, die Atemnot klingt je nach Immunsystem ab.
- **Neugeborene sehen sehr unscharf:** Sehschärfe 0,05, volle Schärfe mit etwa 3 Jahren. Hören ist bei 70 % und reift in 6 Monaten.

## 4. Stündliche Physiologie (Simulation Level 1)

- **Tagesrhythmus:** Melatonin nachts, Cortisol-Spitze am Morgen.
- **Stress** aus der Life Simulation hebt Cortisol.
- **Puls/Atmung** folgen Alter (Neugeborene ~140/45, Erwachsene ~70/14), Aktivität, Fitness, Adrenalin, Fieber und Lungenfunktion.
- **Schlafdruck** steigt wach mit 1/16 pro Stunde und sinkt im Schlaf mit 1/8.
- **Hunger und Durst** wachsen, im Schlaf langsamer. Die Energie folgt Schlafdruck, Hunger, Flüssigkeit und Anstrengung.

## 5. Langfristig (Level 1 und 2, Monatsscheiben bei Zeitsprüngen)

- **Kapazität** je Organ steigt bis ~20–30 Jahre und sinkt danach organ-spezifisch, ab 60 beschleunigt, bezogen auf das **biologische** Alter.
- **Biologische Alterung** = Zeit × (1 + 0,5·chronischer Stress + 0,3·Schlafschuld − 0,2·Fitness [Erwachsene] − Ernährung).
- **Genetik:** Herz-Risiko beschleunigt den Rückgang des Herzens, die Muskelanlage verschiebt die Muskelkapazität, die Erwachsenengröße kommt aus dem Genom.
- **Verschleiß:** Jahrelanger Stress verschleißt das Herz dauerhaft. Akute Schäden heilen über das Immunsystem.
- **Wachstum:** 1 Jahr ≈ 41 %, 5 Jahre ≈ 62 %, 10 Jahre ≈ 79 %, 18 Jahre = 100 % der genetischen Größe. Im Alter wird der Körper etwas kleiner.
- **Sinne im Alter:** Nahsicht ab ~45, Hören ab ~50, Riechen/Schmecken ab ~60, Tasten ab ~65.
- **Lebensende:** Die Vitalfunktionen versagen, wenn Herz, Gehirn oder Lunge unter die Schwelle fallen. Es gibt **kein festes Todesdatum**, das Ende entsteht aus Alterung, Genetik, Lebensstil, Krankheit und Verletzung.

## 6. Symptome

`DeriveSymptoms()` liefert `Genesis.Symptom.*` mit Intensität 0..1. Diese Werte lesen Postprocess, Animation, Kamera und Audio:

| Symptom | entsteht aus |
|---|---|
| BlurredVision / MuffledHearing | Sinnesschärfe (Neugeborene, Mutterleib, Alter) |
| Fatigue | Schlafdruck, Energie |
| HeartPounding | Puls über Ruhepuls, Adrenalin |
| Breathlessness | Atemfrequenz, Sauerstoff |
| Pain | schmerzhafte Zustände |
| Fever / Nausea | Temperatur, Infektion |
| Hunger / Thirst | Bedürfnisse |
| Tremor | Adrenalin, Kälte |
| Dizziness | Sauerstoff, Flüssigkeit |
| Stiffness | Muskel-/Skelettkapazität im Alter |

## 7. API

| Aufruf | Zweck |
|---|---|
| `CreateBodyAtConception(EntityId, GenomeId, Vitality, Level)` | Befruchtung |
| `Birth(EntityId)` | Geburt |
| `SetActivity` / `Eat` / `ApplyInjury` / `ApplyAcuteStressor` / `ApplyBonding` | Gameplay-Ereignisse |
| `GetSymptoms` / `GetSenseAcuity` / `GetDevelopmentStage` | Darstellung |
| `OnVitalFailure` | Übergang zu GenesisDeath |

## 8. Messwerte (Tests)

| Messung | Ergebnis |
|---|---|
| Mit 60 Jahren: biologisches Alter bei Dauerstress / aktivem Leben | **67,3 / 53,4** |
| Herzgesundheit mit 60 (Stress / aktiv) | **0,39 / 0,93** |
| Natürliches Lebensende bei leichtem Dauerstress | **91,1 Jahre** |
| Stundenschritt 50 Level-1-Körper | **0,005 ms** |
| Tagesschritt 1000 Körper | **0,28 ms** |

## 9. Tests (`Genesis.Body.*`)

`PrenatalDevelopment` · `BirthAndNewbornPerception` · `VitalsSleepAndSymptoms` · `LifestyleShapesAging` · `InjuryIllnessAndScars` · `VitalFailureAndNaturalEnd` · `GeneticsAndPersistence` · `Performance.Population`
