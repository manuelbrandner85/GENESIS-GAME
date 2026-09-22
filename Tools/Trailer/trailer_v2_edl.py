# GENESIS Reveal Trailer v2 - Schnittliste (EDL). Einzige Quelle fuer Bildschnitt (build_trailer_v2_edit.py, Blender VSE)
# und Tonmischung (build_trailer_v2_audio.py). Zeiten in Sekunden, 24 fps, Laenge 120 s.
#
# Quellen:
#   GAME  = echte In-Engine-Aufnahme der gebauten Spielfassung (Genesis.exe, feste 24 fps, -dumpmovie, ohne HUD)
#   AI    = KONZEPTBILD (kie.ai: Nano Banana Pro + Kling 3.0), Lebensphasen, die noch nicht spielbar sind
#   STILL = KONZEPT-Standbild mit Kamerafahrt im Schnitt
#   BL    = Blender-Rendering (Cycles)
# Dramaturgie: Akt 1 Ursprung/Geburt (Spiel) - Akt 2 Leben - Akt 3 Alter/Tod - Akt 4 Jenseits/Wiedergeburt - Akt 5 Schoepfung/Titel

import os

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
FPS = 24
LENGTH = 120.0
CAP_A = os.path.join(REPO, "Genesis", "Saved", "Trailer", "Capture", "RunA_Chase")   # Verfolgerkamera, bis Geburtsbeginn
CAP_B = os.path.join(REPO, "Genesis", "Saved", "Trailer", "Capture", "RunB_Ego")     # halbe Ich-Perspektive (Rennen)
CAP_C = os.path.join(REPO, "Genesis", "Saved", "Trailer", "Capture", "RunD_Birth")   # Geburt direkt: Wehen vorgespult, Haut an Haut, Blick
CONCEPT = os.path.join(REPO, "ArtSource", "Generated", "Trailer", "Concept")
BLENDER = os.path.join(REPO, "Genesis", "Saved", "Trailer", "Blender")
AUDIO = os.path.join(REPO, "ArtSource", "Generated", "Trailer", "Audio")
GAME_AUDIO = os.path.join(REPO, "ArtSource", "Generated", "Audio")
VO_OLD = os.path.join(REPO, "Genesis", "Saved", "TrailerAudio", "VO_Takes")

# Frames von Lauf D (Bildpruefung): Kind verlaesst den Geburtskanal 1440, Blickkontakt 1618, Mutter "hallo" 1882
B_BORN = 1440
B_EYE = 1618
C_HALLO = 1882

# ---------------------------------------------------------------- Bild
# (start, ende, art, quelle, optionen)
#   game: quelle = (ordner, erster_frame, schrittweite)   -> Bildfolge (Schrittweite > 1 = Zeitraffer)
#   ai:   quelle = Datei, opt "offset" = Sekunden am Clipanfang ueberspringen
#   still:quelle = Datei, opt "zoom" (Start, Ende), "pan" (dx, dy in Pixel ueber die Dauer)
#   bl:   quelle = Ordner mit PNG-Folge
#   black / white
VIDEO = [
    # ---- Akt 1: Ursprung (Spiel)
    (0.0, 4.0, "black", None, {}),
    (4.0, 8.5, "game", (CAP_A, 20, 1), {"fade_in": 1.6}),                    # Anflug auf die Huelle der Eizelle
    (8.5, 12.0, "game", (CAP_B, 180, 1), {}),                                # halbe Ich-Perspektive: durch den Cumulus
    (12.0, 15.2, "game", (CAP_A, 300, 1), {}),                               # viele Zellen, eine Richtung
    (15.2, 17.0, "game", (CAP_A, 1406, 1), {"flash_out": 0.45}),             # die eigene Zelle zwischen den Coronazellen -> Befruchtung
    (17.0, 20.2, "game", (CAP_A, 1560, 6), {"flash_in": 0.5}),               # Zeitraffer: Vorkerne, 2 -> 4 -> 8 Zellen
    (20.2, 23.6, "game", (CAP_A, 2330, 7), {"fade_out": 0.8}),               # Blastozyste schluepft
    (23.6, 26.8, "game", (CAP_C, 300, 1), {"fade_in": 0.6, "fade_out": 0.6}),   # Wehen: Enge, Druck (Geburtskarte)
    (26.8, 29.0, "black", None, {}),                                          # STILLE vor der Geburt
    (29.0, 31.0, "game", (CAP_C, 1440, 1), {"flash_in": 0.45}),              # Licht - das Kind ist da, die Hebamme hebt es hoch
    (31.0, 33.0, "game", (CAP_C, 1600, 1), {}),                              # die Mutter kommt nah - der erste Blick
    (33.0, 38.2, "game", (CAP_C, 1955, 1), {"dissolve_in": 0.35}),           # "... oh mein Gott ... hallo ... hallo, du" (lippensynchron)
    # ---- Akt 2: Leben (Konzept)
    (38.2, 40.6, "ai", os.path.join(CONCEPT, "V_SH_A01_GrassRun.mp4"), {"dissolve_in": 0.5}),
    (40.6, 42.4, "ai", os.path.join(CONCEPT, "V_SH_A02_LiftedUp.mp4"), {}),
    (42.4, 44.6, "still", os.path.join(CONCEPT, "KF_SH_A03_EmptyBasket.png"), {"zoom": (1.0, 1.05)}),
    (44.6, 47.4, "ai", os.path.join(CONCEPT, "V_SH_A04_RainWindow.mp4"), {}),
    (47.4, 48.9, "still", os.path.join(CONCEPT, "KF_SH_A05_RooftopFriends.png"), {"zoom": (1.06, 1.0), "pan": (-30, 0)}),
    (48.9, 50.3, "still", os.path.join(CONCEPT, "KF_SH_A06_FirstLove.png"), {"zoom": (1.0, 1.08)}),
    (50.3, 53.3, "ai", os.path.join(CONCEPT, "V_SH_A07_Crossroads.mp4"), {}),
    (53.3, 54.8, "still", os.path.join(CONCEPT, "KF_SH_A08_HospitalWait.png"), {"zoom": (1.0, 1.07)}),
    (54.8, 56.1, "still", os.path.join(CONCEPT, "KF_SH_A09_WeddingRing.png"), {"zoom": (1.04, 1.0)}),
    (56.1, 57.4, "still", os.path.join(CONCEPT, "KF_SH_A10_BabyFinger.png"), {"zoom": (1.0, 1.05)}),
    (57.4, 58.5, "still", os.path.join(CONCEPT, "KF_SH_A11_EraMedieval.png"), {"zoom": (1.0, 1.06), "pan": (20, 0)}),
    (58.5, 59.8, "still", os.path.join(CONCEPT, "KF_SH_A12_EraFuture.png"), {"zoom": (1.08, 1.0)}),
    (59.8, 63.8, "ai", os.path.join(CONCEPT, "V_SH_A13_OldBench.mp4"), {"dissolve_in": 0.6}),
    # ---- Akt 3: Alter und Tod
    (63.8, 66.6, "still", os.path.join(CONCEPT, "KF_SH_A14_OldPhoto.png"), {"zoom": (1.0, 1.1)}),
    (66.6, 69.0, "ai", os.path.join(CONCEPT, "V_SH_A01_GrassRun.mp4"), {"offset": 1.5, "dissolve_in": 0.7, "dreamy": True}),
    (69.0, 72.0, "still", os.path.join(CONCEPT, "KF_SH_A16_Ghost.png"), {"zoom": (1.35, 1.42), "pan": (-260, 60), "desat": 0.35}),  # Sterbebett (nur Familie)
    (72.0, 76.0, "ai", os.path.join(CONCEPT, "V_SH_A15_LastBreath.mp4"), {"fade_out": 0.25}),
    (76.0, 78.2, "black", None, {}),                                          # ABSOLUTE STILLE
    # ---- Akt 4: Jenseits und Wiedergeburt
    (78.2, 81.2, "ai", os.path.join(CONCEPT, "V_SH_A16_Ghost.mp4"), {"fade_in": 0.8, "desat": 0.6}),
    (81.2, 83.2, "bl", os.path.join(BLENDER, "BL_Portal"), {}),
    (83.2, 87.2, "ai", os.path.join(CONCEPT, "V_SH_A17_Afterlife.mp4"), {}),
    (87.2, 89.0, "still", os.path.join(CONCEPT, "KF_SH_A18_DogReturns.png"), {"zoom": (1.0, 1.12)}),
    (89.0, 90.6, "game", (CAP_A, 60, 1), {"flash_in": 0.3, "glow": True}),   # ein Lichtpunkt faellt - eine neue Zelle, ein neues Leben
    (90.6, 93.8, "still", os.path.join(CONCEPT, "KF_SH_A19_NewBaby.png"), {"zoom": (1.0, 1.08)}),
    # ---- Akt 5: Schoepfung und Titel
    (93.8, 97.8, "bl", os.path.join(BLENDER, "BL_Galaxy"), {"dissolve_in": 0.5}),
    (97.8, 99.8, "still", os.path.join(CONCEPT, "KF_SH_A20_HandsPlanet.png"), {"zoom": (1.0, 1.1)}),
    (99.8, 103.8, "bl", os.path.join(BLENDER, "BL_Planet"), {}),
    (103.8, 105.3, "black", None, {}),                                        # harte Reduktion, ein Herzschlag
    (105.3, 110.8, "bl", os.path.join(BLENDER, "BL_Title"), {}),
    (110.8, 114.8, "hold", os.path.join(BLENDER, "BL_Title"), {"fade_out": 1.4}),  # Titel steht
    (114.8, 120.0, "black", None, {}),
]
END_CARD = (116.6, 119.6, "In-Engine-Aufnahmen aus GENESIS (Entwicklungsstand)  ·  Lebensphasen: Konzeptbilder")

# ---------------------------------------------------------------- Ton
# Musik: (trailer_start, trailer_ende, musik_start, gain, fade_in, fade_out)
MUSIC_FILE = os.path.join(AUDIO, "MX_Trailer_Epic_A.wav")
MUSIC = [
    (0.0, 26.8, 0.0, 1.0, 0.0, 0.5),
    (31.2, 76.6, 31.2, 1.0, 1.8, 1.4),
    (81.2, 103.8, 134.2, 1.0, 1.2, 0.02),        # Wiedereinstieg im Aufbau, harter Abriss bei 156 s der Musik = 103,8 s
    (106.2, 114.6, 157.0, 0.55, 0.3, 1.2),       # leiser Ausklang unter dem Titel
]
# Stimme: (start, datei, gain, effekt[, max_sekunden[, ab_sekunde]])   effekt: None | "muffled" | "distant" | "room" | "child"
VO = [
    (2.0, os.path.join(VO_OLD, "VO_01a_TAKE_A.wav"), 1.0, None),
    (6.4, os.path.join(VO_OLD, "VO_01b_TAKE_A.wav"), 1.0, None),
    (9.7, os.path.join(AUDIO, "VO_G_03_TAKE_A.wav"), 1.0, None),
    (13.1, os.path.join(AUDIO, "VO_G_04_TAKE_A.wav"), 1.0, None),
    (17.6, os.path.join(AUDIO, "VO_G_05_TAKE_A.wav"), 1.0, None),
    (22.3, os.path.join(AUDIO, "VO_G_06_TAKE_A.wav"), 1.0, None),
    (30.0, os.path.join(GAME_AUDIO, "SpeechNormalized", "D_H_Da_M.wav"), 0.75, "room", 1.6),
    (33.0 + (C_HALLO + 3.2 * 24 - 1955) / 24.0, os.path.join(GAME_AUDIO, "SpeechNormalized", "D_M_Hallo.wav"), 0.95, "room", 5.7, 3.2),   # ab "oh mein Gott", lippensynchron (Zeile beginnt bei Frame 1882)
    (39.2, os.path.join(VO_OLD, "VO_02_TAKE_A.wav"), 1.0, None),
    (42.8, os.path.join(VO_OLD, "VO_03_TAKE_A.wav"), 1.0, None),
    (50.6, os.path.join(VO_OLD, "VO_04_TAKE_A.wav"), 1.0, None),
    (53.4, os.path.join(VO_OLD, "VO_05_TAKE_A.wav"), 1.0, None),
    (59.0, os.path.join(VO_OLD, "VO_06_TAKE_A.wav"), 1.0, None),
    (61.9, os.path.join(VO_OLD, "VO_07_TAKE_A.wav"), 1.0, None),
    (65.2, os.path.join(VO_OLD, "VO_08_TAKE_A.wav"), 1.0, None),
    (68.2, os.path.join(VO_OLD, "VO_09_TAKE_A.wav"), 1.0, None),
    (78.8, os.path.join(VO_OLD, "VO_D1_TAKE_A.wav"), 0.6, "distant"),
    (87.4, os.path.join(VO_OLD, "VO_10_TAKE_A.wav"), 1.0, None),
    (91.2, os.path.join(VO_OLD, "VO_11_TAKE_A.wav"), 1.0, None),
    (94.6, os.path.join(VO_OLD, "VO_12_TAKE_A.wav"), 1.0, None),
    (100.4, os.path.join(VO_OLD, "VO_13_TAKE_A.wav"), 1.0, None),
    (107.4, os.path.join(AUDIO, "VO_G_09_TAKE_A.wav"), 1.0, None),
    (109.9, os.path.join(AUDIO, "VO_G_10_TAKE_A.wav"), 1.0, None),
    (115.4, os.path.join(VO_OLD, "VO_14_TAKE_A.wav"), 0.8, "child"),
]
# Geraeusche: (art, start, ende|None, gain)
SFX = [
    ("heartbeat", [0.7, 1.7, 2.7, 3.7, 4.7, 5.7, 6.7, 7.7], None, 0.7),
    ("fluid", 4.0, 17.0, 0.35),
    ("impulse", 16.5, None, 0.8),
    ("heartbeat_fetal", [17.4, 17.85, 18.3, 18.75, 19.2, 19.65, 20.1, 20.55, 21.0, 21.45, 21.9, 22.35, 22.8, 23.25], None, 0.45),
    ("womb", 23.6, 26.8, 0.7),
    ("heartbeat_fast", [23.8, 24.25, 24.65, 25.05, 25.4, 25.75, 26.1, 26.4], None, 0.75),
    ("cry", 29.15, None, 0.85),
    ("room", 31.4, 38.4, 0.35),
    ("wind", 38.2, 42.4, 0.18),
    ("rain", 44.6, 47.4, 0.35),
    ("wind", 59.8, 66.6, 0.28),
    ("heartbeat_slow", [72.4, 73.9, 75.5], None, 0.6),
    ("last_breath", 74.6, None, 0.8),
    ("room_muffled", 78.2, 81.2, 0.3),
    ("cosmic", 81.2, 103.8, 0.22),
    ("impulse", 89.0, None, 0.5),
    ("heartbeat", [104.3], None, 1.0),
]
SILENCE = [(26.8, 29.0), (76.0, 78.2)]        # erzwungen im Master (Dramaturgie)
