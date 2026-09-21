"""GENESIS – prüft erzeugte Sprachaufnahmen ohne Zuhören.

Je Datei: Dauer, Sprechdauer, Sprechtempo (Silben/s aus dem Text geschätzt), Grundton (Median F0),
Spitze, längste Pause. Auffällig ist: Tempo außerhalb 2–7 Silben/s, F0 außerhalb 140–300 Hz (Frauenstimme),
Übersteuerung, Pausen > 2,5 s oder abgeschnittener Anfang/Ende.
Aufruf: python Tools/Audio/check_speech.py <ordner>   (Texte aus speech_lines.py)
"""
import os, re, struct, sys, wave, math

sys.path.insert(0, os.path.dirname(__file__))
from speech_lines import LINES  # noqa: E402

TEXT = {line_id: text for line_id, _, _, text in LINES}


def syllables(text):
    words = re.findall(r"[A-Za-zÄÖÜäöüß]+", text)
    return sum(max(1, len(re.findall(r"[aeiouyäöü]+", w.lower()))) for w in words)


def load(path):
    with wave.open(path) as w:
        rate, n, ch, width = w.getframerate(), w.getnframes(), w.getnchannels(), w.getsampwidth()
        raw = w.readframes(n)
    data = struct.unpack("<%dh" % (len(raw) // 2), raw) if width == 2 else []
    return [sum(data[i:i + ch]) / ch / 32768.0 for i in range(0, len(data), ch)], rate


def f0(samples, rate):
    frame = int(0.04 * rate)
    values = []
    for start in range(0, len(samples) - frame, frame):
        s = samples[start:start + frame]
        energy = sum(v * v for v in s) / frame
        if energy < 0.0009:
            continue
        mean = sum(s) / frame
        s = [v - mean for v in s]
        lo, hi = int(rate / 400), int(rate / 80)
        best, best_lag = 0.0, 0
        for lag in range(lo, hi, 2):
            c = sum(s[i] * s[i + lag] for i in range(0, frame - lag, 2))
            if c > best:
                best, best_lag = c, lag
        zero = sum(v * v for v in s[::2])
        if best_lag and best > 0.3 * zero:
            values.append(rate / best_lag)
    values.sort()
    return values[len(values) // 2] if values else 0.0


folder = sys.argv[1]
for name in sorted(os.listdir(folder)):
    if not name.endswith(".wav"):
        continue
    line_id = name[:-4]
    samples, rate = load(os.path.join(folder, name))
    block = rate // 50
    env = [math.sqrt(sum(v * v for v in samples[i:i + block]) / block) for i in range(0, len(samples) - block, block)]
    voiced = [i for i, e in enumerate(env) if e > 0.01]
    speech = (voiced[-1] - voiced[0] + 1) / 50.0 if voiced else 0.0
    gap, longest = 0, 0
    for e in env[voiced[0]:voiced[-1]] if voiced else []:
        gap = gap + 1 if e <= 0.01 else 0
        longest = max(longest, gap)
    rate_syl = syllables(TEXT.get(line_id, "")) / max(speech, 0.1)
    pitch = f0(samples, rate)
    peak = max(abs(v) for v in samples)
    flags = []
    if not 1.5 <= rate_syl <= 7.5: flags.append("Tempo")
    if pitch and not 140 <= pitch <= 320: flags.append("Tonhöhe")
    if peak > 0.99: flags.append("Übersteuert")
    if longest / 50.0 > 2.5: flags.append("lange Pause")
    if voiced and (voiced[0] == 0 or voiced[-1] >= len(env) - 1): flags.append("abgeschnitten?")
    print("%-16s %5.1f s | Sprache %4.1f s | %4.1f Silb/s | F0 %5.0f Hz | Spitze %.2f | Pause %.1f s %s"
          % (line_id, len(samples) / rate, speech, rate_syl, pitch, peak, longest / 50.0, ("  <- " + ", ".join(flags)) if flags else ""))
