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

## Teil 1b – der Mutterleib aus Sicht des Kindes (umgesetzt)

Die Schwangerschaft ist kein schwarzer Zeitraffer mehr, sondern eine eigene Karte `L_GEN_Mutterleib`: Die Kamera
**ist** das Kind. Was man sieht und hört, rechnet sich aus seinen Sinnen (Teil 1a) und dem Tag der Mutter.

**Der Tag der Mutter** (`GenesisMotherDay`, Plugin GenesisPeople): Aufstehen ~7 Uhr, Mahlzeiten, Arbeit sitzend,
ein Spaziergang am Nachmittag (an 7 von 10 Tagen), abends spricht sie mit dem Bauch (ab SSW 16), manchmal Musik,
Schlaf ab ~23 Uhr. Jeder Tag etwas anders, aber reproduzierbar. Daraus: Tätigkeit, Puls, Stimme, Verdauung, Wiegen –
und das Licht auf dem Bauch (draußen Tageslicht, drinnen Fenster oder Lampe, nachts nichts).

**Licht im Mutterleib:** Bauchlicht × Kleidung (15 %, Annahme) × Durchlass der Bauchdecke. Den Durchlass gibt es
gemessen nur beim **Schaf** (Parraguez et al. 1998): 0,2 % zur Mitte der Tragzeit, gut 5 % kurz vor dem Termin, mit
dem Tagesgang des Sonnenlichts. Für den Menschen fehlt eine direkte Messung; Reid et al. 2017 kommen mit rotem Licht
auf eine ähnliche Größenordnung. Das Spiel nimmt die Schafwerte logarithmisch zwischen SSW 20 und 40 und sagt das hier
offen. Ergebnis: draußen am Nachmittag in SSW 31 rund 10 lx, im Zimmer unter 1 lx, nachts 0.

**Puls der Mutter:** in der Schwangerschaft bis +15/min über dem Ruhepuls (Sanghavi & Rutherford 2014), dazu Schlaf
und Bewegung. Das Kind hört ihn als Herzschlag im Klang des Ortes.

**Die Momente** (Regie, `GenesisSliceLogic::DefaultGestationMoments`): Zwischen den Momenten läuft die Zeit im
Zeitraffer (16 s), im Moment 8-mal schneller als Echtzeit. Ein Moment sucht sich in seiner Woche den Tag und die Stunde,
zu der die Situation wirklich eintritt (Spaziergang, Bauchgespräch) – die Welt wartet nicht auf den Spieler.

| SSW | Situation | Kapitelzeile | Was das Kind erlebt |
|---|---|---|---|
| 19 | Bauchgespräch | Das Hören beginnt | Dunkel; ihre Stimme als schmales, dumpfes Band um 500 Hz |
| 20,5 | Bauchgespräch | Sie spürt die ersten Bewegungen | Der erste Tritt, den sie spürt – sie antwortet |
| 24 | 3 Uhr | Nachts: ihr Herz, ihr Atem | Stille außer ihrem Körper; Eigengrau |
| 28 | Spaziergang | Die Lider öffnen sich | Erstes rotes Licht, Wiegen im Schritt |
| 31 | Spaziergang | Licht durch den Bauch | Pupillen reagieren; Nabelschnur im Gegenlicht |
| 34 | 20 Uhr | Das Hören wird feiner | 1000 Hz hörbar, Stimmen klarer |
| 37 | Bauchgespräch | Es wird eng | „Nicht mehr lange, dann sehen wir uns.“ |

Zusammen rund 6 Minuten (eine Stellschraube: `GestationMoments[].Seconds`, `GestationTravelSeconds`).

**Was man sieht** (`AGenesisWombScene`):
- Die Höhle nach den Referenzen (Doc 36): Fruchtblase an der Gebärmutterwand, Plazenta hinten oben mit erhabenen
  Gefäßen, die Nabelschnur in Schlingen zum Nabel, drei Gefäße spiralig in der Wharton-Sulze. Sie wächst mit der Woche
  (Innenradius 1,5 cm in SSW 8 bis 14,5 cm am Termin).
- Das Licht kommt nicht von einer Lampe, sondern durch die Bauchwand: Die vordere Wand leuchtet tiefrot, fleckig
  (die Bauchdecke ist nicht überall gleich dick), hinten fast nichts. Gewebe davor scheint im Gegenlicht durch.
- Wahrnehmung statt Kamera: Das dunkeladaptierte Auge gleicht die Lichtmenge aus; wie hell es wirkt, folgt dem Licht
  (logarithmisch), den Lidern und der Pupille. Vor SSW 26 nur diffuses Leuchten hinter den Lidern, danach das Grobe
  wenige Zentimeter vor dem Gesicht, nie scharf. Ohne Licht kein Schwarz, sondern **Eigengrau** – das schwache,
  rauschende Dunkel, das man auch mit geschlossenen Augen sieht. Vor dem bewussten Erleben (SSW 23–26) ist alles
  gedämpfter.
- Eigene Bewegungen als Ruck der Kamera (Schreck, Tritt, Strecken, Schluckauf), ihr Gehen als Wiegen, ihr Atem.

**Was man hört:** ihr Herz, ihr Blut, ihr Darm nach dem Essen – durch den Hörfilter des Kindes (Band und
Empfindlichkeit nach Woche). Ihre Stimme ab SSW 16, wenn sie spricht; nach dem ersten gespürten Tritt ihr
„Hallo, du da drin“, ab SSW 36 „Nicht mehr lange“.

**Unterwegs gefunden und behoben:**
- Die Nabelschnur stand rein schwarz im Bild. Ursache: Lumen gibt Teilen unter ~10 Einheiten keine Oberflächenkarten,
  bei 1 cm = 1 Einheit war die Schnur 4–5 Einheiten groß. Die Szene rechnet jetzt 10 Einheiten je cm; Fokus und Blende
  sind mitskaliert, damit die Unschärfe gleich bleibt.
- Auch danach blieb die Schnur schwarz – physikalisch halb richtig (ihre sichtbare Seite zeigt zur dunklen Rückwand),
  aber es fehlte das Licht, das durch die Sulze dringt. Unreal rechnet Durchlicht nur für Lampen, nicht für eine
  leuchtende Wand; das Gewebematerial gibt es jetzt nach Blickrichtung und Randdicke selbst aus.
- Die Belichtung war physikalisch richtig, aber die Momente im Zimmer waren fast schwarz: 0,4 lx sind 25-mal weniger als
  10 lx, und nur die halbe Anpassung war eingerechnet. Jetzt gleicht das Auge ganz aus, die Wahrnehmung setzt die
  Helligkeit.

Quellen zusätzlich zu oben: Parraguez VH et al. 1998, *Diurnal changes in light intensity inside the pregnant uterus
in sheep*, Anim Reprod Sci 52:123–130 ([DOI](https://doi.org/10.1016/s0378-4320(98)00094-3)); Sanghavi M,
Rutherford JD 2014, *Cardiovascular physiology of pregnancy*, Circulation 130:1003–1008
([DOI](https://doi.org/10.1161/CIRCULATIONAHA.114.009029)).

**Geprüft im gebauten Spiel** (Autopilot, ein Rennen verloren und neu gestartet): Schwangerschaft von 206,7 s bis
574,7 s (368 s), alle sieben Momente, danach die Geburt. Aus dem Log: SSW 19,1 um 21:09 – sie ruht und spricht, Hörband
362–561 Hz; SSW 20,2 spürt sie den ersten Tritt und antwortet; SSW 24 um 3 Uhr schläft sie (0 lx); SSW 28,4 um 17:30 geht
sie spazieren (6,6 lx im Mutterleib), SSW 31,1 um 16:48 ebenso (15,5 lx); SSW 37,1 „Nicht mehr lange“. Leistung in
`L_GEN_Mutterleib` (1280×720): Frame 5,2 ms, GPU 3,9 ms, 58 Draws.

Danach noch korrigiert: Ihre Stimme kam in SSW 19 mit Lautstärke 0,07 – am Lautsprecher praktisch Stille, der
Signature Moment wäre verloren. Wie das Auge an die Lichtmenge passt sich jetzt das Ohr an seine Schwelle an
(`GenesisAudioCoreLogic::UnbornPresentationGain`: was gehört wird, mindestens 0,25, Abstufung bleibt). Ein schwarzer
Strich auf der Nabelschnur war ein Nabelgefäß, das in einer engen Schlinge durch die Oberfläche stach; die Gefäße
liegen jetzt tiefer und zeigen sich stattdessen als weiche Spiralschatten im Durchlicht (~0,2 Windungen je cm wie im
Mittel echter Nabelschnüre).

![SSW 28: Die Lider öffnen sich](Media/GENESIS-044_Mutterleib_SSW28.png)
![SSW 37: Es wird eng](Media/GENESIS-044_Mutterleib_SSW37.png)
