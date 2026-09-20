# 21 – Die erste Entscheidung

Der Durchlauf (GENESIS-023) lief von selbst: von der Befruchtung bis zum ersten Schlaf, ohne dass der
Spieler etwas tun konnte. Das war ein Ablauf, kein Spiel. Dieser Block gibt dem Spieler das, was ein
Mensch an seinem ersten Tag tatsächlich hat – **und keinen Knopf mehr.**

## Drei Dinge

Ein Neugeborenes kann nicht greifen, nicht sprechen, nicht weggehen. Es kann:

| Taste | Was es tut | Was es kostet |
|---|---|---|
| **Leertaste** | **rufen** – die Welt hört es und kommt früher | Wärme und Ruhe |
| **E** | **suchen** – der Brustkrabbelgang zur Brust | nichts, aber es geht nur auf der Haut |
| **Maus** | **hinsehen** – den Kopf zur Stimme wenden | nichts, der Kopf sinkt von selbst zurück |

Mehr Werkzeuge hat ein Mensch an seinem ersten Tag nicht, und deshalb bekommt der Spieler auch
keine mehr. Alles andere entscheidet die Welt: ob jemand das Kind aufhebt, ob jemand mit ihm spricht,
ob es warm wird.

## Gemessen

| Messwert | ohne Eingriff | mit Eingriff |
|---|---|---|
| Erstes Anlegen | nach 32,0 min | **nach 18,0 min** (durchgehend gesucht) |
| Körpertemperatur nach 10 min | 34,92 °C | 34,81 °C (durchgehend gerufen) |
| Gerufene Zeit | 0 min | 10,0 min |

Beides ist wenig, und beides wirkt: Wer sucht, findet die Brust vierzehn Minuten früher. Wer ruft,
verliert dabei Wärme – und wird dafür früher versorgt (jede gerufene Minute holt die Hebamme vier
Minuten früher; `CallShortensCare`).

Wäre eines davon folgenlos, wäre die Eingabe Dekoration. Deshalb prüft der Test
`Genesis.EarlyLife.WhatTheChildCanDo` genau das – einschließlich des Falls, dass Suchen ohne
Hautkontakt nichts bringt, weil da nichts zu suchen ist.

## Die Anzeige

Zwei Zeilen unten im Bild, sonst nichts. Kein Balken, keine Zahl, kein Symbol: Was das Kind spürt,
soll man am Bild und am Ton merken – Wärme an der Farbe, Ruhe an der Kamera, Hunger am Schreien.
Die Anzeige sagt nur, **was möglich ist**, und blendet sich aus, sobald der Spieler es getan hat.
„E: suchen" erscheint erst, wenn das Kind auf der Haut liegt – vorher wäre der Hinweis eine Lüge.

![Die beiden Hinweise in der Geburtsszene](Media/GENESIS-029_Prompt.png)

## Warum klassische Tastenbelegung

Die Tasten liegen als Action-Mappings in `Config/DefaultInput.ini`, nicht als Enhanced-Input-Assets.
Der ganze Aufbau dieses Projekts läuft kopflos über Skripte; ein Eingabe-Asset, das nur im Editor
entsteht, wäre der einzige Schritt, der das nicht tut. Die Steuerung meldet beim Start, welche Tasten
sie in den Einstellungen tatsächlich gefunden hat:

```
Steuerung: Rufen auf SpaceBar, Gamepad_FaceButton_Bottom, Suchen auf E, Gamepad_FaceButton_Right
```

## Offen

- **Geprüft ist die Kette, nicht der Tastendruck.** Dass die Belegung ankommt, steht im Log; dass die
  Anzeige erscheint, zeigt das Bild; dass Rufen und Suchen wirken, messen die Tests über die Logik.
  Ein automatischer Tastendruck ist in dieser Umgebung nicht möglich – das muss ein Mensch einmal
  bestätigen.
- Vor der Geburt kann der Spieler nichts tun. Im Eileiter wäre die Frage, ob er überhaupt jemand ist.
- Kein Menü, kein Pausieren, kein Neustart aus dem Spiel heraus.
