# GENESIS: Der Kreislauf des Lebens
# Baut die menschliche Spermienzelle (SM_GEN_SpermCell) aus realen Maßen, rendert Look-Dev-Bilder und exportiert FBX.
#
# Aufruf:
#   blender -b --factory-startup -P Tools/Blender/Conception/build_sperm_cell.py -- --out ArtSource/Generated/Conception [--render] [--export]
#
# Referenzmaße (WHO-Laborhandbuch, menschliche Spermien, Mittelwerte):
#   Kopf 4,6 µm lang, 2,9 µm breit, in der Seitenansicht birnenförmig (Spitze dünn, Basis dicker), Akrosom ~40–70 % des Kopfes
#   Hals ~1 µm, Mittelstück ~5 µm (Ø ~0,9 µm, spiralige Mitochondrienscheide, ~12 Windungen), Anulus als Einschnürung
#   Hauptstück ~45 µm (Ø ~0,6 → 0,35 µm), Endstück ~5 µm (Ø → ~0,06 µm). Gesamt ~60 µm.
#
# Ausrichtung: Schwimmrichtung +X, Kopfspitze im Ursprung, Schwanz entlang −X. Flache Kopfseite zeigt nach ±Z.
# Farbattribut "Zones": R Akrosom, G Mittelstück, B Kopf (Kern), A Position entlang der Zelle 0 (Spitze) … 1 (Schwanzende).
# Das Schlagen der Geißel entsteht in Unreal per World Position Offset aus A.

import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from genesis_blender_common import (UM, configure_color_management, configure_cycles, enable_gpu, ensure_dir, lerp, measure_exposure,
                                    new_node, reset_scene, script_args, smoothstep)
import bpy

HEAD_LENGTH = 4.6
NECK_END = 5.6
MIDPIECE_END = 10.6
PRINCIPAL_END = 55.6
TOTAL_LENGTH = 60.6
RING_SEGMENTS = 40


def head_half_width(s):
    """Aufsicht: ovaler Kopf, breiteste Stelle leicht vor der Mitte."""
    if s <= 0.0 or s >= HEAD_LENGTH:
        return 0.0
    u = s / HEAD_LENGTH
    return 1.45 * math.sin(math.pi * u ** 0.85) ** 0.6


def head_half_thickness(s):
    """Seitenansicht: dünne Spitze, dickere Basis (birnenförmig)."""
    if s <= 0.0 or s >= HEAD_LENGTH:
        return 0.0
    u = s / HEAD_LENGTH
    return (0.36 + 0.46 * u) * math.sin(math.pi * u ** 0.85) ** 0.5


def tail_radius(s):
    if s < 3.9:
        return 0.0
    if s < NECK_END:
        return 0.40 * smoothstep(3.9, 4.8, s)
    if s < MIDPIECE_END:
        # Mittelstück, am Anulus eingeschnürt
        return lerp(0.40, 0.46, smoothstep(NECK_END, NECK_END + 0.6, s)) * (1.0 - 0.22 * smoothstep(MIDPIECE_END - 0.35, MIDPIECE_END, s))
    if s < PRINCIPAL_END:
        t = (s - MIDPIECE_END) / (PRINCIPAL_END - MIDPIECE_END)
        start = 0.46 * 0.78
        return lerp(start, 0.30, smoothstep(0.0, 0.08, t)) * (1.0 - 0.42 * t)
    t = (s - PRINCIPAL_END) / (TOTAL_LENGTH - PRINCIPAL_END)
    return lerp(0.30 * 0.58, 0.03, t ** 0.8)


def pnorm(a, b, p=4.0):
    return (a ** p + b ** p) ** (1.0 / p)


def midpiece_helix(s, theta):
    """Spiralige Mitochondrienscheide: Rippen, ~12 Windungen über das Mittelstück."""
    mask = smoothstep(NECK_END + 0.2, NECK_END + 0.6, s) * (1.0 - smoothstep(MIDPIECE_END - 0.4, MIDPIECE_END - 0.1, s))
    if mask <= 0.0:
        return 0.0
    pitch = (MIDPIECE_END - NECK_END) / 12.0
    phase = 2.0 * math.pi * s / pitch + theta
    gyre = math.floor(phase / (2.0 * math.pi))
    # Jede Windung besteht aus einzelnen, leicht unterschiedlichen Mitochondrien – keine perfekte Gewindespirale
    unit = 0.65 + 0.35 * math.sin(2.0 * theta + gyre * 1.7) * math.sin(gyre * 2.3 + 0.4)
    ridge = 0.5 + 0.5 * math.sin(phase)
    return mask * 0.018 * unit * ridge ** 6


def organic_variation(s, theta):
    """Keine CAD-Oberfläche: leise, deterministische Unebenheit (±2 %), am Kopf leicht asymmetrisch."""
    value = 0.012 * math.sin(3.0 * theta + s * 1.9) + 0.008 * math.sin(5.0 * theta - s * 3.7 + 1.3) + 0.006 * math.sin(s * 0.9 + 0.7)
    if s < HEAD_LENGTH:
        value += 0.02 * math.cos(theta) * math.sin(math.pi * s / HEAD_LENGTH)
        # Akrosomkappe: Sie liegt als Haube über den vorderen ~55 % des Kopfes und steht minimal vor
        value += 0.035 * (1.0 - smoothstep(2.1, 2.6, s))
        # Äquatorialsegment: deutliche Rinne am Übergang Akrosom → postakrosomale Region
        value -= 0.055 * math.exp(-((s - 2.62) / 0.16) ** 2)
    return value


def head_face_profile(s, theta):
    """Die flachen Kopfseiten sind in der postakrosomalen Region leicht eingedellt."""
    if s <= 1.2 or s >= HEAD_LENGTH:
        return 1.0
    region = smoothstep(1.2, 2.6, s) * (1.0 - smoothstep(3.6, HEAD_LENGTH, s))
    return 1.0 - 0.07 * region * abs(math.sin(theta)) ** 10


def stations():
    values = []
    # Kopf: dicht an der Spitze
    for i in range(1, 110):
        values.append(HEAD_LENGTH * (i / 110.0) ** 1.5)
    s = HEAD_LENGTH
    while s < MIDPIECE_END + 0.5:
        values.append(s)
        s += 0.035  # >= 10 Stationen je Mitochondrien-Windung
    while s < PRINCIPAL_END:
        values.append(s)
        s += 0.22
    while s < TOTAL_LENGTH:
        values.append(s)
        s += 0.12
    return values


def build_mesh(name, lateral_wave=None):
    verts = []
    faces = []
    uvs = []
    zones = []
    ring_starts = []

    # Pol an der Kopfspitze
    verts.append((0.0, 0.0, 0.0))
    tip_index = 0

    station_list = stations()
    for s in station_list:
        a_y = pnorm(head_half_width(s), tail_radius(s))
        a_z = pnorm(head_half_thickness(s), tail_radius(s))
        offset_y = lateral_wave(s) if lateral_wave else 0.0
        ring_starts.append(len(verts))
        for j in range(RING_SEGMENTS):
            theta = 2.0 * math.pi * j / RING_SEGMENTS
            helix = midpiece_helix(s, theta)
            organic = 1.0 + organic_variation(s, theta)
            radius_scale_y = (a_y + helix) * organic
            radius_scale_z = (a_z + helix) * organic * head_face_profile(s, theta)
            verts.append((-s * UM, (math.cos(theta) * radius_scale_y + offset_y) * UM, math.sin(theta) * radius_scale_z * UM))

    end_index = len(verts)
    last_offset = lateral_wave(TOTAL_LENGTH) if lateral_wave else 0.0
    verts.append((-TOTAL_LENGTH * UM, last_offset * UM, 0.0))

    def zone(s):
        acrosome = 1.0 - smoothstep(2.2, 2.7, s)
        midpiece = smoothstep(NECK_END - 0.2, NECK_END + 0.2, s) * (1.0 - smoothstep(MIDPIECE_END - 0.2, MIDPIECE_END + 0.1, s))
        head = 1.0 - smoothstep(HEAD_LENGTH - 0.6, HEAD_LENGTH + 0.2, s)
        return (acrosome, midpiece, head, s / TOTAL_LENGTH)

    face_uvs = []
    face_zones = []
    first = ring_starts[0]
    for j in range(RING_SEGMENTS):
        k = (j + 1) % RING_SEGMENTS
        faces.append((tip_index, first + k, first + j))
        face_uvs.append(((j + 0.5) / RING_SEGMENTS, 0.0, (j + 1) / RING_SEGMENTS, station_list[0] / TOTAL_LENGTH, j / RING_SEGMENTS, station_list[0] / TOTAL_LENGTH))
        face_zones.append((zone(0.0), zone(station_list[0]), zone(station_list[0])))

    for r in range(len(station_list) - 1):
        a = ring_starts[r]
        b = ring_starts[r + 1]
        va = station_list[r] / TOTAL_LENGTH
        vb = station_list[r + 1] / TOTAL_LENGTH
        for j in range(RING_SEGMENTS):
            k = (j + 1) % RING_SEGMENTS
            faces.append((a + j, a + k, b + k, b + j))
            u0 = j / RING_SEGMENTS
            u1 = (j + 1) / RING_SEGMENTS
            face_uvs.append((u0, va, u1, va, u1, vb, u0, vb))
            za = zone(station_list[r])
            zb = zone(station_list[r + 1])
            face_zones.append((za, za, zb, zb))

    last = ring_starts[-1]
    for j in range(RING_SEGMENTS):
        k = (j + 1) % RING_SEGMENTS
        faces.append((last + j, last + k, end_index))
        vl = station_list[-1] / TOTAL_LENGTH
        face_uvs.append((j / RING_SEGMENTS, vl, (j + 1) / RING_SEGMENTS, vl, (j + 0.5) / RING_SEGMENTS, 1.0))
        face_zones.append((zone(station_list[-1]), zone(station_list[-1]), zone(TOTAL_LENGTH)))

    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.validate()

    uv_layer = mesh.uv_layers.new(name="UVMap")
    color = mesh.color_attributes.new(name="Zones", type="FLOAT_COLOR", domain="CORNER")
    loop = 0
    for poly_index, poly in enumerate(mesh.polygons):
        uv = face_uvs[poly_index]
        zc = face_zones[poly_index]
        for corner in range(poly.loop_total):
            uv_layer.data[poly.loop_start + corner].uv = (uv[corner * 2], uv[corner * 2 + 1])
            z = zc[corner]
            color.data[poly.loop_start + corner].color = (z[0], z[1], z[2], z[3])
        loop += poly.loop_total

    # Normalen konsistent nach außen (Wicklung der Pol-Dreiecke)
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()

    for poly in mesh.polygons:
        poly.use_smooth = True

    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def make_material():
    """
    Referenz-Material (Wahrheit für den Unreal-Abgleich).

    Physik: Die Zelle schwimmt in Eileiterflüssigkeit (n ≈ 1,335), Zytoplasma/Membran n ≈ 1,36–1,40.
    Relativer Brechungsindex ≈ 1,04 → Fresnel-Reflexion unter 0,1 %: Die Zelle glänzt kaum, sie ist glasig.
    Sichtbar wird sie durch Streuung im Inneren: dicht gepacktes Chromatin im Kern streut am stärksten,
    das Mittelstück (Mitochondrien) streut und absorbiert leicht gelblich, die Geißel ist fast durchsichtig.
    Zonen aus Objektkoordinaten (funktionieren auch im Volumen-Shader, anders als Farbattribute).
    """
    mat = bpy.data.materials.new("M_GEN_SpermCell_LookDev")
    mat.use_nodes = True
    tree = mat.node_tree
    tree.nodes.clear()

    out = new_node(tree, "ShaderNodeOutputMaterial", (1200, 0))

    # s (µm von der Kopfspitze) = −x / UM
    texcoord = new_node(tree, "ShaderNodeTexCoord", (-1400, 0))
    separate = new_node(tree, "ShaderNodeSeparateXYZ", (-1200, 0))
    tree.links.new(texcoord.outputs["Object"], separate.inputs["Vector"])
    to_um = new_node(tree, "ShaderNodeMath", (-1000, 0), operation="MULTIPLY")
    to_um.inputs[1].default_value = -1.0 / UM
    tree.links.new(separate.outputs["X"], to_um.inputs[0])

    def band(label, start_in, start_full, end_full, end_out, y):
        """Weiche Zonenmaske über s."""
        rise = new_node(tree, "ShaderNodeMapRange", (-800, y), interpolation_type="SMOOTHSTEP", label=label)
        rise.inputs["From Min"].default_value = start_in
        rise.inputs["From Max"].default_value = start_full
        tree.links.new(to_um.outputs[0], rise.inputs["Value"])
        fall = new_node(tree, "ShaderNodeMapRange", (-800, y - 220), interpolation_type="SMOOTHSTEP")
        fall.inputs["From Min"].default_value = end_full
        fall.inputs["From Max"].default_value = end_out
        fall.inputs["To Min"].default_value = 1.0
        fall.inputs["To Max"].default_value = 0.0
        tree.links.new(to_um.outputs[0], fall.inputs["Value"])
        product = new_node(tree, "ShaderNodeMath", (-600, y - 100), operation="MULTIPLY")
        tree.links.new(rise.outputs["Result"], product.inputs[0])
        tree.links.new(fall.outputs["Result"], product.inputs[1])
        return product.outputs[0]

    # Weiche, anatomische Dichtezonen (µm ab Kopfspitze) – keine harten Bauteilgrenzen
    head = band("Kopf", -1.0, 0.0, 4.2, 5.4, 700)
    post_acrosomal = band("Postakrosomal", 2.3, 2.8, 3.9, 4.6, 450)
    nucleus_core = band("Kernkern", 0.8, 1.8, 3.4, 4.3, 250)
    midpiece = band("Mittelstück", NECK_END - 0.9, NECK_END + 0.5, MIDPIECE_END - 0.5, MIDPIECE_END + 0.9, -300)

    # Chromatin ist nicht homogen
    noise = new_node(tree, "ShaderNodeTexNoise", (-800, -800))
    noise.inputs["Scale"].default_value = 260.0
    noise.inputs["Detail"].default_value = 4.0
    tree.links.new(texcoord.outputs["Object"], noise.inputs["Vector"])
    noise_range = new_node(tree, "ShaderNodeMapRange", (-600, -800))
    noise_range.inputs["To Min"].default_value = 0.8
    noise_range.inputs["To Max"].default_value = 1.15
    tree.links.new(noise.outputs["Fac"], noise_range.inputs["Value"])

    # Kleine Kernvakuolen (häufig bei menschlichen Spermien): wenige Zellen des Voronoi-Rasters bekommen eine Aufhellung
    voronoi = new_node(tree, "ShaderNodeTexVoronoi", (-800, -1100))
    voronoi.inputs["Scale"].default_value = 140.0
    tree.links.new(texcoord.outputs["Object"], voronoi.inputs["Vector"])
    vacuole_size = new_node(tree, "ShaderNodeMapRange", (-600, -1050), interpolation_type="SMOOTHSTEP")
    vacuole_size.inputs["From Min"].default_value = 0.12
    vacuole_size.inputs["From Max"].default_value = 0.22
    vacuole_size.inputs["To Min"].default_value = 1.0
    vacuole_size.inputs["To Max"].default_value = 0.0
    tree.links.new(voronoi.outputs["Distance"], vacuole_size.inputs["Value"])
    vacuole_pick = new_node(tree, "ShaderNodeSeparateColor", (-600, -1250))
    tree.links.new(voronoi.outputs["Color"], vacuole_pick.inputs["Color"])
    vacuole_rare = new_node(tree, "ShaderNodeMath", (-450, -1250), operation="GREATER_THAN")
    vacuole_rare.inputs[1].default_value = 0.82
    tree.links.new(vacuole_pick.outputs["Red"], vacuole_rare.inputs[0])
    vacuole = new_node(tree, "ShaderNodeMath", (-300, -1100), operation="MULTIPLY")
    tree.links.new(vacuole_size.outputs["Result"], vacuole.inputs[0])
    tree.links.new(vacuole_rare.outputs[0], vacuole.inputs[1])
    vacuole_in_head = new_node(tree, "ShaderNodeMath", (-150, -1100), operation="MULTIPLY")
    tree.links.new(vacuole.outputs[0], vacuole_in_head.inputs[0])
    tree.links.new(nucleus_core, vacuole_in_head.inputs[1])
    vacuole_keep = new_node(tree, "ShaderNodeMapRange", (0, -1100))
    vacuole_keep.inputs["To Min"].default_value = 1.0
    vacuole_keep.inputs["To Max"].default_value = 0.25
    tree.links.new(vacuole_in_head.outputs[0], vacuole_keep.inputs["Value"])

    # Dichte je Meter Blender (= je 100 µm). Optische Tiefe: Kopf ~0,5, Postakrosomal/Kern ~1,3, Mittelstück ~0,6, Geißel ~0,05
    def scaled(value_node, factor, y):
        node = new_node(tree, "ShaderNodeMath", (-400, y), operation="MULTIPLY")
        node.inputs[1].default_value = factor
        tree.links.new(value_node, node.inputs[0])
        return node.outputs[0]

    def add(a, b, y):
        node = new_node(tree, "ShaderNodeMath", (-200, y), operation="ADD")
        tree.links.new(a, node.inputs[0])
        if isinstance(b, float):
            node.inputs[1].default_value = b
        else:
            tree.links.new(b, node.inputs[1])
        return node.outputs[0]

    total = add(scaled(head, 40.0, 700), 9.0, 650)
    total = add(total, scaled(post_acrosomal, 55.0, 450), 450)
    total = add(total, scaled(nucleus_core, 30.0, 250), 250)
    total = add(total, scaled(midpiece, 60.0, -300), -250)
    with_chromatin = new_node(tree, "ShaderNodeMath", (0, 250), operation="MULTIPLY")
    tree.links.new(total, with_chromatin.inputs[0])
    tree.links.new(noise_range.outputs["Result"], with_chromatin.inputs[1])
    varied = new_node(tree, "ShaderNodeMath", (250, 250), operation="MULTIPLY")
    tree.links.new(with_chromatin.outputs[0], varied.inputs[0])
    tree.links.new(vacuole_keep.outputs["Result"], varied.inputs[1])

    volume = new_node(tree, "ShaderNodeVolumePrincipled", (700, -250))
    volume.inputs["Color"].default_value = (0.93, 0.92, 0.89, 1.0)
    volume.inputs["Anisotropy"].default_value = 0.55  # biologisches Gewebe streut vorwärts
    tree.links.new(varied.outputs[0], volume.inputs["Density"])
    # Mitochondrien absorbieren leicht (Cytochrome) – gelblich
    absorption = new_node(tree, "ShaderNodeMix", (500, -500), data_type="RGBA")
    absorption.inputs["A"].default_value = (0.02, 0.02, 0.025, 1.0)
    absorption.inputs["B"].default_value = (0.05, 0.12, 0.30, 1.0)
    tree.links.new(midpiece, absorption.inputs["Factor"])
    tree.links.new(absorption.outputs["Result"], volume.inputs["Absorption Color"])

    surface = new_node(tree, "ShaderNodeBsdfPrincipled", (700, 200))
    surface.inputs["Base Color"].default_value = (0.95, 0.95, 0.94, 1.0)
    surface.inputs["Transmission Weight"].default_value = 1.0
    surface.inputs["IOR"].default_value = 1.04  # relativ zur Flüssigkeit
    rough_noise = new_node(tree, "ShaderNodeTexNoise", (100, -50))
    rough_noise.inputs["Scale"].default_value = 140.0
    tree.links.new(texcoord.outputs["Object"], rough_noise.inputs["Vector"])
    rough_range = new_node(tree, "ShaderNodeMapRange", (300, -50))
    rough_range.inputs["To Min"].default_value = 0.06
    rough_range.inputs["To Max"].default_value = 0.2
    tree.links.new(rough_noise.outputs["Fac"], rough_range.inputs["Value"])
    tree.links.new(rough_range.outputs["Result"], surface.inputs["Roughness"])

    tree.links.new(surface.outputs["BSDF"], out.inputs["Surface"])
    tree.links.new(volume.outputs["Volume"], out.inputs["Volume"])
    return mat


def swim_wave(s):
    """Nur für das Look-Dev-Bild: Geißelschlag in Momentaufnahme (Amplitude wächst zur Spitze, Wellenlänge ~ halbe Zelllänge)."""
    if s < NECK_END:
        return 0.0
    t = (s - NECK_END) / (TOTAL_LENGTH - NECK_END)
    return 4.0 * t ** 1.3 * math.sin(2.0 * math.pi * (s - NECK_END) / 30.0)


def setup_lookdev_scene(scene, obj, mode):
    world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True
    tree = world.node_tree
    tree.nodes.clear()
    background = new_node(tree, "ShaderNodeBackground", (0, 0))
    output = new_node(tree, "ShaderNodeOutputWorld", (300, 0))
    tree.links.new(background.outputs["Background"], output.inputs["Surface"])

    camera_data = bpy.data.cameras.new("Camera")
    camera_data.sensor_fit = "HORIZONTAL"
    camera_data.sensor_width = 36.0
    camera = bpy.data.objects.new("Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera

    if mode == "neutral":
        # Neutrale Materialprüfung: gleichmäßiges graues Umgebungslicht + große weiche Fläche
        background.inputs["Color"].default_value = (0.18, 0.18, 0.18, 1.0)
        background.inputs["Strength"].default_value = 1.0
        camera_data.lens = 50.0
        camera_data.dof.use_dof = True
        camera_data.dof.aperture_fstop = 11.0
        camera.location = (-0.30, -1.35, 0.35)
        target = (-0.30, 0.0, 0.0)
    else:
        # Endoskop: einzige Lichtquelle sitzt an der Kamera, dunkle Umgebung (im Körper gibt es kein Licht)
        background.inputs["Color"].default_value = (0.0, 0.0, 0.0, 1.0)
        background.inputs["Strength"].default_value = 0.0
        camera_data.lens = 100.0
        camera_data.dof.use_dof = True
        camera_data.dof.aperture_fstop = 5.6
        camera.location = (0.10, -0.32, 0.12)
        target = (-0.06, 0.0, 0.0)
        light_data = bpy.data.lights.new("EndoscopeLight", "AREA")
        light_data.shape = "DISK"
        light_data.size = 0.02
        light_data.energy = 3.0
        light_data.color = (1.0, 0.97, 0.93)  # weiße LED ~5.600 K
        light = bpy.data.objects.new("EndoscopeLight", light_data)
        scene.collection.objects.link(light)
        light.parent = camera
        light.location = (0.015, 0.0, 0.0)

    direction = [target[i] - camera.location[i] for i in range(3)]
    from mathutils import Vector
    camera.rotation_euler = Vector(direction).to_track_quat("-Z", "Y").to_euler()
    camera_data.dof.focus_distance = Vector(direction).length

    if mode == "neutral":
        key_data = bpy.data.lights.new("SoftBox", "AREA")
        key_data.size = 1.5
        key_data.energy = 60.0
        key = bpy.data.objects.new("SoftBox", key_data)
        scene.collection.objects.link(key)
        key.location = (0.2, -1.2, 1.4)
        key.rotation_euler = Vector((-0.2, 1.2, -1.4)).to_track_quat("-Z", "Y").to_euler()
        key.visible_camera = False


def export_fbx(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                             bake_space_transform=False, object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE",
                             use_tspace=True, colors_type="LINEAR", add_leaf_bones=False, path_mode="AUTO")
    print("GENESIS: exportiert", path)


def main():
    args = script_args()
    out_dir = ensure_dir(os.path.abspath(args.get("out", "ArtSource/Generated/Conception")))

    if args.get("export"):
        scene = reset_scene()
        obj = build_mesh("SM_GEN_SpermCell")
        print("GENESIS: Vertices", len(obj.data.vertices), "Faces", len(obj.data.polygons))
        dims = obj.dimensions
        print("GENESIS: Abmessungen µm", round(dims.x / UM, 2), round(dims.y / UM, 2), round(dims.z / UM, 2))
        export_fbx(obj, os.path.join(out_dir, "SM_GEN_SpermCell.fbx"))

    if args.get("render"):
        for mode in ("neutral", "endoscope"):
            scene = reset_scene()
            obj = build_mesh("SpermCell_LookDev", lateral_wave=swim_wave)
            obj.data.materials.append(make_material())
            enable_gpu(scene)
            configure_color_management(scene, exposure=float(args.get("exposure_" + mode, 0.0)))
            configure_cycles(scene, samples=int(args.get("samples", 384)))
            setup_lookdev_scene(scene, obj, mode)
            scene.render.filepath = os.path.join(out_dir, f"LookDev_SpermCell_{mode}.png")
            bpy.ops.render.render(write_still=True)
            print("GENESIS: gerendert", scene.render.filepath)
            measure_exposure(scene.render.filepath)


main()
