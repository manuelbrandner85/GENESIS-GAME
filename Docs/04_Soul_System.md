# 04 – Soul-System (Soul Engine)

Modul: `GenesisSoul` · Persistenz-Ebene: **Soul** · Block: GENESIS-002

---

## 1. Grundidee

Eine Seele ist **kein Charakter** und **keine DNA**. Körper, Familie, Kultur und Epoche wechseln mit jeder Wiedergeburt. Der **Soul Seed** bleibt. Er trägt tiefe Muster, die im neuen Leben ohne erkennbare Ursache durchscheinen.

```
FGenesisSoulSeed
├── SoulId, OriginSeed        Identität, alle Ableitungen reproduzierbar
├── Resonances[]              tiefe Muster (Vorlieben, Ängste, Melodien, Träume, Talente, Emotionen)
├── Echoes[]                  Lebensthemen aus dem Karma-Gericht (offen = Narbe, integriert = Fundament)
├── Bonds[]                   Bindungen zu anderen Seelen (Seelen-Begegnungen)
├── Motif                     musikalisches Motiv der Seele (MetaSounds)
├── Incarnations[]            Leben: EntityId, GenomeId, WorldId, Epoche, Kultur, Tod, Jenseitsbereich
└── Detachment                Fähigkeit loszulassen (Weg zu „Das Nichts“)
```

**Trennung von DNA:** `GenesisSoul` hat keine Abhängigkeit zu `GenesisGenetics`. Eine Inkarnation speichert nur die `GenomeId`.

## 2. Resonanzen

- Muster sind Gameplay Tags unter `Genesis.Soul.Pattern.{Affinity|Fear|Melody|RecurringDream|Talent|Emotional|Bond}.*`.
- **Verstärkung** (`ReinforceResonance`) geschieht, wenn ein Muster erlebt wird: `I ← I + Amount · Gain · (1 − I)`. Der Wert sättigt gegen 1.
- **Übertrag beim Tod:** Muster, die in diesem Leben nicht erlebt wurden, verblassen: `I ← I · ResonancePersistence` (Standard 0,6). Unter der Schwelle 0,05 verschwinden sie.
- Eine neue Seele erhält 1–2 schwache **angeborene** Muster aus `InnatePatternPool` (Project Settings → Genesis → Soul).

Spielwirkung (spätere Module lesen `GetPlayerResonance`):
- GenesisMind: unerklärliche Vorlieben und Ängste
- GenesisDream: wiederkehrende Träume
- GenesisAudio: bestimmte Melodien berühren stärker
- GenesisBody/Talent: Lernkurven

## 3. Echos – Ergebnis des Karma-Gerichts

Das Karma-Gericht (GenesisAfterlife) wählt automatisch 1–3 Themen. Grundlage sind Intensität, Wiederholung und Unerledigtes. Die Soul Engine übernimmt sie:

| Fall | Wirkung |
|---|---|
| Thema **abgeschlossen** (`bResolved`) | Echo wird `Integrated` → **Fundament** |
| Thema **offen** | Echo wird `Open` → **Narbe** |
| Thema existiert schon | `ManifestationCount++`, Gewicht sättigend erhöht, Zustand aktualisiert (ein integriertes Thema kann wieder aufbrechen) |
| Thema nicht berührt | Gewicht verblasst: offen × 0,85, integriert × 0,5 |

**Echo-Entscheidungen:** `ComputeEchoPull(SituationThemes)` gibt an, wie stark eine Situation mit den Echos schwingt. Der Living World Director erzeugt damit thematisch verwandte, **nie identische** Situationen. Das ist ein Echo, keine Strafe: Offene Themen ziehen stärker, integrierte schwächer. Die Mechanik bewertet nicht.

## 4. Seelenbindungen & Seelen-Begegnungen

- Intensive Beziehungen eines Lebens (`FGenesisBondExperience`) werden zu Bindungen mit geteilten Themen.
- Bindungen verblassen pro Leben ohne Begegnung (× 0,75).
- `ComputeRecognition(OtherSoulId, ContextThemes)` ergibt die Wiedererkennung von 0 bis 1. Geteilte Themen im aktuellen Kontext verstärken sie.
- **Keine direkte Kennzeichnung im Spiel.** Die Wiedererkennung steuert nur subtile Signale: Déjà-vu, Motiv-Fragmente in der Musik, Traumbilder, ein Gefühl.
- Begleiterseelen (`CompanionSouls`) sind deterministisch aus Seeds erzeugte NPC-Seelen. Sie können in späteren Leben in anderen Rollen, Geschlechtern und Kulturen wiederkehren.

## 5. Musikalisches Motiv

- 4–7 Noten mit Intervallen (Halbtöne) und Dauern (Sechzehntel) in einem Modus (Ionisch … Lokrisch).
- **Identitätskern:** Die ersten beiden Intervalle ändern sich nie. Die Seele bleibt über alle Leben hörbar dieselbe.
- Pro Leben: ein Intervall nach dem Kern verschiebt sich um ±1.
  - **Spannung** folgt den offenen Echos.
  - **Wärme** folgt der Liebe des Lebens.
  - Der Modus folgt dieser Grundfarbe.
- GenesisSoulMusic interpretiert das Motiv (Phasen-Instrumentierung, Erinnerungen, Todeskomposition – siehe `12_Soul_Music.md`). Die Soul Engine liefert nur die Daten.

## 6. Ablauf einer Wiedergeburt

```
Tod → Geistermodus → Lebensrückblick → Karma-Gericht
    → UGenesisSoulSubsystem::CloseIncarnation(FGenesisLifeClosure)
        1. Erlebte Muster verstärken, andere verblassen
        2. Gerichtsthemen → Echos
        3. Bindungen aktualisieren
        4. Motiv entwickeln
        5. Loslassen → Detachment
    → Soul-Ebene synchron speichern
    → GenesisReincarnation wählt neue Epoche, Kultur, Familie, erzeugt neues Genom
    → UGenesisSoulSubsystem::BeginIncarnation(Record mit neuer EntityId/GenomeId)
```

Alle Zufallsentscheidungen eines Lebensabschlusses leiten sich aus `OriginSeed + Lebensindex` ab und sind reproduzierbar.

## 7. Verborgenheit

Resonanzen, Echos und Bindungen sind **nicht** in Blueprints lesbar und erscheinen nie in der Spiel-UI. Sichtbar sind sie nur im Developer HUD (`showdebug Genesis`, Seite `Soul`). Erst im Jenseits bzw. in der Kosmischen Bibliothek zeigt das Spiel sie erzählerisch.

## 8. Tests (`Genesis.Soul.*`)

| Test | Prüft |
|---|---|
| `CreationIsDeterministic` | gleiche Seeds → gleiche Seele; Motivstruktur; angeborene Muster |
| `Reincarnation.Cycle` | Beginn/Abschluss, keine doppelte Inkarnation, SoulId bleibt, Genom wechselt |
| `CarryOver.ResonancesAndEchoes` | Verblassen, offen vs. integriert, Wiederkehr, Identitätskern über 13 Leben |
| `EchoPullAndRecognition` | Echo-Sog offen > integriert, Wiedererkennung kontextabhängig |
| `Persistence` | vollständiger Rundlauf des Seelenarchivs |
