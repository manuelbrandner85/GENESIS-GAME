# GENESIS – die Gebärmutter für die zweite Woche (GENESIS-040): Schleimhaut, Keim, Einnistungsstelle, Hysteroskop.
#
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Implantation/setup_uterus_scene.py" -unattended
#
# Voraussetzung: Tools/Blender/Implantation/build_endometrium.py -- --out ArtSource/Generated/Implantation --export
# Umgebungsvariablen: GENESIS_SKIP_IMPORT=1 (Mesh nicht neu importieren), GENESIS_EXPOSURE_BIAS, GENESIS_LUX, GENESIS_FOG.
#
# Maßstab der Mikrowelt: 1 µm = 1 Unreal-Einheit.

import os
import unreal

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SOURCE = os.path.join(REPO, "ArtSource", "Generated", "Implantation")
ROOT = "/Game/Genesis/Implantation"
ENVIRONMENT = ROOT + "/Environment"
MATERIALS = ROOT + "/Materials"
MAP_PATH = ROOT + "/Maps/L_GEN_UterineCavity"
CELL_MESH = "/Game/Genesis/Conception/Cells/SM_GEN_OocyteCytoplasm"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
FLOAT1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
FLOAT3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3


def log(message):
    unreal.log_warning("GENESIS: " + message)


def ensure_folders():
    for folder in (ROOT, ENVIRONMENT, MATERIALS, ROOT + "/Maps"):
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


def connect(src, src_output, dst, dst_inputs):
    for name in dst_inputs:
        if mel.connect_material_expressions(src, src_output, dst, name):
            return True
    unreal.log_error("GENESIS: Verbindung fehlgeschlagen %s.%s -> %s %s" % (src.get_name(), src_output, dst.get_name(), dst_inputs))
    return False


def custom(material, x, y, description, code, inputs, output_type, extra_outputs=()):
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
    outputs = []
    for name, kind in extra_outputs:
        output = unreal.CustomOutput()
        output.set_editor_property("output_name", name)
        output.set_editor_property("output_type", kind)
        outputs.append(output)
    if outputs:
        node.set_editor_property("additional_outputs", outputs)
    return node


# ---------------------------------------------------------------------------------------------------------------------
# Parameter der Einnistungsstelle (schreibt AGenesisImplantationSite je Bild)
# ---------------------------------------------------------------------------------------------------------------------

SCALARS = ["EmbryoRadius", "CenterHeight", "Waterline", "Collar", "PlugRadius", "DomeHeight", "Blood", "Hyperemia"]


def create_parameter_collection():
    path = MATERIALS + "/MPC_GEN_Implantation"
    if eal.does_asset_exist(path):
        mpc = eal.load_asset(path)
    else:
        mpc = asset_tools.create_asset("MPC_GEN_Implantation", MATERIALS, unreal.MaterialParameterCollection,
                                       unreal.MaterialParameterCollectionFactoryNew())
    scalars = []
    for name in SCALARS:
        parameter = unreal.CollectionScalarParameter()
        parameter.set_editor_property("parameter_name", name)
        parameter.set_editor_property("default_value", 100.0 if name == "EmbryoRadius" else 0.0)
        scalars.append(parameter)
    mpc.set_editor_property("scalar_parameters", scalars)
    site = unreal.CollectionVectorParameter()
    site.set_editor_property("parameter_name", "Site")
    site.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
    mpc.set_editor_property("vector_parameters", [site])
    eal.save_loaded_asset(mpc)
    return mpc


# ---------------------------------------------------------------------------------------------------------------------
# HLSL: gemeinsame Helfer (Struktur-Trick: Funktionen als Methoden einer lokalen Struktur)
# ---------------------------------------------------------------------------------------------------------------------

HELPERS = """
struct GenesisEndometrium
{
	float H1(float2 p) { return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
	float2 H2(float2 p) { return frac(sin(float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)))) * 43758.5453); }
	float N(float2 p)
	{
		float2 i = floor(p);
		float2 f = frac(p);
		f = f * f * (3.0 - 2.0 * f);
		return lerp(lerp(H1(i), H1(i + float2(1, 0)), f.x), lerp(H1(i + float2(0, 1)), H1(i + float2(1, 1)), f.x), f.y);
	}
	float Fbm(float2 p)
	{
		float v = 0.0;
		float a = 0.5;
		for (int o = 0; o < 4; ++o)
		{
			v += a * N(p);
			p = float2(0.8 * p.x - 0.6 * p.y, 0.6 * p.x + 0.8 * p.y) * 2.03;
			a *= 0.5;
		}
		return v / 0.9375;
	}
	// x = Abstand zum nächsten Zellmittelpunkt, y = zum zweitnächsten, z = Kennwert der Zelle (0..1)
	float3 Worley(float2 p)
	{
		float2 ip = floor(p);
		float2 fp = frac(p);
		float f1 = 8.0;
		float f2 = 8.0;
		float id = 0.0;
		for (int j = -1; j <= 1; ++j)
		{
			for (int i = -1; i <= 1; ++i)
			{
				float2 o = float2(i, j);
				float2 r = o + H2(ip + o) * 0.85 + 0.075 - fp;
				float d = dot(r, r);
				if (d < f1) { f2 = f1; f1 = d; id = H1(ip + o + 17.0); }
				else if (d < f2) { f2 = d; }
			}
		}
		return float3(sqrt(f1), sqrt(f2), id);
	}
	// Epithel im Implantationsfenster: Säulenzellen 6–9 µm, gewölbte Kuppen, Zellgrenzen; Pinopoden (Uterodome) als
	// glatte, ballonartige Vorwölbungen in Feldern; vereinzelt Flimmerzellen. Höhe in µm.
	float4 Epithelium(float2 q)
	{
		float2 warp = float2(N(q / 23.0), N(q / 23.0 + 41.3)) - 0.5;
		float3 c = Worley((q + warp * 5.0) / 7.2);
		float field = Fbm(q / 160.0);
		// Pinopoden: Die ganze Zellkuppe wölbt sich glatt vor (nicht als Pickel), gehäuft in Feldern
		float pino = c.z < lerp(0.0, 0.45, smoothstep(0.45, 0.75, field)) ? 1.0 : 0.0;
		float dome = 1.0 - smoothstep(0.0, 0.6, c.x);
		float border = 1.0 - smoothstep(0.0, 0.07, c.y - c.x);
		float h = 0.8 * dome * dome - 0.3 * border;
		h += pino * 0.5 * pow(1.0 - smoothstep(0.0, 0.62, c.x), 1.5);
		return float4(h, pino, border, c.z);
	}
	// Form der Einnistungsstelle (µm nach oben), wie im World Position Offset
	// Quadrat ohne pow(): pow() mit negativer Basis ist in HLSL undefiniert (NaN) – das gab an den Drüsen weiße Säulen
	float Sq(float x) { return x * x; }
	float Site(float2 q, float pz, float4 S, float R, float W, float C, float P, float D)
	{
		float d = length(q - S.xy);
		float near = exp(-Sq((pz - S.z) / 400.0));
		float z = 0.0;
		if (W > 1.0) { z += 6.0 * C * exp(-Sq((d - W - 5.0) / 11.0)); }
		if (P > 1.0) { z -= 7.0 * (1.0 - smoothstep(0.55, 1.0, d / P)); }
		z += D * exp(-1.6 * Sq(d / max(R, 1.0)));
		z *= near;
		return isfinite(z) ? clamp(z, -20.0, 80.0) : 0.0;
	}
};
GenesisEndometrium e;
"""

WPO_CODE = HELPERS + """
return float3(0.0, 0.0, e.Site(P.xy, P.z, Site, R, Waterline, Collar, PlugR, Dome));
"""

NORMAL_CODE = HELPERS + """
float2 q = P.xy;
float3 n = normalize(N);
// Pixel-Fußabdruck: Wo eine Zelle nur noch wenige Pixel groß ist, blendet das Zellrelief aus (kein Flimmern)
float footprint = length(fwidth(q));
float micro = 1.0 - smoothstep(1.0, 3.2, footprint);
// Das Zellmuster liegt in der Draufsicht (XY): Auf steilen Trichterwänden würde es zu senkrechten Streifen gezogen
micro *= smoothstep(0.55, 0.85, n.z);
const float s = 0.35;
float h0 = e.Epithelium(q).x;
float2 g = float2(e.Epithelium(q + float2(s, 0.0)).x - h0, e.Epithelium(q + float2(0.0, s)).x - h0) / s * micro;
// Wölbung, Wulst und Pfropf der Einnistungsstelle: WPO verschiebt nur die Punkte, die Normale kommt von hier
const float t = 3.0;
float m0 = e.Site(q, P.z, Site, R, Waterline, Collar, PlugR, Dome);
g += float2(e.Site(q + float2(t, 0.0), P.z, Site, R, Waterline, Collar, PlugR, Dome) - m0,
            e.Site(q + float2(0.0, t), P.z, Site, R, Waterline, Collar, PlugR, Dome) - m0) / t;
// Begrenzt: Einzelne steile Stellen kippten die Normale weg vom Licht – schwarze Pünktchen im Bild
g *= Strength;
float gl = length(g);
g *= gl > 0.9 ? 0.9 / gl : 1.0;
float3 grad = float3(g, 0.0);
grad -= n * dot(grad, n);
return normalize(n - grad);
"""

SURFACE_CODE = HELPERS + """
float2 q = P.xy;
float footprint = length(fwidth(q));
float micro = 1.0 - smoothstep(1.0, 3.2, footprint);
micro *= smoothstep(0.55, 0.85, normalize(Nrm).z);
float4 epi = e.Epithelium(q);
float pino = epi.y;
float border = epi.z * micro;
float cellId = epi.w;
float ciliated = cellId > 0.975 ? micro : 0.0;

// Einnistungsstelle: nur auf dieser Wand, nicht auf der gegenüberliegenden
float d = length(q - Site.xy);
float near = exp(-e.Sq((P.z - Site.z) / 400.0));
float hyper = Hyper * exp(-e.Sq(d / (2.4 * R + 150.0))) * near;

// Kapillarnetz 10–20 µm unter dem Epithel: gewundene, verzweigte Röhrchen – Höhenlinien eines verwundenen Rauschens,
// keine Vieleckkanten (die wirkten wie gesprungene Farbe). Unscharf und blass, weil Epithel und Stroma darüber streuen.
float2 w1 = q + (float2(e.Fbm(q / 140.0), e.Fbm(q / 140.0 + 5.2)) - 0.5) * 90.0;
float r1 = abs(e.Fbm(w1 / 75.0) - 0.5);
float capLine = 1.0 - smoothstep(0.0, 0.034 + 0.02 * hyper, r1);
capLine *= 0.55 + 0.45 * e.Fbm(q / 60.0 + 17.0);   // mal tiefer, mal flacher unter dem Epithel
float capBreak = smoothstep(0.25, 0.60, e.Fbm(q / 170.0 + 3.3));
float r2 = abs(e.Fbm(w1 / 300.0 + 13.0) - 0.5);
float venLine = 1.0 - smoothstep(0.0, 0.022, r2);
float venBreak = smoothstep(0.40, 0.70, e.Fbm(q / 520.0 + 5.0));
float vessels = saturate(capLine * (0.30 + 0.70 * capBreak) * (1.0 + 1.3 * hyper) * 0.7 + venLine * venBreak * 0.45 * (1.0 + 0.6 * hyper));

// Grundton: blass rosa bis gelblich (Ödem, Glykogen im Sekretionsendometrium), in Furchen und zwischen den Polstern
// kräftiger; Deziduareaktion stärker durchblutet
float macro = e.Fbm(q / 520.0 + 2.0);
float patch = e.Fbm(q / 190.0 + 8.0);
float3 base = lerp(float3(0.56, 0.36, 0.31), float3(0.50, 0.25, 0.23), smoothstep(0.30, 0.75, macro));
base = lerp(base, base * float3(1.06, 1.02, 0.94), smoothstep(0.55, 0.8, patch) * 0.6);
base *= 0.90 + 0.14 * VC.b;
base = lerp(base, base * float3(0.92, 0.78, 0.78), hyper);
base = lerp(base, float3(0.40, 0.10, 0.09), vessels * 0.30);

// Zellen: Kuppen minimal heller, Grenzen dunkler, jede Zelle etwas anders; Flimmerzellen heller und stumpf
// Die Zellgrenzen sind unter dem Schleimfilm nur ein Hauch – kräftiger wirkte das Epithel wie getrockneter Schlamm
base *= 1.0 + micro * (0.04 * (1.0 - epi.z) - 0.07 * epi.z + 0.04 * (cellId - 0.5));
base = lerp(base, base * float3(1.03, 1.03, 1.02), ciliated * 0.5);

// Drüsenöffnungen: im Trichter dunkler und röter; über manchen glänzt Sekret
float pit = VC.r;
// Der Trichter führt in einen langen Drüsenschlauch: Aus dem Inneren kommt kaum Licht zurück
base = lerp(base, float3(0.035, 0.008, 0.008), pow(saturate(pit), 0.4));
// Kennwert der Drüse liegt im Alpha als 0,5..1: Bei Alpha 0 überschreibt der PNG-Import die Farbe mit Nachbarwerten
float glandTag = saturate((VA - 0.5) * 2.0);
float secretion = step(0.62, glandTag) * smoothstep(0.05, 0.4, pit);
base = lerp(base, float3(0.74, 0.66, 0.58), secretion * 0.35);
base *= 1.0 - 0.10 * VC.g;

// Saum, wo das Epithel den einsinkenden Keim umschließt: gedehnt, leicht gerötet
float ring = Waterline > 1.0 ? Collar * exp(-e.Sq((d - Waterline) / 10.0)) * near : 0.0;
base = lerp(base, float3(0.58, 0.24, 0.21), ring * 0.45);
// Mütterliches Blut in den Lakunen unter dünnem Gewebe: dunkelrot bis violett, fleckig
float blood = saturate(Blood * exp(-e.Sq(d / (0.8 * R))) * near * (0.7 + 0.3 * e.Fbm(q / 40.0 + 9.0)));
base = lerp(base, float3(0.22, 0.025, 0.035), blood * 0.75);
// Im Wasserlinienkreis liegt nicht die Schleimhaut, sondern die untere Wand des Keims (Trophoblast, dichter Zellbelag):
// Durch die glasige Hülle sah man sonst die flache Schleimhaut mitten im Keim
float inside = (Waterline > 1.0 && Collar > 0.01) ? (1.0 - smoothstep(0.85, 0.98, d / Waterline)) * near : 0.0;
base = lerp(base, float3(0.26, 0.23, 0.19) * (0.85 + 0.3 * e.Fbm(q / 12.0)), inside);
// Die zweiblättrige Keimscheibe liegt als dichtere, etwas dunklere Scheibe auf dem Boden des Keims
float disc = inside * (1.0 - smoothstep(0.3, 0.45, d / max(Waterline, 1.0)));
base = lerp(base, float3(0.20, 0.17, 0.14) * (0.9 + 0.2 * e.Fbm(q / 5.0 + 31.0)), disc * 0.85);
// Fibrinpfropf: gelblich-graues Gerinnsel aus Fasern, darin rote Blutkörperchen; unregelmäßiger, durchscheinender Rand
float edgeNoise = 0.82 + 0.3 * e.Fbm((q - Site.xy) / 35.0 + 21.0);
float plug = PlugR > 1.0 ? (1.0 - smoothstep(0.6, 1.0, d / (PlugR * edgeNoise))) * near : 0.0;
float2 fq = q / 3.0;
float fibers = abs(e.Fbm(fq + float2(e.Fbm(q / 20.0), e.Fbm(q / 20.0 + 3.0)) * 4.0) - 0.5);
float fibrin = 1.0 - smoothstep(0.0, 0.08, fibers);
float rbc = smoothstep(0.58, 0.8, e.Fbm(q / 7.0 + 4.0));
float3 clot = lerp(float3(0.58, 0.47, 0.34), float3(0.72, 0.64, 0.52), fibrin * 0.6);
clot = lerp(clot, float3(0.36, 0.05, 0.05), rbc * 0.7);
base = lerp(base, clot, plug * (0.75 + 0.25 * smoothstep(0.0, 0.6, 1.0 - d / max(PlugR, 1.0))));

// Feucht: Schleimfilm auf den Mikrovilli; Pinopoden glatt, Flimmerzellen stumpf, Trichter und Sekret nass
float rough = 0.28 + micro * (0.08 * epi.z - 0.04 * pino);
rough = lerp(rough, 0.45, ciliated);
// Trichterwand: feucht, aber das Licht verliert sich im Schlauch – kein Spiegel (die Wände glänzten hell, gesehen)
rough = lerp(rough, 0.45, saturate(pit * 1.5));
rough = lerp(rough, 0.10, secretion);
rough = lerp(rough, 0.42, plug);
rough = lerp(rough, 0.22, blood);
rough = lerp(rough, 0.35, inside * (1.0 - plug));
Roughness = saturate(rough);
// Specular 0 schaltet in Unreal auch den streifenden Glanz ab (Fresnel mal 50·F0) – die Trichterwände leuchteten sonst weiß
Specular = lerp(0.2, 0.0, pow(saturate(pit), 0.5));
Sss = lerp(float3(0.62, 0.13, 0.09), float3(0.45, 0.03, 0.03), saturate(hyper * 0.6 + blood));
// Im Drüsenschlauch kommt auch kein gestreutes Licht zurück – sonst leuchtet der Trichter rosa (gesehen)
Sss *= 1.0 - pow(saturate(pit), 0.4);
Sss = lerp(Sss, float3(0.16, 0.13, 0.10), inside * (1.0 - plug));
return saturate(base);
"""


def collection_parameter(material, mpc, name, x, y):
    node = expression(material, unreal.MaterialExpressionCollectionParameter, x, y)
    node.set_editor_property("collection", mpc)
    node.set_editor_property("parameter_name", name)
    return node


def site_inputs(material, mpc, node, x, y):
    """Verbindet die Parameter der Einnistungsstelle mit einem Custom-Knoten (Pins Site, R, Waterline …)."""
    pins = {"Site": "Site", "R": "EmbryoRadius", "Waterline": "Waterline", "Collar": "Collar", "PlugR": "PlugRadius",
            "Dome": "DomeHeight", "Blood": "Blood", "Hyper": "Hyperemia"}
    inputs = [str(entry.get_editor_property("input_name")) for entry in node.get_editor_property("inputs")]
    for index, (pin, name) in enumerate(pins.items()):
        if pin in inputs:
            connect(collection_parameter(material, mpc, name, x, y + index * 60), "", node, [pin])


def create_endometrium_material(mpc, data_texture):
    material = fresh_material("M_GEN_Endometrium")
    # Gewebe mit Streuung: Licht dringt ins gut durchblutete Stroma und kommt rötlich zurück
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    material.set_editor_property("tangent_space_normal", False)
    # Nanite braucht die größte Verschiebung, sonst wird die Wölbung an Clustergrenzen abgeschnitten
    material.set_editor_property("max_world_position_offset_displacement", 120.0)

    position = expression(material, unreal.MaterialExpressionWorldPosition, -2400, 0)
    position.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
    # Datenbild über die lokale Lage: gilt für beide Wände, auch die gedrehte Vorderwand. Der FBX-Import kehrt Y um
    # (Blender rechtshändig, Unreal linkshändig). Im Bild geprüft: Die Drüsenmaske liegt genau in den Öffnungen.
    local = expression(material, unreal.MaterialExpressionLocalPosition, -2900, 200)
    data_uv = custom(material, -2650, 200, "Lage im Datenbild", "return float2(0.5 + L.x / 8000.0, 0.5 + L.y / 8000.0);", ["L"],
                     unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    connect(local, "", data_uv, ["L"])
    vertex_color = expression(material, unreal.MaterialExpressionTextureSample, -2400, 200)
    vertex_color.set_editor_property("texture", data_texture)
    vertex_color.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    connect(data_uv, "", vertex_color, ["UVs", "Coordinates"])
    normal_ws = expression(material, unreal.MaterialExpressionVertexNormalWS, -2400, 400)

    surface = custom(material, -1200, 0, "Schleimhaut", SURFACE_CODE,
                     ["P", "VC", "VA", "Nrm", "Site", "R", "Waterline", "Collar", "PlugR", "Blood", "Hyper"], FLOAT3,
                     [("Roughness", FLOAT1), ("Sss", FLOAT3), ("Specular", FLOAT1)])
    connect(position, "", surface, ["P"])
    connect(vertex_color, "RGB", surface, ["VC"])
    connect(vertex_color, "A", surface, ["VA"])
    connect(normal_ws, "", surface, ["Nrm"])
    site_inputs(material, mpc, surface, -1700, 0)
    mel.connect_material_property(surface, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(surface, "Roughness", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(surface, "Sss", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    normal = custom(material, -1200, 700, "Epithel und Einnistungsstelle (Normale)", NORMAL_CODE,
                    ["P", "N", "Site", "R", "Waterline", "Collar", "PlugR", "Dome", "Strength"], FLOAT3)
    connect(position, "", normal, ["P"])
    connect(normal_ws, "", normal, ["N"])
    connect(expression(material, unreal.MaterialExpressionScalarParameter, -1700, 1300, parameter_name="ReliefStrength",
                       default_value=1.0), "", normal, ["Strength"])
    site_inputs(material, mpc, normal, -1700, 700)
    mel.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)

    wpo = custom(material, -1200, 1400, "Einnistungsstelle (Form)", WPO_CODE,
                 ["P", "Site", "R", "Waterline", "Collar", "PlugR", "Dome"], FLOAT3)
    connect(position, "", wpo, ["P"])
    site_inputs(material, mpc, wpo, -1700, 1400)
    mel.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    # Schleimfilm über dem Gewebe: Unter dem achsnahen Licht des Hysteroskops glänzen Wölbungen nass (0,2), Drüsenschläuche nicht
    mel.connect_material_property(surface, "Specular", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(expression(material, unreal.MaterialExpressionConstant, -500, 400, r=0.7), "",
                                  unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("M_GEN_Endometrium erstellt")
    return material


SHELL_CODE = """
// Der geschlüpfte Keim von außen (Hysteroskop, kein Mikroskop): eine dünne Hülle (5–10 µm) aus flachen, vieleckigen
// Trophoblastzellen, ~30 µm breit, gefüllt mit klarer Flüssigkeit. Man sieht hindurch – die Schleimhaut darunter, die
// Rückwand der Hülle, den Embryoblasten als trüben Knoten. Wo man schräg durch die Hülle blickt (am Rand), ist der Weg
// durchs Gewebe länger: dort dichter. Zellgrenzen und Kerne dichter als das flache Zytoplasma.
// Zellmuster in 3D (keine Verzerrung an den Polen), im Objektraum: Es dreht sich mit dem Keim.
struct GenesisTrophoblast
{
	float H1(float3 p) { return frac(sin(dot(p, float3(12.9898, 78.233, 37.719))) * 43758.5453); }
	float3 H3(float3 p)
	{
		p = float3(dot(p, float3(127.1, 311.7, 74.7)), dot(p, float3(269.5, 183.3, 246.1)), dot(p, float3(113.5, 271.9, 124.6)));
		return frac(sin(p) * 43758.5453);
	}
	float3 Worley(float3 p)
	{
		float3 ip = floor(p);
		float3 fp = frac(p);
		float f1 = 8.0;
		float f2 = 8.0;
		float id = 0.0;
		for (int k = -1; k <= 1; ++k)
		for (int j = -1; j <= 1; ++j)
		for (int i = -1; i <= 1; ++i)
		{
			float3 o = float3(i, j, k);
			float3 r = o + H3(ip + o) * 0.8 + 0.1 - fp;
			float d = dot(r, r);
			if (d < f1) { f2 = f1; f1 = d; id = H1(ip + o); }
			else if (d < f2) { f2 = d; }
		}
		return float3(sqrt(f1), sqrt(f2), id);
	}
};
// Was unter der Oberfläche der Schleimhaut liegt, steckt im Gewebe: Ohne diesen Schnitt zeichnete die glasige Hülle
// auch ihre versunkene Innenseite – als helle Schale mitten im Gewebe (im Bild gesehen).
clip(WP.z - Site.z + 1.0);
GenesisTrophoblast t;
float3 c = t.Worley(L / CellSize);
float border = 1.0 - smoothstep(0.0, 0.08, c.y - c.x);
float dome = 1.0 - smoothstep(0.0, 0.65, c.x);
float nucleus = 1.0 - smoothstep(0.16, 0.24, c.x);
float3 base = lerp(float3(0.62, 0.59, 0.54), float3(0.55, 0.51, 0.46), c.z) * Tint;
base *= 1.0 - 0.18 * border;
base = lerp(base, base * float3(0.84, 0.81, 0.76), nucleus * 0.6);
Roughness = 0.30 + 0.12 * border;

float cosv = saturate(abs(dot(normalize(Nrm), normalize(V))));
float a0 = Density + 0.22 * border + 0.12 * nucleus;
Opacity = saturate(1.0 - pow(saturate(1.0 - a0), 1.0 / max(cosv, 0.12)));

// Relief (Zellkuppen, eingezogene Grenzen) als Bildraum-Bump
float h = (0.8 * dome * dome - 0.9 * border + 0.25 * nucleus) * Strength;
float3 dpdx = ddx(WP);
float3 dpdy = ddy(WP);
float3 n = normalize(Nrm);
float3 r1 = cross(dpdy, n);
float3 r2 = cross(n, dpdx);
float det = dot(dpdx, r1);
float3 grad = sign(det) * (ddx(h) * r1 + ddy(h) * r2);
NormalWS = normalize(abs(det) * n - grad);
return base;
"""


def create_cell_layer_material(name, mpc, cell_size, density, translucent, tint_value=1.0, relief=1.0):
    material = fresh_material(name)
    material.set_editor_property("tangent_space_normal", False)
    if translucent:
        # Dünne Zellschicht mit klarer Flüssigkeit: durchscheinend, leicht brechend (Gewebe n ≈ 1,37 gegen Flüssigkeit 1,34)
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
        material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        material.set_editor_property("two_sided", True)
    else:
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    local = expression(material, unreal.MaterialExpressionLocalPosition, -1300, 0)
    world = expression(material, unreal.MaterialExpressionWorldPosition, -1300, 100)
    normal = expression(material, unreal.MaterialExpressionVertexNormalWS, -1300, 200)
    view = expression(material, unreal.MaterialExpressionCameraVectorWS, -1300, 300)
    strength = expression(material, unreal.MaterialExpressionScalarParameter, -1300, 400, parameter_name="ReliefStrength", default_value=relief)
    size = expression(material, unreal.MaterialExpressionScalarParameter, -1300, 500, parameter_name="CellSize", default_value=cell_size)
    dens = expression(material, unreal.MaterialExpressionScalarParameter, -1300, 600, parameter_name="Density", default_value=density)
    tint = expression(material, unreal.MaterialExpressionScalarParameter, -1300, 700, parameter_name="Tint", default_value=tint_value)
    site = expression(material, unreal.MaterialExpressionCollectionParameter, -1300, 800)
    site.set_editor_property("collection", mpc)
    site.set_editor_property("parameter_name", "Site")
    compose = custom(material, -800, 0, "Zellschicht", SHELL_CODE, ["L", "WP", "Nrm", "V", "Strength", "CellSize", "Density", "Tint", "Site"], FLOAT3,
                     [("Roughness", FLOAT1), ("NormalWS", FLOAT3), ("Opacity", FLOAT1)])
    for source, pin in ((local, "L"), (world, "WP"), (normal, "Nrm"), (view, "V"), (strength, "Strength"), (size, "CellSize"), (dens, "Density"), (tint, "Tint"), (site, "Site")):
        connect(source, "", compose, [pin])
    mel.connect_material_property(compose, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(compose, "Roughness", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(compose, "NormalWS", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(expression(material, unreal.MaterialExpressionConstant, -500, 480, r=0.25), "",
                                  unreal.MaterialProperty.MP_SPECULAR)
    if translucent:
        mel.connect_material_property(compose, "Opacity", unreal.MaterialProperty.MP_OPACITY)
        mel.connect_material_property(expression(material, unreal.MaterialExpressionConstant, -500, 560, r=1.02), "",
                                      unreal.MaterialProperty.MP_REFRACTION)
    else:
        sss = expression(material, unreal.MaterialExpressionConstant3Vector, -500, 300)
        sss.set_editor_property("constant", unreal.LinearColor(0.42, 0.35, 0.28, 1.0))
        mel.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        mel.connect_material_property(expression(material, unreal.MaterialExpressionConstant, -500, 400, r=0.6), "",
                                      unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("%s erstellt" % name)
    return material


def create_shell_material(mpc):
    # Kugel mit 55 µm Radius steht für 100 µm Keimradius: Zellen ~30 µm -> 16 Einheiten im Objektraum
    return create_cell_layer_material("M_GEN_TrophoblastShell", mpc, 16.0, 0.10, True)


def create_inner_cell_mass_material(mpc):
    # Embryoblast: dicht gepackte, kleine Zellen (~12 µm) – ein trüber Knoten
    # Durch die Hülle gesehen gedämpft; dicht gepackte Zellen wölben sich deutlich (Maulbeerform)
    return create_cell_layer_material("M_GEN_InnerCellMass", mpc, 6.5, 1.0, False, tint_value=0.38, relief=2.5)

def import_data_texture():
    """R Drüsenöffnung, G Furche, B Polster, A Kennwert der Drüse (Vertexfarben kamen in Unreal nicht an)."""
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, "T_GEN_EndometriumData.png"))
    task.set_editor_property("destination_path", ENVIRONMENT)
    task.set_editor_property("destination_name", "T_GEN_EndometriumData")
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])
    texture = eal.load_asset(ENVIRONMENT + "/T_GEN_EndometriumData")
    # Daten, keine Farbe: linear und unkomprimiert (RGBA16F). Mit „Vector Displacement Map" (BGRA8) las das Material Rot
    # und Blau vertauscht (gemessen: Polster statt Drüse im Rotkanal); Blockkompression würde die Masken stufig machen.
    texture.set_editor_property("srgb", False)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_HDR)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    eal.save_loaded_asset(texture)
    log("Datenbild importiert: %s" % texture.get_path_name())
    return texture


def import_endometrium():
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
    task.set_editor_property("filename", os.path.join(SOURCE, "SM_GEN_Endometrium.fbx"))
    task.set_editor_property("destination_path", ENVIRONMENT)
    task.set_editor_property("destination_name", "SM_GEN_Endometrium")
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])
    mesh = eal.load_asset(ENVIRONMENT + "/SM_GEN_Endometrium")
    if not mesh:
        unreal.log_error("GENESIS: Import der Schleimhaut fehlgeschlagen")
        return None
    bounds = mesh.get_bounds()
    log("Schleimhaut: Ausdehnung %s, Nanite %s" % (bounds.box_extent, mesh.get_editor_property("nanite_settings").get_editor_property("enabled")))
    return mesh


def build_level(mesh, material, mpc, shell, inner):
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    mesh.set_material(0, material)
    eal.save_loaded_asset(mesh)

    # Hinterwand der Gebärmutter (hier nistet sich der Keim meist ein) und – durch einen flüssigkeitsgefüllten Spalt
    # von gut 1,5 mm getrennt – die Vorderwand. Das Cavum ist kein Hohlraum, die Wände liegen fast aneinander.
    walls = [("Endometrium_Posterior", unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))]
    if not os.environ.get("GENESIS_NO_ANTERIOR"):
        # 3,2 mm über der Hinterwand: Die Kamera weicht mit dem wachsenden Keim bis ~2 mm hoch zurück und darf die Vorderwand nie
        # durchstoßen (bei 1,6 mm sah sie deren Drüsenschläuche von außen als weiße Säulen)
        walls.append(("Endometrium_Anterior", unreal.Vector(700, -400, 3200), unreal.Rotator(180, 0, 37)))
    for label, location, rotation in walls:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
        actor.set_actor_label(label)
        component = actor.static_mesh_component
        component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
        component.set_static_mesh(mesh)
        component.set_editor_property("cast_shadow", True)
        component.set_editor_property("evaluate_world_position_offset", True)

    with open(os.path.join(SOURCE, "SM_GEN_Endometrium_site.txt")) as handle:
        surface = float(handle.read().strip())

    # Der Keim erscheint als Hülle (Einnistungsstelle); die Einzelzellen der Simulation gehören ins Mikroskop der ersten Woche
    site = actors.spawn_actor_from_class(unreal.GenesisImplantationSite, unreal.Vector(0, 0, surface))
    site.set_actor_label("ImplantationSite")
    site.set_editor_property("parameters", mpc)
    conceptus = site.get_editor_property("conceptus")
    conceptus.set_static_mesh(eal.load_asset(CELL_MESH))
    conceptus.set_material(0, shell)
    inner_cell_mass = site.get_editor_property("inner_cell_mass")
    inner_cell_mass.set_static_mesh(eal.load_asset(CELL_MESH))
    inner_cell_mass.set_material(0, inner)
    # Die leere Zona (EmptyZona) bleibt vorerst leer: Gestaucht wirkte sie wie eine Kontaktlinse – weglassen statt falsch zeigen
    site.set_editor_property("target_illuminance_lux", float(os.environ.get("GENESIS_LUX", "90.0")))
    # Belichtung wie beim Mikroskop: die Kamera belichtet selbst (physikalisch), das Volume nur für Bloom und Vignette
    # 8,5 EV: gemessen – bei 10 EV lagen die Flächen in der Schulter des Tonemappers, alles wirkte blass und flach
    site.set_editor_property("exposure_bias", float(os.environ.get("GENESIS_EXPOSURE_BIAS", "8.5")))

    # Uterusflüssigkeit: dünn streuend, sichtbar nur im Lichtkegel
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -100000))
    fog.set_actor_label("UterineFluid")
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_component.set_editor_property("fog_density", float(os.environ.get("GENESIS_FOG", "0.004")))
    fog_component.set_editor_property("fog_height_falloff", 0.00001)
    fog_component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    fog_component.set_editor_property("directional_inscattering_luminance", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    fog_component.set_editor_property("enable_volumetric_fog", True)
    fog_component.set_editor_property("volumetric_fog_scattering_distribution", 0.55)
    fog_component.set_editor_property("volumetric_fog_albedo", unreal.Color(236, 226, 220, 255))

    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    volume.set_actor_label("HysteroscopePostProcess")
    volume.set_editor_property("unbound", True)
    settings = volume.get_editor_property("settings")
    for name, value in (
        ("bloom_intensity", 0.08),
        ("lens_flare_intensity", 0.0),
        ("scene_fringe_intensity", 0.0),
        ("vignette_intensity", 0.25),
        ("film_grain_intensity", 0.0),
        ("motion_blur_amount", 0.3),
    ):
        settings.set_editor_property("override_" + name, True)
        settings.set_editor_property(name, value)
    volume.set_editor_property("settings", settings)

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    log("Karte gespeichert: %s -> %s (Oberfläche an der Stelle %.2f µm)" % (MAP_PATH, saved, surface))


ensure_folders()
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)
collection = create_parameter_collection()
data_texture = eal.load_asset(ENVIRONMENT + "/T_GEN_EndometriumData") if os.environ.get("GENESIS_SKIP_IMPORT") else import_data_texture()
endometrium_material = create_endometrium_material(collection, data_texture)
shell_material = create_shell_material(collection)
inner_material = create_inner_cell_mass_material(collection)
endometrium = eal.load_asset(ENVIRONMENT + "/SM_GEN_Endometrium") if os.environ.get("GENESIS_SKIP_IMPORT") else import_endometrium()
if endometrium and not os.environ.get("GENESIS_SKIP_LEVEL"):
    build_level(endometrium, endometrium_material, collection, shell_material, inner_material)
