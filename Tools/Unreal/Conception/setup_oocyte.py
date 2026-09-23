# GENESIS – Befruchtung: importiert die Eizelle, baut ihre Materialien und setzt sie in die Eileiter-Szene (idempotent).
#
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Conception/setup_oocyte.py" -unattended
#
# Maßstab: 1 µm = 1 Unreal-Einheit. Maße aus Tools/Blender/Conception/build_oocyte.py
# (Zellleib r = 55 µm, Zona 58–72 µm, Cumulus bis 118 µm).

import os

import unreal

ROOT = "/Game/Genesis/Conception"
OOCYTE = ROOT + "/Oocyte"
MATERIALS = ROOT + "/Materials"
MAP_PATH = ROOT + "/Maps/L_GEN_OviductAmpulla"

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SOURCE = os.path.join(REPO, "ArtSource", "Generated", "Conception")

# Lage im Kanal: in der Mitte des Abschnitts, leicht aus der Achse – die Eizelle treibt nicht exakt zentriert
OOCYTE_LOCATION = unreal.Vector(1500.0, 120.0, -30.0)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary


def log(message):
    unreal.log("GENESIS: " + message)


def ensure_folders():
    for folder in (ROOT, OOCYTE, MATERIALS):
        if not eal.does_directory_exist(folder):
            eal.make_directory(folder)


# ---------------------------------------------------------------------------------------------------------------------
# Import
# ---------------------------------------------------------------------------------------------------------------------
def import_mesh(fbx_name, asset_name, nanite, compute_normals=False):
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
    # Mikrowelt: feinste Flächen liegen unter der Standard-Schwelle für entartete Dreiecke
    data.set_editor_property("remove_degenerates", False)
    data.set_editor_property("convert_scene_unit", True)
    # Getrennte Zellhüllen in einem Mesh: berechnete Normalen glätten jede Hülle für sich
    data.set_editor_property("normal_import_method",
                             unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS if compute_normals else unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, fbx_name))
    task.set_editor_property("destination_path", OOCYTE)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])

    asset = eal.load_asset(OOCYTE + "/" + asset_name)
    if asset and nanite:
        # Schatten und Licht per Raytracing nehmen das Nanite-Ersatzmodell. Mit der Standard-Toleranz
        # (1,0) war es ein grober Polyeder, dessen Schatten die glatten Coronazellen eckig aussehen ließ
        # – sichtbar erst mit der schärferen Optik des Rennens (GENESIS-037).
        settings = asset.get_editor_property("nanite_settings")
        settings.set_editor_property("fallback_target", unreal.NaniteFallbackTarget.RELATIVE_ERROR)
        settings.set_editor_property("fallback_relative_error", 0.05)
        asset.set_editor_property("nanite_settings", settings)
        eal.save_loaded_asset(asset)
    if asset:
        log("%s importiert, Ausdehnung %s" % (asset_name, asset.get_bounds().box_extent))
    else:
        unreal.log_error("GENESIS: Import fehlgeschlagen: " + fbx_name)
    return asset


# ---------------------------------------------------------------------------------------------------------------------
# Material-Bausteine
# ---------------------------------------------------------------------------------------------------------------------
def load_or_create_material(name):
    """
    Bestehendes Material leeren statt löschen: Ein gelöschtes Asset, auf das das Level verweist,
    bleibt dort als alte Fassung hängen – Änderungen wären im Spiel unsichtbar.
    """
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


def local_position(material, x, y):
    """Position im Objekt (µm), damit das Rauschen mit der Eizelle mitwandert."""
    world = expression(material, unreal.MaterialExpressionWorldPosition, x, y)
    origin = expression(material, unreal.MaterialExpressionObjectPositionWS, x, y + 140)
    subtract = expression(material, unreal.MaterialExpressionSubtract, x + 200, y + 60)
    connect(world, "", subtract, ["A"])
    connect(origin, "", subtract, ["B"])
    return subtract


# Wert-Rauschen (3 Oktaven) – feine Körnung des Zytoplasmas
FBM_CODE = """
float3 p = P * Freq;
float v = 0.0;
float amp = 0.5;
for (int i = 0; i < 3; ++i)
{
	float3 ip = floor(p);
	float3 fp = p - ip;
	fp = fp * fp * (3.0 - 2.0 * fp);
	float n = 0.0;
	for (int c = 0; c < 8; ++c)
	{
		float3 o = float3(c & 1, (c >> 1) & 1, (c >> 2) & 1);
		float h = frac(sin(dot(ip + o, float3(12.9898, 78.233, 37.719))) * 43758.5453);
		float3 w = lerp(1.0 - fp, fp, o);
		n += h * w.x * w.y * w.z;
	}
	v += n * amp;
	p *= 2.03;
	amp *= 0.5;
}
return saturate(v);
"""

# Zellular (F1) – Granula im Ooplasma, Zellgrenzen der Corona
CELLULAR_CODE = """
float3 p = P * Freq;
float3 ip = floor(p);
float3 fp = p - ip;
float d = 8.0;
for (int x = -1; x <= 1; ++x)
for (int y = -1; y <= 1; ++y)
for (int z = -1; z <= 1; ++z)
{
	float3 o = float3(x, y, z);
	float3 g = ip + o;
	float3 h = frac(sin(float3(dot(g, float3(127.1, 311.7, 74.7)), dot(g, float3(269.5, 183.3, 246.1)), dot(g, float3(113.5, 271.9, 124.6)))) * 43758.5453);
	float3 r = o + h - fp;
	d = min(d, dot(r, r));
}
return saturate(sqrt(d));
"""

# Radiale Streifung der Zona pellucida: Glykoprotein-Fasern verlaufen senkrecht zur Oberfläche
ZONA_FIBRE_CODE = """
float3 dir = normalize(P + 0.0001);
float2 sph = float2(atan2(dir.y, dir.x), acos(clamp(dir.z, -1.0, 1.0)));
float2 q = sph * Freq;
float v = 0.0;
float amp = 0.6;
for (int i = 0; i < 3; ++i)
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
	q *= 2.11;
	amp *= 0.5;
}
return saturate(v);
"""


def scalar(material, x, y, name, value):
    return expression(material, unreal.MaterialExpressionScalarParameter, x, y,
                      parameter_name=name, default_value=value)


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


def smoothstep(material, x, y, value, value_out, low, high):
    """Weicher Übergang zwischen zwei Schwellen – für Fugen, Kanten und Übergänge."""
    node = expression(material, unreal.MaterialExpressionSmoothStep, x, y,
                      const_min=low, const_max=high)
    connect(value, value_out, node, ["Value"])
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


def fresnel(material, x, y, exponent, base):
    return expression(material, unreal.MaterialExpressionFresnel, x, y,
                      exponent=exponent, base_reflect_fraction=base)


# ---------------------------------------------------------------------------------------------------------------------
# Materialien
# ---------------------------------------------------------------------------------------------------------------------
def create_ooplasm_material():
    """Zellleib der Eizelle: dicht mit Organellen gefüllt, deshalb stark streuend statt glasig."""
    material = load_or_create_material("M_GEN_Oocyte_Ooplasm")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)

    position = local_position(material, -1600, 0)
    freq_granules = constant(material, -1600, 400, 0.45)   # Granula ~2 µm
    freq_fine = constant(material, -1600, 500, 1.6)        # feinste Körnung unter 1 µm

    granules = custom(material, -1200, 0, "Granula", CELLULAR_CODE, ["P", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(position, "", granules, ["P"])
    connect(freq_granules, "", granules, ["Freq"])

    speckle = custom(material, -1200, 300, "Feinkorn", FBM_CODE, ["P", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(position, "", speckle, ["P"])
    connect(freq_fine, "", speckle, ["Freq"])

    # Körnung: dunkle Granula auf hellerem Grundplasma
    fine = multiply(material, -1000, 100, granules, speckle)

    # Große Schlieren (~20 µm): Ohne sie ist der Zellleib aus einigen hundert µm Abstand eine weiße Kugel –
    # die feine Körnung liegt dort unter einem Bildpunkt.
    coarse = custom(material, -1200, 600, "Plasmaschlieren", FBM_CODE, ["P", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(position, "", coarse, ["P"])
    connect(constant(material, -1500, 660, 0.055), "", coarse, ["Freq"])
    grain = add(material, -800, 300, multiply(material, -950, 260, fine, constant(material, -1150, 330, 0.45)),
                multiply(material, -950, 620, coarse, constant(material, -1150, 690, 0.55)))

    dark = color(material, -700, -200, 0.045, 0.034, 0.028)
    light = color(material, -700, -60, 0.16, 0.125, 0.10)
    base = lerp3(material, -450, -120, dark, light, grain)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Streuung: warmes, fleischfarbenes Durchleuchten – der Zellleib ist keine klare Flüssigkeit
    # Durchleuchtung fast farblos: Rotes Durchscheinen gibt es nur in dickem, durchblutetem Gewebe. Durch eine
    # Zelle von 110 µm geht Licht nahezu ungefärbt – das frühere Rot (0,45/0,21/0,14) machte Eizelle und Polkörper rosa.
    subsurface = color(material, -450, 200, 0.20, 0.175, 0.145)
    mel.connect_material_property(subsurface, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    opacity = scalar(material, -450, 340, "ScatterAmount", 0.16)
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    rough_base = constant(material, -900, 460, 0.28)
    rough_var = multiply(material, -700, 500, grain, constant(material, -900, 560, 0.12))
    roughness = add(material, -450, 480, rough_base, rough_var)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Zytoplasma n ≈ 1,38 gegen Eileiterflüssigkeit n ≈ 1,335: die Zelle glänzt praktisch nicht
    spec = constant(material, -450, 620, 0.03)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    # Cortikalreaktion: Die Granula wandern an die Membran und werden ausgeschüttet – das Plasma hellt kurz auf
    cortical = scalar(material, -450, 720, "CorticalReaction", 0.0)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_Oocyte_Ooplasm gebaut (Parameter CorticalReaction %s)" % cortical.get_editor_property("parameter_name"))
    return material


def create_zona_material():
    """Zona pellucida: glasige Glykoprotein-Hülle mit radialer Faserstruktur, die nach der Befruchtung verhärtet."""
    material = load_or_create_material("M_GEN_Oocyte_Zona")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    # Vor der Schaerfentiefe zeichnen. Unreal legt durchscheinende Flaechen sonst in einen Pass
    # NACH der Schaerfentiefe - dann bleibt eine Zelle direkt vor der Linse gestochen scharf,
    # waehrend alles andere weich ist. Genau daran erkennt man ein Bild als gerechnet.
    material.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    material.set_editor_property("two_sided", True)
    # Brechung nach Brechungsindex (Name der Eigenschaft je nach Engine-Version)
    for property_name in ("refraction_method", "refraction_mode"):
        try:
            material.set_editor_property(property_name, unreal.RefractionMode.RM_INDEX_OF_REFRACTION)
            break
        except Exception as error:
            log("Brechungsmodus über '%s' nicht setzbar: %s" % (property_name, error))

    position = local_position(material, -1900, 0)
    freq = constant(material, -1900, 400, 26.0)
    fibres = custom(material, -1500, 0, "Zonafasern", ZONA_FIBRE_CODE, ["P", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(position, "", fibres, ["P"])
    connect(freq, "", fibres, ["Freq"])

    cortical = scalar(material, -1500, 300, "CorticalReaction", 0.0)

    # Farbe: fast farblos, nach der Cortikalreaktion leicht bernsteinfarben und dichter
    clear = color(material, -1200, -200, 0.11, 0.12, 0.13)
    hardened = color(material, -1200, -60, 0.20, 0.16, 0.11)
    base = lerp3(material, -950, -120, clear, hardened, cortical)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Durchsichtigkeit: dünne, klare Hülle; die Fasern zeichnen sie, die Verhärtung macht sie milchig
    thin = constant(material, -1200, 200, 0.03)
    dense = constant(material, -1200, 280, 0.075)
    opacity_level = lerp3(material, -950, 220, thin, dense, cortical)
    fibre_amount = multiply(material, -1200, 380, fibres, constant(material, -1400, 430, 0.12))
    rim = fresnel(material, -1200, 520, 2.2, 0.04)
    rim_amount = multiply(material, -950, 540, rim, constant(material, -1150, 600, 0.35))
    opacity = add(material, -700, 300, add(material, -850, 300, opacity_level, fibre_amount), rim_amount)
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    smooth = constant(material, -1200, 700, 0.10)
    rough = constant(material, -1200, 780, 0.28)
    roughness = lerp3(material, -950, 720, smooth, rough, cortical)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    spec = constant(material, -700, 640, 0.02)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    # Relativer Brechungsindex gegen die Eileiterflüssigkeit (1,38 / 1,335): kaum Versatz, aber sichtbarer Rand
    # Brechung im Bildraum liest das Bild dahinter versetzt aus. Am Bildrand gibt es kein „dahinter" –
    # die Engine wiederholt die letzte Zeile, und über der Zona standen helle Balken am oberen und unteren
    # Rand (GENESIS-038, gegengeprüft mit r.RefractionQuality 0). Zum Rand hin läuft die Brechung deshalb aus.
    screen = expression(material, unreal.MaterialExpressionScreenPosition, -1000, 900)
    ior = custom(material, -700, 860, "Brechung ohne Randfehler", """
float2 uv = ViewportUV;
float edge = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
return lerp(1.0, 1.04, smoothstep(0.0, 0.12, edge));
""", ["ViewportUV"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(screen, "ViewportUV", ior, ["ViewportUV"])
    mel.connect_material_property(ior, "", unreal.MaterialProperty.MP_REFRACTION)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_Oocyte_Zona gebaut")
    return material


def create_corona_material():
    """Corona radiata: Kranz lebender Zellen, die die Eizelle versorgen – innen streuend, an den Rändern durchscheinend."""
    material = load_or_create_material("M_GEN_Oocyte_Corona")
    # Zweiseitiges Laub-Modell: Eine Cumuluszelle ist nur ~12 µm dick, Licht geht hindurch und tritt hinten wieder aus.
    # Mit dem Streu-Modell (undurchsichtig) bleiben harte Silhouetten – die Zellen sahen aus wie Reiskörner.
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    material.set_editor_property("two_sided", True)

    position = local_position(material, -1600, 0)
    freq = constant(material, -1600, 400, 0.16)
    grain = custom(material, -1200, 0, "Zellkorn", FBM_CODE, ["P", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(position, "", grain, ["P"])
    connect(freq, "", grain, ["Freq"])

    # Jede Coronazelle trägt ihre eigene Tönung (Vertex-Farbe "Cell": R = Zufallswert je Zelle aus dem Blender-Aufbau)
    vertex = expression(material, unreal.MaterialExpressionVertexColor, -1200, 300)
    cell_tint = multiply(material, -950, 240, vertex, grain, "R")
    # Der Zufallswert bestimmt den Ton, das Korn moduliert ihn nur leicht
    variation = add(material, -800, 280, multiply(material, -950, 380, vertex, constant(material, -1150, 430, 0.75), "R"),
                    multiply(material, -950, 470, cell_tint, constant(material, -1150, 520, 0.25)))

    # Berührungstiefe aus dem Blender-Aufbau (Vertexfarbe B): 1 = freie Oberfläche, kleiner = an den
    # Nachbarn gedrückt. Dort, wo zwei Zellen aneinander liegen, kommt kein Licht hin – ohne diese
    # dunklen Fugen verschmelzen die Zellen zu einer hellen Masse, und der Komplex sieht aus wie Popcorn.
    # Die Schwellen müssen der gemessenen Verteilung folgen, sonst behandelt das Material halbe
    # Zellen als Fuge oder gar keine. Gemessen im Aufbau nach GENESIS-033: Median 1,0, unteres
    # Zehntel 0,956 – seit die Zellen einander umschließen statt sich an Ebenen zu schneiden, ist
    # nur noch ein schmaler Saum wirklich gedrückt. Die Schwellen liegen deshalb eng beieinander;
    # mit den alten (0,70/0,95) wäre die ganze Zellwolke eine einzige Fuge gewesen.
    crease = smoothstep(material, -950, 620, vertex, "B", 0.90, 1.0)

    # Deutlich dunkler als Papier: Eine Zellwolke unter dem Endoskoplicht ist keine weiße Wand.
    # Mit hellem Grundton frisst das nahe Licht jede Zeichnung weg (gemessen: Median 0,87 bei 0,04 Tonumfang).
    # Kaum Farbe: Zytoplasma ist farblos, die Wärme kommt vom Licht. Das frühere Rosa war erfunden und machte
    # die Zellen aus der Nähe (Ich-Perspektive, GENESIS-038) zu glatten Plastikeiern.
    dark = color(material, -900, -200, 0.050, 0.041, 0.035)
    # Heller als zuvor: Die zweite Fassung wirkte wie Kieselsteine – lebende Zellen sind durchscheinend und licht
    light = color(material, -900, -60, 0.28, 0.25, 0.215)
    base_tone = lerp3(material, -650, -120, dark, light, variation)
    # Die Fuge behält ein Viertel ihrer Helligkeit: Sie ist dunkel, aber nicht schwarz
    creased = multiply(material, -450, -120, base_tone, add(material, -600, -40, constant(material, -750, -20, 0.25),
                                                            multiply(material, -750, 60, crease, constant(material, -900, 80, 0.75))))

    # Zellkern und Randsaum. Eine Cumuluszelle (~12 µm) ist zum größten Teil Kern (~7–8 µm): durch das klare
    # Zytoplasma sieht man ihn als glatte, etwas dichtere Scheibe mit feinem Saum. Seine Lage im Bild ergibt sich
    # aus der Flächennormale (wie bei den Furchungszellen, GENESIS-038), je Zelle leicht versetzt (Vertexfarbe R).
    # Der helle Saum an der Silhouette ist Brechung am Übergang Zelle/Flüssigkeit – an ihm erkennt man im
    # Mikroskop überhaupt erst eine Zelle.
    normal = expression(material, unreal.MaterialExpressionVertexNormalWS, -1200, 760)
    camera = expression(material, unreal.MaterialExpressionCameraVectorWS, -1200, 840)
    cell = custom(material, -900, 800, "Kern und Saum", """
float3 n = normalize(N);
float3 v = normalize(V);
float facing = saturate(dot(n, v));
float3 np = n - v * dot(n, v);
float3 s = normalize(cross(v, float3(0.0, 0.0, 1.0)) + 1e-4);
float3 u = normalize(cross(v, s));
float2 q = float2(dot(np, s), dot(np, u));
float2 offset = (float2(frac(Cell * 7.31), frac(Cell * 3.17)) - 0.5) * 0.22;
float d = length(q - offset) / lerp(0.42, 0.55, frac(Cell * 5.7));
float nucleus = 1.0 - smoothstep(0.82, 1.0, d);
float envelope = smoothstep(0.78, 0.95, d) * (1.0 - smoothstep(0.98, 1.1, d));
float rim = pow(1.0 - facing, 3.0);
// x: Faktor auf den Grundton, y: aufgehellter Saum
// Kern nur angedeutet: Er ist fast so klar wie das Zytoplasma (erster Versuch mit 0,86/0,35: Spiegeleier)
// Weiches Rauschen statt Würfel-Zufall: Die erste Fassung zeigte sichtbare Voxel
float granules = Fine;
float grain = lerp(0.84, 1.08, granules) * (1.0 - nucleus * 0.1);
return float2(lerp(1.0, 0.95, nucleus) * (1.0 - 0.12 * envelope) * grain, rim);
""", ["N", "V", "Cell", "Fine"], unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    connect(normal, "", cell, ["N"])
    connect(camera, "", cell, ["V"])
    connect(vertex, "R", cell, ["Cell"])
    fine = custom(material, -1200, 900, "Granula", FBM_CODE, ["P", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(position, "", fine, ["P"])
    connect(constant(material, -1400, 960, 1.1), "", fine, ["Freq"])
    connect(fine, "", cell, ["Fine"])
    # Ein Custom-Knoten hat nur einen Ausgang – die Anteile holt eine Komponentenmaske heraus
    shade_factor = expression(material, unreal.MaterialExpressionComponentMask, -600, 800, r=True, g=False, b=False, a=False)
    connect(cell, "", shade_factor, ["", "Input"])
    rim_amount = expression(material, unreal.MaterialExpressionComponentMask, -600, 880, r=False, g=True, b=False, a=False)
    connect(cell, "", rim_amount, ["", "Input"])
    shaded = multiply(material, -300, -120, creased, shade_factor)
    halo = multiply(material, -300, 0, rim_amount, color(material, -450, 40, 0.10, 0.092, 0.082))
    base = add(material, -150, -80, shaded, halo)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Durchleuchtung: warmes Rot, wie Licht durch eine dünne Gewebeschicht.
    # An den Berührungsflächen ist der Weg durch das Gewebe am längsten – dort dringt am wenigsten durch.
    # Fast farblos aus demselben Grund wie beim Zellleib der Eizelle: kein Blut, nur 12 µm Gewebe
    transmission_dark = color(material, -900, 140, 0.10, 0.085, 0.07)
    transmission_light = color(material, -900, 220, 0.22, 0.19, 0.16)
    transmission_tone = lerp3(material, -650, 180, transmission_dark, transmission_light, variation)
    transmission = multiply(material, -450, 180, transmission_tone,
                            add(material, -600, 260, constant(material, -750, 240, 0.12),
                                multiply(material, -750, 320, crease, constant(material, -900, 340, 0.88))))
    mel.connect_material_property(transmission, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    # Mikrovilli: Die Oberfläche einer lebenden Zelle ist auf Bruchteilen eines Mikrometers zottelig.
    # Zu sehen ist davon keine Struktur, sondern ein Streuen – also Rauheit, die mit dem Korn schwankt.
    roughness = add(material, -650, 420, constant(material, -820, 400, 0.34),
                    multiply(material, -820, 470, grain, constant(material, -980, 490, 0.22)))
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = constant(material, -650, 500, 0.03)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_Oocyte_Corona gebaut")
    return material


def create_strands_material():
    """
    Die Fäden der Hyaluronsäure-Matrix: fast klares Gel, das an den Zellen hängt.

    Ein Faden aus Wasser mit Zuckerketten hat einen Brechungsindex von etwa 1,34 – kaum anders als
    die Eileiterflüssigkeit. Er glänzt nicht und wirft keinen Schatten; sichtbar wird er nur,
    weil er Licht streut und an den Rändern heller wird. Deshalb: durchscheinend, sehr geringe
    Deckkraft, und mit dem Blickwinkel steigend (Fresnel).
    """
    material = load_or_create_material("M_GEN_Oocyte_Strands")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    # Vor der Schaerfentiefe zeichnen. Unreal legt durchscheinende Flaechen sonst in einen Pass
    # NACH der Schaerfentiefe - dann bleibt eine Zelle direkt vor der Linse gestochen scharf,
    # waehrend alles andere weich ist. Genau daran erkennt man ein Bild als gerechnet.
    material.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    material.set_editor_property("two_sided", True)

    base = color(material, -700, -100, 0.52, 0.50, 0.47)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Quer getroffen ist ein Faden dicker als längs – das macht der Fresnel-Term.
    # Die Deckkraft bleibt insgesamt winzig: Gel aus 98 % Wasser verdeckt nichts, es schimmert nur.
    rim = fresnel(material, -1050, 120, 2.2, 0.06)
    rim_amount = multiply(material, -800, 140, rim, constant(material, -1000, 220, 0.16))
    opacity = add(material, -560, 140, rim_amount, constant(material, -800, 260, 0.02))
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    roughness = constant(material, -560, 340, 0.12)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = constant(material, -560, 420, 0.02)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Material M_GEN_Oocyte_Strands gebaut")
    return material


# ---------------------------------------------------------------------------------------------------------------------
# Szene
# ---------------------------------------------------------------------------------------------------------------------
def place_oocyte(meshes, materials):
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    existing = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.GenesisOocyte)]
    for actor in existing:
        actors.destroy_actor(actor)

    oocyte = actors.spawn_actor_from_class(unreal.GenesisOocyte, OOCYTE_LOCATION)
    oocyte.set_actor_label("Oocyte")

    # Der Kranz muss Schatten werfen: Erst die Eigenverschattung der Zellen macht aus der Kugel eine Wolke.
    parts = (
        ("ooplasm", "cytoplasm", "ooplasm", True, True),
        ("zona", "zona", "zona", False, True),
        ("polar_body", "polar_body", "ooplasm", False, True),
        ("corona", "corona", "corona", True, True),
        # Die Fäden der Gallerte zwischen den Zellen. Sie werfen keine Schatten – ein 0,5 µm dünner
        # Faden aus Wasser wirft keinen, und die Schattenkarte würde ihn ohnehin nicht auflösen.
        ("cumulus_strands", "strands", "strands", False, True),
        # Die Gallerte als Ganzes bleibt ungesetzt: Als Kugel mit harter Silhouette wirkte sie wie eine
        # Plastikschale. Sichtbar wird der Cumulus über die Zellen und die Fäden dazwischen.
    )
    for property_name, mesh_key, material_key, cast_shadow, ray_tracing in parts:
        component = oocyte.get_editor_property(property_name)
        mesh = meshes.get(mesh_key)
        if not component or not mesh:
            unreal.log_error("GENESIS: Bauteil %s fehlt" % property_name)
            continue
        component.set_static_mesh(mesh)
        component.set_material(0, materials[material_key])
        component.set_editor_property("cast_shadow", cast_shadow)
        # Feinste Membranen: Distanzfelder kosten hier nur Speicher; die klare Gallerte auch im Raytracing nichts
        component.set_editor_property("affect_distance_field_lighting", False)
        component.set_editor_property("visible_in_ray_tracing", ray_tracing)

    # Cumulus-Gallerte: Als lokales Nebelvolumen (LocalFogVolume) getestet und wieder entfernt – zwischen
    # Extinktion 3,5 und 45 war im gemessenen Bild kein Unterschied (Median 0,310 gegen 0,309).
    # Ein Objekt, das nichts tut, bleibt nicht in der Szene. Der Cumulus entsteht aus den Zellen selbst.
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.LocalFogVolume):
            actors.destroy_actor(actor)

    # Streulicht der Eileiterflüssigkeit dämpfen: Direkt vor der Optik entsteht im Lichtkegel sonst ein weißer
    # Schleier, der jeden Materialunterschied überdeckt (nachgewiesen mit einer grün eingefärbten Probe,
    # die trotzdem weißlich ankam). Physikalisch ist die Flüssigkeit fast klar.
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.ExponentialHeightFog):
            fog = actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
            fog.set_editor_property("fog_density", 0.006)
            fog.set_editor_property("volumetric_fog_scattering_distribution", 0.4)
        if isinstance(actor, unreal.GenesisMicroscopeCameraRig):
            light = actor.get_editor_property("endoscope_light")
            light.set_editor_property("volumetric_scattering_intensity", 0.35)
            light.set_editor_property("indirect_lighting_intensity", 1.0)
            light.set_editor_property("intensity", 150.0)
            log("Endoskoplicht auf 150 cd, Streulicht 0,35 gesetzt")

    # Der Schwarm konkurriert ab jetzt um diese Eizelle
    swarms = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.GenesisSpermSwarm)]
    for swarm in swarms:
        swarm.set_editor_property("oocyte", oocyte)
        log("Schwarm %s auf die Eizelle ausgerichtet" % swarm.get_actor_label())
    if not swarms:
        unreal.log_error("GENESIS: Kein Spermienschwarm im Level gefunden")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    log("Eizelle bei %s gesetzt, Level gespeichert: %s" % (OOCYTE_LOCATION, saved))


ensure_folders()
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)

if os.environ.get("GENESIS_SKIP_OOCYTE_IMPORT"):
    mesh_assets = {
        "cytoplasm": eal.load_asset(OOCYTE + "/SM_GEN_OocyteCytoplasm"),
        "zona": eal.load_asset(OOCYTE + "/SM_GEN_OocyteZona"),
        "polar_body": eal.load_asset(OOCYTE + "/SM_GEN_OocytePolarBody"),
        "corona": eal.load_asset(OOCYTE + "/SM_GEN_OocyteCorona"),
        "strands": eal.load_asset(OOCYTE + "/SM_GEN_OocyteStrands"),
    }
else:
    mesh_assets = {
        "cytoplasm": import_mesh("SM_GEN_OocyteCytoplasm.fbx", "SM_GEN_OocyteCytoplasm", nanite=True),
        "zona": import_mesh("SM_GEN_OocyteZona.fbx", "SM_GEN_OocyteZona", nanite=False),
        "polar_body": import_mesh("SM_GEN_OocytePolarBody.fbx", "SM_GEN_OocytePolarBody", nanite=False),
        "corona": import_mesh("SM_GEN_OocyteCorona.fbx", "SM_GEN_OocyteCorona", nanite=True, compute_normals=True),
        # Fäden: Nanite aus, weil sie durchscheinend gerendert werden
        "strands": import_mesh("SM_GEN_OocyteStrands.fbx", "SM_GEN_OocyteStrands", nanite=False, compute_normals=True),
    }

material_assets = {
    "ooplasm": create_ooplasm_material(),
    "zona": create_zona_material(),
    "corona": create_corona_material(),
    "strands": create_strands_material(),
}

place_oocyte(mesh_assets, material_assets)

# Der Keim hängt an der Eizelle: Wer sie neu setzt, muss den Keim mitsetzen. Ohne diesen Aufruf ging der
# Keim beim Neuaufbau verloren, und die erste Woche lief unsichtbar hinter einer unbefruchteten Eizelle
# (gefunden in GENESIS-038 – sichtbar bei 35 und 97 Stunden nach der Befruchtung).
import runpy
runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), "setup_embryo.py"), run_name="__main__")