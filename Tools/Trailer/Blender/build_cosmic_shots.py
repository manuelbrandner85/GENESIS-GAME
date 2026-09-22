# GENESIS Trailer - Blender-Einstellungen fuer Akt 4/5 (Cycles, GPU): Portal, Galaxie-Rueckzug, Welten-Erschaffung, Titel.
#
# Aufruf:
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --factory-startup --python Tools\Trailer\Blender\build_cosmic_shots.py -- <portal|galaxy|planet|title|all> [--preview]
# Ausgabe: Genesis/Saved/Trailer/Blender/<shot>/<shot>_####.png (1920x1080, 24 fps), .blend daneben
#
# Lichtlogik: Im Kosmos gibt es nur selbstleuchtende Quellen (Sterne, Sonne, Stadtlichter, die goldenen Faeden).
# Der Planet wird von einer einzigen Sonne beleuchtet; Glanz entsteht nur im Compositor (Glare), dezent.

import math
import os
import sys

import bpy
import numpy as np

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(REPO, "Genesis", "Saved", "Trailer", "Blender")
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else ["all"]
PREVIEW = "--preview" in ARGS
FPS = 24
FONT_TITLE = "C:/Windows/Fonts/segoeuil.ttf"
rng = np.random.default_rng(1306)


# ---------------------------------------------------------------- Grundlagen
def new_scene(name, frames):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.name = name
    scene.render.engine = "CYCLES"
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "OPTIX"
    prefs.get_devices()
    for d in prefs.devices:
        d.use = d.type == "OPTIX"
    scene.cycles.device = "GPU"
    scene.cycles.samples = 24 if PREVIEW else int(os.environ.get("GENESIS_SAMPLES", "64"))
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = 0.02
    scene.cycles.use_denoising = True
    scene.cycles.max_bounces = 6
    scene.render.resolution_x, scene.render.resolution_y = 1920, 1080
    scene.render.resolution_percentage = 50 if PREVIEW else 100
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, frames
    scene.render.use_motion_blur = True
    scene.render.motion_blur_shutter = 0.5
    scene.view_settings.view_transform = "AgX"
    for look in ("AgX - Medium High Contrast", "Medium High Contrast", "None"):
        try:
            scene.view_settings.look = look
            break
        except TypeError:
            continue
    world = bpy.data.worlds.new("Space")
    world.use_nodes = True
    nt = world.node_tree
    nt.nodes.clear()
    bg = nt.nodes.new("ShaderNodeBackground")
    bg.inputs["Color"].default_value = (0, 0, 0, 1)
    nt.links.new(bg.outputs[0], nt.nodes.new("ShaderNodeOutputWorld").inputs[0])
    scene.world = world
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_depth = "16"
    return scene


def emission_material(name, color, strength):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Color"].default_value = (*color, 1)
    em.inputs["Strength"].default_value = strength
    nt.links.new(em.outputs[0], nt.nodes.new("ShaderNodeOutputMaterial").inputs[0])
    return m, em


def camera(scene, lens=35, loc=(0, -10, 0), target=(0, 0, 0)):
    data = bpy.data.cameras.new("Cam")
    data.lens = lens
    data.sensor_width = 36.0
    data.clip_start = 0.001
    data.clip_end = 1e7
    cam = bpy.data.objects.new("Cam", data)
    scene.collection.objects.link(cam)
    cam.location = loc
    tgt = bpy.data.objects.new("CamTarget", None)
    scene.collection.objects.link(tgt)
    tgt.location = target
    con = cam.constraints.new("TRACK_TO")
    con.target = tgt
    con.track_axis = "TRACK_NEGATIVE_Z"
    con.up_axis = "UP_Y"
    scene.camera = cam
    return cam, tgt


def glare(scene, threshold=1.0, size=7, strength=0.3):
    """Dezenter Fog-Glow wie Streulicht in einem echten Objektiv."""
    scene.use_nodes = True
    tree = scene.compositing_node_group if hasattr(scene, "compositing_node_group") else scene.node_tree
    if tree is None:
        tree = bpy.data.node_groups.new("Compositing", "CompositorNodeTree")
        scene.compositing_node_group = tree
    tree.nodes.clear()
    rl = tree.nodes.new("CompositorNodeRLayers")
    g = tree.nodes.new("CompositorNodeGlare")
    # Blender 5.x: Typ und Werte sind Eingaenge des Knotens
    for value in ("Fog Glow", "FOG_GLOW"):
        try:
            g.inputs["Type"].default_value = value
            break
        except (TypeError, ValueError):
            continue
    for value in ("High", "HIGH"):
        try:
            g.inputs["Quality"].default_value = value
            break
        except (TypeError, ValueError):
            continue
    g.inputs["Threshold"].default_value = threshold
    g.inputs["Strength"].default_value = strength
    g.inputs["Size"].default_value = min(1.0, size / 10.0)
    out = None
    for t in ("CompositorNodeComposite", "NodeGroupOutput"):
        try:
            out = tree.nodes.new(t)
            break
        except RuntimeError:
            continue
    if out.bl_idname == "NodeGroupOutput" and not tree.interface.items_tree:
        tree.interface.new_socket("Image", in_out="OUTPUT", socket_type="NodeSocketColor")
    tree.links.new(rl.outputs["Image"], g.inputs[0])
    tree.links.new(g.outputs[0], out.inputs[0])


def key(obj, path, frame, value, index=-1):
    if index >= 0:
        getattr(obj, path)[index] = value
    else:
        setattr(obj, path, value)
    obj.keyframe_insert(data_path=path, index=index, frame=frame)


def smooth(t):
    return t * t * (3 - 2 * t)


def stars(scene, count=6000, radius=4000.0, seed=3):
    """Sternenhimmel als echte, sehr weit entfernte Punkte (Parallaxe korrekt)."""
    r = np.random.default_rng(seed)
    d = r.normal(size=(count, 3))
    d /= np.linalg.norm(d, axis=1, keepdims=True)
    pts = d * radius * r.uniform(0.8, 1.0, (count, 1))
    mesh = bpy.data.meshes.new("Stars")
    mesh.from_pydata(pts.tolist(), [], [])
    obj = bpy.data.objects.new("Stars", mesh)
    scene.collection.objects.link(obj)
    bright = (r.lognormal(-1.4, 1.0, count) * 1.6).astype(np.float32)
    attr = mesh.attributes.new("brightness", "FLOAT", "POINT")
    attr.data.foreach_set("value", bright)
    temp = mesh.attributes.new("temp", "FLOAT", "POINT")
    temp.data.foreach_set("value", r.uniform(0, 1, count).astype(np.float32))
    points_from_mesh(obj, radius * 0.0006, "M_Stars", star_material)
    return obj


def star_material(name):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    a = nt.nodes.new("ShaderNodeAttribute"); a.attribute_name = "brightness"; a.attribute_type = "GEOMETRY"
    t = nt.nodes.new("ShaderNodeAttribute"); t.attribute_name = "temp"; t.attribute_type = "GEOMETRY"
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (1.0, 0.75, 0.5, 1)
    ramp.color_ramp.elements[1].color = (0.7, 0.8, 1.0, 1)
    pink = ramp.color_ramp.elements.new(0.0)
    pink.color = (1.0, 0.35, 0.55, 1)
    ramp.color_ramp.elements[0].position = 0.0
    shift = nt.nodes.new("ShaderNodeMapRange")
    shift.inputs["From Min"].default_value = -1.0
    shift.inputs["From Max"].default_value = 1.0
    shift.inputs["To Min"].default_value = 0.0
    shift.inputs["To Max"].default_value = 1.0
    nt.links.new(t.outputs["Fac"], shift.inputs["Value"])
    nt.links.new(shift.outputs[0], ramp.inputs[0])
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.0, (1.0, 0.35, 0.55, 1)
    els[1].position, els[1].color = 0.5, (1.0, 0.75, 0.5, 1)
    els[2].position, els[2].color = 1.0, (0.7, 0.8, 1.0, 1)
    mul = nt.nodes.new("ShaderNodeMath"); mul.operation = "MULTIPLY"; mul.inputs[1].default_value = 6.0
    nt.links.new(a.outputs["Fac"], mul.inputs[0])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(ramp.outputs[0], em.inputs["Color"])
    nt.links.new(mul.outputs[0], em.inputs["Strength"])
    nt.links.new(em.outputs[0], nt.nodes.new("ShaderNodeOutputMaterial").inputs[0])
    return m


def points_from_mesh(obj, radius, mat_name, mat_factory):
    """Geometry Nodes: Mesh-Vertices -> Punkte (Cycles rendert sie als echte Kugeln)."""
    mod = obj.modifiers.new("Points", "NODES")
    group = bpy.data.node_groups.new("ToPoints_" + obj.name, "GeometryNodeTree")
    group.interface.new_socket("Geometry", in_out="INPUT", socket_type="NodeSocketGeometry")
    group.interface.new_socket("Geometry", in_out="OUTPUT", socket_type="NodeSocketGeometry")
    gi = group.nodes.new("NodeGroupInput")
    go = group.nodes.new("NodeGroupOutput")
    mp = group.nodes.new("GeometryNodeMeshToPoints")
    mp.inputs["Radius"].default_value = radius
    sm = group.nodes.new("GeometryNodeSetMaterial")
    sm.inputs["Material"].default_value = mat_factory(mat_name)
    group.links.new(gi.outputs[0], mp.inputs["Mesh"])
    group.links.new(mp.outputs["Points"], sm.inputs["Geometry"])
    group.links.new(sm.outputs[0], go.inputs[0])
    mod.node_group = group


def render(scene, shot):
    out_dir = os.path.join(OUT, shot + ("_preview" if PREVIEW else ""))
    os.makedirs(out_dir, exist_ok=True)
    scene.render.filepath = os.path.join(out_dir, shot + "_")
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out_dir, shot + ".blend"))
    if PREVIEW:
        for f in sorted({scene.frame_start, (scene.frame_start + scene.frame_end) // 2, scene.frame_end}):
            scene.frame_set(f)
            scene.render.filepath = os.path.join(out_dir, "%s_%04d" % (shot, f))
            bpy.ops.render.render(write_still=True)
    else:
        bpy.ops.render.render(animation=True)
    print("GENESIS_BLENDER_OK", shot, out_dir)


# ---------------------------------------------------------------- 1 Portal (Auge schliesst -> Lichtkreis oeffnet sich)
def shot_portal():
    frames = 48
    scene = new_scene("Portal", frames)
    camera(scene, 50, (0, -6, 0), (0, 0, 0))
    gold = (1.0, 0.72, 0.36)
    ring_mat, ring_em = emission_material("M_Ring", gold, 30.0)
    bpy.ops.mesh.primitive_torus_add(major_radius=1.0, minor_radius=0.012, major_segments=256, minor_segments=12, rotation=(math.pi / 2, 0, 0))
    ring = bpy.context.active_object
    ring.data.materials.append(ring_mat)
    for f, s in ((1, 0.02), (14, 0.35), (40, 1.0), (48, 1.06)):
        key(ring, "scale", f, (s, s, s))
    # Faeden: feine Spiralen, die in den Kreis hineinlaufen (goldener Faden des Spiels)
    thread_mat, thread_em = emission_material("M_Thread", gold, 14.0)
    for i in range(9):
        curve = bpy.data.curves.new("Thread%d" % i, "CURVE")
        curve.dimensions = "3D"
        curve.bevel_depth = 0.0035
        spline = curve.splines.new("POLY")
        n = 160
        spline.points.add(n - 1)
        phase = i * 2 * math.pi / 9
        for j in range(n):
            u = j / (n - 1)
            r = 3.4 - 2.4 * u
            a = phase + u * 3.2
            spline.points[j].co = (math.cos(a) * r, (1 - u) * 1.5 * math.sin(phase * 2), math.sin(a) * r, 1)
        obj = bpy.data.objects.new("Thread%d" % i, curve)
        scene.collection.objects.link(obj)
        obj.data.materials.append(thread_mat)
        start = 1 + i * 2
        key(curve, "bevel_factor_end", start, 0.0)
        key(curve, "bevel_factor_end", start + 26, 1.0)
        key(curve, "bevel_factor_start", start + 18, 0.0)
        key(curve, "bevel_factor_start", start + 40, 1.0)
    # Innenlicht: weiche Scheibe, die aufblendet
    disk_mat = bpy.data.materials.new("M_Disk")
    disk_mat.use_nodes = True
    nt = disk_mat.node_tree
    nt.nodes.clear()
    tc = nt.nodes.new("ShaderNodeTexCoord")
    grad = nt.nodes.new("ShaderNodeTexGradient"); grad.gradient_type = "SPHERICAL"
    mapping = nt.nodes.new("ShaderNodeMapping")
    nt.links.new(tc.outputs["Object"], mapping.inputs[0])
    nt.links.new(mapping.outputs[0], grad.inputs[0])
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (0, 0, 0, 1)
    ramp.color_ramp.elements[1].position = 0.9
    ramp.color_ramp.elements[1].color = (1.0, 0.86, 0.66, 1)
    nt.links.new(grad.outputs["Fac"], ramp.inputs[0])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(ramp.outputs[0], em.inputs["Color"])
    nt.links.new(em.outputs[0], nt.nodes.new("ShaderNodeOutputMaterial").inputs[0])
    for f, s in ((1, 0.0), (24, 0.2), (48, 4.0)):
        em.inputs["Strength"].default_value = s
        em.inputs["Strength"].keyframe_insert("default_value", frame=f)
    bpy.ops.mesh.primitive_circle_add(vertices=128, radius=0.995, fill_type="NGON", rotation=(math.pi / 2, 0, 0), location=(0, 0.01, 0))
    disk = bpy.context.active_object
    disk.data.materials.append(disk_mat)
    for f, s in ((1, 0.02), (14, 0.35), (40, 1.0), (48, 1.06)):
        key(disk, "scale", f, (s, s, s))
    cam = scene.camera
    key(cam, "location", 1, (0, -6.0, 0))
    key(cam, "location", 48, (0, -3.2, 0))
    stars(scene, 2500, 800.0, seed=11)
    glare(scene, threshold=0.8, size=8)
    render(scene, "BL_Portal")


# ---------------------------------------------------------------- 2 Galaxie-Rueckzug
def shot_galaxy():
    frames = 96
    scene = new_scene("Galaxy", frames)
    # Der Rueckzug ist extrem schnell (logarithmisch ueber vier Groessenordnungen) - Bewegungsunschaerfe wuerde
    # Sterne zu "Warp"-Streifen ziehen. Ohne sie wirkt es wie ein Zeitraffer, nicht wie ein Spiel-Effekt.
    scene.render.use_motion_blur = False
    # Punktfoermige Sterne: der Denoiser wuerde sie zu Flecken verschmieren - mehr Samples, kein Denoiser
    scene.cycles.use_denoising = PREVIEW
    if not PREVIEW:
        scene.cycles.samples = int(os.environ.get("GENESIS_GALAXY_SAMPLES", "96"))
    n = 420000
    arms = 2
    r = rng.gamma(2.2, 0.9, n) * 5.5
    r = r[r < 60]
    n = len(r)
    arm = rng.integers(0, arms, n)
    theta = np.log(r + 0.5) / 0.27 + arm * (2 * np.pi / arms) + rng.normal(0, 0.16, n) * (1.0 + 5.0 / (r + 1.0)) + rng.normal(0, 0.05, n) * r / 10.0
    spur = rng.random(n) < 0.18                       # schwache Nebenarme mit anderem Anstellwinkel
    theta = np.where(spur, theta + 0.9 + rng.normal(0, 0.2, n), theta)
    lane = np.mod(theta - (np.log(r + 0.5) / 0.27 + arm * (2 * np.pi / arms)) + np.pi, 2 * np.pi) - np.pi
    dust = np.clip(1.0 - 0.8 * np.exp(-((lane + 0.12) / 0.06) ** 2), 0.15, 1.0)   # Staubbahn an der Innenkante
    z = rng.normal(0, 0.45 + 2.2 * np.exp(-r / 4.0), n) * 0.6
    pts = np.stack([np.cos(theta) * r, np.sin(theta) * r, z], 1)
    bulge = rng.normal(0, 2.6, (60000, 3)) * np.array([1, 1, 0.55])
    halo = rng.normal(0, 18.0, (40000, 3)) * np.array([1, 1, 0.35])
    # Haufen: Zentren entlang der Arme, je Haufen eine kompakte Wolke (flockige Struktur echter Spiralarme)
    idx = rng.choice(len(pts), 5000, replace=False)
    centers = pts[idx]
    clusters = np.repeat(centers, 18, axis=0) + rng.normal(0, 0.35, (5000 * 18, 3)) * np.array([1, 1, 0.4])
    pts = np.vstack([pts, bulge, halo, clusters])
    rad = np.linalg.norm(pts[:, :2], axis=1)
    mesh = bpy.data.meshes.new("GalaxyPts")
    mesh.from_pydata(pts.tolist(), [], [])
    gal = bpy.data.objects.new("Galaxy", mesh)
    scene.collection.objects.link(gal)
    temp = mesh.attributes.new("temp", "FLOAT", "POINT")
    temp_values = np.clip(rad / 40.0 + rng.normal(0, 0.12, len(pts)), 0, 1).astype(np.float32)
    temp_values[-5000 * 18:] = np.clip(temp_values[-5000 * 18:] + 0.25, 0, 1)   # junge Haufen: blau-weiss
    hii = len(pts) - 5000 * 18 + rng.choice(5000 * 18, 3000, replace=False)
    temp_values[hii] = -1.0                                                          # Sternentstehung: rosa
    temp.data.foreach_set("value", temp_values)
    bright = mesh.attributes.new("brightness", "FLOAT", "POINT")
    b = rng.lognormal(-2.2, 0.9, len(pts)) * (0.5 + 2.4 * np.exp(-rad / 5.0))
    b[:n] *= dust * np.where(spur, 0.45, 1.0)
    b[-5000 * 18:] *= 1.6
    # Sterne bleiben aus jeder Entfernung punktfoermig: kleiner Radius, Leuchtkraft mit der Flaeche ausgeglichen (x100)
    bright.data.foreach_set("value", (b * 100.0).astype(np.float32))
    points_from_mesh(gal, 0.002, "M_GalaxyStars", star_material)
    # diffuses Galaxienleuchten (Volumen), damit die Arme nicht nur aus Einzelpunkten bestehen
    bpy.ops.mesh.primitive_cylinder_add(vertices=96, radius=62, depth=5, location=(0, 0, 0))
    haze = bpy.context.active_object
    vm = bpy.data.materials.new("M_GalaxyHaze")
    vm.use_nodes = True
    vnt = vm.node_tree
    vnt.nodes.clear()
    vol = vnt.nodes.new("ShaderNodeVolumePrincipled")
    vol.inputs["Density"].default_value = 0.0
    tc = vnt.nodes.new("ShaderNodeTexCoord")
    grad = vnt.nodes.new("ShaderNodeTexGradient")
    grad.gradient_type = "SPHERICAL"
    mapping = vnt.nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = (1 / 34.0, 1 / 34.0, 1 / 3.0)
    vnt.links.new(tc.outputs["Object"], mapping.inputs[0])
    vnt.links.new(mapping.outputs[0], grad.inputs[0])
    powr = vnt.nodes.new("ShaderNodeMath")
    powr.operation = "POWER"
    powr.inputs[1].default_value = 3.0
    vnt.links.new(grad.outputs["Fac"], powr.inputs[0])
    em_s = vnt.nodes.new("ShaderNodeMath")
    em_s.operation = "MULTIPLY"
    em_s.inputs[1].default_value = 0.06
    vnt.links.new(powr.outputs[0], em_s.inputs[0])
    vnt.links.new(em_s.outputs[0], vol.inputs["Emission Strength"])
    vol.inputs["Emission Color"].default_value = (1.0, 0.82, 0.62, 1)
    vnt.links.new(vol.outputs[0], vnt.nodes.new("ShaderNodeOutputMaterial").inputs["Volume"])
    haze.data.materials.append(vm)
    haze.parent = gal
    for f, a in ((1, 0.0), (96, math.radians(6))):
        key(gal, "rotation_euler", f, a, index=2)
    stars(scene, 5000, 3000.0, seed=5)
    # Kamera: logarithmischer Rueckzug vom Punkt der Heimat (am Rand eines Arms) bis weit ueber die Galaxie
    home = pts[np.argmin(np.abs(rad - 24.0) + np.abs(pts[:, 2]) * 3)]
    marker_mat, _ = emission_material("M_Home", (0.55, 0.75, 1.0), 60.0)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.004, location=tuple(home))
    bpy.context.active_object.data.materials.append(marker_mat)
    cam, tgt = camera(scene, 35, tuple(home + np.array([0, -0.05, 0.01])), tuple(home))
    d0, d1 = 0.05, 160.0
    for f in range(1, frames + 1, 4):
        t = smooth((f - 1) / (frames - 1))
        d = math.exp(math.log(d0) + (math.log(d1) - math.log(d0)) * t)
        direction = np.array([0.15 * (1 - t), -1.0 + 0.35 * t, 0.12 + 0.8 * t])
        direction /= np.linalg.norm(direction)
        focus = home * (1 - t) + np.zeros(3) * t
        cam.location = tuple(focus + direction * d)
        cam.keyframe_insert("location", frame=f)
        tgt.location = tuple(focus)
        tgt.keyframe_insert("location", frame=f)
    glare(scene, threshold=1.2, size=7)
    render(scene, "BL_Galaxy")


# ---------------------------------------------------------------- 3 Welten-Erschaffung (Planet im Zeitraffer)
def planet_material():
    m = bpy.data.materials.new("M_Planet")
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    tc = nt.nodes.new("ShaderNodeTexCoord")
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 1.6
    noise.inputs["Detail"].default_value = 12.0
    noise.inputs["Roughness"].default_value = 0.62
    nt.links.new(tc.outputs["Object"], noise.inputs["Vector"])
    # Meeresspiegel sinkt -> Kontinente tauchen auf (animiert)
    sea = nt.nodes.new("ShaderNodeValue"); sea.name = "SeaLevel"; sea.outputs[0].default_value = 0.72
    land = nt.nodes.new("ShaderNodeMapRange")
    land.inputs["From Min"].default_value = 0.0
    land.inputs["From Max"].default_value = 0.02
    sub = nt.nodes.new("ShaderNodeMath"); sub.operation = "SUBTRACT"
    nt.links.new(noise.outputs["Fac"], sub.inputs[0])
    nt.links.new(sea.outputs[0], sub.inputs[1])
    nt.links.new(sub.outputs[0], land.inputs["Value"])
    height = nt.nodes.new("ShaderNodeMapRange")
    height.inputs["From Min"].default_value = 0.0
    height.inputs["From Max"].default_value = 0.2
    nt.links.new(sub.outputs[0], height.inputs["Value"])
    land_ramp = nt.nodes.new("ShaderNodeValToRGB")
    els = land_ramp.color_ramp.elements
    els[0].color = (0.16, 0.19, 0.08, 1)
    els[1].position = 0.6; els[1].color = (0.32, 0.26, 0.17, 1)
    e = els.new(0.92); e.color = (0.85, 0.85, 0.86, 1)
    nt.links.new(height.outputs[0], land_ramp.inputs[0])
    ocean = nt.nodes.new("ShaderNodeRGB"); ocean.outputs[0].default_value = (0.008, 0.03, 0.08, 1)
    mix = nt.nodes.new("ShaderNodeMix"); mix.data_type = "RGBA"
    nt.links.new(land.outputs[0], mix.inputs["Factor"])
    nt.links.new(ocean.outputs[0], mix.inputs[6])
    nt.links.new(land_ramp.outputs[0], mix.inputs[7])
    nt.links.new(mix.outputs[2], bsdf.inputs["Base Color"])
    rough = nt.nodes.new("ShaderNodeMapRange")
    rough.inputs["To Min"].default_value = 0.12
    rough.inputs["To Max"].default_value = 0.85
    nt.links.new(land.outputs[0], rough.inputs["Value"])
    nt.links.new(rough.outputs[0], bsdf.inputs["Roughness"])
    # Stadtlichter: nur auf Land, nur auf der Nachtseite, feines Muster (animierte Staerke)
    vor = nt.nodes.new("ShaderNodeTexVoronoi")
    vor.inputs["Scale"].default_value = 160.0
    nt.links.new(tc.outputs["Object"], vor.inputs["Vector"])
    dots = nt.nodes.new("ShaderNodeMapRange")
    dots.inputs["From Min"].default_value = 0.22
    dots.inputs["From Max"].default_value = 0.0
    nt.links.new(vor.outputs["Distance"], dots.inputs["Value"])
    popn = nt.nodes.new("ShaderNodeTexNoise")
    popn.inputs["Scale"].default_value = 9.0
    popn.inputs["Detail"].default_value = 6.0
    nt.links.new(tc.outputs["Object"], popn.inputs["Vector"])
    popr = nt.nodes.new("ShaderNodeMapRange")
    popr.inputs["From Min"].default_value = 0.5
    popr.inputs["From Max"].default_value = 0.66
    nt.links.new(popn.outputs["Fac"], popr.inputs["Value"])
    lr = nt.nodes.new("ShaderNodeMath")
    lr.operation = "MULTIPLY"
    nt.links.new(dots.outputs[0], lr.inputs[0])
    nt.links.new(popr.outputs[0], lr.inputs[1])
    geo = nt.nodes.new("ShaderNodeNewGeometry")
    sun_dir = nt.nodes.new("ShaderNodeCombineXYZ")
    sun_dir.name = "SunDir"
    sun_dir.inputs[0].default_value, sun_dir.inputs[1].default_value, sun_dir.inputs[2].default_value = 0.85, -0.2, 0.3
    dot = nt.nodes.new("ShaderNodeVectorMath"); dot.operation = "DOT_PRODUCT"
    nt.links.new(geo.outputs["Normal"], dot.inputs[0])
    nt.links.new(sun_dir.outputs[0], dot.inputs[1])
    night = nt.nodes.new("ShaderNodeMapRange")
    night.inputs["From Min"].default_value = 0.05
    night.inputs["From Max"].default_value = -0.25
    nt.links.new(dot.outputs["Value"], night.inputs["Value"])
    city_strength = nt.nodes.new("ShaderNodeValue"); city_strength.name = "CityLights"; city_strength.outputs[0].default_value = 0.0
    m1 = nt.nodes.new("ShaderNodeMath"); m1.operation = "MULTIPLY"
    m2 = nt.nodes.new("ShaderNodeMath"); m2.operation = "MULTIPLY"
    m3 = nt.nodes.new("ShaderNodeMath"); m3.operation = "MULTIPLY"
    nt.links.new(lr.outputs[0], m1.inputs[0]); nt.links.new(land.outputs[0], m1.inputs[1])
    nt.links.new(m1.outputs[0], m2.inputs[0]); nt.links.new(night.outputs[0], m2.inputs[1])
    nt.links.new(m2.outputs[0], m3.inputs[0]); nt.links.new(city_strength.outputs[0], m3.inputs[1])
    bsdf.inputs["Emission Color"].default_value = (1.0, 0.62, 0.3, 1)
    nt.links.new(m3.outputs[0], bsdf.inputs["Emission Strength"])
    nt.links.new(bsdf.outputs[0], out.inputs[0])
    return m, sea, city_strength


def cloud_material():
    m = bpy.data.materials.new("M_Clouds")
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    tc = nt.nodes.new("ShaderNodeTexCoord")
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 4.5
    noise.inputs["Detail"].default_value = 10.0
    noise.inputs["Distortion"].default_value = 0.6
    nt.links.new(tc.outputs["Object"], noise.inputs["Vector"])
    cover = nt.nodes.new("ShaderNodeValue"); cover.name = "Cover"; cover.outputs[0].default_value = 0.0
    thr = nt.nodes.new("ShaderNodeMath"); thr.operation = "SUBTRACT"
    nt.links.new(noise.outputs["Fac"], thr.inputs[0])
    inv = nt.nodes.new("ShaderNodeMath"); inv.operation = "SUBTRACT"; inv.inputs[0].default_value = 0.8
    nt.links.new(cover.outputs[0], inv.inputs[1])
    nt.links.new(inv.outputs[0], thr.inputs[1])
    amt = nt.nodes.new("ShaderNodeMapRange")
    amt.inputs["From Min"].default_value = 0.0
    amt.inputs["From Max"].default_value = 0.32
    nt.links.new(thr.outputs[0], amt.inputs["Value"])
    white = nt.nodes.new("ShaderNodeBsdfDiffuse")
    white.inputs["Color"].default_value = (0.9, 0.9, 0.92, 1)
    transp = nt.nodes.new("ShaderNodeBsdfTransparent")
    mix = nt.nodes.new("ShaderNodeMixShader")
    nt.links.new(amt.outputs[0], mix.inputs[0])
    nt.links.new(transp.outputs[0], mix.inputs[1])
    nt.links.new(white.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], nt.nodes.new("ShaderNodeOutputMaterial").inputs[0])
    return m, cover


def atmosphere_material():
    m = bpy.data.materials.new("M_Atmosphere")
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    lw = nt.nodes.new("ShaderNodeLayerWeight")
    lw.inputs["Blend"].default_value = 0.28
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (0, 0, 0, 1)
    ramp.color_ramp.elements[1].color = (0.25, 0.5, 1.0, 1)
    nt.links.new(lw.outputs["Facing"], ramp.inputs[0])
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(ramp.outputs[0], em.inputs["Color"])
    strength = nt.nodes.new("ShaderNodeValue"); strength.name = "AtmoStrength"; strength.outputs[0].default_value = 0.0
    geo = nt.nodes.new("ShaderNodeNewGeometry")
    sd = nt.nodes.new("ShaderNodeCombineXYZ")
    sd.name = "SunDir"
    dot = nt.nodes.new("ShaderNodeVectorMath")
    dot.operation = "DOT_PRODUCT"
    nt.links.new(geo.outputs["Normal"], dot.inputs[0])
    nt.links.new(sd.outputs[0], dot.inputs[1])
    day = nt.nodes.new("ShaderNodeMapRange")
    day.inputs["From Min"].default_value = -0.35
    day.inputs["From Max"].default_value = 0.4
    day.inputs["To Min"].default_value = 0.04
    nt.links.new(dot.outputs["Value"], day.inputs["Value"])
    mul = nt.nodes.new("ShaderNodeMath")
    mul.operation = "MULTIPLY"
    nt.links.new(strength.outputs[0], mul.inputs[0])
    nt.links.new(day.outputs[0], mul.inputs[1])
    nt.links.new(mul.outputs[0], em.inputs["Strength"])
    add = nt.nodes.new("ShaderNodeAddShader")
    nt.links.new(nt.nodes.new("ShaderNodeBsdfTransparent").outputs[0], add.inputs[0])
    nt.links.new(em.outputs[0], add.inputs[1])
    nt.links.new(add.outputs[0], nt.nodes.new("ShaderNodeOutputMaterial").inputs[0])
    m.blend_method = "BLEND" if hasattr(m, "blend_method") else None
    return m, strength


def key_value(node, frames_values):
    for f, v in frames_values:
        node.outputs[0].default_value = v
        node.outputs[0].keyframe_insert("default_value", frame=f)


def shot_planet():
    frames = 96
    scene = new_scene("Planet", frames)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=256, ring_count=128, radius=1.0)
    planet = bpy.context.active_object
    bpy.ops.object.shade_smooth()
    pm, sea, city = planet_material()
    planet.data.materials.append(pm)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=192, ring_count=96, radius=1.012)
    clouds = bpy.context.active_object
    bpy.ops.object.shade_smooth()
    cm, cover = cloud_material()
    clouds.data.materials.append(cm)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=192, ring_count=96, radius=1.045)
    atmo = bpy.context.active_object
    bpy.ops.object.shade_smooth()
    am, atmo_strength = atmosphere_material()
    atmo.data.materials.append(am)
    # Zeitraffer: Meer weicht, Wolken ziehen auf, Atmosphaere leuchtet, Staedte entstehen
    key_value(sea, [(1, 0.75), (40, 0.5), (96, 0.47)])
    key_value(cover, [(1, 0.0), (34, 0.0), (74, 0.16), (96, 0.2)])
    key_value(atmo_strength, [(1, 0.1), (40, 0.9), (96, 1.2)])
    key_value(city, [(1, 0.0), (58, 0.0), (96, 40.0)])
    for f, a in ((1, 0.0), (96, math.radians(70))):
        key(planet, "rotation_euler", f, a, index=2)
    for f, a in ((1, 0.0), (96, math.radians(95))):
        key(clouds, "rotation_euler", f, a, index=2)
    sun = bpy.data.lights.new("Sun", "SUN")
    sun.energy = 6.0
    sun.angle = math.radians(0.53)
    sun_obj = bpy.data.objects.new("Sun", sun)
    scene.collection.objects.link(sun_obj)
    import mathutils
    to_sun = np.array([0.92, 0.3, 0.22])
    to_sun /= np.linalg.norm(to_sun)
    sun_obj.rotation_euler = mathutils.Vector(to_sun.tolist()).to_track_quat("Z", "Y").to_euler()
    for i in range(3):
        pm.node_tree.nodes["SunDir"].inputs[i].default_value = float(to_sun[i])
        am.node_tree.nodes["SunDir"].inputs[i].default_value = float(to_sun[i])
    cam, tgt = camera(scene, 50, (0.4, -3.6, 0.6), (0, 0, 0))
    key(cam, "location", 1, (0.9, -3.9, 0.7))
    key(cam, "location", 96, (-0.3, -3.2, 0.45))
    stars(scene, 5000, 3000.0, seed=9)
    glare(scene, threshold=1.5, size=6)
    render(scene, "BL_Planet")


# ---------------------------------------------------------------- 4 Titel
def shot_title():
    frames = 132
    scene = new_scene("Title", frames)
    cam, tgt = camera(scene, 50, (0, -12, 0), (0, 0, 0))
    key(cam, "location", 1, (0, -12.4, 0))
    key(cam, "location", 132, (0, -11.6, 0))
    gold_m = bpy.data.materials.new("M_TitleGold")
    gold_m.use_nodes = True
    nt = gold_m.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (1.0, 0.78, 0.45, 1)
    bsdf.inputs["Metallic"].default_value = 1.0
    bsdf.inputs["Roughness"].default_value = 0.28
    bsdf.inputs["Emission Color"].default_value = (1.0, 0.74, 0.4, 1)
    em_val = bsdf.inputs["Emission Strength"]
    for f, v in ((1, 0.0), (40, 0.0), (70, 1.1), (132, 0.8)):
        em_val.default_value = v
        em_val.keyframe_insert("default_value", frame=f)
    # Licht fuer den Metallglanz: schmales Streiflicht, das ueber die Buchstaben wandert
    area = bpy.data.lights.new("Sweep", "AREA")
    area.energy = 900
    area.size = 0.6
    area.shape = "RECTANGLE"
    area.size_y = 6.0
    area.color = (1.0, 0.9, 0.75)
    area_obj = bpy.data.objects.new("Sweep", area)
    scene.collection.objects.link(area_obj)
    area_obj.rotation_euler = (math.radians(90), 0, 0)
    key(area_obj, "location", 40, (-7, -4, 0.4))
    key(area_obj, "location", 110, (7, -4, 0.4))

    def text(body, size, z, spacing, emission_frames, name):
        curve = bpy.data.curves.new(name, "FONT")
        curve.body = body
        curve.font = bpy.data.fonts.load(FONT_TITLE)
        curve.size = size
        curve.space_character = spacing
        curve.align_x = "CENTER"
        curve.align_y = "CENTER"
        curve.extrude = size * 0.015
        obj = bpy.data.objects.new(name, curve)
        scene.collection.objects.link(obj)
        obj.rotation_euler = (math.radians(90), 0, 0)
        obj.location = (0, 0, z)
        m = gold_m.copy() if name != "Genesis" else gold_m
        if name != "Genesis":
            b = m.node_tree.nodes["Principled BSDF"]
            b.inputs["Metallic"].default_value = 0.0
            b.inputs["Base Color"].default_value = (0.85, 0.82, 0.76, 1)
            b.inputs["Emission Color"].default_value = (0.9, 0.87, 0.8, 1)
            ev = b.inputs["Emission Strength"]
            if m.node_tree.animation_data:
                m.node_tree.animation_data_clear()
            for f, v in emission_frames:
                ev.default_value = v
                ev.keyframe_insert("default_value", frame=f)
        obj.data.materials.append(m)
        return obj

    title = text("GENESIS", 1.0, 0.45, 1.7, None, "Genesis")
    for f, s in ((40, 0.96), (132, 1.0)):
        key(title, "scale", f, (s, s, s))
    # Aufdecken durch eine Blende (Plane vor dem Titel, schwarz, gleitet weg) statt Einblendung eines Bildes
    text("DER KREISLAUF DES LEBENS", 0.26, -0.72, 1.6, [(1, 0.0), (68, 0.0), (86, 0.9), (132, 0.9)], "Subtitle")
    text("JEDE ENTSCHEIDUNG HINTERLÄSST EIN ECHO.", 0.17, -1.22, 1.5, [(1, 0.0), (92, 0.0), (108, 0.6), (132, 0.6)], "Tagline")
    mask_m, _ = emission_material("M_Mask", (0, 0, 0), 0.0)
    bpy.ops.mesh.primitive_plane_add(size=1, location=(0, -0.6, 0.35), rotation=(math.radians(90), 0, 0))
    mask = bpy.context.active_object
    mask.scale = (14, 1.9, 1)
    mask.data.materials.append(mask_m)
    key(mask, "location", 40, (0, -0.6, 0.35))
    key(mask, "location", 76, (0, -0.6, 3.0))
    # Goldener Faden: zeichnet einen Kreis, der sich zu einer Linie unter dem Titel oeffnet (Kreis -> Echo)
    thread_mat, thread_em = emission_material("M_TitleThread", (1.0, 0.72, 0.36), 9.0)
    for f, v in ((1, 9.0), (46, 9.0), (70, 0.0)):
        thread_em.inputs["Strength"].default_value = v
        thread_em.inputs["Strength"].keyframe_insert("default_value", frame=f)
    for i in range(3):
        curve = bpy.data.curves.new("Circle%d" % i, "CURVE")
        curve.dimensions = "3D"
        curve.bevel_depth = 0.006 - i * 0.0015
        spline = curve.splines.new("POLY")
        n = 256
        spline.points.add(n - 1)
        radius = 2.05 + i * 0.05
        for j in range(n):
            ang = math.pi / 2 - 2 * math.pi * j / (n - 1)
            wob = 0.012 * math.sin(ang * 5 + i)
            spline.points[j].co = (math.cos(ang) * (radius + wob), 0.05 * i, math.sin(ang) * (radius + wob) + 0.1, 1)
        obj = bpy.data.objects.new("Circle%d" % i, curve)
        scene.collection.objects.link(obj)
        obj.data.materials.append(thread_mat)
        key(curve, "bevel_factor_end", 1 + i * 4, 0.0)
        key(curve, "bevel_factor_end", 40 + i * 4, 1.0)
    line_mat, line_em = emission_material("M_TitleLine", (1.0, 0.74, 0.4), 6.0)
    curve = bpy.data.curves.new("Line", "CURVE")
    curve.dimensions = "3D"
    curve.bevel_depth = 0.004
    spline = curve.splines.new("POLY")
    spline.points.add(1)
    spline.points[0].co = (-3.4, 0, -0.22, 1)
    spline.points[1].co = (3.4, 0, -0.22, 1)
    line = bpy.data.objects.new("Line", curve)
    scene.collection.objects.link(line)
    line.data.materials.append(line_mat)
    key(curve, "bevel_factor_start", 58, 0.5)
    key(curve, "bevel_factor_end", 58, 0.5)
    key(curve, "bevel_factor_start", 92, 0.0)
    key(curve, "bevel_factor_end", 92, 1.0)
    stars(scene, 1500, 900.0, seed=21)
    glare(scene, threshold=1.0, size=7)
    render(scene, "BL_Title")


SHOTS = {"portal": shot_portal, "galaxy": shot_galaxy, "planet": shot_planet, "title": shot_title}
for name in (SHOTS if ARGS[0] == "all" else [ARGS[0]]):
    SHOTS[name]()
