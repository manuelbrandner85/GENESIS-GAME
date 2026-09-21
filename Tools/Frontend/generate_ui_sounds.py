# GENESIS: Der Kreislauf des Lebens
# Baut die Menüklänge (Auswahl, Bestätigen, Zurück) als kurze WAV-Dateien.
#
# Aufruf:
#   & "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" Tools\Frontend\generate_ui_sounds.py
#
# Die Klänge sollen zum Spiel passen, nicht zu einem Betriebssystem: kein Piepen, kein Klick aus
# Kunststoff, sondern etwas Weiches, Hölzernes, fast Körperliches – wie ein Fingerknöchel auf Holz
# und ein leise angeschlagenes Glas. Leise genug, dass man sie nach dem zehnten Mal nicht hasst.

import os
import wave

import numpy as np

RATE = 48000
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(REPO, "ArtSource", "Generated", "Frontend", "Audio")


def envelope(length, attack, decay):
    t = np.arange(length) / RATE
    env = np.minimum(t / max(attack, 1e-4), 1.0) * np.exp(-np.maximum(t - attack, 0.0) / decay)
    return env


def one_pole_lowpass(signal, cutoff):
    alpha = 1.0 - np.exp(-2.0 * np.pi * cutoff / RATE)
    out = np.zeros_like(signal)
    state = 0.0
    for index, value in enumerate(signal):
        state += alpha * (value - state)
        out[index] = state
    return out


def partials(length, frequencies, amplitudes, decays, attack=0.002):
    t = np.arange(length) / RATE
    result = np.zeros(length)
    for frequency, amplitude, decay in zip(frequencies, amplitudes, decays):
        result += amplitude * np.sin(2.0 * np.pi * frequency * t) * envelope(length, attack, decay)
    return result


def write(name, signal, peak=0.5):
    signal = signal / max(np.max(np.abs(signal)), 1e-9) * peak
    # Stereo mit einem Hauch Breite: rechts um 0,4 ms verzögert
    delay = int(0.0004 * RATE)
    left = signal
    right = np.concatenate([np.zeros(delay), signal[:-delay]])
    data = (np.stack([left, right], axis=1) * 32767.0).astype(np.int16)
    path = os.path.join(OUT, name + ".wav")
    with wave.open(path, "wb") as file:
        file.setnchannels(2)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(data.tobytes())
    print("GENESIS: %-10s %.3f s  Spitze %.2f" % (name, len(signal) / RATE, peak))


def main():
    os.makedirs(OUT, exist_ok=True)
    rng = np.random.default_rng(31)

    # Auswahl: ein kurzer, gedämpfter Holzton – Anschlag aus gefiltertem Rauschen plus zwei Moden
    length = int(0.11 * RATE)
    knock = one_pole_lowpass(rng.normal(size=length) * envelope(length, 0.0005, 0.006), 2400.0)
    body = partials(length, [640.0, 1710.0], [0.8, 0.25], [0.035, 0.012])
    write("UI_Move", knock * 0.6 + body, peak=0.32)

    # Bestätigen: ein leise angeschlagenes Glas – Grundton mit unharmonischen Obertönen, langes Ausklingen
    length = int(1.3 * RATE)
    fundamental = 659.26  # e''
    glass = partials(length, [fundamental, fundamental * 2.76, fundamental * 5.40, fundamental * 1.5],
                     [1.0, 0.32, 0.10, 0.28], [0.55, 0.22, 0.08, 0.40], attack=0.003)
    tap = one_pole_lowpass(rng.normal(size=length) * envelope(length, 0.0004, 0.004), 5000.0)
    write("UI_Accept", glass + tap * 0.3, peak=0.36)

    # Zurück: derselbe Charakter, eine Quarte tiefer und kürzer – man hört, dass es zurückgeht
    length = int(0.55 * RATE)
    fundamental = 493.88  # h'
    back = partials(length, [fundamental, fundamental * 2.76], [1.0, 0.25], [0.18, 0.06], attack=0.003)
    write("UI_Back", back, peak=0.30)


if __name__ == "__main__":
    main()
