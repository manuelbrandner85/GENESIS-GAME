"""GENESIS – prüft, ob eine Aufnahme wie ein Neugeborenes klingt (ohne Zuhören).

Referenz (Säuglingsschrei, erste Lebenstage): Grundton 350–600 Hz (Median ~450), Schreie von 0,5–1,5 s,
dazwischen kurze Einatmer (0,1–0,5 s), 30–60 Schreie pro Minute; keine Musik (keine stabilen Akkorde).
Aufruf mit Blenders Python (numpy): blender -b --factory-startup --python Tools/Audio/check_cry.py -- <wav> ...
"""
import sys, wave
import numpy as np

for path in sys.argv[sys.argv.index("--") + 1:]:
    with wave.open(path) as w:
        rate, n, ch, width = w.getframerate(), w.getnframes(), w.getnchannels(), w.getsampwidth()
        raw = w.readframes(n)
    if width == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
        x = (b[:, 0].astype(np.int32) | (b[:, 1].astype(np.int32) << 8) | (b[:, 2].astype(np.int32) << 16))
        x = np.where(x >= 1 << 23, x - (1 << 24), x) / float(1 << 23)
    else:
        x = np.frombuffer(raw, dtype=np.int16) / 32768.0
    x = x.reshape(-1, ch).mean(axis=1)
    hop = rate // 100
    env = np.array([np.sqrt(np.mean(x[i:i + hop] ** 2)) for i in range(0, len(x) - hop, hop)])
    loud = env > max(0.02, 0.2 * env.max())
    bursts, gaps, run, gap = [], [], 0, 0
    for v in loud:
        if v:
            if gap and run == 0 and bursts:
                gaps.append(gap / 100)
            run += 1
            gap = 0
        else:
            if run:
                bursts.append(run / 100)
            run = 0
            gap += 1
    f0s = []
    frame = int(0.04 * rate)
    for i in range(0, len(x) - frame, frame):
        s = x[i:i + frame]
        if np.sqrt(np.mean(s ** 2)) < 0.3 * env.max():
            continue
        s = s - s.mean()
        ac = np.correlate(s, s, "full")[frame - 1:]
        lo, hi = int(rate / 1000), int(rate / 200)
        k = lo + int(np.argmax(ac[lo:hi]))
        if ac[k] > 0.35 * ac[0]:
            f0s.append(rate / k)
    bursts = [b for b in bursts if b >= 0.15]
    f0 = float(np.median(f0s)) if f0s else 0.0
    per_min = len(bursts) / (len(x) / rate / 60)
    ok = 330 <= f0 <= 650 and bursts and 0.3 <= np.median(bursts) <= 2.0
    print("GENESIS %-34s %5.1f s | F0 %4.0f Hz | %2d Schreie (%.0f/min), Median %.2f s | Pausen %.2f s | %s"
          % (path.replace("\\", "/").split("/")[-1], len(x) / rate, f0, len(bursts), per_min,
             float(np.median(bursts)) if bursts else 0, float(np.median(gaps)) if gaps else 0,
             "plausibel" if ok else "AUFFÄLLIG"))
