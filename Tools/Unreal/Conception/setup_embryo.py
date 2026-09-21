# GENESIS – Embryo: Material der Furchungszellen und Keim-Actor in der Eileiter-Szene (idempotent).
#
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Conception/setup_embryo.py" -unattended
#
# Die Zellen des Keims werden als Instanzen der Eizell-Kugel (55 µm Radius) dargestellt; ihre Größe ergibt
# sich aus der Simulation. Der Farbton je Zelle kommt über Per-Instance-Daten aus dem Zustand.

import unreal

ROOT = "/Game/Genesis/Conception"
OOCYTE = ROOT + "/Oocyte"
MATERIALS = ROOT + "/Materials"
MAP_PATH = ROOT + "/Maps/L_GEN_OviductAmpulla"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def log(message):
    unreal.log("GENESIS: " + message)


def load_or_create_material(name):
    full = MATERIALS + "/" + name
    if eal.does_asset_exist(full):
        material = eal.load_asset(full)
        mel.delete_all_material_expressions(material)
        material.set_editor_property("used_with_nanite", True)
        return material
    material = asset_tools.create_asset(name, MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    # Nanite-Meshes: ohne Kennung zeigt die gebaute Fassung das Standardmaterial
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


# Zweidimensionales Wert-Rauschen auf der Kugel-UV: Die Körnung gehört zur Zelle und wandert mit ihr,
# anders als weltbezogenes Rauschen, das bei Instanzen für alle Zellen dasselbe Muster ergäbe.
FBM2D_CODE = """
// Drehung je Oktave (36,87°), damit sich die Raster der Oktaven nicht decken. Sie fehlte – der Shader
// kompilierte nicht, und das Spiel zeigte auf jeder Zelle das Ersatzraster der Engine (GENESIS-038).
const float2x2 Rot = float2x2(0.8, -0.6, 0.6, 0.8);
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


# Hoffman-Modulationskontrast (so zeigt jedes IVF-Labor den Keim, Zielbild ArtSource/Reference/kie/02):
# Die Helligkeit folgt dem Gefälle der optischen Weglänge in EINER Bildrichtung – eine Seite jeder Zelle
# hell gesäumt, die andere dunkel, die Mitte mittelgrau. Für eine Kugel ist das Gefälle an der Stelle q
# (Lage im Bild relativ zur Zellmitte, 0 Mitte … 1 Rand) proportional zu q.x / sqrt(1 − |q|²).
# q kommt aus der Flächennormale: Bei einer Kugel ist sie (P − Mitte)/R, ihr Anteil quer zur Blickrichtung
# ist also genau die Lage im Bild – unabhängig von Größe und Abstand der Zelle.
# Der Zellkern liegt in der Mitte und schimmert durch das klare Zytoplasma: eine glatte, runde Fläche mit
# eigenem Reliefsaum und ein bis zwei Kernkörperchen. Die Zygote zeigt stattdessen zwei Vorkerne, die
# sich in der Mitte berühren.
COMPOSE_CODE = """
float3 n = normalize(N);
float3 v = normalize(V);
float3 s = normalize(S - v * dot(S, v));
float3 u = normalize(cross(v, s));
float3 np = n - v * dot(n, v);
float2 q = float2(dot(np, s), dot(np, u));
float r2 = saturate(dot(q, q));

// Relief der Zelle
float relief = clamp(q.x / sqrt(max(1.0 - r2, 0.03)) * 0.42, -1.1, 1.1);

// Kern(e)
float nucleus = 0.0;
float detail = 0.0;
if (NucVis > 0.01)
{
    int count = Pro > 0.5 ? 2 : 1;
    float rn = Pro > 0.5 ? 0.24 : 0.30;
    float sep = Pro > 0.5 ? 0.235 : 0.0;
    for (int k = 0; k < count; ++k)
    {
        float sg = k == 0 ? 1.0 : -1.0;
        float2 d = (q - float2(0.03, sg * sep)) / rn;
        float dd = length(d);
        float inside = 1.0 - smoothstep(0.86, 1.0, dd);
        nucleus = max(nucleus, inside);
        // Saum der Kernhülle: dasselbe Relief im Kleinen
        float rim = smoothstep(0.72, 0.95, dd) * (1.0 - smoothstep(0.98, 1.12, dd));
        detail += rim * d.x * 1.3;
        // Kernkörperchen (Nucleoli): kleine dichte Punkte, bei Vorkernen in einer Reihe an der Berührungsseite
        float2 c1 = Pro > 0.5 ? float2(0.15, -sg * 0.45) : float2(0.28, -0.18);
        float2 c2 = Pro > 0.5 ? float2(-0.25, -sg * 0.40) : float2(-0.22, 0.25);
        float n1 = 1.0 - smoothstep(0.10, 0.17, length(d - c1));
        float n2 = 1.0 - smoothstep(0.08, 0.14, length(d - c2));
        // dunkler Punkt mit eigenem kleinen Reliefsaum
        detail += -0.45 * (n1 + n2) + 0.6 * (n1 * (d.x - c1.x) / 0.17 + n2 * (d.x - c2.x) / 0.14);
    }
    nucleus *= NucVis;
    detail *= NucVis;
}

// Körnung: im Kern fast glatt, im Zytoplasma fein gekörnt
float grain = 0.5 * Fine + 0.3 * Mid + 0.2 * Coarse;
float grainAmount = (1.0 - 0.8 * nucleus) * (0.85 + 0.3 * Tint);
float tone = 0.45 + (grain - 0.5) * 1.1 * grainAmount + relief * 0.34 + detail * 0.36 + nucleus * 0.03;
tone = saturate(tone);

float3 dark = float3(0.012, 0.0115, 0.0105);
float3 light = float3(0.13, 0.123, 0.112);
float3 col = lerp(dark, light, tone);
// Embryoblast etwas dichter gepackt: minimal wärmer und dunkler
col *= lerp(float3(1.0, 1.0, 1.0), float3(0.97, 0.93, 0.88), Inner);
return col;
"""


def create_blastomere_material():
    """
    Furchungszelle: Das Zytoplasma stammt aus der Eizelle, ist also ebenso körnig –
    nur verteilt es sich mit jeder Teilung auf mehr und kleinere Zellen.
    """
    material = load_or_create_material("M_GEN_Blastomere")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    # Die Zellen werden als Instanzen gezeichnet. Ohne diese Kennung setzt das Spiel das Ersatzmaterial
    # der Engine ein – ein graues Schachbrett auf jeder Zelle (gesehen in GENESIS-038).
    material.set_editor_property("used_with_instanced_static_meshes", True)

    float1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
    uv = expression(material, unreal.MaterialExpressionTextureCoordinate, -1700, 0)

    def fbm(y, description, frequency):
        node = custom(material, -1400, y, description, FBM2D_CODE, ["UV", "Freq"], float1)
        connect(uv, "", node, ["UV"])
        connect(constant(material, -1700, y + 80, frequency), "", node, ["Freq"])
        return node

    # Körnung in drei Größen: Granula (~1 µm), Organellenhaufen, Plasmaschlieren
    fine = fbm(-200, "Granula", 160.0)
    grain = fbm(0, "Zytoplasma-Korn", 42.0)
    coarse = fbm(300, "Plasmaschlieren", 6.5)

    # Per-Instance-Daten aus der Simulation: 0 Farbton, 1 Embryoblast, 2 Fragmentierung, 3 Kern sichtbar, 4 Vorkerne
    def instance_data(index, y):
        return expression(material, unreal.MaterialExpressionPerInstanceCustomData, -1400, y, data_index=index)
    tint = instance_data(0, 520)
    inner = instance_data(1, 620)
    nucleus = instance_data(3, 720)
    pronuclei = instance_data(4, 820)

    normal = expression(material, unreal.MaterialExpressionVertexNormalWS, -1400, 920)
    camera = expression(material, unreal.MaterialExpressionCameraVectorWS, -1400, 1020)
    # Scherrichtung des Hoffman-Kontrasts: fest im Bild (Bildrechts), wie der Modulator im Mikroskop
    right = expression(material, unreal.MaterialExpressionTransform, -1400, 1120)
    right.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_VIEW)
    right.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    connect(color(material, -1700, 1120, 1.0, 0.0, 0.0), "", right, ["", "Input"])

    compose = custom(material, -900, 300, "Hoffman-Kontrast", COMPOSE_CODE,
                     ["N", "V", "S", "Fine", "Mid", "Coarse", "Tint", "Inner", "NucVis", "Pro"],
                     unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    for node, pin in ((normal, "N"), (camera, "V"), (right, "S"), (fine, "Fine"), (grain, "Mid"), (coarse, "Coarse"),
                      (tint, "Tint"), (inner, "Inner"), (nucleus, "NucVis"), (pronuclei, "Pro")):
        connect(node, "", compose, [pin])
    mel.connect_material_property(compose, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Streufarbe neutral warmgrau statt rosa: Zytoplasma ist farblos, das Rosa war erfunden
    subsurface = color(material, -420, 200, 0.075, 0.07, 0.062)
    mel.connect_material_property(subsurface, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    opacity = expression(material, unreal.MaterialExpressionScalarParameter, -420, 320,
                         parameter_name="ScatterAmount", default_value=0.14)
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    # Optischer Schnitt: Ein Mikroskop zeigt eine dünne Ebene durch die Mitte des Keims, nicht seine
    # Oberfläche. Ohne das war die Blastozyste eine Kugel aus Zellen, ohne Hohlraum und Embryoblast (gesehen
    # bei 110 h). Was vor der Schnittebene liegt, blendet gerastert aus – wie eine unscharfe Ebene.
    # Die Lage der Ebene setzt der Keim-Actor je Bild (SectionFrontUm = Tiefe, ab der gezeigt wird).
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    # Schwelle genau in der Mitte des Rasters: Mit der Vorgabe 0,333 blieben 17 % der ausgeblendeten Punkte
    # stehen – im Hohlraum ein Tarnmuster aus Zellresten (gesehen bei 118 h)
    material.set_editor_property("opacity_mask_clip_value", 0.5)
    depth = expression(material, unreal.MaterialExpressionPixelDepth, -900, 700)
    front = expression(material, unreal.MaterialExpressionScalarParameter, -900, 780,
                       parameter_name="SectionFrontUm", default_value=-1000000.0)
    # Gerastert (Interleaved Gradient Noise, je Bild versetzt), das Zeit-Antialiasing glättet es zu einem Übergang
    section = custom(material, -650, 720, "Optischer Schnitt", """
float fade = min(saturate((Depth - Front) / 2.5), saturate((Back - Depth) / 2.5));
float2 p = Parameters.SvPosition.xy + 5.588238 * float(View.StateFrameIndexMod8);
float noise = frac(52.9829189 * frac(dot(p, float2(0.06711056, 0.00583715))));
return fade + (noise - 0.5) * 0.98;
""", ["Depth", "Front", "Back"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(depth, "", section, ["Depth"])
    connect(front, "", section, ["Front"])
    back = expression(material, unreal.MaterialExpressionScalarParameter, -900, 860,
                      parameter_name="SectionBackUm", default_value=1000000.0)
    connect(back, "", section, ["Back"])
    mel.connect_material_property(section, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    roughness = add(material, -420, 430, constant(material, -650, 430, 0.30),
                    multiply(material, -650, 500, grain, constant(material, -820, 540, 0.12)))
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Zytoplasma gegen Flüssigkeit: fast kein Glanz (relativer Brechungsindex ~1,04)
    spec = constant(material, -420, 600, 0.03)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_Blastomere gebaut")
    return material


def place_embryo(material):
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.GenesisEmbryo):
            actors.destroy_actor(actor)

    oocytes = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.GenesisOocyte)]
    if not oocytes:
        unreal.log_error("GENESIS: Keine Eizelle im Level – der Keim braucht ihren Platz")
        return
    oocyte = oocytes[0]

    embryo = actors.spawn_actor_from_class(unreal.GenesisEmbryo, oocyte.get_actor_location())
    embryo.set_actor_label("Embryo")
    embryo.set_editor_property("oocyte", oocyte)
    embryo.set_editor_property("cell_mesh", eal.load_asset(OOCYTE + "/SM_GEN_OocyteCytoplasm"))
    embryo.set_editor_property("cell_material", material)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    log("Keim an der Eizelle gesetzt (%s), Level gespeichert: %s" % (oocyte.get_actor_location(), saved))


unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)
place_embryo(create_blastomere_material())
