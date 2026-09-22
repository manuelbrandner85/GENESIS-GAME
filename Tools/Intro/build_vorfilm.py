# GENESIS: Der Kreislauf des Lebens
#
# Macht aus dem Trailer den Vorfilm, der beim Start des Spiels laeuft.
#
# Ein Trailer wirbt, ein Vorfilm empfaengt. Deshalb faellt hinten die Werbekarte weg ("Bald
# erhaeltlich fuer PC / Trendonix Games"): Wer das Spiel startet, hat es bereits. Der Film endet auf
# der Titelkarte und geht ins Schwarz, aus dem danach der Startbildschirm aufblendet.
#
# Der Anfang bleibt, wie er ist. Die vier Sekunden Schwarz sind kein toter Vorlauf: Ab 1,82 s spricht
# der Erzaehler aus dem Dunkel ("Bevor du deinen ersten Atemzug nahmst ..."), erst danach kommt das
# Bild. Ein erster Schnitt bei 3,0 s hat den Satz mitten im Wort abgeschnitten – gefunden erst beim
# Vermessen der Sprachspur, nicht im Bild.
# Sonst wird nichts angefasst: gleicher Schnitt, gleiche Musik, gleiche Farbe.
#
# Aufruf (ohne Oberflaeche):
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --python Tools\Intro\build_vorfilm.py
#
# Ergebnis: Genesis/Content/Movies/GENESIS_Vorfilm.mp4. Der Startablauf (GenesisFrontend) spielt ihn
# nach dem Hinweis ab; der Paketbau nimmt alles aus Content/Movies von selbst mit. Die Untertitel
# stehen in GenesisBootFlow.cpp (FilmSubtitles) und sind an der Sprachspur des Trailers gemessen –
# wer den Schnitt aendert, muss sie mitziehen.

import os
import sys

import bpy

# --- Quelle und Ziel -------------------------------------------------------------------------
WURZEL = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
# Quelle ist der Trailer aus dem Repository (hoehere Bitrate als jede Upload-Fassung), damit der
# Vorfilm aus dem Projekt heraus jederzeit neu entsteht.
QUELLE = os.environ.get("GENESIS_TRAILER",
                        os.path.join(WURZEL, "Docs", "Media", "Trailer", "GENESIS_Trailer_v2.mp4"))
ZIEL = os.environ.get("GENESIS_VORFILM", os.path.join(WURZEL, "Genesis", "Content", "Movies", "GENESIS_Vorfilm"))

# --- Schnitt (Bilder der Quelle, 24 Bilder je Sekunde) ---------------------------------------
# Gemessen an der Quelle: Titelkarte voll bei Bild 2860, ausgeblendet ab Bild 2895; die Werbekarte
# beginnt bei Bild 2925. Die letzte Stimme ("Kennen wir uns?") endet bei 119,84 s = Bild 2877.
ERSTES_BILD = 1
LETZTES_BILD = 2916   # 121,46 s -> mitten im Schwarz, neun Bilder vor der Werbekarte
TON_AUSBLENDUNG = 36  # Bilder (1,5 s), damit der Ton am Schnitt nicht abreisst

FPS = 24
BREITE, HOEHE = 1920, 1080


def baue_szene():
    scn = bpy.data.scenes.new("GENESIS_Vorfilm")
    bpy.context.window.scene = scn

    scn.render.resolution_x = BREITE
    scn.render.resolution_y = HOEHE
    scn.render.resolution_percentage = 100
    scn.render.fps = FPS
    scn.render.use_sequencer = True
    scn.render.use_compositing = False

    # Die Quelle ist bereits fertig gradet. Ohne diese beiden Zeilen wuerde Blender sie ein
    # zweites Mal durch AgX schicken und der Film waere flauer als der Trailer.
    scn.view_settings.view_transform = "Standard"
    scn.view_settings.look = "None"
    scn.view_settings.exposure = 0.0
    scn.view_settings.gamma = 1.0
    scn.sequencer_colorspace_settings.name = "sRGB"

    se = scn.sequence_editor_create()
    se.strips.new_movie("Bild", QUELLE, channel=1, frame_start=1)
    ton = se.strips.new_sound("Ton", QUELLE, channel=2, frame_start=1)

    # Ton sanft auslaufen lassen
    ton.volume = 1.0
    ton.keyframe_insert("volume", frame=LETZTES_BILD - TON_AUSBLENDUNG)
    ton.volume = 0.0
    ton.keyframe_insert("volume", frame=LETZTES_BILD)

    scn.frame_start = ERSTES_BILD
    scn.frame_end = LETZTES_BILD
    return scn


def stelle_encoder(scn):
    # Seit Blender 5 liegt FFMPEG hinter media_type = VIDEO; ohne diese Zeile kennt
    # file_format nur Einzelbilder und bricht mit "enum FFMPEG not found" ab.
    scn.render.image_settings.media_type = "VIDEO"
    scn.render.image_settings.file_format = "FFMPEG"
    ff = scn.render.ffmpeg
    ff.format = "MPEG4"
    ff.codec = "H264"
    # Gemessen gegen die Quelle an den zwei haertesten Stellen (koerniges Zellmakro, dunkle
    # Sterbeszene): HIGH weicht im Mittel 1,3 bis 1,9 von 255 Stufen ab, MEDIUM 1,8 bis 3,2.
    # Im Makro waere MEDIUM als Kornmatsch sichtbar, deshalb HIGH.
    ff.constant_rate_factor = "HIGH"
    ff.ffmpeg_preset = "GOOD"
    ff.gopsize = FPS                      # ein Schluesselbild je Sekunde: sauberes Springen
    ff.audio_codec = "AAC"
    ff.audio_bitrate = 192
    ff.audio_mixrate = 48000
    ff.audio_channels = "STEREO"
    scn.render.filepath = ZIEL


def main():
    if not os.path.exists(QUELLE):
        sys.exit("Quelle fehlt: %s" % QUELLE)
    os.makedirs(os.path.dirname(ZIEL), exist_ok=True)

    scn = baue_szene()
    stelle_encoder(scn)

    bilder = LETZTES_BILD - ERSTES_BILD + 1
    print("GENESIS: Vorfilm %d Bilder = %.1f s -> %s" % (bilder, bilder / float(FPS), ZIEL))
    bpy.ops.render.render(animation=True)

    # Blender haengt den Bildbereich an den Dateinamen
    erwartet = "%s%04d-%04d.mp4" % (ZIEL, ERSTES_BILD, LETZTES_BILD)
    endgueltig = ZIEL + ".mp4"
    if os.path.exists(erwartet):
        if os.path.exists(endgueltig):
            os.remove(endgueltig)
        os.rename(erwartet, endgueltig)
    if os.path.exists(endgueltig):
        print("GENESIS: fertig, %.1f MB" % (os.path.getsize(endgueltig) / 1048576.0))
    else:
        sys.exit("GENESIS: Ausgabe nicht gefunden")


main()
