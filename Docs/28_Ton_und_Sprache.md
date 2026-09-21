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
