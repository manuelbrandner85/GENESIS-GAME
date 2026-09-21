# GENESIS: Der Kreislauf des Lebens
# Bringt die Erzählersätze auf einen gemeinsamen Pegel.
#
# Aufruf:
#   & "C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe" Tools\Frontend\normalize_voice.py
#
# Warum: Die Aufnahmen lagen zwischen −19,8 und −16,4 dBFS (Effektivwert der gesprochenen Teile).
# Neben einer fertig gemasterten Musik war der Erzähler damit zu leise, und von Satz zu Satz
# sprang die Lautstärke. Ziel sind −14 dBFS für jeden Satz – Spitzen werden weich begrenzt,
# damit dabei nichts übersteuert.

import glob
import os
import wave

import numpy as np

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
FOLDER = os.path.join(REPO, "ArtSource", "Generated", "Frontend", "Audio")
TARGET_DBFS = -14.0
CEILING = 0.93


def soft_limit(signal, ceiling):
    # Unterhalb von 80 % der Decke unverändert, darüber weich in die Sättigung
    knee = 0.8 * ceiling
    magnitude = np.abs(signal)
    over = magnitude > knee
    limited = signal.copy()
    limited[over] = np.sign(signal[over]) * (knee + (ceiling - knee) * np.tanh((magnitude[over] - knee) / (ceiling - knee)))
    return limited


def main():
    for path in sorted(glob.glob(os.path.join(FOLDER, "VO_Prolog_*.wav"))):
        with wave.open(path) as file:
            rate, channels, width = file.getframerate(), file.getnchannels(), file.getsampwidth()
            data = np.frombuffer(file.readframes(file.getnframes()), dtype=np.int16).astype(np.float64) / 32768.0
        mono = data.reshape(-1, channels).mean(axis=1)
        speech = mono[np.abs(mono) > 0.02]
        before = 20.0 * np.log10(np.sqrt(np.mean(speech ** 2)))
        gain = 10.0 ** ((TARGET_DBFS - before) / 20.0)
        result = soft_limit(data * gain, CEILING)

        check = result.reshape(-1, channels).mean(axis=1)
        after = 20.0 * np.log10(np.sqrt(np.mean(check[np.abs(check) > 0.02 * gain] ** 2)))
        with wave.open(path, "wb") as file:
            file.setnchannels(channels)
            file.setsampwidth(width)
            file.setframerate(rate)
            file.writeframes((result * 32767.0).astype(np.int16).tobytes())
        print("GENESIS: %-18s %6.1f -> %6.1f dBFS  (+%.1f dB)  Spitze %.2f" % (
            os.path.basename(path), before, after, 20.0 * np.log10(gain), np.max(np.abs(result))))


if __name__ == "__main__":
    main()
