# GENESIS – übernimmt Kasackhose und Clogs der Hebamme (Tools/Blender/Birth/build_scrub_trousers.py) als Skeletal Meshes
# auf dem Skelett ihres Körpers und legt die Materialien an.
#   /Game/Genesis/People/Midwife/SK_GEN_MidwifeTrousers, SK_GEN_MidwifeClogs
#   /Game/Genesis/People/Materials/M_GEN_ScrubFabric, M_GEN_ClogPolymer, M_GEN_Hidden
# Die Meshes enthalten keine MetaHuman-Geometrie (eigene Modelle), nur ihre Hautgewichte für das Skelett.
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Birth/midwife_import_clothes.py" -unattended -nosplash
import os
import unreal

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SRC = os.path.join(REPO, "ArtSource", "Generated", "Birth", "Midwife")
DEST = "/Game/Genesis/People/Midwife"
MATS = "/Game/Genesis/People/Materials"
BODY = "/Game/Genesis/Characters/Midwife/Built/Midwife/Body/SKM_MHC_Midwife_BodyMesh"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(message):
    unreal.log_warning("GENESIS: " + message)


def fresh(name):
    path = MATS + "/" + name
    if eal.does_asset_exist(path):
        material = eal.load_asset(path)
        mel.delete_all_material_expressions(material)
        return material
    return tools.create_asset(name, MATS, unreal.Material, unreal.MaterialFactoryNew())


def node(material, cls, x, y, **props):
    result = mel.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        result.set_editor_property(key, value)
    return result


def scalar(material, x, y, name, value):
    return node(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)


def vector(material, x, y, name, rgb):
    return node(material, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def link(a, a_out, b, b_in):
    if not mel.connect_material_expressions(a, a_out, b, b_in):
        unreal.log_error("GENESIS: Verbindung fehlgeschlagen %s -> %s.%s" % (a.get_name(), b.get_name(), b_in))


def fabric():
    """
    Poly-Baumwoll-Köper: matt (Rauheit 0,8), ein heller Flaum an streifendem Licht (Fresnel), leicht wolkige Farbe
    (Waschgänge). Die Falten kommen aus der Geometrie (Stoffsimulation), nicht aus einer Textur.
    """
    m = fresh("M_GEN_ScrubFabric")
    m.set_editor_property("used_with_skeletal_mesh", True)
    m.set_editor_property("two_sided", True)
    color = vector(m, -900, -300, "Color", (0.030, 0.165, 0.185))
    # Ohne Eingang rechnet das Rauschen mit der Weltposition
    noise = node(m, unreal.MaterialExpressionNoise, -1100, -100, scale=0.004, levels=3,
                 output_min=0.93, output_max=1.04, noise_function=unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_ALU)
    tint = node(m, unreal.MaterialExpressionMultiply, -700, -300)
    link(color, "", tint, "A")
    link(noise, "", tint, "B")
    fresnel = node(m, unreal.MaterialExpressionFresnel, -900, 0, exponent=3.0, base_reflect_fraction=0.0)
    sheen = node(m, unreal.MaterialExpressionMultiply, -700, 0)
    link(fresnel, "", sheen, "A")
    link(scalar(m, -900, 120, "Sheen", 0.09), "", sheen, "B")
    base = node(m, unreal.MaterialExpressionAdd, -450, -200)
    link(tint, "", base, "A")
    link(sheen, "", base, "B")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(m, -450, 0, "Roughness", 0.8), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(scalar(m, -450, 80, "Specular", 0.35), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def polymer():
    """Clog aus weißem Kunststoff (EVA/TPE): leicht gebrochenes Weiß, halbmatt, etwas Lichtdurchgang am Rand."""
    m = fresh("M_GEN_ClogPolymer")
    m.set_editor_property("used_with_skeletal_mesh", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    noise = node(m, unreal.MaterialExpressionNoise, -900, -100, scale=0.02, levels=2,
                 output_min=0.96, output_max=1.02, noise_function=unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_ALU)
    color = vector(m, -900, -300, "Color", (0.70, 0.71, 0.69))
    tint = node(m, unreal.MaterialExpressionMultiply, -600, -250)
    link(color, "", tint, "A")
    link(noise, "", tint, "B")
    mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(m, -450, 0, "Roughness", 0.42), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(scalar(m, -450, 80, "Specular", 0.5), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(vector(m, -450, 160, "Subsurface", (0.55, 0.55, 0.5)), "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    mel.connect_material_property(scalar(m, -450, 260, "Opacity", 0.15), "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def hidden():
    m = fresh("M_GEN_Hidden")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("used_with_skeletal_mesh", True)
    mel.connect_material_property(node(m, unreal.MaterialExpressionConstant, -300, 0, r=0.0), "", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)


def import_skeletal(name, skeleton, material):
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX false")
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("skeleton", skeleton)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("create_physics_asset", False)
    data = options.get_editor_property("skeletal_mesh_import_data")
    data.set_editor_property("import_morph_targets", False)
    data.set_editor_property("update_skeleton_reference_pose", False)
    data.set_editor_property("use_t0_as_ref_pose", False)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SRC, name + ".fbx"))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    tools.import_asset_tasks([task])
    mesh = eal.load_asset(DEST + "/" + name)
    if not mesh:
        unreal.log_error("GENESIS: Import fehlgeschlagen: %s" % name)
        return
    materials = mesh.get_editor_property("materials")
    for slot in materials:
        slot.set_editor_property("material_interface", material)
    mesh.set_editor_property("materials", materials)
    eal.save_loaded_asset(mesh)
    log("%s: %s, Skelett %s" % (name, mesh.get_path_name(), mesh.skeleton.get_path_name()))


for folder in (DEST, MATS):
    if not eal.does_directory_exist(folder):
        eal.make_directory(folder)
body = unreal.load_asset(BODY)
skeleton = body.skeleton
hidden()
import_skeletal("SK_GEN_MidwifeTrousers", skeleton, fabric())
import_skeletal("SK_GEN_MidwifeClogs", skeleton, polymer())
