# GENESIS: Der Kreislauf des Lebens
# Baut den Embryo der dritten und vierten Woche (SK_GEN_EmbryoBody) mit Formschlüsseln für Tag 16 bis Tag 28.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Embryogenesis/build_embryo_body.py -- --out ArtSource/Generated/Embryogenesis [--export] [--render]
#
# Anatomie (Carnegie 7–13, Maße aus O'Rahilly & Müller sowie Langman):
#   Tag 16: flache Keimscheibe, 0,7 mm lang, birnenförmig (vorn breit), Primitivstreifen als Furche im hinteren Drittel.
#   Tag 19: 1,3 mm; die Neuralplatte hebt ihre Ränder, in der Mitte die Neuralrinne.
#   Tag 22: 2,4 mm; 8 Somitenpaare als Wülste rechts und links der Rinne; die Herzwölbung am vorderen Bauch
#           beginnt zu schlagen; der Körper krümmt sich leicht.
#   Tag 25: 3,2 mm; 18 Somitenpaare, Neuralrohr vorn geschlossen, Kopfteil wölbt sich, erste Kiemenbögen.
#   Tag 28: 4,6 mm; 28 Somitenpaare, C-Form mit Schwanz, Herzwölbung deutlich, Augenbläschen, Extremitätenknospen.
#
# Aufbau: eine Mittellinie (der Körper als gebogene Achse), darauf ein ovaler Querschnitt, dessen Höhe und Breite
# dem Tag folgt. Alle fünf Tage entstehen aus derselben Topologie – so lassen sie sich als Formschlüssel
# (Shape Keys → Morph Targets in Unreal) ineinander überführen und im Spiel nach dem Tag mischen.
# Maßstab: 1 µm = 0,01 m (Blender) = 1 cm (Unreal), wie in der ganzen Mikrowelt.

import math
import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Conception"))
from genesis_blender_common import UM, ensure_dir, reset_scene, script_args, smoothstep
import bpy
import numpy as np

RINGS = 96          # Punkte entlang der Körperachse (Kopf → Schwanz)
SEGMENTS = 40       # Punkte um den Querschnitt
SOMITE_MAX = 30


def body_profile(day):
    """Maße des Körpers an diesem Tag: Länge (µm), Grundradius, Krümmung, Somitenzahl."""
    length = float(np.interp(day, [16.0, 19.0, 22.0, 25.0, 28.0], [700.0, 1300.0, 2400.0, 3200.0, 4600.0]))
    radius = float(np.interp(day, [16.0, 19.0, 22.0, 25.0, 28.0], [40.0, 110.0, 230.0, 330.0, 430.0]))
    curvature = float(np.interp(day, [16.0, 19.0, 22.0, 25.0, 28.0], [0.0, 0.10, 0.45, 0.80, 1.0]))
    somites = int(np.interp(day, [19.5, 20.5, 22.0, 25.0, 28.0], [0.0, 1.0, 8.0, 18.0, 28.0]))
    return length, radius, curvature, somites


def bump(x, center, width):
    """Weiche Glocke (0..1) – für Bläschen, Wölbungen und Knospen."""
    return math.exp(-((x - center) / max(1.0e-4, width)) ** 2)


def axis_points(day):
    """
    Mittellinie von Kopf (u=0) zu Schwanz (u=1). In der dritten Woche fast gerade, in der vierten die C-Form:
    Kopf- und Schwanzfalte krümmen den Körper um den Herzbereich herum (Nackenbeuge am stärksten).
    """
    length, _, curvature, _ = body_profile(day)
    u = np.linspace(0.0, 1.0, RINGS)
    # Krümmung ungleich verteilt: vorn (Nackenbeuge) und hinten (Schwanzfalte) stärker als in der Mitte
    local = 1.4 * np.exp(-((u - 0.18) / 0.22) ** 2) + 1.0 * np.exp(-((u - 0.85) / 0.25) ** 2) + 0.55
    angle = np.cumsum(local) / max(1.0e-6, np.sum(local)) * (math.pi * 1.25 * curvature)
    angle = angle - angle[len(angle) // 2]
    step = length / (RINGS - 1)
    x = np.cumsum(np.cos(angle)) * step
    z = np.cumsum(np.sin(angle)) * step
    x -= x.mean()
    z -= z.mean()
    return u, x, z, angle


def build_day(day):
    """Ein Körper für diesen Tag – gleiche Topologie für alle Tage, damit Formschlüssel möglich sind."""
    u, ax, az, angle = axis_points(day)
    length, radius, curvature, somites = body_profile(day)
    flat = 1.0 - smoothstep(16.0, 21.0, day)          # Tag 16: noch eine flache Scheibe
    groove = 1.0 - smoothstep(19.0, 25.5, day)         # offene Neuralrinne, schließt sich
    heart = smoothstep(20.5, 24.0, day)
    head = smoothstep(19.0, 27.0, day)
    buds = smoothstep(26.0, 29.0, day)
    sense = smoothstep(23.0, 27.0, day)

    verts = np.zeros((RINGS * SEGMENTS, 3))
    for ring in range(RINGS):
        t = u[ring]

        # Grundverlauf: Kopfende dick, Rumpf gleichmäßig, Schwanz spitz
        shape = 0.55 + 0.75 * bump(t, 0.16, 0.20) + 0.35 * bump(t, 0.45, 0.28)
        shape *= 1.0 - 0.75 * smoothstep(0.78, 1.0, t)
        # Drei Hirnbläschen am Kopfende (Vorder-, Mittel-, Rautenhirn) – ab der vierten Woche deutlich
        vesicles = head * (0.30 * bump(t, 0.06, 0.05) + 0.24 * bump(t, 0.14, 0.045) + 0.28 * bump(t, 0.23, 0.05))
        r_base = radius * (shape + vesicles)

        for seg in range(SEGMENTS):
            phi = 2.0 * math.pi * seg / SEGMENTS
            cos_p, sin_p = math.cos(phi), math.sin(phi)
            back = max(0.0, cos_p)
            belly = max(0.0, -cos_p)
            side = abs(sin_p)

            r = r_base
            # Herzwölbung: vorn am Bauch, unter dem Kopf
            r += radius * 1.15 * heart * bump(t, 0.30, 0.075) * belly
            # Somiten: Blöcke rechts und links des Rückens (nicht auf der Mittellinie)
            if somites > 0:
                span_start, span_end = 0.28, 0.28 + 0.55 * min(1.0, somites / 28.0)
                if span_start <= t <= span_end:
                    local = (t - span_start) / max(1.0e-4, span_end - span_start)
                    wave = 0.5 + 0.5 * math.cos(2.0 * math.pi * somites * local)
                    beside = math.exp(-((side - 0.55) / 0.35) ** 2)   # neben der Mittellinie
                    r += radius * 0.16 * wave * beside * back
            # Neuralrinne: Furche auf der Mittellinie des Rückens, solange das Rohr offen ist
            r -= radius * 0.45 * groove * math.exp(-(side / 0.22) ** 2) * back
            # Augenbläschen und Ohrgrübchen seitlich am Kopf
            r += radius * 0.22 * sense * bump(t, 0.11, 0.035) * math.exp(-((side - 0.9) / 0.25) ** 2)
            r += radius * 0.14 * sense * bump(t, 0.20, 0.03) * math.exp(-((side - 0.85) / 0.25) ** 2)
            # Extremitätenknospen: kleine seitliche Paddel (Arme Tag 26, Beine etwas später)
            r += radius * 0.40 * buds * bump(t, 0.40, 0.045) * math.exp(-((side - 0.95) / 0.20) ** 2)
            r += radius * 0.32 * buds * bump(t, 0.66, 0.045) * math.exp(-((side - 0.95) / 0.20) ** 2)

            # Tag 16 ist eine flache Scheibe: breit, aber kaum dick
            height_scale = 1.0 - 0.88 * flat
            y = r * sin_p * (1.0 + 0.9 * flat)
            local_z = r * cos_p * height_scale

            # Der Querschnitt steht senkrecht zur Mittellinie
            nx = -math.sin(angle[ring])
            nz = math.cos(angle[ring])
            verts[ring * SEGMENTS + seg] = (ax[ring] + nx * local_z, y, az[ring] + nz * local_z)

    faces = []
    for ring in range(RINGS - 1):
        for seg in range(SEGMENTS):
            a = ring * SEGMENTS + seg
            b = ring * SEGMENTS + (seg + 1) % SEGMENTS
            c = (ring + 1) * SEGMENTS + (seg + 1) % SEGMENTS
            d = (ring + 1) * SEGMENTS + seg
            faces.append((a, b, c, d))
    return verts * UM, faces


def render_check(obj, out_dir, day, samples=96):
    """Prüfbild: Embryo an diesem Tag, warmes Licht auf dunklem Grund (Bildsprache des Covers, Docs/31)."""
    from genesis_blender_common import configure_color_management, configure_cycles, enable_gpu, measure_exposure, new_node
    from mathutils import Vector
    scene = bpy.context.scene

    # Den Formschlüssel dieses Tages voll aufziehen
    for key in obj.data.shape_keys.key_blocks:
        key.value = 1.0 if key.name == "Tag%02d" % int(day) else 0.0

    material = bpy.data.materials.new("LookDev_Embryo")
    material.use_nodes = True
    tree = material.node_tree
    bsdf = tree.nodes["Principled BSDF"]
    # Perlmuttfarben und durchscheinend: Der Embryo ist fast durchsichtig, das Licht geht hindurch
    bsdf.inputs["Base Color"].default_value = (0.82, 0.70, 0.60, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.35
    bsdf.inputs["Subsurface Weight"].default_value = 0.9
    bsdf.inputs["Subsurface Radius"].default_value = (1.0, 0.45, 0.25)
    bsdf.inputs["Subsurface Scale"].default_value = 300.0 * UM
    obj.data.materials.clear()
    obj.data.materials.append(material)

    enable_gpu(scene)
    configure_color_management(scene, exposure=-1.2)
    configure_cycles(scene, samples=samples, width=1400, height=900)
    world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes.clear()
    background = new_node(world.node_tree, "ShaderNodeBackground", (0, 0))
    background.inputs["Color"].default_value = (0.02, 0.012, 0.010, 1.0)
    background.inputs["Strength"].default_value = 0.35
    out = new_node(world.node_tree, "ShaderNodeOutputWorld", (300, 0))
    world.node_tree.links.new(background.outputs[0], out.inputs[0])

    length = body_profile(day)[0]
    camera_data = bpy.data.cameras.new("Camera")
    camera_data.sensor_width = 36.0
    camera_data.lens = 65.0
    camera_data.clip_start = 1.0 * UM
    camera = bpy.data.objects.new("Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    distance = length * 3.2 * UM
    camera.location = Vector((0.2 * length * UM, -distance, 0.35 * distance))
    target = Vector((0.0, 0.0, 0.1 * length * UM))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()

    # Warmes Hauptlicht von vorn oben, kühles Streiflicht von hinten (warm gegen kalt, Docs/31)
    key_data = bpy.data.lights.new("Key", "AREA")
    # Leistung aus dem Abstand: Beleuchtungsstärke E = P / (4π d²). Im Maßstab der Mikrowelt (1 µm = 1 cm) ist der
    # Embryo in Blender zig Meter groß – eine feste Wattzahl brennt das Bild aus (gemessen: 98 % reinweiß).
    key_data.energy = 38.0 * distance ** 2
    key_data.size = length * 2.0 * UM
    key_data.color = (1.0, 0.86, 0.68)
    key_light = bpy.data.objects.new("Key", key_data)
    scene.collection.objects.link(key_light)
    key_light.location = Vector((0.6 * length * UM, -1.4 * length * UM, 1.6 * length * UM))
    key_light.rotation_euler = (Vector((0, 0, 0)) - key_light.location).to_track_quat("-Z", "Y").to_euler()

    rim_data = bpy.data.lights.new("Rim", "AREA")
    rim_data.energy = 14.0 * distance ** 2
    rim_data.size = length * 1.5 * UM
    rim_data.color = (0.62, 0.75, 1.0)
    rim = bpy.data.objects.new("Rim", rim_data)
    scene.collection.objects.link(rim)
    rim.location = Vector((-0.8 * length * UM, 1.6 * length * UM, 0.9 * length * UM))
    rim.rotation_euler = (Vector((0, 0, 0)) - rim.location).to_track_quat("-Z", "Y").to_euler()

    scene.render.filepath = os.path.join(out_dir, "LookDev_Embryo_Tag%02d.png" % int(day))
    bpy.ops.render.render(write_still=True)
    measure_exposure(scene.render.filepath)


def main():
    args = script_args()
    out_dir = ensure_dir(os.path.abspath(args.get("out", "ArtSource/Generated/Embryogenesis")))
    reset_scene()

    days = [16.0, 19.0, 22.0, 25.0, 28.0]
    base_verts, faces = build_day(days[0])
    mesh = bpy.data.meshes.new("SK_GEN_EmbryoBody")
    mesh.from_pydata([tuple(v) for v in base_verts], [], faces)
    mesh.validate()
    for poly in mesh.polygons:
        poly.use_smooth = True
    obj = bpy.data.objects.new("SK_GEN_EmbryoBody", mesh)
    bpy.context.scene.collection.objects.link(obj)
    print("GENESIS: Grundform Tag %.0f: %d Punkte, %d Flächen" % (days[0], len(mesh.vertices), len(mesh.polygons)))

    # Formschlüssel für die übrigen Tage (gleiche Topologie)
    obj.shape_key_add(name="Tag16", from_mix=False)
    for day in days[1:]:
        verts, _ = build_day(day)
        key = obj.shape_key_add(name="Tag%02d" % int(day), from_mix=False)
        for index, co in enumerate(verts):
            key.data[index].co = co
        length, _, _, somites = body_profile(day)
        print("GENESIS: Formschlüssel Tag %.0f: %.2f mm, %d Somitenpaare" % (day, length / 1000.0, somites))

    if args.get("export"):
        path = os.path.join(out_dir, "SK_GEN_EmbryoBody.fbx")
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True,
                                 apply_scale_options="FBX_SCALE_UNITS", object_types={"MESH"},
                                 use_mesh_modifiers=False, mesh_smooth_type="FACE", add_leaf_bones=False)
        print("GENESIS: exportiert", path, round(os.path.getsize(path) / 1e6, 2), "MB")
    if args.get("render"):
        render_check(obj, out_dir, float(args.get("day", 28.0)), int(args.get("samples", 96)))


main()
