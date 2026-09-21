# 28 – Ton und Sprache im ganzen Spiel (GENESIS-039)

Auftrag des Game Directors (2026-09-21): „Sound, Sprache, Ton im gesamten Spiel, wo es Sinn macht. Nutze
kie.ai – alles muss intelligent, realistisch und mit Logik sein."

## Befund: Ab dem Menü war das Spiel still

Eine Prüfung aller Phasen ergab: Hörbar waren nur Vorspann und Menü. Befruchtung, erste Woche, Schwangerschaft,
Geburt und erste Stunde waren **stumm**, obwohl Körperklang, Ortsklang, Stimme und Musik längst als Synthese
gebaut sind. Die Klang-Actors standen früher in den Karten; jedes Skript, das eine Szene neu aufbaute und die
Karte speicherte, verlor sie wieder.

**Lösung:** Der Spielmodus legt den Klang jeder Szene beim Start selbst an (`AGenesisSliceGameMode::EnsureSceneSound`).
Der Ort entscheidet, was klingt; kein Neuaufbau kann die Szene mehr stumm machen.

| Szene | Was klingt |
|---|---|
| Eileiter (Rennen, erste Woche) | Ortsklang Eileiter, Seelenmusik |
| Geburt / erste Stunde | Mutterleib von innen und Kreißsaal von außen (gedämpft bis zum ersten Atemzug), Körperklang, Stimme des Kindes (Schreien), Sprache von Mutter und Hebamme, Seelenmusik |

**Messung ohne Ohren:** Neuer Befehl `genesis.Audio.Record <s> <Name>` nimmt den fertigen Mix des Spiels auf
(`Saved/BouncedWavFiles`), `Tools/Audio/measure_wav.py` misst Pegel, Stille und Klangfarbe.

| Aufnahme | Pegel (RMS) | Stille | Klangfarbe |
|---|---|---|---|
| Rennen im Eileiter | −33 dBFS | 0 % | mittel |
| Geburt, im Mutterleib | −30 dBFS | 0 % | sehr dumpf (gewollt: Bauchdecke, Fruchtwasser) |

## Sprache: Mutter und Hebamme (kie.ai)

32 deutsche Sätze, erzeugt mit **Gemini 3.1 Flash TTS** über kie.ai. ElevenLabs lieferte dort wie schon am
17. und 21.09. nur „Internal Error" (ohne Kosten). Kosten: 31 Credits (≈ 0,16 $). Mutter: Stimme „Sulafat"
(warm, erschöpft, ehrlich); Hebamme: „Gacrux" (ruhig, erfahren). Die Texte und die Anlässe stehen in
`Tools/Audio/speech_lines.py`.

**Jede Zeile hat einen Anlass in der Simulation, keine Zeitmarke** (`AGenesisSceneSpeech`, Logik in
`GenesisSceneSpeechLogic::Choose`, getestet):
- **Schwangerschaft:** Die Mutter spricht mit ihrem Bauch – durch Bauchdecke und Fruchtwasser gedämpft
  (Tiefpass bis 450 Hz, folgt `SoundMuffling` der Geburt).
- **Eröffnung:** Stöhnen zu Beginn einer Wehe; danach abwechselnd Atemanleitung, Lob, Erschöpfung.
- **Übergang:** „Ich kann nicht mehr …" → „Doch, Sie können. Das ist jetzt die schwerste Phase …"
- **Austreibung:** Bei jeder Presswehe „Kinn auf die Brust und mitschieben!", die Mutter presst **gleichzeitig**
  (eigener Kanal), danach „Und ausruhen". Beim Durchtritt des Kopfes: „nicht mehr pressen – nur noch hecheln"
  (Dammschutz).
- **Erschwernisse aus der Simulation:** Herztonabfälle → Seitenlage; Schulterdystokie → Beine heranziehen
  (McRoberts), nicht pressen; Steißlage → ruhige Ansage.
- **Geburt:** „Da ist sie! … ein Mädchen" oder „Da ist er! … ein Junge" – **aus dem Genom des Kindes**. Dann
  die Mutter („Oh mein Gott … hallo, du …"), das Auflegen Haut an Haut, die Atmung.
- **Erste Stunde:** leise Sätze Haut an Haut, Beruhigen, wenn das Kind schreit; Antwort auf Blickkontakt;
  die Hebamme bemerkt das Suchen nach der Brust; ein Satz beim Einschlafen.
- Die frühere Mutterstimme aus Silben ohne Worte ist abgeschaltet – sonst sprächen zwei Mütter.

**Qualitätsprüfung ohne Zuhören** (`Tools/Audio/check_speech.py`): Dauer, Sprechtempo, Grundton, Spitze,
Pausen. Zwei Aufnahmen fielen auf (15 s für zehn Wörter; ein zu leises Flüstern) und wurden neu erzeugt.
Alle Sätze auf −20 dBFS Sprechpegel angeglichen, Geflüstertes auf −26 (`normalize_speech.py`).

## Die Zeit der Geburt dehnt sich zum Höhepunkt

Gemessen: Mit 90 Simulationsminuten je Sekunde war die Austreibung in unter einer Sekunde vorbei – keine Wehe
zu spüren, kein Satz passte. Jetzt: Eröffnung 90, Übergang 0,7, Austreibung 0,125 Simulationsminuten je Sekunde.
Eine Presswehe kommt alle ~20 s und dauert 8–10 s, die Austreibung dauert gut zwei Minuten.
**Eigener Rechenfehler unterwegs:** Zuerst 6 statt 0,125 – die Einheit ist Minuten, nicht Sekunden.

Im Spiel gemessen (Protokoll, Uhrzeit):

```
22.04.21 T_M_KannNicht (Mutter)      22.04.47 P_H_Schieben_02 (Hebamme)
22.04.25 → Austreibung               22.04.48 P_M_Pressen (Mutter, gleichzeitig)
22.04.25 P_M_Pressen (Mutter)        22.04.54 P_H_Pause (Hebamme)
22.04.27 P_H_Pause (Hebamme)         22.05.07 P_H_Schieben_01 …
```

## Offen

- Das Rennen hat nur den Ortsklang: kein Klang der eigenen Bewegung (Geißelschlag), kein Hinweiston beim Binden.
- Kreißsaal-Geräusche (CTG-Doppler, Türen, Gerät) kommen aus der Synthese; echte Aufnahmen (Suno „sounds")
  wären ein nächster Schritt.
- Keine Stimme eines Partners, keine zweite Fachkraft (z. B. Ärztin bei Schulterdystokie).
- Die Stimmenwahl (Sulafat/Gacrux) wartet auf das Ohr des Game Directors.

## GENESIS-039 (Teil 2) – Realistischer: das Kind, der Raum, die Zellen

Game Director: „Fahre fort und realistischer."

### Das Kind schreit mit echten Aufnahmen

Das synthetische Schreien ist abgeschaltet. Jetzt spielen echte Aufnahmen (kie.ai, Suno „sounds", je zwei
Varianten im Wechsel, 15 Credits ≈ 0,08 $). **Geprüft ohne Zuhören** mit `Tools/Audio/check_cry.py`: Grundton
410–471 Hz (Neugeborene: ~450 Hz), 23–37 Schreie pro Minute, kurze Einatmer dazwischen. Die Anlässe kommen
aus der Simulation (`GenesisSceneSpeechLogic::ChooseChild`, Test ChildSounds):
- **Erster Schrei:** eine halbe Sekunde nach der Geburt (Keuchen, Husten, dünner Schrei). Die Hebamme spricht darüber.
- **Kräftig oder wimmernd:** je nach Schreilautstärke des Kindes (Kälte, Hunger, allein). Ohne Pause in Wellen;
  beruhigt es sich, klingt der Schrei aus.
- **Ruhig auf der Haut:** selten Grunzen, Seufzen, Schmatzen. Im Schlaf still.

### Der Kreißsaal hat einen Raum

Echter Raumklang nachts (Lüftung, ferne Schritte, Tür, Monitore) als Schleife unter der Sprache. Vor der Geburt
nur als dumpfes Rauschen durch die Bauchdecke, danach klar. Gemessen nach der Geburt: −21 dBFS, Spitzen
−3 dBFS vom Schreien des Kindes.

**Messfehler gefunden:** Eine Aufnahme war völlig still (−180 dBFS). Ursache: Unreal stellt ein Fenster, das
nicht vorn liegt, stumm (`UnfocusedVolumeMultiplier = 0`). Für Spieler bleibt das so; die Testläufe setzen es
jetzt auf 1, damit jede Messung unabhängig vom Fenster ist.

### Die Zellen um die Eizelle sehen aus wie Zellen

Aus der Ich-Perspektive waren die Cumuluszellen glatte, rosa Plastikeier. Jetzt:
- ein angedeuteter Kern (aus der Flächennormale, je Zelle versetzt)
- feine Granula (~1 µm)
- ein heller Saum an der Silhouette
- kaum Farbe

**Das Rosa war erfunden:** Rotes Durchscheinen gibt es nur in dickem, durchblutetem Gewebe. Durch 12–110 µm
Zelle geht Licht fast farblos. Das galt auch für Eizelle und Polkörper; beide sind jetzt grau-beige.

**Drei Anläufe, alle am Bild geprüft:**
1. Kern zu deutlich: Die Zellen wirkten wie Spiegeleier.
2. Körnung als Würfel-Zufall: sichtbare Voxel; die Zellen wirkten wie Kiesel.
3. Weiches Rauschen und hellerer Ton.

![Vorher: Spiegeleier](Media/GENESIS-039_Coronazellen_Spiegeleier.png)
![Jetzt](Media/GENESIS-039_Coronazellen.png)

### Bewusst nicht gemacht

**Kein Klang im Rennen für die eigene Bewegung:** Ein Spermium hört nichts, und in dieser Größe gibt es keinen
hörbaren Schall. Erfundene „Schwimmgeräusche" wären unrealistisch. Der Ortsklang (Strömung, Herzschlag der
Mutter durchs Gewebe) bleibt.

### Offen

- Die Form der Cumuluszellen ist zu regelmäßig (Ellipsoide aus Blender), ohne Fortsätze zur Zona.
- Die Raumschleife hat die Nahtstelle einer MP3; ein weicher Übergang wäre besser.
- Die Stimmen und das Schreien warten auf das Ohr des Game Directors.
## GENESIS-039 (Teil 3) – Kein Klingeln mehr, die Ohren eines Neugeborenen

Game Director: „Das komische Klingeln im Kreißsaal weg. Für das Baby sollen die Stimmen und die Wahrnehmung
entsprechend da sein, Kreißsaal-Töne realistisch."

**Zwei Quellen des Klingelns, beide gemessen** (`Tools/Audio/check_tones.py`: reine Töne im Spektrum):
1. Die Synthese des Kreißsaals piepte als reiner 980-Hz-Ton bei jedem Herzschlag der Mutter und hatte
   Metallklänge aus Sinustönen (2,4–5,6 kHz). Echte Kreißsäle piepen nicht bei jedem Herzschlag. Die Synthese des
   Raums ist im Spiel abgeschaltet.
2. Die erste Raumaufnahme trug selbst einen Ton: 1002 Hz, +30 dB über dem Raum. Ich hatte „ferne Monitor-Pieptöne"
   bestellt. Neu erzeugt ohne Geräte; alle vier Aufnahmen und der fertige Spielmix nach der Geburt sind frei von
   reinen Tönen, auch mit Musik.

**Das Ohr des Neugeborenen:** In den ersten Stunden stecken noch Fruchtwasser und Käseschmiere in Gehörgang und
Mittelohr. Deshalb fällt das Hörscreening in den ersten 24 h häufiger durch. Nach der Geburt ist deshalb alles oberhalb
3,5 kHz gedämpft; in der ersten Lebensstunde wird es auf der Oktavachse gleichmäßig bis 12 kHz klarer
(Test NewbornHearing).

**Stimmen von dort, wo die Menschen sind:** Vor der Geburt kommen sie ohne Richtung durch die Bauchdecke. Danach
kommen sie aus Richtung und Entfernung; in der Ferne dämpft die Luft die Höhen. Im Spiel gemessen (Protokoll):

```
G_M_Bauch_02   Mutter   ohne Richtung, durch die Bauchdecke
D_H_Da_M       Hebamme  2° links, 0,44 m   (sie hält das Kind)
D_M_Hallo      Mutter   174° links, 1,06 m (hinter dem Kind)
S_M_Warm       Mutter   133° rechts, 0,24 m (Kind auf ihrer Brust)
```

Liegt das Kind auf der Mutter, steht die Hebamme neben dem Bett, nicht mehr vor seinem Gesicht.

**Befund aus dem Bild:** In den ersten Sekunden nach der Geburt sah das Kind einen **leeren Raum** (Schränke, Tür).
In Wirklichkeit fängt die Hebamme es auf und hebt es hoch – sie ist das Erste, was es sieht. Das kommt als
Nächstes (sichtbare Hebamme, MetaHuman).