# GENESIS: Der Kreislauf des Lebens
# Kasackhose und Clogs der Hebamme – echte Kleidung statt einer aufgeblasenen Hülle.
#
# Aufruf (nach Tools/Unreal/Birth/midwife_export.py):
#   blender -b --factory-startup -P Tools/Blender/Birth/build_scrub_trousers.py -- --src ArtSource/Generated/Birth/Midwife [--preview]
#
# Ablauf:
#   1. Körper und Standard-Kleidungsstück (T-Shirt + Shorts) der Hebamme aus dem MetaHuman laden.
#   2. Clogs: geschlossene, abwischbare Kunststoff-Clogs, wie sie im Kreißsaal getragen werden (Arbeitsschutz: geschlossene
#      Ferse, keine Löcher – Flüssigkeiten). Aus der Fußform (Hülle 6 mm über dem Fuß) mit 28 mm Sohle.
#   3. Hose im Schnitt einer Kasackhose (gerade, weites Bein, Saum knapp über dem Boden): aus der Vereinigung der Shorts
#      (Hüfte, Gesäß, Schritt – schon auf ihren Körper geschnitten) und zweier Hosenbeine um die Beinachse, voxel-
#      neu vernetzt zu einer gleichmäßigen, geschlossenen Fläche.
#   4. Stoffsimulation (Blender Cloth): Polyester-Baumwolle 65/35, 150 g/m², Bund gehalten, Schwerkraft; Kollision mit
#      Beinen, Clogs und Shorts. Dabei fällt der Stoff, bildet Falten in der Kniekehle und staucht sich über dem Schuh.
#   5. Hautgewichte vom Körper (Beine) und den Shorts (Hüfte) übertragen, Stoffstärke 0,6 mm, Saum doppelt.
#   6. Export als Skeletal Mesh auf dem Skelett des Körpers: SK_GEN_MidwifeTrousers.fbx, SK_GEN_MidwifeClogs.fbx
#
# Koordinaten wie im FBX: Meter, +Z oben, vorn ist −Y.

import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree


def args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = {}
    key = None
    for token in argv:
        if token.startswith("--"):
            key = token[2:]
            out[key] = True
        elif key:
            out[key] = token
            key = None
    return out


ARGS = args()
SRC = os.path.abspath(ARGS.get("src", "ArtSource/Generated/Birth/Midwife"))

# Beinachse aus dem Skelett (Referenzpose): Hüftgelenk und Sprunggelenk, Meter (vorn = −Y)
HIP = Vector((0.096, -0.023, 0.840))
ANKLE = Vector((0.131, -0.001, 0.072))
WAIST_Z = 0.975      # Bund knapp unter dem Nabel – unter dem Kasack
HEM_Z = 0.030        # Saum hinten gut 3 cm über dem Boden, vorn liegt er auf dem Schuh
SOLE = 0.028         # Clog-Sohle


def log(*parts):
    print("GENESIS", *parts)


def load():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    for name in ("Body", "Outfit"):
        bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, name + ".fbx"))
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    body = next(o for o in meshes if "Body" in o.name)
    outfit = next(o for o in meshes if "Outfit" in o.name)
    armature = body.find_armature() or body.parent
    return body, outfit, armature


def world_copy(src, name, keep=None):
    """Kopie eines Meshes in Weltkoordinaten ohne Modifikatoren/Eltern; keep(face) filtert Flächen."""
    bm = bmesh.new()
    bm.from_mesh(src.data)
    bm.transform(src.matrix_world)
    if keep:
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if not keep(f)], context="FACES")
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def leg_axis(z, side):
    t = (HIP.z - z) / (HIP.z - ANKLE.z)
    t = min(1.2, max(-0.3, t))
    a = Vector((HIP.x * side, HIP.y, HIP.z))
    b = Vector((ANKLE.x * side, ANKLE.y, ANKLE.z))
    return a.lerp(b, t)


def body_leg_radius(body_ws, z, side, band=0.012):
    c = leg_axis(z, side)
    radii = [math.hypot(p.x - c.x, p.y - c.y) for p in body_ws
             if abs(p.z - z) < band and p.x * side > 0.02 and math.hypot(p.x - c.x, p.y - c.y) < 0.16]
    return max(radii) if radii else 0.0


def activate(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def apply_modifiers(obj):
    activate(obj)
    for mod in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)


def tube(name, side, profile, z_top, z_bottom, segments=48, step=0.01):
    """Hosenbein: Ringe um die Beinachse, Radius nach profile(z). Oben und unten offen."""
    bm = bmesh.new()
    rings = []
    n = int(round((z_top - z_bottom) / step))
    for i in range(n + 1):
        z = z_top - (z_top - z_bottom) * i / n
        c = leg_axis(z, side)
        r = profile(z)
        ring = [bm.verts.new((c.x + r * math.cos(2 * math.pi * k / segments),
                              c.y + r * 0.92 * math.sin(2 * math.pi * k / segments), z)) for k in range(segments)]
        rings.append(ring)
    for a, b in zip(rings[:-1], rings[1:]):
        for k in range(segments):
            bm.faces.new((a[k], a[(k + 1) % segments], b[(k + 1) % segments], b[k]))
    # oben geschlossen, damit die Vereinigung ein Volumen ist
    bm.faces.new(list(reversed(rings[0])))
    bm.faces.new(rings[-1])
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def build_clogs(body):
    """
    Clogs: fester, glatter Kunststoff – kein Abdruck des Fußes. Je Fuß die konvexe Hülle des Fußes (Zehen und Gewölbe
    verschwinden darin), 9 mm Wandstärke, vorn eine hohe Zehenkappe, darunter 28 mm Sohle; vernetzt und geglättet.
    Oben ein Einstieg, die Ferse bleibt geschlossen.
    """
    feet = world_copy(body, "Feet", keep=lambda f: max(v.co.z for v in f.verts) < 0.105)
    bm = bmesh.new()
    bm.from_mesh(feet.data)
    bpy.data.objects.remove(feet)
    result = bmesh.new()
    for side in (1, -1):
        points = [v.co.copy() for v in bm.verts if v.co.x * side > 0.0]
        part = bmesh.new()
        for p in points:
            part.verts.new(p)
        hull = bmesh.ops.convex_hull(part, input=part.verts)
        bmesh.ops.delete(part, geom=[v for v in hull.get("geom_interior", []) if isinstance(v, bmesh.types.BMVert)], context="VERTS")
        bmesh.ops.subdivide_edges(part, edges=part.edges, cuts=3, use_grid_fill=True)
        part.normal_update()
        for v in part.verts:
            toe = max(0.0, min(1.0, (-v.co.y - 0.06) / 0.07))
            v.co += v.normal * (0.009 + 0.006 * toe)
            if v.co.z < 0.02:
                v.co.z -= SOLE * (1.0 - 0.3 * max(0.0, min(1.0, (-v.co.y - 0.17) / 0.07)))
            elif toe > 0.0:
                v.co.z += 0.012 * toe
        mesh = bpy.data.meshes.new("tmp")
        part.to_mesh(mesh)
        part.free()
        result.from_mesh(mesh)
        bpy.data.meshes.remove(mesh)
    bm.free()
    mesh = bpy.data.meshes.new("SK_GEN_MidwifeClogs")
    result.to_mesh(mesh)
    result.free()
    clogs = bpy.data.objects.new("SK_GEN_MidwifeClogs", mesh)
    bpy.context.scene.collection.objects.link(clogs)
    remesh = clogs.modifiers.new("Geschlossen", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = 0.005
    smooth = clogs.modifiers.new("Glatt", "SMOOTH")
    smooth.factor = 1.0
    smooth.iterations = 12
    apply_modifiers(clogs)
    # Einstieg: oben über dem Rist offen, Zehenkappe und Ferse bleiben
    bm = bmesh.new()
    bm.from_mesh(clogs.data)
    cut = []
    for f in bm.faces:
        c = f.calc_center_median()
        front = -c.y
        # nur der Knöchelausschnitt: Rist und Zehen bleiben bedeckt (ein Clog hat eine hohe Kappe)
        if c.z > 0.068 and -0.03 < front < 0.045 and f.normal.z > 0.35:
            cut.append(f)
    bmesh.ops.delete(bm, geom=cut, context="FACES")
    bm.to_mesh(clogs.data)
    bm.free()
    clogs.location.z += SOLE
    apply_transform(clogs)
    clogs.data.materials.append(bpy.data.materials.new("Clog"))
    return clogs


def apply_transform(obj):
    activate(obj)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


def surface_radius(tree, center, angle, z, reach):
    """Abstand der äußersten Fläche (Körper/Shorts) vom Mittelpunkt in Richtung angle auf Höhe z (Strahl von außen)."""
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    origin = Vector((center.x, center.y, z)) + direction * reach
    hit, _, _, dist = tree.ray_cast(origin, -direction, reach)
    return reach - dist if hit is not None else 0.0


def build_trousers(body, outfit):
    """
    Schnitt einer Kasackhose als ein Stück: zwei Hosenbeine (Ringe um die Beinachse), im Schritt über einen Steg
    verbunden, darüber Ringe um das Becken bis zum Bund. Jeder Punkt liegt auf der gemessenen Oberfläche von Körper
    und Shorts plus Bewegungszugabe; die Beine fallen gerade (Weite am Saum ~50 cm). Reine Vierecke.
    """
    shorts = world_copy(outfit, "ShortsShape", keep=lambda f: f.material_index % 2 == 1)
    legs = world_copy(body, "LegsShape", keep=lambda f: max(v.co.z for v in f.verts) < 0.80)
    bm = bmesh.new()
    for src in (shorts, legs):
        bm.from_mesh(src.data)
    tree = BVHTree.FromBMesh(bm)
    bm.free()
    for src in (shorts, legs):
        bpy.data.objects.remove(src)

    # Schritt: von unten zwischen den Beinen nach oben bis an die Shorts
    hit, _, _, dist = tree.ray_cast(Vector((0.0, -0.02, 0.45)), Vector((0.0, 0.0, 1.0)), 0.6)
    crotch = (0.45 + dist) if hit is not None else 0.74
    z_c = crotch - 0.012
    log("Schritt bei %.3f m" % crotch)

    n_out, k_in = 36, 12          # Außenbogen (vorn-innen über außen nach hinten-innen), Innenbogen
    front_inner, back_inner = math.radians(-100.0), math.radians(100.0)   # vorn = -Y

    def leg_angles(side):
        # Winkel um die Beinachse; am linken Bein (+X) liegt außen bei 0°, innen bei 180°
        outer = [front_inner + (back_inner - front_inner) * i / (n_out - 1) for i in range(n_out)]
        span = 2 * math.pi - (back_inner - front_inner)
        inner = [back_inner + span * (k + 1) / (k_in + 1) for k in range(k_in)]
        return [(math.pi - a) if side < 0 else a for a in outer + inner]

    def smooth01(a, b, x):
        t = max(0.0, min(1.0, (x - a) / (b - a)))
        return t * t * (3.0 - 2.0 * t)

    def leg_point(side, angle, z):
        c = leg_axis(z, side)
        skin = surface_radius(tree, c, angle, z, 0.16)
        # Zum Schritt hin liegt das Bein wie das Becken an (6 mm über den Shorts), darunter fällt es weit und gerade.
        # Ohne diesen Übergang stand oben ein Absatz über, an dem man innen die Haut sah.
        top = smooth01(z_c - 0.14, z_c - 0.01, z)
        ease = 0.022 * (1.0 - top) + 0.019 * top
        straight = (0.088 if z < 0.40 else 0.088 + (z - 0.40) * 0.08) * (1.0 - top)
        # Innen zwischen den Beinen etwas weniger Zugabe, sonst schneiden sich die Hosenbeine
        inward = max(0.0, -math.cos(angle) * side)
        r = max(skin + ease - 0.004 * inward, straight * (1.0 - 0.25 * inward))
        return Vector((c.x + r * math.cos(angle), c.y + r * 0.95 * math.sin(angle), z))

    bm = bmesh.new()
    step = 0.012
    count = int((z_c - HEM_Z) / step)
    zs_leg = [HEM_Z + (z_c - HEM_Z) * i / count for i in range(count + 1)]
    rings = {}
    for side in (1, -1):
        angles = leg_angles(side)
        rings[side] = [[bm.verts.new(leg_point(side, a, z)) for a in angles] for z in zs_leg]
    n_leg = n_out + k_in
    for side in (1, -1):
        for i in range(len(zs_leg) - 1):
            lo, hi = rings[side][i], rings[side][i + 1]
            for k in range(n_leg):
                k2 = (k + 1) % n_leg
                bm.faces.new((lo[k], lo[k2], hi[k2], hi[k]))

    # Schrittsteg: die Innenbögen beider Beine auf Höhe des Schritts verbinden
    left_top, right_top = rings[1][-1], rings[-1][-1]
    left_inner = [left_top[n_out - 1]] + left_top[n_out:] + [left_top[0]]
    right_inner = [right_top[n_out - 1]] + right_top[n_out:] + [right_top[0]]
    for k in range(len(left_inner) - 1):
        quad = (left_inner[k], left_inner[k + 1], right_inner[k + 1], right_inner[k])
        if len(set(quad)) == 4:
            try:
                bm.faces.new(quad)
            except ValueError:
                pass
    # Becken: Außenbögen beider Beine zu einem Ring, darüber bis zum Bund – Form aus den Shorts
    base = left_top[:n_out] + list(reversed(right_top[:n_out]))
    center = Vector((0.0, -0.01, 0.0))
    prev = base
    z = z_c
    while z < WAIST_Z - 1e-4:
        z = min(WAIST_Z, z + step)
        blend = min(1.0, (z - z_c) / 0.11)
        ring = []
        for v0 in base:
            angle = math.atan2(v0.co.y - center.y, v0.co.x - center.x)
            r = surface_radius(tree, center, angle, z, 0.40) + 0.018
            target = Vector((center.x + r * math.cos(angle), center.y + r * math.sin(angle), z))
            ring.append(bm.verts.new(Vector((v0.co.x, v0.co.y, z)).lerp(target, blend)))
        for k in range(len(ring)):
            k2 = (k + 1) % len(ring)
            try:
                bm.faces.new((prev[k], prev[k2], ring[k2], ring[k]))
            except ValueError:
                pass
        prev = ring
    # Löcher im Schritt schließen (Randkanten zwischen Saum und Bund)
    holes = [e for e in bm.edges if e.is_boundary and 0.3 < (e.verts[0].co.z + e.verts[1].co.z) / 2 < WAIST_Z - 0.01]
    if holes:
        bmesh.ops.holes_fill(bm, edges=holes, sides=0)
        log("Schritt: %d Randkanten geschlossen" % len(holes))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new("SK_GEN_MidwifeTrousers")
    bm.to_mesh(mesh)
    bm.free()
    pants = bpy.data.objects.new("SK_GEN_MidwifeTrousers", mesh)
    bpy.context.scene.collection.objects.link(pants)
    # UV: um Bein/Becken und in der Höhe – der Köper braucht eine Laufrichtung
    activate(pants)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cylinder_project(direction="ALIGN_TO_OBJECT", align="POLAR_ZX", scale_to_bounds=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    log("Hose: %d Punkte, %d Flächen" % (len(mesh.vertices), len(mesh.polygons)))
    return pants


def simulate(pants, colliders, frames=45):
    """Stoff fallen lassen. Der Bund bleibt, wo er ist; über der Hüfte wird der Stoff weich gehalten."""
    group = pants.vertex_groups.new(name="Bund")
    for v in pants.data.vertices:
        w = max(0.0, min(1.0, (v.co.z - (WAIST_Z - 0.12)) / 0.09))
        # Schritt: gehalten, sonst hängt er wie bei einer Haremshose
        if abs(v.co.x) < 0.05 and v.co.z > 0.70:
            w = max(w, 0.6)
        if w > 0.0:
            group.add([v.index], w, "REPLACE")
    cloth = pants.modifiers.new("Stoff", "CLOTH")
    s = cloth.settings
    s.quality = 10
    # Blenders Stoffwerte sind nicht in SI skaliert; die Baumwoll-Voreinstellung (0,3 / 15 / 0,5) verhält sich wie
    # mittelschwerer Köper. Polyester-Baumwolle ist etwas steifer in der Fläche, weich in der Biegung.
    s.mass = 0.3
    s.air_damping = 1.0
    s.tension_stiffness = 18.0
    s.compression_stiffness = 18.0
    s.shear_stiffness = 12.0
    s.bending_stiffness = 2.0
    s.tension_damping = 5.0
    s.bending_damping = 0.5
    s.pin_stiffness = 1.0
    s.vertex_group_mass = "Bund"
    s.shrink_min = 0.0
    cloth.collision_settings.use_collision = True
    cloth.collision_settings.distance_min = 0.004
    cloth.collision_settings.use_self_collision = True
    cloth.collision_settings.self_distance_min = 0.003
    cloth.point_cache.frame_start = 1
    cloth.point_cache.frame_end = frames
    for obj in colliders:
        col = obj.modifiers.new("Kollision", "COLLISION")
        obj.collision.thickness_outer = 0.004
        obj.collision.cloth_friction = 12.0
    scene = bpy.context.scene
    scene.frame_start = 1
    scene.frame_end = frames
    for frame in range(1, frames + 1):
        scene.frame_set(frame)
    activate(pants)
    bpy.ops.object.modifier_apply(modifier="Stoff")
    log("Stoff simuliert: %d Bilder" % frames)


def keep_outside(cloth, skin, gap, z_min=None):
    """
    Kein Punkt des Stoffes liegt näher als gap an der Haut oder in ihr – sonst sticht die Haut durch. Nach dem Glätten
    (Unterteilung) zog die Falte im Schritt den Stoff einige Millimeter in die Oberschenkel.
    """
    bm = bmesh.new()
    bm.from_mesh(skin.data)
    bm.normal_update()
    tree = BVHTree.FromBMesh(bm)
    bm.free()
    moved = 0
    for v in cloth.data.vertices:
        if z_min is not None and v.co.z < z_min:
            continue
        location, normal, _, dist = tree.find_nearest(v.co, 0.03)
        if location is None:
            continue
        depth = (v.co - location).dot(normal)
        if depth < gap:
            v.co += normal * (gap - depth)
            moved += 1
    log("Stoff aus der Haut gehoben: %d Punkte" % moved)


def tuck_under_shirt(pants, cover):
    """
    Der Bund liegt unter dem Kasack: Wo der Kasack (cover, gleiche Koordinaten wie die Hose) die Hose überdeckt, bleibt
    jeder Punkt der Hose 4 mm innerhalb seiner Fläche (Strahl von außen zur Körpermitte). Sonst sticht sie durch den Saum.
    """
    bm = bmesh.new()
    bm.from_mesh(cover.data)
    tree = BVHTree.FromBMesh(bm)
    hem = min(v.co.z for v in bm.verts)
    bm.free()
    moved = 0
    for v in pants.data.vertices:
        if v.co.z < hem - 0.002:
            continue
        flat = Vector((v.co.x, v.co.y + 0.01, 0.0))
        if flat.length < 1e-4:
            continue
        direction = flat.normalized()
        origin = Vector((0.0, -0.01, v.co.z)) + direction * 0.45
        hit, _, _, dist = tree.ray_cast(origin, -direction, 0.45)
        if hit is None:
            continue
        limit = 0.45 - dist - 0.004
        if flat.length > limit:
            v.co.x, v.co.y = direction.x * limit, direction.y * limit - 0.01
            moved += 1
    log("Bund unter den Kasack: %d Punkte (Saum des Kasacks bei %.3f m)" % (moved, hem - SOLE))


def skin(target, sources, armature, legs_from=None, legs_top=0.76, blend=0.05):
    """
    Hautgewichte der nächstgelegenen Fläche übernehmen und an das Skelett binden. sources[0] liefert die Grundgewichte;
    legs_from (der Körper) überschreibt sie an den Beinen bis legs_top – dort muss der Stoff exakt wie die Haut darunter
    mitgehen, sonst sticht die Haut bei jeder Hüftbeugung durch (so geschehen mit Gewichten aus den Shorts).
    """
    passes = [(src, None) for src in sources]
    if legs_from is not None:
        group = target.vertex_groups.new(name="_Beine")
        for v in target.data.vertices:
            w = max(0.0, min(1.0, (legs_top + blend - v.co.z) / blend))
            if w > 0.0:
                group.add([v.index], w, "REPLACE")
        passes.append((legs_from, "_Beine"))
    for index, (src, mask) in enumerate(passes):
        dt = target.modifiers.new("Gewichte_%d" % index, "DATA_TRANSFER")
        dt.object = src
        dt.use_vert_data = True
        dt.data_types_verts = {"VGROUP_WEIGHTS"}
        dt.vert_mapping = "POLYINTERP_NEAREST"
        dt.layers_vgroup_select_src = "ALL"
        dt.layers_vgroup_select_dst = "NAME"
        dt.mix_mode = "REPLACE" if index == 0 or mask else "ADD"
        if mask:
            dt.vertex_group = mask
        activate(target)
        bpy.ops.object.datalayout_transfer(modifier=dt.name)
        bpy.ops.object.modifier_apply(modifier=dt.name)
    for helper in ("_Beine",):
        if target.vertex_groups.get(helper):
            target.vertex_groups.remove(target.vertex_groups[helper])
    activate(target)
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    bpy.ops.object.vertex_group_limit_total(limit=8)
    # Eltern setzen, ohne die Lage zu verändern (das Skelett aus dem FBX trägt eine eigene Drehung/Skalierung)
    world = target.matrix_world.copy()
    target.parent = armature
    target.matrix_parent_inverse = armature.matrix_world.inverted()
    target.matrix_world = world
    mod = target.modifiers.new("Skelett", "ARMATURE")
    mod.object = armature


def preview(objects):
    """Formkontrolle: Workbench-Bilder von vorn, seitlich und schräg hinten (Knie, Saum, Schuh)."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "OBJECT"
    scene.display.shading.show_cavity = True
    scene.display.shading.show_shadows = True
    scene.render.resolution_x, scene.render.resolution_y = 900, 1200
    colors = {"Body": (0.75, 0.55, 0.45, 1), "Outfit": (0.1, 0.5, 0.55, 1), "Trousers": (0.12, 0.45, 0.5, 1), "Clogs": (0.9, 0.9, 0.92, 1), "Tunic": (0.12, 0.45, 0.5, 1)}
    for obj in objects:
        obj.color = next((c for k, c in colors.items() if k in obj.name), (0.6, 0.6, 0.6, 1))
    for name, (loc, target) in {"front": ((0.0, -2.2, 0.6), (0.0, 0.0, 0.5)), "side": ((2.2, 0.0, 0.6), (0.0, 0.0, 0.5)),
                                "back": ((-1.2, 1.9, 0.5), (0.0, 0.0, 0.45)), "hem": ((0.5, -0.9, 0.25), (0.08, 0.0, 0.12)),
                                "top": ((0.35, -1.6, 1.35), (0.0, 0.0, 1.12)), "topside": ((1.8, -0.4, 1.2), (0.0, 0.0, 1.05))}.items():
        cam = bpy.data.objects.new(name, bpy.data.cameras.new(name))
        scene.collection.objects.link(cam)
        cam.location = loc
        cam.rotation_euler = (Vector(target) - Vector(loc)).to_track_quat("-Z", "Y").to_euler()
        cam.data.lens = 50
        scene.camera = cam
        scene.render.filepath = os.path.join(SRC, "preview_%s.png" % name)
        bpy.ops.render.render(write_still=True)
        log("Vorschau", scene.render.filepath)


TUNIC_HEM_Z = 0.785      # Kasack bis über die Hüfte – der Hosenbund liegt darunter


def build_tunic(outfit):
    """
    Kasack (Rundhals, Schlupfform) = das T-Shirt des MetaHuman, verlängert. Schultern, Ärmel und Kragen bleiben dessen
    Geometrie mit allen Hilfsknochen-Gewichten – sie verformen sich beim Heben der Arme richtig (ein eigener Schnitt aus
    Ringen riss dort auf; ein angesetzter Schoß wirkte wie ein Gürtel). Der eingerollte Saum wird abgeschnitten, beide
    Stofflagen laufen gerade bis über die Hüfte weiter und werden unten zu einem neuen Saum geschlossen. Jeder neue Punkt
    trägt die Gewichte des Saumpunkts, von dem er ausgeht. Dazu die aufgesetzte Brusttasche links.
    Enthält MetaHuman-Geometrie: Das Ergebnis bleibt außerhalb des öffentlichen Repos.
    """
    tunic = outfit.copy()
    tunic.data = outfit.data.copy()
    tunic.name = "SK_GEN_MidwifeTunic"
    bpy.context.scene.collection.objects.link(tunic)
    world = tunic.matrix_world.copy()
    tunic.parent = None
    tunic.data.transform(world)
    tunic.matrix_world = Matrix.Identity(4)
    for mod in list(tunic.modifiers):
        tunic.modifiers.remove(mod)

    shorts = world_copy(outfit, "ShortsShape", keep=lambda f: f.material_index % 2 == 1)
    sb = bmesh.new()
    sb.from_mesh(shorts.data)
    for v in sb.verts:
        v.co += v.normal * 0.02            # die Hose liegt 2 cm über den Shorts
    shorts_tree = BVHTree.FromBMesh(sb)
    sb.free()
    bpy.data.objects.remove(shorts)

    bm = bmesh.new()
    bm.from_mesh(tunic.data)
    deform = bm.verts.layers.deform.active
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index % 2 == 1], context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0002)
    hem = min(v.co.z for v in bm.verts)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.calc_center_median().z < hem + 0.014], context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")

    # Randschleifen unten (außen und innen)
    boundary = [e for e in bm.edges if e.is_boundary]
    loops, seen = [], set()
    for start in boundary:
        if start in seen:
            continue
        chain, stack = [], [start]
        while stack:
            e = stack.pop()
            if e in seen:
                continue
            seen.add(e)
            chain.append(e)
            for v in e.verts:
                stack.extend(x for x in v.link_edges if x.is_boundary and x not in seen)
        loops.append(chain)
    loops = [l for l in loops if sum((e.verts[0].co.z + e.verts[1].co.z) / 2 for e in l) / len(l) < hem + 0.06]
    front = min(v.co.y for v in bm.verts if abs(v.co.x) < 0.01 and v.co.z < 1.0)
    back = max(v.co.y for v in bm.verts if abs(v.co.x) < 0.01 and v.co.z < 1.0)
    cy = 0.5 * (front + back)

    def ordered(loop):
        verts = {v for e in loop for v in e.verts}
        return sorted(verts, key=lambda v: math.atan2(v.co.y - cy, v.co.x))

    rings = []
    for loop in loops:
        verts = ordered(loop)
        radius = sum(math.hypot(v.co.x, v.co.y - cy) for v in verts) / len(verts)
        rings.append((radius, verts))
    rings.sort(key=lambda r: -r[0])          # außen zuerst
    log("Kasack: %d Saumschleifen (Radien %s)" % (len(rings), ", ".join("%.3f" % r for r, _ in rings)))
    outer, inner = rings[0][1], rings[-1][1]
    z0 = max(v.co.z for v in outer)
    zs = []
    z = z0 - 0.010
    while z > TUNIC_HEM_Z:
        zs.append(z)
        z -= 0.010
    zs.append(TUNIC_HEM_Z)

    def extend(start, inward):
        columns = []
        previous = [math.hypot(v.co.x, v.co.y - cy) for v in start]
        rows = [start]
        for z in zs:
            row = []
            for k, v in enumerate(start):
                angle = math.atan2(v.co.y - cy, v.co.x)
                need = surface_radius(shorts_tree, Vector((0.0, cy, z)), angle, z, 0.40) + 0.012 - inward
                # sanft aus dem Saum heraus: höchstens 3 mm je Zentimeter nach außen, nie enger (der Stoff fällt gerade)
                r = max(previous[k], min(need, previous[k] + 0.003))
                previous[k] = r
                nv = bm.verts.new((r * math.cos(angle), cy + r * math.sin(angle), z))
                if deform is not None:
                    for group, weight in v[deform].items():
                        nv[deform][group] = weight
                row.append(nv)
            rows.append(row)
        for a, b in zip(rows[:-1], rows[1:]):
            for k in range(len(a)):
                k2 = (k + 1) % len(a)
                try:
                    bm.faces.new((a[k], a[k2], b[k2], b[k]))
                except ValueError:
                    pass
        return rows[-1]

    bottom_outer = extend(outer, 0.0)
    bottom_inner = extend(inner, 0.0025) if inner is not outer else None
    if bottom_inner is not None and len(bottom_inner) == len(bottom_outer):
        for k in range(len(bottom_outer)):
            k2 = (k + 1) % len(bottom_outer)
            try:
                bm.faces.new((bottom_outer[k], bottom_outer[k2], bottom_inner[k2], bottom_inner[k]))
            except ValueError:
                pass
    elif bottom_inner is not None:
        edges = [e for e in bm.edges if e.is_boundary and max(v.co.z for v in e.verts) < TUNIC_HEM_Z + 0.001]
        bmesh.ops.bridge_loops(bm, edges=edges)

    # Brusttasche links (+X = ihre linke Seite), 3 mm vor dem Stoff
    tree = BVHTree.FromBMesh(bm)
    pocket = []
    for i in range(9):
        row = []
        for k in range(9):
            x = 0.045 + 0.11 * k / 8
            z = 1.085 + 0.13 * i / 8
            hit = tree.ray_cast(Vector((x, -0.40, z)), Vector((0.0, 1.0, 0.0)), 0.8)
            src = hit[0]
            row.append(bm.verts.new((x, (src.y - 0.003) if src is not None else -0.12, z)))
        pocket.append(row)
    for i in range(8):
        for k in range(8):
            bm.faces.new((pocket[i][k], pocket[i][k + 1], pocket[i + 1][k + 1], pocket[i + 1][k]))
    for f in bm.faces:
        f.material_index = 0
        f.smooth = True
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    pocket_count = 81
    bm.to_mesh(tunic.data)
    bm.free()
    # Die Tasche (die letzten Punkte) bekommt ihre Gewichte später vom Stoff darunter
    group = tunic.vertex_groups.new(name="_Tasche")
    count = len(tunic.data.vertices)
    group.add(list(range(count - pocket_count, count)), 1.0, "REPLACE")
    while len(tunic.data.materials) > 1:
        tunic.data.materials.pop()
    log("Kasack: aus dem T-Shirt verlängert bis %.3f m (Saum war %.3f m), %d Punkte" % (TUNIC_HEM_Z, hem, len(tunic.data.vertices)))
    return tunic


def bind_tunic(tunic, outfit, armature):
    """Der Kasack trägt die Gewichte des T-Shirts schon; nur die Tasche übernimmt sie vom Stoff darunter."""
    dt = tunic.modifiers.new("Tasche", "DATA_TRANSFER")
    dt.object = outfit
    dt.use_vert_data = True
    dt.data_types_verts = {"VGROUP_WEIGHTS"}
    dt.vert_mapping = "POLYINTERP_NEAREST"
    dt.layers_vgroup_select_src = "ALL"
    dt.layers_vgroup_select_dst = "NAME"
    dt.mix_mode = "REPLACE"
    dt.vertex_group = "_Tasche"
    activate(tunic)
    bpy.ops.object.datalayout_transfer(modifier=dt.name)
    bpy.ops.object.modifier_apply(modifier=dt.name)
    tunic.vertex_groups.remove(tunic.vertex_groups["_Tasche"])
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    world = tunic.matrix_world.copy()
    tunic.parent = armature
    tunic.matrix_parent_inverse = armature.matrix_world.inverted()
    tunic.matrix_world = world
    mod = tunic.modifiers.new("Skelett", "ARMATURE")
    mod.object = armature


def main():
    body, outfit, armature = load()
    clogs = build_clogs(body)
    log("Clogs: %d Punkte" % len(clogs.data.vertices))
    trousers = build_trousers(body, outfit)
    log("Hose vor dem Fall: %d Punkte" % len(trousers.data.vertices))
    legs = world_copy(body, "LegsCollider", keep=lambda f: max(v.co.z for v in f.verts) < 0.80)
    legs.location.z += SOLE
    apply_transform(legs)
    shorts = world_copy(outfit, "ShortsCollider", keep=lambda f: f.material_index % 2 == 1)
    shorts.location.z += SOLE
    apply_transform(shorts)
    trousers.location.z += SOLE
    apply_transform(trousers)
    simulate(trousers, [legs, clogs, shorts])

    # Kasack: das verlängerte T-Shirt des MetaHuman; der Bund der Hose liegt darunter
    tunic = build_tunic(outfit)
    tunic.location.z += SOLE
    apply_transform(tunic)
    tuck_under_shirt(trousers, tunic)
    # Die Naht zwischen Bein und Becken glätten: Ein echter Hosenschnitt hat dort keine Kante
    band = trousers.vertex_groups.new(name="_Naht")
    for v in trousers.data.vertices:
        w = max(0.0, 1.0 - abs(v.co.z - (0.735 + SOLE)) / 0.05)
        if w > 0.0:
            band.add([v.index], w, "REPLACE")
    relax = trousers.modifiers.new("Naht", "SMOOTH")
    relax.factor = 0.8
    relax.iterations = 12
    relax.vertex_group = "_Naht"
    apply_modifiers(trousers)
    if trousers.vertex_groups.get("_Naht"):
        trousers.vertex_groups.remove(trousers.vertex_groups["_Naht"])
    # Nach dem Fall glätten (eine Unterteilung), dann Stoffstärke
    sub = trousers.modifiers.new("Fein", "SUBSURF")
    sub.levels = 1
    solid = trousers.modifiers.new("Stoff", "SOLIDIFY")
    solid.thickness = 0.0012
    solid.offset = 1.0
    apply_modifiers(trousers)
    keep_outside(trousers, legs, 0.005)
    trousers.data.materials.append(bpy.data.materials.new("ScrubFabric"))
    for obj in (trousers, clogs):
        for poly in obj.data.polygons:
            poly.use_smooth = True
    # Die Frau steht auf 28 mm Sohle: Hose und Clogs sitzen darauf; das Skelett hebt die Regie im Spiel
    # (Hebamme 28 mm höher) – hier wieder zurück in die Referenzpose des Skeletts
    for obj in (trousers, clogs, tunic):
        obj.location.z -= SOLE
        apply_transform(obj)
    for obj in (legs, shorts):
        bpy.data.objects.remove(obj)
    skin(trousers, [outfit], armature, legs_from=body)
    skin(clogs, [body], armature)
    bind_tunic(tunic, outfit, armature)
    if "preview" in ARGS:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(SRC, "scrubs_preview.blend"))
        # Der Kasack ersetzt das T-Shirt
        outfit.hide_render = True
        preview([body, trousers, clogs, tunic])
    for obj in (trousers, clogs, tunic):
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        armature.select_set(True)
        bpy.context.view_layer.objects.active = armature
        path = os.path.join(SRC, obj.name + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={"ARMATURE", "MESH"},
                                 add_leaf_bones=False, use_armature_deform_only=False, bake_anim=False,
                                 mesh_smooth_type="FACE", apply_unit_scale=True)
        log("exportiert %s (%d Punkte)" % (os.path.basename(path), len(obj.data.vertices)))


main()
