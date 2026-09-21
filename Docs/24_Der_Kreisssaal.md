# 24 – Der Kreißsaal

In der ersten Stunde nach der Geburt handelt der Spieler zum ersten Mal selbst – rufen, suchen,
hinsehen. Bis GENESIS-034 sah er dabei drei Kästen und eine orange Kugel als „Mutter". Das war der
schwächste Ort des Spiels. Dieser Block baut den Raum, in den das Kind hineingeboren wird.

![Der erste Blick: aus dem Geburtskanal in den Raum](Media/GENESIS-035_Geburt.png)

![Die Hebamme hebt das Kind über den Bauch – Wärmebett, Tür, Geburtsseil](Media/GENESIS-035_Hinueberheben.png)

![Auf der Brust der Mutter, den Kopf zur Seite gedreht](Media/GENESIS-035_Brust.png)

## Maße und Ausstattung

Alles in echten Maßen (1 mm = 1 Einheit, wie der Geburtskanal), gebaut in
`Tools/Blender/Birth/build_delivery_room.py`:

| Teil | Maß / Ausführung |
|---|---|
| Raum | 5,4 × 4,6 m, lichte Höhe 2,9 m; PVC-Boden mit hochgezogener Hohlkehle (100 mm) und Schweißnähten alle 2 m; Kopfwand ruhig grün getönt |
| Decke | Rasterdecke 625 mm, 24-mm-Tragprofile, vier LED-Felder 600 × 600 |
| Fenster | 1500 × 1400 mm, Brüstung 850 mm, Rahmen mit Mittelpfosten, zwei Scheiben mit Zwischenraum, Fensterbank, Lamellenjalousie halb geschlossen (45°) |
| Tür | Doppelflügel-Bettentür 1100 mm mit Sichtfenster, Zarge, Drücker |
| Entbindungsbett | Sitzfläche ~800 mm, Rückenteil 45°, Fußteil abgesenkt (Gebärstellung), Hubsäule, Doppelrollen, Haltegriffe, Seitengitter heruntergeklappt |
| Tücher | echte Stoffsimulation in Metern (Baumwolldecke, blaue Einmalunterlage), danach in den Szenenmaßstab |
| Ausstattung | Wärmebett mit Heizstrahler und gefaltetem Handtuch, CTG auf Wagen, Infusionsständer mit Beutel, Wanduhr über der Tür, Geburtsseil (Tuch) von der Decke, Gebärball, Hocker der Hebamme, Schrankzeile mit Waschbecken und Spender, Untersuchungsleuchte am Deckenarm, warme Wandleuchte über dem Kopfende |

## Licht: nur, was im Raum ist

Abends, kurz nach der Geburt – viele Geburten sind abends oder nachts:

| Quelle | Wert | Herkunft |
|---|---|---|
| Fenster | 540 cd, 8000 K, Fläche = Glasfläche, hinter der Jalousie | Dämmerungshimmel ~320 cd/m² × 1,69 m² |
| 4 LED-Felder | je 720 lm, 4000 K | 3600 lm auf 20 % gedimmt |
| Wandleuchte | 800 lm, 2700 K | warmweißes Leuchtmittel |
| CTG, Wärmebett-Anzeige | 180 cd/m² | Bildschirme |
| Heizstrahler | 25 cd/m², dunkelrot | Heizstab eines Wärmestrahlers |

Das alte Platzhalterlicht (ein Rechtecklicht mit 3200 lm ohne sichtbare Quelle) ist entfernt.

Materialien: ein Master (`M_GEN_RoomSurface`) mit Farb- und Rauheitsschwankung aus einem Rauschen
über die Weltposition – keine Fläche ist gleichmäßig, nichts kachelt –, daraus 28 Instanzen mit
Werkstoffwerten (Wandfarbe 0,70, PVC 0,40, gebürsteter Edelstahl metallisch, Gummi 0,03 …), dazu
selbstleuchtende Flächen mit Leuchtdichte in cd/m², Isolierglas und streuende Haut.

## Die Kamera des Kindes

1. **Geburt:** wie bisher aus dem Kanal. Neu: Die Drehung aus dem Becken (bis 88°) löst sich in
   den ersten acht Sekunden – die Hebamme hält das Kind mit dem Gesicht nach oben. Vorher stand der
   Raum für das Kind minutenlang auf der Seite.
2. **Auf die Haut:** Sobald die Regie das Kind auf die Haut der Mutter legt, hebt die Kamera sich in
   einem Bogen (38 cm hoch, 7 s) über den Bauch auf die Brust.
3. **Auf der Brust:** bäuchlings, den Kopf zur Seite gedreht, wie Neugeborene beim Hautkontakt
   liegen. Die Brust hebt und senkt sich 14-mal je Minute um 4 mm. Das Kind schaut vom Fenster weg in
   den Raum – zum Fenster hin sähe es nur eine überstrahlte Fläche.
4. Sehschärfe unverändert: scharf nur auf 25 cm, alles andere verschwimmt.

## Gefundene und behobene Fehler

- **Das Tageslicht stand an der Wand ohne Fenster.** Der FBX-Weg Blender → Unreal spiegelt die
  Y-Achse; ich hatte die Lichtposition aus Blender übernommen. Ergebnis war ein Licht ohne Herkunft,
  das die fensterlose Wand grell ausleuchtete. Die Achsenregel steht jetzt im Skript.
- **Materialien ohne Nanite-Kennung.** Unreal setzt dann in der gebauten Fassung stillschweigend das
  graue Standardmaterial ein. Betraf die neuen Raummaterialien – und, älter, auch den Geburtskanal,
  die Corona, das Eizellplasma, die Eileiterwand und die Blastomeren (in älteren Protokollen der
  gebauten Fassung belegt). Alle Aufbauskripte setzen die Kennung jetzt ausdrücklich.
- **Eckige Hocker und Bälle.** Raum und Ausstattung enthalten Durchscheinendes (Glas,
  Infusionsbeutel), das Nanite nicht kann – Unreal zeigt dann das stark vereinfachte Ersatz-Mesh. Ein
  Neuimport behält außerdem die alte Nanite-Einstellung. Beide Meshes laufen jetzt ohne Nanite
  (zusammen unter 36 000 Flächen), Nanite wird nach dem Import ausdrücklich abgeschaltet.
- **Milchiges Bild.** Der Dunst aus der Kanalszene (Dichte 0,02) war bei 1 mm = 1 Einheit in einem
  5-m-Raum ein Nebel. Jetzt 0,0012.
- **Belichtung.** +3 EV aus dem Platzhalterraum ergab einen Bildmedian von 0,79 (weiße Wände).
  +1,6 EV und Dämmerlicht: Median 0,26 aus dem Kanal, 0,46 auf der Brust, nichts brennt aus.
- **Der Körper der Mutter** war als Metaball-Form eine dürre Gliederpuppe (bei Metaballs liegt die
  Oberfläche irgendwo innerhalb des Einflussradius). Neu gebaut über ein Skelett mit echten
  Gliedradien (Skin-Modifier), Brust und Rumpf per Voxel-Neuvernetzung zu einer Oberfläche.
- **Tücher über den Knien** hingen als Zelt vor dem Blick aus dem Kanal oder rutschten vom
  gebeugten Knie. Sie sind weg – echt wäre nur eine Hand, die sie hält.

## Geprüft

- 115 von 115 Tests.
- Im Spiel: Geburt → „Kind ist da" → „Kind liegt auf der Haut der Mutter" → Kamera auf der Brust.
- In der gebauten Fassung: alle Materialien sichtbar, keine Warnung „missing usage flag".

## Offen – und ehrlich

- **Die Mutter hat kein Gesicht.** Kopf und Gesicht sind für eine MetaHuman-Figur vorgesehen.
  MetaHuman Creator ist in Unreal 5.8 eingebaut und per Skript steuerbar, aber zwei Schritte laufen
  über Epics Cloud – das Gesichts-Rig und die hochauflösenden Hauttexturen – und brauchen **einmal
  deine Anmeldung mit deinem Epic-Konto im Editor**. Das kann und darf ich nicht übernehmen.
  Von der Brust aus ist das Gesicht jetzt nicht im Bild (das Kind kann den Kopf dafür nicht weit genug
  heben); es wird wichtig, sobald es den ersten Blickkontakt gibt.
- **Keine Hebamme.** Aus demselben Grund.
- **Die Haut der Mutter** ist glatt und gleichmäßig. Aus 3–10 cm Abstand und völlig unscharf fällt
  das kaum auf; für einen scharfen Blick bräuchte sie Poren und Textur (MetaHuman-Haut).
- **Die Füße** sind vereinfacht (keine Zehen). Sie stehen am Bildrand und sind unscharf.
- **Die Befruchtung dauert im Spiel inzwischen über eine Minute** – Folge der richtigen Zeitlupe aus
  GENESIS-030, in der der Spieler nichts tun kann. Das ist ein Tempoproblem für den nächsten Block.
