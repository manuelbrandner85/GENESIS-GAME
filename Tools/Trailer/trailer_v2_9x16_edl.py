# GENESIS Reveal Trailer v2 - Hochformat 9:16 (TikTok / Reels / Shorts), ca. 46 s.
# Eigener Schnitt, keine verkleinerte Fassung: kuerzere Einstellungen, jede Einstellung fuer das Hochformat neu
# ausgerichtet ("focus" = horizontale Position des Motivs in der 16:9-Quelle, wird in die Bildmitte geschoben),
# Titel eigens im Hochformat gerendert (BL_Title_9x16).
# Aufruf: $env:GENESIS_EDL = "trailer_v2_9x16_edl"; dann build_trailer_v2_audio.py und build_trailer_v2_edit.py

import os

from trailer_v2_edl import AUDIO, BLENDER, CAP_A, CAP_B, CAP_C, CONCEPT, FPS, GAME_AUDIO, REPO, VO_OLD, C_HALLO  # noqa: F401

NAME = "GENESIS_Trailer_v2_9x16"
WIDTH, HEIGHT = 1080, 1920
BARS = False
LENGTH = 46.4

VIDEO = [
    (0.0, 2.0, "black", None, {}),
    (2.0, 5.0, "game", (CAP_A, 20, 1), {"fade_in": 1.0, "focus": 0.42}),
    (5.0, 7.6, "game", (CAP_B, 180, 1), {"focus": 0.5}),
    (7.6, 9.0, "game", (CAP_A, 1406, 1), {"flash_out": 0.4, "focus": 0.5}),
    (9.0, 11.4, "game", (CAP_A, 1560, 8), {"flash_in": 0.4, "focus": 0.5}),
    (11.4, 13.4, "game", (CAP_C, 300, 1), {"fade_out": 0.5, "focus": 0.45}),
    (13.4, 14.4, "black", None, {}),
    (14.4, 15.8, "game", (CAP_C, 1440, 1), {"flash_in": 0.4, "focus": 0.55}),
    (15.8, 20.4, "game", (CAP_C, 1955, 1), {"focus": 0.5}),
    (20.4, 22.2, "ai", os.path.join(CONCEPT, "V_SH_A01_GrassRun.mp4"), {"focus": 0.37}),
    (22.2, 23.6, "still", os.path.join(CONCEPT, "KF_SH_A03_EmptyBasket.png"), {"zoom": (1.0, 1.05), "focus": 0.45}),
    (23.6, 25.4, "ai", os.path.join(CONCEPT, "V_SH_A04_RainWindow.mp4"), {"focus": 0.66}),
    (25.4, 26.6, "still", os.path.join(CONCEPT, "KF_SH_A06_FirstLove.png"), {"zoom": (1.0, 1.08), "focus": 0.5}),
    (26.6, 28.4, "ai", os.path.join(CONCEPT, "V_SH_A07_Crossroads.mp4"), {"focus": 0.5}),
    (28.4, 31.2, "ai", os.path.join(CONCEPT, "V_SH_A13_OldBench.mp4"), {"focus": 0.47}),
    (31.2, 33.0, "ai", os.path.join(CONCEPT, "V_SH_A15_LastBreath.mp4"), {"fade_out": 0.25, "focus": 0.58}),
    (33.0, 34.0, "black", None, {}),
    (34.0, 35.8, "ai", os.path.join(CONCEPT, "V_SH_A17_Afterlife.mp4"), {"fade_in": 0.4, "focus": 0.62}),
    (35.8, 38.0, "bl", os.path.join(BLENDER, "BL_Planet"), {"focus": 0.4, "offset": 1.8}),
    (38.0, 38.8, "black", None, {}),
    (38.8, 44.3, "bl", os.path.join(BLENDER, "BL_Title_9x16"), {}),
    (44.3, 45.6, "hold", os.path.join(BLENDER, "BL_Title_9x16"), {"fade_out": 1.2}),
    (45.6, 46.4, "black", None, {}),
]
END_CARD = (41.8, 45.2, "In-Engine-Aufnahmen · Lebensphasen: Konzeptbilder")
END_CARD_POS = (0.5, 0.12)
END_CARD_SIZE = 30

MUSIC_FILE = os.path.join(AUDIO, "MX_Trailer_Epic_A.wav")
MUSIC = [
    (0.0, 13.4, 0.0, 1.0, 0.0, 0.4),
    (20.4, 38.0, 138.4, 1.0, 0.8, 0.02),         # Aufbau -> Hoehepunkt -> harter Abriss (156 s der Musik = 38,0 s)
    (38.8, 45.8, 157.0, 0.55, 0.3, 1.0),
]
VO = [
    (0.6, os.path.join(VO_OLD, "VO_01a_TAKE_A.wav"), 1.0, None),
    (5.0, os.path.join(VO_OLD, "VO_01b_TAKE_A.wav"), 1.0, None),
    (9.3, os.path.join(AUDIO, "VO_G_06_TAKE_A.wav"), 1.0, None),
    (14.9, os.path.join(GAME_AUDIO, "SpeechNormalized", "D_H_Da_M.wav"), 0.75, "room", 0.9),
    (15.8 + (C_HALLO + 3.2 * 24 - 1955) / 24.0, os.path.join(GAME_AUDIO, "SpeechNormalized", "D_M_Hallo.wav"), 0.95, "room", 4.9, 3.2),
    (21.2, os.path.join(VO_OLD, "VO_02_TAKE_A.wav"), 1.0, None),
    (23.1, os.path.join(VO_OLD, "VO_03_TAKE_A.wav"), 1.0, None),
    (26.7, os.path.join(VO_OLD, "VO_04_TAKE_A.wav"), 1.0, None),
    (28.9, os.path.join(VO_OLD, "VO_08_TAKE_A.wav"), 1.0, None),
    (35.9, os.path.join(VO_OLD, "VO_12_TAKE_A.wav"), 1.0, None),
    (38.9, os.path.join(VO_OLD, "VO_13_TAKE_A.wav"), 1.0, None),
    (44.4, os.path.join(VO_OLD, "VO_14_TAKE_A.wav"), 0.8, "child"),
]
SFX = [
    ("heartbeat", [0.4, 1.4, 2.4, 3.4, 4.4], None, 0.7),
    ("fluid", 2.0, 9.0, 0.35),
    ("impulse", 8.8, None, 0.8),
    ("heartbeat_fetal", [9.3, 9.75, 10.2, 10.65, 11.1], None, 0.45),
    ("womb", 11.4, 13.4, 0.7),
    ("heartbeat_fast", [11.6, 12.0, 12.4, 12.8, 13.15], None, 0.75),
    ("cry", 14.45, None, 0.85),
    ("room", 15.6, 20.6, 0.35),
    ("wind", 20.4, 22.2, 0.18),
    ("rain", 23.6, 25.4, 0.35),
    ("wind", 28.4, 31.2, 0.28),
    ("heartbeat_slow", [31.4, 32.5], None, 0.6),
    ("last_breath", 31.9, None, 0.8),
    ("cosmic", 34.0, 38.0, 0.22),
    ("heartbeat", [38.3], None, 1.0),
]
SILENCE = [(13.4, 14.4), (33.0, 34.0)]
