# 19 – Der Klang der Orte

Bis hierher klang in GENESIS nur der Körper: Herzschlag, Blutstrom, Atem, Stimmen. Die Welt um ihn herum
war still. Das ist der Unterschied zwischen einem Messgerät und einem Ort – ein Kreißsaal ist nicht still.
Er summt, klappert, raschelt, piept im Takt eines Herzens, und irgendwo im Gang spricht jemand.

Plugin: `GenesisWorldSound` (Synthese ohne Welt, Komponente am Hörer, ein Actor je Ort).

## Zwei Schichten

1. **Grundton** – drei Rauschbänder (tief, mittel, hoch), langsam atmend. Jeder Raum hat einen,
   und man hört ihn erst, wenn er fehlt.
2. **Ereignisse** – einzelne Geräusche zu Zeitpunkten aus einem **Poisson-Prozess**: im Mittel so und so
   viele je Minute, aber nie im Takt. Ein Raum, in dem es alle fünf Sekunden klappert, klingt nach Maschine;
   ein Raum, in dem es *im Mittel* alle fünf Sekunden klappert, klingt nach Raum.

Über allem liegt die **Bandgrenze des Ortes**: Im Mutterleib kommt oberhalb von etwa 700 Hz nichts mehr
durch – und zwar für alles, auch für jedes einzelne Ereignis. Sonst wäre der Raum dumpf, aber jedes
Geräusch darin hell.

## Die drei Orte des Vertical Slice

| Ort | Grundton | Ereignisse |
|---|---|---|
| Eileiter-Ampulle | Strömen, oben ein feines Schimmern | Tropfen und Blasen, ferner Puls durch das Gewebe |
| Mutterleib (von innen) | tiefes Grundrauschen, Bandgrenze 700 Hz | **Darmgeräusche**, Rauschen des Mutterkuchens im Takt des mütterlichen Herzens, gedämpfte Stimmen |
| Kreißsaal | Lüftung und Geräte, breit und leise | Monitor im Pulstakt, Instrumente, Tücher, Schritte, Stimmen im Gang |

**Gemessen** (Höhenanteil über 2 kHz, bezogen auf die Gesamtlautheit):

| | Mutterleib | Eileiter | Kreißsaal |
|---|---|---|---|
| Höhenanteil | 0,0925 | 0,3510 | 0,5455 |

Der Kreißsaal trägt also fast sechsmal so viel Höhe wie der Mutterleib – genau das ist der Sprung,
den ein Kind bei der Geburt erlebt.

### Darmgeräusche

In GENESIS-012 stand „Darmgeräusche der Mutter fehlen" als offener Punkt. Sie sind jetzt da, und zwar
in der Häufigkeit, in der sie beim Menschen auftreten: **5 bis 30 je Minute**.

**Gemessen:** bei arbeitender Verdauung 21,0 je Minute (vorgesehen 17,6 – der Poisson-Prozess schwankt),
bei ruhender 4 je Minute. Das Rauschen des Mutterkuchens läuft mit 79,5 je Minute bei einem Puls von 78 –
also im Takt des mütterlichen Herzens, mit einer Spur Schwankung, wie ein echtes Herz.

### Der Kreißsaal folgt der Geburt

Der Ort liest mit, was in der Geburt geschieht: Der Puls der Mutter steigt von 82 auf über 126,
der Monitor wird schneller, im Raum geschieht mehr. Nach der Geburt kommt beides in wenigen Minuten herunter.

**Gemessen** (je zwei Minuten): ruhig 2 Instrumente, 10 Schritte, 167 Monitortöne bei Puls 82 –
unter der Austreibung 17 Instrumente, 47 Schritte, 253 Monitortöne bei Puls 124.

## Die Mischung wirkt jetzt

Die Mix-Engine des Audio Core (Prioritäten, Ducking, Zeitkonstanten) gab es seit GENESIS-008 –
**angewendet hat sie bis jetzt niemand.** Das ist nachgeholt: Körperklang, Musik, Stimmen und Orte
holen sich ihren Bus-Pegel und multiplizieren ihn auf ihr Signal. Wer spricht, meldet sich beim Mix an;
ein Schrei läuft dabei nicht über den Dialogbus, sondern über den Bus für lebenswichtige Signale –
er schneidet schneller durch.

**Gemessen:** Wenn jemand spricht, tritt die Umgebung von 0,0 dB auf −5,0 dB zurück und die Musik
auf −8,0 dB; eine halbe Sekunde nach dem Satz steht die Umgebung bei −3,6 dB. Sie kommt also
langsamer zurück, als sie gegangen ist – sonst würde der Mix pumpen.

## Ein Ohr für alles

Der Filter der Hörwahrnehmung liegt jetzt in `GenesisAudioCore` (`FGenesisHearingFilter`) und gilt für
Stimmen **und** für Orte – es ist dasselbe Ohr. In der Geburtsszene stehen deshalb zwei Orte gleichzeitig:

- **WombTone** – von innen gehört, ohne zusätzliche Dämpfung: Das ist der Ort, in dem das Kind liegt.
- **DeliveryRoomTone** – von außen gehört: vor der Geburt durch Bauchdecke und Fruchtwasser gedämpft,
  nach dem ersten Atemzug klar.

Denselben Sprung erlebt das Kind, und es braucht dafür keinen Schnitt und keinen Regler.

## Bedienung (Entwickler)

```
genesis.World.RenderWav mutterleib 20   # Raumprobe schreiben (Saved/Audio)
genesis.World.RenderWav eileiter 15
genesis.World.RenderWav kreisssaal 20
genesis.Debug.Page Place                # HUD-Seite „Klang der Orte"
```

![Klang der Orte während der Geburt](Media/GENESIS-015_Places.png)

Hörproben: `Docs/Media/Audio/GENESIS_Ort_Mutterleib.wav`, `…_Eileiter.wav`, `…_Kreisssaal.wav`.

## Offen

- Mono, ohne Raumhall und ohne Ortung einzelner Ereignisse im Raum.
- Die Stimmen im Gang sind Rauschen mit Sprechrhythmus, keine echten Stimmen (die Stimmsynthese
  läuft noch nicht durch den Ortsklang).
- Keine Türen, keine Geräte-Alarme, kein Wasser, keine Wehen-Geräusche der Mutter (Atmen, Pressen).
- Der Mix im Spiel ist gemessen, der Gesamtklang aber bisher nur über Hörproben geprüft.
