# GENESIS: Der Kreislauf des Lebens
# Baut die reife menschliche Eizelle mit Hülle und Corona radiata (SM_GEN_Oocyte*) aus realen Maßen.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Conception/build_oocyte.py -- --out ArtSource/Generated/Conception [--export] [--render]
#
# Anatomie (reife Eizelle, Metaphase II):
#   - Ooplasma (Zellleib) ~110 µm Durchmesser, körnig (Organellen), unter der Membran Cortikalgranula
#   - Perivitelliner Spalt 1–5 µm mit dem ersten Polkörper (~10 µm)
#   - Zona pellucida 13–15 µm dick, gallertig, aus Glykoproteinfasern – daran binden die Spermien
#   - Corona radiata: 2–3 Lagen radial gestreckter Cumuluszellen (10–20 µm) in einer Hyaluronsäure-Matrix,
#     mit Zellfortsätzen, die bis an die Zona reichen
#   - Der gesamte Cumulus-Oozyten-Komplex misst mehrere hundert µm
#
# Maßstab: 1 µm = 0,01 m in Blender = 1 Unreal-Einheit. Ursprung = Mittelpunkt der Eizelle.

import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from genesis_blender_common import (UM, configure_color_management, configure_cycles, enable_gpu, ensure_dir, measure_exposure, new_node,
                                    reset_scene, script_args)
import bmesh
import bpy
import numpy as np
from mathutils import Vector

OOPLASM_RADIUS = 55.0
PERIVITELLINE = 3.0
ZONA_INNER = OOPLASM_RADIUS + PERIVITELLINE
ZONA_THICKNESS = 14.0
POLAR_BODY_RADIUS = 5.0
CORONA_LAYERS = 3

rng = np.random.default_rng(190619)


def value_noise(points, frequency, seed, octaves=3):
    """Deterministische, glatte Summe aus Sinuswellen – für organische Verformungen."""
    generator = np.random.default_rng(seed)
    total = np.zeros(len(points))
    amplitude, norm = 1.0, 0.0
    for octave in range(octaves):
        scale = frequency * (2.0 ** octave)
        directions = generator.normal(size=(6, 3))
        directions /= np.linalg.norm(directions, axis=1, keepdims=True)
        phases = generator.uniform(0.0, 2.0 * math.pi, 6)
        layer = sum(np.sin(points @ direction * scale + phase) for direction, phase in zip(directions, phases)) / 6.0
        total += layer * amplitude
        norm += amplitude
        amplitude *= 0.5
    return total / norm


def sphere(name, radius, subdivisions):
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=radius * UM)
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def read_vertices(obj):
    mesh = obj.data
    data = np.empty(len(mesh.vertices) * 3)
    mesh.vertices.foreach_get("co", data)
    return data.reshape(-1, 3) / UM


def write_vertices(obj, coordinates):
    obj.data.vertices.foreach_set("co", (coordinates * UM).ravel())
    obj.data.update()


def add_uv_and_attribute(obj, radial_fraction=None, tint=None):
    """
    Kugel-UV plus Farbattribut: R = Zufall je Region (oder je Zelle, wenn tint übergeben wird),
    G = radialer Anteil, B = Rauschen.
    """
    mesh = obj.data
    co = read_vertices(obj)
    length = np.maximum(np.linalg.norm(co, axis=1), 1e-6)
    normal = co / length[:, None]
    u = 0.5 + np.arctan2(normal[:, 1], normal[:, 0]) / (2.0 * math.pi)
    v = 0.5 + np.arcsin(np.clip(normal[:, 2], -1.0, 1.0)) / math.pi
    uv_layer = mesh.uv_layers.new(name="UVMap")
    loop_vertices = np.array([loop.vertex_index for loop in mesh.loops])
    uv_layer.data.foreach_set("uv", np.stack([u[loop_vertices], v[loop_vertices]], axis=1).astype(np.float32).ravel())

    noise = 0.5 + 0.5 * value_noise(co / 20.0, 1.0, 77, octaves=4)
    if tint is not None and len(tint) == len(co):
        # Jede Zelle bekommt ihren eigenen Ton – ohne das sehen 950 Zellen aus wie gegossen
        noise = tint
    fraction = radial_fraction if radial_fraction is not None else np.clip(length / 120.0, 0.0, 1.0)
    color = mesh.color_attributes.new(name="Cell", type="FLOAT_COLOR", domain="POINT")
    color.data.foreach_set("color", np.stack([noise, fraction, 1.0 - noise, np.ones(len(co))], axis=1).astype(np.float32).ravel())


def build_ooplasm():
    """Zellleib: nahezu kugelig, mit feiner Unregelmäßigkeit und Mikrovilli-Relief (im Material verfeinert)."""
    obj = sphere("SM_GEN_OocyteCytoplasm", OOPLASM_RADIUS, 6)
    co = read_vertices(obj)
    direction = co / np.linalg.norm(co, axis=1, keepdims=True)
    radius = OOPLASM_RADIUS * (1.0 + 0.012 * value_noise(direction, 2.0, 11) + 0.006 * value_noise(direction, 9.0, 12))
    write_vertices(obj, direction * radius[:, None])
    add_uv_and_attribute(obj, np.ones(len(co)))
    return obj


def build_zona():
    """Zona pellucida als echte Schale mit Dicke – nicht als einzelne Fläche, sonst fehlt die Brechung an den Kanten."""
    obj = sphere("SM_GEN_OocyteZona", 1.0, 6)
    co = read_vertices(obj)
    direction = co / np.linalg.norm(co, axis=1, keepdims=True)
    outer = (ZONA_INNER + ZONA_THICKNESS) * (1.0 + 0.010 * value_noise(direction, 2.5, 21) + 0.005 * value_noise(direction, 12.0, 22))
    write_vertices(obj, direction * outer[:, None])

    modifier = obj.modifiers.new("Thickness", "SOLIDIFY")
    modifier.thickness = ZONA_THICKNESS * UM
    modifier.offset = -1.0
    modifier.use_even_offset = True
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.select_set(False)
    co = read_vertices(obj)
    fraction = np.clip((np.linalg.norm(co, axis=1) - ZONA_INNER) / ZONA_THICKNESS, 0.0, 1.0)
    add_uv_and_attribute(obj, fraction)
    return obj


def build_polar_body():
    """Erster Polkörper im perivitellinen Spalt – das Nebenprodukt der Reifeteilung."""
    obj = sphere("SM_GEN_OocytePolarBody", POLAR_BODY_RADIUS, 3)
    co = read_vertices(obj)
    direction = co / np.linalg.norm(co, axis=1, keepdims=True)
    radius = POLAR_BODY_RADIUS * (1.0 + 0.10 * value_noise(direction, 3.0, 31))
    position = np.array([0.0, 0.0, OOPLASM_RADIUS + PERIVITELLINE - POLAR_BODY_RADIUS * 0.6])
    write_vertices(obj, direction * radius[:, None] + position)
    add_uv_and_attribute(obj, np.ones(len(co)))
    return obj


def build_corona():
    """
    Corona radiata: radial gestreckte Cumuluszellen in mehreren Lagen, dazwischen Lücken (Matrix).
    Die inneren Zellen schicken Fortsätze bis an die Zona – daran erkennt man den Komplex.
    """
    cells = bmesh.new()
    count = 0
    centers = np.zeros((0, 3))
    tints = []
    attempts = 0
    while count < 2100 and attempts < 400000:
        attempts += 1
        direction = rng.normal(size=3)
        direction /= np.linalg.norm(direction)
        # Fünf sich überlappende Lagen: Die Zellen liegen dicht gepackt wie bei einer Brombeere und berühren sich.
        # Eine einzelne Schale aus abstehenden Zellen sieht aus wie ein Seeigel, nicht wie ein Cumulus.
        layer = int(rng.choice([0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 4]))
        distance = ZONA_INNER + ZONA_THICKNESS + 6.0 + layer * 10.5 + rng.normal(0.0, 3.0)
        # Lücken in der Matrix: nicht überall sitzen Zellen, nach außen immer weniger
        if 0.5 + 0.5 * value_noise(direction[None, :], 1.7, 41)[0] < 0.10 + 0.17 * layer:
            continue
        position = direction * distance
        # Abstand kleiner als die Zellgröße: Die Hüllen überlappen und verschmelzen optisch zu einer Masse
        if len(centers) and np.min(np.linalg.norm(centers - position, axis=1)) < 6.2:
            continue
        centers = np.vstack([centers, position])

        # Kaum noch gestreckt: Cumuluszellen sind rundlich-polygonal, nur die innerste Lage steht radial
        long_axis = rng.uniform(8.5, 13.0)
        cross = rng.uniform(6.5, 10.0)
        temp = bmesh.new()
        # Unterteilung 3: Lebende Zellen haben keine Facetten. Bei 80 Flächen je Zelle bleiben harte Kanten sichtbar.
        bmesh.ops.create_icosphere(temp, subdivisions=3, radius=1.0)
        local = np.array([vert.co[:] for vert in temp.verts])
        local *= np.array([long_axis * 0.5, cross * 0.5, cross * 0.5 * rng.uniform(0.85, 1.05)])
        local *= (1.0 + 0.20 * value_noise(local / cross, 0.7, int(rng.integers(1e6))))[:, None]

        # Nur die innerste Lage richtet sich radial aus (ihre Fortsätze reichen zur Zona);
        # weiter außen liegen die Zellen fast beliebig – sonst entsteht ein Strahlenkranz
        jitter = rng.normal(size=3) * (0.35 + 0.45 * layer)
        x_axis = direction + jitter
        x_axis /= np.linalg.norm(x_axis)
        helper = np.array([0.0, 0.0, 1.0]) if abs(x_axis[2]) < 0.9 else np.array([1.0, 0.0, 0.0])
        y_axis = np.cross(helper, x_axis)
        y_axis /= np.linalg.norm(y_axis)
        z_axis = np.cross(x_axis, y_axis)
        roll = rng.uniform(0.0, 2.0 * math.pi)
        y_axis, z_axis = y_axis * math.cos(roll) + z_axis * math.sin(roll), -y_axis * math.sin(roll) + z_axis * math.cos(roll)
        world = local @ np.stack([x_axis, y_axis, z_axis]) + position

        mesh_temp = bpy.data.meshes.new("tmp")
        temp.verts.ensure_lookup_table()
        for vert, point in zip(temp.verts, world):
            vert.co = Vector(point * UM)
        temp.to_mesh(mesh_temp)
        temp.free()
        cell_tint = float(rng.uniform(0.15, 1.0))
        tints.extend([cell_tint] * len(mesh_temp.vertices))
        cells.from_mesh(mesh_temp)
        bpy.data.meshes.remove(mesh_temp)

        # Fortsatz der innersten Lage zur Zona: dünner Kegel
        if layer == 0 and rng.random() < 0.75:
            process = bmesh.new()
            tip = direction * (ZONA_INNER + ZONA_THICKNESS * 0.35)
            base = position - direction * long_axis * 0.4
            axis = tip - base
            height = np.linalg.norm(axis)
            axis_dir = axis / max(height, 1e-6)
            ring_vertices = []
            for segment in range(6):
                ring = []
                fraction = segment / 5.0
                radius_here = 1.5 * (1.0 - 0.75 * fraction)
                center = base + axis_dir * height * fraction
                for side in range(5):
                    angle = 2.0 * math.pi * side / 5.0
                    offset = (y_axis * math.cos(angle) + z_axis * math.sin(angle)) * radius_here
                    ring.append(process.verts.new(Vector((center + offset) * UM)))
                ring_vertices.append(ring)
            for lower, upper in zip(ring_vertices[:-1], ring_vertices[1:]):
                for side in range(5):
                    other = (side + 1) % 5
                    process.faces.new((lower[side], lower[other], upper[other], upper[side]))
            bmesh.ops.recalc_face_normals(process, faces=process.faces)
            mesh_process = bpy.data.meshes.new("tmp_process")
            process.to_mesh(mesh_process)
            process.free()
            tints.extend([cell_tint] * len(mesh_process.vertices))
            cells.from_mesh(mesh_process)
            bpy.data.meshes.remove(mesh_process)
        count += 1

    mesh = bpy.data.meshes.new("SM_GEN_OocyteCorona")
    cells.to_mesh(mesh)
    cells.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new("SM_GEN_OocyteCorona", mesh)
    bpy.context.scene.collection.objects.link(obj)
    co = read_vertices(obj)
    add_uv_and_attribute(obj, np.clip((np.linalg.norm(co, axis=1) - ZONA_INNER - ZONA_THICKNESS) / 60.0, 0.0, 1.0),
                         tint=np.array(tints))
    print("GENESIS: Corona-Zellen", count, "Farbwerte", len(tints), "Vertices", len(co))
    return obj


def build_matrix():
    """
    Hyaluronsäure-Matrix des Cumulus: eine gallertige, fast klare Masse, in der die Zellen stecken.
    Ohne sie sähe der Komplex aus wie lose schwebende Kugeln. Sie streut Licht und macht den Komplex wolkig.
    """
    obj = sphere("SM_GEN_OocyteMatrix", 1.0, 5)
    co = read_vertices(obj)
    direction = co / np.linalg.norm(co, axis=1, keepdims=True)
    # Die Gallerte muss die äußerste Zelllage einschließen (bis ~128 µm), sonst ragen Zellen sichtbar aus der Wolke
    radius = 118.0 * (1.0 + 0.16 * value_noise(direction, 1.2, 51) + 0.08 * value_noise(direction, 3.5, 52))
    write_vertices(obj, direction * radius[:, None])
    add_uv_and_attribute(obj, np.clip((radius - ZONA_INNER - ZONA_THICKNESS) / 60.0, 0.0, 1.0))
    return obj


def export_fbx(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE", use_tspace=False,
                             colors_type="LINEAR", add_leaf_bones=False)
    print("GENESIS: exportiert", os.path.basename(path), round(os.path.getsize(path) / 1e6, 1), "MB")


def make_material(name, base_color, subsurface_radius, subsurface_scale, transmission, roughness, ior=1.04):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = new_node(tree, "ShaderNodeOutputMaterial", (600, 0))
    bsdf = new_node(tree, "ShaderNodeBsdfPrincipled", (300, 0))
    bsdf.inputs["Base Color"].default_value = base_color
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["IOR"].default_value = ior
    bsdf.inputs["Transmission Weight"].default_value = transmission
    bsdf.inputs["Subsurface Weight"].default_value = 1.0 - transmission
    bsdf.inputs["Subsurface Radius"].default_value = subsurface_radius
    bsdf.inputs["Subsurface Scale"].default_value = subsurface_scale * UM
    coords = new_node(tree, "ShaderNodeTexCoord", (-600, -300))
    grain = new_node(tree, "ShaderNodeTexNoise", (-400, -300))
    grain.inputs["Scale"].default_value = 1.0 / (1.2 * UM)
    tree.links.new(coords.outputs["Object"], grain.inputs["Vector"])
    bump = new_node(tree, "ShaderNodeBump", (0, -300))
    bump.inputs["Strength"].default_value = 0.25
    bump.inputs["Distance"].default_value = 0.3 * UM
    tree.links.new(grain.outputs["Fac"], bump.inputs["Height"])
    tree.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    tree.links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])
    return material


def render_lookdev(objects, out_dir, exposure, samples, energy):
    scene = bpy.context.scene
    materials = {
        "SM_GEN_OocyteCytoplasm": make_material("M_Ooplasm", (0.42, 0.34, 0.26, 1.0), (1.4, 0.9, 0.6), 5.0, 0.10, 0.32),
        "SM_GEN_OocyteZona": make_material("M_Zona", (0.86, 0.80, 0.68, 1.0), (2.0, 1.8, 1.6), 25.0, 0.88, 0.10, ior=1.035),
        "SM_GEN_OocyteCorona": make_material("M_Corona", (0.80, 0.76, 0.70, 1.0), (1.2, 0.8, 0.6), 8.0, 0.86, 0.16, ior=1.045),
        "SM_GEN_OocyteMatrix": make_material("M_Matrix", (0.96, 0.96, 0.95, 1.0), (4.0, 4.0, 4.0), 160.0, 0.995, 0.05, ior=1.008),
        "SM_GEN_OocytePolarBody": make_material("M_PolarBody", (0.62, 0.58, 0.54, 1.0), (1.0, 0.7, 0.5), 3.0, 0.25, 0.30),
    }
    for obj in objects:
        obj.data.materials.clear()
        obj.data.materials.append(materials[obj.name])

    enable_gpu(scene)
    configure_color_management(scene, exposure=exposure)
    configure_cycles(scene, samples=samples, width=1280, height=720)

    world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes.clear()
    background = new_node(world.node_tree, "ShaderNodeBackground", (0, 0))
    background.inputs["Strength"].default_value = 0.0
    world_output = new_node(world.node_tree, "ShaderNodeOutputWorld", (300, 0))
    world.node_tree.links.new(background.outputs[0], world_output.inputs[0])

    camera_data = bpy.data.cameras.new("Camera")
    camera_data.sensor_width = 36.0
    camera_data.lens = 50.0
    camera_data.clip_start = 0.5 * UM
    camera_data.dof.use_dof = True
    camera_data.dof.aperture_fstop = 11.0
    camera = bpy.data.objects.new("Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.location = Vector((320.0, -230.0, 120.0)) * UM
    target = Vector((0.0, 0.0, 0.0))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.dof.focus_distance = (target - camera.location).length

    light_data = bpy.data.lights.new("EndoscopeLight", "SPOT")
    light_data.energy = energy
    light_data.spot_size = math.radians(80.0)
    light_data.spot_blend = 0.5
    light_data.shadow_soft_size = 1.5 * UM
    light = bpy.data.objects.new("EndoscopeLight", light_data)
    scene.collection.objects.link(light)
    light.parent = camera
    light.location = (2.0 * UM, -1.0 * UM, 0.0)

    scene.render.filepath = os.path.join(out_dir, "LookDev_Oocyte.png")
    bpy.ops.render.render(write_still=True)
    measure_exposure(scene.render.filepath)


def main():
    args = script_args()
    out_dir = ensure_dir(os.path.abspath(args.get("out", "ArtSource/Generated/Conception")))
    reset_scene()

    ooplasm = build_ooplasm()
    zona = build_zona()
    polar_body = build_polar_body()
    corona = build_corona()
    matrix = build_matrix()
    for obj in (ooplasm, zona, polar_body, corona, matrix):
        dimensions = obj.dimensions
        print("GENESIS: %-28s %6.1f × %6.1f × %6.1f µm, %d Flächen" % (
            obj.name, dimensions.x / UM, dimensions.y / UM, dimensions.z / UM, len(obj.data.polygons)))

    if args.get("export"):
        for obj in (ooplasm, zona, polar_body, corona, matrix):
            export_fbx(obj, os.path.join(out_dir, obj.name + ".fbx"))
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out_dir, "Oocyte.blend"))
    if args.get("render"):
        render_lookdev([ooplasm, zona, polar_body, corona, matrix], out_dir, float(args.get("exposure", 0.0)),
                       int(args.get("samples", 256)), float(args.get("energy", 30000.0)))


main()
