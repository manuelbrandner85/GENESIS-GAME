"""GENESIS – echte Klänge für Kreißsaal und Neugeborenes über kie.ai (Suno „sounds").

Jeder Klang hat seinen Anlass in der Simulation (GenesisSceneSpeech.cpp):
  SFX_Schrei_Erster    der erste Schrei nach der Geburt (Keuchen, Husten, dünner Schrei) – Stufe FirstBreaths
  SFX_Schrei_Stark     kräftiges Schreien – Schreilautstärke > 0,6 (Kälte, Hunger, Alleinsein)
  SFX_Schrei_Wimmern   Wimmern – leichte Unruhe (0,25–0,6)
  SFX_Baby_Laute       Grunzen, Seufzen, Schmatzen – ruhig auf der Haut der Mutter
  SFX_Kreisssaal_Nacht Raumklang als Schleife – vor der Geburt gedämpft, danach klar
Suno liefert je Auftrag zwei Varianten; beide werden verwendet (Abwechslung statt Wiederholung).
Prüfung: Tools/Audio/check_cry.py (Grundton 330–650 Hz, Schreidauer 0,3–2 s).
"""
import json

COMMON = " Realistic close-microphone field recording, natural, no music, no melody, no voice-over."
SOUNDS = [
    ("SFX_Schrei_Erster", False, "The very first cry of a newborn baby seconds after birth: a sharp gasp, a small wet cough "
                                 "and splutter, then thin, breathy, trembling cries that slowly get stronger. Hospital delivery room."),
    ("SFX_Schrei_Stark", False, "A newborn baby crying hard and rhythmically, one to two days old, cold and hungry, short gasping "
                                "breaths between the cries, quiet hospital room."),
    ("SFX_Schrei_Wimmern", False, "A newborn baby whimpering and fussing softly, short weak unsettled sounds and little sobs, "
                                  "not a full cry, very close and intimate."),
    ("SFX_Baby_Laute", False, "A calm newborn baby lying skin to skin on its mother's chest: tiny grunts, soft sighs, small "
                              "sucking and smacking sounds, quiet breathing. Very quiet and intimate."),
    # Ohne Geräte-Piepen: Die erste Fassung ("faint electronic monitor beeps") trug einen 1002-Hz-Ton mit +30 dB –
    # das Klingeln, das der Game Director hörte. Geprüft mit Tools/Audio/check_tones.py.
    ("SFX_Kreisssaal_Nacht", True, "Room tone of a quiet hospital delivery room at night: soft air conditioning and ventilation "
                                   "noise, the low hum of a building, very distant muffled corridor footsteps, the occasional "
                                   "rustle of a sheet. Absolutely no beeps, no tones, no alarms, no ringing, no electronic sounds."),
]


def jobs():
    return [{"name": name, "model": "ai-music-api/sounds",
             "input": {"prompt": prompt + COMMON, "model": "V5_5", "sound_loop": loop}}
            for name, loop, prompt in SOUNDS]


if __name__ == "__main__":
    print(json.dumps(jobs(), ensure_ascii=False, indent=1))
