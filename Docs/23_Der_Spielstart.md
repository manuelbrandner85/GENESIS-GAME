# 23 – Der Spielstart: vom Logo bis zum ersten Atemzug

Rückmeldung des Game Directors: *„insgesamt ist es nicht wie ein richtiges Spiel – es soll sich wie
ein echtes Spiel verhalten, vom Menü, vom Anfang usw., eine Art Anfangstrailer."*

Bis GENESIS-033 öffnete die gebaute Fassung direkt ein Menü über einer laufenden Szene. Ein Spiel
beginnt aber nicht mit einer Entscheidung, sondern mit einem Auftritt: Wer es startet, wird
empfangen, bekommt eine Stimmung und wird beim Übergang ins Spiel nicht einfach in einen Level
geworfen. Dieser Block baut genau diese Abfolge.

## Der Ablauf

| Stufe | Dauer | Was man sieht und hört | Überspringen |
|---|---|---|---|
| Studio | 4,0 s | „GENESIS TEAM präsentiert", Musik blendet ein | 1 Druck |
| Engine | 3,2 s | „Entwickelt mit Unreal Engine 5" | 1 Druck |
| Hinweis | 6,5 s | Kopfhörer empfohlen, Lichtwechsel, wo man sie dämpft | 1 Druck |
| **Prolog** | 52 s | Eine einzige Kamerafahrt durch den Eileiter auf die Eizelle zu, zehn Sätze des Erzählers mit Untertiteln | 2 Drücke |
| Titel | 7 s | GENESIS – Der Kreislauf des Lebens | 2 Drücke |
| Taste | – | Der Eizellkomplex hinter dem Titel, „Drücke eine beliebige Taste" (atmet, blinkt nicht) | – |
| Menü | – | Hauptmenü mit Musik und Klängen | – |
| **Kapitel** | 6,2 s | Abblenden, „Erstes Kapitel – Der Anfang – Eileiter, am Tag des Eisprungs"; hinter Schwarz wird der Eileiter frisch geladen | – |
| Spiel | – | das Leben | Esc / Start: Pause |
| Ende | 10 s | 6 s nach dem letzten Moment: Abspann, dann zurück ins Menü | – |

Ohne einen einzigen Tastendruck dauert es **72,8 s** bis „Drücke eine beliebige Taste" – gemessen im
Test `Genesis.Frontend.Boot.RunsLikeAGame`. Danach wartet das Spiel; es springt weder von selbst
ins Menü noch ins Spiel.

![Prolog: der Eizellkomplex im Eileiter, mit Untertitel](Media/GENESIS-034_Prolog.png)

![Titelbildschirm](Media/GENESIS-034_Taste.png)

![Kapitelkarte beim Lebensbeginn](Media/GENESIS-034_Kapitel.png)

## Entscheidungen

- **Der Prolog ist in Echtzeit, nicht als Video.** Er spielt im echten Eileiter mit dem echten
  Schwarm. Kein Film kann das, was danach im Spiel kommt, so genau ankündigen – und die gebaute
  Fassung wird dadurch nicht um ein Video größer.
- **Zwei Drücke zum Überspringen der Erzählung.** Der erste zeigt „Nochmal drücken zum Überspringen"
  (verfällt nach 3 s). Ein versehentlicher Druck soll den Prolog nicht beenden – so machen es die
  meisten großen Spiele. Karten lassen sich mit einem Druck überspringen.
- **Der Level wird beim Lebensbeginn frisch geladen – hinter Schwarz.** Vorher begann ein Leben im
  Eileiter, der schon minutenlang hinter dem Menü lief; der Schwarm konnte die Eizelle dann bereits
  erreicht haben. Der Test `Genesis.Frontend.Boot.LifeCycle` prüft, dass geladen wird, wenn das Bild
  ganz schwarz ist und die Kapitelkarte zu lesen ist, und genau einmal.
- **Ende → Menü.** Ein Leben, das zu Ende ist, bleibt nicht einfach stehen: sechs Sekunden für den
  letzten Moment, dann Abspann, dann das Menü mit frischem Eileiter.
- **Der Zustand lebt in der GameInstance.** So überdauert er jeden Ortswechsel, und nach dem
  frischen Laden erscheint nicht wieder das Studiologo.
- **Keine roten Entwicklerzeilen im Bild.** Die spielbare Fassung zeigt die Bildschirmmeldungen der
  Engine nicht mehr (im Protokoll stehen sie weiterhin) – eine Veröffentlichungsfassung zeigt sie nie.
- **Eine Taste wird nicht doppelt gezählt.** Esc ist im Vorspann eine Taste wie jede andere; das
  Menü nimmt im ersten Augenblick nach dem Erscheinen keine Eingaben an, damit der Druck, der den
  Titel weggeklickt hat, nicht gleich „Leben beginnen" auslöst.

## Ton

| | Quelle | Pegel |
|---|---|---|
| Hauptthema (Vorspann, Menü, in Schleife) | **Suno V5 über kie.ai**, instrumental: Klavier, Streicher, Celesta, Harfe, 62 bpm; zwei Fassungen (2:53 und 3:00), die erste ist Menümusik, die zweite liegt als Alternative daneben | Vorspann −4,4 dB, unter dem Erzähler weitere −7 dB |
| Erzähler | kie.ai, Gemini-Stimme „Algieba" (Take A aus dem Trailer) | alle zehn Sätze auf −14 dBFS angeglichen |
| Menüklänge | selbst gebaut (`Tools/Frontend/generate_ui_sounds.py`): Holzton für Auswahl, angeschlagenes Glas für Bestätigen, eine Quarte tiefer für Zurück | leise, damit man sie nach dem zehnten Mal nicht hasst |

**Rückmeldung „die Stimme ist zu leise im Vergleich zur Musik" – umgesetzt.** Gemessen lagen die
Sätze zwischen −19,8 und −16,4 dBFS, von Satz zu Satz um fast 2 dB verschieden. Jetzt:

1. Jeder Satz auf **−14 dBFS** angeglichen (+0,8 bis +5,8 dB), Spitzen weich begrenzt, nichts
   übersteuert (`Tools/Frontend/normalize_voice.py`).
2. Die Musik liegt im Vorspann **4,4 dB** tiefer als im Menü.
3. Während der Erzähler spricht, geht die Musik um weitere **7 dB** zurück – in 0,25 s hinunter, in
   1,4 s wieder hinauf, damit man das Atmen der Mischung nicht hört (Ducking wie im Film).

### ElevenLabs über kie.ai: fehlgeschlagen

Für eine noch natürlichere Erzählerstimme war ElevenLabs Multilingual v2 vorgesehen (Stimme „Brian –
Deep, Resonant and Comforting", dazu „Benjamin" und „Theodore" zum Vergleich). **Alle Aufträge
scheiterten mit „Internal Error, Please try again later"**, auch ein einzelner, minimaler Satz – wie
schon beim Trailer im September. Die 60 Credits, die dabei zunächst abgebucht wurden, hat kie.ai
zurückerstattet. Ich habe die Schnittstelle danach nicht weiter aufgerufen und den Prolog mit den
vorhandenen Gemini-Aufnahmen gebaut. Das Skript (`Tools/Frontend/generate_frontend_audio_kie.py --vo`)
steht bereit, sobald ElevenLabs dort wieder funktioniert.

Guthaben: 1 840,38 vorher, **1 828,38 nachher – verbraucht 12 Credits** (eine Suno-Generierung).
Aufträge und Stile stehen in `Docs/Audio/Frontend_Audio_Manifest.json`; der Schlüssel steht nirgends
im Projekt.

**Rechte:** Die Erzählerstimme ist eine synthetische Standardstimme, kein Klon einer realen Person.
Ob die Suno-Musik in einer **Veröffentlichung** verwendet werden darf, hängt von den
Nutzungsbedingungen von kie.ai/Suno für den verwendeten Tarif ab – das ist vor einer Veröffentlichung
zu prüfen.

## Geprüft

- **115 von 115 Tests**, davon vier neue für den Ablauf (Ablauf ohne Eingriff, Überspringen,
  Lebenszyklus mit Laden hinter Schwarz und Rückkehr ins Menü, Prolog-Zeitplan und Kamerafahrt).
- **Die gebaute Fassung einmal ganz durchgespielt** (über `genesis.Boot.Press`, `genesis.Menu.Accept`
  und `genesis.Boot.End`, die durch dieselben Funktionen laufen wie ein Tastendruck). Protokoll:
  Studio → Engine → Hinweis → Prolog → Titel → Taste → Menü → Kapitel → *Ein Leben beginnt* →
  *Ortswechsel nach L_GEN_OviductAmpulla* → Spiel → Ende → *zurück ins Menü* → Menü. Kein Ton fehlt.
- Die Längen der Erzähleraufnahmen im Code (4,88 s, 3,40 s, …) sind die gemessenen Längen der
  importierten Dateien – kein Satz redet in den nächsten hinein.

## Offen

- **Ob die Mischung jetzt stimmt, muss ein Ohr entscheiden.** Die Pegel sind gemessen und gerechnet;
  den Eindruck bestätigt nur Zuhören.
- **Die Schrift ist die Engine-Schrift.** Für ein Studiologo und einen Titel wäre eine eigene
  Schriftart angemessen.
- **Kein echtes Logo**, nur Text.
- Die Kamerafahrt im Prolog hat keine Schnitte und zeigt nur den Eileiter – Geburt, Leben und
  Tod aus dem Trailer kommen im Prolog noch nicht vor, weil es diese Bilder noch nicht gibt.
