# GENESIS: Der Kreislauf des Lebens
#
# Lookdev des Embryos in SSW 8 (SM_GEN_Fetus_W08 aus build_embryo_w08.py) in Cycles – neben dem Referenzfoto, mit
# derselben Kamera: orthografisch im Pixelraum von Referenz 11 (lunar caustic, CC BY 2.0, lokal in ArtSource/Reference/
# Fetus), zwei weiche Blitze wie bei der Aufnahme („2 off camera SB-800“). Ausgabe: links das Foto, rechts der Render.
# Das Material hier ist die Vorlage für M_GEN_WombTissue in Unreal (Tools/Unreal/Gestation/setup_mutterleib.py).
#
#   blender -b --factory-startup --python Tools/Blender/Gestation/lookdev_embryo_w08.py -- [Ausgabe.png]

import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
GEN = os.path.join(REPO, "ArtSource", "Generated", "Gestation")
REF = os.path.join(REPO, "ArtSource", "Reference", "Fetus", "11_Human_Embryo_-_Approximately_8_weeks_estimated_gestational_age.jpg")
OUT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv and len(sys.argv) > sys.argv.index("--") + 1 else \
    os.path.join(GEN, "Lookdev", "W08_Vergleich.png")
W, H = 1280, 857

# Rahmen und Maßstab aus dem Bauskript (dieselben Landmarken)
BUILD = {"__name__": "genesis_lookdev"}
exec(open(os.path.join(REPO, "Tools", "Blender", "Gestation", "build_embryo_w08.py"), encoding="utf-8").read(), BUILD)
PX_M = BUILD["PX_CM"] * 0.01
EYE = BUILD["EYE_PX"]
GAZE = math.atan2(-(BUILD["GAZE_TO_PX"][1] - EYE[1]), BUILD["GAZE_TO_PX"][0] - EYE[0])
INFO = json.load(open(os.path.join(GEN, "fetus_weeks.json"), encoding="utf-8"))["8"]


def local_m(x_px, y_px, side_px=0.0):
    """Bildpunkt der Referenz -> Netzrahmen (m), wie build_embryo_w08.build()."""
    eye, basis = BUILD["frame"]()
    p = np.array(basis) @ (np.array(BUILD["v3"](x_px, y_px, side_px)[:]) - np.array(eye[:])) * BUILD["TO_REAL"]
    return Vector(p)


def node(tree, kind, x, y, **props):
    n = tree.nodes.new(kind)
    n.location = (x, y)
    for key, value in props.items():
        setattr(n, key, value)
    return n


def sphere_mask(tree, coord, centre, inner, outer, x, y):
    """1 innerhalb inner, weich auf 0 bis outer (m) um centre."""
    dist = node(tree, "ShaderNodeVectorMath", x, y, operation="DISTANCE")
    tree.links.new(coord, dist.inputs[0])
    dist.inputs[1].default_value = centre
    ramp = node(tree, "ShaderNodeMapRange", x + 180, y, interpolation_type="SMOOTHSTEP")
    tree.links.new(dist.outputs["Value"], ramp.inputs["Value"])
    ramp.inputs["From Min"].default_value = inner
    ramp.inputs["From Max"].default_value = outer
    ramp.inputs["To Min"].default_value = 1.0
    ramp.inputs["To Max"].default_value = 0.0
    return ramp.outputs["Result"]


def mix(tree, a, b, factor, x, y):
    m = node(tree, "ShaderNodeMix", x, y, data_type="RGBA", blend_type="MIX")
    if isinstance(a, tuple):
        m.inputs["A"].default_value = a
    else:
        tree.links.new(a, m.inputs["A"])
    if isinstance(b, tuple):
        m.inputs["B"].default_value = b
    else:
        tree.links.new(b, m.inputs["B"])
    if isinstance(factor, float):
        m.inputs["Factor"].default_value = factor
    else:
        tree.links.new(factor, m.inputs["Factor"])
    return m.outputs["Result"]


def scale_socket(tree, socket, amount, x, y):
    m = node(tree, "ShaderNodeMath", x, y, operation="MULTIPLY")
    tree.links.new(socket, m.inputs[0])
    m.inputs[1].default_value = amount
    return m.outputs[0]


def embryo_material():
    """
    Lebende Haut des Embryos (Referenzen: SSW 9 frisch, Ed Uthman; SSW 8/10 in Alkohol, lunar caustic):
    - Makro: rosig durchscheinend; über Herz und Leber dunkelrot (Blut unter ~0,2 mm Wand), Kopf und Glieder heller.
    - Medium: feines Gefäßnetz am Kopf (Oberflächengefäße des Kopfmesenchyms), dunkles Pigment der seitlichen Augen
      mit hellem Rand (Linse, Lidfalte), Ohrhöcker leicht rosiger.
    - Mikro: sehr feine Unebenheit (Zellschichten, Fixierung) – kaum sichtbar, nur im Glanzlicht.
    Streuung wie wasserreiches, kaum pigmentiertes Gewebe: Rot ~1,5 mm weit, Grün/Blau deutlich kürzer.
    """
    mat = bpy.data.materials.new("M_Lookdev_Embryo_W08")
    mat.use_nodes = True
    tree = mat.node_tree
    tree.nodes.clear()
    out = node(tree, "ShaderNodeOutputMaterial", 1400, 0)
    bsdf = node(tree, "ShaderNodeBsdfPrincipled", 1100, 0)
    tree.links.new(bsdf.outputs[0], out.inputs["Surface"])
    coord = node(tree, "ShaderNodeTexCoord", -1400, 0).outputs["Object"]

    base = (0.84, 0.60, 0.42, 1.0)                     # sRGB ~ (237, 203, 173): warm pfirsich-rosig (Referenzen SSW 8–9)
    # Kopf etwas heller und gelblicher (dickeres Mesenchym über dem Gehirn, Referenz SSW 9)
    head_c = local_m(404, 330)
    head = sphere_mask(tree, coord, head_c, 0.0020, 0.0034, -1200, 400)
    colour = mix(tree, base, (0.88, 0.64, 0.56, 1.0), scale_socket(tree, head, 0.6, -800, 400), -600, 300)

    # Gefäßnetz am Kopf: Voronoi-Kanten, durch Rauschen verzerrt, nur in der Kopfmaske
    noise = node(tree, "ShaderNodeTexNoise", -1200, 700)
    noise.inputs["Scale"].default_value = 180.0
    noise.inputs["Detail"].default_value = 3.0
    tree.links.new(coord, noise.inputs["Vector"])
    warp = node(tree, "ShaderNodeVectorMath", -1000, 700, operation="MULTIPLY_ADD")
    tree.links.new(noise.outputs["Color"], warp.inputs[0])
    warp.inputs[1].default_value = (0.0006, 0.0006, 0.0006)
    tree.links.new(coord, warp.inputs[2])
    vor = node(tree, "ShaderNodeTexVoronoi", -800, 700, feature="DISTANCE_TO_EDGE")
    vor.inputs["Scale"].default_value = 320.0
    tree.links.new(warp.outputs[0], vor.inputs["Vector"])
    lines = node(tree, "ShaderNodeMapRange", -600, 700, interpolation_type="SMOOTHSTEP")
    tree.links.new(vor.outputs["Distance"], lines.inputs["Value"])
    lines.inputs["From Min"].default_value = 0.0
    lines.inputs["From Max"].default_value = 0.06
    lines.inputs["To Min"].default_value = 1.0
    lines.inputs["To Max"].default_value = 0.0
    vessel = node(tree, "ShaderNodeMath", -400, 700, operation="MULTIPLY")
    tree.links.new(lines.outputs["Result"], vessel.inputs[0])
    tree.links.new(head, vessel.inputs[1])
    colour = mix(tree, colour, (0.62, 0.16, 0.14, 1.0), scale_socket(tree, vessel.outputs[0], 0.45, -200, 700), 0, 500)

    # Herz und Leber (Lage aus fetus_weeks.json, cm -> m)
    organs = INFO["organs"]
    heart = sphere_mask(tree, coord, Vector(organs["heart"][:3]) * 0.01, 0.9 * organs["heart"][3] * 0.01, 1.6 * organs["heart"][3] * 0.01, -1200, -300)
    liver = sphere_mask(tree, coord, Vector(organs["liver"][:3]) * 0.01, 0.9 * organs["liver"][3] * 0.01, 1.6 * organs["liver"][3] * 0.01, -1200, -500)
    colour = mix(tree, colour, (0.55, 0.12, 0.11, 1.0), scale_socket(tree, heart, 0.55, -800, -300), 200, 200)
    colour = mix(tree, colour, (0.36, 0.08, 0.08, 1.0), scale_socket(tree, liver, 0.65, -800, -500), 400, 100)

    # Augen: seitlich, dunkles Pigment mit hellerem Rand (Referenz: dunkle Scheibe, heller Ring)
    half = INFO["eye_half_cm"] * 0.01
    sep = node(tree, "ShaderNodeSeparateXYZ", -1200, -800)
    tree.links.new(coord, sep.inputs[0])
    xz = node(tree, "ShaderNodeCombineXYZ", -1000, -800)
    tree.links.new(sep.outputs["X"], xz.inputs["X"])
    tree.links.new(sep.outputs["Z"], xz.inputs["Z"])
    radial = node(tree, "ShaderNodeVectorMath", -800, -800, operation="LENGTH")
    tree.links.new(xz.outputs[0], radial.inputs[0])
    absy = node(tree, "ShaderNodeMath", -1000, -950, operation="ABSOLUTE")
    tree.links.new(sep.outputs["Y"], absy.inputs[0])
    side = node(tree, "ShaderNodeMapRange", -800, -950, interpolation_type="SMOOTHSTEP")
    tree.links.new(absy.outputs[0], side.inputs["Value"])
    side.inputs["From Min"].default_value = 0.55 * half
    side.inputs["From Max"].default_value = 0.75 * half
    r_eye = 0.2 * half
    core = node(tree, "ShaderNodeMapRange", -600, -800, interpolation_type="SMOOTHSTEP")
    tree.links.new(radial.outputs["Value"], core.inputs["Value"])
    core.inputs["From Min"].default_value = 0.75 * r_eye
    core.inputs["From Max"].default_value = r_eye
    core.inputs["To Min"].default_value = 1.0
    core.inputs["To Max"].default_value = 0.0
    ring = node(tree, "ShaderNodeMapRange", -600, -1000, interpolation_type="SMOOTHSTEP")
    tree.links.new(radial.outputs["Value"], ring.inputs["Value"])
    ring.inputs["From Min"].default_value = r_eye
    ring.inputs["From Max"].default_value = 1.6 * r_eye
    ring.inputs["To Min"].default_value = 1.0
    ring.inputs["To Max"].default_value = 0.0
    eye = node(tree, "ShaderNodeMath", -400, -800, operation="MULTIPLY")
    tree.links.new(core.outputs["Result"], eye.inputs[0])
    tree.links.new(side.outputs["Result"], eye.inputs[1])
    rim = node(tree, "ShaderNodeMath", -400, -1000, operation="MULTIPLY")
    tree.links.new(ring.outputs["Result"], rim.inputs[0])
    tree.links.new(side.outputs["Result"], rim.inputs[1])
    # Hände, Füße, Glieder heller (Gliedmaske aus build_embryo_w08.py im UV-Kanal „Daten“, x)
    uvmap = node(tree, "ShaderNodeUVMap", -1200, -1300)
    uvmap.uv_map = "Daten"
    uvsep = node(tree, "ShaderNodeSeparateXYZ", -1000, -1300)
    tree.links.new(uvmap.outputs["UV"], uvsep.inputs[0])
    colour = mix(tree, colour, (0.94, 0.86, 0.80, 1.0), scale_socket(tree, uvsep.outputs["X"], 0.75, -800, -1300), 500, -200)
    # Auge (Referenz vergrößert): dunkler grau-blauer Ring (pigmentierte Netzhaut, Iris-Anlage) um die hellere Linse
    lens = node(tree, "ShaderNodeMapRange", -600, -1150, interpolation_type="SMOOTHSTEP")
    tree.links.new(radial.outputs["Value"], lens.inputs["Value"])
    lens.inputs["From Min"].default_value = 0.35 * r_eye
    lens.inputs["From Max"].default_value = 0.6 * r_eye
    lens.inputs["To Min"].default_value = 1.0
    lens.inputs["To Max"].default_value = 0.0
    lens_side = node(tree, "ShaderNodeMath", -400, -1150, operation="MULTIPLY")
    tree.links.new(lens.outputs["Result"], lens_side.inputs[0])
    tree.links.new(side.outputs["Result"], lens_side.inputs[1])
    colour = mix(tree, colour, (0.90, 0.80, 0.74, 1.0), scale_socket(tree, rim.outputs[0], 0.35, -200, -1000), 600, 0)
    colour = mix(tree, colour, (0.10, 0.12, 0.16, 1.0), scale_socket(tree, eye.outputs[0], 0.92, -200, -800), 800, -100)
    colour = mix(tree, colour, (0.42, 0.44, 0.48, 1.0), scale_socket(tree, lens_side.outputs[0], 0.85, -200, -1150), 900, -200)
    # Medium: leichte Fleckigkeit (ungleich dicke Haut, Gefäße darunter) – ~0,5 mm Flecken, wenige Prozent
    mottle = node(tree, "ShaderNodeTexNoise", 400, 300)
    mottle.inputs["Scale"].default_value = 500.0
    mottle.inputs["Detail"].default_value = 2.0
    tree.links.new(coord, mottle.inputs["Vector"])
    mottle_amt = node(tree, "ShaderNodeMapRange", 600, 300)
    tree.links.new(mottle.outputs["Fac"], mottle_amt.inputs["Value"])
    mottle_amt.inputs["From Min"].default_value = 0.35
    mottle_amt.inputs["From Max"].default_value = 0.7
    mottle_amt.inputs["To Min"].default_value = 0.0
    mottle_amt.inputs["To Max"].default_value = 0.2
    colour = mix(tree, colour, (0.70, 0.40, 0.30, 1.0), mottle_amt.outputs["Result"], 900, 100)
    # Mikro: feine helle Pünktchen (Referenz: am Kopf sichtbar) – kleine Voronoi-Zellen
    speck = node(tree, "ShaderNodeTexVoronoi", 400, 150)
    speck.inputs["Scale"].default_value = 3500.0
    tree.links.new(coord, speck.inputs["Vector"])
    speck_amt = node(tree, "ShaderNodeMapRange", 600, 150, interpolation_type="SMOOTHSTEP")
    tree.links.new(speck.outputs["Distance"], speck_amt.inputs["Value"])
    speck_amt.inputs["From Min"].default_value = 0.08
    speck_amt.inputs["From Max"].default_value = 0.2
    speck_amt.inputs["To Min"].default_value = 0.1
    speck_amt.inputs["To Max"].default_value = 0.0
    colour = mix(tree, colour, (0.95, 0.88, 0.80, 1.0), speck_amt.outputs["Result"], 1000, 150)
    tree.links.new(colour, bsdf.inputs["Base Color"])

    # Streuung: wasserreich, kaum Pigment
    bsdf.subsurface_method = "RANDOM_WALK"
    bsdf.inputs["Subsurface Weight"].default_value = 1.0
    bsdf.inputs["Subsurface Radius"].default_value = (1.0, 0.42, 0.25)
    bsdf.inputs["Subsurface Scale"].default_value = float(os.environ.get("GENESIS_SSS", "0.0005"))
    # Gewebe in Flüssigkeit: schwache Spiegelung (relativer Brechungsindex klein), glatt, leicht variierend
    bsdf.inputs["IOR"].default_value = 1.38
    bsdf.inputs["Specular IOR Level"].default_value = 0.25
    rough_noise = node(tree, "ShaderNodeTexNoise", 400, -600)
    rough_noise.inputs["Scale"].default_value = 400.0
    tree.links.new(coord, rough_noise.inputs["Vector"])
    rough = node(tree, "ShaderNodeMapRange", 600, -600)
    tree.links.new(rough_noise.outputs["Fac"], rough.inputs["Value"])
    rough.inputs["To Min"].default_value = 0.42
    rough.inputs["To Max"].default_value = 0.62
    tree.links.new(rough.outputs["Result"], bsdf.inputs["Roughness"])
    # Mikro: feinste Unebenheit
    micro = node(tree, "ShaderNodeTexNoise", 400, -900)
    micro.inputs["Scale"].default_value = 2500.0
    micro.inputs["Detail"].default_value = 4.0
    tree.links.new(coord, micro.inputs["Vector"])
    bump = node(tree, "ShaderNodeBump", 800, -900)
    bump.inputs["Strength"].default_value = 0.35
    bump.inputs["Distance"].default_value = 0.00002
    tree.links.new(micro.outputs["Fac"], bump.inputs["Height"])
    tree.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(GEN, "SM_GEN_Fetus_W08.fbx"))
    body = [o for o in bpy.context.scene.objects if o.type == "MESH"][0]
    body.matrix_world = Matrix.Rotation(-GAZE, 4, "Y") @ body.matrix_world
    body.data.materials.clear()
    body.data.materials.append(embryo_material())

    cam_data = bpy.data.cameras.new("Cam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = W * PX_M
    cam_data.clip_start = 0.0001
    cam_data.clip_end = 1.0
    cam = bpy.data.objects.new("Cam", cam_data)
    bpy.context.scene.collection.objects.link(cam)
    cam.location = ((W / 2 - EYE[0]) * PX_M, -0.1, -(H / 2 - EYE[1]) * PX_M)
    cam.rotation_euler = (math.radians(90), 0, 0)
    bpy.context.scene.camera = cam

    # Zwei weiche Blitze wie bei der Aufnahme: Hauptlicht oben links vorn, Aufheller rechts – Größe einer Softbox
    # im Verhältnis zum 1,5-cm-Objekt (die Referenz zeigt breite, weiche Glanzlichter)
    for name, loc, power, size in (("Blitz_links", (-0.08, -0.05, 0.08), 1.4, 0.025), ("Blitz_rechts", (0.08, -0.05, -0.01), 0.12, 0.04)):
        light = bpy.data.objects.new(name, bpy.data.lights.new(name, "AREA"))
        light.data.energy = power
        light.data.size = size
        light.location = loc
        light.rotation_euler = (-Vector(loc)).to_track_quat("-Z", "Y").to_euler()
        bpy.context.scene.collection.objects.link(light)
    world = bpy.data.worlds.new("Fruchthoehle")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.09, 0.05, 0.025, 1.0)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.45
    sc = bpy.context.scene
    sc.world = world
    sc.render.engine = "CYCLES"
    sc.cycles.device = "GPU"
    sc.cycles.samples = 256
    sc.cycles.use_denoising = True
    sc.render.film_transparent = False
    sc.render.resolution_x, sc.render.resolution_y = W, H
    sc.view_settings.view_transform = "AgX"
    sc.view_settings.exposure = float(os.environ.get("GENESIS_EXPOSURE", "0"))
    return sc


def side_by_side(render_path):
    ref = bpy.data.images.load(REF)
    ren = bpy.data.images.load(render_path)
    a = np.array(ref.pixels[:]).reshape(H, W, 4)
    b = np.array(ren.pixels[:]).reshape(H, W, 4)
    both = np.concatenate([a, b], axis=1)
    img = bpy.data.images.new("Vergleich", 2 * W, H, alpha=True)
    img.pixels = both.ravel()
    img.filepath_raw = OUT
    img.file_format = "PNG"
    img.save()


if __name__ == "__main__":
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    sc = scene()
    render_path = OUT.replace(".png", "_render.png")
    sc.render.filepath = render_path
    bpy.ops.render.render(write_still=True)
    side_by_side(render_path)
    print("GENESIS: Lookdev", OUT)
