# 22 – Startbildschirm, Einstellungen und Controller

Bis GENESIS-030 begann das Spiel von selbst und endete von selbst. Es gab keinen Ort, an dem der
Spieler entscheidet, **wann** sein Leben anfängt, und keinen, an dem er das Spiel an seinen Rechner
und an sich selbst anpasst. Dieser Block baut beides – und macht alles mit dem Controller bedienbar.

## Der Startbildschirm

![Der Startbildschirm](Media/GENESIS-045_Startbildschirm.png)

**Seit GENESIS-045 im Bild des Covers** (Docs/31_Bildsprache.md): „GENESIS" in Gold und in der Schrift des Covers
(Cinzel, eine Kapitälchen-Antiqua nach römischen Inschriften, SIL Open Font License) mit weitem Buchstabenabstand und
einem weichen Schein, darunter eine feine goldene Linie und „DER KREISLAUF DES LEBENS" in Kapitälchen. Über allem steht
der Satz des Covers: **Jede Entscheidung hinterlässt ein Echo.** Der Schleier über der Szene ist nicht mehr bräunlich,
sondern kühl und tief wie der Weltraum auf dem Cover, und er läuft als Verlauf nach unten aus – oben trägt er die
Schrift, unten bleibt das Spiel sichtbar. Die Auswahl im Menü ist ein goldener Strich.

Die Titelschrift ist kein fertiges Font-Asset: Ein solches lässt sich im Editor-Skript nicht anlegen. Importiert wird
der Schriftschnitt (`Tools/Unreal/Frontend/setup_title_font.py`, nur im vollen Editor – headless fehlt die Oberfläche),
die Laufzeitschrift baut das HUD daraus selbst. Weil kein Asset auf sie verweist, steht ihr Ordner in der Cook-Liste;
der Test `Genesis.Frontend.TitleFont` prüft, dass sie da ist.

Hinter dem Menü läuft die Szene weiter. Das ist Absicht: Das Bild des Startbildschirms ist kein
Standbild und kein gerendertes Hintergrundvideo, sondern der Eileiter selbst, in dem in diesem Moment
sechstausend Zellen schwimmen. Wer im Menü steht, schaut dem Spiel schon beim Laufen zu.

| Eintrag | wann er erscheint |
|---|---|
| **Leben beginnen** | solange noch kein Durchlauf läuft |
| **Weiterspielen** / **Von vorn beginnen** | sobald einer läuft (dann ist das Menü ein Pausenmenü) |
| **Einstellungen** | immer |
| **Beenden** | immer |

Vor dem ersten Leben pausiert das Menü nichts – es gibt ja noch nichts anzuhalten. Sobald ein
Durchlauf läuft, hält das Menü die Welt an.

## Die Einstellungen

![Die Einstellungen](Media/GENESIS-031_Einstellungen.png)

Siebzehn Einstellungen in fünf Abschnitten. Jede davon hat einen Satz darunter, der sagt, was sie
kostet oder bringt – eine Einstellung ohne Erklärung ist für die meisten Menschen eine Einstellung,
die sie nicht anfassen.

| Abschnitt | Einstellungen |
|---|---|
| **Bild** | Fenstermodus, Auflösungsskala (50–100 %), Bildratengrenze, Bildsynchronisation |
| **Grafik** | Grafikstufe (Niedrig … Ultra), Hardware-Strahlen (Lumen), Bewegungsunschärfe |
| **Ton** | Gesamt, Stimmen, Musik, Welt und Körper |
| **Steuerung** | Blickempfindlichkeit (0,2–3,0), Y-Achse umkehren, Vibration |
| **Barrierefreiheit** | Untertitel, Schriftgröße (80–200 %), Blitze dämpfen |

Drei Entscheidungen dahinter sind erwähnenswert:

- **Die Auflösungsskala hängt nicht an der Grafikstufe.** Sie ist der wirksamste Regler gegen
  Ruckeln, und wer sie braucht, will deshalb nicht gleichzeitig auf schlechte Schatten verzichten.
- **Lumen bleibt an, auch ohne Hardware-Strahlen.** Wer die Strahlen abschaltet, bekommt die
  indirekte Beleuchtung über Abstandsfelder (`r.Lumen.TraceMeshSDFs 1`) – nicht gar keine.
  Der Test `Genesis.Frontend.SettingsReachTheEngine` prüft genau diesen Ersatz.
- **Lebenswichtige Signale haben keinen eigenen Lautstärkeregler.** Sie hängen nur am Gesamtregler.
  Wer sie wegdrehen könnte, könnte sich das Spiel unspielbar einstellen.

Die Werte liegen in der `GameUserSettings.ini` unter `[/Script/Genesis.PlayerSettings]`, nicht im
Spielstand: Einstellungen gehören zum Gerät, nicht zu dem Leben, das man gerade spielt.

## Controller

Jede Handlung liegt auf einer Taste **und** auf einer Controller-Taste:

| Handlung | Tastatur | Controller |
|---|---|---|
| rufen | Leertaste | A |
| suchen | E | B |
| hinsehen | Maus | rechter Stick |
| Menü / Pause | Esc | Start |
| bestätigen | Enter | A |
| zurück | Rücktaste | B |
| Auswahl bewegen | Pfeiltasten, WASD | Steuerkreuz, linker Stick |

Das prüft der Test `Genesis.Frontend.EveryActionHasAGamepadKey`: Er geht jede Handlung durch und
verlangt für jede mindestens eine Tastatur- und eine Controller-Belegung. Eine Handlung, die nur auf
der Tastatur liegt, fällt sonst erst auf, wenn man mit dem Controller in der Hand daran hängen bleibt.

Die Anzeige im Spiel nennt die Tasten **aus den Einstellungen**, nicht aus dem Quelltext:
„Leertaste / A: rufen". Würden beide getrennt gepflegt, wäre der Hinweis irgendwann eine Lüge.

## Warum kein UMG

Das Menü wird vom HUD auf die Leinwand gezeichnet, nicht aus einem Widget-Blueprint gebaut. Der
gesamte Aufbau dieses Projekts entsteht kopflos aus Skripten – Blender baut die Geometrie, Python
baut die Materialien und Szenen, PowerShell baut und misst. Ein Widget-Blueprint wäre der einzige
Bestandteil, der nur von Hand im Editor entstehen kann und den kein Skript reproduzieren könnte.

Der Preis dafür steht unten unter „Offen".

## Geprüft

- **111 von 111 Tests bestanden**, davon fünf neue für dieses Menü.
- **Die Kette Menü → Regie läuft im Spiel:** Log zeigt `Einstellungen angewendet: Grafik Episch,
  Auflösung 100 %, …` → `Startbildschirm: Der Spieler beginnt ein Leben.` → `Durchlauf: beginnt`.
  Geprüft über `genesis.Menu` und `genesis.Menu.Accept`, die durch dieselben Funktionen laufen wie
  ein Tastendruck.
- **Die Liste passt ins Bild**, auch bei 200 % Schriftgröße: Die Zeilenhöhe wird aus dem vorhandenen
  Platz berechnet. Eine Einstellung, die unter dem Bildrand verschwindet, gibt es nicht.

## Offen

- **Der Tastendruck selbst ist nicht automatisch geprüft.** Dass die Belegung ankommt, steht im Log;
  dass die Kette wirkt, zeigen die Konsolenbefehle. Ob sich das Menü mit einem echten Controller gut
  *anfühlt*, muss ein Mensch einmal bestätigen.
- **Die Schrift ist die Engine-Schrift.** Für eine AAA-Anmutung bräuchte es eine eigene Schriftart
  als Asset; der Titel wird beim Hochskalieren weich. Gehört in den Fotorealismus-Block.
- **Keine Tastenbelegung zum Umstellen.** Die Tasten liegen fest in `DefaultInput.ini`.
- **Keine Auflösungsauswahl**, nur die Skala – die Auflösung folgt dem Fenster.
- **Kein Speicherstand.** „Weiterspielen" heißt: aus der Pause zurück, nicht: ein gespeichertes
  Leben laden.
