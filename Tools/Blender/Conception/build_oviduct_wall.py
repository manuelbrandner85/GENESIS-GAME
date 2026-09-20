# GENESIS: Der Kreislauf des Lebens
# Baut einen nahtlos kachelbaren Abschnitt der Eileiter-Ampulle (SM_GEN_OviductWall) aus anatomischen Maßen.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Conception/build_oviduct_wall.py -- --out ArtSource/Generated/Conception [--export] [--render]
#
# Anatomie (menschliche Ampulla tubae uterinae, Histologie-Referenzwerte):
#   - Lumen der Ampulle mehrere mm, fast vollständig von hohen, verzweigten Schleimhautfalten (Plicae tubariae) ausgefüllt
#   - 8–10 Primärfalten entlang der Längsachse, bis ~0,6 mm hoch, blattartig dünn (~50–120 µm), leicht wellig
#   - Sekundär- und Tertiärfalten zweigen seitlich ab → labyrinthische Spalträume
#   - Epithel (einschichtig, ~20–30 µm hoch, Flimmer- und sekretorische Zellen) als Oberflächen-Detail im Material
# Maßstab: 1 µm = 0,01 m (Blender) = 1 cm (Unreal). Kanalachse = X. Die Faltenspitzen lassen einen freien Kanal (Radius 450 µm),
# passend zum Schwimmmodell (FGenesisOviductChannel.LumenRadiusUm).
#
# Aufbau: Faltenblätter als geschlossene Loft-Körper → Vereinigung über OpenVDB (Mesh to Volume, 4 µm Voxel) → Volume to Mesh →
# exakter Schnitt auf die Abschnittslänge (alle Formfunktionen sind periodisch in X) → Farbattribut für das Material.

import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from genesis_blender_common import (UM, configure_color_management, configure_cycles, enable_gpu, ensure_dir, measure_exposure, new_node,
                                    reset_scene, script_args, smoothstep)
import bmesh
import bpy
import numpy as np
from mathutils import Vector

SEGMENT_LENGTH = 1500.0
WALL_RADIUS = 1150.0
WALL_THICKNESS = 140.0
CHANNEL_RADIUS = 450.0
PRIMARY_FOLDS = 9
VOXEL_UM = 5.0
EXTEND = 40.0  # Überstand vor dem exakten Schnitt

rng = np.random.default_rng(20260917)


def periodic(x, cycles, phase):
    """Periodisch über die Abschnittslänge → nahtloses Kacheln."""
    return math.sin(2.0 * math.pi * cycles * x / SEGMENT_LENGTH + phase)


def fold_outline(center_points, thickness_fn):
    """Geschlossene 2D-Kontur eines Faltenblatts aus Mittellinie (Liste (y, z)) und Dicke je Position u (0 Basis … 1 Spitze)."""
    left = []
    right = []
    count = len(center_points)
    for index, (py, pz) in enumerate(center_points):
        prev_p = center_points[max(0, index - 1)]
        next_p = center_points[min(count - 1, index + 1)]
        ty, tz = next_p[0] - prev_p[0], next_p[1] - prev_p[1]
        length = math.hypot(ty, tz) or 1.0
        ny, nz = -tz / length, ty / length
        half = 0.5 * thickness_fn(index / (count - 1))
        left.append((py + ny * half, pz + nz * half))
        right.append((py - ny * half, pz - nz * half))
    # Abgerundete Spitze: Halbkreis um den letzten Mittelpunkt
    tip_y, tip_z = center_points[-1]
    prev_y, prev_z = center_points[-2]
    dir_y, dir_z = tip_y - prev_y, tip_z - prev_z
    base_angle = math.atan2(dir_z, dir_y)
    half_tip = 0.5 * thickness_fn(1.0)
    cap = []
    for step in range(1, 8):
        angle = base_angle + math.pi / 2.0 - math.pi * step / 8.0
        cap.append((tip_y + math.cos(angle) * half_tip, tip_z + math.sin(angle) * half_tip))
    return left + cap + list(reversed(right))


def loft(name, outline_at_x, x_values):
    """Verbindet gleich lange geschlossene Konturen entlang X zu einem geschlossenen Körper."""
    bm = bmesh.new()
    rings = []
    for x in x_values:
        ring = [bm.verts.new((x * UM, y * UM, z * UM)) for (y, z) in outline_at_x(x)]
        rings.append(ring)
    size = len(rings[0])
    for a, b in zip(rings[:-1], rings[1:]):
        for j in range(size):
            k = (j + 1) % size
            bm.faces.new((a[j], a[k], b[k], b[j]))
    bm.faces.new(list(reversed(rings[0])))
    bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def build_primary_fold(index):
    base_angle = 2.0 * math.pi * index / PRIMARY_FOLDS + rng.uniform(-0.12, 0.12)
    tip_radius = CHANNEL_RADIUS + rng.uniform(25.0, 90.0)
    lean = rng.uniform(-0.25, 0.25)  # Falten neigen sich (liegen im Gewebe nicht radial wie Speichen)
    wave_cycles = int(rng.integers(1, 4))
    wave_amp = rng.uniform(0.03, 0.08)
    wave_phase = rng.uniform(0.0, 2.0 * math.pi)
    tip_cycles = int(rng.integers(2, 6))
    tip_phase = rng.uniform(0.0, 2.0 * math.pi)
    base_thickness = rng.uniform(95.0, 130.0)
    tip_thickness = rng.uniform(45.0, 65.0)
    samples = 24

    def outline(x):
        tip = tip_radius + 35.0 * periodic(x, tip_cycles, tip_phase)
        centers = []
        for s in range(samples):
            u = s / (samples - 1)
            r = (WALL_RADIUS + 60.0) + (tip - (WALL_RADIUS + 60.0)) * u
            # Welligkeit entlang X, Neigung und leichte S-Krümmung zur Spitze
            angle = base_angle + wave_amp * periodic(x, wave_cycles, wave_phase) * u + lean * u * u * 0.35 + 0.05 * math.sin(math.pi * u) * periodic(x, 1, wave_phase)
            centers.append((r * math.cos(angle), r * math.sin(angle)))
        return fold_outline(centers, lambda u: base_thickness + (tip_thickness - base_thickness) * u ** 0.7)

    step = 3.5
    xs = [-EXTEND + i * step for i in range(int((SEGMENT_LENGTH + 2 * EXTEND) / step) + 1)]
    return loft(f"Fold_{index:02d}", outline, xs), base_angle, lean, (wave_cycles, wave_amp, wave_phase)


def build_secondary_folds(primary_index, base_angle, lean, wave):
    """Seitliche Abzweigungen: kürzere Blätter, die schräg aus der Primärfalte wachsen."""
    folds = []
    wave_cycles, wave_amp, wave_phase = wave
    for branch in range(int(rng.integers(2, 5))):
        u_start = rng.uniform(0.25, 0.6)
        side = rng.choice([-1.0, 1.0])
        spread = side * rng.uniform(0.35, 0.7)
        length = rng.uniform(140.0, 320.0)
        thickness = rng.uniform(40.0, 60.0)
        branch_cycles = int(rng.integers(1, 3))
        branch_phase = rng.uniform(0.0, 2.0 * math.pi)
        tip_radius_primary = CHANNEL_RADIUS + 60.0

        def outline(x, u_start=u_start, spread=spread, length=length, thickness=thickness, branch_cycles=branch_cycles, branch_phase=branch_phase):
            r0 = (WALL_RADIUS + 60.0) + (tip_radius_primary - (WALL_RADIUS + 60.0)) * u_start
            a0 = base_angle + wave_amp * periodic(x, wave_cycles, wave_phase) * u_start + lean * u_start * u_start * 0.35
            start = (r0 * math.cos(a0), r0 * math.sin(a0))
            # Richtung: zur Mitte hin, seitlich abgewinkelt; nie in den freien Kanal
            inward = (-math.cos(a0), -math.sin(a0))
            tangent = (-math.sin(a0), math.cos(a0))
            bend = spread + 0.15 * periodic(x, branch_cycles, branch_phase)
            direction = (inward[0] * math.cos(bend) + tangent[0] * math.sin(bend), inward[1] * math.cos(bend) + tangent[1] * math.sin(bend))
            current_length = length * (0.85 + 0.15 * periodic(x, branch_cycles + 1, branch_phase))
            centers = []
            for s in range(14):
                u = s / 13.0
                curl = 0.25 * u * u * (1.0 if spread > 0 else -1.0)
                dy = direction[0] * math.cos(curl) - direction[1] * math.sin(curl)
                dz = direction[0] * math.sin(curl) + direction[1] * math.cos(curl)
                py = start[0] + dy * current_length * u
                pz = start[1] + dz * current_length * u
                radius = math.hypot(py, pz)
                if radius < CHANNEL_RADIUS + 20.0:
                    scale = (CHANNEL_RADIUS + 20.0) / radius
                    py, pz = py * scale, pz * scale
                centers.append((py, pz))
            return fold_outline(centers, lambda u: thickness * (1.0 - 0.35 * u))

        step = 4.5
        xs = [-EXTEND + i * step for i in range(int((SEGMENT_LENGTH + 2 * EXTEND) / step) + 1)]
        folds.append(loft(f"Fold_{primary_index:02d}_{branch}", outline, xs))
    return folds


def build_wall():
    def outline(x):
        points = []
        count = 180
        for j in range(count):
            angle = 2.0 * math.pi * j / count
            r = WALL_RADIUS + 12.0 * math.sin(3.0 * angle + 2.0 * math.pi * x / SEGMENT_LENGTH)
            points.append((r * math.cos(angle), r * math.sin(angle)))
        outer = [(y * (WALL_RADIUS + WALL_THICKNESS) / WALL_RADIUS, z * (WALL_RADIUS + WALL_THICKNESS) / WALL_RADIUS) for (y, z) in points]
        return points, outer

    # Rohr als zwei Konturen: innen und außen, als geschlossener Ring-Körper
    bm = bmesh.new()
    step = 6.0
    xs = [-EXTEND + i * step for i in range(int((SEGMENT_LENGTH + 2 * EXTEND) / step) + 1)]
    inner_rings, outer_rings = [], []
    for x in xs:
        inner, outer = outline(x)
        inner_rings.append([bm.verts.new((x * UM, y * UM, z * UM)) for (y, z) in inner])
        outer_rings.append([bm.verts.new((x * UM, y * UM, z * UM)) for (y, z) in outer])
    size = len(inner_rings[0])
    for rings in (inner_rings, outer_rings):
        for a, b in zip(rings[:-1], rings[1:]):
            for j in range(size):
                k = (j + 1) % size
                bm.faces.new((a[j], a[k], b[k], b[j]))
    for inner, outer in ((inner_rings[0], outer_rings[0]), (inner_rings[-1], outer_rings[-1])):
        for j in range(size):
            k = (j + 1) % size
            bm.faces.new((inner[j], inner[k], outer[k], outer[j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new("Wall")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new("Wall", mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def union_via_volume(objects):
    """Glatte Vereinigung aller Körper über OpenVDB (Geometry Nodes), anschließend als Mesh angewendet."""
    for obj in objects[1:]:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    for obj in objects:
        obj.select_set(True)
    bpy.ops.object.join()
    joined = bpy.context.view_layer.objects.active
    joined.name = "SM_GEN_OviductWall"

    tree = bpy.data.node_groups.new("GENESIS_VolumeUnion", "GeometryNodeTree")
    tree.interface.new_socket("Geometry", in_out="INPUT", socket_type="NodeSocketGeometry")
    tree.interface.new_socket("Geometry", in_out="OUTPUT", socket_type="NodeSocketGeometry")
    group_in = new_node(tree, "NodeGroupInput", (-600, 0))
    group_out = new_node(tree, "NodeGroupOutput", (600, 0))
    to_volume = new_node(tree, "GeometryNodeMeshToVolume", (-300, 0))
    to_volume.inputs["Resolution Mode"].default_value = "Size"
    to_volume.inputs["Voxel Size"].default_value = VOXEL_UM * UM
    to_volume.inputs["Interior Band Width"].default_value = 3.0 * VOXEL_UM * UM
    to_mesh = new_node(tree, "GeometryNodeVolumeToMesh", (0, 0))
    to_mesh.inputs["Resolution Mode"].default_value = "Grid"
    to_mesh.inputs["Threshold"].default_value = 0.1
    to_mesh.inputs["Adaptivity"].default_value = 0.0
    tree.links.new(group_in.outputs[0], to_volume.inputs["Mesh"])
    tree.links.new(to_volume.outputs["Volume"], to_mesh.inputs["Volume"])
    tree.links.new(to_mesh.outputs["Mesh"], group_out.inputs[0])

    modifier = joined.modifiers.new("VolumeUnion", "NODES")
    modifier.node_group = tree
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    print("GENESIS: nach Volumen-Vereinigung", len(joined.data.vertices), "Vertices")
    return joined


def trim_and_finish(obj):
    """Exakter Schnitt auf [0, SEGMENT_LENGTH], Glättung, Farbattribut (Hohlräume, Faltenhöhe, Zufall je Region)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for plane_x, normal in ((0.0, (-1.0, 0.0, 0.0)), (SEGMENT_LENGTH * UM, (1.0, 0.0, 0.0))):
        geometry = bm.verts[:] + bm.edges[:] + bm.faces[:]
        bmesh.ops.bisect_plane(bm, geom=geometry, dist=1e-6, plane_co=(plane_x, 0.0, 0.0), plane_no=normal, clear_outer=True)
    # Sicherheitsnetz: alles außerhalb des Abschnitts entfernen (der Schnitt allein ließ Reste stehen)
    outside = [v for v in bm.verts if v.co.x < -1e-5 or v.co.x > SEGMENT_LENGTH * UM + 1e-5]
    if outside:
        bmesh.ops.delete(bm, geom=outside, context="VERTS")
        print("GENESIS: Reste außerhalb des Abschnitts entfernt:", len(outside))
    # Voxel-Treppen und Loft-Rippen wegglätten (sie lesen sich sonst wie Cordsamt)
    inner = [v for v in bm.verts if 1.0 * UM < v.co.x < (SEGMENT_LENGTH - 1.0) * UM]
    for _ in range(6):
        bmesh.ops.smooth_vert(bm, verts=inner, factor=0.55, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    bm.to_mesh(obj.data)
    bm.free()

    mesh = obj.data
    for poly in mesh.polygons:
        poly.use_smooth = True

    count = len(mesh.vertices)
    co = np.empty(count * 3)
    mesh.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3) / UM
    normals = np.empty(count * 3)
    mesh.vertices.foreach_get("normal", normals)
    normals = normals.reshape(-1, 3)

    radius = np.hypot(co[:, 1], co[:, 2])
    # G: Höhe in der Falte (0 an der Wand, 1 an der Faltenspitze am Kanal)
    height = np.clip((WALL_RADIUS - radius) / (WALL_RADIUS - CHANNEL_RADIUS), 0.0, 1.0)
    # R: Hohlraum – Oberflächen, deren Normale nach außen (zur Wand) zeigt und tief in Spalten liegen, bekommen wenig Licht/Strömung
    radial_dir = np.stack([np.zeros(count), co[:, 1] / np.maximum(radius, 1e-3), co[:, 2] / np.maximum(radius, 1e-3)], axis=1)
    facing_inward = np.clip(-(normals * radial_dir).sum(axis=1), -1.0, 1.0)
    cavity = np.clip((1.0 - height) * 0.7 + (1.0 - (facing_inward * 0.5 + 0.5)) * 0.3, 0.0, 1.0)
    # B: großflächige Variation (Durchblutung, Epitheltyp) – periodisch in X
    variation = 0.5 + 0.25 * np.sin(2.0 * np.pi * co[:, 0] / SEGMENT_LENGTH + np.arctan2(co[:, 2], co[:, 1]) * 2.0) + 0.25 * np.sin(np.arctan2(co[:, 2], co[:, 1]) * 5.0 + 1.3)

    print("GENESIS: Abschnitt X von %.1f bis %.1f µm" % (co[:, 0].min(), co[:, 0].max()))
    # Organische Unregelmäßigkeit (Gewebe ist nie gleichmäßig) – bricht die verbliebene Regelmäßigkeit der Rippen.
    # An den Abschnittsenden läuft sie auf null aus, damit aneinandergesetzte Abschnitte exakt schließen.
    phase = co[:, 0] * 0.09 + np.arctan2(co[:, 2], co[:, 1]) * 3.7
    wobble = (0.9 * np.sin(phase) + 0.6 * np.sin(phase * 2.3 + 1.1) + 0.4 * np.sin(co[:, 0] * 0.31 + radius * 0.05))
    edge = np.clip(np.minimum(co[:, 0], SEGMENT_LENGTH - co[:, 0]) / 30.0, 0.0, 1.0)
    edge = edge * edge * (3.0 - 2.0 * edge)
    displaced = co + normals * (wobble * edge)[:, None] * 1.6
    displaced[:, 0] = np.where(co[:, 0] < 1.0, 0.0, np.where(co[:, 0] > SEGMENT_LENGTH - 1.0, SEGMENT_LENGTH, displaced[:, 0]))
    mesh.vertices.foreach_set("co", (displaced * UM).ravel().astype(np.float64))
    mesh.update()
    co = displaced

    color = mesh.color_attributes.new(name="Tissue", type="FLOAT_COLOR", domain="POINT")
    data = np.stack([cavity, height, np.clip(variation, 0.0, 1.0), np.ones(count)], axis=1).astype(np.float32)
    color.data.foreach_set("color", data.ravel())
    print("GENESIS: Wand fertig", len(mesh.vertices), "Vertices", len(mesh.polygons), "Faces")


def make_lookdev_material():
    """Referenz-Look: Schleimhaut in Flüssigkeit – Subsurface (gut durchblutetes Bindegewebe unter dünnem Epithel), geringe Spiegelung."""
    mat = bpy.data.materials.new("M_GEN_OviductMucosa_LookDev")
    mat.use_nodes = True
    tree = mat.node_tree
    tree.nodes.clear()
    out = new_node(tree, "ShaderNodeOutputMaterial", (900, 0))
    bsdf = new_node(tree, "ShaderNodeBsdfPrincipled", (600, 0))
    attr = new_node(tree, "ShaderNodeAttribute", (-900, 200), attribute_name="Tissue")
    sep = new_node(tree, "ShaderNodeSeparateColor", (-700, 200))
    tree.links.new(attr.outputs["Color"], sep.inputs["Color"])

    # Farbe: Faltenspitzen blasser (Epithel dicker, weniger Gefäße sichtbar), Spalten kräftiger rosa-rot
    tip = new_node(tree, "ShaderNodeRGB", (-700, -50))
    tip.outputs[0].default_value = (0.62, 0.40, 0.36, 1.0)
    deep = new_node(tree, "ShaderNodeRGB", (-700, -200))
    deep.outputs[0].default_value = (0.42, 0.16, 0.14, 1.0)
    mix = new_node(tree, "ShaderNodeMix", (-450, 0), data_type="RGBA")
    tree.links.new(sep.outputs["Red"], mix.inputs["Factor"])
    tree.links.new(tip.outputs[0], mix.inputs["A"])
    tree.links.new(deep.outputs[0], mix.inputs["B"])

    # Epithel-Zellmosaik (Zellen ~10 µm) und Kapillarnetz (Voronoi-Kanten ~80 µm) in Objektkoordinaten
    coords = new_node(tree, "ShaderNodeTexCoord", (-1100, -400))
    cells = new_node(tree, "ShaderNodeTexVoronoi", (-850, -400))
    cells.inputs["Scale"].default_value = 1.0 / (10.0 * UM)
    tree.links.new(coords.outputs["Object"], cells.inputs["Vector"])
    cell_edges = new_node(tree, "ShaderNodeTexVoronoi", (-850, -650), feature="DISTANCE_TO_EDGE")
    cell_edges.inputs["Scale"].default_value = 1.0 / (10.0 * UM)
    tree.links.new(coords.outputs["Object"], cell_edges.inputs["Vector"])
    vessels = new_node(tree, "ShaderNodeTexVoronoi", (-850, -900), feature="DISTANCE_TO_EDGE")
    vessels.inputs["Scale"].default_value = 1.0 / (80.0 * UM)
    vessels.inputs["Randomness"].default_value = 0.9
    noise_warp = new_node(tree, "ShaderNodeTexNoise", (-1100, -900))
    noise_warp.inputs["Scale"].default_value = 1.0 / (60.0 * UM)
    warp_mix = new_node(tree, "ShaderNodeMix", (-950, -1000), data_type="VECTOR")
    warp_mix.inputs["Factor"].default_value = 0.35
    tree.links.new(coords.outputs["Object"], warp_mix.inputs["A"])
    tree.links.new(noise_warp.outputs["Color"], warp_mix.inputs["B"])
    tree.links.new(warp_mix.outputs["Result"], vessels.inputs["Vector"])
    vessel_mask = new_node(tree, "ShaderNodeMapRange", (-600, -900), interpolation_type="SMOOTHSTEP")
    vessel_mask.inputs["From Min"].default_value = 0.0
    vessel_mask.inputs["From Max"].default_value = 0.035
    vessel_mask.inputs["To Min"].default_value = 1.0
    vessel_mask.inputs["To Max"].default_value = 0.0
    tree.links.new(vessels.outputs["Distance"], vessel_mask.inputs["Value"])
    vessel_color = new_node(tree, "ShaderNodeRGB", (-450, -700))
    vessel_color.outputs[0].default_value = (0.30, 0.06, 0.06, 1.0)
    vessel_strength = new_node(tree, "ShaderNodeMath", (-350, -900), operation="MULTIPLY")
    vessel_strength.inputs[1].default_value = 0.45
    tree.links.new(vessel_mask.outputs["Result"], vessel_strength.inputs[0])
    with_vessels = new_node(tree, "ShaderNodeMix", (-200, -100), data_type="RGBA")
    tree.links.new(vessel_strength.outputs[0], with_vessels.inputs["Factor"])
    tree.links.new(mix.outputs["Result"], with_vessels.inputs["A"])
    tree.links.new(vessel_color.outputs[0], with_vessels.inputs["B"])

    # Zellgrenzen leicht vertieft (Bump), Zellkuppen gewölbt
    cell_bump_range = new_node(tree, "ShaderNodeMapRange", (-600, -650), interpolation_type="SMOOTHSTEP")
    cell_bump_range.inputs["From Max"].default_value = 0.35
    tree.links.new(cell_edges.outputs["Distance"], cell_bump_range.inputs["Value"])
    bump = new_node(tree, "ShaderNodeBump", (300, -400))
    bump.inputs["Strength"].default_value = 0.35
    bump.inputs["Distance"].default_value = 1.2 * UM
    tree.links.new(cell_bump_range.outputs["Result"], bump.inputs["Height"])

    rough_noise = new_node(tree, "ShaderNodeTexNoise", (-850, -1200))
    rough_noise.inputs["Scale"].default_value = 1.0 / (25.0 * UM)
    tree.links.new(coords.outputs["Object"], rough_noise.inputs["Vector"])
    rough = new_node(tree, "ShaderNodeMapRange", (-600, -1200))
    rough.inputs["To Min"].default_value = 0.35
    rough.inputs["To Max"].default_value = 0.6
    tree.links.new(rough_noise.outputs["Fac"], rough.inputs["Value"])

    tree.links.new(with_vessels.outputs["Result"], bsdf.inputs["Base Color"])
    tree.links.new(rough.outputs["Result"], bsdf.inputs["Roughness"])
    tree.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    bsdf.inputs["IOR"].default_value = 1.035  # Gewebe (n ≈ 1,38) relativ zur Eileiterflüssigkeit
    bsdf.inputs["Subsurface Weight"].default_value = 1.0
    bsdf.inputs["Subsurface Radius"].default_value = (1.0, 0.35, 0.2)
    bsdf.inputs["Subsurface Scale"].default_value = 40.0 * UM
    tree.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    return mat


def render_lookdev(obj, out_dir, exposure, samples, args_energy=4000.0):
    scene = bpy.context.scene
    material = make_lookdev_material()
    obj.data.materials.clear()
    obj.data.materials.append(material)
    for poly in obj.data.polygons:
        poly.material_index = 0
    bsdf_node = next((n for n in material.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
    base_link = bsdf_node.inputs["Base Color"].links if bsdf_node else []
    print("GENESIS: Materialslots", len(obj.data.materials), "| Base-Color verbunden:", bool(base_link),
          "| Knoten", len(material.node_tree.nodes), "| Farbattribute", [a.name for a in obj.data.color_attributes])
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

    camera_data = bpy.data.cameras.new("Camera")
    camera_data.sensor_width = 36.0
    camera_data.lens = 24.0
    camera_data.clip_start = 1.0 * UM
    camera_data.clip_end = 5000.0 * UM
    camera_data.dof.use_dof = True
    camera_data.dof.aperture_fstop = 8.0
    camera = bpy.data.objects.new("Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    # Im freien Kanal, Blick schräg den Kanal entlang auf die Falten
    camera.location = Vector((200.0, 0.0, -150.0)) * UM
    target = Vector((900.0, 380.0, 60.0)) * UM
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.dof.focus_distance = (target - camera.location).length

    light_data = bpy.data.lights.new("EndoscopeLight", "SPOT")
    # Der Maßstab (1 µm = 0,01 m) verschiebt Abstände um 10^4, die Beleuchtungsstärke also um 10^8.
    # Die Lampenleistung wird entsprechend skaliert; die Belichtung wird anschließend gemessen.
    light_data.energy = float(args_energy)
    light_data.spot_size = math.radians(95.0)
    light_data.spot_blend = 0.6
    light_data.shadow_soft_size = 1.0 * UM
    light_data.color = (1.0, 0.97, 0.93)
    light = bpy.data.objects.new("EndoscopeLight", light_data)
    scene.collection.objects.link(light)
    light.parent = camera
    light.location = (1.5 * UM, -1.0 * UM, 0.0)

    scene.render.filepath = os.path.join(out_dir, "LookDev_OviductWall.png")
    bpy.ops.render.render(write_still=True)
    measure_exposure(scene.render.filepath)


def build_cilia(wall, spacing_um=6.0, edge_spacing_um=2.5):
    """
    Flimmerhärchen (Kinozilien) auf den zum Kanal zeigenden Flächen.

    Real stehen sie ~0,3 µm auseinander – Millionen Halme sind als Geometrie unmöglich. Sichtbar sind sie an zwei Stellen:
    als dichter Saum auf den Silhouetten (Faltenkanten) und als feines Flimmern auf den zugewandten Flächen.
    Deshalb: enger Abstand nahe den Faltenspitzen, weiter in der Fläche. Länge 10 µm wie in der Anatomie.
    UV: y = Höhe am Halm (0 Fuß … 1 Spitze), x = Zufallsphase je Halm (Variation des Schlags).
    """
    mesh = wall.data
    count = len(mesh.vertices)
    co = np.empty(count * 3)
    mesh.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3) / UM
    normals = np.empty(count * 3)
    mesh.vertices.foreach_get("normal", normals)
    normals = normals.reshape(-1, 3)

    radius = np.hypot(co[:, 1], co[:, 2])
    radial = np.stack([np.zeros(count), co[:, 1] / np.maximum(radius, 1e-3), co[:, 2] / np.maximum(radius, 1e-3)], axis=1)
    facing = -(normals * radial).sum(axis=1)
    # Nur Flächen, die in den freien Kanal schauen; in den tiefen Spalten sieht man ohnehin nichts
    candidates = np.where((radius < CHANNEL_RADIUS + 260.0) & (facing > 0.25) & (co[:, 0] > 2.0) & (co[:, 0] < SEGMENT_LENGTH - 2.0))[0]
    if len(candidates) == 0:
        return None

    # Dichter an den Faltenspitzen (dort bilden die Halme die Silhouette), weiter in der Fläche
    tip_proximity = np.clip((CHANNEL_RADIUS + 120.0 - radius[candidates]) / 120.0, 0.0, 1.0)
    spacing = edge_spacing_um + (spacing_um - edge_spacing_um) * (1.0 - tip_proximity)
    keys = np.floor(co[candidates] / spacing[:, None]).astype(np.int64)
    _, unique_index = np.unique(keys, axis=0, return_index=True)
    picks = candidates[np.sort(unique_index)]
    print("GENESIS: Zilien-Standorte", len(picks))

    rng_local = np.random.default_rng(4711)
    sides, segments = 4, 4
    verts, faces, uvs = [], [], []
    for index in picks:
        base = co[index]
        axis = normals[index]
        axis = axis / (np.linalg.norm(axis) + 1e-9)
        # Leichte Neigung: Zilien stehen nie exakt senkrecht
        tilt = rng_local.normal(0.0, 0.18, 3)
        axis = axis + tilt - axis * float(np.dot(axis, tilt))
        axis /= np.linalg.norm(axis) + 1e-9
        helper = np.array([1.0, 0.0, 0.0]) if abs(axis[0]) < 0.9 else np.array([0.0, 1.0, 0.0])
        u_dir = np.cross(helper, axis)
        u_dir /= np.linalg.norm(u_dir) + 1e-9
        v_dir = np.cross(axis, u_dir)
        length = rng_local.uniform(8.5, 11.5)
        phase = rng_local.random()
        start = len(verts)
        for segment in range(segments + 1):
            h = segment / segments
            ring_radius = 0.20 * (1.0 - 0.75 * h)
            center = base + axis * (length * h)
            for side in range(sides):
                angle = 2.0 * math.pi * side / sides
                point = center + (u_dir * math.cos(angle) + v_dir * math.sin(angle)) * ring_radius
                verts.append((point[0] * UM, point[1] * UM, point[2] * UM))
                uvs.append((phase, h))
        for segment in range(segments):
            for side in range(sides):
                a = start + segment * sides + side
                b = start + segment * sides + (side + 1) % sides
                c = start + (segment + 1) * sides + (side + 1) % sides
                d = start + (segment + 1) * sides + side
                faces.append((a, b, c, d))

    cilia_mesh = bpy.data.meshes.new("SM_GEN_OviductCilia")
    cilia_mesh.from_pydata(verts, [], faces)
    cilia_mesh.validate()
    uv_layer = cilia_mesh.uv_layers.new(name="UVMap")
    for poly in cilia_mesh.polygons:
        poly.use_smooth = True
        for corner in range(poly.loop_total):
            loop_index = poly.loop_start + corner
            uv_layer.data[loop_index].uv = uvs[cilia_mesh.loops[loop_index].vertex_index]
    obj = bpy.data.objects.new("SM_GEN_OviductCilia", cilia_mesh)
    bpy.context.scene.collection.objects.link(obj)
    print("GENESIS: Zilien", len(cilia_mesh.vertices), "Vertices", len(cilia_mesh.polygons), "Faces")
    return obj


def decimate(obj, ratio):
    """Dreiecke reduzieren (Nanite braucht die volle Dichte nicht), Randzone der Abschnittsenden dabei unangetastet lassen."""
    if ratio >= 1.0:
        return
    mesh = obj.data
    group = obj.vertex_groups.new(name="Interior")
    count = len(mesh.vertices)
    co = np.empty(count * 3)
    mesh.vertices.foreach_get("co", co)
    xs = co.reshape(-1, 3)[:, 0] / UM
    interior = np.where((xs > 25.0) & (xs < SEGMENT_LENGTH - 25.0))[0]
    group.add(interior.tolist(), 1.0, "REPLACE")

    modifier = obj.modifiers.new("Decimate", "DECIMATE")
    modifier.decimate_type = "COLLAPSE"
    modifier.ratio = ratio
    modifier.vertex_group = group.name
    modifier.use_collapse_triangulate = True
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    print("GENESIS: nach Reduktion", len(obj.data.vertices), "Vertices", len(obj.data.polygons), "Faces")

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
    out_dir = ensure_dir(os.path.abspath(args.get("out", "ArtSource/Generated/Conception")))
    reset_scene()

    parts = [build_wall()]
    for index in range(PRIMARY_FOLDS):
        fold, base_angle, lean, wave = build_primary_fold(index)
        parts.append(fold)
        parts.extend(build_secondary_folds(index, base_angle, lean, wave))
    print("GENESIS: Körper", len(parts))

    wall = union_via_volume(parts)
    trim_and_finish(wall)
    dims = wall.dimensions
    print("GENESIS: Abmessungen µm", round(dims.x / UM, 1), round(dims.y / UM, 1), round(dims.z / UM, 1))

    cilia = build_cilia(wall, spacing_um=float(args.get("cilia_spacing", 6.0)), edge_spacing_um=float(args.get("cilia_edge_spacing", 2.5)))

    if args.get("export"):
        if cilia:
            export_fbx(cilia, os.path.join(out_dir, "SM_GEN_OviductCilia.fbx"))
        decimate(wall, float(args.get("decimate", 0.25)))
        export_fbx(wall, os.path.join(out_dir, "SM_GEN_OviductWall.fbx"))
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out_dir, "OviductWall.blend"))
    if args.get("render"):
        render_lookdev(wall, out_dir, float(args.get("exposure", 0.0)), int(args.get("samples", 256)), float(args.get("energy", 4000.0)))


main()
