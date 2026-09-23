# 34 – Neun Monate: die Schwangerschaft von innen

*GENESIS-044. Woche 5 bis zur Geburt. Game Director: „Das Embryo ist nur ein Teil des ganzen neunmonatigen
Prozesses – es soll der gesamte Schwangerschaftsverlauf sein, auch die Töne und dumpfe Sprache, die Wahrnehmung des
Kindes." Masterprompt (Doc 00), Phase 2 und Signature Moments 4 („die erste Wahrnehmung von Geräuschen") und 5
(„Licht durch den Mutterleib").*

| Bereich | Stand |
|---|---|
| Recherche: Wochentafel mit Quellen | **fertig** (unten) |
| Teil 1a: Fetalzeit als Simulation – Wachstum, Herz, Sinne, Verhalten, Hören nach Recherche (Tests `Genesis.Body.Fetal.*`, `Genesis.Audio.Core.FirstSound`) | **BETA** |
| Teil 1b: Zeitablauf in Momenten, Tagesrhythmus der Mutter, Szene aus Sicht des Kindes | **PLAN** – nach der Bestandsaufnahme (Doc 35) |
| Teil 2: das Kind sichtbar wachsen lassen (Woche 6–40 in Blender, fotoreal) | **PLAN** |
| Teil 3: selbst bewegen, sich Stimmen zuwenden, die Mutter reagiert | **PLAN** |

## Zählweise

Kliniken zählen die Schwangerschaft ab der letzten Regel (**SSW**, Gestationsalter). Das Spiel rechnet ab der
Befruchtung (Weltuhr, `GetGestationalWeeks`): **Wochen nach Befruchtung = SSW − 2.** Die Quellen unten nennen SSW;
die Tafel führt beides.

## Die Wochentafel (recherchiert)

| SSW | nach Befr. | Körper | Bewegung | Sinne |
|---|---|---|---|---|
| 6 | 4 | 4–5 mm, Herz ~90–110/min | Rücken und Nacken beugen sich | – |
| 7–8 | 5–6 | ~10–16 mm | erste gerade noch erkennbare Bewegungen (7,5), Schreckbewegung, Ganzkörperbewegungen (8) | erste Tastrezeptoren um den Mund |
| 9–10 | 7–8 | ~23–31 mm, **Herz am schnellsten: 150–170/min** | Schluckauf, Arme und Beine einzeln, Kopf dreht sich, Hand zum Gesicht, Strecken, Gähnen (10) | Lider verwachsen (bis ~26) |
| 11–12 | 9–10 | ~41–53 mm | Mund öffnet sich, Finger saugen; **ab 12 schluckt es Fruchtwasser**; tritt, krümmt die Zehen, reagiert auf Berührung | Geschmacksknospen |
| 14 | 12 | 90 g | Mehrgebärende spüren erste Bewegungen | Tasten am ganzen Körper |
| 14–18 | 12–16 | 90–222 g | deutlicher Tagesrhythmus der Aktivität | – |
| **19** | 17 | 272 g, ~25 cm (Scheitel–Ferse, SSW 20) | Erstgebärende spüren um SSW 20 die ersten Bewegungen | **erste Reaktion auf einen Ton (500 Hz)** |
| 21 | 19 | 398 g | regelmäßiger Bewegungsplan | – |
| 23–24 | 21–22 | 565–665 g | Schreckreflex bei der Hälfte (24) | Thalamus-Fasern wachsen in die Hirnrinde (23–24): **erst ab hier ist bewusstes Wahrnehmen strukturell möglich** |
| 25–27 | 23–25 | 778–1039 g | Ruhe- und Aktivitätsphasen bilden sich (25–30) | Hören: 96 % reagieren auf 250 und 500 Hz (27), keiner auf 1000 und 3000 Hz; Hörschwelle ~40 dB (27–29) |
| **26–28** | 24–26 | 902–1189 g | Schreckreflex bei allen (28); ~20 % in Beckenendlage (28) | **Augen öffnen sich** (ab 26, ganz ab 28) |
| 31 | 29 | 1707 g | – | Pupillen verengen und weiten sich – Licht wird wahrgenommen |
| 32 | 30 | 1901 g | Schlaf- und Wachzustände deutlich (≈90 % Schlaf); 7–15 % Beckenendlage | – |
| 33–35 | 31–33 | 2103–2527 g | – | 100 % reagieren auf 1000 Hz (33) und 3000 Hz (35) |
| 34 | 32 | 2312 g | wendet den Kopf zu gesichtsähnlichen Lichtmustern durch die Bauchdecke | – |
| 36–38 | 34–36 | 2745–3186 g | vier Verhaltenszustände: ruhiger Schlaf 24 %, aktiver Schlaf 65 %, aktiv wach 11 % | – |
| 40 | 38 | 3617 g, ~51 cm, Herz ~130–140/min | 3–4 % Beckenendlage | Hörschwelle ~13,5 dB (40–42) |

Gewicht: WHO-Wachstumskurve, 50. Perzentile, SSW 14–40 (Tabelle 11). Scheitel-Steiß-Länge bis SSW 14: Robinson &
Fleming 1975.

## Was das Kind hört

- Der Mutterleib ist **nie still**: Grundpegel ≥ 28 dB, beim Singen der Mutter bis 84 dB. Unter 100 Hz dominiert
  tiefes Rauschen (60–85 dB), darüber < 60 dB, oberhalb 500 Hz ~40 dB.
- Es dominieren die Laute der Mutter: **Herzschlag**, Atmung, **Stimme**, Darmgeräusche, Körperbewegung.
- **Die Stimme der Mutter** kommt kaum gedämpft an: Sie läuft durch ihren eigenen Körper. Stimmen von außen dagegen
  sind ab ~60 dB hörbar und oberhalb 250–500 Hz gedämpft. Der Bauch wirkt als Tiefpass: über 600–1000 Hz ~30 dB
  weniger, tiefe Töne (125 Hz) kommen sogar leicht verstärkt an.
- Das Kind hört über **Knochenleitung**, nicht über Gehörgang und Mittelohr (Fruchtwasser).
- Reihenfolge der Tonhöhen: zuerst 500 Hz (SSW 19), dann tiefer (100, 250 Hz), zuletzt höher (1000, 3000 Hz, SSW
  33–35). Die nötige Lautstärke sinkt dabei um 20–30 dB.

## Was das Kind sieht

- Bis SSW ~26 sind die Lider verwachsen. Danach öffnen sich die Augen; ab SSW 31 reagieren die Pupillen.
- Durch die Bauchdecke dringt vor allem **rotes** Licht. Gemessen mit einer 650-nm-Quelle: bei 15 / 20 / 30 mm
  Gewebe noch 36 / 24 / 16 lx im Mutterleib. Das Kind sieht keine Bilder, sondern Helligkeit und diffuses Leuchten;
  mit SSW 34 wendet es den Kopf zu gesichtsähnlichen Lichtmustern.

## Schlaf und Wachen

Bis SSW 32 wechseln Ruhe (58 %) und Aktivität (42 %). Danach deutliche Zustände, fast 90 % der Zeit Schlaf; ab SSW
36–38 vier Zustände wie beim Neugeborenen (ruhiger Schlaf, aktiver Schlaf, ruhig wach, aktiv wach). In aktiven
Phasen: Glieder- und schnelle Augenbewegungen, Atembewegungen, Schluckauf. Stoffe im Blut (Adenosin, Pregnanolon,
Prostaglandin D2) halten das Kind überwiegend in einem schlafähnlichen Zustand.

## Folgerungen für das Spiel

1. **Die Wahrnehmung wächst mit dem Körper.** Vor SSW ~19 hört das Kind nichts; der Spieler erlebt diese Wochen von
   außen (die Szene). Ab SSW 19 dringt zuerst ein mittlerer Ton durch, dann das Tiefe – der Herzschlag der Mutter,
   ihr Rauschen –, erst gegen Ende die Höhen. Das ist Signature Moment 4.
2. **Bewusstes Erleben erst ab SSW ~24** (Thalamus–Rinde). Davor reagiert der Körper (Reflexe, Bewegungen), das
   Erleben ist gedämpft und bruchstückhaft.
3. **Licht durch den Bauch** (Signature Moment 5) erst mit offenen Augen ab SSW 26–28, rot, ohne Formen, abhängig
   davon, wo die Mutter gerade ist (draußen am Tag, drinnen, nachts).
4. **Die Mutter lebt ihren Tag**, und das Kind spürt es: Ihr Puls, ihre Stimme, ihr Essen (Geschmack des
   Fruchtwassers ab SSW 12), ihre Bewegung, Tag und Nacht.
5. **Das Kind schläft meistens** – und hat trotzdem einen eigenen Rhythmus: Schluckauf, Tritte, Daumen, Atembewegungen.

## Quellen

- Bewegungen: de Vries, Visser, Prechtl 1982, *The emergence of fetal behaviour I*
  ([PubMed](https://pubmed.ncbi.nlm.nih.gov/7169027/)); Überblick
  [Wikipedia: Fetal movement](https://en.wikipedia.org/wiki/Fetal_movement); Wahrnehmung durch die Mutter
  ([PubMed](https://pubmed.ncbi.nlm.nih.gov/35779269/)).
- Hören: Hepper & Shahidullah 1994, *Development of fetal hearing*
  ([Semantic Scholar](https://www.semanticscholar.org/paper/Development-of-fetal-hearing-Hepper-Shahidullah/7825aa73b85b243efcb709f8cbab5a5ae84b271b));
  Reaktion auf die Stimme der Mutter ([PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC3858412/)); Schallwelt im
  Mutterleib ([Frontiers](https://www.frontiersin.org/journals/neuroscience/articles/10.3389/fnins.2014.00381/full),
  [PMC: pränatale Musik](https://pmc.ncbi.nlm.nih.gov/articles/PMC3759965/),
  [PubMed: Sound levels in the human uterus](https://pubmed.ncbi.nlm.nih.gov/1635729/)).
- Licht und Sehen: Reid et al. 2017, *The Human Fetus Preferentially Engages with Face-like Visual Stimuli*
  ([PubMed](https://pubmed.ncbi.nlm.nih.gov/28602654/)); Augenbewegungen auf Lichtreiz
  ([PMC](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC7428469/)).
- Schlaf und Zustände: Nijhuis et al. 1982 ([PubMed](https://pubmed.ncbi.nlm.nih.gov/7094856/)); Zustände und
  Anteile ([Nature Sci. Rep.](https://www.nature.com/articles/s41598-022-07476-x),
  [PMC: Schlaf und Sinne](https://pmc.ncbi.nlm.nih.gov/articles/PMC11699341/)).
- Bewusstsein: RCOG, *Fetal Awareness* ([Überblick](https://www.rcog.org.uk/guidance/browse-all-guidance/other-guidelines-and-reports/fetal-awareness-updated-review-of-research-and-recommendations-for-practice/),
  [Evidenz 2022](https://www.rcog.org.uk/media/gdtnncdk/rcog-fetal-awareness-evidence-review-dec-2022.pdf));
  Thalamus-Fasern ([PMC: The fetal pain paradox](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC10072285/)).
- Wachstum: WHO-Wachstumskurven, Kiserud et al. 2017
  ([PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC5261648/)); Scheitel-Steiß-Länge
  ([Wikipedia](https://en.wikipedia.org/wiki/Crown-rump_length)); Herzfrequenz
  ([PMC: Referenzwerte 6–10 Wochen](https://www.ncbi.nlm.nih.gov/pmc/articles/PMC7488996/),
  [PubMed](https://pubmed.ncbi.nlm.nih.gov/7658510/)).
- Geschmack: Mennella et al. 2001, *Prenatal and postnatal flavor learning*
  ([PubMed](https://pubmed.ncbi.nlm.nih.gov/11389286/)).
- Kindslage: Kirchengast 2023 ([Wiley](https://onlinelibrary.wiley.com/doi/full/10.1002/ajhb.23880)),
  [Medscape: Breech Presentation](https://emedicine.medscape.com/article/262159-overview).

## Teil 1a – die Fetalzeit als Simulation (umgesetzt)

- `GenesisFetalTypes.h` / `GenesisFetalLogic` (Plugin GenesisBody): Referenztafel oben als Daten
  (`UGenesisFetalSettings`, Project Settings → Genesis → Fetal), daraus je SSW Länge, Gewicht, Herzfrequenz, Hörband und
  -empfindlichkeit, Augen, Lichtwahrnehmung, Tasten, bewusster Zugang, Kindslage. Dazu ein Verhaltensmodell
  (Ruhe/Aktivität, ab SSW 36 vier Zustände; Schreck, Tritte, Strecken, Kopfdrehen, Gähnen, Daumen, Schlucken,
  Atembewegungen, Schluckauf in Serien), deterministisch aus einem Keim.
- Der Körper wächst jetzt nach der Tafel (vorher Näherungskurven); die Herzfrequenz folgt ihr.
- **Hören neu nach der Recherche:** vorher hörte das Ungeborene von Anfang an den Körper der Mutter mit halber
  Lautstärke. Jetzt: Stille bis SSW 19, dann ein Band von 384–624 Hz, schwach (0,06); SSW 28: 87–800 Hz, 0,57. Der
  Hörfilter hat dafür einen Hochpass bekommen (Stimmen und Klang der Orte).
- Geburt zum Termin: 38 Wochen nach Befruchtung = SSW 40 (vorher 39 = SSW 41).
- Sinneszeitplan des Körpers an die Recherche angeglichen (Hören, Sehen, Schmecken, Tasten).
- Gemessen im Test: Herz am schnellsten 170/min in SSW 9, zum Termin 135/min; SSW 28 Ruhe 59 % / Aktivität 41 %;
  SSW 38 ruhiger Schlaf 24 %, aktiver Schlaf 65 %, wach 11 %. Ein alter Test verlangte volles Hören schon in SSW 28 –
  nach Hepper & Shahidullah falsch (1000 Hz erst ab SSW 33) und angepasst.
