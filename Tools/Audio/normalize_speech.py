"""GENESIS – gleicht die Lautheit der Sprachaufnahmen an (Sprechpegel −20 dBFS RMS, Spitze höchstens −1 dBFS).

Gemessen wird nur, wo gesprochen wird (Blöcke über −45 dBFS) – Pausen dürfen den Pegel nicht verfälschen.
Geflüstertes bleibt leiser als Gesprochenes: Es wird nur bis −26 dBFS angehoben, sonst klänge ein
Flüstern wie ein Ruf.
Aufruf: python Tools/Audio/normalize_speech.py <eingang> <ausgang>
"""
import math, os, struct, sys, wave

TARGET = -20.0
WHISPER_TARGET = -26.0
PEAK = 10 ** (-1.0 / 20)
WHISPER_IDS = {"S_M_Geschafft", "E_M_Blick_01", "Z_M_Schlaf"}

source, target = sys.argv[1], sys.argv[2]
os.makedirs(target, exist_ok=True)
for name in sorted(os.listdir(source)):
    if not name.endswith(".wav"):
        continue
    with wave.open(os.path.join(source, name)) as w:
        params = w.getparams()
        raw = w.readframes(params.nframes)
    samples = list(struct.unpack("<%dh" % (len(raw) // 2), raw))
    block = params.framerate // 50 * params.nchannels
    active = []
    for i in range(0, len(samples) - block, block):
        chunk = samples[i:i + block]
        rms = math.sqrt(sum(s * s for s in chunk) / block) / 32768.0
        if rms > 10 ** (-45 / 20):
            active.append(rms * rms)
    level = 10 * math.log10(sum(active) / len(active)) if active else -90.0
    goal = WHISPER_TARGET if name[:-4] in WHISPER_IDS else TARGET
    gain = 10 ** ((goal - level) / 20)
    peak = max(abs(s) for s in samples) / 32768.0
    gain = min(gain, PEAK / max(peak, 1e-6))
    out = [max(-32768, min(32767, int(round(s * gain)))) for s in samples]
    with wave.open(os.path.join(target, name), "wb") as w:
        w.setparams(params)
        w.writeframes(struct.pack("<%dh" % len(out), *out))
    print("%-16s Sprechpegel %6.1f dBFS -> %+5.1f dB" % (name[:-4], level, 20 * math.log10(gain)))
