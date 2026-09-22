# GENESIS Reveal Trailer v2 - Bildschnitt in Blender (VSE, headless) nach Tools/Trailer/trailer_v2_edl.py.
#
# Aufruf:
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup --python Tools\Trailer\Blender\build_trailer_v2_edit.py -- [--stills] [--master]
# Ausgabe: Genesis/Saved/Trailer/v2/GENESIS_Trailer_v2_1080p.mp4 (Web), mit --master zusaetzlich ProRes 422 HQ (.mov)
#
# Look: Kinobalken 2,39:1, eine dezente Farbkorrektur fuer alles (warme Lichter, neutrale Tiefen), keine Effektfilter.

import os
import sys

import bpy

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
import importlib
edl = importlib.import_module(os.environ.get("GENESIS_EDL", "trailer_v2_edl"))   # andere Fassungen: trailer_v2_social_edl ...

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
FPS = edl.FPS
W, H = getattr(edl, "WIDTH", 1920), getattr(edl, "HEIGHT", 1080)
NAME = getattr(edl, "NAME", "GENESIS_Trailer_v2")
OUT = os.path.join(edl.REPO, "Genesis", "Saved", "Trailer", "v2")
MIX = os.path.join(OUT, NAME + "_Mix.wav")
BAR = int(round((H - W / 2.39) / 2)) if getattr(edl, "BARS", True) else 0


def fr(t):
    return int(round(t * FPS)) + 1


def color(se, name, ch, t0, t1, rgb, alpha=1.0):
    s = se.strips.new_effect(name=name, type="COLOR", channel=ch, frame_start=fr(t0), length=max(1, fr(t1) - fr(t0)))
    s.color = rgb
    s.blend_type = "ALPHA_OVER"
    s.blend_alpha = alpha
    return s


def key_alpha(s, pairs):
    for t, a in pairs:
        s.blend_alpha = a
        s.keyframe_insert("blend_alpha", frame=fr(t))


def game_frames(src, opts, t0, t1):
    folder, first, stride = src
    if first is None:
        tag = opts["b_frame"]
        anchor, delta = (tag.split("-") + ["0"])[:2] if "-" in tag else (tag, "0")
        base_frame = edl.B_BORN if anchor == "born" else edl.B_EYE
        first = base_frame - int(delta)
    count = fr(t1) - fr(t0)
    return folder, ["MovieFrame%05d.png" % (first + i * stride) for i in range(count)]


def add_shot(se, i, shot, prev):
    t0, t1, kind, src, opts = shot
    ch = 2 + i
    d_in = opts.get("dissolve_in", 0.0)
    length = fr(t1) - fr(t0)
    s = None
    if kind == "game":
        folder, files = game_frames(src, opts, t0, t1)
        missing = [f for f in files if not os.path.exists(os.path.join(folder, f))]
        if missing:
            print("WARNUNG: %d fehlende Frames in %s ab %s" % (len(missing), folder, missing[0]))
            files = [f for f in files if f not in missing] or files[:1]
        s = se.strips.new_image(name="S%02d_game" % i, filepath=os.path.join(folder, files[0]), channel=ch,
                                frame_start=fr(t0), fit_method="FILL")
        for f in files[1:]:
            s.elements.append(f)
        s.frame_final_duration = length
    elif kind == "ai":
        s = se.strips.new_movie(name="S%02d_ai" % i, filepath=src, channel=ch, frame_start=fr(t0) - int(opts.get("offset", 0) * FPS),
                                fit_method="FILL")
        s.frame_offset_start = int(opts.get("offset", 0) * FPS)
        s.frame_start = fr(t0) - s.frame_offset_start
        avail = s.frame_duration - s.frame_offset_start
        s.frame_final_duration = min(length, avail)
        if avail < length:
            print("HINWEIS: Clip %s kuerzer als Einstellung (%d < %d Frames) - letztes Bild steht" % (os.path.basename(src), avail, length))
    elif kind in ("still", "hold"):
        path = src if kind == "still" else sorted(p for p in (os.path.join(src, f) for f in os.listdir(src)) if p.endswith(".png"))[-1]
        s = se.strips.new_image(name="S%02d_%s" % (i, kind), filepath=path, channel=ch, frame_start=fr(t0), fit_method="FILL")
        s.frame_final_duration = length
        z0, z1 = opts.get("zoom", (1.0, 1.0))
        px, py = opts.get("pan", (0, 0))
        fx = (0.5 - opts["focus"]) * H * 16 / 9 if "focus" in opts else 0.0
        fill = s.transform.scale_x          # Skalierung aus fit_method="FILL" (im Hochformat > 1) - Zoom kommt obendrauf
        for f, z, x, y in ((fr(t0), z0, fx, 0), (fr(t1) - 1, z1, fx + px, py)):
            s.transform.scale_x = s.transform.scale_y = z * fill
            s.transform.offset_x, s.transform.offset_y = x, y
            for p in ("scale_x", "scale_y", "offset_x", "offset_y"):
                s.transform.keyframe_insert(p, frame=f)
    elif kind == "bl":
        if not os.path.isdir(src):
            print("WARNUNG: Ordner fehlt", src)
            return None
        files = sorted(f for f in os.listdir(src) if f.endswith(".png"))[int(opts.get("offset", 0) * FPS):]
        if not files:
            print("WARNUNG: keine Frames in", src)
            return None
        s = se.strips.new_image(name="S%02d_bl" % i, filepath=os.path.join(src, files[0]), channel=ch, frame_start=fr(t0), fit_method="FILL")
        for f in files[1:length]:
            s.elements.append(f)
        s.frame_final_duration = length
    elif kind == "black":
        return None
    if s is None:
        return None
    s.blend_type = "ALPHA_OVER"
    if "focus" in opts and kind != "still":
        # Neuausrichtung fuer Hoch-/Quadratformat: Quellen sind 16:9, bei FILL fuellt die Hoehe das Bild
        s.transform.offset_x = (0.5 - opts["focus"]) * H * 16 / 9
    if opts.get("desat"):
        s.color_saturation = 1.0 - opts["desat"]
    if opts.get("dreamy"):
        s.color_saturation = 0.8
        s.color_multiply = 1.18
    if opts.get("glow"):
        s.color_multiply = 1.3
    if d_in and prev is not None:
        # Ueberblendung: vorherige Einstellung laeuft d_in weiter, diese blendet darueber auf
        prev.frame_final_duration = prev.frame_final_duration + int(d_in * FPS)
        key_alpha(s, [(t0, 0.0), (t0 + d_in, 1.0)])
    return s


def main():
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y, scene.render.resolution_percentage = W, H, 100
    scene.render.fps, scene.render.fps_base = FPS, 1.0
    scene.frame_start, scene.frame_end = 1, fr(edl.LENGTH) - 1
    scene.view_settings.view_transform = "Standard"      # Bildmaterial ist bereits fertig belichtet
    scene.render.dither_intensity = 1.0                  # gegen Farbstufen in dunklen Verlaeufen (8-Bit-Video)
    se = scene.sequence_editor_create()
    color(se, "Base_Black", 1, 0.0, edl.LENGTH, (0, 0, 0))

    prev = None
    top = 2
    for i, shot in enumerate(edl.VIDEO):
        s = add_shot(se, i, shot, prev)
        if s is not None:
            prev = s
            top = max(top, s.channel)
    fx_ch = top + 1
    # Blenden und Blitze
    for shot in edl.VIDEO:
        t0, t1, kind, src, opts = shot
        if opts.get("fade_in"):
            key_alpha(color(se, "FadeIn", fx_ch, t0, t0 + opts["fade_in"], (0, 0, 0)), [(t0, 1.0), (t0 + opts["fade_in"], 0.0)])
        if opts.get("fade_out"):
            key_alpha(color(se, "FadeOut", fx_ch, t1 - opts["fade_out"], t1, (0, 0, 0)), [(t1 - opts["fade_out"], 0.0), (t1, 1.0)])
        if opts.get("flash_out"):
            d = opts["flash_out"]
            key_alpha(color(se, "FlashOut", fx_ch + 1, t1 - d, t1, (1, 1, 1)), [(t1 - d, 0.0), (t1, 1.0)])
        if opts.get("flash_in"):
            d = opts["flash_in"]
            key_alpha(color(se, "FlashIn", fx_ch + 1, t0, t0 + d, (1, 0.98, 0.94)), [(t0, 1.0), (t0 + d, 0.0)])
    # Einheitlicher Look: warme Lichter, neutrale Tiefen, minimaler Kontrast (Adjustment Layer)
    adj = se.strips.new_effect(name="Grade", type="ADJUSTMENT", channel=fx_ch + 2, frame_start=1, length=fr(edl.LENGTH))
    cb = adj.modifiers.new(name="Balance", type="COLOR_BALANCE")
    try:
        cb.color_balance.lift = (0.985, 0.985, 1.0)
        cb.color_balance.gamma = (1.0, 1.0, 0.99)
        cb.color_balance.gain = (1.03, 1.01, 0.97)
    except AttributeError:
        pass
    # Kinobalken 2,39:1
    for name, y in ((("Bar_Top", H / 2 - BAR / 2), ("Bar_Bottom", -(H / 2 - BAR / 2))) if BAR > 0 else ()):
        b = color(se, name, fx_ch + 3, 0.0, edl.LENGTH, (0, 0, 0))
        b.transform.scale_y = BAR / H
        b.transform.offset_y = y
    # Abspann-Hinweis (Ehrlichkeit: was ist Spiel, was ist Konzept)
    t0, t1, text = edl.END_CARD
    card = se.strips.new_effect(name="EndCard", type="TEXT", channel=fx_ch + 4, frame_start=fr(t0), length=fr(t1) - fr(t0))
    card.text = text
    card.font = bpy.data.fonts.load("C:/Windows/Fonts/segoeui.ttf")     # gleiche Schriftfamilie wie der Titel
    card.font_size = getattr(edl, "END_CARD_SIZE", 22)
    card.wrap_width = 0.9
    card.color = (0.62, 0.6, 0.56, 1)
    card.location = getattr(edl, "END_CARD_POS", (0.5, 0.5))
    card.anchor_x, card.anchor_y = "CENTER", "CENTER"
    card.alignment_x = "CENTER"
    card.blend_type = "ALPHA_OVER"
    key_alpha(card, [(t0, 0.0), (t0 + 0.8, 1.0), (t1 - 0.8, 1.0), (t1, 0.0)])
    if os.path.exists(MIX):
        se.strips.new_sound(name="Mix", filepath=MIX, channel=fx_ch + 6, frame_start=1)
    else:
        print("WARNUNG: Mix fehlt", MIX)

    os.makedirs(OUT, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, NAME + "_Edit.blend"))
    if "--stills" in ARGS:
        scene.render.image_settings.file_format = "JPEG"
        for t in [s[0] + (s[1] - s[0]) * 0.5 for s in edl.VIDEO]:
            scene.frame_set(fr(t))
            scene.render.filepath = os.path.join(OUT, "stills_" + NAME, "%06.2fs.jpg" % t)
            bpy.ops.render.render(write_still=True)
        print("GENESIS_V2_STILLS_OK")
        return
    scene.render.image_settings.media_type = "VIDEO"
    scene.render.image_settings.file_format = "FFMPEG"
    ff = scene.render.ffmpeg
    ff.format, ff.codec = "MPEG4", "H264"
    ff.constant_rate_factor = "PERC_LOSSLESS"
    ff.ffmpeg_preset = "BEST"
    ff.audio_codec, ff.audio_bitrate, ff.audio_mixrate, ff.audio_channels = "AAC", 320, 48000, "STEREO"
    scene.render.filepath = os.path.join(OUT, NAME + ".mp4")
    if "--master-only" not in ARGS:
        bpy.ops.render.render(animation=True)
        print("GENESIS_V2_RENDER_OK", scene.render.filepath)
    if "--master" in ARGS or "--master-only" in ARGS:
        ff.format, ff.codec = "QUICKTIME", "PRORES"
        ff.ffmpeg_prores_profile = "422_HQ"
        ff.audio_codec = "PCM"
        scene.render.filepath = os.path.join(OUT, NAME + "_Master_ProRes422HQ.mov")
        bpy.ops.render.render(animation=True)
        print("GENESIS_V2_MASTER_OK", scene.render.filepath)


main()
