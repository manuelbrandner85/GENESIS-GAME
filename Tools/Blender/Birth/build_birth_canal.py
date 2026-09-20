# GENESIS: Der Kreislauf des Lebens
# Baut den Geburtskanal als begehbare Röhre (SM_GEN_BirthCanal) aus anatomischen Maßen.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Birth/build_birth_canal.py -- --out ArtSource/Generated/Birth [--export]
#
# Anatomie (Geburtskanal am Ende der Austreibungsphase):
#   - Länge des knöchernen und weichen Kanals zusammen etwa 100–140 mm
#   - Engste Stelle im knöchernen Ring (Beckenmitte): Durchmesser ~105 mm, hier passt der Kopf nur gedreht hindurch
#   - Querfalten (Rugae) an der Innenwand, die sich beim Durchtritt glattziehen
#   - Am Ausgang weitet sich der Kanal trichterförmig
#
# Maßstab der Geburtsszene: 1 mm = 1 Unreal-Einheit. In Blender wird deshalb in Zentimetern gebaut
# (1 mm Modell = 1 cm Blender), damit der FBX-Import mit Einheitenumrechnung genau 1 zu 1 ankommt.

import math
import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Conception"))
from genesis_blender_common import UM as UNIT, ensure_dir, reset_scene, script_args
import bmesh
import bpy
import numpy as np

LENGTH = 140.0          # mm, vom tiefsten Punkt bis zum Ausgang
RINGS = 260             # Querschnitte entlang des Kanals
SEGMENTS = 96           # Punkte je Querschnitt
WALL_THICKNESS = 12.0   # mm

rng = np.random.default_rng(210920)


def value_noise(points, frequency, seed, octaves=3):
    """Glatte Summe aus Sinuswellen – deterministisch, für organische Verformungen."""
    generator = np.random.default_rng(seed)
    total = np.zeros(len(points))
    amplitude, norm = 1.0, 0.0
    for _ in range(octaves):
        directions = generator.normal(size=(6, points.shape[1]))
        directions /= np.linalg.norm(directions, axis=1, keepdims=True)
        phases = generator.uniform(0.0, 2.0 * math.pi, 6)
        layer = sum(np.sin(points @ direction * frequency + phase) for direction, phase in zip(directions, phases)) / 6.0
        total += layer * amplitude
        norm += amplitude
        amplitude *= 0.5
        frequency *= 2.0
    return total / norm


def canal_radius(s):
    """
    Radius entlang des Kanals (mm). s = 0 ist tief innen, s = 1 der Ausgang.
    Die engste Stelle liegt bei etwa 45 Prozent – der knöcherne Ring, durch den sich das Kind dreht.
    """
    # Der kindliche Kopf misst etwa 95 mm: Der knöcherne Ring mit 104 mm Durchmesser ist
    # tatsächlich nur wenige Millimeter weiter – deshalb muss sich das Kind drehen.
    deep = 58.0
    narrow = 52.0
    exit_radius = 72.0
    if s < 0.45:
        t = s / 0.45
        return deep + (narrow - deep) * (3.0 * t * t - 2.0 * t * t * t)
    t = (s - 0.45) / 0.55
    # Zum Ausgang hin trichterförmig, am Ende schnell weiter werdend
    return narrow + (exit_radius - narrow) * (t ** 2.2)


def build_canal():
    mesh = bpy.data.meshes.new("SM_GEN_BirthCanal")
    bm = bmesh.new()

    ring_vertices = []
    for ring in range(RINGS):
        s = ring / (RINGS - 1)
        base_radius = canal_radius(s)
        x = s * LENGTH

        vertices = []
        for segment in range(SEGMENTS):
            angle = 2.0 * math.pi * segment / SEGMENTS
            direction = np.array([math.cos(angle), math.sin(angle)])

            # Querfalten: Ringe aus Schleimhaut, die sich beim Durchtritt glattziehen
            rugae = 1.0 - 0.055 * (0.5 + 0.5 * math.cos(s * 46.0)) * (1.0 - s * 0.6)
            # Längsfalten: Die Wand liegt in Falten, solange sie nicht gedehnt ist.
            # Der Winkel wird dabei ungleichmäßig verzerrt – gleichmäßige Falten ergäben im Bild
            # ein regelmäßiges Sechseck, und ein Geburtskanal ist kein Bleistift.
            warped = angle + 0.55 * math.sin(angle * 2.0 + s * 1.7) + 0.22 * math.sin(angle * 3.0 - s * 2.4)
            fold_depth = 0.055 + 0.035 * (0.5 + 0.5 * math.sin(angle * 5.0 + s * 4.0))
            longitudinal = 1.0 - fold_depth * (0.5 + 0.5 * math.cos(warped * 5.0 + s * 3.0)) * (1.0 - s * 0.35)
            sample = np.array([[math.cos(angle) * 1.3, math.sin(angle) * 1.3, s * 6.0]])
            organic = 1.0 + 0.045 * value_noise(sample, 1.4, 11)[0] + 0.02 * value_noise(sample, 5.0, 12)[0]

            radius = base_radius * rugae * longitudinal * organic
            point = np.array([x, direction[0] * radius, direction[1] * radius])
            vertices.append(bm.verts.new((point * UNIT).tolist()))
        ring_vertices.append(vertices)

    for lower, upper in zip(ring_vertices[:-1], ring_vertices[1:]):
        for segment in range(SEGMENTS):
            other = (segment + 1) % SEGMENTS
            bm.faces.new((lower[segment], lower[other], upper[other], upper[segment]))

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()

    for polygon in mesh.polygons:
        polygon.use_smooth = True

    obj = bpy.data.objects.new("SM_GEN_BirthCanal", mesh)
    bpy.context.scene.collection.objects.link(obj)

    # Wandstärke nach außen: Die Kamera steckt innen, die Röhre braucht trotzdem eine echte Wand
    modifier = obj.modifiers.new("Wall", "SOLIDIFY")
    modifier.thickness = WALL_THICKNESS * UNIT
    modifier.offset = 1.0
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.select_set(False)

    add_uv_and_attribute(obj)
    return obj


def add_uv_and_attribute(obj):
    """UV: u = Winkel, v = Weg durch den Kanal. Farbattribut: R = Tiefe im Kanal, G = Faltenrelief."""
    mesh = obj.data
    count = len(mesh.vertices)
    data = np.empty(count * 3)
    mesh.vertices.foreach_get("co", data)
    co = data.reshape(-1, 3) / UNIT

    along = np.clip(co[:, 0] / LENGTH, 0.0, 1.0)
    angle = (np.arctan2(co[:, 2], co[:, 1]) + math.pi) / (2.0 * math.pi)
    radius = np.linalg.norm(co[:, 1:], axis=1)
    expected = np.array([canal_radius(s) for s in along])
    relief = np.clip(0.5 + (radius - expected) / 6.0, 0.0, 1.0)

    uv_layer = mesh.uv_layers.new(name="UVMap")
    loop_vertices = np.array([loop.vertex_index for loop in mesh.loops])
    uv_layer.data.foreach_set("uv", np.stack([angle[loop_vertices], along[loop_vertices]], axis=1).astype(np.float32).ravel())

    color = mesh.color_attributes.new(name="Tissue", type="FLOAT_COLOR", domain="POINT")
    color.data.foreach_set("color", np.stack([along, relief, 1.0 - relief, np.ones(count)], axis=1).astype(np.float32).ravel())


def export_fbx(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False,
                             colors_type="LINEAR", add_leaf_bones=False)
    print("GENESIS: exportiert %s %.1f MB" % (os.path.basename(path), os.path.getsize(path) / (1024 * 1024)))


args = script_args()
out_dir = args.get("out", "ArtSource/Generated/Birth")
reset_scene()

canal = build_canal()
bounds = np.array([[v.co.x, v.co.y, v.co.z] for v in canal.data.vertices]) / UNIT
print("GENESIS: %s  %.0f × %.0f × %.0f mm, %d Flächen" % (
    canal.name, np.ptp(bounds[:, 0]), np.ptp(bounds[:, 1]), np.ptp(bounds[:, 2]), len(canal.data.polygons)))
print("GENESIS: engste Stelle %.0f mm Durchmesser" % (2.0 * min(canal_radius(s / 100.0) for s in range(101))))

if "export" in args:
    ensure_dir(out_dir)
    export_fbx(canal, os.path.abspath(os.path.join(out_dir, "SM_GEN_BirthCanal.fbx")))
