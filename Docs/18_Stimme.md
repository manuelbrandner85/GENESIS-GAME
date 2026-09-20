# 18 – Die Stimme

Eine Stimme ist in GENESIS kein Klangeffekt, sondern ein Körper.
Die **Grundfrequenz** kommt von Länge und Spannung der Stimmlippen, die **Klangfarbe** von der Länge des
Ansatzrohrs zwischen Kehlkopf und Lippen. Beides wächst mit dem Menschen – deshalb klingt ein Kind nicht
wie ein leiser Erwachsener, sondern anders: höher **und** heller.

Nichts davon wird abgespielt. Alles wird erzeugt, aus denselben Werten, die auch der Rest der Simulation
benutzt: Alter, Geschlecht, Körpergröße, Gesundheit von Kehlkopf und Lunge, Erschöpfung, Erregung.

Plugin: `GenesisVoice` (Logik und Synthese ohne Welt, Subsystem mit Stimmprofilen, Actor je sprechender Person).

## Die Stimme über ein Leben

**Gemessen** (Voreinstellung der Stellschrauben, ohne individuelle Streuung):

| Alter | Grundfrequenz | Klangfarbe |
|---|---|---|
| Neugeborenes | 450 Hz | Ansatzrohr 7 cm → Formanten 2,5-fach |
| Kind, 6 Jahre | 265 Hz (Junge wie Mädchen) | Ansatzrohr 10,5 cm → 1,6-fach |
| kurz vor dem Stimmwechsel, 11 Jahre | 242 Hz | |
| erwachsener Mann | 115 Hz | Ansatzrohr 17,5 cm → 1,0 |
| erwachsene Frau | 200 Hz | Ansatzrohr 14,5 cm → 1,2 |
| Mann, 80 Jahre | 135 Hz | die Stimmlippen werden dünner: die Stimme **steigt** |
| Frau, 80 Jahre | 175 Hz | nach den Wechseljahren **sinkt** sie |

Zwei Dinge daran sind kein Zufall, sondern der Punkt:

- **Vor dem Stimmwechsel klingen Jungen und Mädchen gleich.** Der Unterschied entsteht erst,
  wenn der Kehlkopf des Jungen wächst und tiefer rückt – gemessen fällt die Stimme dabei um
  etwa eine Oktave, die des Mädchens nur um wenige Halbtöne.
- **Im Alter nähern sich die Stimmen wieder an.** Aus 85 Hz Abstand werden 40 Hz.

## Wie der Klang entsteht

Quelle-Filter-Modell, wie die Phonetik den Menschen beschreibt:

1. **Quelle**: Die Stimmlippen lassen Luftstöße durch (Rosenberg-Form: weicher Anstieg, steiler Schluss –
   der steile Schluss ist der Grund, warum eine Stimme Obertöne hat).
2. **Unregelmäßigkeit**: Jede Periode ist etwas anders lang (Jitter) und etwas anders laut (Shimmer).
   Ohne sie klingt jede Synthese sofort nach Maschine. Bei Heiserkeit steigt beides, und die Stimmritze
   schließt nicht mehr vollständig – dazu kommt also Rauschen.
3. **Filter**: Vier Resonanzen (Formanten) bilden das Ansatzrohr ab. Ihre Lage entscheidet, welcher Vokal
   zu hören ist; ihre Skalierung ist allein die Länge dieses Rohrs.
4. **Nase**: eine tiefe, schmale Resonanz. Beim Summen ist sie fast alles, was man hört; bei Schnupfen näselt es.
5. **Abstrahlung an den Lippen** wirkt wie eine Ableitung – daher der helle Klang.

**Gemessen:** Profil 112 Hz → synthetisiert 112 Hz (Abweichung 0,0 %); Frau 196 Hz → 196 Hz (0,2 %).
Die Kinderstimme hat oberhalb von 1,5 kHz einen Höhenanteil von 0,445 gegen 0,277 beim Mann –
der Unterschied ist also nicht nur behauptet, sondern hörbar.

## Was ein Mensch von sich geben kann

Worte gibt es hier noch nicht – die kommen mit dem Dialogsystem. Was es gibt, ist alles, was ein Mensch
außerhalb der Sprache äußert, und zwar in der Reihenfolge, in der ein Kind es lernt:

| Laut | Ab |
|---|---|
| Schreien, Quengeln | sofort |
| Gurren | 2 Monate |
| Lachen | 4 Monate |
| Lallen (Silbenketten) | 6 Monate |
| Seufzen, Summen | 1 Jahr |
| Sprechen (Silben mit Sprechmelodie), Rufen, Beruhigen | 1½ Jahre |

Ein Neugeborenes kann also nicht lachen. Das ist keine Spielregel, sondern Entwicklung –
und wer es trotzdem versucht, bekommt keinen Laut, sondern eine Notiz im Log.

**Der Schrei** ist eine Kette aus Ausatmen und hörbarem Einatmen. Genau das unterscheidet ihn von jedem
anderen lauten Geräusch. Gemessen: 503 Hz, Effektivwert 0,263, 25 von 79 Fenstern sind Atempausen –
ruhiges Sprechen liegt bei 0,108.

## Die Stimme der Mutter, gehört aus dem Mutterleib

Das Kind kennt diese Stimme, bevor es sie zum ersten Mal richtig hört: Durch Bauchdecke und Fruchtwasser
fehlen die Höhen, die Sprachmelodie bleibt.

**Gemessen:** Energie über 2 kHz an Luft 0,0295, im Mutterleib 0,0028 – ein Faktor von 10,7.
Die Tonhöhe bleibt dabei bei 203 Hz. Genau deshalb erkennt ein Neugeborenes seine Mutter an der Stimme:
Es kennt ihre Melodie, nicht ihre Worte.

Dasselbe Filter läuft im Spiel: Die Stimmkomponente holt sich Grenzfrequenz und Lautheit aus der
Hörwahrnehmung des Audio Core. Vor der Geburt klingt die Mutter dumpf, nach der Geburt klar –
ohne dass jemand einen Regler umlegt.

## In der Szene

Zwei Actors in `L_GEN_Birth` geben der ersten Stunde ihre Stimmen:

- **ChildVoice** – schreit, wenn es dem Kind schlecht geht (`Calm < 0,35`), quengelt, wenn es besser wird,
  und schweigt, wenn es zufrieden ist. Die Stärke des Schreis ist `1 − Ruhe`.
- **MotherVoice** – spricht, wenn mit dem Kind gesprochen wird: beruhigend und summend, solange es schreit,
  sprechend und summend, wenn es ruhig ist.

Keiner der beiden erfindet etwas. Ein stilles Kind ist kein fehlender Ton, sondern ein zufriedenes Kind.

## Biologisches Geschlecht

Für den Stimmwechsel braucht es das Geschlecht – bis hierher gab es das im Genom nicht.
Es ist jetzt da, und zwar so, wie es entsteht: Die Eizelle trägt immer ein X, das Spermium ein X oder ein Y.
Das Geschlecht des Kindes entscheidet sich also im väterlichen Gameten, hälftig.

Das ist eine körperliche Angabe. Über die Person, die daraus wird, sagt sie nichts –
Geschlechtsidentität ist ein eigenes Thema und wird hier bewusst nicht mitmodelliert.

## Bedienung (Entwickler)

```
genesis.Voice.RenderWav neugeboren 8    # Hörprobe schreiben (Saved/Audio)
genesis.Voice.RenderWav mutter 12       # Proben: neugeboren saeugling kind frau mann alt mutter heiser
genesis.Voice.Say beruhigen 0.6         # Die Mutter beruhigt
genesis.Voice.Cry 0.9                   # Das Kind schreit
genesis.Debug.Page Voice                # HUD-Seite „Stimmen"
```

![Stimmen in der Geburtsszene](Media/GENESIS-013_Voices.png)

Hörproben: `Docs/Media/Audio/GENESIS_Stimme_Neugeboren.wav`, `…_Kind.wav`, `…_Mutter.wav`, `…_Alt.wav`.

## Offen

- **Keine Sprache.** „Sprechen" sind Silben mit richtiger Sprechmelodie, aber ohne Worte –
  bis es Dialoge gibt (GENESIS-014), ist das ein ehrlicher Platzhalter.
- Die Mutter ist noch keine Person der Simulation; ihr Stimmprofil ist angenommen (30 Jahre, erschöpft).
- Mono, keine Ortung im Raum, kein Raumhall.
- Kein Weinen mit Tränen, kein Husten, kein Niesen, kein Gähnen.
- Der Klang im Spiel ist bisher nur über Hörproben geprüft; eine Messung des tatsächlichen Mixes im Spiel fehlt.
