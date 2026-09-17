# 08 – Genetischer Code & Epigenetik

Modul: `GenesisGenetics` · Persistenz-Ebene: **World** · Block: GENESIS-003

---

## 1. Modell

```
FGenesisGenome
├── GenomeId, IndividualSeed
├── MaternalGenomeId, PaternalGenomeId, Generation     → Familienlinien, Ahnenanalyse
├── Traits[]            je Merkmal: Genorte mit Allelpaar (mütterlich / väterlich)
└── EpigeneticMarks[]   je Pfad: Aktivitätsverschiebung −1 … +1
```

- **Merkmale** werden im Gen-Katalog definiert (`UGenesisGeneticsCatalog`, Data Asset `DA_GeneticsCatalog`). Ohne Katalog gelten die eingebauten Standardmerkmale (Größe, Muskelmasse, Stoffwechsel, Pigmentierung, helle Augen, Herz-Kreislauf-Risiko, Angstempfindlichkeit, Musikalität, räumliches Denken, Geruchsschärfe).
- **Additiv (polygen):** viele Genorte, Allele −1…1. Die Summe wird auf eine Normalverteilung normiert → `Mittelwert + StdAbw · z`. Dazu kommen eine feste individuelle Entwicklungsstreuung und die epigenetische Verschiebung.
- **Dominant / Rezessiv:** vereinfachtes Mendel-Modell mit einem Genort.

## 2. Befruchtung

Für jedes Merkmal und jeden Genort gilt:
1. Aus jedem Elternteil wird zufällig eines der beiden Allele gewählt (unabhängige Verteilung).
2. Mutation mit `MutationRate`: Additiv bekommt eine gaußsche Verschiebung, Mendel eine Allel-Umkehr.
3. Fehlt ein Merkmal im Elterngenom (Katalog später erweitert), stammt das Allel aus der Grundbevölkerung.

Der Spermium-Prolog beeinflusst **Basisparameter des Körpers** (GenesisBody), **nicht** die DNA. So bleibt die Genetik biologisch plausibel.

## 3. Epigenetik

| Einfluss | Pfad | Wirkung |
|---|---|---|
| Stress, chronische Belastung | `Genesis.Epigenetic.StressResponse` | erhöht die Ausprägung von Angstempfindlichkeit |
| Ernährung | `Genesis.Epigenetic.Metabolism` | verschiebt Stoffwechsel und Herz-Kreislauf-Risiko |
| Bewegung, Sport | `Genesis.Epigenetic.PhysicalConditioning` | verschiebt das Muskelmasse-Potenzial |

- **Exposition:** `Level ← SättigendeAddition(Level, Dosis · Plastizität)`
- **Rückbildung** ohne Exposition: exponentiell Richtung 0 (Standard 5 % pro Jahr)
- **Transgenerational:** Kind erhält `½ · (Level_Mutter + Level_Vater) · EpigeneticInheritance` (Standard 0,3)

**Keine Wertung:** Ein Pfad wird verstärkt oder gedämpft, nicht verbessert oder verschlechtert. Die Darstellung im Spiel erfolgt über Symptome, Tendenzen und Familiengeschichten, nie über Zahlen.

## 4. Trennung von der Seele

Das Genom kennt keine Seele. Eine Inkarnation im Soul-System referenziert nur die `GenomeId`. Wiedergeburt = neues Genom, gleiche Seele.

## 5. Tests (`Genesis.Genetics.*`)

| Test | Prüft |
|---|---|
| `DNA.Determinism` | gleicher Seed → gleiches Genom und gleiche Ausprägung |
| `DNA.Inheritance` | jedes Allel stammt von den Eltern; rezessive Ausprägung |
| `DNA.PopulationDistribution` | Mittelwert und Streuung der Bevölkerung entsprechen der Definition |
| `Epigenetics` | Aufbau, Wirkung auf Merkmal, abgeschwächte Vererbung, Rückbildung |
| `PoolAndPersistence` | Genom-Pool, Speichern/Laden, deterministische Fortsetzung |
