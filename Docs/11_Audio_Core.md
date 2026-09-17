# 11 – Audio Core

Plugin `GenesisAudioCore` (GENESIS-008). Grundlage aller Audio-Module: **Wie hört diese Person die Welt gerade?**
Die Werte werden aus der Body Simulation abgeleitet. Klang, MetaSounds, Musik und Stimmen lesen sie nur und berechnen sie nicht selbst.

> Stand GENESIS-008: Die Wahrnehmungs-, Körperklang- und Mix-Parameter werden berechnet, geglättet, getestet und im Developer HUD angezeigt.
> **Noch kein hörbarer Klang.** Die Anbindung an Submixes, Sound Classes und prozedurale MetaSounds folgt in GENESIS-011.

## Audio-Plan (Blöcke)

| Block | Modul(e) | Inhalt |
|---|---|---|
| 008 | GenesisAudioCore | Hörwahrnehmung, Körperklang-Parameter, Mix-Engine mit Prioritäten und Ducking |
| 009 | GenesisSoulMusic | Seelenmotiv, Phasen-Instrumentierung, Charaktermotive (Verschmelzen/Trennen), Vererbung, Erinnerungsfragmente, Lebens-Soundtrack |
| 010 | GenesisMusicDirector | Schichten statt Tracks, Emotion → musikalische Parameter, Stille, subjektive Zeit, Quartz-Takt |
| 011 | MetaSounds-Basis | Submixes/Sound Classes je Bus, prozedurale MetaSounds (Herz, Atem, Mutterleib, Seelenmotiv), Wiedergabe, Prüfung durch Aufnahme |
| 012 | GenesisVoiceSystem | VoiceProfile, Stimmalterung, Gesundheit, Babylaute → Wörter → Sätze |
| 013 | GenesisDialogueSystem | Dialog-Datenbank, Lokalisierung DE/EN/IT, Weltwahrheit, Gerüchte, Untertitel |
| 014 | Vertical-Slice-Audio | Entstehung, Embryo, Geburt, erste Minuten |

Audio-Persistenz: Jedes Audio-Modul implementiert `IGenesisPersistentSystem` (kein eigenes Save-Modul). Audio-Debug: je Modul eine HUD-Seite.
Audioquellen: prozedurale MetaSounds aus Engine-Nodes. Freie Fab-Pakete (Ambient/Foley) fügt der Game Director bei Bedarf über sein Epic-Konto hinzu.
Stimmen und Babylaute sind bis zur Produktion markierte Platzhalter. Keine Stimmklone realer Personen, keine geschützten Songs.

## Hörwahrnehmung (`FGenesisHearingPerception`)

| Wert | Mutterleib | Nach der Geburt |
|---|---|---|
| Tiefpass | 150–800 Hz, steigt mit der Reife des Gehörs | 4–20 kHz nach Hörschärfe, Fieber senkt ihn |
| Hochton-Absenkung | 0 | ab 50 Jahren biologisch −0,4 dB/Jahr, **höchstens −12 dB** (nie unangenehm) |
| Außen hörbar | 0,05–0,5 | nach Hörschärfe, Tunnel-Hören senkt ihn |
| Körper hörbar | 0,5–1,0 (Mutterherz, Blutfluss) | 0,05, steigt mit Angst/Schmerz, Herzklopfen und Atemnot |
| Tunnel-Hören | 0 | Adrenalin über 0,5 plus Schmerz |
| Tinnitus | 0 | Kopfverletzung, ab 70 Jahren leicht (max. 0,6) |

**Glättung:** Der Tiefpass wird logarithmisch interpoliert, damit sich jede Oktave gleich schnell anfühlt. Normale Wechsel verwenden die Zeitkonstante 1,5 s.
Ein Umgebungswechsel (Geburt: Mutterleib → Luft) startet einen **Übergang** (`bInEnvironmentTransition`) mit 0,25 s. Dieser bleibt aktiv, bis der Tiefpass auf 2 % am Ziel liegt, und ergibt den hörbaren Frequenzsprung bei der Geburt.
Das Ereignis `OnHearingEnvironmentChanged` meldet den Wechsel an spätere Systeme (Musik, Cinematic).

## Körperklang (`FGenesisBodyAudioParams`)

- **Vor der Geburt:** Mutterherz 72 bpm dominiert. Das eigene Herz ist schneller und wird mit der Organentwicklung kräftiger.
- **Danach:** Herzfrequenz, Stärke, Unregelmäßigkeit (Verschleiß + Herzerkrankung), Atemfrequenz, -tiefe, Atemnot und Zittern. Die Hörbarkeit folgt der Wahrnehmung.

## Mix-Engine

Busse nach Priorität: **Dialog > lebenswichtige Signale > Körper > Foley > Ambient > Musik**.

| Auslöser | Ziel | Absenkung | Attack / Release |
|---|---|---|---|
| Dialog | Musik | −8 dB | 0,35 s / 1,2 s |
| Dialog | Ambient | −5 dB | 0,35 s / 1,5 s |
| Dialog | Foley | −3 dB | 0,35 s / 1,0 s |
| Signal | Musik | −6 dB | 0,15 s / 1,5 s |
| Signal | Ambient | −3 dB | 0,15 s / 1,5 s |
| Körper | Musik | −2 dB | 0,8 s / 2,0 s |

- Die Absenkung skaliert mit der **Wichtigkeit** der Anforderung: Ein beiläufiger Satz duckt weniger als ein entscheidender.
- Mehrere Auslöser summieren sich, begrenzt auf **−18 dB**.
- Exponentielle Zeitkonstanten: keine hörbaren Lautstärkesprünge. Getestet ist ein Maximalschritt unter 0,5 dB pro Frame bei 60 fps.
- Der Körper meldet sich selbst im Mix an, sobald er deutlich hörbar ist (> 0,4).
- Alle Regeln sind unter *Project Settings → Genesis → Audio* einstellbar.

## Schnittstelle

`UGenesisAudioSubsystem` (GameInstance, tickt pro Frame):

- `SetListenerEntity(Id)`. Ohne expliziten Hörer wird der erste voll simulierte Körper verwendet (Entwicklerkomfort).
- `PushMixRequest(Bus, Wichtigkeit, Haltedauer)`.
- `GetHearingPerception()`, `GetBodyAudioParams()`, `GetBusGainDb/Linear(Bus)`.

Die Logik liegt zustandslos in `GenesisAudioCoreLogic` und ist ohne World testbar.

## Tests

- `Genesis.Audio.Core.WombAndBirthHearing`: Tiefpass nach Reife, Mutterherz, Frequenzsprung > 10× bei der Geburt, Übergang in ~2 s abgeschlossen, erster Atem.
- `Genesis.Audio.Core.HearingAcrossLife`: Erwachsene, Schock-Tunnel, Alter (Absenkung begrenzt), Tinnitus nach Kopfverletzung, Herzverschleiß.
- `Genesis.Audio.Core.MixDuckingIsSmooth`: Ducking-Tiefe, Schrittgröße pro Frame, Erholung, Wichtigkeit, Summen-Begrenzung.

## Entwicklerbefehle

- `genesis.Audio.SimulateDialogue [Sekunden] [Wichtigkeit]`: meldet einen Dialog im Mix an.
- `genesis.Debug.Page Audio`: zeigt nur die Audio-Seite im HUD.
