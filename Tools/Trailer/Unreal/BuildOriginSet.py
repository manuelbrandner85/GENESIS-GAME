# GENESIS Trailer - SEQ_001_Origin: Import, Materialien, Set, Licht, Atmosphaere, Kameras und Animation (idempotent).
#
# Massstab: 1 um = 1 cm. Welt ist ein Ausschnitt der Eileiter-Ampulle, erfuellt von Fluessigkeit.
# Lichtlogik: kein Sonnenlicht - warmes, durch Gewebe transmittiertes Licht von oben/hinten (grosse Rect Light "Transmission"),
# dazu das indirekte Licht der Schleimhaut (Lumen). Belichtung manuell (gemessen per Testrender).
# Deterministische Bewegung: Alle Animationen der Zellen haengen am Parameter "SwimTime" der Material Parameter Collection
# MPC_GEN_Microworld (Conception), den der Sequencer keyt (kein Engine-Time -> identisch in jedem Render).
#
# Aufruf: powershell -ExecutionPolicy Bypass -File Tools\Trailer\Run-UnrealPython.ps1 -Script Tools\Trailer\Unreal\BuildOriginSet.py

import json
import math
import os
import random

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
REPO = os.path.abspath(os.path.join(PROJECT_DIR, ".."))
DATA = json.load(open(os.path.join(REPO, "Docs", "Trailer", "ShotList.json"), encoding="utf-8"))
FPS = int(DATA["fps"])
SRC = os.path.join(REPO, "ArtSource", "Generated", "Conception")
CONCEPTION = "/Game/Genesis/Conception"                 # gemeinsame Assets fuer Spiel (Conception-Block) und Trailer
ENV = CONCEPTION + "/Environment"
CELLS = CONCEPTION + "/Cells"
MATS = CONCEPTION + "/Materials"
TRAILER = "/Game/Genesis/Trailer"
MAP = TRAILER + "/Maps/L_SEQ001_Origin"
SUB_SEQ = TRAILER + "/Sequences/Sub/SEQ_001_Origin"
SPERM_MESH = CELLS + "/SM_GEN_SpermCell"                 # Asset der Conception-Session
SPERM_MATERIAL = MATS + "/M_GEN_SpermCell"
MI_OUT = TRAILER + "/Materials"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
failures = []
random.seed(1306)


def fail(msg):
    failures.append(msg)
    unreal.log_error("GENESIS_TRAILER_FAIL " + msg)


def log(msg):
    unreal.log("GENESIS_TRAILER " + msg)


def frame(t):
    return int(round(t * FPS))


def ensure_dir(path):
    if not eal.does_directory_exist(path):
        eal.make_directory(path)


# ---------------------------------------------------------------- Import
def import_files(files, dest):
    ensure_dir(dest)
    tasks = []
    for path in files:
        t = unreal.AssetImportTask()
        t.filename = path
        t.destination_path = dest
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    imported = []
    for t in tasks:
        imported += list(t.imported_object_paths)
    return imported


def import_meshes():
    groups = [(ENV, ["SM_GEN_MucosaFolds", "SM_GEN_Debris"]), (CELLS, ["SM_GEN_OocyteCytoplasm", "SM_GEN_OocyteZona", "SM_GEN_OocyteCorona"])]
    for dest, files in groups:
        for p in import_files([os.path.join(SRC, f + ".fbx") for f in files], dest):
            asset = eal.load_asset(p)
            if isinstance(asset, unreal.MaterialInterface):   # Interchange-Platzhaltermaterialien entfernen
                eal.delete_asset(p)
    meshes = {}
    wanted = [(ENV, "SM_GEN_MucosaFolds")] + [(ENV, "SM_GEN_Debris_{:02d}".format(i)) for i in range(1, 7)] +              [(CELLS, n) for n in ("SM_GEN_OocyteCytoplasm", "SM_GEN_OocyteZona", "SM_GEN_OocyteCorona")]
    for folder, name in wanted:
        found = [a for a in eal.list_assets(folder, recursive=True) if a.split("/")[-1].split(".")[0] == name]
        asset = eal.load_asset(found[0]) if found else None
        if isinstance(asset, unreal.StaticMesh):
            meshes[name] = asset
        else:
            fail("Mesh fehlt nach Import: " + name)
    sperm = eal.load_asset(SPERM_MESH) if eal.does_asset_exist(SPERM_MESH) else None
    if not isinstance(sperm, unreal.StaticMesh):
        unreal.log_warning("GENESIS_TRAILER Spermium noch nicht importiert (Conception-Session): " + SPERM_MESH + " - Set ohne Spermien")
        sperm = None
    meshes["Sperm"] = sperm
    for name in ("SM_GEN_OocyteCytoplasm", "SM_GEN_OocyteZona", "SM_GEN_OocyteCorona", "SM_GEN_MucosaFolds"):
        mesh = meshes.get(name)
        if mesh:
            nanite = mesh.get_editor_property("nanite_settings")
            if not nanite.enabled:
                nanite.enabled = True
                mesh.set_editor_property("nanite_settings", nanite)
                eal.save_loaded_asset(mesh, only_if_is_dirty=False)
    return meshes


def import_textures():
    names = ("T_GEN_Mucosa_BC", "T_GEN_Mucosa_N", "T_GEN_Mucosa_R", "T_GEN_Cell_N", "T_GEN_Cytoplasm_BC")
    import_files([os.path.join(SRC, "Textures", n + ".png") for n in names], ENV + "/Textures")
    tex = {}
    for n in names:
        t = eal.load_asset(ENV + "/Textures/" + n)
        if not isinstance(t, unreal.Texture2D):
            fail("Textur fehlt: " + n)
            continue
        if n.endswith("_N"):
            t.set_editor_property("srgb", False)
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif n.endswith("_R"):
            t.set_editor_property("srgb", False)
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
        else:
            t.set_editor_property("srgb", True)
        eal.save_loaded_asset(t, only_if_is_dirty=False)
        tex[n] = t
    return tex


# ---------------------------------------------------------------- Material-Bausteine
class Graph:
    def __init__(self, path, shading=unreal.MaterialShadingModel.MSM_SUBSURFACE, blend=unreal.BlendMode.BLEND_OPAQUE):
        folder, name = path.rsplit("/", 1)
        if eal.does_asset_exist(path):
            eal.delete_asset(path)
        ensure_dir(folder)
        self.m = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
        self.m.set_editor_property("shading_model", shading)
        self.m.set_editor_property("blend_mode", blend)
        self.x = -1600
        self.y = 0

    def node(self, cls, **props):
        e = mel.create_material_expression(self.m, cls, self.x, self.y)
        self.y += 140
        if self.y > 2400:
            self.y, self.x = 0, self.x + 300
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def link(self, a, b, pin="", out=""):
        if not mel.connect_material_expressions(a, out, b, pin):
            fail("Verbindung fehlgeschlagen in {}: {} -> {}.{}".format(self.m.get_name(), a.get_name(), b.get_name(), pin))

    def out(self, e, prop, out=""):
        if not mel.connect_material_property(e, out, prop):
            fail("Ausgang fehlgeschlagen in {}: {}".format(self.m.get_name(), prop))

    def scalar(self, name, value):
        return self.node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=value)

    def color(self, name, rgb):
        return self.node(unreal.MaterialExpressionVectorParameter, parameter_name=name, default_value=unreal.LinearColor(*rgb, 1.0))

    def const(self, value):
        return self.node(unreal.MaterialExpressionConstant, r=value)

    def op(self, cls, a, b):
        e = self.node(cls)
        self.link(a, e, "A")
        self.link(b, e, "B")
        return e

    def mul(self, a, b):
        return self.op(unreal.MaterialExpressionMultiply, a, b)

    def add(self, a, b):
        return self.op(unreal.MaterialExpressionAdd, a, b)

    def sub(self, a, b):
        return self.op(unreal.MaterialExpressionSubtract, a, b)

    def one(self, cls, a, **props):
        e = self.node(cls, **props)
        self.link(a, e)
        return e

    def append(self, a, b):
        return self.op(unreal.MaterialExpressionAppendVector, a, b)

    def tex(self, name, texture, uv=None, normal=False):
        e = self.node(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name=name, texture=texture,
                      sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal else
                      (unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE if name.endswith("Rough") else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR))
        if uv is not None:
            self.link(uv, e, "UVs")
        return e

    def uv(self, tiling, index=0):
        return self.node(unreal.MaterialExpressionTextureCoordinate, coordinate_index=index, u_tiling=tiling, v_tiling=tiling)

    def mpc(self, collection, name):
        return self.node(unreal.MaterialExpressionCollectionParameter, collection=collection, parameter_name=name)

    def object_hash(self):
        """0..1 Zufallswert je Actor aus seiner Weltposition (fuer Phasenversatz ohne Instancing)."""
        pos = self.node(unreal.MaterialExpressionObjectPositionWS)
        d = self.op(unreal.MaterialExpressionDotProduct, pos, self.node(unreal.MaterialExpressionConstant3Vector,
                                                                         constant=unreal.LinearColor(0.1371, 0.2713, 0.4191, 0)))
        s = self.one(unreal.MaterialExpressionSine, d, period=6.283185)
        return self.one(unreal.MaterialExpressionFrac, self.mul(s, self.const(43758.5453)))

    def finish(self):
        mel.layout_material_expressions(self.m)
        mel.recompile_material(self.m)
        eal.save_loaded_asset(self.m, only_if_is_dirty=False)
        return self.m


def make_mpc():
    """Gemeinsame Material Parameter Collection der Conception-Session (SwimTime wird vom Sequencer gekeyt)."""
    mpc = eal.load_asset(MATS + "/MPC_GEN_Microworld") if eal.does_asset_exist(MATS + "/MPC_GEN_Microworld") else None
    if mpc is None:
        fail("MPC_GEN_Microworld fehlt (Conception-Session)")
    return mpc


def make_mucosa_material(tex):
    g = Graph(MATS + "/M_GEN_Mucosa")
    uv = g.uv(1.0)
    macro_uv = g.uv(0.137)
    bc = g.tex("BaseColorTex", tex["T_GEN_Mucosa_BC"], uv)
    macro = g.tex("MacroTex", tex["T_GEN_Mucosa_BC"], macro_uv)
    macro_l = g.one(unreal.MaterialExpressionComponentMask, macro, r=True, g=False, b=False, a=False)
    macro_mod = g.add(g.mul(macro_l, g.scalar("MacroStrength", 0.9)), g.const(0.45))
    base = g.mul(g.mul(g.one(unreal.MaterialExpressionComponentMask, bc, r=True, g=True, b=True, a=False), macro_mod),
                 g.color("Tint", (1.0, 1.0, 1.0)))
    g.out(base, unreal.MaterialProperty.MP_BASE_COLOR)
    rough = g.tex("RoughnessRough", tex["T_GEN_Mucosa_R"], uv)
    g.out(g.mul(g.one(unreal.MaterialExpressionComponentMask, rough, r=True, g=False, b=False, a=False),
                g.scalar("RoughnessScale", 1.0)), unreal.MaterialProperty.MP_ROUGHNESS)
    n = g.tex("NormalTex", tex["T_GEN_Mucosa_N"], uv, normal=True)
    g.out(n, unreal.MaterialProperty.MP_NORMAL)
    g.out(g.color("SubsurfaceColor", (0.85, 0.18, 0.10)), unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    g.out(g.scalar("SubsurfaceOpacity", 0.55), unreal.MaterialProperty.MP_OPACITY)
    g.out(g.scalar("Specular", 0.45), unreal.MaterialProperty.MP_SPECULAR)
    return g.finish()


def make_cell_material(path, tex, base_rgb, sss_rgb, roughness, opacity, detail_tiling, cytoplasm=False):
    g = Graph(path)
    uv = g.uv(detail_tiling)
    if cytoplasm:
        bc = g.tex("BaseColorTex", tex["T_GEN_Cytoplasm_BC"], uv)
        base = g.mul(g.one(unreal.MaterialExpressionComponentMask, bc, r=True, g=True, b=True, a=False), g.color("Tint", base_rgb))
    else:
        base = g.color("BaseColor", base_rgb)
    g.out(base, unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(g.scalar("Roughness", roughness), unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(g.tex("NormalTex", tex["T_GEN_Cell_N"], uv, normal=True), unreal.MaterialProperty.MP_NORMAL)
    g.out(g.color("SubsurfaceColor", sss_rgb), unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    g.out(g.scalar("SubsurfaceOpacity", opacity), unreal.MaterialProperty.MP_OPACITY)
    return g.finish()


def make_debris_material(tex, mpc):
    """Schwebeteilchen: treiben mit der Stroemung (+X) und taumeln leicht - deterministisch ueber SwimTime."""
    g = Graph(MATS + "/M_GEN_Debris")
    g.out(g.color("BaseColor", (0.55, 0.45, 0.38)), unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(g.scalar("Roughness", 0.45), unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(g.color("SubsurfaceColor", (0.9, 0.5, 0.3)), unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    g.out(g.scalar("SubsurfaceOpacity", 0.8), unreal.MaterialProperty.MP_OPACITY)
    t = g.mpc(mpc, "SwimTime")
    h = g.object_hash()
    drift_x = g.mul(g.mul(t, g.scalar("FlowSpeed", 6.0)), g.add(g.mul(h, g.const(0.8)), g.const(0.6)))
    wob = g.mul(g.one(unreal.MaterialExpressionSine, g.add(g.mul(t, g.const(0.23)), h), period=1.0), g.const(4.0))
    wob_z = g.mul(g.one(unreal.MaterialExpressionSine, g.add(g.mul(t, g.const(0.17)), g.mul(h, g.const(3.1))), period=1.0), g.const(3.0))
    wpo = g.append(g.append(drift_x, wob), wob_z)
    g.out(wpo, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    g.m.set_editor_property("max_world_position_offset_displacement", 250.0)
    return g.finish()


def make_instance(parent, name, scalars, statics=None):
    path = MI_OUT + "/" + name
    ensure_dir(MI_OUT)
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mi = tools.create_asset(name, MI_OUT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", parent)
    for k, v in scalars.items():
        mel.set_material_instance_scalar_parameter_value(mi, k, v)
    for k, v in (statics or {}).items():
        mel.set_material_instance_static_switch_parameter_value(mi, k, v)
        mel.update_material_instance(mi)
    eal.save_loaded_asset(mi, only_if_is_dirty=False)
    return mi


# ---------------------------------------------------------------- Level
def new_level():
    ensure_dir(TRAILER + "/Maps")
    if eal.does_asset_exist(MAP):
        eal.delete_asset(MAP)
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not les.new_level(MAP, False):
        fail("Level konnte nicht angelegt werden")
    return unreal.EditorLevelLibrary.get_editor_world()


def spawn_mesh(mesh, loc, rot=(0, 0, 0), scale=1.0, label=None, material=None, cast_shadow=True):
    a = actors.spawn_actor_from_object(mesh, unreal.Vector(*loc), unreal.Rotator(*rot))
    if a is None:
        fail("Spawn fehlgeschlagen: " + mesh.get_name())
        return None
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    comp = a.static_mesh_component
    if material:
        comp.set_material(0, material)
    comp.set_editor_property("cast_shadow", cast_shadow)
    comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    if label:
        a.set_actor_label(label)
    return a


def build_set(meshes, mats):
    folder = "SEQ001"
    # Schleimhaut: Boden und schraeg ansteigende Wand = Ausschnitt eines Faltenschlauchs
    floor = spawn_mesh(meshes["SM_GEN_MucosaFolds"], (0, 0, -260), (0, 0, 0), 1.0, "Mucosa_Floor", mats["mucosa"])
    wall = spawn_mesh(meshes["SM_GEN_MucosaFolds"], (0, 1900, 900), (-68, 0, 12), 1.0, "Mucosa_Wall", mats["mucosa"])
    ceiling = spawn_mesh(meshes["SM_GEN_MucosaFolds"], (400, -600, 2300), (180, 0, 37), 1.0, "Mucosa_Ceiling", mats["mucosa"])
    for a in (floor, wall, ceiling):
        if a:
            a.set_folder_path(folder + "/Tissue")

    # Held-Spermium (wird im Sequencer bewegt)
    hero = None
    if meshes["Sperm"]:
        hero = spawn_mesh(meshes["Sperm"], (0, 0, 0), (0, 0, 0), 1.0, "Sperm_Hero", mats["sperm_hero"])
        hero.set_folder_path(folder + "/Cells")

    # Schwarm: Stroemungsbahn entlang +X mit Dichtegefaelle, leicht gerichtete Schwimmwinkel
    swarm = 0
    swarm_root = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    swarm_root.set_actor_label("Swarm_Root")
    swarm_root.static_mesh_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    swarm_root.set_folder_path(folder + "/Swarm")
    for i in range(320 if meshes["Sperm"] else 0):
        x = random.uniform(-1400, 1600)
        y = random.gauss(0, 420)
        z = random.gauss(40, 150)
        if abs(y) < 90 and abs(z) < 60 and -300 < x < 400:
            continue                                      # Held freistellen
        yaw = random.gauss(0, 14)
        pitch = random.gauss(0, 8)
        roll = random.uniform(0, 360)
        a = spawn_mesh(meshes["Sperm"], (x, y, z), (roll, pitch, yaw), 1.0, "Sperm_Swarm_{:03d}".format(i), mats["sperm_swarm"],
                       cast_shadow=False)
        if a:
            a.set_folder_path(folder + "/Swarm")
            a.attach_to_actor(swarm_root, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                              unreal.AttachmentRule.KEEP_WORLD, False)
            swarm += 1

    # Eizelle mit Corona radiata voraus in der Stroemung
    egg_loc = (5200, 350, 120)
    for name, key in (("SM_GEN_OocyteCytoplasm", "cyto"), ("SM_GEN_OocyteZona", "zona"), ("SM_GEN_OocyteCorona", "corona")):
        a = spawn_mesh(meshes[name], egg_loc, (0, 0, 0), 1.0, name.replace("SM_GEN_", ""), mats[key])
        if a:
            a.set_folder_path(folder + "/Oocyte")

    # Schwebeteilchen im Volumen rund um die Kamerawege
    debris_meshes = [meshes["SM_GEN_Debris_{:02d}".format(i)] for i in range(1, 7)]
    for i in range(650):
        loc = (random.uniform(-1500, 5600), random.gauss(0, 500), random.uniform(-200, 600))
        a = spawn_mesh(random.choice(debris_meshes), loc, (random.uniform(0, 360), random.uniform(0, 360), random.uniform(0, 360)),
                       random.uniform(0.6, 1.8), "Debris_{:03d}".format(i), mats["debris"], cast_shadow=False)
        if a:
            a.set_folder_path(folder + "/Debris")
    # Schwebeteilchen direkt vor der Kamera von SH_001_020 (Ziel des Fokuszugs, 70-160 cm vor dem Objektiv)
    cam, target = (-860, -295, 245), (400, 0, 60)
    direction = [target[i] - cam[i] for i in range(3)]
    length = math.sqrt(sum(d * d for d in direction))
    direction = [d / length for d in direction]
    for i in range(14):
        dist = random.uniform(70, 160) if i < 5 else random.uniform(160, 900)
        loc = tuple(cam[k] + direction[k] * dist + random.gauss(0, 14 + dist * 0.12) for k in range(3))
        a = spawn_mesh(random.choice(debris_meshes), loc, (random.uniform(0, 360), random.uniform(0, 360), random.uniform(0, 360)),
                       random.uniform(0.8, 1.6), "Debris_Foreground_{:02d}".format(i), mats["debris"], cast_shadow=False)
        if a:
            a.set_folder_path(folder + "/Debris")
    log("Set: Schwarm {} Spermien, 664 Schwebeteilchen".format(swarm))
    return hero, egg_loc, swarm_root


def build_light_and_atmosphere():
    folder = "SEQ001/Light"
    # Transmittiertes Gewebelicht: sehr grosse, warme Flaeche oben-hinten (2200 K)
    key = actors.spawn_actor_from_class(unreal.RectLight, unreal.Vector(-2600, -900, 2600), unreal.Rotator(0, -42, 18))
    key.set_actor_label("Light_TissueTransmission")
    lc = key.rect_light_component
    lc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    lc.set_editor_property("intensity", 1800000.0)
    lc.set_editor_property("use_temperature", True)
    lc.set_editor_property("temperature", 2200.0)
    lc.set_editor_property("source_width", 2400.0)
    lc.set_editor_property("source_height", 2400.0)
    lc.set_editor_property("attenuation_radius", 20000.0)
    lc.set_editor_property("barn_door_angle", 88.0)
    lc.set_editor_property("volumetric_scattering_intensity", 1.0)
    lc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    key.set_folder_path(folder)

    # Gegenlicht aus der Tiefe des Eileiters (Licht, das durch die duennere Wand dahinter faellt), schwaecher, 2600 K
    back = actors.spawn_actor_from_class(unreal.RectLight, unreal.Vector(7800, 300, 700), unreal.Rotator(0, -4, 180))
    back.set_actor_label("Light_DeepTransmission")
    bc = back.rect_light_component
    bc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    bc.set_editor_property("intensity", 700000.0)
    bc.set_editor_property("use_temperature", True)
    bc.set_editor_property("temperature", 2600.0)
    bc.set_editor_property("source_width", 3000.0)
    bc.set_editor_property("source_height", 2000.0)
    bc.set_editor_property("attenuation_radius", 20000.0)
    bc.set_editor_property("volumetric_scattering_intensity", 1.6)
    bc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    back.set_folder_path(folder)

    # Fluessigkeit: dichter, warm streuender volumetrischer Nebel (Eileiterfluessigkeit mit Schwebstoffen)
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -300), unreal.Rotator(0, 0, 0))
    fog.set_actor_label("Fluid_VolumetricFog")
    fc = fog.component
    fc.set_editor_property("fog_density", 0.35)
    fc.set_editor_property("fog_height_falloff", 0.0005)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.02, 0.006, 0.004, 1))
    fc.set_editor_property("volumetric_fog", True)
    fc.set_editor_property("volumetric_fog_scattering_distribution", 0.55)
    fc.set_editor_property("volumetric_fog_albedo", unreal.Color(r=255, g=214, b=190, a=255))
    fc.set_editor_property("volumetric_fog_extinction_scale", 1.0)
    fc.set_editor_property("volumetric_fog_distance", 6000.0)
    fog.set_folder_path(folder)

    # Kamera-Look: manuelle Belichtung, dezente Optik
    ppv = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    ppv.set_actor_label("PP_SEQ001_Look")
    ppv.set_editor_property("unbound", True)
    s = ppv.settings
    for prop, value in (("override_auto_exposure_method", True), ("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL),
                        ("override_auto_exposure_apply_physical_camera_exposure", True), ("auto_exposure_apply_physical_camera_exposure", False),
                        ("override_auto_exposure_bias", True), ("auto_exposure_bias", float(os.environ.get("GENESIS_SEQ001_EV", "10.0"))),
                        ("override_bloom_intensity", True), ("bloom_intensity", 0.15),
                        ("override_vignette_intensity", True), ("vignette_intensity", 0.22),
                        ("override_scene_fringe_intensity", True), ("scene_fringe_intensity", 0.0),
                        ("override_film_grain_intensity", True), ("film_grain_intensity", 0.0),
                        ("override_motion_blur_amount", True), ("motion_blur_amount", 0.5),
                        ("override_motion_blur_max", True), ("motion_blur_max", 5.0),
                        ("override_lens_flare_intensity", True), ("lens_flare_intensity", 0.0),
                        ("override_white_temp", True), ("white_temp", 5200.0)):
        s.set_editor_property(prop, value)
    ppv.set_editor_property("settings", s)
    ppv.set_folder_path(folder)


# ---------------------------------------------------------------- Sequencer
def look_at(src, dst):
    d = unreal.Vector(dst[0] - src[0], dst[1] - src[1], dst[2] - src[2])
    yaw = math.degrees(math.atan2(d.y, d.x))
    pitch = math.degrees(math.atan2(d.z, math.hypot(d.x, d.y)))
    return pitch, yaw


def key_transform(section, keys):
    channels = {c.channel_name: c for c in section.get_all_channels()}
    for t, loc, rot in keys:
        for name, value in (("Location.X", loc[0]), ("Location.Y", loc[1]), ("Location.Z", loc[2]),
                            ("Rotation.X", rot[0]), ("Rotation.Y", rot[1]), ("Rotation.Z", rot[2])):
            if name in channels:
                channels[name].add_key(unreal.FrameNumber(frame(t)), value)


def build_sequence(hero, egg_loc, mpc, swarm_root):
    shots = [s for s in DATA["shots"] if s["seq"] == "SEQ_001_Origin"]
    start = shots[0]["start"]
    length = shots[-1]["end"] - start
    if eal.does_asset_exist(SUB_SEQ):
        eal.delete_asset(SUB_SEQ)
    folder, name = SUB_SEQ.rsplit("/", 1)
    seq = tools.create_asset(name, folder, unreal.LevelSequence, unreal.LevelSequenceFactoryNew())
    seq.set_display_rate(unreal.FrameRate(FPS, 1))
    seq.set_playback_start(0)
    seq.set_playback_end(frame(length))

    # Deterministische Zelldynamik
    mpc_track = seq.add_track(unreal.MovieSceneMaterialParameterCollectionTrack)
    mpc_track.set_editor_property("mpc", mpc)
    mpc_section = mpc_track.add_section()
    mpc_section.set_range(0, frame(length))
    mpc_section.add_scalar_parameter_key("SwimTime", unreal.FrameNumber(0), 0.0)
    mpc_section.add_scalar_parameter_key("SwimTime", unreal.FrameNumber(frame(length)), length)

    # Held: schwimmt mit 15 cm/s (Zeitlupen-Interpretation) durch die Shots 030-040, erreicht in 050 die Eizelle
    speed = 15.0
    t030, t040, t050 = 8.0 - start, 11.5 - start, 15.0 - start
    contact = (egg_loc[0] - 74.0 - 2.5, egg_loc[1], egg_loc[2])       # Corona-Aussenrand
    hero_keys = [
        (0.0, (-120.0, 0.0, 0.0), (0, 0, 0)),
        (t040, (-120.0 + speed * t040, 0.0, 0.0), (0, 0, 0)),
        (t050 - 0.0417, (-120.0 + speed * t050, 0.0, 0.0), (0, 1.5, 3)),
        (t050, (contact[0] - 70.0, contact[1] + 20.0, contact[2] - 8.0), (0, 2, -8)),
        (length, (contact[0] - 70.0 + 35.0 * (length - t050) / 2.0, contact[1] + 20.0 - 10.0 * (length - t050) / 2.0, contact[2] - 8.0), (0, 2, -8)),
    ]
    # Schwarm treibt gemeinsam mit der Stroemung (14 cm/s), einzelne Zellen behalten ihre Richtung/Phase
    root_binding = seq.add_possessable(swarm_root)
    rs = root_binding.add_track(unreal.MovieScene3DTransformTrack).add_section()
    rs.set_range(0, frame(length))
    key_transform(rs, [(0.0, (0.0, 0.0, 0.0), (0, 0, 0)), (length, (14.0 * length, 0.0, 0.0), (0, 0, 0))])
    if hero:
        hero_binding = seq.add_possessable(hero)
        hs = hero_binding.add_track(unreal.MovieScene3DTransformTrack).add_section()
        hs.set_range(0, frame(length))
        key_transform(hs, hero_keys)
        for c in hs.get_all_channels():
            for k in c.get_keys():
                try:
                    k.set_interpolation_mode(unreal.RichCurveInterpMode.RCIM_LINEAR)
                except Exception:
                    pass

    cut_track = seq.add_track(unreal.MovieSceneCameraCutTrack)
    specs = {
        # shot: (keys [(t_local_in_shot, cam_loc, look_target)], focus_distance cm, aperture)
        "SH_001_010_Darkness": ([(0.0, (-900, -300, 250), (400, 0, 60)), (4.0, (-880, -300, 245), (400, 0, 60))], 1200.0, 2.8),
        "SH_001_020_FirstLight": ([(0.0, (-900, -300, 250), (400, 0, 60)), (4.0, (-840, -290, 240), (400, 0, 60))], 5000.0, 2.8),
        "SH_001_030_SpermSwarm": ([(0.0, (-1200, -700, 900), (300, 0, 0)), (3.5, (-1120, -650, 620), (380, 0, 0))], 1500.0, 4.0),
        "SH_001_040_SpermTracking": ([(0.0, (-95, -62, 14), "hero"), (3.5, (-95 + 48, -60, 12), "hero")], 72.0, 8.0),
        "SH_001_050_Fertilization": ([(0.0, "orbit0", "contact"), (2.0, "orbit1", "contact")], 150.0, 11.0),
    }
    hero_start_x = -120.0
    for shot in shots:
        keys, focus, aperture = specs[shot["id"]]
        f0, f1 = frame(shot["start"] - start), frame(shot["end"] - start)
        binding = seq.add_spawnable_from_class(unreal.CineCameraActor)
        binding.set_name(shot["id"])
        cine = binding.get_object_template().get_editor_property("camera_component")
        fb = cine.get_editor_property("filmback")
        fb.sensor_width, fb.sensor_height = 24.89, 14.0
        cine.set_editor_property("filmback", fb)
        cine.set_editor_property("current_focal_length", float(shot["cam"]["lens"]))
        cine.set_editor_property("current_aperture", aperture)
        focus_settings = cine.get_editor_property("focus_settings")
        focus_settings.focus_method = unreal.CameraFocusMethod.MANUAL
        focus_settings.manual_focus_distance = focus
        cine.set_editor_property("focus_settings", focus_settings)
        spawn = binding.add_track(unreal.MovieSceneSpawnTrack)
        sp = spawn.add_section()
        sp.set_range(f0, f1)
        tt = binding.add_track(unreal.MovieScene3DTransformTrack)
        ts = tt.add_section()
        ts.set_range(f0, f1)
        world_keys = []
        for t_shot, loc, target in keys:
            t_seq = shot["start"] - start + t_shot
            hero_x = hero_start_x + speed * t_seq
            if loc == "orbit0" or loc == "orbit1":
                ang = math.radians(-30 if loc == "orbit0" else 10)          # Kreisbewegung (Kreis-Motiv)
                r = 190.0
                loc = (contact[0] - math.cos(ang) * r, contact[1] + math.sin(ang) * r - 40, contact[2] + 35)
            if target == "hero":
                target = (hero_x - 2.3, 0.0, 0.0)   # Kopfmitte (SM_GEN_SpermCell: Kopfspitze im Ursprung)
            elif target == "contact":
                target = contact
            pitch, yaw = look_at(loc, target)
            world_keys.append((t_seq, loc, (0.0, pitch, yaw)))
        key_transform(ts, world_keys)
        if shot["id"] == "SH_001_020_FirstLight":                      # Rack Focus: unendlich -> Schwebeteilchen
            focus_track = binding.add_track(unreal.MovieSceneFloatTrack)
            focus_track.set_property_name_and_path("ManualFocusDistance", "CameraComponent.FocusSettings.ManualFocusDistance")
            fsec = focus_track.add_section()
            fsec.set_range(f0, f1)
            ch = fsec.get_all_channels()[0]
            for t_shot, value in ((0.0, 5000.0), (1.6, 5000.0), (3.0, 95.0), (4.0, 95.0)):   # Fokuszug 1,4 s
                ch.add_key(unreal.FrameNumber(f0 + frame(t_shot)), value)
        cut = cut_track.add_section()
        cut.set_range(f0, f1)
        cut.set_camera_binding_id(seq.get_binding_id(binding))
    eal.save_loaded_asset(seq, only_if_is_dirty=False)
    log("SEQ_001_Origin: {} Shots, {} Frames".format(len(shots), frame(length)))
    return seq


def main():
    ensure_dir(ENV)
    meshes = import_meshes()
    tex = import_textures()
    mpc = make_mpc()
    m_mucosa = make_mucosa_material(tex)
    m_sperm = eal.load_asset(SPERM_MATERIAL) if eal.does_asset_exist(SPERM_MATERIAL) else None
    if meshes["Sperm"] and not isinstance(m_sperm, unreal.MaterialInterface):
        unreal.log_warning("GENESIS_TRAILER Spermium-Material fehlt noch: " + SPERM_MATERIAL + " - Mesh-Standardmaterial")
        m_sperm = None
    mats = {
        "mucosa": m_mucosa,
        "cyto": make_cell_material(MATS + "/M_GEN_OocyteCytoplasm", tex, (1.0, 0.92, 0.82), (0.95, 0.6, 0.4), 0.55, 0.85, 3.0, True),
        "zona": make_cell_material(MATS + "/M_GEN_OocyteZona", tex, (0.72, 0.68, 0.62), (0.95, 0.75, 0.6), 0.28, 0.95, 5.0),
        "corona": make_cell_material(MATS + "/M_GEN_OocyteCorona", tex, (0.6, 0.5, 0.42), (0.9, 0.5, 0.35), 0.4, 0.8, 2.0),
        "debris": make_debris_material(tex, mpc),
        "sperm_hero": make_instance(m_sperm, "MI_Trailer_SpermHero", {}, {"UseSequencerTime": True}) if m_sperm else None,
        "sperm_swarm": make_instance(m_sperm, "MI_Trailer_SpermSwarm", {}, {"UseSequencerTime": True}) if m_sperm else None,
    }
    world = new_level()
    hero, egg_loc, swarm_root = build_set(meshes, mats)
    build_light_and_atmosphere()
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build_sequence(hero, egg_loc, mpc, swarm_root)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()

    count = len(actors.get_all_level_actors())
    log("Level {} gespeichert, Actors {}".format(MAP, count))
    if not failures:
        unreal.log("GENESIS_TRAILER_OK")


main()
