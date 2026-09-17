# GENESIS Trailer - Soul Theme, Sound Design und Mix fuer das Animatic.
#
# Musik:  komponiert in diesem Skript (eigene Komposition "GENESIS Soul Theme"), gespielt mit den gesampelten
#         Instrumenten der Unreal-Engine-Bibliothek (Harmonix-Plugin, exportiert durch Unreal/ExportLibraryAudio.py).
# Sound:  Unreal liefert keine Herzschlag/Atem/Wind/Fluessigkeit-Sounds -> prozedurale Synthese (PLACEHOLDER, wird
#         als MetaSound in Unreal nachgebaut bzw. durch lizenzierte Aufnahmen ersetzt).
# Stimme: PLACEHOLDER aus Tools/Trailer/Audio/Generate-TempVO.ps1.
#
# Ausfuehren mit dem Python von Blender (numpy enthalten):
#   "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" Tools\Trailer\Audio\build_trailer_audio.py
# Ausgabe: Genesis/Saved/TrailerAudio/Mix/*.wav (Stems + Gesamtmix), Timing-Bericht auf der Konsole.

import glob
import json
import os
import re
import wave

import numpy as np

SR = 48000
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
AUDIO = os.path.join(REPO, "Genesis", "Saved", "TrailerAudio")
LIB = os.path.join(AUDIO, "Library")
OUT = os.path.join(AUDIO, "Mix")
DATA = json.load(open(os.path.join(REPO, "Docs", "Trailer", "ShotList.json"), encoding="utf-8"))
LENGTH = float(DATA["duration"])
N = int((LENGTH + 4.0) * SR)
rng = np.random.default_rng(1306)


# ---------------------------------------------------------------- Grundlagen
def read_wav(path):
    with wave.open(path) as w:
        sr, ch, width, n = w.getframerate(), w.getnchannels(), w.getsampwidth(), w.getnframes()
        raw = w.readframes(n)
    if width == 2:
        x = np.frombuffer(raw, dtype=np.int16).astype(np.float32) / 32768.0
    elif width == 3:
        b = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        v = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
        v = np.where(v >= 1 << 23, v - (1 << 24), v)
        x = v.astype(np.float32) / float(1 << 23)
    else:
        raise ValueError("Nicht unterstuetzte Bittiefe: " + path)
    x = x.reshape(-1, ch)
    if ch == 1:
        x = np.repeat(x, 2, axis=1)
    if sr != SR:
        t_old = np.arange(len(x)) / sr
        t_new = np.arange(int(len(x) * SR / sr)) / SR
        x = np.stack([np.interp(t_new, t_old, x[:, c]) for c in range(2)], axis=1).astype(np.float32)
    return x


def write_wav(path, x):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    pcm = (np.clip(x, -1.0, 1.0) * 32767.0).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


def bus():
    return np.zeros((N, 2), dtype=np.float32)


def add(target, sig, t, gain=1.0, pan=0.0):
    i = int(t * SR)
    if i >= N or len(sig) == 0:
        return
    if sig.ndim == 1:
        sig = np.stack([sig, sig], axis=1)
    sig = sig[: N - i]
    left = np.cos((pan + 1.0) * np.pi / 4.0) * np.sqrt(2.0)
    right = np.sin((pan + 1.0) * np.pi / 4.0) * np.sqrt(2.0)
    target[i:i + len(sig), 0] += sig[:, 0] * gain * left
    target[i:i + len(sig), 1] += sig[:, 1] * gain * right


def fft_filter(x, lo=0.0, hi=None, tilt_db_oct=0.0):
    """Bandpass/Tiefpass im Frequenzbereich (weiche Flanken). x: mono oder stereo."""
    mono = x.ndim == 1
    xs = x[:, None] if mono else x
    n = len(xs)
    f = np.fft.rfftfreq(n, 1.0 / SR)
    g = np.ones_like(f)
    if lo > 0:
        g *= 1.0 / np.sqrt(1.0 + (lo / np.maximum(f, 1e-3)) ** 4)
    if hi:
        g *= 1.0 / np.sqrt(1.0 + (f / hi) ** 4)
    if tilt_db_oct:
        g *= (np.maximum(f, 20.0) / 1000.0) ** (tilt_db_oct / 6.02)
    out = np.stack([np.fft.irfft(np.fft.rfft(xs[:, c]) * g, n) for c in range(xs.shape[1])], axis=1)
    return out[:, 0].astype(np.float32) if mono else out.astype(np.float32)


def noise(dur, lo=20.0, hi=20000.0, tilt=0.0, stereo=True):
    n = int(dur * SR)
    x = rng.standard_normal((n, 2 if stereo else 1)).astype(np.float32)
    x = fft_filter(x, lo, hi, tilt)
    return x / (np.max(np.abs(x)) + 1e-9)


def envelope(n, points):
    """points: [(t_sec, gain), ...] linear interpoliert."""
    t = np.arange(n) / SR
    ts, gs = zip(*points)
    return np.interp(t, ts, gs).astype(np.float32)


def slow_lfo(n, rate, depth, base=1.0):
    ctrl_t = np.arange(0, n / SR + 1.0 / rate, 1.0 / rate)
    ctrl = base + depth * (rng.random(len(ctrl_t)) * 2.0 - 1.0)
    return np.interp(np.arange(n) / SR, ctrl_t, ctrl).astype(np.float32)


def reverb(x, rt60=2.8, predelay=0.02, tone=6000.0, seed=7):
    """Faltungshall mit synthetischer, stereo-dekorrelierter Impulsantwort."""
    r = np.random.default_rng(seed)
    n_ir = int(rt60 * SR)
    t = np.arange(n_ir) / SR
    ir = r.standard_normal((n_ir, 2)).astype(np.float32) * np.exp(-6.91 * t / rt60)[:, None]
    ir = fft_filter(ir, 120.0, tone)
    ir[: int(predelay * SR)] = 0.0
    ir /= np.sqrt(np.sum(ir ** 2, axis=0, keepdims=True)) + 1e-9
    size = 1 << int(np.ceil(np.log2(len(x) + n_ir)))
    out = np.zeros_like(x)
    for c in range(2):
        y = np.fft.irfft(np.fft.rfft(x[:, c], size) * np.fft.rfft(ir[:, c], size), size)
        out[:, c] = y[: len(x)]
    return out.astype(np.float32)


# ---------------------------------------------------------------- Sampler (Unreal-Bibliothek)
NOTE_PC = {"c": 0, "c#": 1, "db": 1, "d": 2, "d#": 3, "eb": 3, "e": 4, "f": 5, "f#": 6, "gb": 6, "g": 7,
           "g#": 8, "ab": 8, "a": 9, "a#": 10, "bb": 10, "b": 11}


def midi(name):
    m = re.match(r"([A-Ga-g][#b]?)(-?\d)", name)
    return 12 * (int(m.group(2)) + 1) + NOTE_PC[m.group(1).lower()]


class Instrument:
    def __init__(self, folder, attack, release, loop=True):
        self.samples = []
        for path in glob.glob(os.path.join(LIB, folder, "*.wav")):
            base = os.path.splitext(os.path.basename(path))[0]
            nums = re.findall(r"_(\d{2,3})(?=_|$)", base)
            if not nums:
                continue
            root = int(nums[0])
            corrected = False
            named = re.search(r"_\d{2,3}_([A-Ga-g][#b]?)-?\d$", base)
            if named and root % 12 != NOTE_PC[named.group(1).lower()]:
                # Widerspruch Zahl/Notenname in der Bibliothek (z. B. legatostrings_85_c5) -> Notenname gilt
                pc = NOTE_PC[named.group(1).lower()]
                root = min((root + d for d in range(-6, 6) if (root + d) % 12 == pc), key=lambda v: abs(v - root))
                corrected = True
            self.samples.append((root, corrected, path))
        # Bei doppelten Grundtoenen gewinnt das eindeutig benannte Sample (z. B. pizstrings_72_c4 vor pizstrings_75_c4)
        unique = {}
        for root, corrected, path in sorted(self.samples, key=lambda s: s[1], reverse=True):
            unique[root] = path
        self.samples = list(unique.items())
        if not self.samples:
            raise RuntimeError("Keine Samples fuer " + folder + " - zuerst ExportLibraryAudio.py ausfuehren.")
        self.samples.sort()
        self.cache = {}
        self.attack, self.release, self.loop = attack, release, loop

    def data(self, path):
        if path not in self.cache:
            x = read_wav(path)
            self.cache[path] = x / (np.max(np.abs(x)) + 1e-9)
        return self.cache[path]

    def render(self, note, dur, vel):
        root, path = min(self.samples, key=lambda s: abs(s[0] - note))
        src = self.data(path)
        ratio = 2.0 ** ((note - root) / 12.0)
        total = dur + self.release
        need = int(total * ratio * SR) + 2
        if need > len(src):
            if self.loop:
                a, b = int(len(src) * 0.35), int(len(src) * 0.9)
                seg, fade = src[a:b], int(0.12 * SR)
                ramp = np.linspace(0.0, 1.0, fade, dtype=np.float32)[:, None]
                out = src[:b].copy()
                while len(out) < need:
                    tail = out[-fade:] * (1.0 - ramp) + seg[:fade] * ramp
                    out = np.concatenate([out[:-fade], tail, seg[fade:]])
                src = out
            else:
                src = np.concatenate([src, np.zeros((need - len(src), 2), dtype=np.float32)])
        pos = np.arange(int(total * SR)) * ratio
        sig = np.stack([np.interp(pos, np.arange(len(src)), src[:, c]) for c in range(2)], axis=1).astype(np.float32)
        n = len(sig)
        env = np.ones(n, dtype=np.float32)
        a = max(1, int(self.attack * SR))
        env[:a] = np.linspace(0.0, 1.0, a) ** 2
        s_end = min(n, int(dur * SR))
        rel = n - s_end
        if rel > 0:
            env[s_end:] = np.exp(-5.0 * np.linspace(0.0, 1.0, rel))
        return sig * env[:, None] * vel


PIANO = Instrument("Piano", 0.004, 1.6, loop=False)
STRINGS = Instrument("StringsLegato", 0.35, 1.4)
PIZZ = Instrument("StringsPizzicato", 0.003, 0.35, loop=False)
HORNS = Instrument("Horns", 0.25, 1.2)
FLUTE = Instrument("Flute", 0.3, 1.0)
VIBES = Instrument("Vibraphone", 0.003, 2.5, loop=False)
BASS = Instrument("UprightBass", 0.02, 0.8, loop=False)

music = bus()
music_send = bus()   # Hallanteil


def play(inst, note, t, dur, vel, pan=0.0, send=0.35):
    if isinstance(note, str):
        note = midi(note)
    sig = inst.render(note, dur, vel)
    add(music, sig, t, 1.0, pan)
    add(music_send, sig, t, send, pan)


# ---------------------------------------------------------------- GENESIS Soul Theme (eigene Komposition)
# D-Moll, 60 BPM (1 Schlag = 1 Sekunde = Ruhepuls). Das Motiv endet auf A und kehrt zum D zurueck: ein Kreis.
MOTIF = [(0, "A4", 1), (1, "D5", 1), (2, "E5", 0.5), (2.5, "F5", 1.5), (4, "E5", 1), (5, "D5", 1), (6, "C5", 1),
         (7, "A4", 1), (8, "Bb4", 1), (9, "A4", 1), (10, "G4", 1), (11, "F4", 1), (12, "E4", 1), (13, "F4", 0.5),
         (13.5, "G4", 0.5), (14, "A4", 2)]
CHORDS = {"Dm": ["D3", "A3", "D4", "F4"], "Bb": ["Bb2", "F3", "Bb3", "D4"], "F": ["F2", "C3", "F3", "A3"],
          "C": ["C3", "G3", "C4", "E4"], "Gm": ["G2", "D3", "G3", "Bb3"], "A": ["A2", "E3", "A3", "C#4"],
          "D": ["D3", "A3", "D4", "F#4"]}
PROGRESSION = ["Dm", "Bb", "F", "C", "Gm", "Dm", "Bb", "A"]   # je 2 Schlaege, 16 Schlaege Zyklus


def transpose(name, octaves):
    return midi(name) + 12 * octaves


def motif_notes(start, end, octave=0):
    loop = 0
    while True:
        base = start + 16 * loop
        if base >= end:
            return
        for beat, name, dur in MOTIF:
            t = base + beat
            if t < end:
                yield t, transpose(name, octave), min(dur, end - t), beat
        loop += 1


def chords(start, end, step=2.0):
    i = 0
    t = start
    while t < end:
        yield t, PROGRESSION[i % len(PROGRESSION)], min(step, end - t)
        t += step
        i += 1


def ramp(t, t0, t1, g0, g1):
    if t1 <= t0:
        return g1
    u = min(1.0, max(0.0, (t - t0) / (t1 - t0)))
    return g0 + (g1 - g0) * u


def compose():
    # 8-27 s: Fluestern - einzelne Pianotoene, sehr leise
    sparse = {0, 1, 2.5, 5, 7, 9, 11, 14}
    for t, n, d, beat in motif_notes(8.5, 26.0):
        if beat in sparse:
            play(PIANO, n, t, d + 0.8, 0.10 * ramp(t, 8.5, 12.0, 0.4, 1.0) * ramp(t, 23.0, 26.0, 1.0, 0.3), -0.1, 0.6)

    # 30-39 s: warmer Aufbau nach der Geburt
    for t, n, d, _ in motif_notes(30.0, 38.6):
        play(PIANO, n, t, d + 0.3, 0.30 * ramp(t, 30.0, 33.0, 0.6, 1.0), 0.05, 0.4)
    for t, c, d in chords(30.0, 38.6):
        play(PIANO, CHORDS[c][0], t, d, 0.16, -0.2, 0.4)
        if t >= 32.0:
            for k, name in enumerate(CHORDS[c][1:]):
                play(STRINGS, name, t, d + 0.1, 0.06 * ramp(t, 32.0, 36.0, 0.3, 1.0), -0.4 + 0.4 * k, 0.5)

    # 43-53 s: Jugend - Pulse (16tel Pizzicato), Pad, Bass; ab 48.5 haelt die Musik den Atem an
    for t, c, d in chords(43.0, 53.0):
        tones = CHORDS[c]
        pattern = [tones[0] + "", tones[1], tones[2], tones[1]]
        for s in range(int(d / 0.25)):
            ts = t + s * 0.25
            accent = 1.0 if s % 4 == 0 else 0.7
            hold = 0.45 if ts >= 48.5 else 1.0
            play(PIZZ, transpose(pattern[s % 4], 1 if s % 8 >= 4 else 0), ts, 0.2, 0.16 * accent * hold, 0.3 * (1 if s % 2 else -1), 0.25)
        for k, name in enumerate(tones[1:]):
            play(STRINGS, name, t, d + 0.1, 0.07 * (0.6 if t >= 48.5 else 1.0), -0.3 + 0.3 * k, 0.4)
        if t < 48.5:
            play(BASS, transpose(tones[0], -1), t, 0.9, 0.35, 0.0, 0.15)
    for t, n, d, beat in motif_notes(45.0, 48.5, 1):
        play(PIANO, n, t, d, 0.12, 0.2, 0.5)

    # 53-69 s: Erwachsenenleben - groessere Dynamik
    for t, c, d in chords(53.0, 69.0):
        g = ramp(t, 53.0, 62.0, 0.55, 1.0) * ramp(t, 66.0, 68.5, 1.0, 0.35)
        tones = CHORDS[c]
        for k, name in enumerate(tones):
            play(STRINGS, name, t, d + 0.1, 0.09 * g, -0.45 + 0.3 * k, 0.45)
        if t >= 57.0:
            for name in tones[1:3]:
                play(HORNS, name, t, d + 0.1, 0.10 * g, 0.2, 0.5)
        play(BASS, transpose(tones[0], -1), t, 1.6, 0.40 * g, 0.0, 0.15)
        for s in range(int(d / 0.25)):
            ts = t + s * 0.25
            play(PIZZ, transpose(tones[(s % 3) + 1], 1), ts, 0.2, 0.09 * g * (1.0 if s % 4 == 0 else 0.6), 0.35 * (1 if s % 2 else -1), 0.2)
    for t, n, d, _ in motif_notes(55.0, 68.5):
        g = ramp(t, 55.0, 62.0, 0.6, 1.0) * ramp(t, 66.0, 68.5, 1.0, 0.4)
        play(STRINGS, n, t, d + 0.2, 0.20 * g, 0.1, 0.5)
        play(PIANO, n + 12, t, d, 0.10 * g, 0.3, 0.5)

    # 69-80 s: Alter - stark reduziert, Spieluhr-Motiv (Vibraphon hoch) ab 76 s
    for name in ["D3", "A3", "F4"]:
        play(STRINGS, name, 69.0, 7.5, 0.05, 0.0, 0.7)
    for t, n, d, _ in motif_notes(76.0, 80.0, 1):
        play(VIBES, n, t, d, 0.20, 0.15, 0.7)

    # 80-87.7 s: Tod - ein tiefer Streicherton, endet mit dem letzten Herzschlag
    for name in ["D3", "A3"]:
        sig = STRINGS.render(midi(name), 7.7, 0.05)
        sig *= envelope(len(sig), [(0, 1.0), (5.0, 0.6), (7.6, 0.0), (99, 0.0)])[:, None]
        add(music, sig, 80.0)
        add(music_send, sig, 80.0, 0.5)

    # 94-110 s: Jenseits - orchestraler, kosmischer Aufbau (Chor-Pad = Streicher+Floete hoch, PLACEHOLDER fuer Chor)
    for t, c, d in chords(94.0, 110.0):
        g = ramp(t, 94.0, 99.5, 0.35, 0.75) * ramp(t, 101.0, 103.0, 1.0, 0.8) * ramp(t, 106.0, 110.0, 1.0, 1.35)
        tones = CHORDS[c]
        for k, name in enumerate(tones):
            play(STRINGS, name, t, d + 0.3, 0.10 * g, -0.5 + 0.33 * k, 0.7)
            play(STRINGS, transpose(name, 1), t, d + 0.3, 0.05 * g, 0.5 - 0.33 * k, 0.8)
        play(FLUTE, transpose(tones[2], 1), t, d + 0.3, 0.07 * g, 0.4, 0.8)
        if t >= 98.0:
            play(HORNS, tones[1], t, d + 0.2, 0.12 * g, -0.2, 0.6)
        if t >= 101.0:
            play(BASS, transpose(tones[0], -1), t, 1.8, 0.35 * g, 0.0, 0.2)
    for t, n, d, _ in motif_notes(99.5, 110.0):
        g = ramp(t, 99.5, 103.0, 0.6, 0.9) * ramp(t, 106.0, 110.0, 1.0, 1.3)
        play(FLUTE, n, t, d + 0.2, 0.22 * g, 0.2, 0.7)
        play(HORNS, n - 12, t, d + 0.2, 0.10 * g, -0.1, 0.6)

    # 110-113 s: groesster musikalischer Moment -> D-Dur (Picardie) -> harte Reduktion bei 113.0
    climax = [(110.0, "Bb", "F5"), (111.0, "C", "E5"), (112.0, "D", "F#5")]
    for t, c, mel in climax:
        d = 1.0 if t < 112.0 else 1.05
        for k, name in enumerate(CHORDS[c]):
            play(STRINGS, name, t, d, 0.16, -0.5 + 0.33 * k, 0.6)
            play(HORNS, transpose(name, 0), t, d, 0.14, 0.3 - 0.2 * k, 0.6)
        play(STRINGS, mel, t, d, 0.30, 0.0, 0.6)
        play(HORNS, midi(mel) - 12, t, d, 0.22, 0.0, 0.6)
        play(PIANO, transpose(CHORDS[c][0], -1), t, d, 0.45, 0.0, 0.6)
        play(BASS, transpose(CHORDS[c][0], -1), t, d, 0.55, 0.0, 0.2)


# ---------------------------------------------------------------- Sound Design (prozedural, PLACEHOLDER)
def heartbeat(pitch=1.0, bright=0.0):
    def thump(freq, dur, amp):
        n = int(dur * SR)
        t = np.arange(n) / SR
        f = freq * (1.0 + 0.8 * np.exp(-t * 30.0))
        x = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t * 18.0)
        click = rng.standard_normal(n) * np.exp(-t * 90.0) * 0.15
        return (x + fft_filter(click.astype(np.float32), 30.0, 300.0 + 1500.0 * bright)) * amp
    lub = thump(52.0 * pitch, 0.28, 1.0)
    dub = thump(62.0 * pitch, 0.22, 0.65)
    out = np.zeros(int(0.6 * SR), dtype=np.float32)
    out[: len(lub)] += lub
    j = int(0.26 * SR / max(pitch, 0.8))
    out[j:j + len(dub)] += dub[: len(out) - j]
    return fft_filter(out, 25.0, 900.0 + 3000.0 * bright)


def baby_cry():
    """Formant-Synthese, drei Schreie (PLACEHOLDER)."""
    parts = []
    for dur, f0 in [(0.9, 470.0), (1.1, 520.0), (1.3, 490.0)]:
        n = int(dur * SR)
        t = np.arange(n) / SR
        contour = f0 * (1.0 + 0.12 * np.sin(np.pi * t / dur)) * (1.0 + 0.015 * np.sin(2 * np.pi * 6.5 * t))
        phase = np.cumsum(contour) / SR
        saw = (2.0 * (phase % 1.0) - 1.0).astype(np.float32)
        voiced = sum(fft_filter(saw, fc * 0.8, fc * 1.25) * g for fc, g in [(1100, 1.0), (2600, 0.6), (4100, 0.25)])
        voiced += noise(dur, 2000, 8000, stereo=False)[:, 0] * 0.05
        voiced *= envelope(n, [(0, 0.0), (0.06, 1.0), (dur * 0.7, 0.9), (dur, 0.0)])
        parts.append(voiced)
        parts.append(np.zeros(int(0.35 * SR), dtype=np.float32))
    x = np.concatenate(parts)
    return x / (np.max(np.abs(x)) + 1e-9)


def breath(dur, inhale=True, lo=400.0, hi=5000.0):
    x = noise(dur, lo, hi, -3.0)
    n = len(x)
    shape = [(0, 0.0), (dur * 0.35, 1.0), (dur, 0.0)] if inhale else [(0, 0.0), (dur * 0.1, 1.0), (dur, 0.0)]
    return x * envelope(n, shape)[:, None]


def build_sfx(sfx, music_gate):
    for cue in DATA["sfx"]:
        kind, gain = cue["id"], cue.get("gain", 1.0)
        if "times" in cue:
            for t in cue["times"]:
                t += rng.uniform(-0.01, 0.01)
                if kind in ("heartbeat", "heartbeat_title"):
                    add(sfx, heartbeat(1.0, 0.0), t, gain)
                elif kind == "heartbeat_fetal":
                    add(sfx, heartbeat(1.7, 0.5), t, gain, 0.1)
                elif kind == "heartbeat_birth":
                    add(sfx, heartbeat(1.25, 0.1), t, gain)
                elif kind == "heartbeat_slow":
                    add(sfx, heartbeat(0.9, 0.0), t, gain)
                elif kind == "impulse":
                    n = int(2.5 * SR)
                    tt = np.arange(n) / SR
                    boom = np.sin(2 * np.pi * 38.0 * tt) * np.exp(-tt * 2.2)
                    shimmer = noise(2.5, 3000, 12000) * envelope(n, [(0, 0), (0.05, 0.5), (2.5, 0)])[:, None]
                    swell = noise(1.2, 200, 4000) * envelope(int(1.2 * SR), [(0, 0), (1.15, 0.6), (1.2, 0)])[:, None]
                    add(sfx, swell, t - 1.2, gain * 0.6)
                    add(sfx, boom.astype(np.float32), t, gain)
                    add(sfx, shimmer, t, gain * 0.35)
                elif kind == "first_breath":
                    add(sfx, breath(0.7, True, 300, 6000), t, gain * 0.9)
                    add(sfx, baby_cry(), t + 0.75, gain * 0.55)
                elif kind == "door_slam":
                    n = int(0.8 * SR)
                    tt = np.arange(n) / SR
                    thud = np.sin(2 * np.pi * 70 * tt) * np.exp(-tt * 12) + fft_filter(rng.standard_normal(n).astype(np.float32), 80, 2500) * np.exp(-tt * 25) * 0.6
                    add(sfx, thud.astype(np.float32), t, gain)
                elif kind == "phone_buzz":
                    n = int(0.55 * SR)
                    tt = np.arange(n) / SR
                    buzz = np.sign(np.sin(2 * np.pi * 160 * tt)) * 0.3 * (np.sin(2 * np.pi * 22 * tt) > 0)
                    add(sfx, fft_filter(buzz.astype(np.float32), 100, 1800), t, gain, 0.3)
                elif kind == "wave":
                    x = noise(4.0, 150, 9000, -2.0)
                    add(sfx, x * envelope(len(x), [(0, 0), (1.3, 1.0), (4.0, 0)])[:, None], t, gain)
                elif kind == "last_breath":
                    add(sfx, breath(1.8, False, 250, 3000), t, gain * 0.7)
        else:
            dur = cue["end"] - cue["start"]
            n = int(dur * SR)
            fade = [(0, 0.0), (min(0.8, dur / 3), 1.0), (dur - min(0.8, dur / 3), 1.0), (dur, 0.0)]
            if kind == "fluid":
                x = noise(dur, 30, 900, -4.0) * slow_lfo(n, 1.5, 0.35)[:, None]
                for _ in range(int(dur * 6)):  # Blaeschen / Mikrostroemung
                    bl = int(rng.uniform(0.02, 0.06) * SR)
                    tb = np.arange(bl) / SR
                    f = rng.uniform(300, 900) * (1 + tb * 8)
                    b = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-tb * 60) * rng.uniform(0.05, 0.15)
                    k = int(rng.uniform(0, dur) * SR)
                    x[k:k + bl, int(rng.integers(0, 2))] += b[: max(0, min(bl, n - k))]
            elif kind == "womb_pressure":
                x = noise(dur, 20, 160, -6.0) * envelope(n, [(0, 0.3), (dur, 1.0)])[:, None] * slow_lfo(n, 3.0, 0.4)[:, None]
            elif kind == "wind":
                x = noise(dur, 150, 3500, -3.0) * slow_lfo(n, 0.8, 0.45)[:, None]
            elif kind == "rain":
                x = noise(dur, 1500, 14000, -1.0) * 0.6
                for _ in range(int(dur * 120)):
                    k = int(rng.uniform(0, dur) * SR)
                    x[k:k + 40, int(rng.integers(0, 2))] += rng.uniform(0.1, 0.5) * np.exp(-np.arange(40) / 6.0)[: max(0, min(40, n - k))]
            elif kind == "cosmic_air":
                x = noise(dur, 80, 2500, -5.0) * slow_lfo(n, 0.4, 0.3)[:, None]
                tt = np.arange(n) / SR
                for f in (880.0, 1318.5, 1760.0):
                    x += (0.05 * np.sin(2 * np.pi * f * tt + rng.uniform(0, 6)) * slow_lfo(n, 0.3, 0.9, 0.5))[:, None]
            else:
                continue
            add(sfx, x * envelope(n, fade)[:, None], cue["start"], gain)

    # Tiefer Titel-Ton unter GENESIS
    n = int(4.0 * SR)
    tt = np.arange(n) / SR
    drone = (np.sin(2 * np.pi * 55.0 * tt) + 0.3 * np.sin(2 * np.pi * 110.0 * tt)) * envelope(n, [(0, 0), (1.5, 0.12), (3.0, 0.1), (4.0, 0)])
    add(sfx, drone.astype(np.float32), 114.0)


# ---------------------------------------------------------------- Voice-over (PLACEHOLDER)
def trim(x, thresh_db=-45.0):
    level = np.max(np.abs(x), axis=1)
    idx = np.where(level > 10 ** (thresh_db / 20.0))[0]
    if len(idx) == 0:
        return x
    a = max(0, idx[0] - int(0.02 * SR))
    b = min(len(x), idx[-1] + int(0.12 * SR))
    return x[a:b]


def build_vo(vo):
    report = []
    placed = []
    for line in DATA["voiceover"]:
        path = os.path.join(AUDIO, "VO", line["id"] + ".wav")
        if not os.path.exists(path):
            raise RuntimeError("VO fehlt: " + path + " (Generate-TempVO.ps1 ausfuehren)")
        x = trim(read_wav(path))
        x /= np.max(np.abs(x)) + 1e-9
        fx = line.get("fx")
        gain = 0.8
        if fx == "muffled":
            x = fft_filter(x, 60, 350) * 2.2
            gain = 0.5
        elif fx == "distant":
            x = fft_filter(x, 200, 1400)
            x = x * 0.5 + reverb(np.concatenate([x, np.zeros((SR, 2), np.float32)]), 1.6)[: len(x)] * 0.9
            gain = 0.45
        elif line["speaker"] == "child":
            gain = 0.55
        add(vo, x * 0.85 + reverb(np.concatenate([x, np.zeros((SR // 2, 2), np.float32)]), 0.6, 0.01, 5000)[: len(x)] * 0.12, line["start"], gain)
        dur = len(x) / SR
        placed.append((line["start"], line["start"] + dur))
        report.append((line["id"], line["start"], round(dur, 2), round(line["start"] + dur, 2)))
    return report, placed


# ---------------------------------------------------------------- Mix
def main():
    os.makedirs(OUT, exist_ok=True)
    print("Komponiere Soul Theme ...")
    compose()
    wet = reverb(music_send, 3.2, 0.03, 7000)
    mx = music + wet * 0.55
    # Harte Reduktion bei 113.0 (inkl. Hallfahne), Tod-Stille 87.7-94, Geburt-Stille 27-29.5
    gate = envelope(N, [(0, 1), (26.6, 1), (27.4, 0), (29.8, 0), (29.9, 1), (38.9, 1), (39.25, 0), (42.9, 0), (43.0, 1),
                        (87.6, 1), (88.0, 0), (93.9, 0), (94.0, 1), (113.0, 1), (113.06, 0), (999, 0)])
    mx *= gate[:, None]

    print("Sound Design ...")
    sfx = bus()
    build_sfx(sfx, gate)
    # Geburt: vor 27.5 alles gedaempft (Tiefpass), Oeffnung der vollen Frequenzbreite mit dem ersten Atemzug
    a, b = int(24.8 * SR), int(27.5 * SR)
    sfx[a:b] = fft_filter(sfx[a:b], 20, 320) * 1.6
    sfx[int(27.5 * SR):int(28.0 * SR)] = 0.0
    sfx[int(88.0 * SR):int(90.0 * SR)] = 0.0   # absolute Stille nach dem Tod
    sfx_wet = reverb(sfx, 1.4, 0.015, 5000, 11)
    sfx_mix = sfx + sfx_wet * 0.18
    for t0, t1 in [(27.5, 28.0), (88.0, 90.0)]:   # Hallfahnen in den Stille-Momenten ebenfalls entfernen
        sfx_mix[int(t0 * SR) - 2400:int(t0 * SR)] *= np.linspace(1, 0, 2400, dtype=np.float32)[:, None]
        sfx_mix[int(t0 * SR):int(t1 * SR)] = 0.0

    print("Voice-over ...")
    vo = bus()
    report, placed = build_vo(vo)

    # Ducking: Musik -5 dB unter der Stimme (weich)
    duck = np.ones(N, dtype=np.float32)
    for s, e in placed:
        duck[int((s - 0.15) * SR):int((e + 0.25) * SR)] = 0.56
    k = int(0.25 * SR)
    duck = np.convolve(duck, np.ones(k) / k, mode="same").astype(np.float32)
    mx *= duck[:, None]

    master = mx * 0.9 + sfx_mix * 0.8 + vo * 1.0
    master = master[: int(LENGTH * SR)]
    for t0, t1 in [(27.5, 28.0), (88.0, 90.0)]:   # Stille ist Dramaturgie: im Master erzwingen
        master[int(t0 * SR) - 1200:int(t0 * SR)] *= np.linspace(1, 0, 1200, dtype=np.float32)[:, None]
        master[int(t0 * SR):int(t1 * SR)] = 0.0
    peak = np.max(np.abs(master))
    master = np.tanh(master / peak * 1.15) / np.tanh(1.15) * 0.89   # weicher Limiter, -1 dBFS
    scale = 0.89 / peak * 1.0
    write_wav(os.path.join(OUT, "MX_SoulTheme_Trailer_Animatic.wav"), mx[: int(LENGTH * SR)] * scale)
    write_wav(os.path.join(OUT, "SFX_Trailer_Animatic.wav"), sfx_mix[: int(LENGTH * SR)] * scale)
    write_wav(os.path.join(OUT, "VO_Trailer_Placeholder.wav"), vo[: int(LENGTH * SR)] * scale)
    write_wav(os.path.join(OUT, "GENESIS_Trailer_AnimaticMix.wav"), master)

    print("\nVO-Timing (Start / Dauer / Ende):")
    for i, (vid, s, d, e) in enumerate(report):
        nxt = report[i + 1][1] if i + 1 < len(report) else LENGTH
        flag = "  <-- UEBERLAPPUNG" if e > nxt + 0.01 else ""
        print("  {:6s} {:7.2f} {:5.2f} {:7.2f}{}".format(vid, s, d, e, flag))
    rms = lambda x: 20 * np.log10(np.sqrt(np.mean(x ** 2)) + 1e-9)
    print("\nMaster: Spitzenpegel vor Limiter {:.2f}, RMS gesamt {:.1f} dBFS".format(peak, rms(master)))
    for t0, t1, label in [(0, 8, "Eroeffnung"), (27.5, 28.0, "Stille vor Geburt"), (88.0, 90.0, "Stille nach Tod"), (110, 113, "Hoehepunkt")]:
        print("  {:20s} RMS {:.1f} dBFS".format(label, rms(master[int(t0 * SR):int(t1 * SR)])))
    print("Ausgabe:", OUT)


if __name__ == "__main__":
    main()
