# GENESIS Reveal Trailer v2 - Quadrat 1:1 (Instagram / Facebook Feed), Teaser ca. 33 s.
# Nur die staerksten Momente: Ursprung, Geburt und Mutter, drei Lebensbilder, Tod, Jenseits, Planet, Titel mit Tagline.
# Aufruf: $env:GENESIS_EDL = "trailer_v2_1x1_edl"; dann build_trailer_v2_audio.py und build_trailer_v2_edit.py

import os

from trailer_v2_edl import AUDIO, BLENDER, CAP_A, CAP_B, CAP_C, CONCEPT, FPS, GAME_AUDIO, REPO, VO_OLD, C_HALLO  # noqa: F401

NAME = "GENESIS_Trailer_v2_1x1"
WIDTH, HEIGHT = 1080, 1080
BARS = False
LENGTH = 33.0

VIDEO = [
    (0.0, 1.5, "black", None, {}),
    (1.5, 4.5, "game", (CAP_A, 20, 1), {"fade_in": 0.8, "focus": 0.42}),
    (4.5, 6.0, "game", (CAP_A, 1406, 1), {"flash_out": 0.4, "focus": 0.5}),
    (6.0, 8.4, "game", (CAP_A, 1560, 8), {"flash_in": 0.4, "fade_out": 0.4, "focus": 0.5}),
    (8.4, 9.4, "black", None, {}),
    (9.4, 10.8, "game", (CAP_C, 1440, 1), {"flash_in": 0.4, "focus": 0.55}),
    (10.8, 15.4, "game", (CAP_C, 1955, 1), {"focus": 0.5}),
    (15.4, 17.2, "ai", os.path.join(CONCEPT, "V_SH_A01_GrassRun.mp4"), {"focus": 0.37}),
    (17.2, 18.8, "ai", os.path.join(CONCEPT, "V_SH_A04_RainWindow.mp4"), {"focus": 0.62}),
    (18.8, 21.0, "ai", os.path.join(CONCEPT, "V_SH_A13_OldBench.mp4"), {"focus": 0.47}),
    (21.0, 22.6, "ai", os.path.join(CONCEPT, "V_SH_A15_LastBreath.mp4"), {"fade_out": 0.25, "focus": 0.58}),
    (22.6, 23.2, "black", None, {}),
    (23.2, 25.0, "ai", os.path.join(CONCEPT, "V_SH_A17_Afterlife.mp4"), {"fade_in": 0.3, "focus": 0.6}),
    (25.0, 26.6, "bl", os.path.join(BLENDER, "BL_Planet"), {"focus": 0.4, "offset": 1.8}),
    (26.6, 27.2, "black", None, {}),
    (27.2, 32.7, "bl", os.path.join(BLENDER, "BL_Title_1x1"), {}),
    (32.7, 33.0, "hold", os.path.join(BLENDER, "BL_Title_1x1"), {"fade_out": 0.3}),
]
END_CARD = (30.2, 32.9, "In-Engine-Aufnahmen · Lebensphasen: Konzeptbilder")
END_CARD_POS = (0.5, 0.08)
END_CARD_SIZE = 24

MUSIC_FILE = os.path.join(AUDIO, "MX_Trailer_Epic_A.wav")
MUSIC = [
    (0.0, 8.4, 0.0, 1.0, 0.0, 0.4),
    (15.4, 26.6, 144.9, 1.0, 0.6, 0.02),          # harter Abriss (156 s der Musik) genau vor dem Titel
    (27.2, 33.0, 157.0, 0.55, 0.3, 0.6),
]
VO = [
    (0.4, os.path.join(VO_OLD, "VO_01a_TAKE_A.wav"), 1.0, None),
    (6.2, os.path.join(AUDIO, "VO_G_06_TAKE_A.wav"), 1.0, None),
    (10.8 + (C_HALLO + 3.2 * 24 - 1955) / 24.0, os.path.join(GAME_AUDIO, "SpeechNormalized", "D_M_Hallo.wav"), 0.95, "room", 4.4, 3.2),
    (15.6, os.path.join(VO_OLD, "VO_02_TAKE_A.wav"), 1.0, None),
    (17.8, os.path.join(VO_OLD, "VO_03_TAKE_A.wav"), 1.0, None),
    (27.8, os.path.join(AUDIO, "VO_G_09_TAKE_A.wav"), 1.0, None),
    (30.1, os.path.join(AUDIO, "VO_G_10_TAKE_A.wav"), 1.0, None),
]
SFX = [
    ("heartbeat", [0.3, 1.3, 2.3, 3.3], None, 0.7),
    ("fluid", 1.5, 6.0, 0.35),
    ("impulse", 5.8, None, 0.8),
    ("heartbeat_fetal", [6.3, 6.75, 7.2, 7.65], None, 0.45),
    ("cry", 9.45, None, 0.85),
    ("room", 10.6, 15.6, 0.35),
    ("rain", 17.2, 18.8, 0.3),
    ("wind", 18.8, 21.0, 0.28),
    ("last_breath", 21.6, None, 0.8),
    ("cosmic", 23.2, 26.6, 0.22),
    ("heartbeat", [26.9], None, 1.0),
]
SILENCE = [(8.4, 9.4), (22.6, 23.2)]
