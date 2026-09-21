"""GENESIS – misst eine Tonaufnahme: Pegel (RMS/Spitze in dBFS), Stille-Anteil und grobe Klangfarbe.

Aufruf: python Tools/Audio/measure_wav.py <datei.wav> [...]
Ohne Ohren prüfbar: Ist die Szene hörbar, zu laut, übersteuert, dumpf oder hell?
"""
import math, struct, sys, wave


def read(path):
    with wave.open(path, "rb") as w:
        channels, width, rate, frames = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(frames)
    if width == 2:
        data = struct.unpack("<%dh" % (len(raw) // 2), raw)
        scale = 32768.0
    elif width == 4:
        data = struct.unpack("<%di" % (len(raw) // 4), raw)
        scale = 2147483648.0
    else:
        data = [int.from_bytes(raw[i:i + 3], "little", signed=True) for i in range(0, len(raw), 3)]
        scale = 8388608.0
    mono = [sum(data[i:i + channels]) / channels / scale for i in range(0, len(data), channels)]
    return mono, rate


def db(value):
    return 20 * math.log10(max(value, 1e-9))


for path in sys.argv[1:]:
    samples, rate = read(path)
    rms = math.sqrt(sum(s * s for s in samples) / max(1, len(samples)))
    peak = max((abs(s) for s in samples), default=0.0)
    block = rate // 10
    blocks = [samples[i:i + block] for i in range(0, len(samples) - block, block)]
    silent = sum(1 for b in blocks if math.sqrt(sum(s * s for s in b) / len(b)) < 10 ** (-60 / 20)) / max(1, len(blocks))
    # Klangfarbe: Anteil der Energie im Differenzsignal (hohe Frequenzen) – dumpf < 0,05 < hell
    diff = [samples[i] - samples[i - 1] for i in range(1, len(samples))]
    bright = (sum(d * d for d in diff) / max(1, len(diff))) / max(rms * rms, 1e-12)
    print("%s: %.1f s | RMS %.1f dBFS | Spitze %.1f dBFS | still %.0f %% | Helligkeit %.3f"
          % (path.split("\\")[-1], len(samples) / rate, db(rms), db(peak), 100 * silent, bright))
