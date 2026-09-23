# 20 – Der Durchlauf

Bis hierher gab es die Teile: die Befruchtung, die erste Woche, die Geburt, die erste Stunde.
Jeder lief für sich, gestartet über Konsolenbefehle. Was fehlte, war das, worum es geht –
**ein Leben am Stück**: von der Verschmelzung zweier Zellen bis zu dem Moment, in dem ein Mensch
satt und warm einschläft.

Plugin: `GenesisSlice` (Regeln ohne Welt, Regie als Subsystem, das die Karten wechselt).

## Was die Regie tut – und was nicht

Sie **erfindet nichts**. Sie schaut zu, was die Systeme melden, und entscheidet nur, wann gewartet,
wann gesprungen und wann der Ort gewechselt wird. Ob der Keim sich einnistet, ob die Geburt gut verläuft,
ob das Kind zur Ruhe kommt – das entscheiden die Systeme, nicht die Regie.

| Phase | Was geschieht | Wie die Zeit läuft |
|---|---|---|
| Befruchtung | Der Schwarm im Eileiter, eine Eizelle, ein Treffer | Echtzeit |
| Erste Woche | Furchung, Morula, Blastozyste, Schlüpfen, Einnistung | 6 Stunden je Schritt |
| Schwangerschaft | Aus Sicht des Kindes im Mutterleib (`L_GEN_Mutterleib`, Doc 34 Teil 1b): sieben Momente von SSW 19 bis 37 | Zeitraffer zwischen den Momenten (16 s), im Moment 8-mal Echtzeit; zusammen gut 6 Minuten |
| Geburt | Wehen, Enge, Drehung, Licht, erster Atemzug | 90 Simulationsminuten je Sekunde |
| Erste Stunde | Wärme, eine vertraute Stimme, das erste Anlegen | 2 Simulationsminuten je Sekunde |

Die Weltuhr wird dabei **je Phase** gestellt: In den Szenen, die man wirklich erlebt, läuft sie in Echtzeit.
Sonst liefen zwei Zeitraffer übereinander – und genau das ist beim ersten Durchlauf passiert:
Die erste Stunde war nach sieben Sekunden vorbei, ohne dass irgendetwas davon zu sehen war.

## Gemessen: ein Leben in 86 Sekunden

Ein vollständiger Durchlauf (Seed 4711, ohne Eingriff von außen):

| Zeitpunkt | Ereignis |
|---|---|
| 0,0 s | Durchlauf beginnt im Eileiter |
| 26,3 s | Befruchtung → erste Woche |
| 34,2 s | Einnistung → Schwangerschaft |
| 47,7 s | Termin (39,1 Wochen) → Geburt, Ortswechsel nach `L_GEN_Birth` |
| 53,9 s | geboren → erste Stunde |
| 54,9 s | das Kind kommt auf die Haut der Mutter |
| 70 s | erstes Anlegen, erste Erinnerung abgelegt (Bindung 0,54) |
| 86,0 s | **erster Schlaf** – 64 Minuten alt, 37,0 °C, Ruhe 1,00, Bindung 0,83 |

Keine Fehler und keine Warnungen aus Genesis-Modulen im Log.

## Die Versorgung nach der Geburt

Das Kind kommt zwei Minuten nach der Geburt auf die Haut der Mutter, und es wird mit ihm gesprochen.
Das ist **keine Entscheidung des Spielers**, sondern die übliche Versorgung in einem Kreißsaal –
solange es keine Eingabe gibt, handelt die Welt. Mit `genesis.Newborn.SkinToSkin 0` lässt sich das
Gegenteil erzwingen; dann kühlt das Kind aus, und der Durchlauf endet ohne den guten Schluss.

Denn der gute Schluss wird nicht geschenkt: Der Durchlauf endet mit **„Das Kind schläft"** nur dann,
wenn es tatsächlich eingeschlafen ist. Ist die Stunde vorbei und das Kind nicht zur Ruhe gekommen,
heißt das Ende **„Die erste Stunde ist vorbei – das Kind ist nicht zur Ruhe gekommen"**.
Ein Keim, der sich nicht weiterentwickelt, und ein Kind, das die Geburt nicht überlebt, beenden den
Durchlauf ebenfalls – mit dem jeweiligen Grund.

## Performance (gemessen, 1137×600, RTX-Klasse-GPU, Ryzen 7 5700X)

| Szene | FPS | Frame | Game | GPU | Draws | Dreiecke |
|---|---|---|---|---|---|---|
| Eileiter (Schwarm aktiv) | 178 | 5,62 ms | 2,73 ms | 3,83 ms | 72 | 5,33 M |
| Geburt (Wehen laufen) | 193 | 5,18 ms | 2,01 ms | 3,19 ms | 75 | 12,6 K |

Der Spiel-Thread liegt in beiden Szenen bei 2–3 ms – die Simulation ist also nicht der Engpass.
Eine Engine-Warnung bleibt offen: „Ray Tracing Geometry – always resident memory exceeds 20 % of the budget"
im Eileiter (83 von 400 MiB); das ist eine Speicherwarnung der Engine, keine Bremse.

## Bedienung (Entwickler)

```
genesis.Slice.Start          # Durchlauf starten (optional mit Seed)
genesis.Slice.Start 4711     # derselbe Durchlauf wie im Protokoll
genesis.Slice.Abort          # abbrechen
genesis.Debug.Page Slice     # HUD-Seite „Durchlauf"
```

![Der Durchlauf in der ersten Stunde](Media/GENESIS-023_Run.png)

## Nebenbefund: verzögerte Befehle überleben jetzt einen Ortswechsel

`genesis.Debug.After` hat seine Befehle an die Welt geschickt, in der es aufgerufen wurde. Nach einem
Kartenwechsel war das die falsche: Die alte Welt lebt im Speicher noch eine Weile weiter, gespielt wird
aber in der neuen. Jetzt gilt die Welt des Viewports, und die Befehle laufen über den PlayerController –
nur so erreichen Befehle wie `showdebug` ihr Ziel. Ohne diese Korrektur hätte es von diesem Block
kein Bild gegeben.

## Offen

- **Keine Eingabe.** Der Durchlauf läuft von selbst; der Spieler kann noch nichts entscheiden.
  Das ist der nächste große Schritt: aus einem Ablauf wird ein Spiel.
- Der Ortswechsel ist ein harter Ladevorgang mit Schwarzblende, kein weicher Übergang.
- ~~Zwischen Einnistung und Geburt gibt es nichts zu sehen~~ – seit GENESIS-044 Teil 1b erlebt man die
  Schwangerschaft aus Sicht des Kindes (Doc 34). Noch offen: der Fetus selbst von außen (Teil 2).
- Kreißsaal, Mutter und Gestalt bleiben Platzhalter.
