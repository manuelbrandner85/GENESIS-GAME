# GENESIS: Der Kreislauf des Lebens
# Das warme Tuch, mit dem die Hebamme das Kind auf der Brust der Mutter abtrocknet und dann zudeckt.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Birth/build_baby_towel.py -- --out ArtSource/Generated/Birth
#
# Gebaut im Raum der Kamera des Kindes (1 mm = 1 Unreal-Einheit, wie der Kreißsaal): +X ist die Blickrichtung,
# +Z „oben" (vom Kind aus: sein Hinterkopf und Rücken). Das Kind liegt bäuchlings, den Kopf zur Seite gedreht;
# das Tuch liegt über Hinterkopf und Rücken, fällt an den Seiten auf die Brust der Mutter und endet vorn über der
# Stirn. Vom Tuch sieht das Kind nur den vorderen Rand – oben im Bild, ganz nah und völlig unscharf.
#
# Maße: Frotteetuch 60 × 45 cm, Flor ~3,5 mm. Ein Neugeborenes ist ~50 cm lang, Kopfumfang ~35 cm: Der Scheitel
# liegt etwa 5–6 cm über und hinter dem Auge, der Rücken fällt nach hinten ab.

import math
import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Conception"))
from genesis_blender_common import UM as UNIT, ensure_dir, reset_scene, script_args  # noqa: E402
import bmesh  # noqa: E402
import bpy  # noqa: E402

COLS, ROWS = 96, 120            # quer (Y) × längs (X)
HALF_WIDTH = 230.0              # mm
BACK = -440.0                   # hinteres Ende über dem Po


def smoothstep(a, b, x):
    t = min(1.0, max(0.0, (x - a) / (b - a)))
    return t * t * (3.0 - 2.0 * t)


def front_edge(y):
    # Der vordere Rand ist nie gerade: leicht gewellt, in der Mitte über der Stirn etwas vorgezogen
    return 34.0 + 9.0 * math.sin(y * 0.031 + 0.7) + 5.0 * math.sin(y * 0.083) + 8.0 * math.exp(-(y / 70.0) ** 2)


def height(x, y):
    # Form darunter: Hinterkopf (Scheitel ~58 mm über dem Auge, 60 mm dahinter), dann Nacken und Rücken
    head = 58.0 * math.exp(-((x + 60.0) / 85.0) ** 2)
    back = 34.0 * smoothstep(-120.0, -260.0, x) * (1.0 - 0.35 * smoothstep(-300.0, -440.0, x))
    crown = max(head, back) + 14.0
    # Seitlich fällt das Tuch über Kopf und Schultern auf die Brust der Mutter
    side = max(0.0, (abs(y) - 55.0) / 175.0)
    z = crown - 175.0 * side ** 1.5
    # Falten: breite Wellen quer und längs, dazu der Knick, wo es über den Kopf gezogen wurde
    z += 6.5 * math.sin(y * 0.043 + x * 0.011) + 3.5 * math.sin(x * 0.052 + y * 0.021 + 1.3)
    z += 5.0 * math.exp(-((x + 150.0) / 25.0) ** 2) * math.cos(y * 0.05)
    # Der vordere Rand hängt ein wenig nach unten über (Gewicht des Stoffes)
    edge = front_edge(y)
    z -= 16.0 * smoothstep(edge - 45.0, edge, x)
    return z


def build():
    bm = bmesh.new()
    verts = []
    for row in range(ROWS + 1):
        line = []
        for col in range(COLS + 1):
            y = -HALF_WIDTH + 2.0 * HALF_WIDTH * col / COLS
            x = BACK + (front_edge(y) - BACK) * row / ROWS
            line.append(bm.verts.new((x * UNIT, y * UNIT, height(x, y) * UNIT)))
        verts.append(line)
    uv_layer = bm.loops.layers.uv.new("UVMap")
    for row in range(ROWS):
        for col in range(COLS):
            face = bm.faces.new((verts[row][col], verts[row][col + 1], verts[row + 1][col + 1], verts[row + 1][col]))
            for loop, (c, r) in zip(face.loops, ((col, row), (col + 1, row), (col + 1, row + 1), (col, row + 1))):
                loop[uv_layer].uv = (c / COLS, r / ROWS)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new("SM_GEN_BabyTowel")
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new("SM_GEN_BabyTowel", mesh)
    bpy.context.scene.collection.objects.link(obj)
    material = bpy.data.materials.new("Towel")
    mesh.materials.append(material)
    # Frottee hat Körper: ~3,5 mm Flor, die Kanten sind umgenäht und rund
    solid = obj.modifiers.new("Flor", "SOLIDIFY")
    solid.thickness = 3.5 * UNIT
    solid.offset = -1.0
    solid.use_even_offset = True
    bevel = obj.modifiers.new("Saum", "BEVEL")
    bevel.width = 1.2 * UNIT
    bevel.segments = 2
    bevel.limit_method = "ANGLE"
    return obj


def main():
    args = script_args()
    out_dir = ensure_dir(os.path.abspath(args.get("out", "ArtSource/Generated/Birth")))
    reset_scene()
    obj = build()
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(out_dir, "SM_GEN_BabyTowel.fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False,
                             add_leaf_bones=False)
    print("GENESIS: exportiert %s %.2f MB" % (os.path.basename(path), os.path.getsize(path) / (1024 * 1024)))


main()
