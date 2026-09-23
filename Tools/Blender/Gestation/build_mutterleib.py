# GENESIS: Der Kreislauf des Lebens
#
# Der Mutterleib von innen, aus Sicht des Kindes (GENESIS-044 Teil 1b, Docs/34, Referenzen Docs/36):
#
#   * Wand: die Fruchtblase (Amnion und Chorion, ab SSW ~14 verschmolzen) an der Gebärmutterwand – eine glatte,
#     glänzende Haut mit sanften Falten. Durch die vordere Wand (zum Bauch der Mutter, +X) kommt Licht, fast nur Rot.
#   * Plazenta: hinten oben an der Wand (die häufigste Lage), die Kindseite glänzend, bläulich-violett durchscheinend
#     über dunkelrotem Gewebe; große Gefäße laufen erhaben vom Ansatz der Nabelschnur aus und teilen sich ungleich.
#   * Nabelschnur: gewunden, weißlich-bläulich glänzend, zwei Arterien und eine Vene spiralig in der Wharton-Sulze.
#
# Gebaut für 10 cm Innenradius (AGenesisWombScene skaliert mit der Woche). Einheit: 1 cm = 0,01 m (Unreal: 1 cm).
# Die Kamera – das Kind – sitzt bei (-2,5 | 0 | 1) cm und blickt nach vorn; der Nabel darunter.
#
# Headless:  blender -b <datei.blend> --python-expr "g={'__name__':'genesis'}; exec(open(r'...build_mutterleib.py').read(), g); g['export_all']()"

import math
import os
import bpy
import numpy as np

CM = 0.01
OUT_DIR = r"C:\Users\manue\Desktop\Genesis Game\ArtSource\Generated\Gestation"
COLL = "Mutterleib"
SEMI = np.array([10.0, 8.5, 9.0])                       # Innenhalbachsen (cm): länglich zum Fundus hin
PLACENTA_DIR = np.array([-0.62, 0.25, 0.74])            # hinten oben
PLACENTA_ANGLE = math.radians(38.0)
NAVEL = np.array([-2.0, 0.0, -3.2])                     # Nabel des Kindes unter der Kamera
GEFAESS_WINDUNGEN = 5                                    # Windungen über die Länge (~0,2 je cm wie im Mittel echter Nabelschnüre; Material liest sie mit)
rng = np.random.default_rng(40)

# Die gemeinsamen Bauteile (Kugelnetz, Rauschen, Röhren) aus der Fruchthöhle
_lib = {"__name__": "genesis_lib", "GENESIS_NO_RUN": 1}
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals()
                       else r"C:\Users\manue\Desktop\Genesis Game\Tools\Blender\Gestation",
                       "..", "Embryogenesis", "build_fruchthoehle.py"), encoding="utf-8").read(), _lib)
uv_sphere, noise3, bezier, frames = _lib["uv_sphere"], _lib["noise3"], _lib["bezier"], _lib["frames"]


def new_collection():
    coll = bpy.data.collections.get(COLL)
    if coll is None:
        coll = bpy.data.collections.new(COLL)
        bpy.context.scene.collection.children.link(coll)
    for obj in list(coll.objects):
        data = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
        if isinstance(data, bpy.types.Mesh) and data.users == 0:
            bpy.data.meshes.remove(data)
    return coll


def mesh_object(coll, name, verts, faces, flip=False):
    mesh = bpy.data.meshes.new("SM_" + name)
    mesh.from_pydata([tuple(v) for v in verts], [], [tuple(f) for f in faces])
    if flip:
        mesh.flip_normals()
    mesh.update()
    for p in mesh.polygons:
        p.use_smooth = True
    obj = bpy.data.objects.new(name, mesh)
    coll.objects.link(obj)
    return obj


def wall_surface(unit):
    """Innenwand: Ellipsoid mit sanften Wellen; an der Plazenta nach innen gewölbt (sie ist 2–3 cm dick am Termin)."""
    n = PLACENTA_DIR / np.linalg.norm(PLACENTA_DIR)
    cosang = unit @ n
    plate = np.clip((cosang - math.cos(PLACENTA_ANGLE)) / (1.0 - math.cos(PLACENTA_ANGLE)), 0.0, 1.0)
    wave = 1.0 + 0.02 * noise3(unit, 3.0, 5) + 0.004 * noise3(unit, 18.0, 6)
    pts = unit * SEMI * wave[:, None]
    bulge = 1.4 * np.sin(plate * math.pi / 2.0) ** 0.6   # cm nach innen
    radius = np.linalg.norm(pts, axis=1)
    pts = pts * ((radius - bulge) / radius)[:, None]
    return pts, cosang


def build_wall_and_placenta(coll):
    unit, faces = uv_sphere(320, 160)
    pts, cosang = wall_surface(unit)
    inside = cosang >= math.cos(PLACENTA_ANGLE)
    parts = {}
    for name, keep in (("Placenta", True), ("Wall", False)):
        sel = [f for f in faces if (keep and any(inside[v] for v in f)) or (not keep and not any(inside[v] for v in f))]
        used = sorted({v for f in sel for v in f})
        remap = {v: i for i, v in enumerate(used)}
        # Normalen nach innen: Die Kamera ist drinnen (uv_sphere zeigt nach außen)
        parts[name] = mesh_object(coll, name, pts[used] * CM, [tuple(remap[v] for v in f) for f in sel], flip=True)
    return parts


def placenta_vessels(coll):
    """Gefäße der Kindseite: vom Ansatz der Nabelschnur aus, ungleich geteilt (Murray), erhaben auf der Oberfläche."""
    n = PLACENTA_DIR / np.linalg.norm(PLACENTA_DIR)
    a = np.cross(n, [0.0, 0.0, 1.0]); a /= np.linalg.norm(a)
    b = np.cross(n, a)
    paths = []

    def grow(angle_pos, heading, radius, depth):
        # Winkelkoordinaten in der Tangentialebene um den Ansatz (Bogenmaß auf der Einheitskugel)
        length = radius * rng.uniform(9.0, 16.0) / 10.0     # rad
        steps = max(8, int(length / 0.02))
        pts = [angle_pos.copy()]
        h = heading
        bend = rng.uniform(-1.2, 1.2) / max(length, 1e-3)
        for _ in range(steps):
            h += bend * (length / steps) + rng.normal(0.0, 0.05)
            angle_pos = angle_pos + np.array([math.cos(h), math.sin(h)]) * (length / steps)
            pts.append(angle_pos.copy())
        if np.linalg.norm(angle_pos) > PLACENTA_ANGLE * 0.95:
            pts = [p for p in pts if np.linalg.norm(p) <= PLACENTA_ANGLE * 0.95] or pts[:2]
        paths.append((radius, radius * 0.9, pts))
        if depth <= 0 or radius < 0.05 or np.linalg.norm(angle_pos) > PLACENTA_ANGLE * 0.9:
            return
        share = rng.uniform(0.15, 0.45)
        big, small = radius * (1 - share) ** (1 / 3), radius * share ** (1 / 3)
        side = rng.choice([-1.0, 1.0])
        grow(angle_pos, h + side * rng.uniform(0.1, 0.35), big, depth - 1)
        grow(angle_pos, h - side * rng.uniform(0.6, 1.1), small, depth - 1)

    for k in range(7):
        heading = 2 * math.pi * k / 7 + rng.uniform(-0.3, 0.3)
        grow(np.array([math.cos(heading), math.sin(heading)]) * 0.03, heading, rng.uniform(0.28, 0.38), 6)

    verts, faces = [], []
    ring = 10
    for r0, r1, pts in paths:
        # auf die Plazentafläche legen (etwas über der Oberfläche: die Gefäße liegen erhaben)
        world = []
        for p in pts:
            theta = np.linalg.norm(p)
            d = n * math.cos(theta) + (a * p[0] + b * p[1]) / max(theta, 1e-6) * math.sin(theta) if theta > 1e-6 else n
            surf, _ = wall_surface(np.array([d / np.linalg.norm(d)]))
            world.append(surf[0] * (1.0 - 0.02))
        P = np.array(world)
        count = len(P)
        if count < 2:
            continue
        base = len(verts)
        for i in range(count):
            t = i / max(1, count - 1)
            rad = r0 + (r1 - r0) * t
            tan = P[min(i + 1, count - 1)] - P[max(i - 1, 0)]
            tan /= np.linalg.norm(tan) + 1e-9
            up = -P[i] / (np.linalg.norm(P[i]) + 1e-9)
            side = np.cross(tan, up); side /= np.linalg.norm(side) + 1e-9
            up2 = np.cross(side, tan)
            for k in range(ring):
                ang = 2 * math.pi * k / ring
                verts.append(P[i] + (math.cos(ang) * side + math.sin(ang) * up2) * rad)
        for i in range(count - 1):
            for k in range(ring):
                q = base + i * ring + k
                w = base + i * ring + (k + 1) % ring
                faces.append((q, q + ring, w + ring, w))
    return mesh_object(coll, "PlacentaVessels", np.array(verts) * CM, faces)


def build_cord(coll):
    """Nabelschnur vom Ansatz an der Plazenta in lockeren Schlingen zum Nabel, gewunden, drei Gefäße darin."""
    n = PLACENTA_DIR / np.linalg.norm(PLACENTA_DIR)
    surf, _ = wall_surface(np.array([n]))
    start = surf[0] * 0.97
    # Eine Schlinge schwebt vor dem Gesicht durch das Licht der vorderen Wand – so zeigen sie Fetoskopie-Aufnahmen
    ctrl1 = np.array([4.0, 4.5, 5.0])
    ctrl2 = np.array([5.0, -2.5, -1.5])
    base = bezier(start, ctrl1, ctrl2, NAVEL, 220)
    # lockere Schlingen um die Grundlinie
    P, sides, ups = frames(base)
    t = np.linspace(0.0, 1.0, len(P))
    loop = 1.1 * np.sin(math.pi * t)[:, None]
    path = P + loop * (np.cos(t * 7.0)[:, None] * sides + np.sin(t * 7.0)[:, None] * ups)
    radius = np.full(len(path), 0.65)                      # ~1,3 cm Durchmesser (am Termin 1–2 cm)
    P, sides, ups = frames(path)
    ring = 24
    verts, faces = [], []
    for i in range(len(P)):
        for k in range(ring):
            ang = 2 * math.pi * k / ring
            wobble = 1.0 + 0.05 * math.sin(3 * ang + i * 0.4)
            verts.append(P[i] + (math.cos(ang) * sides[i] + math.sin(ang) * ups[i]) * radius[i] * wobble)
    for i in range(len(P) - 1):
        for k in range(ring):
            q = i * ring + k
            w = i * ring + (k + 1) % ring
            faces.append((q, q + ring, w + ring, w))
    cord = mesh_object(coll, "Cord", np.array(verts) * CM, faces)
    # UV: u entlang der Schnur (0..1), v um sie herum (0..1). Das Material liest daraus die Spirale der Gefäße
    # (GEFAESS_WINDUNGEN, Phasen wie unten), die im Gegenlicht als dunkle Stränge durch die Sulze scheinen.
    uv = cord.data.uv_layers.new(name="UVMap")
    for poly in cord.data.polygons:
        i0, k0 = divmod(poly.vertices[0], ring)
        for loop_index, vertex in zip(poly.loop_indices, poly.vertices):
            i, k = divmod(vertex, ring)
            if k0 == ring - 1 and k == 0:
                k = ring                                     # Naht: v läuft bis 1 statt auf 0 zurück
            uv.data[loop_index].uv = (i / (len(P) - 1), k / ring)

    # zwei Arterien und eine Vene, spiralig (GEFAESS_WINDUNGEN), in der Sulze – tief genug, dass keine in einer engen
    # Schlinge durch die Oberfläche sticht
    vverts, vfaces = [], []
    for phase, rad in ((0.0, 0.13), (2.1, 0.13), (4.2, 0.19)):
        pts = [P[i] + (math.cos(phase + t[i] * 2 * GEFAESS_WINDUNGEN * math.pi) * sides[i]
                       + math.sin(phase + t[i] * 2 * GEFAESS_WINDUNGEN * math.pi) * ups[i]) * (radius[i] * 0.45)
               for i in range(len(P))]
        Q, qs, qu = frames(np.array(pts))
        vb = len(vverts)
        vr = 10
        for i in range(len(Q)):
            for k in range(vr):
                ang = 2 * math.pi * k / vr
                vverts.append(Q[i] + (math.cos(ang) * qs[i] + math.sin(ang) * qu[i]) * rad)
        for i in range(len(Q) - 1):
            for k in range(vr):
                q = vb + i * vr + k
                w = vb + i * vr + (k + 1) % vr
                vfaces.append((q, q + vr, w + vr, w))
    vessels = mesh_object(coll, "CordVessels", np.array(vverts) * CM, vfaces)
    return cord, vessels


def build():
    coll = new_collection()
    parts = build_wall_and_placenta(coll)
    parts["PlacentaVessels"] = placenta_vessels(coll)
    parts["Cord"], parts["CordVessels"] = build_cord(coll)
    print("GENESIS: Mutterleib gebaut", {k: len(v.data.polygons) for k, v in parts.items()})
    return parts


EXPORTS = {"Wall": "SM_GEN_Womb_Wall", "Placenta": "SM_GEN_Womb_Placenta", "PlacentaVessels": "SM_GEN_Womb_PlacentaVessels",
           "Cord": "SM_GEN_Womb_Cord", "CordVessels": "SM_GEN_Womb_CordVessels"}


def export_all(out_dir=OUT_DIR):
    build()
    os.makedirs(out_dir, exist_ok=True)
    for obj_name, asset in EXPORTS.items():
        obj = bpy.data.objects[obj_name]
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        path = os.path.join(out_dir, asset + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True,
                                 apply_scale_options="FBX_SCALE_UNITS", object_types={"MESH"},
                                 use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False, add_leaf_bones=False)
        print("GENESIS: exportiert", asset, round(os.path.getsize(path) / 1e6, 1), "MB")
