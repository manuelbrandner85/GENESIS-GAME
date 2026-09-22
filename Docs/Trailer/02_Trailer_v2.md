# 02 – GENESIS Reveal Trailer v2 (In-Engine + Konzept)

**Stand:** 2026-09-22 · **Länge:** 120 s · 24 fps · 1920 × 1080 mit Kinobalken 2,39:1
**Auftrag des Game Directors:** kinoreif, dramatisch, „muss abholen“ – das ganze Prinzip des Spiels vom Spermium bis zum Erschaffen von Welten, mit passender Stimme und Musik.
**Entscheidung (2026-09-22):** Mix aus echten Spielszenen, KI-Konzeptbildern (kie.ai) für die noch nicht spielbaren Lebensphasen und Blender für Kosmos, Welten-Erschaffung und Titel.

Einzige Quelle für Bild und Ton ist die Schnittliste [`Tools/Trailer/trailer_v2_edl.py`](../../Tools/Trailer/trailer_v2_edl.py).

## Dramaturgie

| Akt | Zeit | Inhalt | Quelle |
|---|---|---|---|
| 1 Ursprung | 0–38 s | Schwarz, Herzschlag → Anflug auf die Eizelle, Rennen (Verfolger- und Ich-Perspektive), Befruchtung (Weißblitz), erste Woche im Zeitraffer (2 → 4 → 8 Zellen, Blastozyste schlüpft), Wehen, **2 s Stille**, Licht, erster Schrei, „Da ist er!“, die Mutter: „Oh … oh mein Gott … hallo …“ (lippensynchron) | **Spiel** (Genesis.exe) |
| 2 Leben | 38–64 s | Kindheit im Gras, hochgehoben, leerer Hundekorb („Du wirst verlieren“), Regenfenster, Freunde, erste Liebe, Weggabelung („Entscheidungen …“), Krankenhaus, Ring, Babyhand, Mittelalter, Zukunft, Alter am Meer | Konzeptbilder |
| 3 Alter & Tod | 64–78 s | Foto, Rückblende Kindheit, Sterbebett, das Auge schließt sich, letzter Herzschlag, **2 s absolute Stille** | Konzeptbilder |
| 4 Jenseits & Wiedergeburt | 78–94 s | Geist neben dem eigenen Körper (Stimmen fern), Portal aus Licht, Jenseits-Landschaft, der Hund kehrt zurück, ein Lichtpunkt – eine neue Zelle, neues Kind in anderer Kultur | Konzept + Blender + Spiel |
| 5 Schöpfung & Titel | 94–120 s | Rückzug bis zur Galaxie, Hände mit neugeborenem Planeten, Welten-Erschaffung im Zeitraffer (Kontinente, Wolken, Städte), harter Abriss, ein Herzschlag, **GENESIS**, „Und jede Entscheidung … hinterlässt ein Echo.“, Kinderstimme: „Kennen wir uns?“ | Blender |

## Quellen und Ehrlichkeit
- **Spielszenen** sind echte In-Engine-Aufnahmen der gebauten Spielfassung (`Build/Windows/Genesis.exe`, Development) mit festem Zeitschritt (`-benchmark -fps=24 -dumpmovie`), ohne HUD (`showhud`), offscreen gerendert (`-RenderOffscreen`), damit keine Eingabe die Aufnahme stört. Rennen mit Autopilot (`genesis.Race.AutoPilot 1`), Geburt direkt in `L_GEN_Birth` (Kind gezeugt, 273 Tage übersprungen, Wehen vorgespult, danach Haut an Haut und Blicksuche).
- **Konzeptbilder** (Lebensphasen 6 bis 86 Jahre, Jenseits) sind KI-generiert und **keine Spielaufnahmen**. Der Abspann sagt das: „In-Engine-Aufnahmen aus GENESIS (Entwicklungsstand) · Lebensphasen: Konzeptbilder“. Manifest: [`Concept_Shots_Manifest.json`](Concept_Shots_Manifest.json).
- Die Hauptfigur bleibt über alle Alter erkennbar: ein Referenzbild (Kind, Jugendlicher, Mann, Greis mit Narbe durch die linke Augenbraue) steuert alle Schlüsselbilder. Das Kind ist ein Junge – wie das Kind der aufgenommenen Geburt („Da ist er! … ein Junge“).
- **Blender:** Portal (goldene Fäden → Lichtkreis), Galaxie (≈ 600 000 Sterne, Staubbahnen, Haufen, Sternentstehungsgebiete), Planet (Meeresspiegel sinkt → Kontinente, Wolken, Atmosphäre im Gegenlicht, Städte auf der Nachtseite), Titel (Faden-Kreis öffnet sich zur Linie, GENESIS in Gold). Skript: `Tools/Trailer/Blender/build_cosmic_shots.py`.

## Ton
- **Musik:** Suno (kie.ai) „MX_Trailer_Epic_A“ – aus 4 Fassungen nach Lautheitsverlauf gewählt: 15 s fast Stille (Klavier, Herzschlag), stetiger Aufbau, Höhepunkt bei 2:34, harter Abriss bei 2:36 (−9 dB) → liegt exakt auf dem Schnitt vor dem Titel. Stille vor der Geburt, nach dem Tod und vor dem Titel.
- **Erzähler:** Gemini TTS „Algieba“ (dieselbe Stimme wie im Prolog des Spiels). Neue Sätze: „Millionen machen sich auf den Weg.“, „Nur einer kommt an.“, „Aus einer Zelle werden zwei.“, „Aus zweien … ein Mensch.“, „Und jede Entscheidung … hinterlässt ein Echo.“
- **Originaltöne des Spiels:** erster Schrei, Hebamme, Mutter, Raumklang des Kreißsaals.
- Mischung: Musik weicht unter der Stimme (−6 dB, unter Originaltönen −9 dB), Stille-Momente gemessen −180 dBFS, Limiter −1 dBFS. Skript: `Tools/Trailer/Audio/build_trailer_v2_audio.py`.

## Kosten kie.ai (2026-09-22)
| Posten | Credits |
|---|---|
| 4 Musikfassungen + 12 Stimm-Takes | 30,76 |
| Referenzbild Hauptfigur | 18 |
| 20 Schlüsselbilder (Nano Banana Pro 2K, je 18) | 360 |
| 2 Videos Stufe L (720p, 3 s, je 42) + 4 Videos Stufe H (1080p, 4 s, je 72) + Test | 72 + 42 + 342 |
| **Summe** | **≈ 870** (Guthaben 1.628 → 758) |

## Pipeline
1. Aufnahme: `Build/Windows/Genesis.exe … -benchmark -fps=24 -dumpmovie -RenderOffscreen` → `Genesis/Saved/Trailer/Capture/Run*`
2. Musik/Stimme: `Tools/Trailer/Audio/generate_trailer_audio_kie.py --music --vo`
3. Konzeptbilder: `Tools/Trailer/Audio/generate_concept_shots.py`
4. Blender: `blender -b --python Tools/Trailer/Blender/build_cosmic_shots.py -- all`
5. Ton: `build_trailer_v2_audio.py` · 6. Schnitt/Render: `blender -b --python Tools/Trailer/Blender/build_trailer_v2_edit.py -- [--master]`

## Status (2026-09-22)
- **Fertig gerendert** (`Genesis/Saved/Trailer/v2/`, Web-Fassungen zusätzlich in `Docs/Media/Trailer/`, LFS):
  | Fassung | Datei | Format | Länge |
  |---|---|---|---|
  | Haupttrailer (YouTube) | `GENESIS_Trailer_v2.mp4` | 1920 × 1080, Kinobalken 2,39:1 | 120 s |
  | Master | `GENESIS_Trailer_v2_Master_ProRes422HQ.mov` (2,3 GB, nur lokal) | 1920 × 1080 ProRes 422 HQ, PCM | 120 s |
  | TikTok / Reels / Shorts | `GENESIS_Trailer_v2_9x16.mp4` | 1080 × 1920 | 46,4 s |
  | Instagram / Facebook Feed | `GENESIS_Trailer_v2_1x1.mp4` | 1080 × 1080 | 33 s |
- **Social-Fassungen sind eigene Schnitte**, kein Zuschnitt: kürzere Einstellungen, eigener Musikschnitt (Aufbau → Abriss genau vor dem Titel), jede Einstellung neu ausgerichtet (Motiv in die Bildmitte, `focus` in `Tools/Trailer/trailer_v2_9x16_edl.py` / `trailer_v2_1x1_edl.py`), Titel eigens im Hoch- bzw. Quadratformat in Blender gerendert (`GENESIS_FORMAT=9x16|1x1`). Aufruf: `$env:GENESIS_EDL="trailer_v2_9x16_edl"` vor Ton- und Schnittskript.
- Geprüft: Übergänge (Weißblitz Befruchtung/Geburt, Überblendungen), Kinobalken, Titel in voller Auflösung, Stille-Momente −180 dBFS, keine Stimmüberlappung, Mutter lippensynchron (Zeile beginnt Frame 1882, „oh mein Gott“ ab 3,2 s, „hallo“ bei Frame ≈ 2060).
- Behobene Fehler während der Produktion: Startmenü mitten in der Aufnahme (Eingabe ins Aufnahmefenster → Aufnahme offscreen), Aufnahme endete zu früh (Verzögerungsbefehl zählt Echtzeit), Befehlszeile zu lang, Geburtskamera verlässt den Kanal erst ~42 s nach „geboren“, Galaxie-Sterne als Scheiben / Warp-Streifen, Stadtlichter als Flecken, Streiflicht statt Fog-Glow (Blender-5.2-API).
- Offen: Abnahme per Ohr und Auge durch den Game Director (Mischung, Erzählerstimme, Musikwahl).
- Nicht versioniert (Projektregel `ArtSource/Generated/`): Konzeptbilder/-videos, Musik, Stimm-Takes und Aufnahmen liegen lokal; Prompts und Auftrags-IDs stehen in den Manifesten.

## Bekannte Grenzen
- Master in 1080p (nicht 4K): Spielaufnahmen und Konzeptvideos liegen in 1080p/720p vor; hochskalieren wäre keine echte 4K-Qualität.
- Konzeptbilder sind Platzhalter für die spätere echte Umsetzung im Spiel (MetaHumans in allen Altersstufen).
- Suno- und KI-Bild-Rechte vor Veröffentlichung nach den Nutzungsbedingungen des Tarifs prüfen.
