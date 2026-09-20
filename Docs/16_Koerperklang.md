# 16 – Körperklang: die hörbare Simulation

Der Audio Core (GENESIS-008) rechnet seit Block 8 aus, **wie** ein Mensch gerade hört – Tiefpass im
Mutterleib, Sprung bei der Geburt, Tunnelhören bei Angst. Was fehlte, war jemand, der diese Werte
in Schallwellen übersetzt. Dieser Block liefert ihn.

Plugin: `GenesisSound`.

## Warum Synthese und keine Aufnahmen

Jeder Ton entsteht aus den Werten der Simulation, in dem Moment, in dem sie gelten:

- Ein **Herzschlag** ist zwei gedämpfte Schwingungen – die erste beim Schließen der Segelklappen (42 Hz),
  die zweite beim Schließen der Taschenklappen (58 Hz), etwa ein Drittel des Zyklus später. Bei schnellem
  Puls rücken sie zusammen, genau wie in der Wirklichkeit.
- Der **Blutstrom** ist gefiltertes Rauschen, dessen Lautstärke im Takt des Auswurfs steigt.
- Der **Mutterleib** ist braunes Rauschen: integriertes weißes Rauschen, tief und dauernd.
- Das **Ohrenrauschen bei Sauerstoffmangel** ist kein Effekt, sondern ein Symptom – es steigt mit dem
  Sauerstoffdefizit.

Drei Gründe sprechen dagegen, das aus Klangdateien zu bauen: Der Klang folgt so jederzeit exakt den
Werten der Simulation (Herzfrequenz aus dem Körper, Tiefpass aus der Hörwahrnehmung, Druck aus der Geburt),
er ist bei gleichem Zustand reproduzierbar, und er lässt sich **messen**.

## Gemessen

| Prüfung | Ergebnis |
|---|---|
| Herzschlag zählbar | 120 Schläge in 60 s bei 120/min |
| Mutterleib ist ein Tiefpass | Energie über 2 kHz: Luft ist **10,8-mal** stärker als Mutterleib |
| Die Geburt öffnet die Ohren | Höhen springen um den **Faktor 7,6** innerhalb einer Viertelsekunde |
| Wehe presst | lauter (0,188 gegen 0,141) und zugleich dumpfer |
| Reproduzierbar | gleicher Seed, gleiche Werte → Unterschied 0,00000000 |
| Kein Übersteuern | Spitzenwert 0,689 |

## Hören statt lesen

Ein Bild kann man ansehen, einen Klang muss man hören. Deshalb schreibt GENESIS auf Befehl kurze
Proben aus derselben Synthese, die auch im Spiel läuft:

```
genesis.Sound.RenderWav mutterleib 20
genesis.Sound.RenderWav wehen 30
genesis.Sound.RenderWav geburt 14
genesis.Sound.RenderWav neugeboren 12
```

Die Dateien landen in `Genesis/Saved/Audio/`. Zwei davon liegen als Beleg im Projekt:
`Docs/Media/Audio/GENESIS_Mutterleib.wav` und `GENESIS_Geburt.wav` (dort hört man den Sprung).

## In der Szene

`AGenesisBodySoundActor` trägt die Klangquelle. Sie ist **nicht im Raum geortet**: Ein Herzschlag, den man
im eigenen Kopf hört, wandert nicht, wenn man den Kopf dreht. Der Actor steht in der Zeugungs- und in der
Geburtsszene; er zieht sich seine Werte jeden Tick selbst aus der Simulation.

## Offen (bewusst, nicht versteckt)

- **Musik**: inzwischen hörbar, siehe unten (GENESIS-011).
- **Die Welt draußen klingt noch nicht**: Stimmen, Geräte, Raumhall im Kreißsaal fehlen.
- Die Synthese ist **mono**. Für Kopfhörer wäre eine leichte Verbreiterung sinnvoll, für den Mutterleib
  aber nicht – dort gibt es keine Richtung.
- **Gurgeln und Darmgeräusche** der Mutter fehlen; sie gehören zum Mutterleib dazu.

## Die Seelenmusik klingt (GENESIS-011)

Soul Music (GENESIS-009) rechnet seit Block 9 aus, **welche** Töne ein Mensch hat: seine Intervalle,
seine Tonart, seine Instrumentierung je Lebensphase. Gespielt hat sie bisher niemand.

`FGenesisMusicSynth` ist der Klangkörper dazu. Acht Instrumente, jedes aus dem gebaut, was es ausmacht:

| Instrument | Woraus es besteht |
|---|---|
| Spieldose | Unharmonische Teiltöne (1 : 2,76 : 5,4 : 8,93) – deshalb der glockige Klang; sofortiger Anschlag, langes Ausklingen |
| Klavier | Sechs harmonische Obertöne, Hammergeräusch im Anschlag, Abfall über 1,3 s |
| Gitarre | Gezupfte Saite (Karplus-Strong): ein Rauschimpuls läuft im Kreis und verliert dabei Höhen |
| Streicher | Sieben Obertöne, 280 ms Anschwellen, Vibrato mit 5,2 Hz |
| Holzbläser | Ungerade Obertöne wie bei einer gedackten Pfeife, dazu Atemrauschen |
| Orchester | Acht Obertöne, breit, mit langem Ausklang |
| Chor | Grundton plus zwei Formantbereiche, langsames Vibrato, 1,4 s Ausklang |
| Klangfläche | Leicht verstimmte Teiltöne, 1,2 s Anschwellen, 2,5 s Ausklang |

Dazu ein Nachhall (vier Kammfilter, zwei Allpässe), dessen Größe aus der Phrase kommt: `Space` ist der Raum,
`Presence` die Nähe. Beides berechnet Soul Music aus der Lebensphase.

### Gemessen

| Prüfung | Ergebnis |
|---|---|
| Tonhöhe | MIDI 69 → 440,4 Hz gemessen (0,08 % Abweichung), MIDI 76 → 657,5 Hz (0,26 %) |
| Instrumente unterscheidbar | Höhenanteil Spieldose 0,371 gegen Klangfläche 0,263; erste 120 ms: 0,1433 gegen 0,0067 |
| Lebensphasen klingen anders | Kindheit 8 Noten bei 84 BPM, Höhenanteil 0,495 – Alter 4 Noten bei 58 BPM, 0,175 |
| Ausklang | nach 0,3 s 0,029, nach 1,6 s 0,007 – der Raum verklingt |
| Kein Übersteuern | Spitzenwert 0,237 |
| Reproduzierbar | zweimal gerendert: Unterschied 0,00000000 |

### Im Spiel

`AGenesisMusicActor` spielt das Leitmotiv der hörenden Person. Wechselt die Lebensphase, holt er das
Motiv sofort neu – dieselbe Melodie, andere Instrumente. Zwischen zwei Wiederholungen liegen sechs
Sekunden Stille: Musik, die ohne Atem durchläuft, wird zur Tapete.

```
genesis.Music.RenderWav kindheit 24
genesis.Music.RenderWav alter 24
genesis.Music.RenderWav tod 24
```

Zwei Proben liegen im Projekt: `Docs/Media/Audio/GENESIS_Motiv_kindheit.wav` (Spieldose, hell, 84 BPM)
und `GENESIS_Motiv_alter.wav` (dunkel, 58 BPM) – dasselbe Motiv derselben Seele.

### Ein Fehler, den erst die Messung zeigte

Der erste Test des Ausklangs schlug fehl: Nach 1,6 Sekunden war es **lauter** als nach 0,3. Grund war
nicht der Klang, sondern die Schnittstelle – die Hörprobe wiederholte die Phrase automatisch, wenn die
Probe länger war als das Motiv. In der Messung sah das aus wie ein Nachhall, der anschwillt.
Das Wiederholen ist jetzt ein ausdrücklicher Schalter, keine stille Annahme.
