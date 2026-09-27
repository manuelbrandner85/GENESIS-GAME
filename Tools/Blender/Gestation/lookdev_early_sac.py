# GENESIS: Der Kreislauf des Lebens
#
# Lookdev der Hüllen in SSW 8 in Cycles, mit Kamera und Licht wie im Spiel (AGenesisWombScene, Außenansicht): Embryo
# (build_embryo_w08.py) in der Amnionblase, Dottersack am Stiel in der Chorionhöhle, die Chorionwand ringsum.
# Vorlage für die Materialien in Unreal (setup_mutterleib.py). Referenzen: Embryonen 6–9 Wochen mit intakten Hüllen
# (lunar caustic, Ed Uthman; ArtSource/Reference/Fetus, QUELLEN.md), Dottersack nach Cullen (gemeinfrei).
#
# Physik: Das Amnion (Brechzahl ~1,37) liegt in Flüssigkeit (~1,335) – relativ nur ~1,03: fast unsichtbar, nur zarte
# Reflexe und ein milchiger Schimmer. Die Chorionhöhle ist mit leicht gelblicher, zäher Flüssigkeit mit feinen Fasern
# gefüllt (Magma reticulare) – ein leichter Dunst.
#
#   blender -b --factory-startup --python Tools/Blender/Gestation/lookdev_early_sac.py -- [Ausgabe.png]

import json
import math
import os
import sys

import bpy
from mathutils import Vector

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
GEN = os.path.join(REPO, "ArtSource", "Generated", "Gestation")
OUT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv and len(sys.argv) > sys.argv.index("--") + 1 else \
    os.path.join(GEN, "Lookdev", "W08_Huellen.png")
INFO = json.load(open(os.path.join(GEN, "fetus_weeks.json"), encoding="utf-8"))["8"]
CM = 0.01
CAVITY_R = 1.5 * CM                                   # Chorionhöhle SSW 8 (GenesisWombPerception::CavityRadiusCm)
AMNION_R = 0.5 * (1.1 * INFO["crl"] - 0.07) * CM      # Horrow 1992
YOLK_R = 0.22 * CM                                    # 4–5 mm Durchmesser (Rempen 1987)


def enclosing_sphere(points):
    """Kleinste umschließende Kugel (Ritter, dann verfeinert) – wie AGenesisWombScene für das Amnion."""
    pts = [Vector(p) for p in points]
    a = pts[0]
    b = max(pts, key=lambda p: (p - a).length)
    c = max(pts, key=lambda p: (p - b).length)
    centre = (b + c) * 0.5
    radius = (c - b).length * 0.5
    for _ in range(3):
        for p in pts:
            d = (p - centre).length
            if d > radius:
                grow = 0.5 * (d - radius)
                radius += grow
                centre += (p - centre).normalized() * grow
    return centre, radius

# Das Embryo-Lookdev liefert das Hautmaterial
LOOK = {"__name__": "genesis_lookdev_lib", "__file__": os.path.join(REPO, "Tools", "Blender", "Gestation", "lookdev_embryo_w08.py")}
exec(open(os.path.join(REPO, "Tools", "Blender", "Gestation", "lookdev_embryo_w08.py"), encoding="utf-8").read(), LOOK)


def imp(name):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(GEN, name + ".fbx"))
    return [o for o in bpy.data.objects if o not in before and o.type == "MESH"][0]


def principled(name, **inputs):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    for key, value in inputs.items():
        bsdf.inputs[key].default_value = value
    return mat, bsdf


def amnion_material():
    # Dünner Film (0,02–0,05 mm) in Flüssigkeit: keine Brechung wie eine Glaskugel (Cycles ließe dann kein direktes Licht
    # zum Embryo durch), sondern durchsichtig + schwache Spiegelung (Fresnel, relative Brechzahl ~1,03, zum Rand stärker)
    # + ein Hauch milchige Trübung.
    mat = bpy.data.materials.new("M_Amnion")
    mat.use_nodes = True
    tree = mat.node_tree
    tree.nodes.clear()
    out = tree.nodes.new("ShaderNodeOutputMaterial")
    clear = tree.nodes.new("ShaderNodeBsdfTransparent")
    gloss = tree.nodes.new("ShaderNodeBsdfGlossy")
    gloss.inputs["Roughness"].default_value = 0.15
    milk = tree.nodes.new("ShaderNodeBsdfDiffuse")
    milk.inputs["Color"].default_value = (0.92, 0.88, 0.86, 1.0)
    fresnel = tree.nodes.new("ShaderNodeFresnel")
    fresnel.inputs["IOR"].default_value = float(os.environ.get("GENESIS_AMNION_IOR", "1.04"))
    # Zum Rand hin milchiger: Schräg gesehen ist der Weg durch die dünne Haut länger (Referenz: Ränder weißlich)
    layer = tree.nodes.new("ShaderNodeLayerWeight")
    layer.inputs["Blend"].default_value = 0.35
    milk_amt = tree.nodes.new("ShaderNodeMapRange")
    tree.links.new(layer.outputs["Facing"], milk_amt.inputs["Value"])
    milk_amt.inputs["To Min"].default_value = float(os.environ.get("GENESIS_AMNION_MILK", "0.03"))
    milk_amt.inputs["To Max"].default_value = 0.35
    haze = tree.nodes.new("ShaderNodeMixShader")
    tree.links.new(milk_amt.outputs["Result"], haze.inputs["Fac"])
    tree.links.new(clear.outputs[0], haze.inputs[1])
    tree.links.new(milk.outputs[0], haze.inputs[2])
    shine = tree.nodes.new("ShaderNodeMixShader")
    tree.links.new(fresnel.outputs[0], shine.inputs["Fac"])
    tree.links.new(haze.outputs[0], shine.inputs[1])
    tree.links.new(gloss.outputs[0], shine.inputs[2])
    tree.links.new(shine.outputs[0], out.inputs["Surface"])
    return mat


def yolk_material():
    # Dottersack: gelblich-rosa durchscheinend, Netz der Dottergefäße (Cullen) auf der Oberfläche
    mat, bsdf = principled("M_YolkSac", **{"Base Color": (0.86, 0.66, 0.44, 1.0), "Roughness": 0.3,
                                           "Subsurface Weight": 1.0, "Subsurface Radius": (1.0, 0.6, 0.35),
                                           "Subsurface Scale": 0.0008, "IOR": 1.38, "Specular IOR Level": 0.3})
    bsdf.subsurface_method = "RANDOM_WALK"
    tree = mat.node_tree
    coord = tree.nodes.new("ShaderNodeTexCoord")
    noise = tree.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 300.0            # Objektraum in m (Netz 1 cm Radius)
    tree.links.new(coord.outputs["Object"], noise.inputs["Vector"])
    warp = tree.nodes.new("ShaderNodeVectorMath")
    warp.operation = "MULTIPLY_ADD"
    tree.links.new(noise.outputs["Color"], warp.inputs[0])
    warp.inputs[1].default_value = (0.0025, 0.0025, 0.0025)
    tree.links.new(coord.outputs["Object"], warp.inputs[2])
    vor = tree.nodes.new("ShaderNodeTexVoronoi")
    vor.feature = "DISTANCE_TO_EDGE"
    vor.inputs["Scale"].default_value = 450.0
    tree.links.new(warp.outputs[0], vor.inputs["Vector"])
    ramp = tree.nodes.new("ShaderNodeMapRange")
    ramp.interpolation_type = "SMOOTHSTEP"
    tree.links.new(vor.outputs["Distance"], ramp.inputs["Value"])
    ramp.inputs["From Max"].default_value = 0.05
    ramp.inputs["To Min"].default_value = 1.0
    ramp.inputs["To Max"].default_value = 0.0
    mix = tree.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.inputs["A"].default_value = (0.86, 0.66, 0.44, 1.0)
    mix.inputs["B"].default_value = (0.55, 0.10, 0.10, 1.0)
    tree.links.new(ramp.outputs["Result"], mix.inputs["Factor"])
    tree.links.new(mix.outputs["Result"], bsdf.inputs["Base Color"])
    return mat


def chorion_material():
    # Innenseite der Chorionhöhle: glatte, glänzende, blass rosa-graue Haut, dahinter rot die Zotten (Referenzen: aufgeschnittene
    # Fruchtsäcke – innen bläulich-grau glatt, dahinter rotbraun)
    mat, bsdf = principled("M_Chorion", **{"Base Color": (0.55, 0.36, 0.34, 1.0), "Roughness": 0.25,
                                           "Subsurface Weight": 1.0, "Subsurface Radius": (1.0, 0.25, 0.2),
                                           "Subsurface Scale": 0.002, "IOR": 1.38, "Specular IOR Level": 0.4})
    bsdf.subsurface_method = "RANDOM_WALK"
    tree = mat.node_tree
    coord = tree.nodes.new("ShaderNodeTexCoord")
    noise = tree.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 250.0
    noise.inputs["Detail"].default_value = 6.0
    tree.links.new(coord.outputs["Object"], noise.inputs["Vector"])
    mix = tree.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.inputs["A"].default_value = (0.55, 0.36, 0.34, 1.0)
    mix.inputs["B"].default_value = (0.38, 0.12, 0.11, 1.0)
    tree.links.new(noise.outputs["Fac"], mix.inputs["Factor"])
    tree.links.new(mix.outputs["Result"], bsdf.inputs["Base Color"])
    # Wie im Spiel nur von innen sichtbar (die Kamera blickt wie ein 3D-Ultraschall von außen hinein): Rückseiten durchsichtig
    geo = tree.nodes.new("ShaderNodeNewGeometry")
    clear = tree.nodes.new("ShaderNodeBsdfTransparent")
    cull = tree.nodes.new("ShaderNodeMixShader")
    tree.links.new(geo.outputs["Backfacing"], cull.inputs["Fac"])
    tree.links.new(bsdf.outputs[0], cull.inputs[1])
    tree.links.new(clear.outputs[0], cull.inputs[2])
    tree.links.new(cull.outputs[0], tree.nodes["Material Output"].inputs["Surface"])
    return mat


def scene():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    embryo = imp("SM_GEN_Fetus_W08")
    embryo.data.materials.clear()
    embryo.data.materials.append(LOOK["embryo_material"]())
    # Amnion eng um den Embryo: Horrow (1,1 × SSL) als Untergrenze, sonst die kleinste umschließende Kugel + 6 %
    # (um den Mittelwert der Punkte zentriert ragten Kopf und Beine hinaus)
    hull_centre, hull_radius = enclosing_sphere(INFO["hull"])
    centre = hull_centre * CM
    amnion_r = max(AMNION_R, 1.06 * hull_radius * CM)
    amnion = imp("SM_GEN_Amnion")
    amnion.location = centre
    amnion.scale = (amnion_r / CM,) * 3
    amnion.data.materials.clear()
    amnion.data.materials.append(amnion_material())
    # Dottersack außerhalb des Amnions, unter dem Nabel zur Seite; Stiel zum Nabelstrang
    navel = Vector(INFO["navel"]) * CM
    out_dir = (navel - centre).normalized()
    side = out_dir.cross(Vector((0, 0, 1))).normalized()
    # Der Dottersack schwimmt in der Chorionhöhle, näher an der Wand als am Amnion, hinter dem Embryo (vom Blick weg)
    yolk_dir = (out_dir * 0.5 - side * 0.3 + Vector((0.0, 0.8, 0.0))).normalized()
    yolk_pos = centre + yolk_dir * (amnion_r + 0.55 * (CAVITY_R - amnion_r))
    yolk = imp("SM_GEN_YolkSac")
    yolk.location = yolk_pos
    yolk.scale = (YOLK_R / CM,) * 3
    yolk.rotation_mode = "QUATERNION"
    yolk.rotation_quaternion = (-(navel - yolk_pos)).to_track_quat("X", "Z")
    yolk.data.materials.clear()
    yolk.data.materials.append(yolk_material())
    stalk = imp("SM_GEN_YolkStalk")
    start = yolk_pos + (navel - yolk_pos).normalized() * YOLK_R
    length = (navel - start).length
    stalk.location = start
    stalk.rotation_mode = "QUATERNION"
    stalk.rotation_quaternion = (navel - start).to_track_quat("X", "Z")
    stalk.scale = (length / CM, 0.025, 0.025)
    stalk.data.materials.append(yolk_material())
    # Chorionwand: Kugel ringsum, Normalen nach innen
    bpy.ops.mesh.primitive_uv_sphere_add(segments=96, ring_count=48, radius=CAVITY_R, location=centre)
    wall = bpy.context.active_object
    wall.data.flip_normals()          # nach innen (im Edit-Modus per Operator blieb die Kugel unverändert: Wand blockierte alles)
    bpy.ops.object.shade_smooth()
    wall.data.materials.append(chorion_material())

    # Kamera wie im Spiel: 20 mm auf Kleinbild, 1,6 × Scheitel-Steiß-Länge vom Gesicht, von der Seite
    cam_data = bpy.data.cameras.new("Cam")
    cam_data.lens = 20.0
    cam_data.sensor_width = 36.0
    cam_data.clip_start = 0.0002
    cam = bpy.data.objects.new("Cam", cam_data)
    bpy.context.scene.collection.objects.link(cam)
    distance = 1.6 * INFO["crl"] * CM
    look = centre.lerp(Vector((0, 0, 0)), 0.35)
    cam.location = look + Vector((0.25, -1.0, 0.3)).normalized() * distance
    cam.rotation_euler = (look - cam.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = cam
    # Kaltlicht durch einen zweiten Zugang: ~25° oben links der Optik, weich
    light = bpy.data.objects.new("Kaltlicht", bpy.data.lights.new("Kaltlicht", "POINT"))
    right = (look - cam.location).normalized().cross(Vector((0, 0, 1))).normalized()
    offset = -right * 0.3 * distance + Vector((0, 0, 0.32 * distance))
    # wie im Spiel: Der Lichtzugang bleibt in der Höhle (außerhalb warf die Wand ihren Schatten auf alles)
    for _ in range(12):
        if (cam.location + offset - centre).length <= 0.85 * CAVITY_R:
            break
        offset *= 0.7
    light.location = cam.location + offset
    if (light.location - centre).length > 0.85 * CAVITY_R:
        light.location = centre + (light.location - centre).normalized() * 0.85 * CAVITY_R
    light.data.shadow_soft_size = 0.22 * distance
    light.data.energy = float(os.environ.get("GENESIS_LIGHT", "0.004"))
    light.data.color = (1.0, 0.96, 0.92)
    bpy.context.scene.collection.objects.link(light)

    sc = bpy.context.scene
    world = bpy.data.worlds.new("Dunkel")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.0
    # Dunst der Chorionhöhle: leicht gelbliche Flüssigkeit mit feinen Fasern
    vol = world.node_tree.nodes.new("ShaderNodeVolumePrincipled")
    vol.inputs["Color"].default_value = (0.9, 0.8, 0.65, 1.0)
    vol.inputs["Density"].default_value = float(os.environ.get("GENESIS_HAZE", "8.0"))
    world.node_tree.links.new(vol.outputs[0], world.node_tree.nodes["World Output"].inputs["Volume"])
    sc.world = world
    sc.render.engine = "CYCLES"
    sc.cycles.device = "GPU"
    sc.cycles.samples = int(os.environ.get("GENESIS_SAMPLES", "384"))
    sc.cycles.use_denoising = True
    sc.cycles.transmission_bounces = 16
    sc.cycles.max_bounces = 16
    sc.render.resolution_x, sc.render.resolution_y = 1600, 900
    sc.view_settings.view_transform = "AgX"
    sc.view_settings.exposure = float(os.environ.get("GENESIS_EXPOSURE", "0"))
    print("GENESIS: Kamera", tuple(round(v / CM, 2) for v in cam.location), "Blick", tuple(round(v / CM, 2) for v in look), "Mitte", tuple(round(v / CM, 2) for v in centre), "Licht", tuple(round(v / CM, 2) for v in light.location))
    return sc


if __name__ == "__main__":
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    sc = scene()
    sc.render.filepath = OUT
    bpy.ops.render.render(write_still=True)
    print("GENESIS: Lookdev", OUT)
