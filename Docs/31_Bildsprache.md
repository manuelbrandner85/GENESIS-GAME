# 31 – Die Bildsprache: das Cover als Maßstab

*Game Director, 2026-09-22: „Dies wird das Cover, orientiere dich an das Cover."*

![Das Cover](Media/Cover/GENESIS_Cover.png)

Das Cover ist ab jetzt der Maßstab für alles, was man vom Spiel sieht: Startbildschirm, Szenen, Trailer, Bilder in der
Dokumentation. Es zeigt ein einziges Leben als Spirale – Samenzelle und Eizelle, Keim, Kind, Junge, junger Mann, Mann,
alter Mann – um eine Galaxie herum, und unten einen Menschen allein vor einer weiten Welt. Darüber steht: **Jede
Entscheidung hinterlässt ein Echo.**

| Bereich | Stand |
|---|---|
| Farben und Licht gemessen, Regeln festgeschrieben | **PRODUCTION READY** |
| Startbildschirm und Titelkarte | **PRODUCTION READY**: im Bild geprüft |
| Farbstimmung der Szenen | **IN ARBEIT** |

## Gemessen, nicht geschätzt

Die Werte stammen aus dem Coverbild selbst (Mittelwerte je Bereich, linear und als sRGB-Wert):

| Rolle | sRGB | linear | Anteil am Bild |
|---|---|---|---|
| Tiefen (Weltraum) | `#0B0A0C` | 0,003 0,003 0,004 | 3 % |
| Dunkel, kühl (Nebel, Fels) | `#2D3237` | 0,026 0,032 0,039 | 12 % |
| tiefes Blau (Leuchten im Dunkeln) | `#1D262F` | 0,012 0,020 0,028 | 4 % |
| Mitten | `#605F64` | 0,116 0,116 0,127 | 34 % |
| kaltes Leuchten (Fäden, Sterne) | `#798999` | 0,190 0,251 0,318 | 18 % |
| warmes Licht, Gold | `#C6A28F` | 0,567 0,362 0,276 | 13 % |
| Haut im Licht | `#CFBDB0` | 0,621 0,511 0,433 | 13 % |
| Spitzlichter | `#E9E2DD` | 0,812 0,758 0,722 | 13 % |

**Kalt ist die Welt (36 % der Fläche), warm ist das Leben (27 %).** Das ist die Grundregel: Der Hintergrund ist kühl und
tief, das Lebendige ist warm und leuchtet von innen. Die mittlere Leuchtdichte des Covers liegt bei 0,28 – das Bild ist
dunkel, aber nie schwarz zugelaufen; die Spitzlichter brennen nicht aus.

## Regeln

1. **Licht hat eine Quelle und einen Schein.** Im Cover glüht jede Figur am Rand; der Kern der Spirale leuchtet. In den
   Szenen heißt das: eine begründete Hauptlichtquelle (Endoskop, Fenster, Deckenfeld), dazu ein sehr schwaches, kühles
   Streulicht – nie flach, nie ohne Herkunft.
2. **Warm gegen kalt.** Haut, Gewebe, Kerzenlicht gehen ins Warme; Raum, Ferne, Schatten ins Kühle. Kein Grünstich, kein
   „Teal & Orange" als Effekt – der Kontrast entsteht aus den Lichtfarben, nicht aus dem Farbregler.
3. **Dunkel, aber lesbar.** Mittlere Leuchtdichte um 0,25–0,30; Tiefen bleiben bei 0,003 statt bei 0. Nichts Wichtiges
   liegt in der Schulter des Tonemappers (das hat in GENESIS-040 Teil 2 die Schleimhaut flach aussehen lassen).
4. **Kreis und Spirale.** Wo eine Komposition frei wählbar ist (Menü, Trailer, Übergänge, Bildunterschriften), führt sie
   im Bogen. Der Kreislauf ist das Thema des Spiels.
5. **Gold ist das Zeichen des Spiels.** Gold (`#C6A28F` bis `#E9E2DD`) markiert Titel, Auswahl und Übergänge – sparsam,
   nie als Fläche.
6. **Typografie:** Serifen, Großbuchstaben, weiter Buchstabenabstand für Titel und Kapitel; die Laufschrift im Spiel
   bleibt gut lesbar. Der Untertitel steht unter einer feinen Linie, wie auf dem Cover.
7. **Kein Effektfeuerwerk.** Bloom nur als Schein um echte Lichter, keine Lens Flares, keine chromatische Aberration,
   kaum Vignette. Was auf dem Cover leuchtet, leuchtet, weil dort etwas hell ist.

## Umgesetzt: Titelkarte und Startbildschirm

Die Schrift ist **Cinzel** (SIL Open Font License, darf mitgeliefert werden) – eine Antiqua nach römischen Inschriften,
dieselbe Anmutung wie auf dem Cover. Sie liegt als Quelle unter `ArtSource/Fonts/Cinzel` samt Lizenz.

| Titelkarte (vor dem Menü) | Startbildschirm |
|---|---|
| ![](Media/GENESIS-045_Titelkarte.png) | ![](Media/GENESIS-045_Startbildschirm.png) |

- **Titel:** Großbuchstaben, weiter Abstand (20 Einheiten), Gold `#C6A28F`–`#E9E2DD`, ein weicher Schein aus acht
  versetzten, sehr schwachen Durchgängen statt eines Schlagschattens; darunter eine feine goldene Linie und der
  Untertitel in Kapitälchen.
- **Der Satz des Covers** steht über dem Titel: „Jede Entscheidung hinterlässt ein Echo."
- **Der Schleier** über der laufenden Szene ist tiefes Blauschwarz (`#0B0A0C`) statt des früheren Brauns und läuft als
  Verlauf nach unten aus – oben trägt er die Schrift, unten bleibt das Spiel sichtbar. Eine einzelne dunkle Fläche
  hinterließ eine sichtbare Kante, 32 Streifen feine Linien; beides im Bild gesehen, deshalb ein echter Verlauf.
- **Die Auswahl** im Menü ist ein goldener Strich.

Eigene Fehler dabei: Der Import der Schrift bricht headless ab (die Oberfläche fehlt) – er läuft jetzt im vollen Editor.
Ein Font-Asset lässt sich aus einem Skript gar nicht bauen; das HUD baut die Laufzeitschrift selbst aus dem
Schriftschnitt. Und weil kein Asset auf sie verweist, steht ihr Ordner in der Cook-Liste (Test `Genesis.Frontend.TitleFont`).

## Als Nächstes

- **Farbstimmung der Szenen:** ein gemeinsamer, zurückhaltender Look (kühle Tiefen, warme Lichter, Leuchtdichte im
  gemessenen Band) statt je Szene eigener Einstellungen – `Tools/Unreal/Frontend/apply_cover_look.py`.
- Kapitelkarten und Abspann in derselben Schrift.
