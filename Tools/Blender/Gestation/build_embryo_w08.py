# GENESIS: Der Kreislauf des Lebens
#
# Der Embryo in SSW 8 (6 Wochen nach der Befruchtung, Carnegie-Stadium ~17–18) für den frühen Moment im Mutterleib
# (Docs/37, Teil 2c). Das MakeHuman-Baby taugt dafür nicht: In dieser Woche hat das Kind noch einen Schwanzrest, Hand- und
# Fußplatten mit Fingerstrahlen, einen Kopf fast so groß wie der Rumpf, seitlich stehende Augen mit dunklem Pigment,
# Ohrhöcker statt Ohrmuscheln und eine große Herz-Leber-Wölbung; der Darm tritt in den Nabelstrang aus.
#
# Nachgezeichnet nach der Seitenansicht (Referenz lokal, nicht im Repository):
#   ArtSource/Reference/Fetus/11_Human_Embryo_-_Approximately_8_weeks_estimated_gestational_age.jpg
#   (lunar caustic, CC BY 2.0: „approximately 6 weeks from conception, i.e. 8 weeks from LMP“)
# Landmarken in Bildpunkten (1280 × 857) an einem 50-px-Raster abgelesen. Scheitel (410|170) bis Steiß (1015|510) = 694 px
# = Scheitel-Steiß-Länge 1,55 cm (Robinson & Fleming, wie build_fetus.py WEEKS[8]).
#
# Ausgabe wie build_fetus.py: SM_GEN_Fetus_W08 (Ursprung zwischen den Augen, +X Blick, +Y links, +Z Scheitel, cm) und der
# Eintrag „8“ in fetus_weeks.json (Mitte, Nabel, Hülle, halber Augenabstand).
#
#   blender -b --python-expr "g={'__name__':'genesis'}; exec(open(r'...build_embryo_w08.py', encoding='utf-8').read(), g); g['export']()"

import json
import math
import os

import bpy
import numpy as np
from mathutils import Matrix, Vector

CM = 0.01
OUT_DIR = r"C:\Users\manue\Desktop\Genesis Game\ArtSource\Generated\Gestation"
WEEK = 8
CRL_CM = 1.55
PX_CM = CRL_CM / 694.0
EYE_PX = (475.0, 365.0)
GAZE_TO_PX = (585.0, 330.0)             # Schnauze: dorthin „blickt“ das Gesicht
NAVEL_PX = (905.0, 350.0)
SINGLE_GAIN = 0.575                     # Metaball-Ellipsoid sichtbar ~0,58 × eingestellt (gemessen, build_embryo_day28.py)
CHAIN_GAIN = 0.80
TRIANGLES = 200000

# Rumpf in Querschnitten: Rücken (unten im Bild) und Bauch (oben), halbe Breite seitlich (px)
STATIONS = [
    ((345, 592), (585, 445), 110),      # Nacken / Hals
    ((430, 648), (650, 410), 118),
    ((540, 662), (720, 385), 122),      # Herzwölbung
    ((650, 664), (790, 365), 122),
    ((750, 650), (860, 362), 118),      # Leber
    ((840, 625), (910, 370), 110),
    ((910, 595), (960, 410), 98),       # Becken
    ((965, 560), (995, 445), 82),
    ((1005, 520), (1018, 480), 55),     # Steiß
]

# Kopf: Hirnschädel fast so groß wie der Rumpf, Vorderhirn wölbt sich nach vorn, darunter Oberkiefer, Schnauze, Unterkiefer
HEAD = [
    # x, y, Halbachsen (Bild-x, seitlich, Bild-z) px, Steifigkeit
    (404, 345, (156, 124, 156), 2.4),   # Hirnschädel: rund, keine Kastenform
    (420, 470, (125, 108, 80), 1.6),    # Hinterkopf und Schädelbasis zum Nacken
    (478, 276, (80, 100, 76), 2.2),     # Vorderhirn (Hemisphären) wölbt sich nach vorn
    (548, 380, (50, 64, 52), 1.8),      # Oberkiefer, Gesicht
    (580, 334, (26, 34, 28), 1.8),      # Schnauze (Stirn-Nasen-Wulst)
    (546, 428, (44, 58, 20), 1.8),      # Unterkiefer
]


# Gebaut wird groß (1 Bildpunkt = 1 cm in Blender) und erst am Ende auf echte Größe skaliert: Metaballs haben einen festen
# Einflussradius – im echten Maßstab (1,5 cm) verschmolz alles und das Netz kam auf 0,98 statt ~1,7 cm Länge heraus.
BUILD = 0.01                            # m je Bildpunkt beim Bauen
TO_REAL = PX_CM * CM / BUILD            # Faktor Bau -> echte Größe


def s(v_px):
    return v_px * BUILD


def v3(x, y, side=0.0):
    """Bildpunkt -> Blender-Koordinaten (m): x nach rechts, y (Bild) nach unten = −z, side seitlich."""
    return Vector((s(x), s(side), -s(y)))


def ellipsoid(mb, centre, semi_px, angle=0.0, stiffness=2.0, gain=SINGLE_GAIN, negative=False):
    e = mb.elements.new(type="ELLIPSOID")
    e.co = centre
    e.radius = 1.0
    e.size_x, e.size_y, e.size_z = (s(v) / gain for v in semi_px)
    e.stiffness = stiffness
    e.use_negative = negative
    e.rotation = Matrix.Rotation(-angle, 4, "Y").to_quaternion()
    return e


def catmull(points, samples):
    P = np.array(points, dtype=float)
    dense = []
    for i in range(len(P) - 1):
        p0, p1, p2, p3 = P[max(i - 1, 0)], P[i], P[i + 1], P[min(i + 2, len(P) - 1)]
        for t in np.linspace(0.0, 1.0, 30, endpoint=False):
            t2, t3 = t * t, t * t * t
            dense.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 + (-p0 + 3 * p1 - 3 * p2 + p3) * t3))
    dense.append(P[-1])
    dense = np.array(dense)
    arc = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(dense[:, :2], axis=0), axis=1))])
    target = np.linspace(0.0, arc[-1], samples)
    return np.stack([np.interp(target, arc, dense[:, k]) for k in range(dense.shape[1])], axis=1)


def limb(mb, points_px, radii_px, side_px):
    """Glied als dichte Kugelkette (Arm, Bein); side_px je Punkt oder für alle gleich."""
    sides = side_px if isinstance(side_px, (list, tuple)) else [side_px] * len(points_px)
    for x, y, r, sd in catmull([(p[0], p[1], r, sd) for p, r, sd in zip(points_px, radii_px, sides)], 30):
        e = mb.elements.new(type="BALL")
        e.co = v3(x, y, sd)
        e.radius = s(r) / SINGLE_GAIN
        e.stiffness = 1.8


def build_metaball():
    mb = bpy.data.metaballs.new("MB_Embryo_W08")
    mb.threshold = 0.6
    mb.resolution = s(8.0)
    mb.render_resolution = s(3.0)

    # Rumpf
    dors = catmull([st[0] for st in STATIONS], 60)
    vent = catmull([st[1] for st in STATIONS], 60)
    widths = np.interp(np.linspace(0, 1, 60), np.linspace(0, 1, len(STATIONS)), [st[2] for st in STATIONS])
    for d, v, w in zip(dors, vent, widths):
        mid = 0.5 * (d + v)
        across = d - v
        half_depth = 0.5 * np.linalg.norm(across)
        angle = math.atan2(-across[1], across[0]) - math.pi / 2
        ellipsoid(mb, v3(*mid), (max(half_depth * 0.42, 14.0), w, half_depth), angle, 1.6, CHAIN_GAIN)

    # Kopf
    for x, y, semi, stiff in HEAD:
        ellipsoid(mb, v3(x, y), semi, 0.0, stiff)
    for side in (-1.0, 1.0):
        # Augen: seitlich, nur flach vorgewölbt (das dunkle Pigment malt das Material)
        ellipsoid(mb, v3(475, 365, side * 100), (24, 9, 24), 0.0, 2.0)
        # Ohrhöcker: ein flacher Ring aus sechs Wülsten um die spätere Ohröffnung, kaum erhaben (sonst wirken sie wie
        # aufgeklebte Perlen), in der Mitte die Grube
        for k in range(6):
            a = k * math.pi / 3.0
            ellipsoid(mb, v3(485 + 15 * math.cos(a), 470 + 15 * math.sin(a), side * 92), (10, 6, 10), -a, 1.4)
        ellipsoid(mb, v3(485, 470, side * 100), (8, 8, 8), 0.0, 1.6, negative=True)
    # Mundspalte zwischen Ober- und Unterkiefer (negativ = schneidet ein)
    ellipsoid(mb, v3(566, 404), (26, 48, 5), -0.35, 2.0, negative=True)

    # Herz-Leber-Wölbung: die Körperwand wölbt sich über dem großen Herzen und der Leber
    ellipsoid(mb, v3(750, 412), (80, 96, 70), 0.0, 2.4)
    ellipsoid(mb, v3(838, 408), (72, 90, 64), 0.0, 2.2)
    # Nabelstrang-Ansatz mit dem ausgetretenen Darm (physiologischer Nabelbruch ab Woche 6 nach der Befruchtung)
    ellipsoid(mb, v3(905, 352), (34, 36, 30), 0.0, 2.2)

    # Arme: Schulter an der Flanke, Unterarm nach vorn-oben zur Mitte, Handplatten vor der Brust (Handflächen zueinander)
    for side in (-1.0, 1.0):
        limb(mb, [(695, 548), (692, 505), (680, 462)], [28, 23, 20], [side * 104, side * 102, side * 100])
        ellipsoid(mb, v3(676, 432, side * 102), (44, 11, 32), 0.25, 1.8)
        # Fingerstrahlen: flache, längliche Rippen am Rand der Handplatte, radial – keine Kugeln
        for fx, fy in ((644, 414), (662, 405), (684, 405), (704, 414)):
            angle = math.atan2(-(fy - 432), fx - 676)
            ellipsoid(mb, v3(fx, fy, side * 102), (14, 7, 7), angle, 1.6)
        # Beine: kurz, noch Paddel an der Hüfte, Fußsohlen einander zugewandt – Fußplatte ohne Zehen
        limb(mb, [(935, 545), (952, 502), (964, 462)], [28, 22, 18], [side * 90, side * 80, side * 68])
        ellipsoid(mb, v3(962, 428, side * 64), (40, 11, 22), -0.2, 1.8)

    # Schwanzrest am Steiß
    limb(mb, [(1000, 505), (1006, 526), (996, 546)], [20, 14, 8], 0.0)
    return mb


def to_mesh(mobj, voxel_cm):
    mb = mobj.data
    mb.resolution = mb.render_resolution
    bpy.context.view_layer.update()
    mesh = bpy.data.meshes.new_from_object(mobj.evaluated_get(bpy.context.evaluated_depsgraph_get()))
    obj = bpy.data.objects.new("SM_GEN_Fetus_W%02d" % WEEK, mesh)
    bpy.context.scene.collection.objects.link(obj)
    bpy.data.objects.remove(mobj, do_unlink=True)
    remesh = obj.modifiers.new("Gleichmaessig", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = voxel_cm * CM
    smooth = obj.modifiers.new("Glaetten", "SMOOTH")
    smooth.factor = 0.5
    smooth.iterations = 6
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    for mod in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    # Ausdünnen auf ~200 000 Dreiecke: auf 1,5 cm ist das unter der Pixelgröße jeder Kameraeinstellung im Spiel
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    decimate = obj.modifiers.new("Ausduennen", "DECIMATE")
    decimate.ratio = min(1.0, TRIANGLES / tris)
    bpy.ops.object.modifier_apply(modifier=decimate.name)
    for poly in obj.data.polygons:
        poly.use_smooth = True
    return obj


def frame():
    """Bildebene -> Netzrahmen: Ursprung zwischen den Augen, +X Blick zur Schnauze, +Z senkrecht dazu (Scheitel)."""
    eye = v3(*EYE_PX)
    gaze = (v3(*GAZE_TO_PX) - eye).normalized()
    up = Vector((-gaze.z, 0.0, gaze.x))          # in der Bildebene um 90° gedreht
    if up.z < 0:
        up = -up
    left = up.cross(gaze).normalized()
    basis = Matrix((gaze, left, up))             # Zeilen = neue Achsen
    return eye, basis


def build():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    mobj = bpy.data.objects.new("MB_Embryo_W08", build_metaball())
    bpy.context.scene.collection.objects.link(mobj)
    obj = to_mesh(mobj, 694.0 / 520.0 * BUILD / CM)
    eye, basis = frame()
    co = np.array([v.co[:] for v in obj.data.vertices])
    local = (co - np.array(eye[:])) @ np.array(basis).T * TO_REAL
    for v, p in zip(obj.data.vertices, local):
        v.co = Vector(p)
    obj.data.update()
    navel = np.array(basis) @ (np.array(v3(*NAVEL_PX)[:]) - np.array(eye[:])) * TO_REAL
    # Hülle: äußerste Punkte in 96 Richtungen (wie build_fetus.py)
    hull = []
    golden = math.pi * (3.0 - math.sqrt(5.0))
    for i in range(96):
        zc = 1.0 - 2.0 * (i + 0.5) / 96
        r = math.sqrt(1.0 - zc * zc)
        d = np.array([math.cos(golden * i) * r, math.sin(golden * i) * r, zc])
        hull.append([float(x) / CM for x in local[np.argmax(local @ d)]])
    info = dict(navel=[float(x) / CM for x in navel], center=[float(x) / CM for x in local.mean(axis=0)], crl=CRL_CM,
                hull=hull, eye_half_cm=104 * PX_CM, eye_sideways=1.0, source="build_embryo_w08.py")
    print("GENESIS: Embryo SSW 8 – %d Punkte, Ausdehnung %s cm" % (len(obj.data.vertices), [round(v / CM, 2) for v in obj.dimensions]))
    return obj, info


def export(out_dir=OUT_DIR):
    obj, info = build()
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(out_dir, obj.name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             object_types={"MESH"}, mesh_smooth_type="FACE", use_tspace=False, add_leaf_bones=False)
    table_path = os.path.join(out_dir, "fetus_weeks.json")
    table = json.load(open(table_path, encoding="utf-8")) if os.path.exists(table_path) else {}
    table[str(WEEK)] = info
    json.dump(table, open(table_path, "w", encoding="utf-8"), indent=1)
    print("GENESIS: exportiert", path, round(os.path.getsize(path) / 1e6, 1), "MB")
