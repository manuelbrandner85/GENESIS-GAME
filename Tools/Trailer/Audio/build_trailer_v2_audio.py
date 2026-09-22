# GENESIS Reveal Trailer v2 - Tonmischung nach der Schnittliste (Tools/Trailer/trailer_v2_edl.py).
# Musik (Suno, MX_Trailer_Epic_A), Erzaehler (Gemini "Algieba"), Originaltoene des Spiels (Hebamme, Mutter, Schrei,
# Raumklang), Geraeusche (prozedural aus build_trailer_audio.py). Master: Ducking unter Stimme, erzwungene Stille, -1 dBFS.
#
# Aufruf: & "<Blender-Python>" Tools\Trailer\Audio\build_trailer_v2_audio.py
# Ausgabe: Genesis/Saved/Trailer/v2/GENESIS_Trailer_v2_Mix.wav (+ Stems)

import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))
import build_trailer_audio as base          # Grundfunktionen (Filter, Hall, Herzschlag, Rauschen ...)
import importlib
edl = importlib.import_module(os.environ.get("GENESIS_EDL", "trailer_v2_edl"))
NAME = getattr(edl, "NAME", "GENESIS_Trailer_v2")

SR = base.SR
N = int((edl.LENGTH + 2.0) * SR)
OUT = os.path.join(edl.REPO, "Genesis", "Saved", "Trailer", "v2")


def bus():
    return np.zeros((N, 2), dtype=np.float32)


def add(target, sig, t, gain=1.0):
    i = int(round(t * SR))
    if i >= N or len(sig) == 0:
        return
    if sig.ndim == 1:
        sig = np.stack([sig, sig], 1)
    sig = sig[: N - i]
    target[i:i + len(sig)] += sig * gain


def music():
    x = base.read_wav(edl.MUSIC_FILE)
    out = bus()
    for t0, t1, m0, gain, fin, fout in edl.MUSIC:
        a = int(m0 * SR)
        seg = x[a:a + int((t1 - t0) * SR)].copy()
        n = len(seg)
        env = np.ones(n, dtype=np.float32)
        if fin > 0:
            k = min(n, int(fin * SR))
            env[:k] = np.linspace(0, 1, k) ** 1.5
        if fout > 0:
            k = min(n, int(fout * SR))
            env[n - k:] *= np.linspace(1, 0, k) ** 1.2
        add(out, seg * env[:, None], t0, gain)
    return out


def room_verb(x, rt=0.7):
    pad = np.concatenate([x, np.zeros((SR, 2), np.float32)])
    return x * 0.8 + base.reverb(pad, rt, 0.012, 6000)[: len(x)] * 0.25


def voices():
    out = bus()
    placed = []
    for entry in edl.VO:
        t, path, gain, fx = entry[:4]
        if not os.path.exists(path):
            raise SystemExit("Stimme fehlt: " + path)
        x = base.trim(base.read_wav(path))
        if len(entry) > 5:                      # erst ab einer Stelle im Satz (Lippensynchronitaet)
            x = x[int(entry[5] * SR):]
        if len(entry) > 4:                      # nur ein Teil des Satzes (z. B. "Da ist er!")
            x = x[: int(entry[4] * SR)].copy()
            k = int(0.12 * SR)
            x[-k:] *= np.linspace(1, 0, k, dtype=np.float32)[:, None]
        x = x / (np.max(np.abs(x)) + 1e-9)
        if fx == "muffled":           # hinter der Bauchdecke gehoert
            x = base.fft_filter(x, 60, 380) * 2.0
        elif fx == "distant":         # der Geist hoert die Lebenden fern
            x = base.fft_filter(x, 220, 1600)
            x = x * 0.45 + base.reverb(np.concatenate([x, np.zeros((SR * 2, 2), np.float32)]), 2.2)[: len(x)] * 0.9
        elif fx == "room":            # Kreisssaal
            x = room_verb(x, 0.6)
        elif fx == "child":
            x = x * 0.85 + base.reverb(np.concatenate([x, np.zeros((SR, 2), np.float32)]), 1.4)[: len(x)] * 0.3
        else:                         # Erzaehler: trocken, minimaler Raum
            x = x * 0.9 + base.reverb(np.concatenate([x, np.zeros((SR // 2, 2), np.float32)]), 0.5, 0.01, 5000)[: len(x)] * 0.1
        add(out, x, t, 0.72 * gain)
        dur = len(x) / SR
        placed.append((t, t + dur, os.path.basename(path), fx))
    return out, placed


def game_sound(name, gain=1.0):
    path = os.path.join(edl.GAME_AUDIO, "SoundsWav", name)
    if not os.path.exists(path):
        path = os.path.join(edl.GAME_AUDIO, "RoomCandidatesWav", name)
    x = base.read_wav(path)
    return x / (np.max(np.abs(x)) + 1e-9) * gain


def sfx():
    out = bus()
    for kind, a, b, gain in edl.SFX:
        if kind.startswith("heartbeat"):
            pitch, bright = {"heartbeat": (1.0, 0.0), "heartbeat_fetal": (1.7, 0.5), "heartbeat_fast": (1.25, 0.1),
                             "heartbeat_slow": (0.88, 0.0)}[kind]
            for t in a:
                add(out, base.heartbeat(pitch, bright), t, gain)
        elif kind == "impulse":
            n = int(2.5 * SR)
            tt = np.arange(n) / SR
            boom = (np.sin(2 * np.pi * 38.0 * tt) * np.exp(-tt * 2.2)).astype(np.float32)
            shimmer = base.noise(2.5, 3000, 12000) * base.envelope(n, [(0, 0), (0.05, 0.5), (2.5, 0)])[:, None]
            swell = base.noise(1.2, 200, 4000) * base.envelope(int(1.2 * SR), [(0, 0), (1.15, 0.6), (1.2, 0)])[:, None]
            add(out, swell, a - 1.2, gain * 0.6)
            add(out, boom, a, gain)
            add(out, shimmer, a, gain * 0.3)
        elif kind == "cry":
            breath = base.breath(0.7, True, 300, 6000)
            add(out, breath, a, gain * 0.8)
            add(out, room_verb(game_sound("SFX_Schrei_Erster.wav")), a + 0.7, gain)
        elif kind == "last_breath":
            add(out, base.breath(2.0, False, 250, 3000), a, gain * 0.7)
        else:
            dur = b - a
            n = int(dur * SR)
            fade = [(0, 0.0), (min(0.8, dur / 3), 1.0), (dur - min(0.8, dur / 3), 1.0), (dur, 0.0)]
            if kind == "fluid":
                x = base.noise(dur, 30, 900, -4.0) * base.slow_lfo(n, 1.5, 0.35)[:, None]
            elif kind == "womb":
                x = base.noise(dur, 20, 160, -6.0) * base.envelope(n, [(0, 0.4), (dur, 1.0)])[:, None] * base.slow_lfo(n, 3.0, 0.4)[:, None]
            elif kind in ("room", "room_muffled"):
                r = game_sound("SFX_Kreisssaal_Nacht.wav")
                reps = int(np.ceil(n / len(r))) + 1
                x = np.concatenate([r] * reps)[:n]
                if kind == "room_muffled":
                    x = base.fft_filter(x, 60, 700)
            elif kind == "wind":
                x = base.noise(dur, 150, 3500, -3.0) * base.slow_lfo(n, 0.8, 0.45)[:, None]
            elif kind == "rain":
                x = base.noise(dur, 1500, 14000, -1.0) * 0.6
            elif kind == "cosmic":
                x = base.noise(dur, 80, 2500, -5.0) * base.slow_lfo(n, 0.4, 0.3)[:, None]
                tt = np.arange(n) / SR
                for f in (880.0, 1318.5, 1760.0):
                    x += (0.04 * np.sin(2 * np.pi * f * tt) * base.slow_lfo(n, 0.3, 0.9, 0.5))[:, None]
            else:
                continue
            add(out, x * base.envelope(n, fade)[:, None], a, gain)
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    mx = music()
    vo, placed = voices()
    fx = sfx()
    # Ducking: Musik -6 dB unter dem Erzaehler, -9 dB unter Originaltoenen (weich)
    duck = np.ones(N, dtype=np.float32)
    for s, e, name, kind in placed:
        depth = 0.35 if kind == "room" else 0.5
        a, b = int((s - 0.2) * SR), int((e + 0.3) * SR)
        duck[max(0, a):b] = np.minimum(duck[max(0, a):b], depth)
    k = int(0.3 * SR)
    duck = np.convolve(duck, np.ones(k) / k, mode="same").astype(np.float32)
    master = mx * 0.85 * duck[:, None] + fx * 0.8 + vo
    for t0, t1 in edl.SILENCE:
        a, b = int(t0 * SR), int(t1 * SR)
        master[a - 1200:a] *= np.linspace(1, 0, 1200, dtype=np.float32)[:, None]
        master[a:b] = 0.0
    master = master[: int(edl.LENGTH * SR)]
    peak = float(np.max(np.abs(master)))
    master = np.tanh(master / peak * 1.1) / np.tanh(1.1) * 0.89
    base.write_wav(os.path.join(OUT, NAME + "_Mix.wav"), master)
    base.write_wav(os.path.join(OUT, NAME + "_Stem_Music.wav"), (mx * duck[:, None])[: int(edl.LENGTH * SR)] / peak * 0.89)
    base.write_wav(os.path.join(OUT, NAME + "_Stem_Voice.wav"), vo[: int(edl.LENGTH * SR)] / peak * 0.89)
    base.write_wav(os.path.join(OUT, NAME + "_Stem_SFX.wav"), fx[: int(edl.LENGTH * SR)] / peak * 0.89)
    print("Stimmen (Start / Ende / Datei):")
    placed.sort()
    for i, (s, e, name, kind) in enumerate(placed):
        nxt = placed[i + 1][0] if i + 1 < len(placed) else edl.LENGTH
        warn = "  <-- UEBERLAPPUNG" if e > nxt + 0.02 else ""
        print("  %6.2f %6.2f  %-26s %s%s" % (s, e, name, kind or "", warn))
    rms = lambda x: 20 * np.log10(np.sqrt(np.mean(x ** 2)) + 1e-9)
    print("Master RMS %.1f dBFS, Spitze vor Limiter %.2f" % (rms(master), peak))
    for t0, t1 in edl.SILENCE:
        print("  Stille %.1f-%.1f: %.1f dBFS" % (t0, t1, rms(master[int(t0 * SR):int(t1 * SR)])))
    print("GENESIS_V2_AUDIO_OK", OUT)


if __name__ == "__main__":
    main()
