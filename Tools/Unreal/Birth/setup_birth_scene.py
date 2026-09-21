# GENESIS – Geburt: importiert den Geburtskanal, baut sein Material und die Szene L_GEN_Birth (idempotent).
#
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Birth/setup_birth_scene.py" -unattended
#
# Maßstab der Geburtsszene: 1 mm = 1 Unreal-Einheit (der Kanal ist 140 Einheiten lang).
# Das ist ein anderer Maßstab als die Mikrowelt der Zeugung (dort 1 µm = 1 Einheit) – beide Szenen
# sind eigene Level, deshalb stören sie sich nicht.

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import unreal

ROOT = "/Game/Genesis/Birth"
MESHES = ROOT + "/Meshes"
MATERIALS = ROOT + "/Materials"
MAP_PATH = ROOT + "/Maps/L_GEN_Birth"

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SOURCE = os.path.join(REPO, "ArtSource", "Generated", "Birth")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def log(message):
    unreal.log("GENESIS: " + message)


def ensure_folders():
    for folder in (ROOT, MESHES, MATERIALS, ROOT + "/Maps"):
        if not eal.does_directory_exist(folder):
            eal.make_directory(folder)


def import_mesh(fbx_name, asset_name, nanite=True):
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX false")
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    data = options.get_editor_property("static_mesh_import_data")
    data.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
    data.set_editor_property("combine_meshes", True)
    data.set_editor_property("auto_generate_collision", False)
    data.set_editor_property("generate_lightmap_u_vs", False)
    data.set_editor_property("build_nanite", nanite)
    data.set_editor_property("remove_degenerates", False)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("compute_weighted_normals", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, fbx_name))
    task.set_editor_property("destination_path", MESHES)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])

    asset = eal.load_asset(MESHES + "/" + asset_name)
    if asset:
        log("%s importiert, Ausdehnung %s" % (asset_name, asset.get_bounds().box_extent))
    else:
        unreal.log_error("GENESIS: Import fehlgeschlagen: " + fbx_name)
    return asset


# ---------------------------------------------------------------------------------------------------------------------
# Material
# ---------------------------------------------------------------------------------------------------------------------
def load_or_create_material(name):
    full = MATERIALS + "/" + name
    if eal.does_asset_exist(full):
        material = eal.load_asset(full)
        mel.delete_all_material_expressions(material)
        material.set_editor_property("used_with_nanite", True)
        return material
    material = asset_tools.create_asset(name, MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    # Der Kanal ist ein Nanite-Mesh: Ohne diese Kennung zeigt die gebaute Fassung das graue Standardmaterial
    material.set_editor_property("used_with_nanite", True)
    return material


def expression(material, cls, x, y, **props):
    node = mel.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def connect(src, src_output, dst, dst_inputs):
    for name in dst_inputs:
        if mel.connect_material_expressions(src, src_output, dst, name):
            return True
    unreal.log_error("GENESIS: Verbindung fehlgeschlagen %s.%s -> %s %s" % (src.get_name(), src_output, dst.get_name(), dst_inputs))
    return False


def custom(material, x, y, description, code, inputs, output_type):
    node = expression(material, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("description", description)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", output_type)
    entries = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        entries.append(entry)
    node.set_editor_property("inputs", entries)
    return node


def constant(material, x, y, value):
    return expression(material, unreal.MaterialExpressionConstant, x, y, r=value)


def color(material, x, y, r, g, b):
    return expression(material, unreal.MaterialExpressionConstant3Vector, x, y,
                      constant=unreal.LinearColor(r, g, b, 1.0))


def lerp3(material, x, y, a, b, alpha, alpha_out=""):
    node = expression(material, unreal.MaterialExpressionLinearInterpolate, x, y)
    connect(a, "", node, ["A"])
    connect(b, "", node, ["B"])
    connect(alpha, alpha_out, node, ["Alpha"])
    return node


def multiply(material, x, y, a, b, a_out="", b_out=""):
    node = expression(material, unreal.MaterialExpressionMultiply, x, y)
    connect(a, a_out, node, ["A"])
    connect(b, b_out, node, ["B"])
    return node


def add(material, x, y, a, b, a_out="", b_out=""):
    node = expression(material, unreal.MaterialExpressionAdd, x, y)
    connect(a, a_out, node, ["A"])
    connect(b, b_out, node, ["B"])
    return node


FBM2D_CODE = """
float2x2 Rot = float2x2(0.8, -0.6, 0.6, 0.8);
float2 q = UV * Freq;
float v = 0.0;
float amp = 0.5;
for (int i = 0; i < 4; ++i)
{
	float2 ip = floor(q);
	float2 fp = q - ip;
	fp = fp * fp * (3.0 - 2.0 * fp);
	float n = 0.0;
	for (int c = 0; c < 4; ++c)
	{
		float2 o = float2(c & 1, (c >> 1) & 1);
		float h = frac(sin(dot(ip + o, float2(41.7, 289.3))) * 24634.6345);
		float2 w = lerp(1.0 - fp, fp, o);
		n += h * w.x * w.y;
	}
	v += n * amp;
	q = mul(Rot, q) * 2.07;
	amp *= 0.5;
}
return saturate(v);
"""


def create_canal_material():
    """
    Schleimhaut des Geburtskanals: feuchtes, durchblutetes Gewebe. Anders als im Mikrokosmos gibt es hier
    echten Glanz – der Schleimfilm liegt an Luft und spiegelt, sobald Licht von draußen hereinfällt.
    """
    material = load_or_create_material("M_GEN_BirthCanal")
    # Dünne Falten werden von hinten durchleuchtet, sobald Licht aus dem Ausgang kommt
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    material.set_editor_property("two_sided", True)

    uv = expression(material, unreal.MaterialExpressionTextureCoordinate, -1800, 0)
    fine = custom(material, -1500, 0, "Schleimhautkorn", FBM2D_CODE, ["UV", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(uv, "", fine, ["UV"])
    connect(constant(material, -1800, 160, 90.0), "", fine, ["Freq"])

    coarse = custom(material, -1500, 300, "Durchblutung", FBM2D_CODE, ["UV", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(uv, "", coarse, ["UV"])
    connect(constant(material, -1800, 380, 12.0), "", coarse, ["Freq"])

    # Farbattribut aus Blender: R = Weg durch den Kanal, G = Faltenrelief
    vertex = expression(material, unreal.MaterialExpressionVertexColor, -1500, 560)

    # Faltentäler sind dunkler und stärker durchblutet als die Kuppen
    relief = multiply(material, -1200, 420, vertex, coarse, "G")
    structure = add(material, -1000, 200,
                    multiply(material, -1200, 120, fine, constant(material, -1400, 180, 0.35)),
                    multiply(material, -1200, 500, relief, constant(material, -1400, 560, 0.65)))

    deep = color(material, -1000, -220, 0.075, 0.016, 0.013)
    high = color(material, -1000, -100, 0.30, 0.085, 0.07)
    base = lerp3(material, -700, -160, deep, high, structure)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Durchleuchtung: Gewebe vor Licht wird rot, nicht weiß
    transmission = color(material, -700, 120, 0.42, 0.055, 0.04)
    mel.connect_material_property(transmission, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    # Schleimfilm: glänzt, aber nicht überall gleich
    roughness = add(material, -700, 280, constant(material, -900, 280, 0.16),
                    multiply(material, -900, 350, fine, constant(material, -1050, 400, 0.30)))
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    spec = constant(material, -700, 470, 0.55)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_BirthCanal gebaut")
    return material


# ---------------------------------------------------------------------------------------------------------------------
# Szene
# ---------------------------------------------------------------------------------------------------------------------
def build_level(canal_mesh, canal_material):
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if canal_mesh and canal_material:
        canal_mesh.set_material(0, canal_material)
        eal.save_loaded_asset(canal_mesh)

        canal = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(-140.0, 0.0, 0.0))
        canal.set_actor_label("BirthCanal")
        component = canal.static_mesh_component
        component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
        component.set_static_mesh(canal_mesh)
        component.set_editor_property("cast_shadow", True)

    # Ein schwacher Schein tief im Kanal: Ohne ihn wäre das Bild vor der Austreibung vollständig schwarz.
    # Physikalisch ist das Streulicht, das durch das gedehnte Gewebe dringt.
    glow = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(-60.0, 0.0, 0.0))
    glow.set_actor_label("TissueGlow")
    glow_component = glow.get_component_by_class(unreal.PointLightComponent)
    glow_component.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    glow_component.set_editor_property("intensity", 0.9)
    glow_component.set_editor_property("attenuation_radius", 400.0)
    glow_component.set_editor_property("temperature", 1900.0)
    glow_component.set_editor_property("use_temperature", True)
    glow_component.set_editor_property("cast_shadows", False)

    # Der Kreißsaal (GENESIS-035) ersetzt die Platzhalter – Kästen, Kugel und ein Licht ohne Herkunft.
    # Das Licht kommt jetzt aus dem Raum selbst: Fenster, gedimmte Deckenfelder, Wandleuchte.
    import delivery_room
    delivery_room.build(actors, import_mesh)

    rig = actors.spawn_actor_from_class(unreal.GenesisBirthCameraRig, unreal.Vector(0.0, 0.0, 0.0))
    rig.set_actor_label("ChildCamera")

    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -100000))
    fog.set_actor_label("MoistAir")
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    # Kaum wahrnehmbar: In einem Raum mit 1 mm = 1 Einheit macht der alte Wert (0,02) aus 5 m Luft
    # einen milchigen Nebel. Ein Kreißsaal hat keinen sichtbaren Dunst – nur einen Hauch im Fensterlicht.
    fog_component.set_editor_property("fog_density", 0.0012)
    fog_component.set_editor_property("fog_height_falloff", 0.00001)
    fog_component.set_editor_property("enable_volumetric_fog", True)
    fog_component.set_editor_property("volumetric_fog_scattering_distribution", 0.5)

    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    volume.set_actor_label("BirthPostProcess")
    volume.set_editor_property("unbound", True)
    settings = volume.get_editor_property("settings")
    for name, value in (
        ("bloom_intensity", 0.9),          # Das Licht draußen blendet – ein Neugeborenes hat keine Blende
        ("lens_flare_intensity", 0.0),
        ("scene_fringe_intensity", 0.0),
        ("film_grain_intensity", 0.12),
        ("motion_blur_amount", 0.4),
    ):
        settings.set_editor_property("override_" + name, True)
        settings.set_editor_property(name, value)
    volume.set_editor_property("settings", settings)

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    log("Geburtsszene gespeichert: %s -> %s" % (MAP_PATH, saved))


ensure_folders()
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)

mesh = eal.load_asset(MESHES + "/SM_GEN_BirthCanal") if os.environ.get("GENESIS_SKIP_CANAL_IMPORT") \
    else import_mesh("SM_GEN_BirthCanal.fbx", "SM_GEN_BirthCanal")
build_level(mesh, create_canal_material())
