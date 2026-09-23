# GENESIS: Der Kreislauf des Lebens
#
# Die Umgebung des Embryos am Ende der vierten Woche (GENESIS-041 Teil 5): die Fruchthöhle.
#
#   * Amnion: eine dünne, durchsichtige Haut, die den Embryo eng umschließt (Fruchtwasser darin).
#     Mit vier Wochen misst die Amnionhöhle nur wenig mehr als der Embryo; sie setzt am Nabel an.
#   * Dottersack: außerhalb des Amnions in der Chorionhöhle, rund 3,5 mm (Ultraschall 6. SSW: 3–5 mm,
#     über 6 mm gilt als auffällig). Auf ihm entsteht das erste Blut: Blutinseln und ein Netz von
#     Dottergefäßen, die zum Dottergang zusammenlaufen.
#   * Dottergang: vom Nabel des Embryos zum Dottersack, mit Dotterarterie und -vene.
#   * Haftstiel: vom Schwanzende des Nabels zur Chorionplatte, mit den Nabelgefäßen – die eigentliche Verbindung zur
#     Mutter; aus ihm und dem Dottergang wird ab Woche 4–8 die Nabelschnur.
#   * Chorionplatte: die Wand der Fruchtblase dort, wo sie in der Gebärmutterwand verankert ist (werdende Plazenta),
#     mit Gefäßen, die vom Haftstiel ausstrahlen. Die übrige Wand der Chorionhöhle: glatt, fern, dunkel.
#
# Lage im selben Koordinatensystem wie der Embryo (build_embryo_day28.py): 1 µm = 0,01 m, der Embryo
# um den Ursprung, Körperebene = XZ, Nabelansatz bei Bildpunkt (345 | 530) des Referenzausschnitts.
#
# Im laufenden Blender:  exec(open(r"...\build_fruchthoehle.py", encoding="utf-8").read())

import math
import os
import bpy
import numpy as np
from mathutils import Vector, Matrix

UM = 0.01
PX_UM = 6.7
CENTER_PX = (375.0, 380.0)
SCENE = "GEN_Embryo_Tag28"
COLL = "Fruchthoehle_Tag28"
OUT_DIR = r"C:\Users\manue\Desktop\Genesis Game\ArtSource\Generated\Embryogenesis"
rng = np.random.default_rng(28)


def px(x, y):
    return np.array([(x - CENTER_PX[0]) * PX_UM, -(y - CENTER_PX[1]) * PX_UM])


def v_um(x, y, z):
    return Vector((x * UM, y * UM, z * UM))


# Nabel (Ansatz von Dottergang und Haftstiel): auf der Bauchwand unterhalb der Leberanlage, über dem eingerollten
# Schwanz. Per Strahl gegen die Hülle gemessen: Die Wand liegt dort bei z ≈ -420 µm, die Normale zeigt nach unten.
# (Eine erste Fassung setzte den Nabel in die Lücke zwischen Bauch und Schwanz – der Stiel endete offen im Wasser.)
UMBILICUS = np.array([-100.0, -420.0])
UMBILICUS_INSIDE = 70.0             # der Stiel beginnt knapp unter der Haut, damit der Übergang geschlossen ist
# Der Dottersack liegt neben dem Embryo auf der Bauchseite, etwas hinter ihm, 3,5 mm Durchmesser. Im Bild der
# Fruchthöhlen-Kamera steht er links unten, der Embryo rechts oben, der Dottergang verbindet beide. Er liegt weit
# genug hinten, dass er im Weitwinkel nicht das Bild beherrscht.
YOLK_RADIUS = 1750.0
YOLK_CENTER = np.array([-4600.0, 1200.0, -2100.0])
# Amnion: Ellipsoid um den Embryo (Halbachsen in µm), leicht gegen die Körpermitte versetzt
AMNION_CENTER = np.array([150.0, 0.0, 150.0])
AMNION_SEMI = np.array([3500.0, 2300.0, 3500.0])
CHORION_RADIUS = 11000.0            # Chorionhöhle ~ 22 mm Durchmesser (6. SSW)
# Die eigentliche Lebensleitung: Der Haftstiel verbindet den Embryo mit der Chorionplatte – der Wand der Fruchtblase auf
# der Seite, mit der sie in der Gebärmutterwand (Decidua basalis) verankert ist; dort wächst die Plazenta. Der Dottersack
# dagegen schwebt frei. (Game Director: „Sollte da nicht die Gebärmutter dran sein und kein schwebender Ball?" – eine
# erste Fassung ließ den Haftstiel ins Dunkel laufen, und das Kind schien am Dottersack zu hängen.)
# Die Platte steht hinter dem Embryo, jenseits des Amnions: Der Ansatz liegt auf dem Sichtstrahl der Szenenkamera durch
# den Bildpunkt (760 | 420) von 1066 × 600 – rechts unter dem Embryo –, 2,8 mm tiefer als seine Mitte; die Wand steht
# dort quer zum Blick. Der Haftstiel wird so rund 5 mm lang und läuft quer durchs Bild. (Die erste Rechnung setzte den
# Versatz in Mikrometern statt auf dem Sichtstrahl an – durch die Perspektive landete der Ansatz hinter dem Embryo.)
WALL_NORMAL = np.array([0.810, 0.414, -0.415])              # Sichtstrahl zum Ansatz (Blender-Achsen)
PLACENTA_SITE = np.array([3375.0, -321.0, -4255.0])         # Ansatz des Haftstiels an der Chorionplatte
CHORION_CENTER = PLACENTA_SITE - WALL_NORMAL / np.linalg.norm(WALL_NORMAL) * CHORION_RADIUS
PLATE_ANGLE = math.radians(34.0)                             # Ausdehnung der Chorionplatte um den Ansatz


def new_collection():
    scn = bpy.data.scenes.get(SCENE) or bpy.context.scene
    if bpy.context.window:
        bpy.context.window.scene = scn
    coll = bpy.data.collections.get(COLL)
    if coll is None:
        coll = bpy.data.collections.new(COLL)
        scn.collection.children.link(coll)
    for obj in list(coll.objects):
        data = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
        if isinstance(data, bpy.types.Mesh) and data.users == 0:
            bpy.data.meshes.remove(data)
        if isinstance(data, bpy.types.Curve) and data.users == 0:
            bpy.data.curves.remove(data)
    return coll


def mesh_object(coll, name, verts, faces):
    mesh = bpy.data.meshes.new("SM_" + name)
    mesh.from_pydata([tuple(v) for v in verts], [], [tuple(f) for f in faces])
    mesh.update()
    for p in mesh.polygons:
        p.use_smooth = True
    obj = bpy.data.objects.new(name, mesh)
    coll.objects.link(obj)
    return obj


def uv_sphere(res_u=160, res_v=80):
    verts, faces = [], []
    for j in range(res_v + 1):
        t = math.pi * j / res_v
        for i in range(res_u):
            p = 2 * math.pi * i / res_u
            verts.append((math.sin(t) * math.cos(p), math.sin(t) * math.sin(p), math.cos(t)))
    for j in range(res_v):
        for i in range(res_u):
            a = j * res_u + i
            b = j * res_u + (i + 1) % res_u
            faces.append((a, a + res_u, b + res_u, b))   # Normalen nach außen (Unreal zeichnet nur Vorderseiten)
    return np.array(verts), faces


def noise3(p, freq, seed):
    """Glatte Pseudozufallsverformung aus überlagerten Sinuswellen – billig, ohne Zusatzmodul."""
    r = np.random.default_rng(seed)
    total = np.zeros(len(p))
    for k in range(6):
        d = r.normal(size=3)
        d /= np.linalg.norm(d)
        ph = r.uniform(0, 2 * math.pi)
        total += np.sin((p @ d) * freq * (1.7 ** k) + ph) / (1.8 ** k)
    return total / 1.9


# ------------------------------------------------------------------------------------------------
# Amnion: dünne Haut mit sanften Falten, am Nabel eingezogen
# ------------------------------------------------------------------------------------------------

def build_amnion(coll):
    unit, faces = uv_sphere(400, 200)   # fein genug für die Falten
    pts = unit * AMNION_SEMI
    # leichte Unregelmäßigkeit: eine Membran in Flüssigkeit ist nie eine perfekte Kugel
    pts *= (1.0 + 0.025 * noise3(unit, 3.0, 7))[:, None]
    pts += AMNION_CENTER
    # Am Nabel setzt das Amnion an der Körperwand an: dort zieht es sich zum Embryo hin
    u = np.array([UMBILICUS[0], 0.0, UMBILICUS[1]])
    d = pts - u
    dist = np.linalg.norm(d, axis=1)
    pull = np.exp(-(dist / 1900.0) ** 2)
    pts = pts - d * (pull * 0.72)[:, None]
    # Falten: Eine Membran in Flüssigkeit liegt nie glatt. Scharfe Knicke (aus dem Betrag einer Welle, zur Spitze
    # geschärft) mit 20–45 µm Höhe, am Nabel, wo die Haut zusammengezogen ist, dichter und tiefer. Auf jedem Knick fängt
    # sich das Licht als schmales Glanzlicht – daran erkennt man die echte Fruchtblase (Doc 36).
    normal = (pts - AMNION_CENTER) / np.linalg.norm(pts - AMNION_CENTER, axis=1)[:, None]
    crease = (1.0 - np.abs(noise3(unit, 7.0, 71))) ** 5 + 0.6 * (1.0 - np.abs(noise3(unit, 15.0, 72))) ** 6
    gather = 1.0 + 2.5 * np.exp(-(dist / 1400.0) ** 2)
    pts = pts + normal * (crease * 32.0 * gather)[:, None]
    return mesh_object(coll, "Amnion", pts * UM, faces)


# ------------------------------------------------------------------------------------------------
# Dottersack mit Blutinseln und Dottergefäßen
# ------------------------------------------------------------------------------------------------

def stalk_pole():
    """Richtung vom Dottersack zum Nabel (dort setzt der Dottergang an)."""
    u = np.array([UMBILICUS[0], 0.0, UMBILICUS[1]])
    d = u - YOLK_CENTER
    return d / np.linalg.norm(d)


def build_yolk_sac(coll):
    unit, faces = uv_sphere(220, 110)
    wobble = 1.0 + 0.03 * noise3(unit, 2.5, 11) + 0.006 * noise3(unit, 14.0, 12)
    pts = unit * YOLK_RADIUS * wobble[:, None] + YOLK_CENTER
    return mesh_object(coll, "Dottersack", pts * UM, faces)


def vessel_tree(n_roots=8, seed=3, center=None, radius=YOLK_RADIUS, pole=None, lift=1.001, span=1.0,
                sizes=((19.0, 7.0), (9.5, 4.0), (5.0, 2.5))):
    """
    Dottergefäße: Von den Blutinseln am fernen Pol laufen sie über die Oberfläche zusammen und münden am
    Stielpol in Dotterarterie und -vene. Als Pfade auf der Kugel (Radius, Liste von Punkten).
    Dieselbe Bauart dient den Gefäßen der Chorionplatte: dort von innen auf der Wand (lift < 1), nur als Kappe um den
    Ansatz des Haftstiels (span < 1).
    """
    r = np.random.default_rng(seed)
    center = YOLK_CENTER if center is None else center
    pole = stalk_pole() if pole is None else pole
    # lokale Basis: z zum Stiel
    a = np.cross(pole, [0.0, 0.0, 1.0])
    if np.linalg.norm(a) < 1e-3:
        a = np.cross(pole, [1.0, 0.0, 0.0])
    a /= np.linalg.norm(a)
    b = np.cross(pole, a)

    def on_sphere(theta, phi, lift=1.0):
        d = math.sin(theta) * (math.cos(phi) * a + math.sin(phi) * b) + math.cos(theta) * pole
        return center + d * radius * lift

    paths = []
    for k in range(n_roots):
        phi0 = 2 * math.pi * k / n_roots + r.uniform(-0.3, 0.3)
        # Hauptast vom Stielpol (theta 0) nach hinten, leicht geschlängelt
        steps = 60
        main = []
        # Die Hauptäste reichen verschieden weit und schlängeln sich – laufen sie alle bis zum Gegenpol, sieht die Kugel
        # aus wie ein Ball mit Nähten
        reach = r.uniform(1.3, 2.3) * span
        wiggle = r.uniform(0.35, 0.7)
        for s in range(steps):
            t = s / (steps - 1)
            theta = 0.12 * span + t * reach
            phi = phi0 + wiggle * math.sin(t * 5.0 + k) * t
            main.append(on_sphere(theta, phi, lift))
        paths.append((sizes[0][0], sizes[0][1], main))
        # Seitenäste und Zweige in ruhigen Bögen. Die Zufallswerte gelten je Ast, nicht je Schritt –
        # eine erste Fassung zog sie je Schritt und ergab gerade Stäbe kreuz und quer.
        main_theta = [0.12 * span + (s / (steps - 1)) * reach for s in range(steps)]
        for j in range(7):
            t0 = 0.18 + 0.1 * j + r.uniform(-0.03, 0.03)
            i0 = int(t0 * (steps - 1))
            base_theta = main_theta[i0]
            base_phi = phi0 + wiggle * math.sin(t0 * 5.0 + k) * t0
            side = r.choice([-1.0, 1.0])
            reach_theta = r.uniform(0.25, 0.55) * span
            reach_phi = side * r.uniform(0.35, 0.75)
            bend = r.uniform(-0.25, 0.25)
            branch = []
            for s in range(40):
                t = s / 39.0
                theta = base_theta + reach_theta * t + bend * math.sin(math.pi * t) * 0.3
                phi = base_phi + reach_phi * (1.0 - (1.0 - t) ** 1.6)
                branch.append(on_sphere(theta, phi, lift))
            branch[0] = main[i0]
            paths.append((sizes[1][0], sizes[1][1], branch))
            for m in range(3):
                t1 = r.uniform(0.35, 0.85)
                i1 = int(t1 * 39)
                th_start = base_theta + reach_theta * t1
                ph_start = base_phi + reach_phi * (1.0 - (1.0 - t1) ** 1.6)
                dth = r.uniform(0.12, 0.3) * span
                dph = r.choice([-1.0, 1.0]) * r.uniform(0.18, 0.4)
                twig = []
                for s in range(18):
                    t = s / 17.0
                    twig.append(on_sphere(th_start + dth * t, ph_start + dph * (1.0 - (1.0 - t) ** 1.8), lift))
                twig[0] = branch[i1]
                paths.append((sizes[2][0], sizes[2][1], twig))
    return paths


def blood_islands(n=180, seed=5):
    """Blutinseln: rötliche Zellhaufen am fernen Pol, wo die Blutbildung begann."""
    r = np.random.default_rng(seed)
    pole = stalk_pole()
    pts = []
    while len(pts) < n:
        d = r.normal(size=3)
        d /= np.linalg.norm(d)
        if d @ pole < -0.1:        # Gegenseite des Stiels
            pts.append(YOLK_CENTER + d * YOLK_RADIUS * 1.002)
    return np.array(pts)


def tubes_from_paths(coll, name, paths, ring=10, center=None):
    """Röhren entlang der Pfade, Radius vom Anfang zum Ende abnehmend."""
    verts, faces = [], []
    for r0, r1, path in paths:
        P = np.array(path)
        n = len(P)
        base = len(verts)
        for i in range(n):
            t = i / max(1, n - 1)
            rad = r0 + (r1 - r0) * t
            tan = P[min(i + 1, n - 1)] - P[max(i - 1, 0)]
            tan /= np.linalg.norm(tan) + 1e-9
            up = (P[i] - (YOLK_CENTER if center is None else center))
            up /= np.linalg.norm(up) + 1e-9
            side = np.cross(tan, up)
            side /= np.linalg.norm(side) + 1e-9
            up2 = np.cross(side, tan)
            for k in range(ring):
                a = 2 * math.pi * k / ring
                verts.append(P[i] + (math.cos(a) * side + math.sin(a) * up2) * rad)
        for i in range(n - 1):
            for k in range(ring):
                a = base + i * ring + k
                b = base + i * ring + (k + 1) % ring
                faces.append((a, a + ring, b + ring, b))   # Normalen nach außen
    return mesh_object(coll, name, np.array(verts) * UM, faces)


def blobs(coll, name, centers, radius, jitter=0.4, seed=9):
    r = np.random.default_rng(seed)
    unit, faces_one = uv_sphere(12, 6)
    verts, faces = [], []
    for c in centers:
        s = radius * (1.0 + jitter * r.uniform(-1, 1))
        flat = (c - YOLK_CENTER) / np.linalg.norm(c - YOLK_CENTER)
        pts = unit * s
        # flach auf der Oberfläche
        pts -= np.outer(pts @ flat, flat) * 0.6
        base = len(verts)
        verts.extend(list(pts + c))
        faces.extend([tuple(v + base for v in f) for f in faces_one])
    return mesh_object(coll, name, np.array(verts) * UM, faces)


# ------------------------------------------------------------------------------------------------
# Dottergang und Haftstiel
# ------------------------------------------------------------------------------------------------

def bezier(p0, p1, p2, p3, n):
    t = np.linspace(0, 1, n)[:, None]
    return (1 - t) ** 3 * p0 + 3 * (1 - t) ** 2 * t * p1 + 3 * (1 - t) * t ** 2 * p2 + t ** 3 * p3


def frames(path):
    """Mitlaufendes Koordinatensystem entlang eines Pfads (parallel transportiert, damit es sich nicht verdreht)."""
    P = np.array(path)
    n = len(P)
    tans = np.array([P[min(i + 1, n - 1)] - P[max(i - 1, 0)] for i in range(n)])
    tans /= np.linalg.norm(tans, axis=1)[:, None] + 1e-9
    side = np.cross(tans[0], [0.0, 1.0, 0.0])
    if np.linalg.norm(side) < 1e-3:
        side = np.cross(tans[0], [1.0, 0.0, 0.0])
    side /= np.linalg.norm(side)
    sides, ups = [], []
    for i in range(n):
        side = side - tans[i] * (side @ tans[i])
        side /= np.linalg.norm(side) + 1e-9
        sides.append(side)
        ups.append(np.cross(side, tans[i]))
    return P, np.array(sides), np.array(ups)


def tube(coll, name, path, radii, ring=24, wobble=0.0, seed=1, cap_start=False):
    P, sides, ups = frames(path)
    n = len(P)
    verts, faces = [], []
    r = np.random.default_rng(seed)
    phases = r.uniform(0, 2 * math.pi, 3)
    for i in range(n):
        rad = radii[i]
        for k in range(ring):
            a = 2 * math.pi * k / ring
            w = 1.0 + wobble * math.sin(a * 2 + phases[0] + i * 0.15) * math.sin(i * 0.07 + phases[1])
            verts.append(P[i] + (math.cos(a) * sides[i] + math.sin(a) * ups[i]) * rad * w)
    for i in range(n - 1):
        for k in range(ring):
            a = i * ring + k
            b = i * ring + (k + 1) % ring
            faces.append((a, a + ring, b + ring, b))   # Normalen nach außen
    if cap_start:
        # geschlossener Anfang: Das Ende steckt in der Körperwand, ein offenes Rohr wäre durch die Haut zu sehen
        centre = len(verts)
        verts.append(P[0] - (P[1] - P[0]) * 0.5)
        for k in range(ring):
            faces.append((centre, k, (k + 1) % ring))
    return mesh_object(coll, name, np.array(verts) * UM, faces)


def build_stalks(coll):
    u = np.array([UMBILICUS[0], 0.0, UMBILICUS[1]])
    wall_in = np.array([0.0, 0.0, 1.0])          # Normale der Bauchwand zeigt nach unten, innen ist oben
    start = u + wall_in * UMBILICUS_INSIDE
    pole = stalk_pole()
    end = YOLK_CENTER + pole * YOLK_RADIUS * 0.96
    # Dottergang: Er verlässt den Nabel nach unten und zur Seite (am eingerollten Schwanz vorbei), schwingt dann in
    # ruhigem Bogen zum Dottersack; eng in der Mitte, am Nabel und am Dottersack trichterförmig
    path = bezier(start, start + np.array([-250.0, 900.0, -500.0]), end + pole * 1500.0 + np.array([200.0, 300.0, 500.0]), end, 110)
    t = np.linspace(0, 1, 110)
    radii = 150.0 + 170.0 * np.exp(-(t / 0.07) ** 2) + 240.0 * np.exp(-((1 - t) / 0.06) ** 2)
    duct = tube(coll, "Dottergang", path, radii, wobble=0.02, seed=2, cap_start=True)
    # Dotterarterie und -vene: in der Wand des Gangs, schwach gewunden, nur knapp unter der Oberfläche – durch das
    # gallertige Gewebe schimmern sie dunkelrot, sie liegen nicht als Schläuche obenauf
    P, sides, ups = frames(path)
    vessels = []
    for phase, rad in ((0.4, 38.0), (0.4 + math.pi * 0.85, 46.0)):
        pts = []
        for i in range(len(P)):
            a = phase + t[i] * 2.2 * math.pi
            depth = radii[i] - rad * 0.5
            pts.append(P[i] + (math.cos(a) * sides[i] + math.sin(a) * ups[i]) * depth)
        vessels.append((rad, rad * 0.9, pts[3:-2]))
    vit = tubes_from_paths(coll, "Dottergang_Gefaesse", vessels, ring=12)

    # Haftstiel: kurz und kräftig, vom Schwanzende des Nabels zur Chorionplatte. Er trägt die Nabelgefäße (mit vier
    # Wochen zwei Arterien und zwei Venen), die sich in der Platte verzweigen. Am Ansatz weitet er sich trichterförmig
    # in die Wand und reicht ein Stück hinein, damit kein Spalt bleibt.
    tail_side = px(262, 578)
    s0 = np.array([tail_side[0], 0.0, tail_side[1]])
    wall_n = (PLACENTA_SITE - CHORION_CENTER) / CHORION_RADIUS
    s3 = PLACENTA_SITE + wall_n * 120.0
    spath = bezier(s0, s0 + np.array([900.0, 700.0, -700.0]), s3 - wall_n * 1400.0, s3, 90)
    ts = np.linspace(0, 1, 90)
    sradii = 290.0 + 80.0 * np.exp(-(ts / 0.1) ** 2) + 230.0 * np.exp(-((1 - ts) / 0.07) ** 2)
    body_stalk = tube(coll, "Haftstiel", spath, sradii, ring=28, wobble=0.025, seed=4, cap_start=True)
    P, sides, ups = frames(spath)
    cord = []
    for phase, rad in ((0.3, 42.0), (0.3 + math.pi * 0.5, 42.0), (0.3 + math.pi, 55.0), (0.3 + math.pi * 1.5, 50.0)):
        pts = []
        for i in range(len(P)):
            a = phase + ts[i] * 1.6 * math.pi
            depth = sradii[i] - rad * 0.5       # halb in der Wand: sie zeichnen sich als rote Stränge durch
            pts.append(P[i] + (math.cos(a) * sides[i] + math.sin(a) * ups[i]) * depth)
        cord.append((rad, rad * 0.95, pts[3:]))
    cord_vessels = tubes_from_paths(coll, "Haftstiel_Gefaesse", cord, ring=12)
    return duct, vit, body_stalk, cord_vessels


# ------------------------------------------------------------------------------------------------
# Wand der Chorionhöhle: fern, zottig, von innen gesehen
# ------------------------------------------------------------------------------------------------

def chorion_surface(unit):
    """Innenwand der Chorionhöhle: sanft wellig; an der Chorionplatte flach gewölbt mit Buckeln, die von den Zotten
    und den Bluträumen der Mutter dahinter kommen. Genau durch den Ansatz des Haftstiels."""
    n = (PLACENTA_SITE - CHORION_CENTER) / CHORION_RADIUS
    cosang = unit @ n
    plate = np.clip((cosang - math.cos(PLATE_ANGLE + 0.15)) / 0.15, 0.0, 1.0)
    wave = 0.03 * noise3(unit, 3.0, 21) + 0.008 * noise3(unit, 22.0, 22)
    lumps = 0.006 * noise3(unit, 60.0, 23)
    # an der Platte selbst ruhiger (sie ist gespannt), dafür mit Buckeln; der Ansatz liegt exakt auf dem Radius
    at_site = np.exp(-((1.0 - cosang) / 0.004) ** 2)
    rough = 1.0 + (wave * (1.0 - 0.7 * plate) + lumps * plate) * (1.0 - at_site)
    return unit * CHORION_RADIUS * rough[:, None] + CHORION_CENTER, cosang


def plate_vessels(seed=17, n_main=6):
    """
    Gefäßbaum der Chorionplatte in der Tangentialebene am Ansatz: Hauptgefäße laufen in sanften Bögen nach außen und
    gabeln sich (jeder Ast etwas dünner und kürzer, leicht auseinanderstrebend). Rückgabe wie vessel_tree, die Punkte
    liegen noch in der Ebene und werden danach auf die Wand gelegt.
    """
    r = np.random.default_rng(seed)
    n = (PLACENTA_SITE - CHORION_CENTER) / CHORION_RADIUS
    a = np.cross(n, [0.0, 0.0, 1.0])
    a /= np.linalg.norm(a)
    b = np.cross(n, a)
    paths = []

    def grow(pos, heading, radius, depth):
        # Gefäße teilen sich ungleich (Murray: r³ = r1³ + r2³): Der kräftige Ast läuft fast gerade weiter, der dünne
        # zweigt steil ab. Die Abschnittslänge folgt dem Radius und streut. So wird der Baum organisch statt
        # spiegelbildlich gegabelt – eine erste Fassung gabelte symmetrisch und sah aus wie ein Diagramm (Doc 35).
        length = radius * r.uniform(18.0, 38.0)
        pts = [pos]
        steps = max(6, int(length / 45.0))
        bend = r.uniform(-0.9, 0.9) / length
        h = heading
        for _ in range(steps):
            h += bend * (length / steps) + r.normal(0.0, 0.07)
            pos = pos + (math.cos(h) * a + math.sin(h) * b) * (length / steps)
            pts.append(pos)
        paths.append((radius, radius * 0.92, pts))
        # kleine Seitenzweige unterwegs
        if radius > 8.0:
            for _ in range(r.integers(0, 3)):
                i = r.integers(2, len(pts) - 1)
                side = r.choice([-1.0, 1.0])
                twig_h = h + side * r.uniform(0.9, 1.5)
                if depth > 1:
                    grow(pts[i], twig_h, radius * r.uniform(0.25, 0.4), depth - 2)
        if depth <= 0 or radius < 4.0:
            return
        if r.random() < 0.12:
            grow(pos, h + r.normal(0.0, 0.15), radius * 0.93, depth - 1)   # läuft ungeteilt weiter
            return
        share = r.uniform(0.12, 0.45)
        big, small = radius * (1.0 - share) ** (1.0 / 3.0), radius * share ** (1.0 / 3.0)
        side = r.choice([-1.0, 1.0])
        grow(pos, h + side * r.uniform(0.1, 0.35), big, depth - 1)
        grow(pos, h - side * r.uniform(0.6, 1.1), small, depth - 1)

    start = PLACENTA_SITE - n * 40.0
    for k in range(n_main):
        heading = 2 * math.pi * k / n_main + r.uniform(-0.35, 0.35)
        grow(start + (math.cos(heading) * a + math.sin(heading) * b) * 180.0, heading, r.uniform(30.0, 46.0), 6)
    return paths


def build_chorion(coll):
    """Zwei Teile: die Chorionplatte um den Ansatz (werdende Plazenta, zur Gebärmutterwand hin) und die übrige, glatte
    Wand (Chorion laeve). Normalen nach innen – die Kamera ist drinnen."""
    unit, faces = uv_sphere(640, 320)   # die Platte steht groß hinter dem Embryo: feines Netz (Nanite)
    pts, cosang = chorion_surface(unit)
    in_plate = cosang >= math.cos(PLATE_ANGLE)
    parts = {}
    for name, keep in (("Chorionplatte", True), ("Chorionhoehle", False)):
        sel = [f for f in faces if all(in_plate[v] == keep for v in f) or (keep and any(in_plate[v] for v in f))]
        used = sorted({v for f in sel for v in f})
        remap = {v: i for i, v in enumerate(used)}
        obj = mesh_object(coll, name, pts[used] * UM, [tuple(remap[v] for v in f) for f in sel])
        bpy.context.view_layer.objects.active = obj
        obj.data.flip_normals()
        parts[name] = obj
    # Gefäße der Chorionplatte: vom Ansatz des Haftstiels strahlen sie aus und teilen sich gabelig, flach in der Wand
    # (eine erste Fassung nahm den Gefäßbaum des Dottersacks – um den Pol herum ringelte er sich wie Stacheldraht)
    laid = []
    for r0, r1, path in plate_vessels():
        P = np.array(path)
        u = (P - CHORION_CENTER) / np.linalg.norm(P - CHORION_CENTER, axis=1)[:, None]
        surf, _ = chorion_surface(u)
        inward = -u * (0.4 * np.linspace(r0, r1, len(P)))[:, None]
        laid.append((r0, r1, list(surf + inward)))
    parts["Chorionplatte_Gefaesse"] = tubes_from_paths(coll, "Chorionplatte_Gefaesse", laid, center=CHORION_CENTER)
    return parts


def build():
    coll = new_collection()
    parts = {
        "Amnion": build_amnion(coll),
        "Dottersack": build_yolk_sac(coll),
    }
    paths = vessel_tree()
    parts["Dottersack_Gefaesse"] = tubes_from_paths(coll, "Dottersack_Gefaesse", paths)
    parts["Dottersack_Blutinseln"] = blobs(coll, "Dottersack_Blutinseln", blood_islands(90), 14.0)
    duct, vit, body, cord = build_stalks(coll)
    parts["Dottergang"] = duct
    parts["Dottergang_Gefaesse"] = vit
    parts["Haftstiel"] = body
    parts["Haftstiel_Gefaesse"] = cord
    parts.update(build_chorion(coll))
    for obj in parts.values():
        obj.data.materials.clear()
    report = {k: len(v.data.polygons) for k, v in parts.items()}
    print("GENESIS: Fruchthöhle gebaut", report)
    return parts


EXPORTS = {
    "Amnion": "SM_GEN_Embryo28_Amnion",
    "Dottersack": "SM_GEN_Embryo28_Dottersack",
    "Dottersack_Gefaesse": "SM_GEN_Embryo28_DottersackGefaesse",
    "Dottersack_Blutinseln": "SM_GEN_Embryo28_Blutinseln",
    "Dottergang": "SM_GEN_Embryo28_Dottergang",
    "Dottergang_Gefaesse": "SM_GEN_Embryo28_DottergangGefaesse",
    "Haftstiel": "SM_GEN_Embryo28_Haftstiel",
    "Haftstiel_Gefaesse": "SM_GEN_Embryo28_HaftstielGefaesse",
    "Chorionplatte": "SM_GEN_Embryo28_Chorionplatte",
    "Chorionplatte_Gefaesse": "SM_GEN_Embryo28_ChorionplatteGefaesse",
    "Chorionhoehle": "SM_GEN_Embryo28_Chorionhoehle",
}


def export_all(out_dir=OUT_DIR):
    os.makedirs(out_dir, exist_ok=True)
    for obj_name, asset in EXPORTS.items():
        obj = bpy.data.objects[obj_name]
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        path = os.path.join(out_dir, asset + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True,
                                 apply_scale_options="FBX_SCALE_UNITS", object_types={"MESH"},
                                 use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False,
                                 add_leaf_bones=False)
        print("GENESIS: exportiert", asset, round(os.path.getsize(path) / 1e6, 1), "MB")


if "GENESIS_NO_RUN" not in globals():
    build()
