# GENESIS – der Mutterleib aus Sicht des Kindes (GENESIS-044 Teil 1b): Import, Materialien, Karte L_GEN_Mutterleib.
#
# Ausführung headless (nach Tools/Blender/Gestation/build_mutterleib.py → export_all):
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Gestation/setup_mutterleib.py" -unattended
# Umgebungsvariablen: GENESIS_SKIP_IMPORT=1, GENESIS_REIMPORT=<Skelettnetze, kommagetrennt>.
#
# Maßstab: 1 cm = 1 Unreal-Einheit, gebaut für 10 cm Innenradius; AGenesisWombScene skaliert mit der Woche.
# Materialien nach den Referenzen (Docs/36): Wand eine glänzende Haut, durch die vorne Rotlicht dringt; Kindseite der
# Plazenta bläulich-violett durchscheinend, Gefäße erhaben; Nabelschnur weißlich-bläulich, Gefäße dunkel darin.

import json
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "Embryogenesis"))
os.environ["GENESIS_EMBRYO_LIB_ONLY"] = "1"
import setup_embryo as lib  # noqa: E402

eal = lib.eal
mel = lib.mel
SOURCE = os.path.join(lib.REPO, "ArtSource", "Generated", "Gestation")
ROOT = "/Game/Genesis/Gestation"
MESHES = ROOT + "/Meshes"
MATERIALS = ROOT + "/Materials"
MAP_PATH = ROOT + "/Maps/L_GEN_Mutterleib"
SPEECH = "/Game/Genesis/Audio/Speech/"

# Nabelgefäße im Durchlicht: Spirale aus der UV der Nabelschnur (Tools/Blender/Gestation/build_mutterleib.py,
# GEFAESS_WINDUNGEN und die Phasen der drei Gefäße müssen dazu passen)
VESSEL_CODE = """
// u entlang der Schnur, v um sie herum. Drei Gefäße (zwei Arterien, eine Vene) winden sich spiralig: v = Phase/2pi + Windungen*u.
// Von außen gesehen liegen sie als Schatten tief in der Sulze: sehr weicher Rand, die Vene breiter.
// Der FBX-Import spiegelt V (in Unreal gemessen, GENESIS-047): Mit UV.y wanden sich die Gefäße rechtsherum. Echte
// Nabelschnüre sind 7:1 linksgewunden (Lacro 1987, DOI 10.1016/s0002-9378(87)80067-4) – daher 1 − V wie in Blender.
const float Phase[3] = { 0.0, 2.1, 4.2 };
const float Width[3] = { 0.07, 0.07, 0.09 };
float Around = 1.0 - UV.y;
float Mask = 0.0;
for (int Index = 0; Index < 3; ++Index)
{
    float Distance = abs(frac(Around - Turns * UV.x - Phase[Index] / 6.2831853 + 0.5) - 0.5);
    Mask = max(Mask, 1.0 - smoothstep(0.1 * Width[Index], 1.6 * Width[Index], Distance));
}
return Mask;
"""

BODY_SKIN_CODE = """
// Haut des Kindes von außen (nur Koerper = 1): Käseschmiere und Lanugo nach der Woche.
// Käseschmiere (Vernix caseosa) entsteht im letzten Drittel, von Kopf zu Fuß und vom Rücken zum Bauch (Nishijima 2019,
// DOI 10.1111/jog.14103); zum Termin löst sie sich teils ins Fruchtwasser (Lamberti 1978). Sie liegt in Flecken, wachsig.
// Lanugo: feine Härchen ab SSW ~20, zum Termin größtenteils abgestoßen – im Gegenlicht ein weicher Saum am Umriss.
Roughness = Rough;
Emission = Emit;
if (Body < 0.5) return Base;
float3 n = normalize(NLocal);                        // Netz: +X Blickrichtung (Gesicht), +Z Scheitel
float dorsal = saturate(0.35 - 0.8 * n.x);           // Rücken zuerst
float cranial = saturate(0.5 + 0.5 * n.z);           // Kopf zuerst
float threshold = 1.0 - Vernix * (0.45 + 0.35 * dorsal + 0.2 * cranial);
float cover = Vernix > 0.001 ? smoothstep(threshold - 0.15, threshold + 0.15, Patch) : 0.0;
float3 color = lerp(Base, float3(0.86, 0.82, 0.74), 0.9 * cover);
// Augen: dunkles Pigment der Netzhaut scheint durch die dünnen, verklebten Lider (Netz: Augen bei (0, ±EyeHalf, 0) cm).
// Nur auf der Gesichtsseite (x > −0,3 · Abstand), weicher Rand – ein Fleck unter der Haut, kein aufgemaltes Auge.
float eyeRadius = 0.42 * EyeHalf;
float dl = length(float2(P.y - EyeHalf, P.z));
float dr = length(float2(P.y + EyeHalf, P.z));
float front = smoothstep(-0.3 * EyeHalf, 0.1 * EyeHalf, P.x);
float eye = EyeHalf > 0.0 ? (1.0 - smoothstep(0.45 * eyeRadius, eyeRadius, min(dl, dr))) * front * Pigment : 0.0;
color = lerp(color, float3(0.035, 0.03, 0.045), 0.95 * eye);
Roughness = lerp(Rough, 0.68, cover);
float3 V = normalize(Camera);
float rim = pow(1.0 - saturate(abs(dot(normalize(NWorld), V))), 3.0);
Emission = (Emit * (1.0 - 0.3 * cover) + Light * rim * Lanugo * 0.45) * (1.0 - 0.8 * eye);   // Käseschmiere: dünne Schicht
color = lerp(color, float3(0.78, 0.72, 0.66), Lanugo * rim * 0.2);
return color;
"""

DENT_CODE = """
// Bis zu vier Dellen: D = Kontaktpunkt auf der Oberfläche (Welt), T = Tiefe (Welt). Weicher Abfall, flacher Wulst ringsum.
float3 Dents[4] = { D0, D1, D2, D3 };
float Depths[4] = { T0, T1, T2, T3 };
float3 Offset = 0;
for (int Index = 0; Index < 4; ++Index)
{
    float Depth = Depths[Index];
    if (Depth <= 0) continue;
    float Distance = length(WorldPos - Dents[Index]);
    float Inner = saturate(1.0 - Distance / Radius);
    Inner = Inner * Inner * (3.0 - 2.0 * Inner);
    float Ring = saturate(1.0 - abs(Distance - 1.35 * Radius) / (0.5 * Radius));
    Offset += Normal * Depth * (0.2 * Ring - Inner);
}
return Offset;
"""

PARTS = {
    # Bauteil: (Nanite, Komponente)
    "SM_GEN_Womb_Wall": (True, "wall"),
    "SM_GEN_Womb_Placenta": (True, "placenta"),
    "SM_GEN_Womb_PlacentaVessels": (True, "placenta_vessels"),
}

# Instanzen des Gewebematerials (Farbe, Durchschein, Rauheit, Streuung, Durchlass für das Rotlicht von hinten,
# Gefäßmuster: wie stark die Nabelgefäße das Durchlicht schlucken – nur die Nabelschnur hat die UV dafür)
LOOKS = {
    # Kindseite der Plazenta: glänzende, bläulich-violette Haut über dunkelrotem Gewebe (2–3 cm dick: kaum Durchlass)
    "MI_GEN_Womb_Placenta": ((0.14, 0.055, 0.075), (0.36, 0.10, 0.12), 0.28, 0.9, 0.05, 0.0),
    # die großen Gefäße darauf: dunkles Blau-Violett, Blut schluckt das Licht
    "MI_GEN_Womb_PlacentaVessels": ((0.07, 0.03, 0.07), (0.20, 0.05, 0.10), 0.25, 0.9, 0.01, 0.0),
    # Nabelschnur: weißlich-bläulich, die Wharton-Sulze gallertig durchscheinend, die Gefäße als dunkle Spirale darin
    "MI_GEN_Womb_Cord": ((0.42, 0.42, 0.48), (0.55, 0.45, 0.50), 0.22, 0.95, 0.45, 0.5),
    # Haut des Kindes (die eigene Hand, Teil 2a): dünn, rosig über dem Blut darunter – im Gegenlicht leuchten die
    # Finger rot durch wie eine Hand vor einer Taschenlampe
    "MI_GEN_Womb_Skin": ((0.55, 0.33, 0.30), (0.85, 0.25, 0.18), 0.35, 0.95, 0.6, 0.0),
    # Das Kind selbst (Teil 2b): dieselbe Haut, ohne die Nagel- und Faltenkanäle der eigenen Hand
    "MI_GEN_Womb_FetusSkin": ((0.55, 0.33, 0.30), (0.85, 0.25, 0.18), 0.4, 0.95, 0.5, 0.0),
}

# SSW 10 für den frühen Moment von außen (Docs/37 Teil 2c); SSW 8 ist noch ein Embryo – eigenes Modell, folgt
FETUS_WEEKS = (10, 12, 16, 20, 24, 28, 32, 36, 40)


def ensure_folders():
    for folder in (ROOT, MESHES, MATERIALS, ROOT + "/Maps"):
        if not eal.does_directory_exist(folder):
            eal.make_directory(folder)


def create_looks(parent):
    looks = {}
    for name, (colour, through, rough, scatter, transmit, vessels) in LOOKS.items():
        path = MATERIALS + "/" + name
        mi = eal.load_asset(path) if eal.does_asset_exist(path) else lib.asset_tools.create_asset(
            name, MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, parent)
        mel.set_material_instance_vector_parameter_value(mi, "Farbe", unreal.LinearColor(*colour, 1.0))
        mel.set_material_instance_vector_parameter_value(mi, "Durchschein", unreal.LinearColor(*through, 1.0))
        mel.set_material_instance_scalar_parameter_value(mi, "Rauheit", rough)
        mel.set_material_instance_scalar_parameter_value(mi, "Streuung", scatter)
        mel.set_material_instance_scalar_parameter_value(mi, "Durchlass", transmit)
        mel.set_material_instance_scalar_parameter_value(mi, "Gefaessmuster", vessels)
        mel.set_material_instance_scalar_parameter_value(mi, "Anatomie", 1.0 if name == "MI_GEN_Womb_Skin" else 0.0)
        # Käseschmiere in Flecken und Lanugo-Saum nur am Körper des Kindes (die eigene Hand hat die Faltenmaske)
        mel.set_material_instance_scalar_parameter_value(mi, "Koerper", 1.0 if name == "MI_GEN_Womb_FetusSkin" else 0.0)
        eal.save_loaded_asset(mi)
        looks[name] = mi
    return looks


def light_from_belly(material, x, y, direction):
    """Rotlicht durch den Bauch: Glow × Lichtfarbe × (Grundanteil + Anteil zum Bauch²). direction: Richtungsknoten."""
    belly = lib.vector_param(material, "BellyDirection", x - 300, y + 150, (1.0, 0.0, 0.0))
    dot = lib.expression(material, unreal.MaterialExpressionDotProduct, x - 100, y)
    lib.link(direction, dot, "A")
    lib.link(belly, dot, "B")
    front = lib.expression(material, unreal.MaterialExpressionSaturate, x + 20, y)
    lib.link(dot, front, "")
    square = lib.expression(material, unreal.MaterialExpressionMultiply, x + 140, y)
    lib.link(front, square, "A")
    lib.link(front, square, "B")
    ambient = lib.expression(material, unreal.MaterialExpressionAdd, x + 260, y)
    lib.link(square, ambient, "A")
    lib.link(lib.scalar_param(material, "Grundleuchten", x + 140, y + 120, 0.06), ambient, "B")
    glow = lib.expression(material, unreal.MaterialExpressionMultiply, x + 380, y + 20)
    lib.link(ambient, glow, "A")
    lib.link(lib.scalar_param(material, "Glow", x + 260, y + 160, 0.0), glow, "B")
    tint = lib.expression(material, unreal.MaterialExpressionMultiply, x + 500, y + 20)
    lib.link(glow, tint, "A")
    lib.link(lib.vector_param(material, "Lichtfarbe", x + 380, y + 200, (1.0, 0.09, 0.04)), tint, "B")
    return tint


def create_tissue_material():
    """
    Gewebe in der Höhle (Plazenta, Nabelschnur): nass, streuend – und im Gegenlicht durchscheinend. Unreal rechnet
    Durchlicht (Subsurface-Transmission) nur für Lampen, nicht für die leuchtende Wand. Das Licht, das von der
    Bauchseite durch das Gewebe zum Auge kommt, gibt deshalb die Emission aus: Glow × Rot × Durchlass, stärker, wenn
    der Blick zum Bauch geht (das Gewebe steht vor der hellen Wand), und am Rand, wo der Weg durchs Gewebe kurz ist.
    """
    path = MATERIALS + "/M_GEN_WombTissue"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    material = lib.asset_tools.create_asset("M_GEN_WombTissue", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    material.set_editor_property("used_with_nanite", True)
    material.set_editor_property("used_with_skeletal_mesh", True)
    # Haut des Kindes: Nägel und Falten aus Blender (UV-Kanal 2: x = Nagel, y = Falte, build_fetal_hand.py) –
    # Nägel heller und glatter, Falten dunkler. Nur wo „Anatomie“ = 1 (die Hand); die übrigen Netze haben den Kanal nicht.
    data = lib.expression(material, unreal.MaterialExpressionTextureCoordinate, -1700, -500)
    data.set_editor_property("coordinate_index", 1)
    nail_mask = lib.expression(material, unreal.MaterialExpressionComponentMask, -1550, -560, r=True, g=False, b=False, a=False)
    lib.link(data, nail_mask, "")
    crease_channel = lib.expression(material, unreal.MaterialExpressionComponentMask, -1650, -440, r=False, g=True, b=False, a=False)
    lib.link(data, crease_channel, "")
    # Der FBX-Import spiegelt V auch im Datenkanal: In Unreal lag die Falte auf 91 % der Hand bei 1 (gemessen,
    # GENESIS-047) – die ganze Hand war dunkel und ab SSW 36 ganz mit Käseschmiere belegt. Zurückspiegeln.
    crease_mask = lib.expression(material, unreal.MaterialExpressionOneMinus, -1550, -440)
    lib.link(crease_channel, crease_mask, "")
    anatomy = lib.scalar_param(material, "Anatomie", -1550, -320, 0.0)
    nail = lib.expression(material, unreal.MaterialExpressionMultiply, -1400, -560)
    lib.link(nail_mask, nail, "A")
    lib.link(anatomy, nail, "B")
    crease = lib.expression(material, unreal.MaterialExpressionMultiply, -1400, -440)
    lib.link(crease_mask, crease, "A")
    lib.link(anatomy, crease, "B")
    darken = lib.expression(material, unreal.MaterialExpressionLinearInterpolate, -1250, -440)
    lib.link(lib.expression(material, unreal.MaterialExpressionConstant, -1400, -380, r=1.0), darken, "A")
    lib.link(lib.expression(material, unreal.MaterialExpressionConstant, -1400, -340, r=0.62), darken, "B")
    lib.link(crease, darken, "Alpha")
    skin = lib.expression(material, unreal.MaterialExpressionMultiply, -1100, -300)
    lib.link(lib.vector_param(material, "Farbe", -1250, -300, (0.4, 0.3, 0.3)), skin, "A")
    lib.link(darken, skin, "B")
    colour = lib.expression(material, unreal.MaterialExpressionLinearInterpolate, -950, -250)
    lib.link(skin, colour, "A")
    lib.link(lib.vector_param(material, "Nagelfarbe", -1100, -180, (0.78, 0.62, 0.60)), colour, "B")
    lib.link(nail, colour, "Alpha")
    # Käseschmiere (Vernix caseosa, ab SSW ~36): weißlich-wachsig, vor allem in den Falten (Nishijima 2019)
    vernix_amount = lib.expression(material, unreal.MaterialExpressionMultiply, -950, -380)
    lib.link(crease, vernix_amount, "A")
    lib.link(lib.scalar_param(material, "Kaeseschmiere", -1100, -420, 0.0), vernix_amount, "B")
    with_vernix = lib.expression(material, unreal.MaterialExpressionLinearInterpolate, -800, -250)
    lib.link(colour, with_vernix, "A")
    lib.link(lib.vector_param(material, "Vernixfarbe", -950, -330, (0.86, 0.82, 0.74)), with_vernix, "B")
    lib.link(vernix_amount, with_vernix, "Alpha")
    mel.connect_material_property(with_vernix, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = lib.expression(material, unreal.MaterialExpressionLinearInterpolate, -950, -120)
    lib.link(lib.scalar_param(material, "Rauheit", -1100, -120, 0.3), rough, "A")
    lib.link(lib.expression(material, unreal.MaterialExpressionConstant, -1100, -60, r=0.18), rough, "B")
    lib.link(nail, rough, "Alpha")
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(lib.scalar_param(material, "Glanz", -900, 0, 0.3), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(lib.vector_param(material, "Durchschein", -900, 100, (0.5, 0.3, 0.3)), "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    mel.connect_material_property(lib.scalar_param(material, "Streuung", -900, 200, 0.9), "", unreal.MaterialProperty.MP_OPACITY)

    # Blickrichtung (vom Auge weg): Steht das Gewebe vor der Bauchwand, kommt ihr Licht hindurch
    camera = lib.expression(material, unreal.MaterialExpressionCameraVectorWS, -1900, 400)
    away = lib.expression(material, unreal.MaterialExpressionMultiply, -1750, 400)
    lib.link(camera, away, "A")
    minus = lib.expression(material, unreal.MaterialExpressionConstant, -1900, 480)
    minus.set_editor_property("r", -1.0)
    lib.link(minus, away, "B")
    light = light_from_belly(material, -1400, 400, away)
    # Am Rand kürzerer Weg durchs Gewebe: mehr Durchlicht
    rim = lib.expression(material, unreal.MaterialExpressionFresnel, -900, 700, exponent=2.0, base_reflect_fraction=0.35)
    through = lib.expression(material, unreal.MaterialExpressionMultiply, -700, 450)
    lib.link(light, through, "A")
    lib.link(rim, through, "B")
    emit = lib.expression(material, unreal.MaterialExpressionMultiply, -550, 450)
    lib.link(through, emit, "A")
    lib.link(lib.scalar_param(material, "Durchlass", -700, 600, 0.2), emit, "B")
    # Die Nabelgefäße schlucken das Durchlicht (Blut): Emission × (1 − Gefäßmuster × Maske)
    uv = lib.expression(material, unreal.MaterialExpressionTextureCoordinate, -1100, 900)
    turns = lib.expression(material, unreal.MaterialExpressionConstant, -1100, 980)
    turns.set_editor_property("r", 5.0)
    vessels = lib.expression(material, unreal.MaterialExpressionCustom, -900, 920)
    vessels.set_editor_property("description", "Nabelgefaesse")
    vessels.set_editor_property("code", VESSEL_CODE)
    vessels.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    entries = []
    for input_name in ("UV", "Turns"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", input_name)
        entries.append(entry)
    vessels.set_editor_property("inputs", entries)
    lib.link(uv, vessels, "UV")
    lib.link(turns, vessels, "Turns")
    shade = lib.expression(material, unreal.MaterialExpressionMultiply, -700, 900)
    lib.link(vessels, shade, "A")
    lib.link(lib.scalar_param(material, "Gefaessmuster", -900, 1040, 0.0), shade, "B")
    keep = lib.expression(material, unreal.MaterialExpressionOneMinus, -550, 900)
    lib.link(shade, keep, "")
    shaded = lib.expression(material, unreal.MaterialExpressionMultiply, -400, 500)
    lib.link(emit, shaded, "A")
    lib.link(keep, shaded, "B")
    mel.connect_material_property(shaded, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # Körper des Kindes (Koerper = 1): Käseschmiere in Flecken (Rauschen in der Lage im Netz, cm), Lanugo als Saum
    body = lib.expression(material, unreal.MaterialExpressionCustom, -250, -100)
    body.set_editor_property("description", "Haut des Kindes")
    body.set_editor_property("code", BODY_SKIN_CODE)
    body.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    entries = []
    for input_name in ("Base", "Rough", "Emit", "NLocal", "NWorld", "Camera", "Light", "Patch", "Body", "Vernix", "Lanugo",
                       "P", "EyeHalf", "Pigment"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", input_name)
        entries.append(entry)
    body.set_editor_property("inputs", entries)
    extra = []
    for output_name, output_type in (("Roughness", unreal.CustomMaterialOutputType.CMOT_FLOAT1), ("Emission", unreal.CustomMaterialOutputType.CMOT_FLOAT3)):
        output = unreal.CustomOutput()
        output.set_editor_property("output_name", output_name)
        output.set_editor_property("output_type", output_type)
        extra.append(output)
    body.set_editor_property("additional_outputs", extra)
    normal_ws = lib.expression(material, unreal.MaterialExpressionVertexNormalWS, -700, -40)
    normal_local = lib.expression(material, unreal.MaterialExpressionTransform, -550, -40)
    normal_local.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_WORLD)
    normal_local.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_LOCAL)
    lib.link(normal_ws, normal_local, "")
    patch = lib.expression(material, unreal.MaterialExpressionNoise, -550, 60)
    # Simplex statt Gradient: Das Gradient-Rauschen zeigte an der Schwelle feine gerade Streifen (Gitterartefakte)
    patch.set_editor_property("noise_function", unreal.NoiseFunction.NOISEFUNCTION_SIMPLEX_TEX)
    # Flecken von 1–2 cm (Lage im Netz in cm), eine Oktave: Mit drei Oktaven zerfiel die Schmiere an der Schwelle in
    # ein feines Krakelee (gesehen bei SSW 34)
    patch.set_editor_property("scale", 0.55)
    patch.set_editor_property("levels", 1)
    patch.set_editor_property("output_min", 0.0)
    patch.set_editor_property("output_max", 1.0)
    lib.link(lib.expression(material, unreal.MaterialExpressionLocalPosition, -700, 60), patch, "Position")
    for node, pin in ((with_vernix, "Base"), (rough, "Rough"), (shaded, "Emit"), (normal_local, "NLocal"), (normal_ws, "NWorld"),
                      (camera, "Camera"), (light, "Light"), (patch, "Patch"),
                      (lib.scalar_param(material, "Koerper", -550, 160, 0.0), "Body"),
                      (lib.scalar_param(material, "Kaeseschmiere", -550, 220, 0.0), "Vernix"),
                      (lib.scalar_param(material, "Lanugo", -550, 280, 0.0), "Lanugo"),
                      (lib.expression(material, unreal.MaterialExpressionLocalPosition, -550, 340), "P"),
                      (lib.scalar_param(material, "AugenAbstand", -550, 400, 0.0), "EyeHalf"),
                      (lib.scalar_param(material, "Augenpigment", -550, 460, 0.0), "Pigment")):
        lib.link(node, body, pin)
    mel.connect_material_property(body, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(body, "Roughness", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(body, "Emission", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # Delle: Wo ein Finger drückt, gibt die Wharton-Sulze nach – die Oberfläche weicht weich zurück, ringsum wölbt sie sich
    # leicht (das Volumen bleibt). Die Szene setzt bis zu vier Kontakte (Welt: Lage, Tiefe) und den Einflussradius.
    dent = lib.expression(material, unreal.MaterialExpressionCustom, -700, 1300)
    dent.set_editor_property("description", "Dellen")
    dent.set_editor_property("code", DENT_CODE)
    dent.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for input_name in ("WorldPos", "Normal", "D0", "D1", "D2", "D3", "T0", "T1", "T2", "T3", "Radius"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", input_name)
        inputs.append(entry)
    dent.set_editor_property("inputs", inputs)
    lib.link(lib.expression(material, unreal.MaterialExpressionWorldPosition, -1000, 1250), dent, "WorldPos")
    lib.link(lib.expression(material, unreal.MaterialExpressionVertexNormalWS, -1000, 1300), dent, "Normal")
    for k in range(4):
        lib.link(lib.vector_param(material, "Delle%d" % k, -1000, 1350 + 60 * k, (0.0, 0.0, -1.0e6)), dent, "D%d" % k)
        lib.link(lib.scalar_param(material, "Tiefe%d" % k, -1200, 1350 + 60 * k, 0.0), dent, "T%d" % k)
    lib.link(lib.scalar_param(material, "DellenRadius", -1000, 1600, 1.0), dent, "Radius")
    mel.connect_material_property(dent, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    material.set_editor_property("max_world_position_offset_displacement", 30.0)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def create_wall_material():
    """
    Die Wand: Fruchtblase an der Gebärmutterwand, nass und glänzend. Das Licht der Außenwelt kommt diffus durch den
    Bauch – aus dem Inneren gesehen leuchtet deshalb die Wand selbst, vorne (zum Bauch hin) am hellsten, hinten fast
    nicht. Durch Haut, Fett und Muskel kommt fast nur Rot (Docs/34). Die Szene setzt Glow nach der Lichtmenge im
    Mutterleib (lx) und die Richtung zum Bauch.
    """
    path = MATERIALS + "/M_GEN_WombWall"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    material = lib.asset_tools.create_asset("M_GEN_WombWall", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    material.set_editor_property("used_with_nanite", True)
    colour = lib.vector_param(material, "Farbe", -900, -200, (0.20, 0.050, 0.045))
    mel.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(lib.scalar_param(material, "Rauheit", -900, -100, 0.3), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(lib.scalar_param(material, "Glanz", -900, 0, 0.35), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(lib.vector_param(material, "Durchschein", -900, 100, (0.45, 0.07, 0.05)), "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    mel.connect_material_property(lib.scalar_param(material, "Streuung", -900, 200, 0.85), "", unreal.MaterialProperty.MP_OPACITY)

    # Leuchten: Glow × Rotlicht × (Grundanteil + Vorderseite²) × Fleckung
    world = lib.expression(material, unreal.MaterialExpressionWorldPosition, -1900, 400)
    center = lib.vector_param(material, "Center", -1900, 500, (0.0, 0.0, 0.0))
    offset = lib.expression(material, unreal.MaterialExpressionSubtract, -1750, 450)
    lib.link(world, offset, "A")
    lib.link(center, offset, "B")
    normal = lib.expression(material, unreal.MaterialExpressionNormalize, -1600, 450)
    lib.link(offset, normal, "VectorInput")
    light = light_from_belly(material, -1400, 400, normal)
    # Die Bauchdecke ist nicht überall gleich dick (Fett, Muskelstränge, Gefäße): Das Licht kommt fleckig an.
    # Das Rauschen läuft über die Richtung vom Mittelpunkt – es wächst mit der Höhle mit.
    spread = lib.expression(material, unreal.MaterialExpressionMultiply, -1450, 800)
    lib.link(normal, spread, "A")
    frequency = lib.expression(material, unreal.MaterialExpressionConstant, -1600, 880)
    frequency.set_editor_property("r", 3.5)
    lib.link(frequency, spread, "B")
    mottle = lib.expression(material, unreal.MaterialExpressionNoise, -1250, 800, scale=1.0, levels=4, output_min=0.55,
                            output_max=1.3, noise_function=unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_ALU)
    lib.link(spread, mottle, "Position")
    lit = lib.expression(material, unreal.MaterialExpressionMultiply, -700, 520)
    lib.link(light, lit, "A")
    lib.link(mottle, lit, "B")
    # Ihre Hand auf dem Bauch (GENESIS-044 Teil 2a) hält Licht ab: weicher Schatten um die Richtung, in der sie liegt.
    # Eine Hand bedeckt von innen gesehen etwa 30–40° der vorderen Wand; der Rand ist durch das Gewebe weich.
    hand = lib.vector_param(material, "HandDirection", -1450, 1050, (1.0, 0.0, 0.0))
    hand_dot = lib.expression(material, unreal.MaterialExpressionDotProduct, -1250, 1000)
    lib.link(normal, hand_dot, "A")
    lib.link(hand, hand_dot, "B")
    hand_code = lib.expression(material, unreal.MaterialExpressionCustom, -1050, 1000)
    hand_code.set_editor_property("description", "Handschatten")
    hand_code.set_editor_property("code", "return 1.0 - Press * 0.88 * smoothstep(0.74, 0.94, Cos);")
    hand_code.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    entries = []
    for input_name in ("Cos", "Press"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", input_name)
        entries.append(entry)
    hand_code.set_editor_property("inputs", entries)
    lib.link(hand_dot, hand_code, "Cos")
    lib.link(lib.scalar_param(material, "HandPress", -1250, 1120, 0.0), hand_code, "Press")
    shaded = lib.expression(material, unreal.MaterialExpressionMultiply, -550, 560)
    lib.link(lit, shaded, "A")
    lib.link(hand_code, shaded, "B")
    mel.connect_material_property(shaded, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def import_skinned(name, material):
    """Skelettnetz mit eigenem Skelett: die Hand (build_fetal_hand.py) und die weiche Nabelschnur (build_mutterleib.py)."""
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX false")
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("create_physics_asset", False)
    data = options.get_editor_property("skeletal_mesh_import_data")
    data.set_editor_property("import_morph_targets", False)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS)
    # Frisch importieren: ein Neuimport behält alte Materialplätze (dann bleibt ein Abschnitt ohne Haut)
    for old in (MESHES + "/" + name, MESHES + "/" + name + "_Skeleton"):
        if eal.does_asset_exist(old):
            eal.delete_asset(old)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, name + ".fbx"))
    task.set_editor_property("destination_path", MESHES)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    lib.asset_tools.import_asset_tasks([task])
    mesh = eal.load_asset(MESHES + "/" + name)
    if not mesh:
        unreal.log_error("GENESIS: Import fehlgeschlagen: " + name)
        return None
    # Die Einträge sind Kopien: neu aufbauen und zurückschreiben, sonst bleibt der Platz leer (graues Standardmaterial)
    materials = [unreal.SkeletalMaterial(material_interface=material, material_slot_name=slot.get_editor_property("material_slot_name"))
                 for slot in mesh.get_editor_property("materials")]
    mesh.set_editor_property("materials", materials)
    eal.save_loaded_asset(mesh)
    lib.log("%s: %d Materialplätze" % (name, len(materials)))
    # Das Skelett entsteht beim Import als eigenes Asset – ohne Speichern fehlt es im Spiel (Absturz beim Laden)
    if mesh.skeleton:
        eal.save_loaded_asset(mesh.skeleton, False)
        lib.log("Skelett gespeichert: %s" % mesh.skeleton.get_path_name())
    bounds = mesh.get_bounds()
    lib.log("%s: Ausdehnung %s, %d Knochen" % (name, bounds.box_extent, len(mesh.skeleton.get_editor_property("bone_tree")) if mesh.skeleton else -1))
    return mesh


def import_fetuses():
    """Das Kind je Woche (Tools/Blender/Gestation/build_fetus.py): Nanite-Netze und die Tabelle mit Mitte und Nabel."""
    table = json.load(open(os.path.join(SOURCE, "fetus_weeks.json"), encoding="utf-8"))
    stages = []
    reimport = set(filter(None, (os.environ.get("GENESIS_REIMPORT") or "").split(",")))
    for week in FETUS_WEEKS:
        name = "SM_GEN_Fetus_W%02d" % week
        fresh = name in reimport or "SM_GEN_Fetus" in reimport
        if os.environ.get("GENESIS_SKIP_IMPORT") and not fresh and eal.does_asset_exist(MESHES + "/" + name):
            mesh = eal.load_asset(MESHES + "/" + name)
        else:
            mesh = lib.import_part(name, True, SOURCE, MESHES)
        info = table.get(str(week))
        if not mesh or not info:
            unreal.log_error("GENESIS: Fetus SSW %d fehlt" % week)
            continue
        stage = unreal.GenesisFetusStage()
        stage.set_editor_property("weeks", float(week))
        stage.set_editor_property("mesh", mesh)
        # Blender → Unreal: Y gespiegelt (links bleibt links)
        c, n = info["center"], info["navel"]
        stage.set_editor_property("center", unreal.Vector(c[0], -c[1], c[2]))
        stage.set_editor_property("navel", unreal.Vector(n[0], -n[1], n[2]))
        stage.set_editor_property("crown_rump_cm", float(info["crl"]))
        stage.set_editor_property("eye_half_spacing_cm", float(info.get("eye_half_cm", 0.0)))
        stage.set_editor_property("hull", [unreal.Vector(h[0], -h[1], h[2]) for h in info.get("hull", [])])
        stages.append(stage)
    lib.log("Fetus: %d Alter" % len(stages))
    return stages


def build_level(meshes, wall_material, looks):
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    scene = actors.spawn_actor_from_class(unreal.GenesisWombScene, unreal.Vector(0, 0, 0))
    scene.set_actor_label("WombScene")
    materials = {
        "SM_GEN_Womb_Wall": wall_material,
        "SM_GEN_Womb_Placenta": looks["MI_GEN_Womb_Placenta"],
        "SM_GEN_Womb_PlacentaVessels": looks["MI_GEN_Womb_PlacentaVessels"],
    }
    for asset, (_, prop) in PARTS.items():
        component = scene.get_editor_property(prop)
        component.set_static_mesh(meshes[asset])
        component.set_material(0, materials[asset])
    scene.set_editor_property("fetus_stages", meshes.get("fetus_stages", []))
    scene.get_editor_property("fetus").set_material(0, looks["MI_GEN_Womb_FetusSkin"])
    for asset, prop, look in (("SK_GEN_FetalHand", "own_hand", "MI_GEN_Womb_Skin"), ("SK_GEN_Womb_Cord", "cord", "MI_GEN_Womb_Cord")):
        skinned = meshes.get(asset)
        if skinned:
            component = scene.get_editor_property(prop)
            component.set_skinned_asset_and_update(skinned)
            component.set_material(0, looks[look])

    # Ihre Stimme (Tools/Audio/speech_lines.py): an den Bauch; nach dem ersten Tritt; gegen Ende
    scene.set_editor_property("belly_lines", [eal.load_asset(SPEECH + "VO_S_M_Beruhigen"), eal.load_asset(SPEECH + "VO_Z_M_Schlaf")])
    scene.set_editor_property("kick_line", eal.load_asset(SPEECH + "VO_G_M_Bauch_01"))
    scene.set_editor_property("late_line", eal.load_asset(SPEECH + "VO_G_M_Bauch_02"))

    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    volume.set_actor_label("MutterleibPostProcess")
    volume.set_editor_property("unbound", True)
    settings = volume.get_editor_property("settings")
    for name, value in (("bloom_intensity", 0.15), ("lens_flare_intensity", 0.0), ("scene_fringe_intensity", 0.0),
                        ("vignette_intensity", 0.45), ("film_grain_intensity", 0.0), ("motion_blur_amount", 0.3)):
        settings.set_editor_property("override_" + name, True)
        settings.set_editor_property(name, value)
    volume.set_editor_property("settings", settings)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    lib.log("Karte gespeichert: %s -> %s" % (MAP_PATH, saved))


def main():
    ensure_folders()
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)
    looks = create_looks(create_tissue_material())
    wall = create_wall_material()
    meshes = {}
    for asset, (nanite, _) in PARTS.items():
        if os.environ.get("GENESIS_SKIP_IMPORT"):
            meshes[asset] = eal.load_asset(MESHES + "/" + asset)
        else:
            meshes[asset] = lib.import_part(asset, nanite, SOURCE, MESHES)
    # GENESIS_REIMPORT=SK_GEN_Womb_Cord,… importiert einzelne Skelettnetze neu, auch wenn GENESIS_SKIP_IMPORT gesetzt ist
    reimport = set(filter(None, (os.environ.get("GENESIS_REIMPORT") or "").split(",")))
    for asset, look in (("SK_GEN_FetalHand", "MI_GEN_Womb_Skin"), ("SK_GEN_Womb_Cord", "MI_GEN_Womb_Cord")):
        keep = os.environ.get("GENESIS_SKIP_IMPORT") and asset not in reimport and eal.does_asset_exist(MESHES + "/" + asset)
        meshes[asset] = eal.load_asset(MESHES + "/" + asset) if keep else import_skinned(asset, looks[look])
    # Alte starre Nabelschnur und die unsichtbaren Gefäßröhren: ersetzt, aus dem Projekt nehmen
    for old in (MESHES + "/SM_GEN_Womb_Cord", MESHES + "/SM_GEN_Womb_CordVessels", MATERIALS + "/MI_GEN_Womb_CordVessels"):
        if eal.does_asset_exist(old):
            eal.delete_asset(old)
    meshes["fetus_stages"] = import_fetuses()
    build_level(meshes, wall, looks)


main()
