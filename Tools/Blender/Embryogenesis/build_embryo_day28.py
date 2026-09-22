# GENESIS: Der Kreislauf des Lebens
#
# Der Embryo am Ende der vierten Woche (Tag 28, Carnegie 12–13), modelliert nach Referenzen:
#   * Präparat 4–5 Wochen (Wikimedia Commons, CC BY 2.0): Umriss, Lage von Herz, Kiemenbögen,
#     Knospen und Schwanz – als Landmarken in Bildpunkten abgelesen (Raster 50 px), siehe LANDMARKS.
#   * Blechschmidt-Rekonstruktionsmodelle 2,5 / 3,4 / 4,2 / 6,3 mm: Querschnitt und Tiefe.
#   * Lehrbuch: größte Länge 4,6 mm an Tag 28, ~30 Somitenpaare, Armknospe als Leiste, Beinknospe
#     gerade angelegt. Das Präparat ist etwas älter (Carnegie 14): Knospen hier kleiner als dort.
# Die Referenzbilder liegen nur lokal (ArtSource/Reference/Embryo, siehe QUELLEN.md).
#
# Aufbau: weich verschmelzende Grundformen (Metaballs) entlang der gemessenen Achse, danach Netz,
# gleichmäßig neu vernetzt, und die feinen Formen (Somitenstreifen, Kiemenfurchen) als Verschiebung.
#
# Maßstab des Projekts: 1 µm = 0,01 m in Blender (1 Unreal-Einheit).
#
# Im laufenden Blender:  exec(open(r"...\build_embryo_day28.py", encoding="utf-8").read())

import math
import bpy
import numpy as np
from mathutils import Vector, Matrix, Quaternion

UM = 0.01
SCENE = "GEN_Embryo_Tag28"
LENGTH_UM = 4600.0
SOMITES = 30              # Somitenpaare am Ende der vierten Woche (Simulation: 28–30)

# Bildpunkte im Ausschnitt crop_praeparat_seite.png (750 x 760, Ursprung oben links).
# 685 px größte Länge = 4600 µm  ->  6,7 µm je Bildpunkt. Bildmitte (375 | 380) = Ursprung.
PX_UM = 6.7
CENTER_PX = (375.0, 380.0)


def px(x, y):
    """Bildpunkt -> (x, z) in µm; Bild-y zeigt nach unten, z nach oben."""
    return np.array([(x - CENTER_PX[0]) * PX_UM, -(y - CENTER_PX[1]) * PX_UM])


# Der Rumpf in Querschnitten, vom Nacken bis zur Schwanzspitze: je Station ein Punkt auf dem Rücken
# und einer auf der Bauchseite (Bildpunkte, im Raster abgelesen) und die halbe Breite (seitlich).
# Wichtig: Das "C" ist innen nicht hohl. Zwischen Rücken und Bauch liegen Schlund, Herz und Leber –
# am Nacken ist der Körper fast 2 mm tief. Ein erster Versuch mit einer Röhre um das Neuralrohr sah
# deshalb aus wie ein Wurm.
STATIONS = [
    # Rücken (x, y)   Bauch (x, y)   halbe Breite
    ((708, 262), (505, 300), 112),   # Nacken
    ((716, 360), (470, 400), 114),   # die Körperwand umschließt Vorhof und Leber
    ((705, 450), (450, 480), 114),
    ((680, 540), (470, 530), 108),   # Armknospe
    ((630, 622), (460, 560), 100),
    ((540, 690), (405, 582), 94),    # Lende
    ((420, 724), (335, 592), 86),
    ((300, 724), (272, 590), 80),    # Kreuzbein
    ((180, 695), (232, 572), 72),
    ((82, 630), (210, 545), 64),     # Beinknospe
    ((48, 530), (172, 520), 52),     # Schwanzansatz
    ((82, 442), (160, 482), 38),     # Schwanz rollt sich ein
    ((150, 410), (182, 458), 27),
    ((222, 412), (234, 446), 18),    # Schwanzspitze: stumpf, kein Horn
]

# Die Achse (für die Somitenstreifen später): Mitte des Neuralrohrs, knapp unter dem Rücken
AXIS = [
    # x,   y,   halbe Tiefe
    (300, 205, 105),   # Vorderhirn
    (360, 140, 100),
    (440, 115, 95),    # Mittelhirn
    (530, 135, 95),
    (600, 190, 90),    # Rautenhirn
    (640, 270, 82),    # Nacken
    (645, 360, 82),
    (630, 450, 84),    # Brust (Armknospe)
    (590, 540, 84),
    (520, 615, 82),    # Lende
    (420, 665, 80),
    (310, 675, 76),    # Kreuzbein
    (200, 640, 68),
    (120, 575, 55),    # Beinknospe, Schwanzansatz
    (95, 500, 42),     # Schwanz rollt sich ein
    (135, 445, 30),
    (205, 425, 20),
    (255, 430, 12),    # Schwanzspitze
]


def catmull(points, samples):
    """Glatte Kurve durch die Punkte (Catmull-Rom), gleichmäßig nach Bogenlänge abgetastet."""
    P = np.array(points, dtype=float)
    dense = []
    for i in range(len(P) - 1):
        p0 = P[max(i - 1, 0)]
        p1, p2 = P[i], P[i + 1]
        p3 = P[min(i + 2, len(P) - 1)]
        for s in np.linspace(0.0, 1.0, 40, endpoint=False):
            s2, s3 = s * s, s * s * s
            dense.append(0.5 * ((2 * p1) + (-p0 + p2) * s + (2 * p0 - 5 * p1 + 4 * p2 - p3) * s2 + (-p0 + 3 * p1 - 3 * p2 + p3) * s3))
    dense.append(P[-1])
    dense = np.array(dense)
    seg = np.linalg.norm(np.diff(dense[:, :2], axis=0), axis=1)
    arc = np.concatenate([[0.0], np.cumsum(seg)])
    target = np.linspace(0.0, arc[-1], samples)
    return np.stack([np.interp(target, arc, dense[:, k]) for k in range(dense.shape[1])], axis=1), arc[-1]


def v3(p2, y=0.0):
    return Vector((p2[0] * UM, y * UM, p2[1] * UM))


def add_ellipsoid(mb, center, axes_um, angle=0.0, stiffness=2.0, negative=False):
    """Ellipsoid; axes_um = (in Bildrichtung x, seitlich y, in Bildrichtung z); angle dreht in der Bildebene."""
    e = mb.elements.new(type="ELLIPSOID")
    e.co = center
    e.radius = 1.0
    e.size_x, e.size_y, e.size_z = axes_um[0] * UM, axes_um[1] * UM, axes_um[2] * UM
    e.stiffness = stiffness
    e.use_negative = negative
    e.rotation = Matrix.Rotation(-angle, 4, "Y").to_quaternion()
    return e


# Ein einzelnes Metaball-Ellipsoid ist sichtbar nur ~0,58 so groß wie eingestellt (Steifigkeit 2,
# Schwelle 0,6 – gemessen). In dichten Ketten addieren sich die Felder; dafür gilt CHAIN_GAIN,
# ebenfalls am Netz abgeglichen (measure_stations).
SINGLE_GAIN = 0.575
CHAIN_GAIN = 0.80


# Der Kopf: drei Hirnbläschen als Kuppen plus Boden über den Kiemenbögen. Dieselbe Liste baut die
# Körperhülle und – leicht eingerückt – die Hirnwand darin, damit beide dieselbe Form haben.
HEAD_BLOBS = [
    # x,   y,   Halbachsen (Bild-x, seitlich, Bild-z), Winkel, Steifigkeit
    (318, 190, (92, 96, 112), 0.0, 2.0),     # Vorderhirn, wölbt sich nach vorn-unten
    (440, 128, (118, 100, 88), 0.0, 2.0),    # Mittelhirn: die Kuppe, höchster Punkt
    (575, 180, (88, 92, 80), -0.5, 1.6),     # Rautenhirn, fällt zum Nacken ab
    (455, 225, (120, 92, 60), 0.0, 1.5),     # Boden des Kopfes über den Kiemenbögen
]


def blob(mb, x, y, semi_px, side=0.0, angle=0.0, stiffness=2.0, negative=False):
    """Eine Grundform mit sichtbaren Halbachsen semi_px = (Bild-x, seitlich, Bild-z) in Bildpunkten."""
    s = [v * PX_UM / SINGLE_GAIN for v in semi_px]
    return add_ellipsoid(mb, v3(px(x, y), side * PX_UM), s, angle, stiffness, negative)


def build_metaball():
    mb = bpy.data.metaballs.new("MB_Embryo")
    mb.resolution = 0.8
    mb.render_resolution = 0.3
    mb.threshold = 0.6

    # 1) Rumpf: Querschnitte zwischen Rücken- und Bauchlinie, dicht gesetzt
    dors, _ = catmull([s[0] for s in STATIONS], 110)
    vent, _ = catmull([s[1] for s in STATIONS], 110)
    widths = np.interp(np.linspace(0, 1, 110), np.linspace(0, 1, len(STATIONS)), [s[2] for s in STATIONS])
    for i in range(110):
        d, v = dors[i], vent[i]
        mid = 0.5 * (d + v)
        across = d - v
        half_depth = 0.5 * np.linalg.norm(across)
        # Tiefenachse zeigt vom Bauch zum Rücken; die "entlang"-Achse steht senkrecht darauf
        angle = math.atan2(-across[1], across[0]) - math.pi / 2
        along = max(half_depth * 0.45, 12.0)
        s = [along * PX_UM / CHAIN_GAIN, widths[i] * PX_UM / CHAIN_GAIN, half_depth * PX_UM / CHAIN_GAIN]
        add_ellipsoid(mb, v3(px(*mid)), s, angle, stiffness=1.6)

    # 1b) Rückenwülste: Auf Höhe der Somiten ist der Rumpf am breitesten (die Somiten bilden die
    #     Wülste beiderseits des Neuralrohrs). Eine Ellipse um die Körpermitte lief zum Rücken hin
    #     spitz zu – in der Rückenansicht standen die Somiten dann als Perlenreihen heraus.
    pts, backs, depths = somite_axis(140)
    halfw = body_half_width(140)
    for i in range(0, 132):
        p, back, depth, w = pts[i], backs[i], depths[i], halfw[i]
        tangent = pts[min(i + 1, 139)] - pts[max(i - 1, 0)]
        angle = math.atan2(tangent[1], tangent[0])
        seg = np.linalg.norm(tangent) * 0.5
        # am Nacken weich einblenden – ein harter Beginn zeichnete eine waagrechte Kante
        ramp = 0.45 + 0.55 * float(np.clip(i / 14.0, 0.0, 1.0)) ** 0.7
        e = add_ellipsoid(mb, v3(p - back * depth * 0.02), (max(seg * 1.2, 20.0) / CHAIN_GAIN,
                                                            w * 0.92 * ramp / CHAIN_GAIN,
                                                            depth * 0.24 / CHAIN_GAIN), 0.0, stiffness=1.6)
        e.rotation = Matrix.Rotation(-angle, 4, "Y").to_quaternion()

    # 2) Kopf: drei Hirnbläschen als Kuppen, keine Füllmasse – sonst wird er ein Kasten
    for x, y, semi, angle, stiff in HEAD_BLOBS:
        blob(mb, x, y, semi, 0.0, angle, stiffness=stiff)
    # Übergang Rautenhirn -> Nacken ohne Kerbe: sonst überbrückt das Neuralrohr sie außerhalb der Haut
    blob(mb, 655, 228, (62, 88, 66), 0.0, -0.9, stiffness=1.6)
    # Augenbläschen: flache Kuppen, die aus dem Vorderhirn herauswachsen – keine aufgesetzten Kugeln
    for side in (-1.0, 1.0):
        blob(mb, 292, 215, (44, 18, 48), side * 90, stiffness=1.7)

    # 3) Kiemenbögen: Leisten seitlich am Hals, quer zur Körperachse (wie Kiemenbögen eben), vorn
    #    groß, nach hinten kleiner; dazwischen bleiben die Furchen stehen
    #    Flach an der Seite des Halses: eine erste Fassung hing wie Zähne unter dem Kopf.
    #    Darunter der Schlundboden, der Kopf, Herzwölbung und Nacken verbindet – ohne ihn blieb in der
    #    Seitenansicht ein Loch, durch das der Hintergrund schien.
    blob(mb, 470, 298, (78, 70, 48), stiffness=1.6)
    blob(mb, 520, 318, (50, 66, 50), stiffness=1.6)
    for (cx, cy), long_px, short_px, ang in (((380, 272), 46, 18, -1.05),
                                             ((452, 285), 38, 15, -1.20),
                                             ((508, 292), 30, 12, -1.35),
                                             ((552, 296), 22, 10, -1.45)):
        for side in (-1.0, 1.0):
            blob(mb, cx, cy, (long_px, 18, short_px), side * 82, ang, stiffness=1.8)

    # 4) Herzwölbung: eine eigene, runde Kugel (höhere Steifigkeit = klarere Kontur),
    #    dahinter kleiner der Vorhof, darunter der Leberwulst
    blob(mb, 368, 374, (68, 60, 66), stiffness=3.2)
    blob(mb, 468, 385, (40, 46, 44), stiffness=1.4)
    blob(mb, 452, 475, (44, 50, 42), stiffness=1.4)

    # 5) Knospen: Arm als flache Leiste (Tag 28 kleiner als im Präparat), Bein gerade angelegt.
    #    Niedrig und breit, damit sie aus der Flanke wachsen statt wie Erbsen daraufzusitzen.
    for side in (-1.0, 1.0):
        blob(mb, 565, 500, (62, 10, 24), side * 110, -0.7, stiffness=1.6)
        blob(mb, 160, 555, (34, 11, 26), side * 68, 0.9, stiffness=1.7)

    # Der Stiel zum Dottersack (Nabelansatz) ist ein eigenes Objekt der Szene: Er läuft in der
    # Körperebene durch die Öffnung des "C" nach vorn-oben zum Dottersack.
    return mb


# ------------------------------------------------------------------------------------------------
# Netz und Feinformen
# ------------------------------------------------------------------------------------------------

VOXEL_UM = 9.0            # Auflösung des fertigen Netzes: 9 µm – Somitenfurchen bekommen 2–3 Zellen
SOMITE_REGION = (0.02, 0.86)   # entlang der Rückenlinie: vom Hinterhaupt bis zum Schwanzansatz


def somite_axis(samples=600):
    """Mitte des Neuralrohrs: knapp unter der Rückenlinie. Liefert Punkte (µm), Rückenrichtung, Tiefe."""
    dors, _ = catmull([s[0] for s in STATIONS], samples)
    vent, _ = catmull([s[1] for s in STATIONS], samples)
    pts, backs, depths = [], [], []
    for d, v in zip(dors, vent):
        across = px(*d) - px(*v)
        depth = np.linalg.norm(across)
        back = across / (depth + 1e-9)
        pts.append(px(*d) - back * depth * 0.16)
        backs.append(back)
        depths.append(depth)
    return np.array(pts), np.array(backs), np.array(depths)


def body_half_width(samples):
    """Halbe Körperbreite (µm) entlang derselben Abtastung wie somite_axis."""
    return np.interp(np.linspace(0, 1, samples), np.linspace(0, 1, len(STATIONS)), [s[2] for s in STATIONS]) * PX_UM


def to_mesh(metaball_obj):
    """Metaball in feiner Auflösung auswerten, als Netz übernehmen und gleichmäßig neu vernetzen."""
    mb = metaball_obj.data
    mb.resolution = mb.render_resolution
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(metaball_obj.evaluated_get(dg))
    mesh.name = "SM_GEN_Embryo_Tag28"
    obj = bpy.data.objects.new("Embryo_Tag28", mesh)
    for coll in metaball_obj.users_collection:
        coll.objects.link(obj)
    metaball_obj.hide_viewport = True
    metaball_obj.hide_render = True
    mb.resolution = 0.8

    remesh = obj.modifiers.new("Gleichmaessig", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = VOXEL_UM * UM
    remesh.adaptivity = 0.0
    remesh.use_smooth_shade = True
    smooth = obj.modifiers.new("Glaetten", "SMOOTH")
    smooth.factor = 0.5
    smooth.iterations = 6
    dg = bpy.context.evaluated_depsgraph_get()
    baked = bpy.data.meshes.new_from_object(obj.evaluated_get(dg))
    obj.modifiers.clear()
    old = obj.data
    obj.data = baked
    baked.name = "SM_GEN_Embryo_Tag28"
    bpy.data.meshes.remove(old)
    for poly in baked.polygons:
        poly.use_smooth = True
    return obj


def surface_detail(obj):
    """Somitenstreifen, Mittellinie des Neuralrohrs, Ohr- und Linsengrübchen – als Verschiebung entlang der Normale."""
    mesh = obj.data
    n = len(mesh.vertices)
    co = np.empty(n * 3, dtype=np.float64)
    mesh.vertices.foreach_get("co", co)
    co = co.reshape(n, 3) / UM                      # in µm
    nor = np.empty(n * 3, dtype=np.float64)
    mesh.vertices.foreach_get("normal", nor)
    nor = nor.reshape(n, 3)

    pts, backs, depths = somite_axis()
    m = len(pts)

    # Nächster Achsenpunkt je Netzpunkt – blockweise, damit der Speicher reicht
    xz = co[:, [0, 2]]
    k = np.empty(n, dtype=np.int64)
    for start in range(0, n, 8000):
        block = xz[start:start + 8000]
        d2 = ((block[:, None, :] - pts[None, :, :]) ** 2).sum(axis=2)
        k[start:start + 8000] = d2.argmin(axis=1)

    # Somitenzählung entlang der Achse: Abstand wächst mit der Körpertiefe (vorn groß, hinten klein)
    seg = np.linalg.norm(np.diff(pts, axis=0), axis=1)
    local = np.concatenate([[0.0], seg / (depths[1:] + 1e-6)])
    u_all = np.cumsum(local)
    s_all = np.linspace(0.0, 1.0, m)
    lo, hi = SOMITE_REGION
    u_lo, u_hi = np.interp(lo, s_all, u_all), np.interp(hi, s_all, u_all)

    r_plane = xz - pts[k]
    dorsal = (r_plane * backs[k]).sum(axis=1)
    theta = np.degrees(np.arctan2(np.abs(co[:, 1]), dorsal))       # 0 = Rückenmitte, 90 = Flanke
    s = s_all[k]
    depth = depths[k]

    # Nur die Rumpfoberfläche: Punkte, deren nächster Achsenpunkt der erste ist, gehören zum Kopf
    trunk = np.clip((s - 0.004) / 0.03, 0.0, 1.0)

    # Mittellinie: Neuralrohr kaum erhaben, daneben eine sehr flache Rinne. Eine erste Fassung mit
    # dreifacher Tiefe stand als Grat heraus – im Präparat bleibt der Rückenumriss glatt.
    disp = trunk * 0.006 * depth * np.exp(-(theta / 12.0) ** 2)
    disp -= trunk * 0.004 * depth * np.exp(-((theta - 26.0) / 9.0) ** 2)

    # Somiten: weiche Querstreifen auf der Flanke neben dem Neuralrohr – flache Blöcke, getrennt von
    # breiten, seichten Furchen. Im Präparat lesen sie sich als Schattenlinien unter der Haut, nicht als Kamm.
    fade = np.clip(np.minimum((s - lo + 0.02) / 0.05, (hi + 0.02 - s) / 0.05), 0.0, 1.0)
    band = np.exp(-((theta - 58.0) / 28.0) ** 2) * fade
    u = (u_all[k] - u_lo) / (u_hi - u_lo) * SOMITES
    frac = u - np.floor(u)
    groove = np.exp(-((frac - 0.5) / 0.14) ** 2)
    block = np.sin(np.pi * frac) ** 0.8
    disp += band * depth * (0.004 * block - 0.010 * groove)

    # Grübchen: Ohrplakode über dem 2. Kiemenbogen, Linsenplakode auf dem Augenbläschen
    for (cx, cy), side_px, radius_px, depth_px in (((560, 245), 96, 22, 7), ((292, 215), 108, 20, 5)):
        c = px(cx, cy)
        for side in (-1.0, 1.0):
            centre = np.array([c[0], side * side_px * PX_UM, c[1]])
            dist = np.linalg.norm(co - centre, axis=1)
            disp -= depth_px * PX_UM * np.exp(-(dist / (radius_px * PX_UM)) ** 2)

    new = (co + nor * disp[:, None]) * UM
    mesh.vertices.foreach_set("co", new.ravel())
    mesh.update()
    return float(disp.min()), float(disp.max())


# ------------------------------------------------------------------------------------------------
# Innere Organe: Mit vier Wochen ist der Embryo fast gallertartig durchsichtig. Was man sieht, sind
# die dichteren Strukturen darunter – deshalb existieren sie als eigene Körper, nicht als Farbe:
#   Herzschlauch mit Blut (S-Schleife), Aortenbögen und Rückenaorten, Hirnwand um die Ventrikel,
#   Neuralrohr mit Lumen, 30 Somitenpaare, Leberanlage.
# ------------------------------------------------------------------------------------------------

def tube(mb, points_px, radii_px, side_px=0.0, negative=False, samples=40, stiffness=1.8):
    """Ein Schlauch aus dicht gesetzten Kugeln entlang einer Kurve durch Bildpunkte."""
    curve, _ = catmull([(p[0], p[1], r) for p, r in zip(points_px, radii_px)], samples)
    for x, y, r in curve:
        e = mb.elements.new(type="BALL")
        e.co = v3(px(x, y), side_px * PX_UM)
        e.radius = r * PX_UM * UM / SINGLE_GAIN * 1.0
        e.stiffness = stiffness
        e.use_negative = negative


def organ(coll, name, build_fn, voxel_um, material):
    mb = bpy.data.metaballs.new("MB_" + name)
    mb.resolution = 0.6
    mb.render_resolution = voxel_um * UM * 2.0
    mb.threshold = 0.6
    build_fn(mb)
    mobj = bpy.data.objects.new(name + "_Grundform", mb)
    coll.objects.link(mobj)
    mb.resolution = mb.render_resolution
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(mobj.evaluated_get(dg))
    mesh.name = "SM_" + name
    obj = bpy.data.objects.new(name, mesh)
    coll.objects.link(obj)
    bpy.data.objects.remove(mobj, do_unlink=True)
    bpy.data.metaballs.remove(mb)
    smooth = obj.modifiers.new("Glaetten", "SMOOTH")
    smooth.factor = 0.5
    smooth.iterations = 4
    for poly in mesh.polygons:
        poly.use_smooth = True
    obj.data.materials.append(material)
    return obj


def simple_material(name, base, sss_radius, sss_scale_um, roughness=0.45, ior=1.04):
    """Organgewebe unter der Haut: diffus-streuend, kaum spiegelnd (im Fruchtwasser)."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = base
    bsdf.inputs["Subsurface Weight"].default_value = 1.0
    bsdf.inputs["Subsurface Radius"].default_value = sss_radius
    bsdf.inputs["Subsurface Scale"].default_value = sss_scale_um * UM
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["IOR"].default_value = ior
    return mat


def inner_organs(coll):
    # Blut: dunkles Karmin – gesehen durch Herzbeutel und Körperwand, nicht leuchtend rot
    blood = simple_material("M_GEN_Embryo_Blut", (0.20, 0.022, 0.020, 1.0), (1.0, 0.22, 0.18), 40.0)
    # Neuralgewebe lebend: milchig-glasig, nicht kreideweiß (weiß wird es erst im fixierten Präparat)
    # Lebend eher glasig-grau als weiß: in der Rückenansicht lag es sonst als weißer Stab obenauf
    neural = simple_material("M_GEN_Embryo_Neuralgewebe", (0.60, 0.53, 0.53, 1.0), (1.0, 0.85, 0.8), 220.0)
    # Somiten: nur etwas dichter als das Gewebe ringsum – sie sollen durchschimmern, nicht leuchten
    somite = simple_material("M_GEN_Embryo_Somiten", (0.86, 0.72, 0.70, 1.0), (1.0, 0.75, 0.65), 90.0)
    liver = simple_material("M_GEN_Embryo_Leberanlage", (0.40, 0.12, 0.10, 1.0), (1.0, 0.4, 0.3), 60.0)

    # Herzschlauch: vom Venensinus über Vorhof und Kammer (Schleife nach vorn-unten) zum Bulbus und
    # Truncus, der zum Aortensack unter den Kiemenbögen aufsteigt
    def heart(mb):
        # Der Schlauch füllt den Herzbeutel nicht aus – ringsum liegt Flüssigkeit
        pts = [(505, 410), (480, 382), (445, 372), (400, 400), (358, 392), (345, 356), (372, 326), (405, 305), (428, 290)]
        rad = [17, 21, 18, 26, 26, 21, 15, 11, 8]
        tube(mb, pts, rad, samples=70)

    # Aortenbögen: je Kiemenbogen ein Gefäß vom Aortensack um den Schlund zur Rückenaorta
    def vessels(mb):
        for side in (-1.0, 1.0):
            for (ax, ay), r in (((378, 282), 7), ((452, 296), 7), ((510, 305), 6)):
                tube(mb, [(428, 288), (ax, ay + 6), ((ax + 560) / 2, 250), (590, 240)], [9, r, r, 8], side * 40, samples=24)
            # Rückenaorta: unter den Somiten nach hinten, ab der Brust vereint
            pts, backs, depths = somite_axis(40)
            path, rad = [], []
            for i in range(2, 34):
                p = pts[i] - backs[i] * depths[i] * 0.34
                path.append(((p[0] / PX_UM) + CENTER_PX[0], -(p[1] / PX_UM) + CENTER_PX[1]))
                rad.append(max(4.0, 8.0 - i * 0.12))
            tube(mb, path, rad, side * max(0.0, 30.0), samples=90)

    # Hirnwand: dickwandige Schalen um die Ventrikel (negativ = Hohlraum mit Flüssigkeit)
    def brain(mb):
        # Der Kopf ist mit vier Wochen fast nur Gehirn: Die Wand ist dieselbe Form wie der Kopf, nur
        # eingerückt – drei einzelne Kugeln leuchteten als helle Flecken durch die Haut.
        # 95 %: Die Haut liegt direkt auf dem Gehirn. Mit 90 % blieb in Unreal ein Band mit hartem Rand
        for x, y, semi, angle, stiff in HEAD_BLOBS[:3]:
            blob(mb, x, y, tuple(v * 0.95 for v in semi), 0.0, angle, stiffness=stiff)
            blob(mb, x, y, tuple(v * 0.55 for v in semi), 0.0, angle, stiffness=2.4, negative=True)
        # Augenbläschen: Ausstülpungen der Hirnwand
        for side in (-1.0, 1.0):
            blob(mb, 292, 215, (38, 15, 42), side * 84, stiffness=1.7)
        # Neuralrohr mit Lumen entlang des Rückens – es beginnt im Rautenhirn, nicht am Nacken: Hirn
        # und Rückenmark sind ein durchgehendes Rohr (sonst endet es mit einer hellen Kappe)
        pts, backs, depths = somite_axis(60)
        path, rad = [(560, 185), (600, 215)], [depths[0] / PX_UM * 0.11, depths[0] / PX_UM * 0.11]
        for i in range(0, 52):
            # etwas tiefer als die Somitenachse: am Nacken trat es sonst durch die Haut
            q = pts[i] - backs[i] * depths[i] * 0.06
            path.append(((q[0] / PX_UM) + CENTER_PX[0], -(q[1] / PX_UM) + CENTER_PX[1]))
            rad.append(depths[i] / PX_UM * (0.10 - 0.03 * max(0.0, (i - 30) / 22.0)))
        tube(mb, path, rad, samples=160)
        tube(mb, path, [r * 0.38 for r in rad], negative=True, samples=160, stiffness=2.4)

    # Somiten: 30 Paare neben dem Neuralrohr, nach hinten kleiner und jünger
    def somites(mb):
        pts, backs, depths = somite_axis(900)
        halfw = body_half_width(900)
        seg = np.linalg.norm(np.diff(pts, axis=0), axis=1)
        u = np.cumsum(np.concatenate([[0.0], seg / (depths[1:] + 1e-6)]))
        s_all = np.linspace(0.0, 1.0, len(pts))
        lo, hi = SOMITE_REGION
        u_lo, u_hi = np.interp(lo, s_all, u), np.interp(hi, s_all, u)
        for j in range(SOMITES):
            target = u_lo + (j + 0.5) / SOMITES * (u_hi - u_lo)
            i = int(np.searchsorted(u, target))
            i = min(i, len(pts) - 2)
            p, back, depth = pts[i], backs[i], depths[i]
            spacing = (u_hi - u_lo) / SOMITES * depth
            tangent = pts[i + 1] - pts[i]
            angle = math.atan2(tangent[1], tangent[0])
            # Neben dem Neuralrohr, etwas tiefer als seine Mitte – eine erste Fassung lag zu weit außen
            # und stand als Perlenkette aus dem Rücken
            # Seitlich nach der Körperbreite, nicht nach der Tiefe: Am Schwanz ist der Körper schmal,
            # dort standen sie sonst heraus
            centre = p - back * depth * 0.07
            w = halfw[i]
            # Nach hinten jünger und kleiner – und der Schwanz ist schmal: dort schrumpfen sie mit,
            # sonst schimmern sie als Perlen durch die Haut
            tail = float(np.clip((s_all[i] - 0.55) / 0.30, 0.0, 1.0))
            shrink = 1.0 - 0.45 * tail
            for side in (-1.0, 1.0):
                e = mb.elements.new(type="ELLIPSOID")
                e.co = v3(centre, side * w * (0.36 - 0.08 * tail))
                e.radius = 1.0
                e.size_x = spacing * 0.36 * UM / SINGLE_GAIN
                e.size_y = w * 0.17 * shrink * UM / SINGLE_GAIN
                e.size_z = min(depth * 0.090, w * 0.24) * shrink * UM / SINGLE_GAIN
                e.stiffness = 2.0
                e.rotation = Matrix.Rotation(-angle, 4, "Y").to_quaternion()

    def liver_bud(mb):
        blob(mb, 458, 470, (34, 36, 30), stiffness=2.0)

    made = [
        organ(coll, "Embryo_Tag28_Herz", heart, 5.0, blood),
        organ(coll, "Embryo_Tag28_Gefaesse", vessels, 3.0, blood),
        organ(coll, "Embryo_Tag28_Neuralrohr", brain, 6.0, neural),
        organ(coll, "Embryo_Tag28_Somiten", somites, 4.0, somite),
        organ(coll, "Embryo_Tag28_Leberanlage", liver_bud, 6.0, liver),
    ]
    return made


# ------------------------------------------------------------------------------------------------
# Gewebe: was unter der Haut liegt, bestimmt die Farbe. Mit vier Wochen ist die Haut eine einzige
# Zellschicht – man sieht hindurch: das Herz rot (Blut fließt seit Tag 22), die Hirnbläschen glasig
# (mit Flüssigkeit gefüllt), Somiten und Leber dichter. Als Farbattribut "Gewebe":
#   R = Blut, G = Flüssigkeit (durchscheinend), B = Dichte
# ------------------------------------------------------------------------------------------------

def tissue_masks(obj):
    mesh = obj.data
    n = len(mesh.vertices)
    co = np.empty(n * 3)
    mesh.vertices.foreach_get("co", co)
    co = co.reshape(n, 3) / UM

    def near(cx, cy, radius_px, side_px=0.0):
        c = px(cx, cy)
        centre = np.array([c[0], side_px * PX_UM, c[1]])
        d = np.linalg.norm(co - centre, axis=1) / (radius_px * PX_UM)
        return np.clip(1.25 - d, 0.0, 1.0)

    blood = np.maximum.reduce([
        near(368, 374, 70) ** 0.7,                   # Herz: das Blut schimmert durch die Wand
        0.75 * near(470, 385, 52),                   # Vorhof
        0.55 * near(452, 475, 58),                   # Leber: dunkler, bräunlich
        0.35 * near(420, 300, 70),                   # Aortenbögen in den Kiemenbögen
    ])
    fluid = np.maximum.reduce([
        near(318, 190, 110), near(440, 128, 120), near(575, 180, 100),   # Hirnbläschen
    ]) * 0.9
    fluid = np.where(co[:, 2] > px(0, 250)[1], fluid, fluid * 0.4)
    dense = np.clip(0.35 + 0.4 * near(560, 500, 60, 112) + 0.4 * near(160, 555, 40, 68), 0.0, 1.0)

    attr = mesh.color_attributes.get("Gewebe") or mesh.color_attributes.new("Gewebe", "FLOAT_COLOR", "POINT")
    rgba = np.stack([blood, fluid, dense, np.ones(n)], axis=1).astype(np.float32)
    attr.data.foreach_set("color", rgba.ravel())
    mesh.color_attributes.active_color = attr
    return float(blood.max()), float(fluid.max())


def make_material(obj):
    """Lebendes Gewebe: stark durchscheinend, blass-rosa, wo Blut liegt rot, wo Flüssigkeit liegt glasig."""
    mat = bpy.data.materials.get("M_GEN_Embryo_Gewebe") or bpy.data.materials.new("M_GEN_Embryo_Gewebe")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial"); out.location = (900, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled"); bsdf.location = (560, 0)
    attr = nt.nodes.new("ShaderNodeVertexColor"); attr.location = (-700, 0)
    attr.layer_name = "Gewebe"
    sep = nt.nodes.new("ShaderNodeSeparateColor"); sep.location = (-480, 0)
    nt.links.new(attr.outputs["Color"], sep.inputs["Color"])

    # Grundfarbe: blasses, leicht warmes Gewebe; mit Blut tiefrot, mit Flüssigkeit heller
    tissue = nt.nodes.new("ShaderNodeRGB"); tissue.location = (-480, 260)
    tissue.outputs[0].default_value = (0.78, 0.60, 0.55, 1.0)
    red = nt.nodes.new("ShaderNodeRGB"); red.location = (-480, 420)
    red.outputs[0].default_value = (0.42, 0.045, 0.035, 1.0)
    mix_blood = nt.nodes.new("ShaderNodeMix"); mix_blood.location = (-220, 300)
    mix_blood.data_type = "RGBA"
    nt.links.new(sep.outputs["Red"], mix_blood.inputs["Factor"])
    nt.links.new(tissue.outputs[0], mix_blood.inputs["A"])
    nt.links.new(red.outputs[0], mix_blood.inputs["B"])

    # Feinste Ebene: Ektodermzellen (~10 µm) als kaum sichtbares Relief
    cells = nt.nodes.new("ShaderNodeTexVoronoi"); cells.location = (-220, -320)
    cells.feature = "DISTANCE_TO_EDGE"
    cells.inputs["Scale"].default_value = 1.0 / (10.0 * UM)
    bump = nt.nodes.new("ShaderNodeBump"); bump.location = (200, -320)
    bump.inputs["Strength"].default_value = 0.08
    bump.inputs["Distance"].default_value = 1.5 * UM
    nt.links.new(cells.outputs["Distance"], bump.inputs["Height"])

    nt.links.new(mix_blood.outputs["Result"], bsdf.inputs["Base Color"])
    # Die Hülle ist eine einzelne Zellschicht: überwiegend durchlässig. Im Fruchtwasser ist der
    # Brechzahlsprung winzig (Gewebe 1,38 / Fruchtwasser 1,335 = 1,03) – daher kaum Glanz. Eine erste
    # Fassung rechnete gegen Luft (Brechzahl 1,38, nasse Deckschicht) und sah aus wie Plastik.
    bsdf.inputs["Transmission Weight"].default_value = 0.9
    bsdf.inputs["Subsurface Weight"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.32
    bsdf.inputs["IOR"].default_value = 1.035
    nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])

    # Darunter das Mesenchym: leicht milchig, schwach rosa. Streuung 0,06/m = mittlere freie
    # Weglänge 1,7 mm – der Körper (1–2 mm tief) ist dunstig durchscheinend, die Organe bleiben sichtbar.
    vol = nt.nodes.new("ShaderNodeVolumePrincipled"); vol.location = (560, -420)
    vol.inputs["Color"].default_value = (0.95, 0.86, 0.84, 1.0)
    vol.inputs["Density"].default_value = 0.08
    vol.inputs["Anisotropy"].default_value = 0.35
    # Kaum Absorption: Gewebe ohne Blut schluckt wenig – eine erste Fassung wurde grau-braun
    vol.inputs["Absorption Color"].default_value = (0.96, 0.88, 0.87, 1.0)
    nt.links.new(vol.outputs["Volume"], out.inputs["Volume"])

    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return mat


def clear_collection(coll):
    for obj in list(coll.objects):
        data = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
        if data is not None and data.users == 0:
            if isinstance(data, bpy.types.MetaBall):
                bpy.data.metaballs.remove(data)
            elif isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)


def build():
    scn = bpy.data.scenes.get(SCENE) or bpy.data.scenes.new(SCENE)
    if bpy.context.window:
        bpy.context.window.scene = scn
    coll = bpy.data.collections.get("Embryo_Tag28")
    if coll is None:
        coll = bpy.data.collections.new("Embryo_Tag28")
        scn.collection.children.link(coll)
    clear_collection(coll)
    obj = bpy.data.objects.new("Embryo_Tag28_Grundform", build_metaball())
    coll.objects.link(obj)
    return obj


def finish(metaball_obj):
    """Grundform -> Netz mit Feinformen."""
    obj = to_mesh(metaball_obj)
    lo, hi = surface_detail(obj)
    tissue_masks(obj)
    make_material(obj)
    inner_organs(obj.users_collection[0])
    print("GENESIS: Embryo Tag 28: %d Punkte, Verschiebung %.1f bis %.1f µm" % (len(obj.data.vertices), lo, hi))
    return obj


# ------------------------------------------------------------------------------------------------
# Export nach Unreal (1 µm = 1 Unreal-Einheit, wie alle Mikroszenen)
# ------------------------------------------------------------------------------------------------

# Die Hülle wird in Unreal durchscheinend gezeichnet – das kann Nanite nicht. Sie ist glatt, deshalb
# verliert sie bei 30 % der Dreiecke nichts Sichtbares; die Organe bleiben in voller Auflösung (Nanite).
SHELL_KEEP = 0.30
EXPORT_PARTS = {
    "Embryo_Tag28": "SM_GEN_Embryo28_Huelle",
    "Embryo_Tag28_Herz": "SM_GEN_Embryo28_Herz",
    "Embryo_Tag28_Gefaesse": "SM_GEN_Embryo28_Gefaesse",
    "Embryo_Tag28_Neuralrohr": "SM_GEN_Embryo28_Neuralrohr",
    "Embryo_Tag28_Somiten": "SM_GEN_Embryo28_Somiten",
    "Embryo_Tag28_Leberanlage": "SM_GEN_Embryo28_Leberanlage",
}


def export_all(out_dir):
    import os
    os.makedirs(out_dir, exist_ok=True)
    report = {}
    for obj_name, asset in EXPORT_PARTS.items():
        obj = bpy.data.objects[obj_name]
        added = None
        if obj_name == "Embryo_Tag28":
            added = obj.modifiers.new("Fuer_Unreal", "DECIMATE")
            added.ratio = SHELL_KEEP
        bpy.ops.object.select_all(action="DESELECT")
        obj.hide_viewport = False
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        path = os.path.join(out_dir, asset + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True,
                                 apply_scale_options="FBX_SCALE_UNITS", object_types={"MESH"},
                                 use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False,
                                 add_leaf_bones=False, bake_space_transform=False)
        if added is not None:
            obj.modifiers.remove(added)
        report[asset] = round(os.path.getsize(path) / 1e6, 1)
        print("GENESIS: exportiert %s (%.1f MB)" % (path, report[asset]))
    return report


if "GENESIS_ONLY_BLOCKOUT" not in globals():
    finish(build())
else:
    build()
