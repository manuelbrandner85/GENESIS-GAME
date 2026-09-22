# GENESIS: Der Kreislauf des Lebens
# Baut die Oberfläche der Gebärmutterschleimhaut im Implantationsfenster (SM_GEN_Endometrium) aus anatomischen Maßen.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Implantation/build_endometrium.py -- --out ArtSource/Generated/Implantation [--export] [--render]
#
# Anatomie (Sekretionsphase, Zyklustag 20–24, Endometrium 8–14 mm dick; Histologie und Hysteroskopie):
#   - Oberfläche: einschichtiges Säulenepithel (Zellen 5–10 µm breit) mit Pinopoden – beides zu fein für Geometrie,
#     kommt ins Material.
#   - Das Stroma ist ödematös aufgequollen: flache Polster (Durchmesser 0,4–0,9 mm, 20–45 µm hoch), dazwischen seichte Furchen.
#   - Drüsenöffnungen: 30–55 µm weit, im Mittel ~180 µm auseinander, trichterförmig; die Drüse läuft darunter weiter
#     (gewunden, Sekret), sichtbar ist nur ihr Eingang. Leichter Wulst am Rand.
#   - Die Einnistungsstelle (Mitte) liegt auf einem Polster und frei von Drüsenöffnungen im Umkreis des Keims.
# Maßstab: 1 µm = 0,01 m (Blender) = 1 cm (Unreal). Z zeigt ins Cavum uteri (zur Kamera).
#
# Aufbau: Höhenfeld auf einem Gitter, das in der Mitte 4 µm fein ist und nach außen bis 16 µm gröber wird
# (Nanite; der Rand liegt weit außerhalb des scharfen Bildbereichs). Farbattribut für das Material:
#   R = Tiefe in einer Drüsenöffnung (0..1), G = Furche (0..1), B = Polsterhöhe (0..1), A = Kennwert der Drüse (Sekret).

import math
import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Conception"))
from genesis_blender_common import (UM, configure_color_management, configure_cycles, enable_gpu, ensure_dir, measure_exposure, new_node,
                                    reset_scene, script_args, smoothstep)
import bpy
import numpy as np
from mathutils import Vector

HALF_EXTENT = 4000.0        # ±4 mm: Rand weit außerhalb des Bildes
FINE_STEP = 4.0             # µm, in der Mitte
FINE_RADIUS = 1300.0        # bis hierhin fein
COARSE_STEP = 16.0          # µm, am Rand
SITE_CLEAR_RADIUS = 330.0   # keine Drüsenöffnung, wo der Keim liegt
GLAND_SPACING = 175.0      # mittlerer Abstand; ausgedünnt nach Feldern (siehe build_heightfield)

rng = np.random.default_rng(20260922)


def axis_samples():
    """Stützstellen je Achse: fein in der Mitte, zum Rand linear gröber."""
    positions = [0.0]
    x = 0.0
    while x < HALF_EXTENT:
        t = smoothstep(FINE_RADIUS, HALF_EXTENT, x)
        x += FINE_STEP + (COARSE_STEP - FINE_STEP) * t
        positions.append(min(x, HALF_EXTENT))
    half = np.array(positions)
    return np.concatenate([-half[:0:-1], half])


def smoothstep_np(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def value_noise_2d(x, y, cell, seed):
    """Glattes Wertrauschen (kubisch interpoliert) – reproduzierbar, ohne externe Bibliotheken."""
    local = np.random.default_rng(seed)
    size = int(2 * HALF_EXTENT / cell) + 4
    grid = local.random((size, size))
    gx = (x + HALF_EXTENT) / cell + 1.0
    gy = (y + HALF_EXTENT) / cell + 1.0
    ix = np.floor(gx).astype(int)
    iy = np.floor(gy).astype(int)
    fx = gx - ix
    fy = gy - iy
    sx = fx * fx * (3 - 2 * fx)
    sy = fy * fy * (3 - 2 * fy)
    ix = np.clip(ix, 0, size - 2)
    iy = np.clip(iy, 0, size - 2)
    a = grid[iy, ix]
    b = grid[iy, ix + 1]
    c = grid[iy + 1, ix]
    d = grid[iy + 1, ix + 1]
    return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy


def fbm(x, y, cell, octaves, seed, gain=0.5):
    total = np.zeros_like(x)
    amplitude = 1.0
    norm = 0.0
    for octave in range(octaves):
        total += amplitude * value_noise_2d(x, y, cell / (2 ** octave), seed + octave * 101)
        norm += amplitude
        amplitude *= gain
    return total / norm


def poisson_points(spacing, extent, seed, exclude_center=0.0):
    """Punkte mit Mindestabstand (Dart Throwing auf einem Hilfsgitter)."""
    local = np.random.default_rng(seed)
    cell = spacing / math.sqrt(2.0)
    count = int(2 * extent / cell) + 1
    grid = -np.ones((count, count), dtype=int)
    points = []
    attempts = int((2 * extent / spacing) ** 2 * 6)
    for _ in range(attempts):
        p = local.uniform(-extent, extent, 2)
        if math.hypot(p[0], p[1]) < exclude_center:
            continue
        gx = int((p[0] + extent) / cell)
        gy = int((p[1] + extent) / cell)
        ok = True
        for ny in range(max(0, gy - 2), min(count, gy + 3)):
            for nx in range(max(0, gx - 2), min(count, gx + 3)):
                index = grid[ny, nx]
                if index >= 0 and math.hypot(points[index][0] - p[0], points[index][1] - p[1]) < spacing:
                    ok = False
                    break
            if not ok:
                break
        if ok:
            grid[gy, gx] = len(points)
            points.append(p)
    return np.array(points)


def build_heightfield():
    xs = axis_samples()
    X, Y = np.meshgrid(xs, xs)
    R = np.hypot(X, Y)
    print("GENESIS: Gitter", X.shape, "=", X.size, "Punkte, Schritt Mitte", FINE_STEP, "Rand", COARSE_STEP)

    # Polster: ödematöses Stroma wölbt die Oberfläche in flachen Kissen
    cushion = fbm(X, Y, 700.0, 3, 11)
    height = 70.0 * (cushion - 0.5)

    # Furchen zwischen den Polstern: Kanten eines Voronoi-Musters (F2 − F1), mal tiefer, mal ausgelaufen
    # Die Koordinaten werden vorher verwunden: Echte Furchen laufen geschwungen, nie gerade von Knoten zu Knoten
    seeds = poisson_points(620.0, HALF_EXTENT + 700.0, 23)
    WX = X + 170.0 * (fbm(X, Y, 520.0, 3, 91) - 0.5) + 45.0 * (fbm(X, Y, 130.0, 2, 93) - 0.5)
    WY = Y + 170.0 * (fbm(X, Y, 520.0, 3, 97) - 0.5) + 45.0 * (fbm(X, Y, 130.0, 2, 99) - 0.5)
    best1 = np.full(X.shape, 1e9)
    best2 = np.full(X.shape, 1e9)
    for sx, sy in seeds:
        d = np.hypot(WX - sx, WY - sy)
        closer = d < best1
        best2 = np.where(closer, best1, np.minimum(best2, d))
        best1 = np.where(closer, d, best1)
    edge = best2 - best1
    strength = np.clip(fbm(X, Y, 900.0, 2, 37) * 1.6 - 0.35, 0.0, 1.0)
    # Breite schwankt entlang der Furche; der Querschnitt ist weich (gequollenes Gewebe hat keine Kanten)
    width = 45.0 + 35.0 * fbm(X, Y, 300.0, 2, 41)
    furrow = np.exp(-(edge / width) ** 2) * strength
    height -= 24.0 * furrow

    # Feines Quellrelief (Ödem, darunterliegende Kapillaren und Drüsenschläuche)
    height += 6.0 * (fbm(X, Y, 110.0, 3, 51) - 0.5)
    height += 1.6 * (fbm(X, Y, 28.0, 2, 67) - 0.5)

    # Einnistungsstelle: auf einem sanften Polster, dort ohne Furche
    site = 1.0 - smoothstep_np(200.0, 700.0, R)
    height = height * (1.0 - 0.6 * site) + 14.0 * np.exp(-(R / 520.0) ** 2)
    furrow *= 1.0 - site

    # Drüsenöffnungen
    gland_depth = np.zeros_like(X)
    gland_id = np.zeros_like(X)
    # Nicht gleichmäßig wie ein Golfball: Drüsen stehen in dichteren und lichteren Feldern (Dichte aus Rauschen),
    # Größe und Form streuen stark, manche Öffnungen sind schlitzförmig
    candidates = poisson_points(GLAND_SPACING * 0.8, HALF_EXTENT - 40.0, 71, exclude_center=SITE_CLEAR_RADIUS)
    density = fbm(candidates[:, 0], candidates[:, 1], 650.0, 2, 77)
    local = np.random.default_rng(83)
    keep = local.random(len(candidates)) < 0.25 + 0.75 * smoothstep_np(0.3, 0.72, density)
    glands = candidates[keep]
    print("GENESIS: Drüsenöffnungen", len(glands), "von", len(candidates))
    for gx, gy in glands:
        radius = local.uniform(11.0, 20.0) if local.random() < 0.65 else local.uniform(20.0, 28.0)
        stretch = local.uniform(1.0, 1.4) if local.random() < 0.85 else local.uniform(1.5, 1.9)
        angle = local.uniform(0.0, math.pi)
        depth = local.uniform(45.0, 80.0)
        tag = local.random()
        reach = 2.2 * radius * stretch
        ix0 = np.searchsorted(xs, gx - reach)
        ix1 = np.searchsorted(xs, gx + reach)
        iy0 = np.searchsorted(xs, gy - reach)
        iy1 = np.searchsorted(xs, gy + reach)
        if ix1 <= ix0 or iy1 <= iy0:
            continue
        lx = X[iy0:iy1, ix0:ix1] - gx
        ly = Y[iy0:iy1, ix0:ix1] - gy
        ca, sa = math.cos(angle), math.sin(angle)
        u = (lx * ca + ly * sa) / stretch
        v = -lx * sa + ly * ca
        d = np.hypot(u, v) / radius
        # Trichter: steile Wand, runder Übergang an der Kante; leichter Wulst um die Öffnung
        pit = 1.0 - np.clip((d - 0.45) / (1.25 - 0.45), 0.0, 1.0)
        pit = pit * pit * (3.0 - 2.0 * pit)
        collar = np.exp(-((d - 1.55) / 0.45) ** 2)
        height[iy0:iy1, ix0:ix1] += -depth * pit + 2.2 * collar
        gland_depth[iy0:iy1, ix0:ix1] = np.maximum(gland_depth[iy0:iy1, ix0:ix1], pit)
        gland_id[iy0:iy1, ix0:ix1] = np.where(pit > 0.02, tag, gland_id[iy0:iy1, ix0:ix1])

    cushion_norm = np.clip((height - np.percentile(height, 2)) / (np.percentile(height, 98) - np.percentile(height, 2) + 1e-6), 0.0, 1.0)
    return xs, X, Y, height, gland_depth, furrow, cushion_norm, gland_id, glands


def build_mesh(xs, X, Y, height, colors):
    count = len(xs)
    vertices = np.stack([X.ravel(), Y.ravel(), height.ravel()], axis=1) * UM
    mesh = bpy.data.meshes.new("SM_GEN_Endometrium")
    mesh.vertices.add(len(vertices))
    mesh.vertices.foreach_set("co", vertices.astype(np.float32).ravel())

    ii, jj = np.meshgrid(np.arange(count - 1), np.arange(count - 1))
    a = (jj * count + ii).ravel()
    quads = np.stack([a, a + 1, a + 1 + count, a + count], axis=1)
    face_count = len(quads)
    mesh.loops.add(face_count * 4)
    mesh.loops.foreach_set("vertex_index", quads.astype(np.int32).ravel())
    mesh.polygons.add(face_count)
    mesh.polygons.foreach_set("loop_start", (np.arange(face_count) * 4).astype(np.int32))
    mesh.update(calc_edges=True)
    mesh.validate()
    mesh.polygons.foreach_set("use_smooth", np.ones(face_count, dtype=bool))

    # UV: Draufsicht in mm (für Details, die nicht aus der Weltposition kommen)
    uv = mesh.uv_layers.new(name="UVMap")
    loop_vertices = quads.ravel()
    uv_values = np.stack([X.ravel()[loop_vertices] / 1000.0, Y.ravel()[loop_vertices] / 1000.0], axis=1)
    uv.data.foreach_set("uv", uv_values.astype(np.float32).ravel())

    attribute = mesh.color_attributes.new(name="Col", type="FLOAT_COLOR", domain="POINT")
    attribute.data.foreach_set("color", colors.astype(np.float32).ravel())
    # Ohne aktive Farbschicht schreibt der FBX-Export keine Vertexfarben – die Drüsenöffnungen blieben in Unreal hell
    mesh.color_attributes.active_color = attribute
    mesh.color_attributes.render_color_index = 0

    obj = bpy.data.objects.new("SM_GEN_Endometrium", mesh)
    bpy.context.scene.collection.objects.link(obj)
    print("GENESIS: Mesh", len(mesh.vertices), "Vertices", len(mesh.polygons), "Faces")
    return obj


DATA_SIZE = 2048


def resample_uniform(xs, field, size):
    """Vom ungleichmäßigen Gitter auf ein gleichmäßiges (Zeilen, dann Spalten linear)."""
    target = -HALF_EXTENT + (np.arange(size) + 0.5) * (2.0 * HALF_EXTENT / size)
    rows = np.empty((field.shape[0], size))
    for index in range(field.shape[0]):
        rows[index] = np.interp(target, xs, field[index])
    out = np.empty((size, size))
    for index in range(size):
        out[:, index] = np.interp(target, xs, rows[:, index])
    return out


def write_data_texture(xs, channels, path):
    """
    Datenbild für das Material: R Drüsenöffnung, G Furche, B Polster, A 0,5 + ½ Kennwert der Drüse – 3,9 µm je Texel über ±4 mm.
    Vertexfarben kamen in Unreal nicht an (weder hier noch an der Eileiterwand); ein Bild ist außerdem in der Ferne
    stabil, wo Nanite die Geometrie vereinfacht. Zeile 0 der Datei = größtes Y (Unreal liest Bilder von oben).
    """
    planes = [resample_uniform(xs, channel, DATA_SIZE) for channel in channels]
    # Alpha nie 0: Sonst ersetzt der PNG-Import von Unreal die Farbe durchsichtiger Pixel durch Nachbarfarben (Infill)
    planes[3] = 0.5 + 0.5 * planes[3]
    rgba = np.stack(planes, axis=-1).astype(np.float32)   # [y, x, c], y aufsteigend = Blender-Zeilen von unten
    image = bpy.data.images.new("T_GEN_EndometriumData", DATA_SIZE, DATA_SIZE, alpha=True, float_buffer=False)
    image.colorspace_settings.name = "Non-Color"
    image.pixels.foreach_set(np.clip(rgba, 0.0, 1.0).ravel())
    image.filepath_raw = path
    image.file_format = "PNG"
    image.save()
    print("GENESIS: Datenbild", path, "Mittelwerte", [round(float(p.mean()), 4) for p in planes])


def make_lookdev_material():
    """Nur für die Prüfung in Blender – das Spielmaterial entsteht in Unreal (M_GEN_Endometrium)."""
    mat = bpy.data.materials.new("LookDev_Endometrium")
    mat.use_nodes = True
    tree = mat.node_tree
    tree.nodes.clear()
    out = new_node(tree, "ShaderNodeOutputMaterial", (900, 0))
    bsdf = new_node(tree, "ShaderNodeBsdfPrincipled", (500, 0))
    attr = new_node(tree, "ShaderNodeAttribute", (-700, 0), attribute_name="Col")
    sep = new_node(tree, "ShaderNodeSeparateColor", (-500, 0))
    tree.links.new(attr.outputs["Color"], sep.inputs["Color"])

    # Gefäßnetz unter dem Epithel: Voronoi-Kanten in zwei Größen
    coord = new_node(tree, "ShaderNodeTexCoord", (-900, -400))
    vor1 = new_node(tree, "ShaderNodeTexVoronoi", (-700, -400), feature="DISTANCE_TO_EDGE")
    vor1.inputs["Scale"].default_value = 1.0 / (55.0 * UM)
    vor2 = new_node(tree, "ShaderNodeTexVoronoi", (-700, -650), feature="DISTANCE_TO_EDGE")
    vor2.inputs["Scale"].default_value = 1.0 / (190.0 * UM)
    tree.links.new(coord.outputs["Object"], vor1.inputs["Vector"])
    tree.links.new(coord.outputs["Object"], vor2.inputs["Vector"])
    ramp1 = new_node(tree, "ShaderNodeMapRange", (-450, -400))
    ramp1.inputs["From Min"].default_value = 0.0
    ramp1.inputs["From Max"].default_value = 0.06
    ramp1.inputs["To Min"].default_value = 1.0
    ramp1.inputs["To Max"].default_value = 0.0
    tree.links.new(vor1.outputs["Distance"], ramp1.inputs["Value"])
    ramp2 = new_node(tree, "ShaderNodeMapRange", (-450, -650))
    ramp2.inputs["From Min"].default_value = 0.0
    ramp2.inputs["From Max"].default_value = 0.035
    ramp2.inputs["To Min"].default_value = 1.0
    ramp2.inputs["To Max"].default_value = 0.0
    tree.links.new(vor2.outputs["Distance"], ramp2.inputs["Value"])
    vessels = new_node(tree, "ShaderNodeMath", (-250, -500), operation="MAXIMUM")
    tree.links.new(ramp1.outputs[0], vessels.inputs[0])
    tree.links.new(ramp2.outputs[0], vessels.inputs[1])

    tissue = new_node(tree, "ShaderNodeRGB", (-250, 200))
    tissue.outputs[0].default_value = (0.62, 0.34, 0.30, 1.0)
    vessel_col = new_node(tree, "ShaderNodeRGB", (-250, 350))
    vessel_col.outputs[0].default_value = (0.40, 0.06, 0.05, 1.0)
    mix_v = new_node(tree, "ShaderNodeMix", (0, 200), data_type="RGBA")
    mix_v.inputs["Factor"].default_value = 0.5
    scale_v = new_node(tree, "ShaderNodeMath", (-100, -500), operation="MULTIPLY")
    scale_v.inputs[1].default_value = 0.55
    tree.links.new(vessels.outputs[0], scale_v.inputs[0])
    tree.links.new(scale_v.outputs[0], mix_v.inputs["Factor"])
    tree.links.new(tissue.outputs[0], mix_v.inputs["A"])
    tree.links.new(vessel_col.outputs[0], mix_v.inputs["B"])
    gland_col = new_node(tree, "ShaderNodeRGB", (0, 400))
    gland_col.outputs[0].default_value = (0.20, 0.07, 0.06, 1.0)
    mix_g = new_node(tree, "ShaderNodeMix", (250, 200), data_type="RGBA")
    tree.links.new(sep.outputs["Red"], mix_g.inputs["Factor"])
    tree.links.new(mix_v.outputs["Result"], mix_g.inputs["A"])
    tree.links.new(gland_col.outputs[0], mix_g.inputs["B"])
    tree.links.new(mix_g.outputs["Result"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.25
    bsdf.inputs["IOR"].default_value = 1.04  # Gewebe gegen Uterusflüssigkeit
    bsdf.inputs["Subsurface Weight"].default_value = 1.0
    bsdf.inputs["Subsurface Radius"].default_value = (1.0, 0.3, 0.18)
    bsdf.inputs["Subsurface Scale"].default_value = 120.0 * UM
    tree.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    return mat


def render_lookdev(obj, out_dir, exposure, samples, energy):
    scene = bpy.context.scene
    obj.data.materials.clear()
    obj.data.materials.append(make_lookdev_material())
    enable_gpu(scene)
    configure_color_management(scene, exposure=exposure)
    configure_cycles(scene, samples=samples, width=int(os.environ.get("GENESIS_W", "1600")), height=int(os.environ.get("GENESIS_H", "900")))

    world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes.clear()
    background = new_node(world.node_tree, "ShaderNodeBackground", (0, 0))
    background.inputs["Strength"].default_value = 0.0
    world_out = new_node(world.node_tree, "ShaderNodeOutputWorld", (300, 0))
    world.node_tree.links.new(background.outputs[0], world_out.inputs[0])

    # Maßstabsprobe: geschlüpfte Blastozyste, 200 µm, auf der Einnistungsstelle
    bpy.ops.mesh.primitive_uv_sphere_add(segments=64, ring_count=32, radius=100.0 * UM, location=(0.0, 0.0, (14.0 + 100.0) * UM))
    probe = bpy.context.active_object
    probe_mat = bpy.data.materials.new("Probe")
    probe_mat.use_nodes = True
    pb = probe_mat.node_tree.nodes["Principled BSDF"]
    pb.inputs["Base Color"].default_value = (0.85, 0.80, 0.74, 1.0)
    pb.inputs["Roughness"].default_value = 0.3
    pb.inputs["Transmission Weight"].default_value = 0.6
    pb.inputs["IOR"].default_value = 1.03
    probe.data.materials.append(probe_mat)

    camera_data = bpy.data.cameras.new("Camera")
    camera_data.sensor_width = 36.0
    camera_data.lens = 35.0
    camera_data.clip_start = 1.0 * UM
    camera_data.clip_end = 20000.0 * UM
    camera_data.dof.use_dof = True
    camera_data.dof.aperture_fstop = 11.0
    camera = bpy.data.objects.new("Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.location = Vector((-150.0, -900.0, 620.0)) * UM
    target = Vector((0.0, 0.0, 60.0)) * UM
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.dof.focus_distance = (target - camera.location).length

    light_data = bpy.data.lights.new("HysteroscopeLight", "SPOT")
    light_data.energy = float(energy)
    light_data.spot_size = math.radians(100.0)
    light_data.spot_blend = 0.7
    light_data.shadow_soft_size = 25.0 * UM
    light_data.color = (1.0, 0.96, 0.90)
    light = bpy.data.objects.new("HysteroscopeLight", light_data)
    scene.collection.objects.link(light)
    light.parent = camera
    light.location = (40.0 * UM, -30.0 * UM, 0.0)

    scene.render.filepath = os.path.join(out_dir, "LookDev_Endometrium.png")
    bpy.ops.render.render(write_still=True)
    measure_exposure(scene.render.filepath)


def export_fbx(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False,
                             colors_type="LINEAR", add_leaf_bones=False)
    print("GENESIS: exportiert", path, round(os.path.getsize(path) / 1e6, 1), "MB")


def main():
    args = script_args()
    out_dir = ensure_dir(os.path.abspath(args.get("out", "ArtSource/Generated/Implantation")))
    reset_scene()
    xs, X, Y, height, gland_depth, furrow, cushion, gland_id, glands = build_heightfield()
    colors = np.stack([gland_depth.ravel(), furrow.ravel(), cushion.ravel(), gland_id.ravel()], axis=1)
    print("GENESIS: Höhe µm min %.1f max %.1f | Drüsen %d" % (height.min(), height.max(), len(glands)))
    center = int(np.argmin(np.abs(xs)))
    # Die Einnistungsstelle in Unreal steht genau auf der Oberfläche: diesen Wert übernimmt setup_uterus_scene.py
    site_height = float(height[center, center])
    print("GENESIS: Oberfläche an der Einnistungsstelle %.2f µm" % site_height)
    with open(os.path.join(out_dir, "SM_GEN_Endometrium_site.txt"), "w") as handle:
        handle.write("%.3f" % site_height)
    obj = build_mesh(xs, X, Y, height, colors)
    if args.get("export"):
        export_fbx(obj, os.path.join(out_dir, "SM_GEN_Endometrium.fbx"))
        write_data_texture(xs, [gland_depth, furrow, cushion, gland_id], os.path.join(out_dir, "T_GEN_EndometriumData.png"))
    if args.get("render"):
        render_lookdev(obj, out_dir, float(args.get("exposure", 0.0)), int(args.get("samples", 128)), float(args.get("energy", 20000.0)))


main()
