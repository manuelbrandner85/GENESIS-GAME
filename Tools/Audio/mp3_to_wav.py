"""GENESIS – wandelt mp3 (kie.ai) in 48-kHz-WAV für Unreal, mit Blenders Audiobibliothek (audaspace).

Aufruf: blender -b --factory-startup --python Tools/Audio/mp3_to_wav.py -- <eingangsordner> <ausgangsordner>
Mono bleibt mono, Stereo bleibt stereo. 24 bit, damit leise Stellen (Atmen, Flüstern) nicht rauschen.
"""
import os, sys
import aud

args = sys.argv[sys.argv.index("--") + 1:]
source, target = args[0], args[1]
os.makedirs(target, exist_ok=True)
for name in sorted(os.listdir(source)):
    if not name.lower().endswith(".mp3"):
        continue
    wav = os.path.join(target, os.path.splitext(name)[0] + ".wav")
    if os.path.exists(wav) and os.path.getmtime(wav) >= os.path.getmtime(os.path.join(source, name)):
        continue
    sound = aud.Sound(os.path.join(source, name))
    specs = sound.specs  # (rate, channels)
    channels = aud.CHANNELS_STEREO if specs[1] >= 2 else aud.CHANNELS_MONO
    sound.resample(48000, False).write(wav, rate=48000, channels=channels, format=aud.FORMAT_S24,
                                        container=aud.CONTAINER_WAV, codec=aud.CODEC_PCM)
    print("GENESIS wav:", wav)
