# 25 – Die Mutter und der erste Blick

Bis GENESIS-035 lag das Kind auf einem Körper ohne Gesicht. Dieser Block gibt der Mutter ein
Gesicht, einen Atem und einen Blick – und dem Spieler die erste Handlung, auf die ein Mensch
antwortet: Wer auf ihrer Brust den Blick hebt, wird von ihr hochgeholt, bis sich die Gesichter
auf 25 cm gegenüber sind.

![Blickkontakt: Gesicht zu Gesicht auf 25 cm](Media/GENESIS-036_Blickkontakt.png)

![Auf der Brust: an ihrem Arm entlang in den Raum](Media/GENESIS-036_Brust.png)

![Von der Brust nach oben – sie sieht das Kind an (Editor, ohne Schärfentiefe)](Media/GENESIS-036_Von_der_Brust.png)

![Die Mutter im Entbindungsbett (Editor-Ansicht)](Media/GENESIS-036_Mutter.png)

## Die Figur

Gebaut im MetaHuman Creator, der in Unreal 5.8 eingebaut ist (`/Game/Genesis/Characters/Mother`):

| Teil | Wahl | Grund |
|---|---|---|
| Gesicht | Vorlage „Celeste", Frau um 30 | natürliches, ungeschminktes Gesicht als Ausgangspunkt |
| Gesichts-Rig | Gelenke **und** Korrektur-Formen (Epic-Cloud) | Mimik aus 25 cm Nähe braucht die Formen, Gelenke allein reichen nur für Abstand |
| Hauttexturen | Gesicht 4K, Körper 2K | das Gesicht wird aus einer Handbreite gesehen, der Körper nie scharf |
| Haare | tiefer Zopf | nach einer Geburt ist das Haar zusammengebunden |
| Wimpern | natürlich, leicht geschwungen | keine Schminke im Kreißsaal |
| Kleidung | keine (Grundunterwäsche des Körpers) | das Standard-Shirt des Creators blendet beim Zusammensetzen die Haut darunter aus – siehe Fehler unten |
| Aufbau | Kino-Pipeline, `BP_Mother` | nahe Kamera |

**Nicht im Repository.** Die MetaHuman-Dateien (~1 GB) liegen nur lokal: Das Repository ist
öffentlich, und MetaHuman-Quelldateien dürfen nach der Unreal-Lizenz nicht öffentlich im Rohformat
verteilt werden (im gebauten Spiel sind sie enthalten). Neu erzeugen auf einem anderen Rechner:

1. Plugin „MetaHuman Creator" ist im Projekt aktiv; im Editor bei Epic anmelden.
2. Neuer MetaHuman-Charakter `/Game/Genesis/Characters/Mother/MHC_Mother`, Vorlage „Celeste" anwenden.
3. Haare und Kleidung: „Short Low Ponytail", Wimpern „Medium Slight Curl", Standard-Kleidung **entfernen**.
4. Hauttexturen Gesicht 4K anfordern, dann „Gesamtes Rig erstellen" (Gelenke und Formen).
5. Zusammensetzen: Kino-Pipeline, Name „Mother", Pfad `/Game/Genesis/Characters/Mother/Built`,
   gemeinsamer Ordner `/Game/Genesis/Characters/Common`.
6. `py "Tools/Unreal/Birth/mother.py"` im Editor – setzt Figur und Rig in den Kreißsaal.

Die Figur liegt per Skript im Bett (`Tools/Unreal/Birth/mother.py`): Hüftgelenk 10 cm über der
Sitzfläche, Rücken am 45°-Rückenteil, Maßstab 10 (Figur in cm, Szene in mm). Der Platzhalterkörper
aus GENESIS-035 ist ausgeblendet; die Decke aus der Stoffsimulation liegt weiter, die Beine der Figur
sind darunter ausgeblendet.

## Wie sie lebt – `GenesisPeople`

Neues Plugin, kein Animations-Blueprint: Eine eigene Animationsinstanz rechnet die Pose jedes Bild
aus der Referenzpose des MetaHuman-Skeletts.

| Was | Wie | Herkunft der Werte |
|---|---|---|
| Haltung | Hüfte gebeugt, Rücken rundet sich ins Kissen, Knie leicht angewinkelt | – |
| Atem | 14 Züge/min, Einatmen 40 % des Zuges; Brustwirbelsäule streckt sich, Schlüsselbeine heben sich | Ruheatmung 12–20/min; Einatmen:Ausatmen ≈ 1:1,5 |
| Kamera auf der Brust | liegt auf **ihrem** Atem (gleicher Takt) und an ihrem obersten Brustwirbel | sonst schwebte das Kind über einer Brust im anderen Rhythmus |
| Blick | Hals und Kopf wenden sich dem Kind zu (Hals 60 %, Kopf 40 % der Drehung), den Rest übernehmen die Augen; Kopf leicht zur Seite geneigt | die seitliche Kopfneigung gegenüber Säuglingen ist eine der unwillkürlichsten Gesten |
| Lidschlag | im Mittel 12/min, rechtsschief verteilte Abstände, schließt in ⅓, öffnet in ⅔ der 0,25 s; beim Blickkontakt 40 % seltener | Ruherate 15–20/min, beim aufmerksamen Hinsehen deutlich weniger |
| Hände | Zwei-Knochen-IK: rechts flach auf dem oberen Rücken des Kindes, links am Po; vor dem Gesicht rechts hinter Kopf und Nacken | – |
| Lächeln | ruhig 0,1; beim Blickkontakt Mundwinkel 0,45 und Wangen – es kommt schneller (0,6/s), als es geht (0,2/s) | – |

Gesicht: Das MetaHuman-Gesicht übernimmt Pose und Steuerkurven vom Körper. Die Animationsinstanz
schreibt deshalb Lidschlag, Augenrichtung und Lächeln als `CTRL_expressions_*`-Kurven.

## Der erste Blick (En face)

1. Auf der Brust liegt das Kind bäuchlings, den Kopf zur Seite – es sieht an ihrem Arm entlang in den Raum.
2. **Hält der Spieler den Blick oben** (0,6 s), sucht das Kind ihr Gesicht. Die Mutter antwortet: Sie
   hebt es in 2,8 s mit beiden Händen vor ihr Gesicht, waagerecht vor sich auf Augenhöhe, 25 cm entfernt.
3. Liegen ihre Augen in der Blickmitte des Kindes (< 10°), ist das **Blickkontakt**: Sie lächelt,
   blinzelt seltener, und die Bindung der ersten Stunde wächst schneller (+80 % der Grundrate,
   nur mit Hautkontakt).
4. Ein deutlicher Blick nach unten legt das Kind zurück auf die Brust.

Warum 25 cm: Mütter halten Neugeborene zum Ansehen spontan in 20–30 cm Abstand – genau die
Entfernung, auf die ein Neugeborenes scharf sieht (die Kamera des Kindes stellt nach der Geburt
auf 25 cm scharf, `NewbornFocusDistanceMm`). Das Spiel muss die Entfernung nicht erfinden; sie passt, weil beides stimmt.

Hinweis im Bild, sobald das Kind auf der Haut liegt: „Blick nach oben halten: ihr Gesicht suchen".

Prüfhilfe: `genesis.Mother.SeekFace 1` erzwingt das Suchen (wie ein gehaltener Blick).

## Gefundene und behobene Fehler

- **Durchsichtiger Oberkörper.** Das Standard-Shirt des Creators war beim ersten Zusammensetzen
  angezogen. MetaHuman blendet dann die Haut unter der Kleidung aus, damit nichts durchsticht. Das
  Shirt habe ich ausgeblendet – übrig blieb ein Körper mit Loch in Shirt-Form, durch das man die
  Matratze sah. Behoben durch Ausziehen im Creator und neues Zusammensetzen.
- **Das Gesicht stand still, der Hals drehte sich.** Im Editor lief die Gesichtsanimation nicht mit;
  der Kopf stand in der Grundhaltung neben einem geneigten Hals. Die Vorschau schaltet sie jetzt ein.
- **Augenpunkt 4 cm zu hoch.** Geschätzt statt nachgesehen. Jetzt aus den Augenknochen des
  Gesichtsskeletts.
- **Das Kind über ihrem Kopf.** „Vor dem Gesicht" war entlang ihrer Blickachse gerechnet – bei 45°
  Rückenlehne zeigt die schräg zur Decke, und ihre Arme hoben das Kind über den Kopf. Jetzt
  waagerecht vor ihr.
- **Flackernder Blickkontakt** (in der gebauten Fassung gefunden): Herzschlag und Atem ließen den
  Blick um die 10°-Grenze pendeln, der Kontakt ging mehrmals je Sekunde an und aus. Jetzt beginnt er
  unter 10° und endet erst über 15°.
- **Die Kamera lag im alten Platzhalterkörper.** Die Brust der echten Figur verdeckte den Blick
  nach oben. Die Augen des Kindes hängen jetzt an ihrem obersten Brustwirbel, berechnet aus ihrer
  Anatomie – rundet sich ihr Rücken, geht das Kind mit.

## Geprüft

- 119 von 119 Tests, davon neu: Atem (14/min, Einatmen kürzer), Lidschlag (Rate, Verteilung,
  Lidform, seltener beim Blickkontakt), En face (nur auf der Brust, Dauer, Abstand 20–30 cm,
  Lächeln nur beim Blickkontakt), Bindung durch Blickkontakt (nur mit Haut).
- Im Spiel: Geburt → Hautkontakt → Brustlage → Suchen → En face → „Erste Stunde: Blickkontakt mit der Mutter."

## Offen – und ehrlich

- **Das Kind hat keinen Körper.** Es ist die Kamera. Aus seiner Sicht fehlt nichts; von außen
  hielte die Mutter Luft.
- **Hautkontakt mit Unterwäsche.** Echt liegt das Kind auf nackter Haut, meist mit geöffnetem
  Krankenhaushemd. Ein Hemd, das so fällt, ist ein eigenes Kleidungsstück (Stoffsimulation) – noch nicht gebaut.
- **Keine Stimme beim Blickkontakt.** Mütter sprechen dabei fast immer, hoch und langsam
  („Ammensprache"). Die Stimmen laufen über kie.ai; das kommt mit dem nächsten Tonblock.
- **Keine Hebamme.** Gleicher Weg wie die Mutter, jetzt ohne Anmeldungshürde.
- **Arbeitsspeicher und Grafikspeicher:** Beim Bauen lief parallel ein Film-Render eines anderen
  Projekts; die Meldung „Video memory exhausted" im Editor stammt aus dieser Zeit. Im Spiel selbst
  nicht beobachtet, aber auf der 12-GB-Karte im Blick behalten (4K-Gesicht + Haar-Strähnen).

## GENESIS-039 (Teil 8) – Das Gesicht nach der Geburt

Bei der Durchsicht Bild für Bild (fertiges Spiel, Full HD) sah die Mutter aus wie zu einem Fototermin: trocken, gepudert,
frisch. Nach Stunden Wehen und Presswehen ist ein Gesicht nass vor Schweiß und gerötet.

- **Schweiß und Röte aus der Anstrengung** (`GenesisPeopleRendering::SkinExertion`): Bei voller Anstrengung Rauheit der Haut
  ×0,48 und Glanz ×1,4 (Schweißfilm), Grundfarbe rot ×1,08, grün/blau ×0,90 (Durchblutung) – über die Regler der MetaHuman-
  Haut („… Global Multiply Post-Bake") an Gesicht und Körper.
- **Klingt in der ersten Stunde ab** (`ExertionAfterBirth`): Halbwertszeit gut 14 min – der Schweiß verdunstet, die Röte geht
  zurück. Die Regie setzt den Wert aus der Zeit seit der Geburt.
- **Das Tuch** über dem Kind lag zu weit vorn: Beim Blick zur Mutter verdeckte es als graues Band das obere Drittel des Bildes,
  auch ihr Gesicht. Jetzt liegt es auf Hinterkopf und Rücken.

| vorher | nach der Geburt |
|---|---|
| ![](Media/GENESIS-039_Mutter_vorher.png) | ![](Media/GENESIS-039_Mutter_nach_der_Geburt.png) |

Offen: Die Haare sitzen noch wie frisch frisiert (Strähnen, feucht am Haaransatz), die Wimpern sind die des MetaHuman-Presets.
