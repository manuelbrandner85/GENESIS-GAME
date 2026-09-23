# GENESIS: Der Kreislauf des Lebens
#
# Die Hand des Kindes – die eigene Hand des Spielers im Mutterleib (GENESIS-044 Teil 2a, Docs/37).
#
#   * Rechte Hand mit Unterarm, gebaut für die Größe am Termin (Handlänge ~6 cm, Neugeborene 6–6,5 cm);
#     AGenesisWombScene skaliert mit der Woche.
#   * Fetale Proportionen: kurze, weiche Finger mit rundlichen Kuppen, breiter Handteller, dicker Daumenballen,
#     der Unterarm kurz. Die Haut ist dünn – im Gegenlicht der Bauchwand leuchten die Finger rot durch (Material).
#   * Skelett: Unterarm, Handteller, Daumen (3), vier Finger (je 3) – damit die Hand sich öffnen, schließen (Greifreflex)
#     und zum Mund gehen kann. Gewichte automatisch aus der Nähe zu den Knochen.
#
# Einheit: cm im Skript, exportiert in m (wie die übrigen Gestation-Bauteile). Achsen: +X vom Unterarm zu den Fingern,
# +Y zur Daumenseite, +Z Handrücken.
#
# Headless:  blender -b --factory-startup --python-expr "g={'__name__':'genesis'}; exec(open(r'...build_fetal_hand.py').read(), g); g['export_all']()"

import math
import os
import bpy
import numpy as np
from mathutils import Vector

CM = 0.01
OUT_DIR = r"C:\Users\manue\Desktop\Genesis Game\ArtSource\Generated\Gestation"
ASSET = "SK_GEN_FetalHand"

# Finger: Name, Kopf des Mittelhandknochens (x, y), Spreizung (Grad), Längen der Glieder (cm), Radius am Grund (cm)
# Neugeborenenhand: der Mittelfinger etwa 43 % der Handlänge, kurze, weiche Glieder, breiter Handteller.
FINGERS = [
    ("Index", (2.95, 0.80), 7.0, (1.10, 0.72, 0.55), 0.35),
    ("Middle", (3.05, 0.27), 1.0, (1.22, 0.80, 0.58), 0.36),
    ("Ring", (2.95, -0.27), -5.0, (1.12, 0.75, 0.55), 0.34),
    ("Little", (2.72, -0.78), -11.0, (0.86, 0.56, 0.46), 0.30),
]
THUMB_BASE = Vector((0.60, 0.92, -0.30))
THUMB_DIR = Vector((0.62, 0.66, -0.42)).normalized()
THUMB_LENGTHS = (0.90, 0.80, 0.62)


def clean_scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh)
    for arm in list(bpy.data.armatures):
        bpy.data.armatures.remove(arm)


def chains():
    """Die Knochenketten (cm): Name → Liste von Punkten, dazu Radien der Haut an jedem Punkt."""
    result = {}
    result["Forearm"] = ([Vector((-6.5, 0.0, 0.0)), Vector((0.0, 0.0, 0.0))], [0.78, 0.62])
    result["Palm"] = ([Vector((0.0, 0.0, 0.0)), Vector((2.9, 0.0, 0.0))], [0.62, 0.55])
    for name, (hx, hy), spread, lengths, radius in FINGERS:
        head = Vector((hx, hy, 0.02))
        direction = Vector((math.cos(math.radians(spread)), math.sin(math.radians(spread)), -0.08)).normalized()
        points = [head]
        for length in lengths:
            points.append(points[-1] + direction * length)
            direction = (direction + Vector((0.0, 0.0, -0.10))).normalized()     # entspannt leicht gebeugt
        radii = [radius, radius * 0.92, radius * 0.86, radius * 0.80]
        result[name] = (points, radii)
    points = [THUMB_BASE]
    direction = THUMB_DIR.copy()
    for length in THUMB_LENGTHS:
        points.append(points[-1] + direction * length)
        direction = (direction + Vector((0.12, -0.05, 0.0))).normalized()
    result["Thumb"] = (points, [0.46, 0.40, 0.36, 0.31])
    return result


def build_skin_mesh(bone_chains):
    """Haut über ein Skin-Gerüst: Unterarm, Handteller (die Mittelhandknochen als Strahlen), Finger, Daumen."""
    verts, edges, radii = [], [], []

    def add_chain(points, rads, connect_to=None, steps=3):
        prev = connect_to
        for i in range(len(points) - 1):
            for s in range(steps):
                if i == 0 and s == 0 and connect_to is not None:
                    continue                                   # der Anfang ist der Punkt, an den die Kette anschließt
                t = s / steps
                p = points[i].lerp(points[i + 1], t)
                r = rads[i] + (rads[i + 1] - rads[i]) * t
                verts.append(p)
                radii.append(r)
                idx = len(verts) - 1
                if prev is not None:
                    edges.append((prev, idx))
                prev = idx
        verts.append(points[-1])
        radii.append(rads[-1])
        idx = len(verts) - 1
        edges.append((prev, idx))
        return idx

    arm_points, arm_radii = bone_chains["Forearm"]
    wrist = add_chain(arm_points, arm_radii, None, steps=5)
    # Mittelhand: vom Handgelenk zu jedem Fingergrund, dazu die Finger
    for name, (hx, hy), _spread, _lengths, radius in FINGERS:
        points, rads = bone_chains[name]
        base = Vector((0.35, hy * 0.45, 0.0))
        start = add_chain([Vector((0.0, 0.0, 0.0)), base], [0.60, 0.50], wrist, steps=1)
        knuckle = add_chain([base, points[0]], [0.50, radius * 1.18], start, steps=3)
        add_chain(points, rads, knuckle, steps=3)
    points, rads = bone_chains["Thumb"]
    ball = add_chain([Vector((0.0, 0.0, 0.0)), points[0]], [0.60, 0.55], wrist, steps=2)
    add_chain(points, rads, ball, steps=3)

    mesh = bpy.data.meshes.new(ASSET + "_Mesh")
    mesh.from_pydata([tuple(v * CM) for v in verts], edges, [])
    obj = bpy.data.objects.new(ASSET, mesh)
    bpy.context.scene.collection.objects.link(obj)
    modifier = obj.modifiers.new("Haut", "SKIN")
    modifier.use_smooth_shade = True
    modifier.branch_smoothing = 0.8
    skin = mesh.skin_vertices[0].data
    for i, r in enumerate(radii):
        skin[i].radius = (r * CM, r * CM)
    skin[0].use_root = True
    obj.modifiers.new("Glatt", "SUBSURF").levels = 2

    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    for mod in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)

    # Handteller flach, Daumenballen voll, Fingerkuppen rundlich; die Handinnenfläche leicht hohl
    for v in obj.data.vertices:
        x, y, z = v.co / CM
        if -0.3 < x < 3.1:
            palm = min(1.0, (x + 0.3) / 0.6) * min(1.0, (3.1 - x) / 0.6)
            z *= 1.0 - 0.38 * palm
            if z < 0.0:
                z += 0.06 * palm * max(0.0, 1.0 - abs(y) / 1.1)
        v.co = Vector((x, y, z)) * CM
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


def build_armature(bone_chains):
    arm_data = bpy.data.armatures.new("Armature")
    arm = bpy.data.objects.new("Armature", arm_data)
    bpy.context.scene.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode="EDIT")
    bones = arm_data.edit_bones

    def bone(name, head, tail, parent=None):
        b = bones.new(name)
        b.head = head * CM
        b.tail = tail * CM
        b.roll = 0.0
        if parent:
            b.parent = parent
            b.use_connect = False
        return b

    forearm = bone("Forearm", *bone_chains["Forearm"][0])
    palm = bone("Palm", *bone_chains["Palm"][0], parent=forearm)
    for name, *_ in FINGERS:
        points = bone_chains[name][0]
        parent = palm
        for i, suffix in enumerate(("1", "2", "3")):
            parent = bone(name + suffix, points[i], points[i + 1], parent)
    points = bone_chains["Thumb"][0]
    parent = palm
    for i, suffix in enumerate(("1", "2", "3")):
        parent = bone("Thumb" + suffix, points[i], points[i + 1], parent)
    bpy.ops.object.mode_set(mode="OBJECT")
    return arm


META_SCALE = 1.0 / 0.575    # ein Metaball-Element ist sichtbar nur ~0,575 × seiner Größe (Steifigkeit 2, Schwelle 0,6)


def _gauss(x, width):
    return np.exp(-(x / width) ** 2)


def _fold(x, width):
    """Hautfalte: eine weiche Rinne, daneben die zusammengeschobene Haut als flacher Wulst (kein geritzter Strich)."""
    return _gauss(x, width) - 0.35 * _gauss(x, 2.6 * width)


def anatomy(obj, bone_chains):
    """
    Was eine Neugeborenenhand von einer Knetform unterscheidet (cm, entlang der Normalen):
      * Beugefalten der Finger (Grund-, Mittel-, Endgelenk) und des Handtellers (Fünffinger- und Daumenfurche)
      * Grübchen über den Fingerknöcheln – Babyhände haben dort Dellen statt Knöchel
      * die Speckfalte am Handgelenk („Armband“), davor und dahinter kleine Wülste
      * feine Querfalten auf dem Handrücken der Fingergelenke
      * Nägel: dünn, leicht erhaben, mit Nagelwall; die Beere darunter flacher
    Dazu Datenkanäle für das Material (UV-Kanal 2): x = Nagel, y = Falte.
    """
    mesh = obj.data
    co = np.array([v.co[:] for v in mesh.vertices])
    nrm = np.array([v.normal[:] for v in mesh.vertices])
    count = len(co)
    offset = np.zeros(count)
    nail = np.zeros(count)
    crease = np.zeros(count)
    palmar = np.clip(-nrm[:, 2], 0.0, 1.0)
    dorsal = np.clip(nrm[:, 2], 0.0, 1.0)

    # Nächstes Fingerglied je Punkt
    segments = []
    for name in [f[0] for f in FINGERS] + ["Thumb"]:
        points, rads = bone_chains[name]
        for i in range(3):
            segments.append((name, i, np.array(points[i][:]), np.array(points[i + 1][:]), rads[i + 1]))
    best = np.full(count, 1e9)
    seg_of = np.full(count, -1)
    along = np.zeros(count)
    lateral = np.zeros(count)
    for k, (name, i, a, b, r) in enumerate(segments):
        d = b - a
        length = np.linalg.norm(d)
        u = d / length
        t = np.clip((co - a) @ u, -0.3, length + 0.5)
        foot = a + t[:, None] * u
        dist = np.linalg.norm(co - foot, axis=1)
        side = np.cross(u, [0.0, 0.0, 1.0])
        side /= np.linalg.norm(side) + 1e-9
        closer = dist < best
        best[closer] = dist[closer]
        seg_of[closer] = k
        along[closer] = t[closer]
        lateral[closer] = ((co - foot) @ side)[closer] / (r + 1e-6)
    on_finger = best < 0.75

    for k, (name, i, a, b, r) in enumerate(segments):
        m = on_finger & (seg_of == k)
        if not m.any():
            continue
        length = np.linalg.norm(b - a)
        s = along[m]
        joint = 0.12 if i == 0 else 0.0                     # Grundfurche liegt etwas jenseits des Knöchels
        c = _gauss(s - joint, 0.05) * palmar[m] ** 1.5
        offset[m] -= 0.042 * _fold(s - joint, 0.05) * palmar[m] ** 1.5
        crease[m] = np.maximum(crease[m], c)
        if i > 0:
            # feine Querfalten auf dem Rücken über dem Gelenk
            wrinkle = (0.5 + 0.5 * np.cos(2.0 * math.pi * s / 0.07)) * _gauss(s, 0.09) * dorsal[m] ** 2
            offset[m] -= 0.012 * wrinkle
            crease[m] = np.maximum(crease[m], 0.5 * wrinkle)
        if i == 0 and name != "Thumb":
            # Grübchen über dem Knöchel
            offset[m] -= 0.05 * _gauss(s + 0.05, 0.13) * _gauss(lateral[m], 0.5) * dorsal[m]
        if i == 2:
            rel = s / length
            # Beere: oben unter dem Nagel etwas flacher
            offset[m] -= 0.04 * np.clip((rel - 0.55) / 0.45, 0.0, 1.0) * dorsal[m]
            # Nagel: dünn, leicht erhaben, mit Wall am Rand und an der Wurzel
            in_nail = np.clip((rel - 0.38) / 0.06, 0.0, 1.0) * np.clip((0.72 - np.abs(lateral[m])) / 0.08, 0.0, 1.0) * np.clip((dorsal[m] - 0.35) / 0.2, 0.0, 1.0)
            # Nagelwall: flach und breit – ein schmaler, tiefer Graben sähe aus wie ein Schnitt
            near_nail = np.clip((rel - 0.3) / 0.08, 0.0, 1.0) * np.clip((dorsal[m] - 0.3) / 0.2, 0.0, 1.0)
            border = (_gauss(np.abs(lateral[m]) - 0.76, 0.09) + _gauss(rel - 0.37, 0.06)) * near_nail
            offset[m] += 0.010 * in_nail - 0.009 * border
            nail[m] = np.maximum(nail[m], in_nail)
            crease[m] = np.maximum(crease[m], 0.4 * border)

    # Handgelenk: Speckfalte als Ring, dorsal schwächer, davor und dahinter kleine Wülste
    x = co[:, 0]
    ring = _gauss(x + 0.25, 0.06) * (1.0 - 0.5 * dorsal)
    offset += -0.09 * ring + 0.03 * _gauss(x + 0.5, 0.15) + 0.02 * _gauss(x - 0.1, 0.12)
    crease = np.maximum(crease, ring)

    # Handteller: Fünffingerfurche (quer unter den Fingern) und Daumenfurche (im Bogen um den Daumenballen)
    palm = (~on_finger) & (x > -0.1) & (x < 3.1) & (palmar > 0.4)
    y = co[:, 1]
    distal_line = 2.35 + 0.18 * y - 0.12 * y * y                      # leicht geschwungen
    thenar_arc = np.hypot(x - 0.9, (y - 0.9) * 0.9) - 0.95             # Bogen um den Daumenballen
    for line in (x - distal_line, thenar_arc):
        c = _gauss(line, 0.05) * palmar ** 2 * palm
        offset -= 0.038 * _fold(line, 0.05) * palmar ** 2 * palm
        crease = np.maximum(crease, c)

    co += nrm * offset[:, None]
    for v, p in zip(mesh.vertices, co):
        v.co = p

    # UV-Kanal 1: eine einfache Projektion (Unreal braucht ihn); Kanal 2: Nagel und Falte für das Material
    base = mesh.uv_layers.new(name="UVMap")
    data = mesh.uv_layers.new(name="Daten")
    for poly in mesh.polygons:
        for li, vi in zip(poly.loop_indices, poly.vertices):
            base.data[li].uv = ((co[vi, 0] + 7.0) / 14.0, (co[vi, 1] + 2.0) / 4.0)
            data.data[li].uv = (float(nail[vi]), float(np.clip(crease[vi], 0.0, 1.0)))
    print("GENESIS: Anatomie – Nagelpunkte %d, Faltenpunkte %d" % (int((nail > 0.5).sum()), int((crease > 0.5).sum())))


def build_metaball_mesh(bone_chains):
    """
    Die Hand aus weich verschmelzenden Körpern: So entstehen der runde Handteller, die Hautfalten zwischen den Fingern
    und der volle Daumenballen einer Neugeborenenhand – eine Röhre je Knochen wirkt dagegen wie ein Handschuh.
    """
    data = bpy.data.metaballs.new(ASSET + "_Meta")
    # In cm bauen: Blender begrenzt die Auflösung von Metaballs nach unten – in m verschwänden die Finger
    data.resolution = 0.06
    data.render_resolution = 0.06
    data.threshold = 0.6
    meta = bpy.data.objects.new(ASSET + "_Meta", data)
    bpy.context.scene.collection.objects.link(meta)

    def ellipsoid(center, half, rotation=None):
        e = data.elements.new(type="ELLIPSOID")
        e.co = center
        e.radius = 1.0                                   # Ausdehnung = Radius × Größe
        e.size_x, e.size_y, e.size_z = (h * META_SCALE for h in half)
        e.stiffness = 2.0
        if rotation is not None:
            e.rotation = rotation

    def capsule(a, b, radius):
        e = data.elements.new(type="CAPSULE")
        e.co = (a + b) * 0.5
        e.radius = radius * META_SCALE
        e.size_x = (b - a).length * 0.5
        e.rotation = Vector((1.0, 0.0, 0.0)).rotation_difference((b - a).normalized())
        e.stiffness = 2.0

    # Unterarm (am Termin mit Fettpolster) und Handgelenk
    capsule(Vector((-6.5, 0.0, 0.0)), Vector((-3.0, 0.0, 0.0)), 0.95)
    capsule(Vector((-3.0, 0.0, 0.0)), Vector((-0.2, 0.0, 0.0)), 0.80)
    # Handteller: breit, dick, zum Rand hin gerundet; Daumen- und Kleinfingerballen
    ellipsoid(Vector((1.55, 0.0, 0.0)), (1.55, 1.25, 0.62))
    ellipsoid(Vector((0.85, 0.72, -0.28)), (0.95, 0.55, 0.50))
    ellipsoid(Vector((1.15, -0.85, -0.18)), (0.95, 0.40, 0.42))
    for name, *_rest in FINGERS:
        points, rads = bone_chains[name]
        for i in range(len(points) - 1):
            capsule(points[i], points[i + 1], rads[i + 1] * 1.05)
        # Fingerbeere: unten voll, oben flach unter dem Nagel – keine Kugel
        tip_dir = (points[-1] - points[-2]).normalized()
        ellipsoid(points[-1] - tip_dir * 0.08 + Vector((0.0, 0.0, -0.05)), (rads[-1] * 1.05, rads[-1] * 0.95, rads[-1] * 0.78),
                  Vector((1.0, 0.0, 0.0)).rotation_difference(tip_dir))
    points, rads = bone_chains["Thumb"]
    for i in range(len(points) - 1):
        capsule(points[i], points[i + 1], rads[i + 1] * 1.05)
    tip_dir = (points[-1] - points[-2]).normalized()
    ellipsoid(points[-1] - tip_dir * 0.08, (rads[-1] * 1.05, rads[-1] * 0.95, rads[-1] * 0.8),
              Vector((1.0, 0.0, 0.0)).rotation_difference(tip_dir))

    bpy.context.view_layer.objects.active = meta
    meta.select_set(True)
    bpy.ops.object.convert(target="MESH")
    obj = bpy.context.view_layer.objects.active
    obj.name = ASSET
    obj.data.name = ASSET + "_Mesh"
    # Metaball-Netze sind ungleichmäßig: glätten und auf eine saubere Dichte bringen
    remesh = obj.modifiers.new("Gleichmaessig", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = 0.025
    # Glätten: Babyfinger haben an den Gelenken Falten, keine Wülste
    smooth = obj.modifiers.new("Glatt", "SMOOTH")
    smooth.factor = 0.7
    smooth.iterations = 18
    for mod in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    anatomy(obj, bone_chains)
    soften = obj.modifiers.new("Weich", "SMOOTH")
    soften.factor = 0.35
    soften.iterations = 2
    decimate = obj.modifiers.new("Dichte", "DECIMATE")
    decimate.ratio = 0.45
    for mod in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    # Ein Materialplatz (der Metaball bringt einen eigenen mit – in Unreal bliebe dieser Abschnitt ohne Haut)
    obj.data.materials.clear()
    # cm → m
    for v in obj.data.vertices:
        v.co = v.co * CM
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


def skin_weights(hand, arm, sharpness=0.16):
    """
    Jeder Hautpunkt hängt an den nächstgelegenen Knochen: Gewicht nach Abstand zum Knochenstück, weich übergeblendet
    (an den Gelenken halb-halb), ohne Übersprechen zum Nachbarfinger (sharpness in cm).
    """
    co = np.array([v.co[:] for v in hand.data.vertices]) / CM
    names, dists = [], []
    for bone in arm.data.bones:
        a = np.array(bone.head_local[:]) / CM
        b = np.array(bone.tail_local[:]) / CM
        d = b - a
        t = np.clip(((co - a) @ d) / (d @ d), 0.0, 1.0)
        dists.append(np.linalg.norm(co - (a + t[:, None] * d), axis=1))
        names.append(bone.name)
    dists = np.array(dists)                                   # Knochen × Punkte
    nearest = dists.min(axis=0)
    weights = np.exp(-((dists - nearest) / sharpness) ** 2)
    weights[weights < 0.01] = 0.0
    weights /= weights.sum(axis=0, keepdims=True)
    for k, name in enumerate(names):
        group = hand.vertex_groups.get(name) or hand.vertex_groups.new(name=name)
        idx = np.nonzero(weights[k] > 0.0)[0]
        for i in idx:
            group.add([int(i)], float(weights[k, i]), "REPLACE")
    print("GENESIS: Gewichte je Knochen", {n: int((weights[k] > 0.5).sum()) for k, n in enumerate(names)})


def build():
    clean_scene()
    bone_chains = chains()
    hand = build_metaball_mesh(bone_chains)
    arm = build_armature(bone_chains)
    bpy.ops.object.select_all(action="DESELECT")
    hand.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    # Gewichte selbst rechnen: Die automatische Gewichtung (Bone Heat) scheitert bei so kleinen Objekten still –
    # dann hängt kein Punkt an einem Knochen und die Hand bleibt starr wie aus einem Stück.
    bpy.ops.object.parent_set(type="ARMATURE_NAME")
    skin_weights(hand, arm)
    print("GENESIS: Hand gebaut", len(hand.data.polygons), "Flächen,", len(arm.data.bones), "Knochen")
    return hand, arm


def export_all(out_dir=OUT_DIR):
    hand, arm = build()
    os.makedirs(out_dir, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    hand.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    path = os.path.join(out_dir, ASSET + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             object_types={"ARMATURE", "MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE",
                             add_leaf_bones=False, use_armature_deform_only=True, bake_anim=False)
    print("GENESIS: exportiert", ASSET, round(os.path.getsize(path) / 1e6, 2), "MB")
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out_dir, "GEN_FetalHand.blend"))


if __name__ == "__main__":
    export_all()
