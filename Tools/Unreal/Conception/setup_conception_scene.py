# GENESIS – Entstehung: importiert die Spermienzelle, baut Material + Parameter-Collection und die Eileiter-Szene (idempotent).
#
# Ausführung headless (Editor geschlossen oder ohne diese Assets geöffnet):
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Conception/setup_conception_scene.py" -unattended
#
# Maßstab der Mikrowelt: 1 µm = 1 Unreal-Einheit (cm). Aussehen-Referenz: Blender-Look-Dev (Tools/Blender/Conception/build_sperm_cell.py).

import math
import os
import sys

import unreal

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from genesis_vertex_colors import log_vertex_colors

ROOT = "/Game/Genesis/Conception"
CELLS = ROOT + "/Cells"
MATERIALS = ROOT + "/Materials"
MAPS = ROOT + "/Maps"
MAP_PATH = MAPS + "/L_GEN_OviductAmpulla"

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SPERM_FBX = os.path.join(REPO, "ArtSource", "Generated", "Conception", "SM_GEN_SpermCell.fbx")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def log(message):
    unreal.log("GENESIS: " + message)


def ensure_folders():
    for folder in (ROOT, CELLS, MATERIALS, MAPS, ROOT + "/Environment"):
        if not eal.does_directory_exist(folder):
            eal.make_directory(folder)


# ---------------------------------------------------------------------------------------------------------------------
# Import
# ---------------------------------------------------------------------------------------------------------------------
def import_sperm_mesh():
    # Klassischer FBX-Weg statt Interchange: nur er nimmt die Einstellungen aus FbxImportUI unten entgegen
    # (Vertexfarben kommen auf beiden Wegen an – gemessen 2026-09-22, siehe Tools/Unreal/genesis_vertex_colors.py).
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
    data.set_editor_property("build_nanite", False)
    data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    # Mikrowelt: feinste Geißelflächen liegen unter der Standard-Schwelle für entartete Dreiecke
    data.set_editor_property("remove_degenerates", False)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("import_uniform_scale", float(os.environ.get("GENESIS_IMPORT_SCALE", "1.0")))

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SPERM_FBX)
    task.set_editor_property("destination_path", CELLS)
    task.set_editor_property("destination_name", "SM_GEN_SpermCell")
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])

    mesh = eal.load_asset(CELLS + "/SM_GEN_SpermCell")
    if not mesh:
        unreal.log_error("GENESIS: Import der Spermienzelle fehlgeschlagen: " + SPERM_FBX)
        return None
    bounds = mesh.get_bounds()
    log("Spermienzelle Bounds Ursprung %s Ausdehnung %s" % (bounds.origin, bounds.box_extent))
    # "Zones" trägt die Abschnitte der Zelle; das Material rechnet mit VC.g und VC.b
    log_vertex_colors("Spermienzelle", mesh, expect=("g", "b"))
    return mesh


# ---------------------------------------------------------------------------------------------------------------------
# Material
# ---------------------------------------------------------------------------------------------------------------------
def load_or_create(name, path, asset_class, factory):
    full = path + "/" + name
    if eal.does_asset_exist(full):
        return eal.load_asset(full)
    return asset_tools.create_asset(name, path, asset_class, factory)


def create_parameter_collection():
    mpc = load_or_create("MPC_GEN_Microworld", MATERIALS, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    names = [p.get_editor_property("parameter_name") for p in mpc.get_editor_property("scalar_parameters")]
    if "SwimTime" not in [str(n) for n in names]:
        parameter = unreal.CollectionScalarParameter()
        parameter.set_editor_property("parameter_name", "SwimTime")
        parameter.set_editor_property("default_value", 0.0)
        mpc.set_editor_property("scalar_parameters", list(mpc.get_editor_property("scalar_parameters")) + [parameter])
    eal.save_loaded_asset(mpc)
    return mpc


def expression(material, cls, x, y, **props):
    node = mel.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def connect(src, src_output, dst, dst_inputs):
    """Verbindet über mögliche Pin-Namen; meldet einen Fehler, statt still zu scheitern."""
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


WPO_CODE = """
// Geißelschlag als Winkelwelle (GENESIS-038, Docs/26): Nicht die Lage, sondern der Tangentenwinkel ψ der Geißel
// schwingt – ψ(s,t) = ψ0(s) + A(s)·[sin(k·s − ωt) + ε·sin(2(k·s − ωt) + φ2)], A wächst vom Mittelstück zur Spitze.
// Die Mittellinie ist das Integral des Winkels, dadurch bleibt die Geißel exakt 55 µm lang. Die frühere seitliche
// Verschiebung dehnte die Geißel bei kurzen Wellen um ein Vielfaches und ließ sie zappeln statt schlagen.
// Lage entlang der Zelle aus der UV (V: 0 Kopfspitze … 1 Geißelende), lokale Achsen: X vorn, Y Schlagseite.
float s = UV.y * 60.6;                       // µm ab Kopfspitze
const float s0 = 5.6;                        // Hals: davor starr (Kopf)
const float L = 55.0;
if (s <= s0) return float3(0.0, 0.0, 0.0);

float asym = saturate(Asym);
float k = 6.2831853 / max(Lambda, 1.0);
float w = 6.2831853 * Phase;
float A0 = 0.10 + 0.40 * asym;               // Auslenkung am Mittelstück (rad)
float eps = 0.05 + 0.25 * asym;              // Oberwelle – hyperaktiviert peitschenartig
float bias = 0.45 * asym;                    // einseitige Grundkrümmung

// Mittellinie numerisch integrieren (Mittelpunktregel)
const int N = 24;
float ds = (s - s0) / N;
float2 p = float2(-s0, 0.0);
for (int i = 0; i < N; ++i)
{
    float sm = (i + 0.5) * ds;
    float tt = sm / L;
    float ramp = saturate(sm / 1.5);         // weicher Übergang am Hals, kein Knick
    float x = k * sm - w;
    float a = bias * tt + ramp * lerp(A0, Amp, tt) * (sin(x) + eps * sin(2.0 * x + 1.3));
    p += float2(-cos(a), sin(a)) * ds;
}
// Der ganze Ring des Querschnitts wandert auf die gebogene Mittellinie. Die Drehung des Querschnitts
// selbst (Dicke unter 1 µm) sieht man nicht. Positionsknoten liefern bei Instanzen keine verlässliche
// Lage im Mesh – ein erster Versuch damit schleuderte die Geißeln quer durch den Kanal.
return float3(p.x + s, p.y, 0.0);
"""

PHASE_CODE = """
// Einzelobjekte ohne Instanzdaten: Phasenversatz aus der Objektposition, damit nicht alle Geißeln im Gleichtakt schlagen
float h = frac(sin(dot(ObjPos, float3(0.1371, 0.2713, 0.4191))) * 43758.5453);
return frac(T * BeatHz + h);
"""

OPACITY_CODE = """
// Optische Dichte entlang der Zelle (Referenz: Blender-Volumenmaterial). Kern und postakrosomale Region am dichtesten,
// Mittelstück (Mitochondrien) mittel, Geißel fast durchsichtig. Dünnere Weglänge am Rand (Blickwinkel).
float s = UV.y * 60.6;
float post = smoothstep(2.3, 2.8, s) * (1.0 - smoothstep(3.9, 4.6, s));
float core = smoothstep(0.8, 1.8, s) * (1.0 - smoothstep(3.4, 4.3, s));
float tau = 0.03 + 0.55 * VC.b + 0.55 * post + 0.40 * core + 0.30 * VC.g;

// Weglänge durch die Zelle: mittig blickt man durch die volle Dicke, am Rand nur streifend.
// Die Kante streut zusätzlich stark (Brechung am Übergang Zelle/Flüssigkeit) – daraus entsteht der helle Saum,
// an dem man eine Zelle im Mikroskop überhaupt erst erkennt.
float facing = saturate(abs(dot(normalize(N), normalize(V))));
float rim = pow(1.0 - facing, 3.0);
tau *= 0.30 + 0.70 * facing;
// Der Saum gehört vor allem zum dicken Kopf; die haarfeine Geißel bliebe sonst als heller Draht stehen
tau += rim * (0.06 + 0.6 * VC.b);
return saturate(1.0 - exp(-tau));
"""

COLOR_CODE = """
// Streufarbe: blass, Mittelstück leicht gelblich (Cytochrome). Der dicht gepackte Kern streut am stärksten
// und hebt sich dadurch heller ab; die Akrosomkappe davor bleibt klarer.
float s = UV.y * 60.6;
float core = smoothstep(0.9, 1.9, s) * (1.0 - smoothstep(3.6, 4.4, s));
float3 base = lerp(float3(0.22, 0.215, 0.205), float3(0.21, 0.19, 0.16), VC.g);
return lerp(base, float3(0.34, 0.335, 0.32), core * 0.7);
"""


def create_sperm_material(mpc):
    path = MATERIALS + "/M_GEN_SpermCell"
    # Neu aufbauen statt löschen: Andere Level (Trailer) verweisen direkt auf das Material, ein Löschen
    # ließe dort leere Verweise zurück
    if eal.does_asset_exist(path):
        material = eal.load_asset(path)
        mel.delete_all_material_expressions(material)
    else:
        material = asset_tools.create_asset("M_GEN_SpermCell", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    # Vor der Schaerfentiefe zeichnen. Unreal legt durchscheinende Flaechen sonst in einen Pass
    # NACH der Schaerfentiefe - dann bleibt eine Zelle direkt vor der Linse gestochen scharf,
    # waehrend alles andere weich ist. Genau daran erkennt man ein Bild als gerechnet.
    material.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    material.set_editor_property("used_with_instanced_static_meshes", True)

    float1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
    float3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3

    local_pos = expression(material, unreal.MaterialExpressionLocalPosition, -1600, 0)
    vertex_color = expression(material, unreal.MaterialExpressionVertexColor, -1600, 300)
    texture_coords = expression(material, unreal.MaterialExpressionTextureCoordinate, -1600, 450)

    # Zeitquelle: Echtzeit oder Sequencer-Zeit (MPC) für bildgenaue Renders
    time = expression(material, unreal.MaterialExpressionTime, -2000, -600)
    swim_time = expression(material, unreal.MaterialExpressionCollectionParameter, -2000, -500)
    swim_time.set_editor_property("collection", mpc)
    swim_time.set_editor_property("parameter_name", "SwimTime")
    use_sequencer = expression(material, unreal.MaterialExpressionStaticSwitchParameter, -1800, -560, parameter_name="UseSequencerTime", default_value=False)
    connect(swim_time, "", use_sequencer, ["True", "A"])
    connect(time, "", use_sequencer, ["False", "B"])

    object_pos = expression(material, unreal.MaterialExpressionObjectPositionWS, -1800, -700)
    beat_hz = expression(material, unreal.MaterialExpressionScalarParameter, -1800, -440, parameter_name="BeatHz", default_value=15.0)
    fallback_phase = custom(material, -1500, -600, "Fallback-Phase", PHASE_CODE, ["ObjPos", "T", "BeatHz"], float1)
    connect(object_pos, "", fallback_phase, ["ObjPos"])
    connect(use_sequencer, "", fallback_phase, ["T"])
    connect(beat_hz, "", fallback_phase, ["BeatHz"])

    def instance_data(index, default_node, y):
        node = expression(material, unreal.MaterialExpressionPerInstanceCustomData, -1200, y, data_index=index)
        connect(default_node, "", node, ["DefaultValue", "Default Value"])
        return node

    # Standardwerte für Einzelobjekte ohne Instanzdaten: progressive Zelle in zäher Eileiterflüssigkeit
    tip_amp = expression(material, unreal.MaterialExpressionScalarParameter, -1500, -300, parameter_name="TipAngleRad", default_value=0.75)
    asym = expression(material, unreal.MaterialExpressionScalarParameter, -1500, -200, parameter_name="Asymmetry", default_value=0.05)
    wavelength = expression(material, unreal.MaterialExpressionScalarParameter, -1500, -100, parameter_name="WavelengthUm", default_value=17.0)
    phase = instance_data(0, fallback_phase, -600)
    amplitude = instance_data(1, tip_amp, -300)
    asymmetry = instance_data(2, asym, -200)
    lam = instance_data(3, wavelength, -100)

    wpo = custom(material, -800, -300, "Geisselschlag", WPO_CODE, ["UV", "Phase", "Amp", "Asym", "Lambda"], float3)
    connect(texture_coords, "", wpo, ["UV"])
    connect(phase, "", wpo, ["Phase"])
    connect(amplitude, "", wpo, ["Amp"])
    connect(asymmetry, "", wpo, ["Asym"])
    connect(lam, "", wpo, ["Lambda"])
    to_world = expression(material, unreal.MaterialExpressionTransform, -500, -300)
    to_world.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    to_world.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    connect(wpo, "", to_world, ["", "Input"])
    mel.connect_material_property(to_world, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    normal = expression(material, unreal.MaterialExpressionVertexNormalWS, -1200, 500)
    camera_vector = expression(material, unreal.MaterialExpressionCameraVectorWS, -1200, 600)
    opacity = custom(material, -800, 300, "Optische Dichte", OPACITY_CODE, ["UV", "VC", "N", "V"], float1)
    connect(texture_coords, "", opacity, ["UV"])
    connect(vertex_color, "", opacity, ["VC"])
    connect(normal, "", opacity, ["N"])
    connect(camera_vector, "", opacity, ["V"])
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    color = custom(material, -800, 100, "Streufarbe", COLOR_CODE, ["UV", "VC"], float3)
    connect(texture_coords, "", color, ["UV"])
    connect(vertex_color, "", color, ["VC"])
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Relativer Brechungsindex ~1,04 in Flüssigkeit: kaum Spiegelung (F0 ≈ 0,0004 → Specular ≈ 0,005)
    specular = expression(material, unreal.MaterialExpressionConstant, -500, 500, r=0.005)
    mel.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    roughness = expression(material, unreal.MaterialExpressionConstant, -500, 600, r=0.18)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_SpermCell erstellt")

    instance = load_or_create("MI_GEN_SpermCell", MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, material)
    eal.save_loaded_asset(instance)
    return instance


# ---------------------------------------------------------------------------------------------------------------------
# Szene
# ---------------------------------------------------------------------------------------------------------------------
MUCOSA_SURFACE_CODE = """
// Gewebeoberfläche: Kapillarnetz (unregelmäßig, verzweigt) und Epithel-Zellmosaik.
// Cavity = Vertexfarbe R (tief in den Spalten), Height = G (0 Wandbasis … 1 Faltenspitze), Variation = B.
float cavity = VC.r;
float height = VC.g;

// Kapillaren: zwei Maßstäbe, nur dort sichtbar, wo das Epithel dünn ist; keine geschlossenen Waben
float fine  = saturate(1.0 - abs(Vessel1 - 0.42) * 20.0);
float coarse = saturate(1.0 - abs(Vessel2 - 0.50) * 9.0);
float vessels = saturate(fine * (0.45 + 0.55 * Break) + coarse * 0.55 * saturate(Break - 0.25));
vessels *= saturate(0.35 + 0.9 * height);   // an den Faltenspitzen liegt das Gefäßnetz dichter unter der Oberfläche

// Farbe: Spitzen blasser und wärmer, Spalten dunkler und gesättigter (weniger Licht, mehr Blut darunter)
float3 tip  = float3(0.52, 0.28, 0.25);
float3 deep = float3(0.26, 0.085, 0.075);
float3 base = lerp(deep, tip, saturate(height * 0.85 + 0.15 * VC.b));
base *= lerp(1.0, 0.55, cavity);
base = lerp(base, float3(0.32, 0.055, 0.05), vessels * 0.55);

// Zellmosaik moduliert die Helligkeit minimal (Zellkuppen heller als die Grenzen)
// Epithel: Flimmer- und Drüsenzellen liegen in Flecken, die Zellgrößen wechseln – kein gleichmäßiges Strickmuster
float mosaic = lerp(Cells, Cells2, saturate(Patch * 1.4 - 0.2));
base *= 0.88 + 0.26 * mosaic;
base *= 1.0 - 0.10 * saturate(Patch - 0.55);   // Drüsenzellfelder etwas matter und dunkler

// Schleimfilm: überall feucht, an den umströmten Faltenspitzen am glattesten
float roughness = lerp(0.42, 0.22, saturate(height));
roughness *= 0.88 + 0.28 * (1.0 - mosaic) + 0.12 * saturate(Patch - 0.5);
Roughness = saturate(roughness);
Vessels = vessels;
return base;
"""

MUCOSA_NORMAL_CODE = """
// Relief aus den Höhenwerten dreier versetzter Abtastungen (Zellmosaik + grobe Unebenheit) als Weltnormale
float3 gradient = float3(Hx - H0, Hy - H0, Hz - H0) / max(Delta, 0.0001);
float3 perturbed = normalize(WorldNormal - gradient * Strength);
return perturbed;
"""


def create_mucosa_material():
    path = MATERIALS + "/M_GEN_OviductMucosa"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    material = asset_tools.create_asset("M_GEN_OviductMucosa", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    # Nanite-Mesh: ohne Kennung zeigt die gebaute Fassung das Standardmaterial
    material.set_editor_property("used_with_nanite", True)
    # Schleimhautfalten sind dünne Blätter (50–120 µm). Vom Licht abgewandte Seiten werden von hinten durchleuchtet,
    # deshalb das Modell für beidseitig durchscheinendes Gewebe statt einfachem Subsurface.
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    material.set_editor_property("two_sided", True)

    float1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
    float3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3

    vertex_color = expression(material, unreal.MaterialExpressionVertexColor, -1800, 400)
    world_position = expression(material, unreal.MaterialExpressionWorldPosition, -2200, -200)

    def noise(scale_um, y, function, levels=2, turbulence=False, offset=None):
        """Prozedurales Rauschen in Weltkoordinaten (1 Einheit = 1 µm)."""
        position = world_position
        if offset is not None:
            add = expression(material, unreal.MaterialExpressionAdd, -2000, y)
            constant = expression(material, unreal.MaterialExpressionConstant3Vector, -2200, y + 80)
            constant.set_editor_property("constant", unreal.LinearColor(offset[0], offset[1], offset[2], 0.0))
            connect(world_position, "", add, ["A"])
            connect(constant, "", add, ["B"])
            position = add
        node = expression(material, unreal.MaterialExpressionNoise, -1700, y)
        node.set_editor_property("noise_function", function)
        node.set_editor_property("scale", 1.0 / scale_um)
        node.set_editor_property("levels", levels)
        node.set_editor_property("turbulence", turbulence)
        node.set_editor_property("output_min", 0.0)
        node.set_editor_property("output_max", 1.0)
        connect(position, "", node, ["Position", ""])
        return node

    voronoi = unreal.NoiseFunction.NOISEFUNCTION_VORONOI_ALU
    gradient = unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_ALU

    vessel_fine = noise(70.0, -900, voronoi, levels=1)
    vessel_coarse = noise(210.0, -1100, voronoi, levels=1)
    vessel_break = noise(320.0, -1300, gradient, levels=3, turbulence=True)
    cells = noise(11.0, -1500, voronoi, levels=1)
    cells_coarse = noise(16.0, -1600, voronoi, levels=1)
    patches = noise(190.0, -1700, gradient, levels=2)

    surface = custom(material, -1100, 0, "Schleimhaut", MUCOSA_SURFACE_CODE, ["VC", "Vessel1", "Vessel2", "Break", "Cells", "Cells2", "Patch"], float3)
    outputs = [unreal.CustomOutput(), unreal.CustomOutput()]
    outputs[0].set_editor_property("output_name", "Roughness")
    outputs[0].set_editor_property("output_type", float1)
    outputs[1].set_editor_property("output_name", "Vessels")
    outputs[1].set_editor_property("output_type", float1)
    surface.set_editor_property("additional_outputs", outputs)
    connect(vertex_color, "", surface, ["VC"])
    connect(vessel_fine, "", surface, ["Vessel1"])
    connect(vessel_coarse, "", surface, ["Vessel2"])
    connect(vessel_break, "", surface, ["Break"])
    connect(cells, "", surface, ["Cells"])
    connect(cells_coarse, "", surface, ["Cells2"])
    connect(patches, "", surface, ["Patch"])

    mel.connect_material_property(surface, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(surface, "Roughness", unreal.MaterialProperty.MP_ROUGHNESS)

    # Relief: Zellmosaik als Höhenfeld, Normale aus dem Gradienten dreier versetzter Abtastungen
    delta = 3.0
    height0 = noise(11.0, -1900, voronoi, levels=2)
    height_x = noise(11.0, -2100, voronoi, levels=2, offset=(delta, 0.0, 0.0))
    height_y = noise(11.0, -2300, voronoi, levels=2, offset=(0.0, delta, 0.0))
    height_z = noise(11.0, -2500, voronoi, levels=2, offset=(0.0, 0.0, delta))
    world_normal = expression(material, unreal.MaterialExpressionVertexNormalWS, -1700, -2700)
    delta_node = expression(material, unreal.MaterialExpressionConstant, -1700, -2750, r=delta)
    strength = expression(material, unreal.MaterialExpressionScalarParameter, -1700, -2800, parameter_name="ReliefStrength", default_value=4.0)
    normal_custom = custom(material, -1100, -2200, "Epithel-Relief", MUCOSA_NORMAL_CODE,
                           ["H0", "Hx", "Hy", "Hz", "Delta", "WorldNormal", "Strength"], float3)
    for source, pin in ((height0, "H0"), (height_x, "Hx"), (height_y, "Hy"), (height_z, "Hz"),
                        (delta_node, "Delta"), (world_normal, "WorldNormal"), (strength, "Strength")):
        connect(source, "", normal_custom, [pin])
    to_tangent = expression(material, unreal.MaterialExpressionTransform, -700, -2200)
    to_tangent.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
    to_tangent.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_TANGENT)
    connect(normal_custom, "", to_tangent, ["", "Input"])
    mel.connect_material_property(to_tangent, "", unreal.MaterialProperty.MP_NORMAL)

    # Subsurface: gut durchblutetes Bindegewebe unter dünnem Epithel
    # Durchleuchtungsfarbe: Blut im gut durchbluteten Bindegewebe – dünne Faltenränder glühen rötlich
    subsurface_color = expression(material, unreal.MaterialExpressionConstant3Vector, -700, 400)
    subsurface_color.set_editor_property("constant", unreal.LinearColor(0.42, 0.055, 0.04, 1.0))
    mel.connect_material_property(subsurface_color, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    # In Flüssigkeit ist der Brechungsunterschied klein; ein dünner Schleimfilm gibt aber etwas mehr Glanz als nacktes Gewebe
    specular = expression(material, unreal.MaterialExpressionConstant, -700, 600, r=0.035)
    mel.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_OviductMucosa erstellt")
    return material


CILIA_CODE = """
// Metachroner Schlag: Die Zilien schlagen nicht gleichzeitig, sondern als Welle, die über das Epithel läuft
// (Wellenlänge ~25 µm, 8 Schläge/s). Der Halm biegt sich zur Spitze hin immer stärker.
// Der Arbeitsschlag geht Richtung Gebärmutter (−X), die Rückholbewegung ist langsamer und flacher.
float h = UV.y;                       // 0 am Fuß, 1 an der Spitze
float jitter = UV.x;
float wave = sin(6.2831853 * (WorldPos.x / 25.0 - Time * 8.0 + jitter));
float stroke = wave >= 0.0 ? pow(wave, 0.6) : -pow(-wave, 1.6) * 0.55;   // schneller Arbeitsschlag, weiche Rückholung
float bend = pow(h, 1.8) * Amplitude;
return float3(-stroke * bend, 0.0, 0.0) + float3(0.0, 0.0, -0.25 * bend * (1.0 - abs(stroke)));
"""


def create_cilia_material():
    path = MATERIALS + "/M_GEN_Cilia"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    material = asset_tools.create_asset("M_GEN_Cilia", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    # Nanite-Mesh: ohne Kennung zeigt die gebaute Fassung das Standardmaterial
    material.set_editor_property("used_with_nanite", True)
    material.set_editor_property("two_sided", True)

    float3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
    uv = expression(material, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    world_position = expression(material, unreal.MaterialExpressionWorldPosition, -1200, 150)
    time = expression(material, unreal.MaterialExpressionTime, -1400, 300)
    time_scale = expression(material, unreal.MaterialExpressionScalarParameter, -1400, 380, parameter_name="TimeScale", default_value=0.25)
    scaled_time = expression(material, unreal.MaterialExpressionMultiply, -1200, 320)
    connect(time, "", scaled_time, ["A"])
    connect(time_scale, "", scaled_time, ["B"])
    amplitude = expression(material, unreal.MaterialExpressionScalarParameter, -1200, 450, parameter_name="BeatAmplitudeUm", default_value=2.6)

    beat = custom(material, -800, 100, "Metachroner Zilienschlag", CILIA_CODE, ["UV", "WorldPos", "Time", "Amplitude"], float3)
    connect(uv, "", beat, ["UV"])
    connect(world_position, "", beat, ["WorldPos"])
    connect(scaled_time, "", beat, ["Time"])
    connect(amplitude, "", beat, ["Amplitude"])
    mel.connect_material_property(beat, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    # Zilien sind fast durchsichtig; sichtbar werden sie durch Streuung – blasser Ton, kaum Spiegelung
    color = expression(material, unreal.MaterialExpressionConstant3Vector, -800, 400)
    color.set_editor_property("constant", unreal.LinearColor(0.30, 0.28, 0.27, 1.0))
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = expression(material, unreal.MaterialExpressionConstant, -800, 500, r=0.35)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    specular = expression(material, unreal.MaterialExpressionConstant, -800, 560, r=0.03)
    mel.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_Cilia erstellt")
    return material


def import_mesh(fbx_name, asset_name, destination, nanite, expect_colors=None):
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

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(REPO, "ArtSource", "Generated", "Conception", fbx_name))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])
    asset = eal.load_asset(destination + "/" + asset_name)
    if asset:
        log("%s importiert, Ausdehnung %s" % (asset_name, asset.get_bounds().box_extent))
        if expect_colors:
            log_vertex_colors(asset_name, asset, expect=expect_colors)
    else:
        unreal.log_error("GENESIS: Import fehlgeschlagen: " + fbx_name)
    return asset


def import_wall():
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
    data.set_editor_property("build_nanite", True)
    data.set_editor_property("remove_degenerates", False)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("compute_weighted_normals", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(REPO, "ArtSource", "Generated", "Conception", "SM_GEN_OviductWall.fbx"))
    task.set_editor_property("destination_path", ROOT + "/Environment")
    task.set_editor_property("destination_name", "SM_GEN_OviductWall")
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])

    wall = eal.load_asset(ROOT + "/Environment/SM_GEN_OviductWall")
    if not wall:
        unreal.log_error("GENESIS: Import der Eileiterwand fehlgeschlagen")
        return None
    bounds = wall.get_bounds()
    log("Eileiterwand Bounds Ursprung %s Ausdehnung %s Nanite %s" % (bounds.origin, bounds.box_extent, wall.get_editor_property("nanite_settings").get_editor_property("enabled")))
    # R Spalttiefe, G Höhe in der Falte, B großflächige Variation – M_GEN_OviductMucosa rechnet mit allen dreien
    log_vertex_colors("Eileiterwand", wall, expect=("r", "g", "b"))
    return wall


def build_level(mesh, material, wall_mesh=None, wall_material=None):
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Eileiterabschnitt: zwei nahtlos aneinandergesetzte Segmente à 1.500 µm = Kanallänge des Schwimmmodells
    if wall_mesh:
        if wall_material:
            wall_mesh.set_material(0, wall_material)
            eal.save_loaded_asset(wall_mesh)
        # Vier Abschnitte: Die offenen Schnittflächen liegen weit außerhalb des Kamerabereichs
        cilia_mesh = eal.load_asset(ROOT + "/Environment/SM_GEN_OviductCilia")
        cilia_material = eal.load_asset(MATERIALS + "/M_GEN_Cilia")
        if cilia_mesh and cilia_material:
            cilia_mesh.set_material(0, cilia_material)
            eal.save_loaded_asset(cilia_mesh)

        for index in range(-1, 3):
            segment = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(index * 1500.0, 0.0, 0.0))
            segment.set_actor_label("OviductWall_%d" % (index + 1))
            component = segment.static_mesh_component
            component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
            component.set_static_mesh(wall_mesh)
            component.set_editor_property("cast_shadow", True)

            if cilia_mesh:
                # Flimmerhärchen: bewegen sich im Material, deshalb beweglich und ohne Schatten (bei 0,2 µm Dicke unsichtbar)
                cilia_actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(index * 1500.0, 0.0, 0.0))
                cilia_actor.set_actor_label("OviductCilia_%d" % (index + 1))
                cilia_component = cilia_actor.static_mesh_component
                cilia_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
                cilia_component.set_static_mesh(cilia_mesh)
                cilia_component.set_editor_property("cast_shadow", False)
                # Bei 0,2 µm Halmdicke bringt Raytracing nichts, kostet aber viel Speicher
                cilia_component.set_editor_property("visible_in_ray_tracing", False)
                cilia_component.set_editor_property("affect_distance_field_lighting", False)

        # Schwebeteilchen der Eileiterflüssigkeit (Zelltrümmer, Sekretflocken), driften mit dem Zilienstrom
        debris_meshes = [eal.load_asset(ROOT + "/Environment/SM_GEN_Debris_%02d" % (i + 1)) for i in range(4)]
        debris_meshes = [m for m in debris_meshes if m]
        if debris_meshes:
            particles = actors.spawn_actor_from_class(unreal.GenesisFluidParticles, unreal.Vector(0, 0, 0))
            particles.set_actor_label("TubalFluidParticles")
            particles.set_editor_property("meshes", debris_meshes)
            particles.set_editor_property("particle_count", 700)
            channel = particles.get_editor_property("channel")
            channel.set_editor_property("length_um", 3000.0)
            particles.set_editor_property("channel", channel)
            log("Schwebeteilchen: %d Formen" % len(debris_meshes))
    swarm = actors.spawn_actor_from_class(unreal.GenesisSpermSwarm, unreal.Vector(0, 0, 0))
    swarm.set_actor_label("SpermSwarm")
    swarm.set_editor_property("cell_mesh", mesh)
    swarm.set_editor_property("cell_material", material)
    # 6 000 Zellen: gemessen 97 FPS. Weniger sieht nicht nach Schwarm aus, mehr kostet den Spiel-Thread.
    swarm.set_editor_property("cell_count", 6000)
    swarm.set_editor_property("seed", 7)

    rig = actors.spawn_actor_from_class(unreal.GenesisMicroscopeCameraRig, unreal.Vector(-200, 0, 0))
    rig.set_actor_label("MicroscopeCamera")
    rig.set_editor_property("swarm", swarm)
    rig.set_editor_property("orbit_distance_um", 150.0)
    # Seitlicher Blick: nur quer zur Schwimmrichtung ist die Geißelwelle zu sehen
    rig.set_editor_property("orbit_azimuth_degrees", 96.0)
    rig.set_editor_property("orbit_elevation_degrees", 12.0)
    rig.set_editor_property("look_behind_head_um", 22.0)
    camera = rig.get_editor_property("camera")
    camera.set_editor_property("current_focal_length", 50.0)
    camera.set_editor_property("current_aperture", 16.0)
    light = rig.get_editor_property("endoscope_light")
    # Physikalisch: 3.000 cd ergeben im Arbeitsabstand ~110 µm (= 1,1 m skaliert) ca. 2.500 lx → Belichtung EV100 ≈ 10
    light.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    light.set_editor_property("intensity", float(os.environ.get("GENESIS_LIGHT_CD", "150.0")))
    # Mehrfachstreuung im Gewebe: ohne indirektes Licht sind die Faltenrücken hart schwarz
    light.set_editor_property("indirect_lighting_intensity", 1.0)
    light.set_editor_property("volumetric_scattering_intensity", 0.35)
    log("Endoskoplicht Intensität gelesen: %s (Einheit %s)" % (light.get_editor_property("intensity"), light.get_editor_property("intensity_units")))

    # Flüssigkeit: sehr dünn streuendes Medium – sichtbar nur im Lichtkegel des Endoskops, kein Umgebungslicht
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -100000))
    fog.set_actor_label("TubalFluid")
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_component.set_editor_property("fog_density", float(os.environ.get("GENESIS_FOG", "0.006")))
    fog_component.set_editor_property("fog_height_falloff", 0.00001)
    fog_component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    fog_component.set_editor_property("directional_inscattering_luminance", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    fog_component.set_editor_property("enable_volumetric_fog", True)
    fog_component.set_editor_property("volumetric_fog_scattering_distribution", 0.55)
    fog_component.set_editor_property("volumetric_fog_albedo", unreal.Color(235, 232, 225, 255))

    # Kamera-Nachbearbeitung: filmisch, zurückhaltend, manuelle Belichtung
    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    volume.set_actor_label("MicroscopePostProcess")
    volume.set_editor_property("unbound", True)
    settings = volume.get_editor_property("settings")
    for name, value in (
        ("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL),
        ("auto_exposure_bias", float(os.environ.get("GENESIS_EXPOSURE_BIAS", "14.0"))),
        ("auto_exposure_apply_physical_camera_exposure", False),
        ("bloom_intensity", 0.1),
        ("lens_flare_intensity", 0.0),
        ("scene_fringe_intensity", 0.0),
        ("vignette_intensity", 0.2),
        ("film_grain_intensity", 0.0),
        ("motion_blur_amount", 0.5),
        ("motion_blur_max", 5.0),
    ):
        settings.set_editor_property("override_" + name, True)
        settings.set_editor_property(name, value)
    volume.set_editor_property("settings", settings)

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    log("Level gespeichert: %s -> %s" % (MAP_PATH, saved))


ensure_folders()
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)
sperm_mesh = eal.load_asset(CELLS + "/SM_GEN_SpermCell") if os.environ.get("GENESIS_SKIP_SPERM_IMPORT") else import_sperm_mesh()
collection = create_parameter_collection()
sperm_material = create_sperm_material(collection)
mucosa_material = create_mucosa_material()
create_cilia_material()
if not os.environ.get("GENESIS_SKIP_CILIA_IMPORT"):
    import_mesh("SM_GEN_OviductCilia.fbx", "SM_GEN_OviductCilia", ROOT + "/Environment", nanite=False)
oviduct_wall = eal.load_asset(ROOT + "/Environment/SM_GEN_OviductWall") if os.environ.get("GENESIS_SKIP_WALL_IMPORT") else import_wall()
# Mit GENESIS_SKIP_LEVEL bleibt das bestehende Level unangetastet (z. B. wenn nur ein Mesh neu importiert wird):
# build_level legt die Karte neu an und würde alles verlieren, was danach hineingesetzt wurde (Eizelle, Gallerte).
if sperm_mesh and not os.environ.get("GENESIS_SKIP_LEVEL"):
    build_level(sperm_mesh, sperm_material, oviduct_wall, mucosa_material)
