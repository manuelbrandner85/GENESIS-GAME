# 33 – Der Vorfilm

*GENESIS-046. Der Film, der beim Start des Spiels läuft, bevor der Startbildschirm erscheint.*

Game Director: „Nutze diesen Trailer, aber so, dass er läuft, bevor das Spiel beginnt – weniger als Trailer, viel
mehr so, wie es bei Spielen üblich ist: Am Anfang läuft der kurze Film."

| Bereich | Stand |
|---|---|
| Schnitt aus dem Trailer, Kodierung gemessen | **PRODUCTION READY** |
| Wiedergabe im Startablauf, Ton, Untertitel, Überspringen | **PRODUCTION READY** – im gebauten Spiel geprüft (Bild, Protokoll, Tonpegel) |

![Der Vorfilm im gebauten Spiel, Untertitel im Kinobalken](Media/GENESIS-046_Vorfilm.png)

## Wo er läuft

```
Studio → Engine → Hinweis → VORFILM → „Drücke eine beliebige Taste" → Menü → Kapitel → Spiel
```

- **Nach dem Hinweis, nicht davor.** Der Film hat helle Blitze (Befruchtung, Geburt); der Hinweis auf Lichtwechsel
  gehört vor ihn. Deshalb ist er kein Engine-Startfilm (der liefe noch vor dem Studiologo), sondern eine Stufe des
  Startablaufs (`EGenesisBootStage::Vorfilm`), abgespielt über das Media Framework (Windows Media Foundation).
- **Er ersetzt den Prolog.** Prolog und Film sprechen dieselben Sätze („Bevor du deinen ersten Atemzug nahmst …"). Beide
  hintereinander hieße: denselben Text zweimal in drei Minuten. Der Film zeigt dazu, was der Prolog nicht konnte –
  Geburt, Kindheit, Alter, Tod und die Rückkehr (Doc 23, Abschnitt „Offen").
- **Danach direkt der Startbildschirm.** Der Film endet auf der Titelkarte; eine zweite Titelkarte des Spiels
  dahinter wäre derselbe Titel zweimal. Aus dem Schwarz blendet der Eileiter mit „Drücke eine beliebige Taste" auf.
- **Fehlt die Datei, erzählt der Prolog.** Der Film liegt nicht im Repository (siehe unten); ein frischer Klon
  spielt deshalb den bisherigen Prolog. `-genesisnofilm` erzwingt das zum Prüfen.
- **Zweimal drücken zum Überspringen**, wie beim Prolog: Der erste Druck zeigt „Nochmal drücken zum Überspringen",
  der zweite beendet den Film – der Ton klingt in 0,6 s aus statt abzureißen.

![Erster Tastendruck: der Hinweis unten rechts](Media/GENESIS-046_Ueberspringen.png)

![Nach dem Film: der Startbildschirm](Media/GENESIS-046_Startbildschirm.png)

## Ein Trailer wirbt, ein Vorfilm empfängt

Schnitt, Musik und Farbe des Trailers bleiben unverändert. Weg ist nur, was wirbt:

| Stelle | Im Trailer | Im Vorfilm |
|---|---|---|
| Ende | Titelkarte, danach „Bald erhältlich für PC / Trendonix Games" | endet auf der Titelkarte im Schwarz |

Gemessen an der Quelle (24 Bilder/s): Titelkarte voll bei Bild 2860, ausgeblendet ab 2895, die Werbekarte beginnt
bei 2925. Geschnitten wird bei Bild 2916, mitten im Schwarz. Der Ton läuft über die letzten 1,5 s aus.

**Der Anfang bleibt.** Er wirkt wie vier Sekunden totes Schwarz – ist es aber nicht: Ab 1,82 s spricht der Erzähler
aus dem Dunkel, erst bei 4,2 s kommt das Bild. Ein erster Schnitt bei 3,0 s hat den ersten Satz mitten im Wort
abgeschnitten. Gefunden wurde das erst beim Vermessen der Sprachspur für die Untertitel, nicht beim Ansehen der
Bilder (eigener Fehler, vor der Abgabe behoben).

**Ergebnis:** 2:01,5 (2916 Bilder), 1920 × 1080, 24 Bilder/s, H.264 + AAC 192 kbit/s, 300 MB.

## Kodierung – gemessen, nicht geschätzt

Mittlere Abweichung gegen die Quelle an den zwei schwierigsten Stellen, in 8-Bit-Stufen (von 255):

| Stelle | Stufe HIGH | Stufe MEDIUM |
|---|---|---|
| körniges Zellmakro (Detail) | 1,92 | 3,22 |
| dunkle Sterbeszene (Banding) | 1,34 | 1,84 |

MEDIUM wäre bis zu viermal kleiner, verdoppelt im Makro aber den Fehler – das Korn würde zu Matsch. Deshalb HIGH.
Quelle ist der Trailer im Repository (478 MB), nicht die hochgeladene Fassung (285 MB, gleicher Schnitt).

## Untertitel

An der Sprachspur des Trailers gemessen (Pegel über −38 dB, Lücken unter 0,45 s gehören zur Phrase), 22 Zeilen:
Erzähler ohne Namen, alle anderen mit („Hebamme: Da ist er!", „Mutter: Oh mein Gott … hallo …", „Tochter: Ich bin
hier …", „Kind: Kennen wir uns?"). Sie stehen im unteren Kinobalken, den der Film selbst mitbringt – so verdecken sie
nichts vom Bild –, rund 37 px hoch bei 1080p, und folgen der Einstellung „Untertitel".

- **Sie folgen dem Bild, nicht der Uhr des Ablaufs.** Der Abspieler meldet jeden Frame, wo der Film steht. Die Uhr
  des Ablaufs zählt ein hängendes Bild nur als Zehntelsekunde; der Film läuft weiter.
- „Jede Entscheidung … hinterlässt ein Echo." hat **keinen** Untertitel: Der Satz steht in dem Moment als Schrift auf
  der Titelkarte.
- Jede Zeile steht eine halbe Sekunde länger als gesprochen, aber nie über den Beginn der nächsten hinaus (an einer
  Stelle liegen nur 0,46 s zwischen zwei Sätzen).

## Ton

- Der Film trägt Stimme und Musik in einer Spur und folgt der Gesamtlautstärke.
- Die Menümusik weicht in 1,5 s ganz und kehrt nach dem Film in 4 s zurück; die Szene dahinter (Eileiter) tritt über
  denselben Verlauf zurück (`UGenesisAudioSubsystem::SetSceneGain`).
- **Gemessen im gebauten Spiel** (Pegelmesser der Windows-Audiositzung des Spiels, je Sekunde der Höchstwert): Pegel
  0,2–0,6 während des Films – und genau in Filmsekunde 27 und 28 **0,000**. Das sind die zwei Sekunden Stille vor
  der Geburt, die im Schnitt des Trailers stehen (26,8–29,0 s). Damit ist belegt: Der Ton kommt aus dem Film, er ist
  synchron, und unter ihm spielt nichts anderes.

## Neu erzeugen

Der Vorfilm liegt nicht im Repository (`.gitignore`): Er wäre eine zweite Kopie des Trailers im LFS-Speicher.
Er entsteht in etwa zwei Minuten aus dem Trailer:

```
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --python Tools\Intro\build_vorfilm.py
```

Ändert sich der Trailer, muss das Skript noch einmal laufen – und die Untertitelzeiten in
`GenesisBootFlow.cpp` (`FilmSubtitles`) und die Länge (`FilmLength`) müssen mitgezogen werden.

Stand der Quelle: die Trailerfassung vom 22.09.2026, 22:48 (3014 Bilder, 478 MB). Sie lag beim Bau des Vorfilms
noch **nicht** im Repository, sondern nur in der Arbeitskopie der Trailer-Arbeit; Schnittbilder und Untertitel
passen genau zu ihr. Wer den Vorfilm aus einem älteren Stand des Repositorys erzeugt, prüft zuerst die Bildzahl.

## Tests

- `Genesis.Frontend.Boot.Film`: Nach dem Hinweis kommt der Vorfilm, dahinter ist es schwarz, keine Prologstimme
  darüber; Filmende führt zum Startbildschirm (nicht zur Titelkarte), ein spätes Filmende ändert nichts mehr; meldet
  der Abspieler nie ein Ende, geht es nach Filmlänge trotzdem weiter; zwei Drücke zum Überspringen; ohne Film der
  Prolog; Filmlänge = 2916 Bilder.
- `Genesis.Frontend.Boot.FilmSubtitles`: keine Zeile reicht in die nächste, alle liegen im Film, jede ist mindestens
  eine Sekunde lang und erscheint pünktlich; erster Satz über dem Schwarz, Pause ohne Text, Untertitel folgen der
  Filmzeit statt der Ablaufuhr, kein Text auf der Titelkarte, „Kennen wir uns?" am Schluss.
- Alle bisherigen Tests des Startablaufs laufen unverändert (ohne Film = Prolog). 147 von 147.

## Prüfung (Qualitätspunkte)

| Punkt | Ergebnis |
|---|---|
| Bild | Farbe unverändert (Blender liest und schreibt ohne eigene Tonkurve); im Spiel passend ins Fenster eingepasst, Rest schwarz. |
| Ton | im gebauten Spiel gemessen, synchron, nichts darunter. |
| Spielgefühl | Hinweis vor dem Film, zweimal drücken zum Überspringen, sanfter Übergang zum Startbildschirm. |
| Barrierefreiheit | Untertitel mit Sprechernamen, abschaltbar, an Bild gekoppelt. |
| Robustheit | fehlende Datei → Prolog; Datei defekt → Startbildschirm statt zwei Minuten Schwarz; kein Endsignal → weiter nach Filmlänge. |
| Leistung | Wiedergabe über Media Foundation; die Szene dahinter läuft weiter wie im Prolog. |

## Offen

- Der Film läuft bei **jedem** Start. Üblich ist auch „nur beim ersten Start" oder ein Schalter in den Einstellungen.
- Gezeigt wird die heutige Fassung des Trailers, teils aus Konzeptbildern. Sobald die Szenen im Spiel selbst so
  aussehen (Wochen 3–8, die Fetalzeit), kann der Vorfilm aus echten Spielbildern neu geschnitten werden.
- Die Untertitel nutzen die Standardschrift der Engine (gut lesbar, aber nicht die Schrift des Covers).
