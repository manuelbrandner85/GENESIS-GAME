# GENESIS: Der Kreislauf des Lebens
#
# Das Kind im Mutterleib, Woche für Woche (GENESIS-044 Teil 2b, Docs/37, Referenzen Docs/36 Board B).
#
# Grundlage ist ein anatomisch sauberes Menschenmodell aus MPFB2 (MakeHuman, Blender-Erweiterung; erzeugte Menschen sind
# CC0): ein Säugling (Alter-Regler 0). Daraus wird je SSW der Fetus:
#   * Maße: Kopf, Rumpf, Arme, Beine, Füße einzeln auf die Wachstumskurven skaliert (WEEKS, 50. Perzentile)
#   * Fett: früh dünn, ab SSW 28 zunehmend rund (Makro „weight“)
#   * Haltung: Rücken gerundet, Kinn zur Brust, Hände am Gesicht (IK), Beine angezogen, Finger locker gebeugt, Lider zu
#
# Lage des Netzes: Ursprung = Mitte zwischen den Augen, +X = Blickrichtung, +Z = Scheitel (so setzt die Szene das Kind
# dorthin, wo die Kamera – seine Augen – ist). Je Woche ein Netz SM_GEN_Fetus_Wxx und eine Tabelle fetus_weeks.json.
#
# Voraussetzung: MPFB2 in Blender installiert (extensions.blender.org, add-on mpfb).
# Headless:  blender -b --factory-startup --python-expr "g={'__name__':'genesis'}; exec(open(r'...build_fetus.py').read(), g); g['export_all']()"

import importlib
import json
import math
import os

import addon_utils
import bpy
import numpy as np
from mathutils import Matrix, Vector

CM = 0.01
OUT_DIR = r"C:\Users\manue\Desktop\Genesis Game\ArtSource\Generated\Gestation"

# Wachstum (50. Perzentile, cm; Recherche Docs/37): Scheitel-Steiß (CRL: Robinson & Fleming 1975, ab 16 SSW Archie 2006),
# Kopf- und Bauchumfang, Femur (INTERGROWTH-21st, Papageorghiou 2014), Humerus und Fuß (Chitty & Altman 2002).
# SSW 8–10: nur die Scheitel-Steiß-Länge ist belastbar – die übrigen Maße nach den Proportionen (Kopf ≈ halbe Länge).
WEEKS = {
    8:  dict(CRL=1.55, HC=2.2,   AC=1.7,   FL=0.12, HL=0.14, FT=0.20),
    10: dict(CRL=3.2,  HC=4.3,   AC=3.3,   FL=0.33, HL=0.36, FT=0.42),
    12: dict(CRL=5.5,  HC=6.81,  AC=5.58,  FL=0.77, HL=0.71, FT=0.89),
    16: dict(CRL=11.6, HC=12.29, AC=10.32, FL=1.95, HL=2.0,  FT=2.06),
    20: dict(CRL=16.6, HC=17.25, AC=14.77, FL=3.13, HL=3.15, FT=3.26),
    24: dict(CRL=21.2, HC=21.91, AC=19.12, FL=4.19, HL=4.07, FT=4.44),
    28: dict(CRL=25.4, HC=26.04, AC=23.33, FL=5.13, HL=4.85, FT=5.53),
    32: dict(CRL=29.2, HC=29.44, AC=27.39, FL=5.94, HL=5.5,  FT=6.51),
    36: dict(CRL=32.7, HC=31.94, AC=31.28, FL=6.64, HL=6.03, FT=7.33),
    40: dict(CRL=35.7, HC=33.39, AC=34.98, FL=7.21, HL=6.47, FT=7.96),
}

SIDES = (".L", ".R")
LID_CLOSE = -40.0                                         # Oberlid-Knochen um X: −40° schließt die Lider


def smooth(a, b, x):
    t = min(1.0, max(0.0, (x - a) / (b - a)))
    return t * t * (3.0 - 2.0 * t)


def mpfb():
    name = next(m.__name__ for m in addon_utils.modules() if m.__name__.endswith(".mpfb"))
    addon_utils.enable(name, default_set=True)
    hs = importlib.import_module(name + ".services.humanservice").HumanService
    ts = importlib.import_module(name + ".services.targetservice").TargetService
    return hs, ts


def clean_scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for coll in (bpy.data.meshes, bpy.data.armatures, bpy.data.materials):
        for item in list(coll):
            coll.remove(item)


def create_baby(week):
    hs, ts = mpfb()
    macro = ts.get_default_macro_info_dict()
    macro["age"] = 0.0                                   # Säugling – der jüngste Körper, den MakeHuman kennt
    macro["gender"] = 0.5
    macro["muscle"] = 0.2
    macro["weight"] = 0.1 + 0.75 * smooth(26.0, 40.0, week)   # Fett erst ab SSW 28
    # feet_on_ground übernimmt den Maßstab ins Netz (sonst liegen die Punkte in MakeHuman-Einheiten)
    basemesh = hs.create_human(macro_detail_dict=macro, scale=0.1, feet_on_ground=True)
    rig = hs.add_builtin_rig(basemesh, "default")
    # Die Körperform steckt in Formschlüsseln (die Rohpunkte sind der Erwachsene): übernehmen, dann erst messen
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = basemesh
    basemesh.select_set(True)
    if basemesh.data.shape_keys:
        bpy.ops.object.shape_key_remove(all=True, apply_mix=True)
    return basemesh, rig


# ---------------------------------------------------------------------------------------------------------------------
# Messen und Proportionen
# ---------------------------------------------------------------------------------------------------------------------

def bone_len(rig, *names):
    return sum(rig.data.bones[n].length for n in names) / CM


def girth_at(co, z, band, half_width=None):
    """Umfang (cm) eines waagerechten Schnitts um die Höhe z (m) – als Ellipse aus Breite und Tiefe."""
    sel = co[np.abs(co[:, 2] - z) < band]
    if half_width is not None:
        sel = sel[np.abs(sel[:, 0]) <= half_width]          # Arme ausschließen
    if len(sel) < 3:
        return 0.0
    a = (sel[:, 0].max() - sel[:, 0].min()) / 2
    b = (sel[:, 1].max() - sel[:, 1].min()) / 2
    return math.pi * (3 * (a + b) - math.sqrt((3 * a + b) * (a + 3 * b))) / CM


def measure(basemesh, rig):
    """Maße des Grundmodells (cm): Kopfumfang, Rumpflänge, Bauchumfang, Oberarm, Oberschenkel, Fuß."""
    bones = rig.data.bones
    head_z = bones["head"].head_local.z
    co = np.array([v.co[:] for v in basemesh.data.vertices])
    body = basemesh.vertex_groups["body"].index
    mask = np.array([any(g.group == body for g in v.groups) for v in basemesh.data.vertices])
    co = co[mask]
    top = co[:, 2].max()
    hc = girth_at(co, head_z + 0.55 * (top - head_z), 0.008)
    trunk = (bones["neck01"].head_local - bones["spine05"].head_local).length / CM
    # Bauch: nur bis zur Hüftbreite (die Arme hängen seitlich daneben)
    hip_half = abs(bones["upperleg01.L"].head_local.x) * 1.35
    ac = girth_at(co, bones["spine04"].head_local.z, 0.008, hip_half)
    return dict(HC=hc, trunk=trunk, AC=ac, HL=bone_len(rig, "upperarm01.L", "upperarm02.L"),
                FL=bone_len(rig, "upperleg01.L", "upperleg02.L"), FT=extent(basemesh, ("foot", "toe")),
                HAND=extent(basemesh, ("wrist", "finger", "metacarpal")) * 0.9)


def extent(basemesh, prefixes):
    """Länge (cm) der Punkte, die überwiegend an diesen Knochen hängen, entlang ihrer Hauptrichtung (linke Seite)."""
    idx = [vg.index for vg in basemesh.vertex_groups if vg.name.endswith(".L") and any(vg.name.startswith(p) for p in prefixes)]
    pts = np.array([v.co[:] for v in basemesh.data.vertices if any(g.group in idx and g.weight > 0.5 for g in v.groups)])
    centered = pts - pts.mean(axis=0)
    axis = np.linalg.svd(centered, full_matrices=False)[2][0]
    proj = centered @ axis
    return float(proj.max() - proj.min()) / CM


def proportion(rig, base, week):
    """Knochen einzeln auf die Maße der Woche skalieren (ohne Weitergabe an die Kinder, dann stimmt jede Länge absolut)."""
    t = WEEKS[week]
    # Rumpf: Scheitel-Steiß minus Kopf (Scheitel–Kinn ≈ 1,2 × Kopfdurchmesser) minus ein kurzer Hals
    trunk_target = max(0.3 * t["CRL"], t["CRL"] - 1.2 * t["HC"] / math.pi - 0.04 * t["CRL"])
    s_head = t["HC"] / base["HC"]
    s_trunk = trunk_target / base["trunk"]
    s_girth = t["AC"] / base["AC"]
    s_arm = t["HL"] / base["HL"]
    s_leg = t["FL"] / base["FL"]
    s_foot = t["FT"] / base["FT"]
    s_hand = 0.8 * t["FT"] / base["HAND"]                   # Hand ≈ 0,8 × Fuß (Halder 1999, Hebbar 2013)
    for bone in rig.data.bones:
        bone.inherit_scale = "NONE"
    for pb in rig.pose.bones:
        n = pb.name
        if n.startswith("spine") or n.startswith("pelvis") or n.startswith("breast") or n == "root":
            s = (s_girth, s_trunk, s_girth)                  # Y = entlang des Knochens
        elif n.startswith("neck"):
            s = (s_head * 0.9, s_trunk * 0.8, s_head * 0.9)
        elif n.startswith("clavicle") or n.startswith("shoulder"):
            s = (s_girth,) * 3
        elif n.startswith("upperarm") or n.startswith("lowerarm"):
            s = (s_arm,) * 3
        elif n.startswith("wrist") or n.startswith("finger") or n.startswith("metacarpal"):
            s = (s_hand,) * 3
        elif n.startswith("upperleg") or n.startswith("lowerleg"):
            s = (s_leg,) * 3
        elif n.startswith("foot") or n.startswith("toe"):
            s = (s_foot,) * 3
        else:
            s = (s_head,) * 3                                  # Kopf, Kiefer, Gesicht
        pb.scale = s
    bpy.context.view_layer.update()
    return dict(head=s_head, trunk=s_trunk, girth=s_girth, arm=s_arm, leg=s_leg, foot=s_foot)


# ---------------------------------------------------------------------------------------------------------------------
# Haltung
# ---------------------------------------------------------------------------------------------------------------------

def rotate_world(rig, name, axis, degrees):
    """Einen Knochen (mit allem, was an ihm hängt) um eine Welt-Achse durch seinen Kopf drehen."""
    bpy.context.view_layer.update()
    pb = rig.pose.bones[name]
    head = pb.head.copy()
    R = Matrix.Rotation(math.radians(degrees), 4, axis)
    pb.matrix = Matrix.Translation(head) @ R @ Matrix.Translation(-head) @ pb.matrix
    bpy.context.view_layer.update()


def pose(rig, week):
    """
    Fetale Haltung (Docs/37): Rücken nach vorn gerundet, Kinn zur Brust, Oberschenkel am Bauch, Knie gebeugt, Füße
    angezogen, Hände an Kinn und Wange, Finger locker gebeugt. Das Modell schaut nach −Y, links ist +X: Beugen nach vorn
    = positive Drehung um +X für nach oben zeigende Knochen.
    """
    late = smooth(26.0, 38.0, week)
    early = 1.0 - smooth(9.0, 14.0, week)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="POSE")
    X, Y, Z = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
    # Zum Termin wird es eng: Das Kind liegt als „Fruchtwalze“ – Rücken stärker gerundet, Oberschenkel bis an den Bauch,
    # Knie ganz gebeugt, Beine näher beieinander (Williams Obstetrics, Haltung = Flexion). Vorher blieb die Haltung
    # von SSW 28 bis 40 fast gleich, und am Termin ragten die Füße durch die Wand (fetus_fit.py: 6 % zu groß).
    term = smooth(32.0, 40.0, week)
    for name, deg in (("spine05", 8), ("spine04", 10), ("spine03", 10), ("spine02", 8), ("spine01", 6)):
        rotate_world(rig, name, X, deg * (1.0 + 0.6 * early + 0.3 * late + 0.35 * term))
    for name, deg in (("neck01", 10), ("neck02", 10), ("neck03", 8), ("head", 8)):
        rotate_world(rig, name, X, deg * (1.0 + 0.4 * early + 0.3 * late + 0.2 * term))
    for side, sx in ((".L", 1.0), (".R", -1.0)):
        # Fetale Haltung (Williams Obstetrics): Oberschenkel über den Bauch gebeugt, Knie ganz gebeugt, Unterschenkel
        # gekreuzt, Füße an Gesäß und Gegenseite – so kompakt, dass das Kind in die Fruchtblase passt
        # Oberschenkel seitlich am Bauch vorbei (gespreizt, nicht in ihn hinein), Knie ganz gebeugt, Füße gekreuzt
        rotate_world(rig, "upperleg01" + side, X, -(95.0 + 10.0 * late + 12.0 * term))
        rotate_world(rig, "upperleg01" + side, Y, (42.0 - 10.0 * term) * sx)  # gespreizt, Knie neben dem Bauch
        rotate_world(rig, "lowerleg01" + side, X, 122.0 + 6.0 * late + 10.0 * term)
        rotate_world(rig, "lowerleg01" + side, Z, 45.0 * sx)                  # Unterschenkel zur Mitte, Füße kreuzen
        rotate_world(rig, "foot" + side, X, -35.0)
    # Arme per IK: das Handgelenk vor Kinn bzw. Wange, Ellbogen nach unten-außen. Richtungen aus dem Kopf-Rahmen
    # (Augen, Kopfknochen), damit sie zur Beugung des Halses passen.
    bpy.context.view_layer.update()
    eye_l, eye_r = rig.pose.bones["eye.L"].head.copy(), rig.pose.bones["eye.R"].head.copy()
    left = (eye_l - eye_r).normalized()
    up = (rig.pose.bones["head"].matrix.to_3x3() @ Vector((0, 1, 0))).normalized()
    fwd = left.cross(up).normalized()
    up = fwd.cross(left).normalized()
    chin = rig.pose.bones["jaw"].tail.copy()
    hand = 0.8 * WEEKS[week]["FT"] * CM
    helpers = []
    for side, sx in ((".L", 1.0), (".R", -1.0)):
        if sx < 0:
            wrist = chin + fwd * 0.7 * hand - up * 0.55 * hand - left * 0.25 * hand      # rechte Hand am Kinn, nah am Mund
        else:
            wrist = chin + fwd * 0.55 * hand + up * 0.1 * hand + left * 0.95 * hand      # linke Hand an der Wange
        target = bpy.data.objects.new("Ziel" + side, None)
        bpy.context.scene.collection.objects.link(target)
        target.location = rig.matrix_world @ wrist
        pole = bpy.data.objects.new("Pol" + side, None)
        bpy.context.scene.collection.objects.link(pole)
        elbow = rig.pose.bones["lowerarm01" + side].head
        pole.location = rig.matrix_world @ (elbow + (left * 0.7 * sx - up * 1.0 - fwd * 0.2).normalized() * 6 * hand)
        ik = rig.pose.bones["lowerarm02" + side].constraints.new("IK")
        ik.target = target
        ik.pole_target = pole
        ik.pole_angle = math.radians(-90)
        ik.chain_count = 4
        ik.use_stretch = False
        helpers += [target, pole]
    bpy.context.view_layer.update()
    bpy.ops.pose.select_all(action="SELECT")
    bpy.ops.pose.visual_transform_apply()
    for pb in rig.pose.bones:
        for c in list(pb.constraints):
            pb.constraints.remove(c)
    for h in helpers:
        bpy.data.objects.remove(h)
    # Finger locker gebeugt (der kleine Finger am stärksten), Daumen angelegt
    for side in SIDES:
        for finger, curl in ((2, 25), (3, 30), (4, 35), (5, 42)):
            for seg in (1, 2, 3):
                pb = rig.pose.bones["finger%d-%d%s" % (finger, seg, side)]
                pb.rotation_mode = "XYZ"
                pb.rotation_euler.x += math.radians(curl * (1.0 if seg == 1 else 0.9))
        for seg in (2, 3):
            pb = rig.pose.bones["finger1-%d%s" % (seg, side)]
            pb.rotation_mode = "XYZ"
            pb.rotation_euler.x += math.radians(15)
    # Lider zu (vor SSW 26 verwachsen, danach im Schlaf – die meiste Zeit)
    for side in SIDES:
        pb = rig.pose.bones["orbicularis03" + side]
        pb.rotation_mode = "XYZ"
        pb.rotation_euler.x += math.radians(LID_CLOSE)
    bpy.context.view_layer.update()
    bpy.ops.object.mode_set(mode="OBJECT")


def smooth_folds(obj, rig):
    """Kniekehle, Leiste, Ellenbeuge: dort, wo sich die Haut beim Beugen staucht, gezielt glätten (Laplace)."""
    import bmesh
    joints = []
    # Am Knie nur ein enger Bereich: Mit dem Radius der Leiste (0,9 × Oberschenkel) wurde die Kniescheibe mitgeglättet –
    # von vorn sahen die Knie flach aus. Die Kniekehle liegt hinten, dicht am Gelenk.
    for name, share in (("lowerleg01", 0.35), ("upperleg01", 0.9), ("lowerarm01", 0.9)):
        for side in SIDES:
            pb = rig.pose.bones.get(name + side)
            if pb is not None:
                parent = pb.parent
                radius = share * (parent.length if parent is not None else pb.length)
                joints.append((np.array((rig.matrix_world @ pb.head)[:]), radius))
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()
    co = np.array([v.co[:] for v in bm.verts])
    weight = np.zeros(len(co))
    for center, radius in joints:
        d = np.linalg.norm(co - center, axis=1)
        weight = np.maximum(weight, np.clip(1.0 - d / radius, 0.0, 1.0))
    neighbours = [[e.other_vert(v).index for e in v.link_edges] for v in bm.verts]
    for _ in range(12):
        avg = np.array([co[n].mean(axis=0) if n else co[i] for i, n in enumerate(neighbours)])
        co = co + (avg - co) * (0.6 * weight)[:, None]
    for v, p in zip(bm.verts, co):
        v.co = p
    bm.to_mesh(obj.data)
    bm.free()


def bake(basemesh, rig):
    """Haltung ins Netz übernehmen, Hilfsgeometrie entfernen, glätten."""
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = basemesh
    basemesh.select_set(True)
    # Die Körperform steckt in Formschlüsseln (MPFB-Ziele): zuerst ins Netz übernehmen
    if basemesh.data.shape_keys:
        bpy.ops.object.shape_key_remove(all=True, apply_mix=True)
    for mod in list(basemesh.modifiers):
        if mod.type not in ("MASK", "ARMATURE"):
            basemesh.modifiers.remove(mod)
    # Starke Beugung (Hüfte, Knie, Ellbogen) staucht die Haut beim Skinning: eine korrigierende Glättung nach dem Skelett
    # hält die Form der Ruhelage und glättet nur, was dabei zerknittert oder aufreißt
    fix = basemesh.modifiers.new("Korrektur", "CORRECTIVE_SMOOTH")
    fix.rest_source = "ORCO"
    fix.smooth_type = "LENGTH_WEIGHTED"
    fix.factor = 0.8
    fix.iterations = 20
    for mod in list(basemesh.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    smooth_folds(basemesh, rig)
    sub = basemesh.modifiers.new("Glatt", "SUBSURF")
    sub.levels = 2
    bpy.ops.object.modifier_apply(modifier=sub.name)
    # Wo sich Glieder und Rumpf berühren (Oberschenkel am Bauch, Hand am Kinn), liegen zwei Hautflächen ineinander.
    # Ein feines Volumen-Neuvernetzen macht daraus eine geschlossene Haut mit weicher Falte – wie echte Haut
    height = max(basemesh.dimensions)
    remesh = basemesh.modifiers.new("Eine Haut", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = height / 900.0
    remesh.adaptivity = 0.0
    bpy.ops.object.modifier_apply(modifier=remesh.name)
    relax = basemesh.modifiers.new("Falten weich", "SMOOTH")
    relax.factor = 0.5
    relax.iterations = 4
    bpy.ops.object.modifier_apply(modifier=relax.name)
    # Ausdünnen: ~250 000 Flächen genügen für Nanite und halten Lider, Lippen, Finger (Datei ~10 MB statt 80 MB)
    thin = basemesh.modifiers.new("Dichte", "DECIMATE")
    thin.ratio = min(1.0, 250000 / max(1, len(basemesh.data.polygons)))
    bpy.ops.object.modifier_apply(modifier=thin.name)
    bpy.data.objects.remove(rig)
    for p in basemesh.data.polygons:
        p.use_smooth = True
    return basemesh


def build(week):
    clean_scene()
    basemesh, rig = create_baby(week)
    base = measure(basemesh, rig)
    scales = proportion(rig, base, week)
    pose(rig, week)
    # Augenmitte und Blickrichtung aus dem Skelett (Augenknochen), bevor das Skelett verschwindet
    bpy.context.view_layer.update()
    eye_l = rig.matrix_world @ rig.pose.bones["eye.L"].head
    eye_r = rig.matrix_world @ rig.pose.bones["eye.R"].head
    eyes = (eye_l + eye_r) * 0.5
    head_m = (rig.matrix_world @ rig.pose.bones["head"].matrix).to_3x3()
    up = (head_m @ Vector((0, 1, 0))).normalized()                # der Kopfknochen zeigt zum Scheitel
    left = (eye_l - eye_r).normalized()
    forward = left.cross(up).normalized()
    up = forward.cross(left).normalized()
    navel_ref = rig.matrix_world @ rig.pose.bones["spine04"].head
    obj = bake(basemesh, rig)
    # In den Augen-Raum setzen: Ursprung zwischen den Augen, +X vorn, +Y links, +Z oben
    basis = np.array(Matrix((forward, left, up)))
    co = np.array([v.co[:] for v in obj.data.vertices])
    local = (co - np.array(eyes[:])) @ basis.T
    for v, p in zip(obj.data.vertices, local):
        v.co = Vector(p)
    obj.data.update()
    # Nabel: vorderster Punkt des Bauchs auf Höhe von spine04
    nref = basis @ (np.array(navel_ref[:]) - np.array(eyes[:]))
    near = np.abs(local[:, 2] - nref[2]) < 0.06 * WEEKS[week]["CRL"] * CM
    navel = local[near][np.argmax(local[near, 0])] if near.any() else nref
    obj.name = "SM_GEN_Fetus_W%02d" % week
    obj.data.materials.clear()
    # Hülle: die äußersten Punkte in 96 Richtungen – damit legt das Spiel das Kind so in die Höhle, dass nichts herausragt
    hull = []
    golden = math.pi * (3.0 - math.sqrt(5.0))
    for i in range(96):
        zc = 1.0 - 2.0 * (i + 0.5) / 96
        r = math.sqrt(1.0 - zc * zc)
        d = np.array([math.cos(golden * i) * r, math.sin(golden * i) * r, zc])
        hull.append([float(x) / CM for x in local[np.argmax(local @ d)]])
    info = dict(navel=[float(x) / CM for x in navel], center=[float(x) / CM for x in local.mean(axis=0)],
                crl=WEEKS[week]["CRL"], scales=scales, base=base, hull=hull)
    print("GENESIS: Fetus SSW %d – %d Flächen, Grundmodell %s, Skalen %s" % (week, len(obj.data.polygons),
          {k: round(v, 1) for k, v in base.items()}, {k: round(v, 3) for k, v in scales.items()}))
    return obj, info


def export_all(out_dir=OUT_DIR, weeks=None):
    os.makedirs(out_dir, exist_ok=True)
    table_path = os.path.join(out_dir, "fetus_weeks.json")
    table = json.load(open(table_path, encoding="utf-8")) if os.path.exists(table_path) else {}
    for week in weeks or sorted(WEEKS):
        obj, info = build(week)
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        path = os.path.join(out_dir, obj.name + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                                 object_types={"MESH"}, mesh_smooth_type="FACE", use_tspace=False, add_leaf_bones=False)
        table[str(week)] = info
        print("GENESIS: exportiert", obj.name, round(os.path.getsize(path) / 1e6, 1), "MB")
    with open(table_path, "w", encoding="utf-8") as f:
        json.dump(table, f, indent=1)
