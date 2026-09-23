# GENESIS – der Embryo an Tag 28 in Unreal (GENESIS-041 Teil 4): Import und Gewebe. Die Szene baut
# setup_fruchthoehle.py; die frühere Prüfkarte L_GEN_EmbryoLookdev ist seit der Bestandsaufnahme (Doc 35) entfernt.
#
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Embryogenesis/setup_embryo.py" -unattended
#
# Voraussetzung: Tools/Blender/Embryogenesis/build_embryo_day28.py (export_all) hat die FBX-Dateien nach
# ArtSource/Generated/Embryogenesis geschrieben.
# Umgebungsvariablen: GENESIS_SKIP_IMPORT=1.
#
# Maßstab der Mikrowelt: 1 µm = 1 Unreal-Einheit. Der Embryo ist 4600 Einheiten lang.
#
# Aufbau wie in Blender: Mit vier Wochen ist der Embryo fast durchsichtig. Die Hülle ist deshalb
# durchscheinend (kein Nanite möglich, darum in Blender auf 30 % reduziert), die Organe darin sind
# undurchsichtig mit Streuung (Subsurface) und laufen über Nanite in voller Auflösung.

import os
import unreal

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SOURCE = os.path.join(REPO, "ArtSource", "Generated", "Embryogenesis")
ROOT = "/Game/Genesis/Embryogenesis"
MESHES = ROOT + "/Meshes"
MATERIALS = ROOT + "/Materials"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary

# Teil -> (Nanite, Materialinstanz)
PARTS = {
    "SM_GEN_Embryo28_Huelle": (False, None),
    "SM_GEN_Embryo28_Herz": (True, "MI_GEN_Embryo_Blut"),
    "SM_GEN_Embryo28_Gefaesse": (True, "MI_GEN_Embryo_Blut"),
    "SM_GEN_Embryo28_Neuralrohr": (True, "MI_GEN_Embryo_Neuralgewebe"),
    "SM_GEN_Embryo28_Somiten": (True, "MI_GEN_Embryo_Somiten"),
    "SM_GEN_Embryo28_Leberanlage": (True, "MI_GEN_Embryo_Leberanlage"),
}

# Gewebefarben wie im Blender-Lookdev (linear). Durchschein = Farbe des Lichts, das durch das Gewebe kommt.
ORGAN_LOOKS = {
    # Blut: dunkles Karmin, gesehen durch Herzbeutel und Körperwand
    # Subsurface in Unreal hellt stärker auf als in Cycles: Werte gegenüber Blender abgesenkt (gemessen am
    # ersten Bild – Neuralgewebe und Somiten erschienen kreideweiß, das Blut lachsfarben)
    # Dazu stehen die Organe im Licht, das schon durch Gewebe gegangen ist – dunkler als an der Oberfläche.
    # Am Bild gegen Blender gemessen (gleiche Stellen): Herz war 50 % zu hell, Hirn gleich hell, aber zu warm
    "MI_GEN_Embryo_Blut": ((0.065, 0.008, 0.007), (0.22, 0.015, 0.012), 0.40, 0.9),
    # Neuralgewebe lebend: milchig-glasig, nicht kreideweiß
    # Hirn und Somiten liegen dicht unter der Haut und bekommen kaum Schleier – deshalb nahe am Gewebeton,
    # sonst stehen sie als helle Masse und Perlenkette heraus
    "MI_GEN_Embryo_Neuralgewebe": ((0.21, 0.16, 0.17), (0.34, 0.22, 0.22), 0.45, 0.9),
    # Somiten: nur etwas dichter als das Gewebe ringsum – im goldenen Licht erschienen sie als helle Perlen
    "MI_GEN_Embryo_Somiten": ((0.22, 0.155, 0.155), (0.36, 0.21, 0.19), 0.45, 0.9),
    "MI_GEN_Embryo_Leberanlage": ((0.13, 0.032, 0.028), (0.30, 0.05, 0.04), 0.45, 0.8),
}


def log(message):
    unreal.log_warning("GENESIS: " + message)


def ensure_folders():
    for folder in (ROOT, MESHES, MATERIALS, ROOT + "/Maps"):
        if not eal.does_directory_exist(folder):
            eal.make_directory(folder)


def fresh_material(name):
    path = MATERIALS + "/" + name
    if eal.does_asset_exist(path):
        material = eal.load_asset(path)
        mel.delete_all_material_expressions(material)
    else:
        material = asset_tools.create_asset(name, MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    # Nanite-Meshes: ohne Kennung zeigt die gebaute Fassung still das Standardmaterial
    material.set_editor_property("used_with_nanite", True)
    return material


def expression(material, cls, x, y, **props):
    node = mel.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def vector_param(material, name, x, y, value):
    node = expression(material, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name)
    node.set_editor_property("default_value", unreal.LinearColor(value[0], value[1], value[2], 1.0))
    return node


def scalar_param(material, name, x, y, value):
    return expression(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)


def link(src, dst, pin, src_out=""):
    """Verbindung im Materialgraphen – ein falscher Anschlussname soll auffallen, nicht still fehlen.
    Knoten mit nur einem Eingang heißen je nach Typ "", "Input" oder "UV"; die Varianten werden probiert."""
    for name in (pin, "", "Input", "UV"):
        if mel.connect_material_expressions(src, src_out, dst, name):
            return True
    unreal.log_error("GENESIS: Verbindung fehlgeschlagen %s -> %s.%s" % (src.get_name(), dst.get_name(), pin))
    return False


# ------------------------------------------------------------------------------------------------
# Materialien
# ------------------------------------------------------------------------------------------------

def create_organ_material():
    """Organgewebe unter der Haut: streuend (Subsurface), im Fruchtwasser kaum spiegelnd."""
    material = fresh_material("M_GEN_EmbryoOrgan")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    colour = vector_param(material, "Farbe", -600, 0, (0.6, 0.5, 0.5))
    through = vector_param(material, "Durchschein", -600, 200, (0.8, 0.6, 0.55))
    rough = scalar_param(material, "Rauheit", -600, 400, 0.45)
    spec = scalar_param(material, "Glanz", -600, 500, 0.12)
    scatter = scalar_param(material, "Streuung", -600, 600, 0.85)
    mel.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(through, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(scatter, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def create_organ_instances(parent):
    instances = {}
    for name, (colour, through, rough, scatter) in ORGAN_LOOKS.items():
        path = MATERIALS + "/" + name
        if eal.does_asset_exist(path):
            mi = eal.load_asset(path)
        else:
            mi = asset_tools.create_asset(name, MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, parent)
        mel.set_material_instance_vector_parameter_value(mi, "Farbe", unreal.LinearColor(*colour, 1.0))
        mel.set_material_instance_vector_parameter_value(mi, "Durchschein", unreal.LinearColor(*through, 1.0))
        mel.set_material_instance_scalar_parameter_value(mi, "Rauheit", rough)
        mel.set_material_instance_scalar_parameter_value(mi, "Streuung", scatter)
        eal.save_loaded_asset(mi)
        instances[name] = mi
    return instances


def create_shell_material():
    """
    Die Hülle: eine einzelne Zellschicht über schwach streuendem Mesenchym. Frontal sieht man weit
    hinein, am Rand läuft der Blick lange durch Gewebe – dort ist sie dichter (Fresnel).
    Im Fruchtwasser ist der Brechzahlsprung winzig (1,38 zu 1,335): kaum Glanz, keine Brechung.
    """
    material = fresh_material("M_GEN_EmbryoHuelle")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    material.set_editor_property("two_sided", False)
    colour = vector_param(material, "Farbe", -800, 0, (0.78, 0.66, 0.66))

    # Das Mesenchym darunter streut (Blender: Volumen, freie Weglänge 1,7 mm). In Echtzeit nach
    # Beer-Lambert: Deckung = 1 - exp(-d / Weglänge), d = Gewebe zwischen Haut und dem Organ dahinter
    # (Szenentiefe minus Hauttiefe). Ein Organ dicht unter der Haut bleibt klar, ein tiefes wird milchig;
    # liegt nichts dahinter, zählt die ganze Körperdicke. Eine erste Fassung mit fester Deckung sah aus
    # wie Organe in Glas – ein Lehrmodell statt Gewebe.
    scene_depth = expression(material, unreal.MaterialExpressionSceneDepth, -800, 250)
    pixel_depth = expression(material, unreal.MaterialExpressionPixelDepth, -800, 350)
    # Wirksame Weglänge 0,7 mm statt der 1,7 mm aus Blender: Dort wird das Licht zweimal durch Gewebe
    # geschwächt (auf dem Weg zum Organ und zurück) und streut milchig; hier treffen die Lichter die Organe
    # ungeschwächt. Mit 1,7 mm blieb das Herz (0,4 mm tief) fast klar.
    path_length = scalar_param(material, "Weglaenge", -800, 450, 700.0)       # µm = Einheiten
    # 0,9 mm: mittlere Rumpfdicke. Mit 1,4 mm deckte die Hülle dort, wo nichts dahinter liegt, zu 86 % ab
    # und wirkte wie Latex, sobald alle drei Lichter auf ihr lagen
    thickness = scalar_param(material, "Koerperdicke", -800, 550, 900.0)      # wo nichts dahinter liegt
    fresnel = expression(material, unreal.MaterialExpressionFresnel, -800, 650)
    fresnel.set_editor_property("exponent", 2.5)
    fresnel.set_editor_property("base_reflect_fraction", 0.0)
    edge = scalar_param(material, "RandDichte", -800, 750, 0.30)
    haze = expression(material, unreal.MaterialExpressionCustom, -400, 400)
    haze.set_editor_property("description", "Gewebe nach Beer-Lambert")
    haze.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    haze.set_editor_property("code",
        "float d = clamp(SceneD - PixelD, 0.0, Thick);\n"
        "float a = 1.0 - exp(-d / max(Path, 1.0));\n"
        "return saturate(a + Edge * Fres * (1.0 - a));\n")
    entries = []
    for name in ("SceneD", "PixelD", "Path", "Thick", "Fres", "Edge"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        entries.append(entry)
    haze.set_editor_property("inputs", entries)
    for source, pin in ((scene_depth, "SceneD"), (pixel_depth, "PixelD"), (path_length, "Path"),
                        (thickness, "Thick"), (fresnel, "Fres"), (edge, "Edge")):
        link(source, haze, pin)

    # Streuung verwischt, was dahinter liegt – umso stärker, je mehr Gewebe dazwischen ist (vorwärts
    # streuendes Licht). Ohne diese Unschärfe blieb jede Organkante messerscharf: ein Glasmodell.
    # Das Bild hinter der Haut wird mit 13 Abtastpunkten geholt; Radius (Bildanteil) = k * d / Tiefe.
    blur_k = scalar_param(material, "Unschaerfe", -800, 850, 0.30)
    radius = expression(material, unreal.MaterialExpressionCustom, -400, 700)
    radius.set_editor_property("description", "Streuradius im Bild")
    radius.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    radius.set_editor_property("code", "float d = clamp(SceneD - PixelD, 0.0, Thick);\nreturn K * d / max(PixelD, 1.0);\n")
    r_entries = []
    for name in ("SceneD", "PixelD", "Thick", "K"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        r_entries.append(entry)
    radius.set_editor_property("inputs", r_entries)
    for source, pin in ((scene_depth, "SceneD"), (pixel_depth, "PixelD"), (thickness, "Thick"), (blur_k, "K")):
        link(source, radius, pin)

    # Tiefenbewusst: Ein Abtastpunkt zählt nur, wenn hinter ihm Körper liegt (Szenentiefe innerhalb der
    # Körperdicke). Neben einem Organ liegt in Wirklichkeit weiteres Gewebe, im Bild aber der schwarze
    # Hintergrund – ohne diese Prüfung bekam jede Organkante einen dunklen Ring.
    # Ein SceneColor-Knoten im Graphen sorgt dafür, dass das Szenenbild überhaupt gebunden wird.
    keep = expression(material, unreal.MaterialExpressionSceneColor, 0, 1100)
    screen = expression(material, unreal.MaterialExpressionScreenPosition, 0, 1000)
    blurred = expression(material, unreal.MaterialExpressionCustom, 300, 1000)
    blurred.set_editor_property("description", "Tiefenbewusste Streuunschärfe")
    blurred.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    blurred.set_editor_property("code", """
float2 taps[13] = { float2(0, 0),
    float2(0.5, 0), float2(0.25, 0.433), float2(-0.25, 0.433), float2(-0.5, 0), float2(-0.25, -0.433), float2(0.25, -0.433),
    float2(0.866, 0.5), float2(0, 1), float2(-0.866, 0.5), float2(-0.866, -0.5), float2(0, -1), float2(0.866, -0.5) };
float aspect = View.ViewSizeAndInvSize.x * View.ViewSizeAndInvSize.w;
float3 acc = 0;
float wsum = 0;
for (int i = 0; i < 13; i++)
{
    float2 v = VUV + float2(taps[i].x, taps[i].y * aspect) * R;
    float2 buv = ViewportUVToBufferUV(v);
    float behind = CalcSceneDepth(buv) - PixelD;
    float w = (i == 0 || behind < Thick) ? 1.0 : 0.0;
    acc += DecodeSceneColorForMaterialNode(buv) * w;
    wsum += w;
}
return acc / max(wsum, 1.0) + Keep * 0.0;
""")
    b_entries = []
    for name in ("VUV", "R", "PixelD", "Thick", "Keep"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        b_entries.append(entry)
    blurred.set_editor_property("inputs", b_entries)
    link(screen, blurred, "VUV", "ViewportUV")
    link(radius, blurred, "R")
    link(pixel_depth, blurred, "PixelD")
    link(thickness, blurred, "Thick")
    link(keep, blurred, "Keep")

    # Zusammensetzen bei voller Deckung: Gewebe (beleuchtet) mit Anteil a, dahinter das verwischte Bild mit 1 - a
    tissue = expression(material, unreal.MaterialExpressionMultiply, 0, 0)
    link(colour, tissue, "A")
    link(haze, tissue, "B")
    through = expression(material, unreal.MaterialExpressionOneMinus, 600, 500)
    link(haze, through, "Input")
    behind = expression(material, unreal.MaterialExpressionMultiply, 800, 1000)
    link(blurred, behind, "A")
    link(through, behind, "B")
    mel.connect_material_property(tissue, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(behind, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(expression(material, unreal.MaterialExpressionConstant, 600, 600, r=1.0), "",
                                  unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(scalar_param(material, "Glanz", -800, 600, 0.06), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(scalar_param(material, "Rauheit", -800, 700, 0.32), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


# ------------------------------------------------------------------------------------------------
# Import
# ------------------------------------------------------------------------------------------------

def import_part(asset, nanite):
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX false")
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    data = options.get_editor_property("static_mesh_import_data")
    data.set_editor_property("combine_meshes", True)
    data.set_editor_property("auto_generate_collision", False)
    data.set_editor_property("generate_lightmap_u_vs", False)
    data.set_editor_property("build_nanite", nanite)
    data.set_editor_property("remove_degenerates", False)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("compute_weighted_normals", True)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, asset + ".fbx"))
    task.set_editor_property("destination_path", MESHES)
    task.set_editor_property("destination_name", asset)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])
    mesh = eal.load_asset(MESHES + "/" + asset)
    if not mesh:
        unreal.log_error("GENESIS: Import fehlgeschlagen: " + asset)
        return None
    # Ein Reimport behält die alte Nanite-Einstellung – ausdrücklich setzen (GENESIS-035)
    settings = mesh.get_editor_property("nanite_settings")
    if settings.get_editor_property("enabled") != nanite:
        settings.set_editor_property("enabled", nanite)
        mesh.set_editor_property("nanite_settings", settings)
    eal.save_loaded_asset(mesh)
    bounds = mesh.get_bounds()
    log("%s: Ausdehnung %s, Nanite %s" % (asset, bounds.box_extent, nanite))
    return mesh


def main():
    ensure_folders()
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)
    organ_material = create_organ_material()
    organ_instances = create_organ_instances(organ_material)
    shell_material = create_shell_material()
    imported = {}
    for part, (use_nanite, _) in PARTS.items():
        if os.environ.get("GENESIS_SKIP_IMPORT"):
            imported[part] = eal.load_asset(MESHES + "/" + part)
        else:
            imported[part] = import_part(part, use_nanite)
    log("Embryo importiert, Materialien gebaut (Szene: setup_fruchthoehle.py)")


# Als Bibliothek (setup_fruchthoehle.py) nur die Funktionen laden
if not os.environ.get("GENESIS_EMBRYO_LIB_ONLY"):
    main()
