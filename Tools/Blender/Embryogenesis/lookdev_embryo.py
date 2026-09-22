# GENESIS: Der Kreislauf des Lebens
#
# Lookdev für den Embryo (Tag 28): Kamera, Licht und Welt nach dem Cover (Docs/31) – goldenes
# Hauptlicht, kühles Gegenlicht, Tiefen blauschwarz – und ein Cycles-Bild zur Prüfung von Form und
# Gewebe. Belichtung wird gemessen (measure_exposure), nicht geschätzt.
#
# Im laufenden Blender nach build_embryo_day28.py:
#   exec(open(r"...\lookdev_embryo.py", encoding="utf-8").read())

import math
import os
import sys

import bpy
from mathutils import Vector

ROOT = r"C:\Users\manue\Desktop\Genesis Game"
sys.path.insert(0, os.path.join(ROOT, "Tools", "Blender", "Conception"))
from genesis_blender_common import configure_color_management, configure_cycles, enable_gpu, measure_exposure  # noqa: E402

OUT_DIR = os.path.join(ROOT, "ArtSource", "Generated", "Embryogenesis")
TARGET = Vector((3.0, 0.0, 0.0))        # Mitte des Embryos (m)


def aim(obj, target):
    direction = target - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def spherical(azimuth_deg, elevation_deg, distance):
    a, e = math.radians(azimuth_deg), math.radians(elevation_deg)
    return TARGET + Vector((math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e))) * distance


def area_light(coll, name, azimuth, elevation, distance, size, irradiance, colour):
    """Flächenlicht mit Leistung aus der Bestrahlungsstärke am Motiv: P = E · 4π · d²."""
    data = bpy.data.lights.get(name) or bpy.data.lights.new(name, "AREA")
    data.shape = "DISK"
    data.size = size
    data.energy = irradiance * 4.0 * math.pi * distance ** 2
    data.color = colour
    obj = bpy.data.objects.get(name) or bpy.data.objects.new(name, data)
    if obj.name not in coll.objects:
        coll.objects.link(obj)
    obj.location = spherical(azimuth, elevation, distance)
    aim(obj, TARGET)
    return obj


def setup(scene, view="dreiviertel"):
    coll = bpy.data.collections.get("Lookdev")
    if coll is None:
        coll = bpy.data.collections.new("Lookdev")
        scene.collection.children.link(coll)

    cam_data = bpy.data.cameras.get("CAM_Lookdev") or bpy.data.cameras.new("CAM_Lookdev")
    cam_data.lens = 85.0
    cam_data.sensor_width = 36.0
    cam_data.clip_start = 1.0
    cam_data.clip_end = 5000.0
    cam = bpy.data.objects.get("CAM_Lookdev") or bpy.data.objects.new("CAM_Lookdev", cam_data)
    if cam.name not in coll.objects:
        coll.objects.link(cam)
    views = {
        "dreiviertel": (-38.0, 14.0),   # von vorn links: Gesicht, Herz, Flanke mit Somiten
        "seite": (0.0, 0.0),
        "ruecken": (70.0, 20.0),
    }
    az, el = views[view]
    cam.location = spherical(az, el, 230.0)
    aim(cam, TARGET)
    scene.camera = cam

    # Cover: goldenes Hauptlicht von oben vorn, kühles Gegenlicht von hinten, kaum Aufhellung
    area_light(coll, "L_Key_Gold", -60.0, 42.0, 260.0, 90.0, 0.55, (1.0, 0.80, 0.60))
    area_light(coll, "L_Rim_Kuehl", 140.0, 30.0, 260.0, 70.0, 0.45, (0.62, 0.74, 1.0))
    area_light(coll, "L_Fill", 20.0, -25.0, 260.0, 120.0, 0.05, (0.75, 0.82, 1.0))

    world = bpy.data.worlds.get("W_Fruchtwasser") or bpy.data.worlds.new("W_Fruchtwasser")
    world.use_nodes = True
    nodes = world.node_tree.nodes
    bg = next((n for n in nodes if n.type == "BACKGROUND"), None)
    if bg is None:
        # Eine neue Welt hat in Blender 5 keine Knoten
        nodes.clear()
        bg = nodes.new("ShaderNodeBackground")
        out = nodes.new("ShaderNodeOutputWorld")
        world.node_tree.links.new(bg.outputs["Background"], out.inputs["Surface"])
    bg.inputs["Color"].default_value = (0.0035, 0.0036, 0.0048, 1.0)   # Cover-Tiefe #0B0A0C, leicht kühl
    bg.inputs["Strength"].default_value = 1.0
    scene.world = world

    for name in ("REF_Praeparat", "Embryo_Tag28_Grundform"):
        if name in bpy.data.objects:
            bpy.data.objects[name].hide_render = True


def render(view="dreiviertel", samples=192, exposure=0.0, width=1600, height=1200):
    scene = bpy.context.window.scene if bpy.context.window else bpy.context.scene
    setup(scene, view)
    enable_gpu(scene)
    configure_cycles(scene, samples=samples, noise_threshold=0.02, width=width, height=height)
    configure_color_management(scene, exposure=exposure)
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, "lookdev_tag28_%s.png" % view)
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return path, measure_exposure(path)


if "GENESIS_LOOKDEV_VIEW" in globals():
    LOOKDEV_RESULT = render(GENESIS_LOOKDEV_VIEW)
