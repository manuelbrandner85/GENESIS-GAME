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

- **Musik fehlt**: Soul Music (GENESIS-009) berechnet Motive, aber niemand spielt sie. Dafür sind
  MetaSounds der richtige Ort – sie lassen sich später im Editor bearbeiten (GENESIS-011/012).
- **Die Welt draußen klingt noch nicht**: Stimmen, Geräte, Raumhall im Kreißsaal fehlen.
- Die Synthese ist **mono**. Für Kopfhörer wäre eine leichte Verbreiterung sinnvoll, für den Mutterleib
  aber nicht – dort gibt es keine Richtung.
- **Gurgeln und Darmgeräusche** der Mutter fehlen; sie gehören zum Mutterleib dazu.
