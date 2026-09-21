"""GENESIS – findet reine Töne (Piepen, Klingeln) in einer Aufnahme, ohne Zuhören.

Mittelt das Spektrum über die ganze Aufnahme (Welch, 8192er Fenster) und meldet schmale Spitzen zwischen
400 Hz und 8 kHz, die mehr als 12 dB über dem Umfeld (±150 Hz, Median) liegen. Raumklang ohne Geräte zeigt
keine solchen Spitzen; ein Monitor-Piepen oder ein Metallklang schon.
Aufruf: blender -b --factory-startup --python Tools/Audio/check_tones.py -- <wav> ...
"""
import sys, wave
import numpy as np


def load(path):
    with wave.open(path) as w:
        rate, n, ch, width = w.getframerate(), w.getnframes(), w.getnchannels(), w.getsampwidth()
        raw = w.readframes(n)
    if width == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
        x = (b[:, 0].astype(np.int32) | (b[:, 1].astype(np.int32) << 8) | (b[:, 2].astype(np.int32) << 16))
        x = np.where(x >= 1 << 23, x - (1 << 24), x) / float(1 << 23)
    elif width == 4:
        x = np.frombuffer(raw, dtype=np.int32) / 2147483648.0
    else:
        x = np.frombuffer(raw, dtype=np.int16) / 32768.0
    return x.reshape(-1, ch).mean(axis=1), rate


for path in sys.argv[sys.argv.index("--") + 1:]:
    x, rate = load(path)
    size = 8192
    window = np.hanning(size)
    frames = [x[i:i + size] * window for i in range(0, len(x) - size, size // 2)]
    power = np.mean([np.abs(np.fft.rfft(f)) ** 2 for f in frames], axis=0)
    db = 10 * np.log10(power + 1e-20)
    freqs = np.fft.rfftfreq(size, 1.0 / rate)
    bin_hz = freqs[1]
    span = int(150 / bin_hz)
    peaks = []
    for i in range(int(400 / bin_hz), min(len(db) - span, int(8000 / bin_hz))):
        local = np.median(db[i - span:i + span])
        if db[i] - local > 12 and db[i] == db[i - 2:i + 3].max():
            peaks.append((freqs[i], db[i] - local))
    peaks.sort(key=lambda p: -p[1])
    name = path.replace("\\", "/").split("/")[-1]
    if peaks:
        print("GENESIS %-30s TÖNE: %s" % (name, ", ".join("%.0f Hz (+%.0f dB)" % p for p in peaks[:6])))
    else:
        print("GENESIS %-30s keine reinen Töne" % name)
