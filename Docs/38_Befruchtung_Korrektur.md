# 38 – Befruchtung: Abgleich mit der Forschung und Korrekturen

*Auftrag des Game Directors (2026-09-23): „Recherchiere auch die Befruchtung, dass dies korrigiert wird.“*
Grundlage: Prüfung des Spiels (Plugin GenesisConception, GenesisEmbryo, Blender/Unreal-Skripte, Texte) gegen die
Literatur. Quellen über PubMed; wo nur Mausdaten vorliegen, steht es dabei.

## Was schon stimmt

Maße des Spermiums (Kopf 4,6 × 2,9 µm, 60 µm lang), Größe der Eizelle (110 µm Ooplasma), Schwimmgeschwindigkeiten
und Geißelschlag gegen CASA-Messungen, Rheotaxis (Miki & Clapham 2013), Zeiten für Vorkerne und erste Teilung.

## Was falsch ist – nach Gewicht

| Prio | Im Spiel | In Wirklichkeit | Korrektur |
|---|---|---|---|
| **Kritisch** | 6.000 Zellen starten als Pulk vor der Eizelle | ~18 h nach der Besamung finden sich in **beiden** Eileitern zusammen im Median **251** Spermien (79–1.386; Williams 1993, DOI 10.1093/oxfordjournals.humrep.a137975); sie bleiben bis 5 Tage befruchtungsfähig und kommen verteilt an (Wilcox 1995) | 50–150 Zellen im Abschnitt, nach und nach eintreffend; Rennen neu abstimmen |
| **Kritisch** | „Nur einer kommt an.“, „Eine andere Zelle war schneller.“, „Platz x von 6000“ | Viele kommen an; bei der Maus erreichen in fast der Hälfte der Fälle 2–9 Spermien den Spalt unter der Zona, **eines verschmilzt** (Dubois 2025, DOI 10.1038/s44319-025-00670-8). Entscheidend ist der richtige Reifezustand zur richtigen Zeit (Kapazitation nur 50–240 min, Cohen-Dayag 1995, DOI 10.1073/pnas.92.24.11039), nicht die Geschwindigkeit | Texte: „Hunderte kommen an. Nur eine verschmilzt.“ / „Eine andere Zelle ist verschmolzen.“; Platzanzeige ersetzen; Trailer-Satz neu (erst nach Rückfrage, kie.ai) |
| **Kritisch** | Zona in 12–40 s durchquert, Verschmelzung im selben Augenblick | Maus, Lebendaufnahme: **~13 min** durch die Zona (Jin 2011, DOI 10.1073/pnas.1018202108), danach im Mittel **16 ± 6 min** im perivitellinen Spalt bis zur Verschmelzung (Dubois 2025) | Neuer Schritt „im Spalt unter der Zona“; Zeitsprünge sichtbar mit Uhr statt heimlicher 20–60-facher Beschleunigung |
| **Kritisch** | Cumulus 118 µm Radius, ~2.600 Zellen | Um die frisch gesprungene Eizelle ~**20.000** Cumuluszellen, die Masse misst **Millimeter** (Ortiz 1982, zitiert PMC3955418) | Dichte Corona bleibt, außen lockerer Cumulus über mehrere hundert µm; Eileiter-Geometrie anpassen |
| Wichtig | 53 % hyperaktiviert, Lockstoff wirkt auch ohne Kapazitation | Nur **2–14 %** jeweils kapazitiert, einmalig und vorübergehend; nur sie folgen Lockstoffen (Cohen-Dayag 1995) | Kapazitation als eigener Zustand (~10 %), Lockstoff nur für diese |
| Wichtig | Akrosomreaktion erst an der Zona | Maus: bei 12 von 13 erfolgreichen Spermien schon **im Cumulus** (Jin 2011); beim Menschen umstritten | Beginn schon im Cumulus erlauben |
| Wichtig | Nach ~1,8 s werden alle Anhaftenden abgewiesen | Zona-Block über Ovastacin/ZP2 (Burkart 2012), Membranblock über JUNO-Verlust (Bianchi 2014) – **Minuten**; überzählige Spermien bleiben im Spalt liegen (Dubois 2025) | Schutz über Minuten, Überzählige bleiben sichtbar |
| Wichtig | Radial, Kopf voraus | Spermien liegen flach an und dringen schräg ein (Drobnis 1988); Verschmelzung am Äquatorialsegment, seitlich am Kopf (Satouh 2012) | Schräger Eintritt, tangentiale Verschmelzung |
| Wichtig | Nur ein Polkörper | Der 2. Polkörper erscheint nach ~3,5 h | 2. Polkörper ergänzen |
| Klein | Zona 14 µm | 16,7–17,7 µm (Valeri 2011) | 17 µm |
| Klein | „bohrt sich durch (Enzyme)“ | eher mechanisch, Hebelschläge (Bedford 1998; Drobnis 1988) | Wortlaut |
| Klein | Anzeige „hpi“, Uhr ab Verschmelzung | hpi = Stunden nach Besamung | Beschriftung korrigieren |

Belege schwach: Die Zeiten für Zona, Spalt und Schutzmechanismen stammen überwiegend von der Maus; die Zahl im
menschlichen Eileiter beruht auf einer Studie mit 10 Frauen.

## Reihenfolge der Umsetzung (GENESIS-047)

1. Texte (Vorspann-Untertitel, Anzeige, Niederlage) – schnell und am sichtbarsten.
2. Ablauf: Schritt im perivitellinen Spalt, sichtbare Zeitsprünge, Schutz über Minuten, Überzählige bleiben liegen.
3. Weniger Zellen, eigener Kapazitationszustand, Ankunft über Zeit; Rennen neu messen.
4. Cumulus in echter Größe, Eileiter-Geometrie.
5. Details: 2. Polkörper, schräges Eindringen, Zona 17 µm, „hpi“.

Weitere Quellen: Suarez & Pacey 2006 (DOI 10.1093/humupd/dmi047), Burkart 2012 (DOI 10.1083/jcb.201112094),
Bianchi 2014 (DOI 10.1038/nature13203), Inoue 2005 (DOI 10.1038/nature03362), Satouh 2012 (DOI 10.1242/jcs.100867),
Drobnis 1988 (DOI 10.1016/0012-1606(88)90437-x), Bedford 1998 (DOI 10.1095/biolreprod59.6.1275), Valeri 2011
(DOI 10.1007/s10815-011-9555-3), Miki & Clapham 2013 (DOI 10.1016/j.cub.2013.02.007), Wilcox 1995
(DOI 10.1056/NEJM199512073332301).

## Umsetzung GENESIS-047 Teil 1 (2026-09-23)

| Punkt | Umgesetzt | Gemessen (Tests) |
|---|---|---|
| Zahl der Zellen | 150 im Abschnitt, Feld reicht weit zurück, Ankunft nach und nach | je Rennen 4–10 an der Zona, 1–8 im Spalt |
| Kapazitation | eigener Zustand (`bCapacitated`, 10 %); nur diese folgen dem Lockstoff, hyperaktivieren, binden | 8,9 % im Schwarm; ohne Kapazitation keine Bindung |
| Akrosomreaktion | kann im Cumulus beginnen (Teil 1: 0,08 je s; seit Teil 2a 0,02 je s über den längeren Weg), sonst an der Zona | – |
| Zona | 17 µm, schräg (40°) und mechanisch, 0,015–0,035 µm/s | im Mittel 12,7 min |
| Perivitelliner Spalt | neuer Zustand `Perivitelline`, Kopf flach an der Membran, 15,8 ± 5,7 min | im Mittel 15,6 min; kürzester Weg Zona → Verschmelzung 17 min |
| Schutz | Cortikalreaktion 20 min, Zona bindet nach 5 min nicht mehr; Gebundene lösen sich, Steckende bleiben, Überzählige bleiben im Spalt | Zona-Block nach 5,0 min; zwei im Spalt → eine verschmilzt, eine bleibt liegen |
| Zeit sichtbar | Zeitraffer ×45 an der Eizelle mit Uhr, logarithmische Rampe; Uhr „an der Eizelle“ bzw. „nach der Verschmelzung“ | Befruchtung nach 19–35 min biologisch, rund 30–45 s Spielzeit |
| 2. Polkörper | schnürt sich 2,5–3,5 h nach der Verschmelzung ab | – |
| „hpi“ | ersetzt durch „h seit Verschmelzung“ | – |

Offen für Teil 2: Cumulus in echter Größe (Millimeter) und Eileiter-Geometrie; dabei der Blick auf Zona und Spalt
(im Zeitraffer verdeckt ihn heute der dichte Zellkranz – geplant ist ein optischer Schnitt wie bei der Blastozyste);
Kapazitation als zeitlich begrenzter Zustand; der Satz im Vorfilm.

## Umsetzung GENESIS-047 Teil 2a: Cumulus und Eileiter in echter Größe (2026-09-23)

Recherche:
- Menschliche Eizellen tragen im Mittel 13.583 Cumuluszellen, reife 16.073 ± 2.595; zum Eisprung bilden rund 20.000 Zellen
  eine Masse von mehreren Millimetern (Ortiz 1982). Quelle: [PMC3955418](https://pmc.ncbi.nlm.nih.gov/articles/PMC3955418/)
- Maus: Der expandierte Komplex ist ein Sphäroid von ~500 µm mit ~1.500 Zellen; außen eine zellfreie Hyaluronsäure-Hülle
  bis 200 µm, im Körper doppelt so dick; die Gallerte ist extrem weich (~1 Pa). Quelle: Chen 2016,
  [PMC4919561](https://pmc.ncbi.nlm.nih.gov/articles/PMC4919561/)
- Ampulle: außen 4–10 mm, das Lumen durch Primär- und Sekundärfalten labyrinthisch (Histologie-Lehrbücher; 3D-Rekonstruktion
  Castro 2019, DOI 10.5114/pjr.2019.86824).
- Die meisten befruchtenden Mausspermien beginnen die Akrosomreaktion schon im Cumulus (Jin 2011, DOI 10.1073/pnas.1018202108).

Umgesetzt:

| Punkt | Vorher | Jetzt |
|---|---|---|
| Cumulus | dichte Corona bis 118 µm (2.600 Zellen) | Corona bis 118 µm + 13.400 freie Zellen bis 550 µm (zusammen ~16.000), Dichte fällt nach außen (gemessen innen 49.000, außen 11.000 je mm³), zellfreie Gallerte bis 700 µm |
| Zellen als Hindernis | Spermien schwammen durch Zellen | dieselben Zellen für Bild und Physik; die Spitze gleitet an der Membran entlang (Test: 0 Schritte in einer Zelle) |
| Eileiter | freier Kanal 0,9 mm, Wand 2,3 mm | freier Kanal 1,8 mm, Wand 4 mm, 14 Hauptfalten mit Seitenfalten (Blender, neu gebaut) |
| Zellen im Abschnitt | 150 | 80 – mit 150 fanden 10–14 den Spalt, mehr als bei der Maus im Reagenzglas (2–9) |
| Kapazitiert | 10 % | 8 % (Mitte von 2–14 %) |
| Zeit im Spalt | jede Zelle 16 ± 6 min für sich | Bereitschaft der Membran je Eizelle (± 5,4 min), je Zelle ± 1,7 min – so gilt 15,8 ± 5,7 min für die Siegerin, wie gemessen |
| Zeitlupe in der Gallerte | – | 0,5 statt 0,3 (hyperaktiviert 9–15 Hz, weiter ≥ 8 Bilder je Schlag) |

Rennen neu gemessen: gut gelenkt 4 von 5, ohne Führung 0 von 5; je Rennen 4–8 Zellen an der Zona, 2–7 im Spalt.

Offen (Teil 2b): Blick auf Zona und Spalt im Zeitraffer (optischer Schnitt durch Corona und Cumulus); die Eileiterwand liegt
jetzt 0,9 mm entfernt und bleibt im Endoskoplicht fast dunkel – Licht und Kamera auf den weiteren Raum abstimmen; Fäden der
Matrix im äußeren Cumulus.
