# GENESIS Trailer - Animatic v1 (Blender Video Sequence Editor, headless).
# Baut aus Docs/Trailer/ShotList.json eine komplette 120-s-Animatic mit Shot-Boards, animierter Kamera-Andeutung,
# Voice-over-Untertiteln, Uebergaengen, Timecode und dem Tonmix (Genesis/Saved/TrailerAudio/Mix).
#
# Aufruf:
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup --python Tools\Trailer\Blender\build_animatic.py -- [--stills]
# Ausgabe: Genesis/Saved/Trailer/Animatic/GENESIS_Trailer_Animatic_v1.mp4 (+ Stills fuer die Pruefung)

import json
import math
import os
import random
import sys
import wave

import bpy

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
DATA = json.load(open(os.path.join(REPO, "Docs", "Trailer", "ShotList.json"), encoding="utf-8"))
MIX = os.path.join(REPO, "Genesis", "Saved", "TrailerAudio", "Mix", "GENESIS_Trailer_AnimaticMix.wav")
VO_DIR = os.path.join(REPO, "Genesis", "Saved", "TrailerAudio", "VO")
OUT_DIR = os.path.join(REPO, "Genesis", "Saved", "Trailer", "Animatic")
FPS = int(DATA["fps"])
W, H = 1920, 1080
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
random.seed(1306)

SEQ_COLOR = {s["id"]: s["color"] for s in DATA["sequences"]}
BLACK_SHOTS = {"SH_001_010_Darkness", "SH_003_020_Silence", "SH_008_050_Silence", "SH_012_010_Heartbeat",
               "SH_012_020_TitleReveal", "SH_012_030_KnowEachOther"}


def frame(t):
    return int(round(t * FPS)) + 1


def color_strip(se, name, ch, t0, t1, rgb, alpha=1.0):
    s = se.strips.new_effect(name=name, type="COLOR", channel=ch, frame_start=frame(t0), length=max(1, frame(t1) - frame(t0)))
    s.color = rgb[:3]
    s.blend_type = "ALPHA_OVER"
    s.blend_alpha = alpha
    return s


def text_strip(se, name, ch, t0, t1, text, size, loc, anchor=("CENTER", "CENTER"), color=(1, 1, 1, 1), wrap=0.9, box=False, bold=False):
    s = se.strips.new_effect(name=name, type="TEXT", channel=ch, frame_start=frame(t0), length=max(1, frame(t1) - frame(t0)))
    s.text = text
    s.font_size = size
    s.location = loc
    s.anchor_x, s.anchor_y = anchor
    s.alignment_x = anchor[0] if anchor[0] in ("LEFT", "CENTER", "RIGHT") else "CENTER"
    s.color = color
    s.wrap_width = wrap
    s.use_shadow = True
    s.shadow_color = (0, 0, 0, 0.8)
    s.use_bold = bold
    if box:
        s.use_box = True
        s.box_color = (0, 0, 0, 0.55)
        s.box_margin = 0.012
    s.blend_type = "ALPHA_OVER"
    return s


def key(strip, prop, f, value):
    setattr(strip.transform, prop, value)
    strip.transform.keyframe_insert(prop, frame=f)


def animate_camera(strip, shot):
    """Kamerabewegung als Bewegung des Bildausschnitts (Board-Animatic)."""
    cam = shot["cam"]
    move, amount = cam.get("move", "static"), float(cam.get("amount", 0.1))
    f0, f1 = frame(shot["start"]), frame(shot["end"]) - 1
    tr = strip.transform
    base = 0.62
    for p, v in (("scale_x", base), ("scale_y", base), ("offset_x", 0.0), ("offset_y", 0.0), ("rotation", 0.0)):
        setattr(tr, p, v)
    if move == "dolly_in" or move == "handheld_push":
        key(strip, "scale_x", f0, base); key(strip, "scale_y", f0, base)
        key(strip, "scale_x", f1, base * (1 + amount * 2.5)); key(strip, "scale_y", f1, base * (1 + amount * 2.5))
    elif move == "dolly_out":
        key(strip, "scale_x", f0, base * (1 + amount * 2.5)); key(strip, "scale_y", f0, base * (1 + amount * 2.5))
        key(strip, "scale_x", f1, base); key(strip, "scale_y", f1, base)
    elif move in ("track_left", "track_right", "track_follow"):
        d = -1 if move == "track_left" else 1
        key(strip, "offset_x", f0, -d * amount * 500); key(strip, "offset_x", f1, d * amount * 500)
    elif move in ("crane_up", "crane_down"):
        d = 1 if move == "crane_up" else -1
        key(strip, "offset_y", f0, d * amount * 350); key(strip, "offset_y", f1, -d * amount * 350)
    elif move in ("orbit", "roll_open", "handheld_orbit"):
        key(strip, "rotation", f0, 0.0); key(strip, "rotation", f1, math.radians(amount))
    elif move == "pullback_cosmic":
        key(strip, "scale_x", f0, 2.2); key(strip, "scale_y", f0, 2.2)
        key(strip, "scale_x", f1, 0.03); key(strip, "scale_y", f1, 0.03)
    elif move == "static_drift":
        key(strip, "offset_x", f0, -15.0); key(strip, "offset_x", f1, 15.0)
    if move.startswith("handheld"):
        for f in range(f0, f1, 3):
            key(strip, "offset_x", f, random.uniform(-12, 12))
            key(strip, "offset_y", f, random.uniform(-9, 9))
    if strip.id_data.animation_data and strip.id_data.animation_data.action:
        for fc in strip.id_data.animation_data.action.fcurves if hasattr(strip.id_data.animation_data.action, "fcurves") else []:
            for kp in fc.keyframe_points:
                kp.interpolation = "BEZIER"


def fade(se, name, ch, t0, t1, rgb, a0, a1):
    s = color_strip(se, name, ch, t0, t1, rgb, a0)
    s.keyframe_insert("blend_alpha", frame=frame(t0))
    s.blend_alpha = a1
    s.keyframe_insert("blend_alpha", frame=frame(t1) - 1)
    return s


def wav_seconds(path):
    with wave.open(path) as w:
        return w.getnframes() / float(w.getframerate())


def main():
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y, scene.render.resolution_percentage = W, H, 100
    scene.render.fps, scene.render.fps_base = FPS, 1.0
    scene.frame_start, scene.frame_end = 1, frame(DATA["duration"]) - 1
    se = scene.sequence_editor_create()

    for i, shot in enumerate(DATA["shots"]):
        t0, t1 = shot["start"], shot["end"]
        black = shot["id"] in BLACK_SHOTS
        rgb = (0, 0, 0) if black else SEQ_COLOR[shot["seq"]]
        color_strip(se, shot["id"] + "_BG", 1, t0, t1, rgb)
        if not black:
            lighter = tuple(min(1.0, c * 1.9 + 0.08) for c in rgb)
            frame_strip = color_strip(se, shot["id"] + "_CAM", 2, t0, t1, lighter, 0.55)
            animate_camera(frame_strip, shot)
            text_strip(se, shot["id"] + "_DESC", 4, t0, t1, shot["desc"], 40, (0.5, 0.56), wrap=0.62)
            cam = shot["cam"]
            cam_line = "KAMERA  {}  |  {} mm  |  {}  |  Fokus: {}".format(cam["rig"], cam["lens"], cam["move"], cam["focus"])
            text_strip(se, shot["id"] + "_CAMTXT", 5, t0, t1, cam_line, 24, (0.03, 0.2), ("LEFT", "BOTTOM"), (0.85, 0.85, 0.85, 1), 0.9)
        text_strip(se, shot["id"] + "_ID", 6, t0, t1, "{}   {:.1f}-{:.1f} s   {}".format(shot["id"], t0, t1, shot["seq"]), 24,
                   (0.03, 0.96), ("LEFT", "TOP"), (0.9, 0.9, 0.9, 0.9), 0.9)
        badge = "GAMEPLAY-ZIEL ({})".format(shot.get("gameplay_block", "")) if shot["source"] == "GAMEPLAY_TARGET" else "CINEMATIC CONCEPT"
        text_strip(se, shot["id"] + "_SRC", 7, t0, t1, badge, 22, (0.97, 0.96), ("RIGHT", "TOP"), (1.0, 0.8, 0.35, 0.9), 0.5)

        tr = shot.get("transition", "")
        if "Aufblende" in tr and "Weiss" not in tr:
            fade(se, shot["id"] + "_FADEIN", 9, t0, min(t1, t0 + 3.0), (0, 0, 0), 1.0, 0.0)
        if "Weiss" in tr:
            fade(se, shot["id"] + "_WHITE", 9, max(t0, t1 - 0.35), t1, (1, 1, 1), 0.0, 1.0)
        if "Ausblende" in tr:
            fade(se, shot["id"] + "_FADEOUT", 9, max(t0, t1 - 1.0), t1, (0, 0, 0), 0.0, 1.0)

    # Titel (Animatic-Andeutung, finales Title Design folgt in Unreal)
    gold = (0.86, 0.70, 0.38, 1.0)
    text_strip(se, "TITLE_GENESIS", 10, 114.2, 119.6, "G E N E S I S", 120, (0.5, 0.55), color=gold, wrap=1.0, bold=True)
    text_strip(se, "TITLE_SUB", 11, 115.0, 119.6, "DER KREISLAUF DES LEBENS", 44, (0.5, 0.43), color=(0.9, 0.85, 0.75, 1), wrap=1.0)
    text_strip(se, "TITLE_TAG", 12, 116.2, 119.6, "JEDE ENTSCHEIDUNG HINTERLÄSST EIN ECHO.", 30, (0.5, 0.34), color=(0.8, 0.8, 0.8, 1), wrap=1.0)
    fade(se, "TITLE_FADEOUT", 13, 119.4, DATA["duration"], (0, 0, 0), 0.0, 1.0)

    # Voice-over-Untertitel (Laenge aus den gewaehlten Takes)
    for line in DATA["voiceover"]:
        path = os.path.join(VO_DIR, line["id"] + ".wav")
        dur = wav_seconds(path) if os.path.exists(path) else 2.5
        prefix = {"mother": "MUTTER (gedaempft): ", "daughter": "TOCHTER (fern): ", "child": "KIND: "}.get(line["speaker"], "")
        text_strip(se, line["id"] + "_SUB", 14, line["start"], min(DATA["duration"], line["start"] + dur + 0.2),
                   prefix + line["text"], 46, (0.5, 0.08), ("CENTER", "BOTTOM"), (1, 1, 1, 1), 0.8, box=True)

    text_strip(se, "WATERMARK", 15, 0.0, DATA["duration"], "GENESIS REVEAL TRAILER - ANIMATIC v1 - PLATZHALTER, NICHT FINAL", 18,
               (0.5, 0.995), ("CENTER", "TOP"), (1, 1, 1, 0.45), 1.0)

    if os.path.exists(MIX):
        snd = se.strips.new_sound(name="MIX", filepath=MIX, channel=16, frame_start=1)
    else:
        print("WARNUNG: Mix fehlt", MIX)

    scene.render.use_stamp = True
    scene.render.use_stamp_date = scene.render.use_stamp_render_time = scene.render.use_stamp_camera = False
    scene.render.use_stamp_lens = scene.render.use_stamp_scene = scene.render.use_stamp_filename = False
    scene.render.use_stamp_time = True
    scene.render.use_stamp_frame = True
    scene.render.stamp_font_size = 20

    os.makedirs(OUT_DIR, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, "GENESIS_Trailer_Animatic_v1.blend"))

    if "--stills" in ARGS:
        scene.render.image_settings.file_format = "PNG"
        for t in (2.0, 13.0, 29.0, 36.5, 42.0, 50.0, 56.9, 64.5, 71.0, 87.0, 97.0, 108.5, 116.8):
            scene.frame_set(frame(t))
            scene.render.filepath = os.path.join(OUT_DIR, "stills", "animatic_{:06.1f}s.png".format(t))
            bpy.ops.render.render(write_still=True)
        return

    scene.render.image_settings.media_type = "VIDEO"
    scene.render.image_settings.file_format = "FFMPEG"
    ff = scene.render.ffmpeg
    ff.format, ff.codec = "MPEG4", "H264"
    ff.constant_rate_factor = "HIGH"
    ff.ffmpeg_preset = "GOOD"
    ff.audio_codec, ff.audio_bitrate, ff.audio_mixrate, ff.audio_channels = "AAC", 256, 48000, "STEREO"
    scene.render.filepath = os.path.join(OUT_DIR, "GENESIS_Trailer_Animatic_v1.mp4")
    bpy.ops.render.render(animation=True)
    print("GENESIS_ANIMATIC_OK", scene.render.filepath)


main()
