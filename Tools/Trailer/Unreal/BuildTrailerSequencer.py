# GENESIS Trailer - baut die Unreal-Sequencer-Struktur aus Docs/Trailer/ShotList.json (idempotent).
#
#   /Game/Genesis/Trailer/Maps/L_Trailer_Stage                  Buehne (ohne World Partition)
#   /Game/Genesis/Trailer/Sequences/LS_GENESIS_REVEAL_TRAILER   Master (24 fps, 120 s)
#   /Game/Genesis/Trailer/Sequences/Sub/SEQ_001_Origin ... SEQ_012_Title
#   /Game/Genesis/Trailer/Audio/*                               Stems des Animatic-Mix (PLACEHOLDER)
#
# Jede Sub-Sequence enthaelt je Shot eine Spawnable-CineCamera (SH_...) mit Brennweite, Filmback 16:9 (Super 35),
# Blende und Transform-Keys fuer die geplante Kamerabewegung sowie einen Camera-Cut-Track.
# Der Master enthaelt den Subsequence-Track, Audio-Tracks (Musik, SFX, VO), einen Fade-Track und Shot-Marker.
#
# Aufruf: powershell -ExecutionPolicy Bypass -File Tools\Trailer\Run-UnrealPython.ps1 -Script Tools\Trailer\Unreal\BuildTrailerSequencer.py

import json
import math
import os

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
REPO = os.path.abspath(os.path.join(PROJECT_DIR, ".."))
DATA = json.load(open(os.path.join(REPO, "Docs", "Trailer", "ShotList.json"), encoding="utf-8"))
FPS = int(DATA["fps"])
ROOT = "/Game/Genesis/Trailer"
MAP = ROOT + "/Maps/L_Trailer_Stage"
SEQ_DIR = ROOT + "/Sequences"
SUB_DIR = SEQ_DIR + "/Sub"
AUDIO_DIR = ROOT + "/Audio"
MASTER_NAME = "LS_GENESIS_REVEAL_TRAILER"
MIX_DIR = os.path.join(PROJECT_DIR, "Saved", "TrailerAudio", "Mix")
STEMS = [("MX_SoulTheme_Trailer_Animatic", "Musik - Soul Theme (Unreal Harmonix Instrumente)"),
         ("SFX_Trailer_Animatic", "Sound Design (PLACEHOLDER)"),
         ("VO_Trailer_Placeholder", "Voice-over (PLACEHOLDER)")]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
failures = []


def fail(msg):
    failures.append(msg)
    unreal.log_error("GENESIS_TRAILER_FAIL " + msg)


def frame(t):
    return int(round(t * FPS))


def ensure_dir(path):
    if not eal.does_directory_exist(path):
        eal.make_directory(path)


def recreate_sequence(folder, name):
    path = folder + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    seq = asset_tools.create_asset(name, folder, unreal.LevelSequence, unreal.LevelSequenceFactoryNew())
    seq.set_display_rate(unreal.FrameRate(FPS, 1))
    return seq


# ---------------------------------------------------------------- Buehne
def build_stage():
    ensure_dir(ROOT + "/Maps")
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Engine/Maps/Templates", ROOT], True)
    if eal.does_asset_exist(MAP):
        unreal.log("GENESIS_TRAILER Buehne existiert: " + MAP)
        return
    if eal.duplicate_asset("/Engine/Maps/Templates/Template_Default", MAP) is None:
        fail("Buehne konnte nicht angelegt werden")
        return
    eal.save_asset(MAP, only_if_is_dirty=False)
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
        comp = actor.static_mesh_component
        if comp and comp.static_mesh and comp.static_mesh.get_path_name() == "/Engine/EngineSky/SM_SkySphere.SM_SkySphere":
            actors.destroy_actor(actor)
    eal.save_loaded_asset(world, only_if_is_dirty=False)
    unreal.log("GENESIS_TRAILER Buehne angelegt: " + MAP)


# ---------------------------------------------------------------- Audio
def import_audio():
    ensure_dir(AUDIO_DIR)
    tasks = []
    for name, _ in STEMS:
        src = os.path.join(MIX_DIR, name + ".wav")
        if not os.path.exists(src):
            fail("Stem fehlt: " + src)
            continue
        task = unreal.AssetImportTask()
        task.filename = src
        task.destination_path = AUDIO_DIR
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    asset_tools.import_asset_tasks(tasks)
    sounds = {}
    for name, _ in STEMS:
        sound = eal.load_asset(AUDIO_DIR + "/" + name)
        if isinstance(sound, unreal.SoundWave):
            sounds[name] = sound
        else:
            fail("Import fehlgeschlagen: " + name)
    return sounds


# ---------------------------------------------------------------- Kamera
def camera_path(shot, index):
    """Liefert Keys [(t_local, location, rotation)] fuer die geplante Bewegung. Einheiten: cm, Grad."""
    cam = shot["cam"]
    move, amount = cam.get("move", "static"), float(cam.get("amount", 0.1))
    dur = shot["end"] - shot["start"]
    origin = unreal.Vector(index * 2000.0, 0.0, 170.0)   # jeder Shot bekommt seinen eigenen Buehnenbereich
    target_dist = 400.0
    look = unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0)
    keys = []
    if move in ("dolly_in", "handheld_push", "dolly_out"):
        d = amount * target_dist * (1 if move != "dolly_out" else -1)
        keys = [(0.0, origin, look), (dur, origin + unreal.Vector(d, 0, 0), look)]
    elif move in ("track_left", "track_right", "track_follow"):
        d = amount * 600.0 * (-1 if move == "track_left" else 1)
        keys = [(0.0, origin + unreal.Vector(0, -d / 2, 0), look), (dur, origin + unreal.Vector(0, d / 2, 0), look)]
    elif move in ("crane_up", "crane_down"):
        d = amount * 500.0 * (1 if move == "crane_up" else -1)
        keys = [(0.0, origin + unreal.Vector(0, 0, -d / 2), unreal.Rotator(0.0, 8.0 * (1 if d < 0 else -1), 0.0)),
                (dur, origin + unreal.Vector(0, 0, d / 2), unreal.Rotator(0.0, -8.0 * (1 if d < 0 else -1), 0.0))]
    elif move in ("orbit", "handheld_orbit", "roll_open"):
        center = origin + unreal.Vector(target_dist, 0, 0)
        steps = 6
        for s in range(steps + 1):
            u = s / steps
            ang = math.radians(-amount / 2 + amount * u)
            if move == "roll_open":   # Kreismotiv als Rollen um die Blickachse
                keys.append((dur * u, origin, unreal.Rotator(roll=-amount / 2 + amount * u, pitch=0.0, yaw=0.0)))
            else:
                loc = center + unreal.Vector(-math.cos(ang) * target_dist, math.sin(ang) * target_dist, 0)
                keys.append((dur * u, loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=-math.degrees(ang))))
    elif move == "pullback_cosmic":
        keys = [(0.0, origin, unreal.Rotator(roll=0.0, pitch=-89.0, yaw=0.0)),
                (dur, origin + unreal.Vector(0, 0, 500000.0), unreal.Rotator(roll=0.0, pitch=-89.0, yaw=0.0))]
    elif move == "static_drift":
        keys = [(0.0, origin, look), (dur, origin + unreal.Vector(0, amount * 200.0, 0), look)]
    else:
        keys = [(0.0, origin, look)]
    return keys


def build_sub_sequence(seq_info, shots, first_index):
    seq_start = shots[0]["start"]
    seq_end = shots[-1]["end"]
    sub = recreate_sequence(SUB_DIR, seq_info["id"])
    sub.set_playback_start(0)
    sub.set_playback_end(frame(seq_end - seq_start))
    cut_track = sub.add_track(unreal.MovieSceneCameraCutTrack)
    for i, shot in enumerate(shots):
        f0, f1 = frame(shot["start"] - seq_start), frame(shot["end"] - seq_start)
        binding = sub.add_spawnable_from_class(unreal.CineCameraActor)
        binding.set_name(shot["id"])
        template = binding.get_object_template()
        cine = template.get_editor_property("camera_component")   # get_cine_camera_component() ist am Template None
        # Structs sind in Python Kopien -> veraendern und zurueckschreiben
        filmback = cine.get_editor_property("filmback")
        filmback.sensor_width = 24.89      # Super 35
        filmback.sensor_height = 14.0      # 16:9
        cine.set_editor_property("filmback", filmback)
        cine.set_editor_property("current_focal_length", float(shot["cam"]["lens"]))
        cine.set_editor_property("current_aperture", 2.0 if shot["cam"]["rig"] in ("macro", "dolly") else 2.8)
        focus = cine.get_editor_property("focus_settings")
        focus.focus_method = unreal.CameraFocusMethod.MANUAL
        focus.manual_focus_distance = 400.0
        cine.set_editor_property("focus_settings", focus)
        # Spawn-Track: Kamera existiert nur waehrend ihres Shots
        spawn = binding.add_track(unreal.MovieSceneSpawnTrack)
        spawn_section = spawn.add_section()
        spawn_section.set_range(f0, f1)
        keys = [(t + (shot["start"] - seq_start), l, r) for t, l, r in camera_path(shot, first_index + i)]
        track = binding.add_track(unreal.MovieScene3DTransformTrack)
        section = track.add_section()
        section.set_range(f0, f1)
        channels = {c.channel_name: c for c in section.get_all_channels()}
        for t, loc, rot in keys:
            for cname, value in (("Location.X", loc.x), ("Location.Y", loc.y), ("Location.Z", loc.z),
                                 ("Rotation.X", rot.roll), ("Rotation.Y", rot.pitch), ("Rotation.Z", rot.yaw)):
                if cname in channels:
                    channels[cname].add_key(unreal.FrameNumber(frame(t)), value)
        cut = cut_track.add_section()
        cut.set_range(f0, f1)
        cut.set_camera_binding_id(sub.get_binding_id(binding))
        mark = unreal.MovieSceneMarkedFrame()
        mark.label = shot["id"]
        tick = sub.get_tick_resolution()
        mark.frame_number = unreal.FrameNumber(int(round((shot["start"] - seq_start) * tick.numerator / tick.denominator)))
        sub.add_marked_frame(mark)
    eal.save_loaded_asset(sub, only_if_is_dirty=False)
    return sub


def build_master(subs, sounds):
    master = recreate_sequence(SEQ_DIR, MASTER_NAME)
    master.set_playback_start(0)
    master.set_playback_end(frame(DATA["duration"]))
    tick = master.get_tick_resolution()

    sub_track = master.add_track(unreal.MovieSceneSubTrack)
    sub_track.set_display_name("Sequenzen")
    for seq_info in DATA["sequences"]:
        shots = [s for s in DATA["shots"] if s["seq"] == seq_info["id"]]
        section = sub_track.add_section()
        section.set_sequence(subs[seq_info["id"]])
        section.set_range(frame(shots[0]["start"]), frame(shots[-1]["end"]))

    for row, (name, label) in enumerate(STEMS):
        if name not in sounds:
            continue
        track = master.add_track(unreal.MovieSceneAudioTrack)
        track.set_display_name(label)
        section = track.add_section()
        section.set_sound(sounds[name])
        section.set_range(0, frame(DATA["duration"]))

    fade_track = master.add_track(unreal.MovieSceneFadeTrack)
    fade = fade_track.add_section()
    fade.set_range(0, frame(DATA["duration"]))
    fade_ch = fade.get_all_channels()[0]
    for t, v in [(0.0, 1.0), (4.0, 1.0), (7.0, 0.0), (119.4, 0.0), (120.0, 1.0)]:
        fade_ch.add_key(unreal.FrameNumber(frame(t)), v)

    for shot in DATA["shots"]:
        mark = unreal.MovieSceneMarkedFrame()
        mark.label = shot["id"]
        mark.frame_number = unreal.FrameNumber(int(round(shot["start"] * tick.numerator / tick.denominator)))
        master.add_marked_frame(mark)
    eal.save_loaded_asset(master, only_if_is_dirty=False)
    return master


def main():
    for d in (ROOT, SEQ_DIR, SUB_DIR, AUDIO_DIR):
        ensure_dir(d)
    build_stage()
    sounds = import_audio()
    subs = {}
    index = 0
    for seq_info in DATA["sequences"]:
        shots = [s for s in DATA["shots"] if s["seq"] == seq_info["id"]]
        existing = SUB_DIR + "/" + seq_info["id"]
        in_production = any(s["status"] not in ("PLANNED", "ANIMATIC") for s in shots)
        if in_production and eal.does_asset_exist(existing):
            # Produktionssequenzen (eigene Skripte, z. B. BuildOriginSet.py) nicht mit Blocking ueberschreiben
            subs[seq_info["id"]] = eal.load_asset(existing)
            unreal.log("GENESIS_TRAILER {} in Produktion - unveraendert uebernommen".format(seq_info["id"]))
        else:
            subs[seq_info["id"]] = build_sub_sequence(seq_info, shots, index)
        index += len(shots)
    master = build_master(subs, sounds)

    # Selbstpruefung: zurueckgelesene Werte statt Annahmen
    master = eal.load_asset(SEQ_DIR + "/" + MASTER_NAME)
    rate = master.get_display_rate()
    sub_sections = [s for t in master.get_tracks() if isinstance(t, unreal.MovieSceneSubTrack) for s in t.get_sections()]
    cameras = 0
    for seq_info in DATA["sequences"]:
        sub = eal.load_asset(SUB_DIR + "/" + seq_info["id"])
        cameras += len([b for b in sub.get_spawnables() if str(b.get_display_name()).startswith("SH_")])
        by_name = {str(b.get_display_name()): b for b in sub.get_spawnables()}   # Reihenfolge ist nicht garantiert
        for shot in [s for s in DATA["shots"] if s["seq"] == seq_info["id"]]:
            b = by_name.get(shot["id"])
            if b is None:
                fail("Kamera fehlt: " + shot["id"])
                continue
            lens = b.get_object_template().get_editor_property("camera_component").get_editor_property("current_focal_length")
            if abs(lens - float(shot["cam"]["lens"])) > 0.01:
                fail("Brennweite nicht gesetzt: {} ({} statt {})".format(shot["id"], lens, shot["cam"]["lens"]))
    audio_tracks = [t for t in master.get_tracks() if isinstance(t, unreal.MovieSceneAudioTrack)]
    unreal.log("GENESIS_TRAILER Master {} fps, Ende Frame {}, Sub-Sequences {}, Kameras {}, Audio-Tracks {}, Marker {}".format(
        rate.numerator, master.get_playback_end(), len(sub_sections), cameras, len(audio_tracks), len(master.get_marked_frames())))
    if len(sub_sections) != len(DATA["sequences"]):
        fail("Anzahl Sub-Sequences stimmt nicht")
    if cameras != len(DATA["shots"]):
        fail("Anzahl Kameras {} != Shots {}".format(cameras, len(DATA["shots"])))
    if master.get_playback_end() != frame(DATA["duration"]):
        fail("Laenge stimmt nicht")
    if not failures:
        unreal.log("GENESIS_TRAILER_OK")


main()
