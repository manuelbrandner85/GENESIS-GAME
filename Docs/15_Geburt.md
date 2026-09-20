# 15 – Die Geburt

Die Geburt wird in GENESIS nicht erzählt, sondern erlebt – und zwar aus der Sicht des Kindes.
Für die Mutter sind Wehen Schmerz. Für das Kind sind sie Druckwellen: Die Gebärmutter presst, der Mutterkuchen
wird schlechter durchblutet, der Sauerstoff fällt für eine Minute, dann kommt er zurück. Dieses Auf und Ab
ist der Rhythmus der letzten Stunden vor dem ersten Atemzug.

Plugin: `GenesisBirth` (Logik ohne Welt, Subsystem auf der Weltuhr, Kamera des Kindes als Actor).

## Der Ablauf

| Abschnitt | Muttermund | Wehen | Was das Kind erlebt |
|---|---|---|---|
| Eröffnung (früh) | 0–3 cm | alle 15 min, 35 s | Druckwellen, zwischen denen alles ruhig ist |
| Eröffnung (aktiv) | 3–7 cm | alle 4 min, 55 s | Der Sauerstoff sinkt bei jeder Wehe deutlich |
| Übergangsphase | 7–10 cm | alle 2,5 min, 80 s | Die Pausen reichen zur Erholung nicht mehr aus |
| Austreibung | 10 cm | alle 2,5 min, 70 s | Enge, Drehung um etwa 90°, Licht am Ende |
| Geboren | – | – | Kälte, Klarheit, der erste Atemzug |

**Im Spiel gemessen** (Kind am Termin, 39 Wochen, ohne Erschwernis):

| Ereignis | Zeit | Zustand |
|---|---|---|
| Eröffnung früh → aktiv | 5,3 h | 3,0 cm, Sauerstoff 0,79 |
| aktiv → Übergang | 7,7 h | 7,0 cm, Sauerstoff 0,93 |
| Übergang → Austreibung | 8,9 h | 10,0 cm, Sauerstoff 0,49, Herz 106/min |
| geboren | 9,2 h | 94 Wehen, 42 min Sauerstoffmangel |

Zum Vergleich: Eine erste Geburt am Termin dauert 8–14 Stunden, die Austreibungsphase 30–60 Minuten.

## Was das Kind spürt

Die Wahrnehmung ist keine Dekoration, sondern der eigentliche Inhalt dieses Blocks:

- **Druck**: Der Verlauf einer Wehe baut sich auf, hält und lässt langsamer nach, als er gekommen ist –
  eine Sinuswelle wäre symmetrisch, eine Wehe ist es nicht.
- **Sauerstoff**: Gemessen schwankt er in der aktiven Phase zwischen 0,70 und 1,00. Der Herzschlag folgt
  ihm nach unten (bis 106/min) – das ist die Kurve, die im Kreißsaal auf dem Monitor läuft.
- **Enge**: Der Bildwinkel wird kleiner, während der Kopf im knöchernen Ring steckt (104 mm Durchmesser,
  der Kopf misst 95 mm – deshalb muss sich das Kind drehen).
- **Licht**: Erst beim Durchtritt des Kopfes fällt Licht auf das Gesicht. Der Wechsel ist ein Sprung.
- **Sehschärfe**: Ein Neugeborenes sieht mit etwa 20/400 und stellt nur auf Armlänge scharf. Deshalb hat
  die Kamera nach der Geburt eine Schärfeebene bei 25 cm und alles andere verschwimmt.
- **Sauerstoffmangel** macht die Sicht grau und eng: Der Tunnelblick ist keine Erfindung, sondern die
  Netzhaut, der das Blut fehlt.

Der erste Atemzug kommt nicht sofort – er braucht ein paar Sekunden, und bei schwacher Lungenreife länger.
Erst danach steigt der Sauerstoff.

## Übergabe an die anderen Systeme

Mit der Geburt wird der Körper geboren (`UGenesisBodySubsystem::Birth`). Das genügt: Das Audio-System leitet
aus dem Körperzustand ab, dass ab jetzt in **Luft** statt in Fruchtwasser gehört wird – der Tiefpass fällt weg,
und die Welt wird schlagartig laut und klar. Gleichzeitig wechselt die Musik in die Lebensphase „Geburt".

## Maßstab und Szene

Die Geburtsszene rechnet in **1 mm = 1 Unreal-Einheit** (die Mikrowelt der Zeugung in 1 µm = 1 Einheit).
Beide sind eigene Level und stören sich nicht.

Der Geburtskanal (`Tools/Blender/Birth/build_birth_canal.py`) ist 140 mm lang, seine engste Stelle hat
104 mm Durchmesser. Er hat Querfalten, unregelmäßige Längsfalten (regelmäßige ergaben im Bild ein Sechseck)
und eine 12 mm dicke Wand. Das Material ist zweiseitig durchscheinendes Gewebe: Licht von draußen leuchtet
die Falten von hinten rot durch.

![Blick aus dem Kanal](Media/GENESIS-021_BirthCanal.png)
![Der Kanal von innen](Media/GENESIS-021_Tunnel.png)

## Entwicklerbefehle

- `genesis.Body.Conceive 0.75` + `genesis.Clock.SkipDays 273` – ein Kind am Termin erzeugen.
- `genesis.Birth.Start` – die Wehen beginnen.
- `genesis.Birth.Speed 90` – 90 Simulationsminuten je Sekunde (eine Geburt in etwa sechs Sekunden).
- `genesis.Birth.Advance <min>` – um Minuten weiterspringen.
- `genesis.Birth.Exposure <Kanal> [draußen]` – Belichtung der Kindkamera (EV) für Bildmessreihen.
- `genesis.Debug.Page Birth` – Abschnitt, Muttermund, Druck, Sauerstoff, Herzschlag, Licht, Enge.

## Offen (bewusst, nicht versteckt)

- Der **Kreißsaal ist ein Platzhalter**: drei Flächen für Boden, Wand und Decke, ein Licht und eine
  Gestalt aus einer Kugel (`Presence_PLACEHOLDER`). Ein Neugeborenes sieht zwar nur Flächen und Farben,
  aber eine echte Umgebung mit Menschen fehlt.
- Der **Ton** der Geburt fehlt: Herzschlag, Rauschen der Mutter, der Sprung ins Hören in Luft sind im
  Audio-System angelegt (GENESIS-008), aber es gibt noch keine MetaSounds, die sie hörbar machen.
- Die **Nabelschnur** und die Nachgeburt sind nicht dargestellt.
- Das erste Zustandsbild (Apgar) wird im Moment der Geburt berechnet und danach neu – gemessen 4/10 direkt
  bei der Geburt, 8/10 nach dem ersten Atemzug. Real wird es nach einer und nach fünf Minuten erhoben;
  diese beiden Zeitpunkte fehlen als eigene Werte.
