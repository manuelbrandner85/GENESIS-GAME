# 00 – GENESIS Reveal Trailer: Produktion

**Titel:** GENESIS: DER KREISLAUF DES LEBENS – Cinematic Reveal Trailer
**Länge:** 120,0 s · 24 fps · Master 3840 × 2160
**Kernsatz:** JEDE ENTSCHEIDUNG HINTERLÄSST EIN ECHO.
**Stand:** 2026-09-17 · Phase **ANIMATIC v1** (kein finales Bild vorhanden)

Einzige Datenquelle ist [`ShotList.json`](ShotList.json). Daraus werden erzeugt: Tabelle [`01_ShotList.md`](01_ShotList.md), der Tonmix, das Blender-Animatic und die Unreal-Sequencer-Struktur. Wer das Timing ändert, ändert nur die JSON und startet die Skripte neu (siehe unten).

---

## 1. Analyse der vorhandenen GENESIS-Assets (2026-09-17)

| Bereich | Vorhanden | Für den Trailer nutzbar |
|---|---|---|
| Simulation (C++) | Soul, Genetik, Memory Graph, Life Simulation, Decision Engine, Body Simulation, Audio Core (008), Soul Music (009, Motive je Person, noch ohne Wiedergabe) | Nur als Hintergrund: Die Systeme haben noch **keine sichtbare Darstellung** |
| Maps | `L_DevSandbox` (Template-Boden, Himmel) | Nein |
| 3D-Assets, Materialien, VFX | **keine** | – |
| Figuren / MetaHumans | **keine** (MetaHuman-Plugins in UE 5.8 installiert) | – |
| Animationen | **keine** | – |
| Audio | **kein** Soul Theme vorhanden. Unreal-Bibliothek: Harmonix-Instrumente (Piano, Streicher legato/pizzicato, Hörner, Flöte, Vibraphon, Kontrabass) | Ja → Soul Theme daraus komponiert |
| Stimmen | Unreal hat **keine deutsche Stimme** (TextToSpeech-Plugin = Flite, nur Englisch) | Nein → kie.ai (s. u.) |

**Konsequenz:** Der Trailer muss praktisch vollständig neu gebaut werden. Echtes Gameplay existiert für keinen Shot. Alle Shots sind intern als `CONCEPT` markiert. Shots mit `GAMEPLAY_TARGET` sollen durch echtes Gameplay ersetzt werden, sobald der jeweilige Block (GENESIS-010 bis 014) spielbar ist. **Extern darf nichts davon als spielbar dargestellt werden, bevor es das ist.**

## 2. Asset-Lückenliste

| # | Asset | Werkzeug | Für Shots | Priorität |
|---|---|---|---|---|
| A1 | Spermium (Kopf, Mittelstück, Geißel, Rig/Shape-Anim), Schwarm-Instanzen | Blender → Unreal (Niagara/ISM) | SH_001_030–050 | hoch (auch Gameplay 010) |
| A2 | Eizelle mit Corona radiata und Zona pellucida, Zellteilung 2/4/8/Morula | Blender (Geometry Nodes) → Alembic/Unreal | SH_001_050, SH_002_010, SH_010_010 | hoch |
| A3 | Organische Mikro-Umgebung (Schleimhautfalten, Fluid, Schwebstoffe) | Blender + Unreal Volumetrics | SH_001_020–040 | hoch |
| A4 | Embryo, Fetus (Hand, Gesicht) – respektvoll, stilisiert-realistisch | Blender Sculpt + SSS | SH_002_020–040 | hoch |
| A5 | MetaHumans: Protagonist/in in 5 Altersstufen (Baby, 6, 16, 30, 78/86), Mutter, Vater, Partner/in, Tochter, Enkel, neue Familie | MetaHuman Creator (UE 5.8) | ~40 Shots | **kritisch** |
| A6 | Hund (Kindheit + Jenseits) | Fab-lizenziert oder Blender | SH_004_040, SH_009_060 | mittel |
| A7 | Sets: Kreißsaal, Kinderzimmer, Wiese, Schulhof, Dach/Stadt nachts, Küche, Krankenhausflur, Bahnsteig, Klippe mit Bank, Sterbezimmer | Unreal (Megascans/Fab + Blender-Props) | viele | hoch |
| A8 | Epochen-Sets: Mittelalterdorf, Renaissance-Werkstatt, Industriehalle, Zukunftsstadt, Lehmhaus mit Öllampe, Bergdorf | Unreal + Blender-Props | SH_006_100–130, SH_010_020–030 | mittel |
| A9 | Jenseits: kosmische Landschaft, Lichtpfade, Bibliothek, Erinnerungsfragmente, Portal | Unreal (Niagara, Volumetrics) + Blender | SH_009_030–060 | hoch |
| A10 | Planet mit Zeitraffer (Kontinente, Wolken, Stadtlichter), Sonnensystem, Galaxie | Blender (Shader) → Unreal | SH_011_* | hoch |
| A11 | Goldener Faden (Spline-Mesh + Material), Kreisel, Ehering, Halsband, Foto, Spieluhr | Blender → Unreal | wiederkehrende Symbole | mittel |
| A12 | Titel „GENESIS“ aus goldenen Fäden | Blender (Curves) → Unreal | SH_012_020 | mittel |
| A13 | Sound Design: Herzschlag, Fluid, Atem, Babyschrei, Wind, Regen, Tür, Krankenhaus | Unreal MetaSounds / lizenzierte Aufnahmen | alle | mittel |
| A14 | Chor für das Jenseits (derzeit Streicher-/Flöten-Pad als Platzhalter) | lizenziert oder eigene Aufnahme | SEQ_009–011 | mittel |

## 3. Pipeline und Skripte

| Schritt | Skript | Ergebnis |
|---|---|---|
| Instrumente aus der Unreal-Bibliothek exportieren | `Tools/Trailer/Run-UnrealPython.ps1 -Script Tools/Trailer/Unreal/ExportLibraryAudio.py -EnablePlugins Harmonix` | 127 WAV-Samples in `Genesis/Saved/TrailerAudio/Library` |
| Stimmen erzeugen (kie.ai) | `$env:KIE_API_KEY=…` · Blender-Python `Tools/Trailer/Audio/generate_vo_kie.py` | Takes A/B + Auswahl, Messwerte in [`VO_Takes.json`](VO_Takes.json) |
| Soul Theme, Sound Design, Mix | Blender-Python `Tools/Trailer/Audio/build_trailer_audio.py` | Stems + Mix in `Genesis/Saved/TrailerAudio/Mix` |
| Animatic | `blender -b --factory-startup --python Tools/Trailer/Blender/build_animatic.py` | `Genesis/Saved/Trailer/Animatic/GENESIS_Trailer_Animatic_v1.mp4` |
| Unreal-Sequencer | `Tools/Trailer/Run-UnrealPython.ps1 -Script Tools/Trailer/Unreal/BuildTrailerSequencer.py` | Map, Master, 12 Sub-Sequences, 65 Kameras, Audio |
| Shot-Tabelle | Blender-Python `Tools/Trailer/make_shotlist_md.py` | `01_ShotList.md` |

Blender-Python: `C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe` (enthält numpy; ein System-Python gibt es nicht).

### Unreal-Struktur
```
/Game/Genesis/Trailer/Maps/L_Trailer_Stage
/Game/Genesis/Trailer/Sequences/LS_GENESIS_REVEAL_TRAILER     Master: Subsequence-Track, 3 Audio-Tracks, Fade-Track, 65 Shot-Marker
/Game/Genesis/Trailer/Sequences/Sub/SEQ_001_Origin … SEQ_012_Title
    je Shot: Spawnable-CineCamera "SH_xxx_yyy_Name" (Super 35, 16:9, Brennweite laut Liste), Spawn-Track, Transform-Keys, Camera Cut
/Game/Genesis/Trailer/Audio/MX_SoulTheme_Trailer_Animatic, SFX_Trailer_Animatic, VO_Trailer_Placeholder
```
Die Shot-Nummern folgen der Sequenz (`SH_<Sequenz>_<Shot>`). Die Beispiele im Briefing waren nicht einheitlich nummeriert (z. B. „SH_002_010_SpermTracking“ liegt dramaturgisch in SEQ_001), deshalb gilt die Sequenznummer.

Jeder Shot hat einen eigenen Bühnenbereich (X = Shotindex × 20 m). Die Kamerabewegungen sind **Blocking**: Richtung und Größe stimmen, die Feinabstimmung erfolgt mit den echten Sets.

## 4. Stimmen

- **Geprüft 2026-09-17:** ElevenLabs über kie.ai (v3, Multilingual v2, Turbo 2.5) antwortete bei jeder Anfrage mit „Internal Error“, auch beim unveränderten Beispiel aus der Doku. Dabei wurden 0 Credits verbraucht.
- **Verwendet:** Google Gemini 3.1 Flash TTS über kie.ai, Deutsch, mit Regieangaben (Szene, Rollenprofil, Tonangaben je Zeile).
- **Casting:** Erzähler Take A = *Algieba*, Take B = *Charon*. Mutter *Sulafat*/*Vindemiatrix*, Tochter *Vindemiatrix*/*Gacrux*, Kind *Leda*/*Puck*. Automatisch gewählt ist Take A (einheitliche Erzählerstimme). **Die Entscheidung per Gehör trifft der Game Director.**
- **Kosten:** 34 Takes = 18,33 Credits, dazu ein Testlauf mit 1,65 Credits und ein abgebrochener Lauf mit 12,14 Credits (20 fertige Takes, deren Auftrags-IDs verloren gingen, weil der Download mit 403 scheiterte; das ist inzwischen behoben). Guthaben danach: 1.840,38 Credits.
- **Status: PLACEHOLDER.** Es sind KI-Stimmen ohne Imitation realer Personen. Vor einer Veröffentlichung Lizenzbedingungen von kie.ai/Google prüfen oder durch eine eigene Aufnahme ersetzen.
- Der API-Schlüssel wird nur als Umgebungsvariable übergeben und liegt **nicht** im Repository.

## 5. Musik – GENESIS Soul Theme

- **Eigene Komposition** (im Skript notiert), D-Moll, **60 BPM = Ruhepuls**, sodass der Herzschlag der Eröffnung das Tempo vorgibt.
- Das Motiv (16 Schläge) endet auf A und kehrt zum D zurück, klanglich also ein Kreis. Harmonien: Dm – B – F – C – Gm – Dm – B – A.
- Instrumente: ausschließlich gesampelte Instrumente der Unreal-Engine-Bibliothek (Harmonix-Plugin, Epic-Engine-Content).
- Höhepunkt 110–113 s: B – C – **D-Dur** (Picardische Terz: aus Moll wird Hoffnung), danach harte Reduktion bei 113,0 s.
- Stimmung gemessen: Abweichung der gesampelten Töne ±13 Cent oder weniger.
- Chor im Jenseits ist noch ein Platzhalter (Streicher + Flöte hoch).
- **Bezug zu GENESIS-009 (Soul Music):** Im Spiel werden Seelenmotive je Person aus dem Soul Seed erzeugt. Ein festes Soul Theme gibt es dort nicht. Das Trailer-Thema ist das **Marken-Thema** und passt ins Format `FGenesisSoulMotif`: Identitätskern = erste zwei Intervalle **+5, +2** (A→D→E), Modus Äolisch, Dauern in Sechzehnteln (4, 4, 2, 6, 4, 4, 4, 4, …). Soll das Spiel das Marken-Thema hörbar zitieren (z. B. als Seelenmotiv der ersten Inkarnation), ist das eine Game-Design-Entscheidung.

| Zeit | Zustand |
|---|---|
| 0–8 | keine Musik, nur Herzschlag |
| 8–27 | Soul Theme als Flüstern: einzelne Pianotöne |
| 27–29,5 | Musik vollständig weg (Geburt) |
| 29,5–39 | warmer Aufbau: Piano + Streicher |
| 39–43 | Stille (Verlust) |
| 43–53 | rhythmisch: Pizzicato-Puls, Bass |
| 53–69 | größere Dynamik: Streicher-Melodie, Hörner |
| 69–80 | stark reduziert, Spieluhr-Motiv ab 76 s |
| 80–88 | fast Stille, endet mit dem letzten Herzschlag |
| 88–94 | absolute Stille (gemessen: −180 dBFS bei 88–90 s) |
| 94–110 | orchestraler, kosmischer Aufbau |
| 110–113 | größter Moment |
| 113–120 | harte Reduktion, ein Herzschlag, Titel, „Kennen wir uns?“ |

## 6. Rechtliches

- Musik: eigene Komposition mit Epic-Engine-Content (Nutzung in Unreal-Projekten laut UE-EULA). Keine Imitation geschützter Filmmusik.
- Stimmen: KI-generiert, keine Nachahmung bekannter Sprecher. Siehe Status in Abschnitt 4.
- Sound Design: prozedural erzeugt, ohne fremde Aufnahmen.

## 7. Definition of Done – Status

| Punkt | Status |
|---|---|
| Komplette Sequencer-Timeline steht | ✅ Struktur (Master, 12 Sub-Sequences, 65 Kameras, Audio, Marker) |
| Alle Shots vorhanden | ⏳ als Kamera-Blocking, **ohne Inhalt** |
| Kameras final | ❌ Blocking |
| Animationen funktionieren | ❌ |
| Keine sichtbaren Placeholder | ❌ |
| Beleuchtung, Materialien, VFX final | ❌ |
| Voice-over final | ⏳ KI-Takes, Casting-Entscheidung offen |
| Sound Design final | ❌ prozeduraler Platzhalter |
| Musik final | ⏳ Soul Theme v1 (Chor fehlt) |
| Mix final | ⏳ Animatic-Mix |
| Titel final | ❌ Text-Andeutung im Animatic |
| Master Render / 16:9 / 9:16 | ❌ |
| Keine kritischen Render-/Engine-Fehler | ✅ für Skripte und Assets bis jetzt |
| Git Commit | ✅ je Arbeitsschritt |

**Gesamtstatus Trailer: ANIMATIC.** Nichts davon ist finales Bildmaterial.

## 8. Offene Entscheidungen für den Game Director

1. **Erzählerstimme:** Take A (Algieba) oder Take B (Charon)? → `Genesis/Saved/TrailerAudio/Mix/VO_Casting_TakeA_vs_TakeB.wav`
2. **MetaHumans (A5):** MetaHuman Creator lädt die Texturdaten nach dem Epic-Login. Das Login muss einmal von Hand im Editor erfolgen.
3. **Fab-Inhalte (Megascans, Hund, Epochen-Sets):** Download braucht das Epic-Konto (manuell) und eine Lizenzprüfung.
4. **Chor:** lizenzierte Chor-Library oder eigene Aufnahme?

## 9. Nächste Produktionsschritte (Shot für Shot)
1. SEQ_001/002 (biologisch, ohne Menschen): Spermium, Eizelle, Mikro-Umgebung in Blender → Unreal, Licht, Volumetrics, Testrender über MRQ.
2. SEQ_011/012: Planet, Kosmos, Titel aus goldenen Fäden.
3. SEQ_009: Jenseits-Umgebung.
4. Menschen-Sequenzen nach MetaHuman-Freigabe.

![Animatic Board](../Media/TRAILER_Animatic_v1_Board.png)
![Animatic Titel](../Media/TRAILER_Animatic_v1_Title.png)
