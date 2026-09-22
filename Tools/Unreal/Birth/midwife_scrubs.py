# GENESIS: Der Kreislauf des Lebens
#
# Kreißsaal-Kleidung der Hebamme, die der MetaHuman Creator nicht mitbringt (er kennt nur T-Shirt und Shorts):
#   M_GEN_ScrubTrousers  – Kasackhose: gerade, weit geschnittene Beine aus Mischgewebe, Saum über dem Knöchel
#   M_GEN_NitrileGloves  – Untersuchungshandschuhe aus Nitril, blau, bis gut 6 cm über das Handgelenk
#   Die Shorts des Standard-Kleidungsstücks bleiben (umgefärbt) als oberer Teil der Hose: Unter ihnen hat der
#   MetaHuman-Körper keine Haut, die Hosenbeine setzen an ihrem Saum an.
#
# Beide Kleidungsstücke sind eine zweite Hülle auf dem Körper-Mesh der Hebamme (gleiches Skelett, Leader Pose):
# Das Material wählt über die Position in der Referenzpose aus, welcher Teil des Körpers bekleidet ist, und
# schiebt die Hülle dort nach außen – bei der Hose zu einer geraden Röhre um die Beinachse, beim Handschuh
# 0,7 mm über die Haut mit einem aufgerollten Rand. Maße aus dem Skelett der Hebamme (Referenzpose, cm):
#   Hüftgelenk (±9,6 | 2,3 | 84,0), Sprunggelenk (±13,1 | 0,1 | 7,2), Ellenbogen (±28,5 | −1,5 | 106,8),
#   Handgelenk (±40,1 | 12,6 | 92,9).
#
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Birth/midwife_scrubs.py" -unattended -nosplash

import unreal

FOLDER = "/Game/Genesis/People/Materials"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

TROUSERS_WPO = r"""
// Beine der Referenzpose: Hose von der Taille bis über den Knöchel, Hände und Arme ausgenommen
float side = P.x >= 0 ? 1.0 : -1.0;
float ax = abs(P.x);
float3 n = normalize(N);
// An Hüfte und Gesäß liegt der Stoff an (unter dem Kasack), darunter fällt er locker
float ease = lerp(0.3, 1.1, 1.0 - smoothstep(80.0, 92.0, P.z));
float3 o = n * ease;
// Unterhalb des Schritts: gerade Röhre um die Beinachse (Hüftgelenk -> Sprunggelenk)
float t = saturate((84.0 - P.z) / 76.8);
float2 c = lerp(float2(9.6 * side, 2.3), float2(13.1 * side, 0.1), t);
float2 r = P.xy - c;
float len = length(r);
float2 dir = len > 0.01 ? r / len : float2(side, 0.0);
// Oben verschwindet das Hosenbein im Saum des Oberteils der Hose (Shorts des MetaHuman, umgefärbt): Dort liegt es
// knapp an und weitet sich darunter zur geraden Röhre – sonst stünde am Saum eine Stufe
float R = max(len + 0.5, lerp(LegRadius, len + 0.5, smoothstep(48.0, 64.0, P.z)));
// Falten: Über dem Knöchel staucht sich der Stoff, am Knie gibt er nach – kleine Wellen um das Bein
float angle = atan2(dir.y, dir.x);
float fold = 0.22 * sin(P.z * 0.85 + angle * 3.0 + side) + 0.18 * sin(P.z * 0.31 - angle * 2.0);
fold *= 0.5 + 1.2 * (1.0 - smoothstep(12.0, 40.0, P.z));
float tube = 1.0 - smoothstep(63.0, 76.0, P.z);
float3 tubeOffset = float3(dir * max(0.3, R - len + fold * (1.0 - smoothstep(48.0, 64.0, P.z))), 0.0) + n * 0.15;
o = lerp(o, tubeOffset, tube);
float legs = (1.0 - smoothstep(21.0, 24.0, ax)) * step(P.z, Waist) * step(Hem, P.z);
return o * legs;
"""

TROUSERS_MASK = r"""
float ax = abs(P.x);
return (1.0 - smoothstep(21.0, 24.0, ax)) * step(P.z, Waist) * step(Hem, P.z);
"""

GLOVE_WPO = r"""
float side = P.x >= 0 ? 1.0 : -1.0;
float3 W = float3(40.1 * side, 12.6, 92.9);
float3 E = float3(28.5 * side, -1.5, 106.8);
float3 d = normalize(W - E);
float s = dot(P - W, d);
float hand = step(-Cuff, s) * step(25.0, abs(P.x));
// Nitril liegt 0,1 mm stark auf der Haut. Die Hülle steht 1,6 mm ab: Mit 0,7 mm schien bei gebeugten Fingern die Haut
// durch (die Hautgewichte des Körpers dehnen die Knöchel anders als die Hülle).
// Am Stulpenrand ist der Handschuh zu einem dünnen Wulst aufgerollt.
float bead = exp(-pow((s + Cuff - 0.45) / 0.45, 2.0)) * 0.16;
return normalize(N) * (0.16 + bead) * hand;
"""

GLOVE_MASK = r"""
float side = P.x >= 0 ? 1.0 : -1.0;
float3 W = float3(40.1 * side, 12.6, 92.9);
float3 E = float3(28.5 * side, -1.5, 106.8);
float s = dot(P - W, normalize(W - E));
return step(-Cuff, s) * step(25.0, abs(P.x));
"""

# Leichte Farbschwankung: Mischgewebe ist nie ganz gleichmäßig (Waschgänge, Faserverlauf)
FABRIC_VARIATION = r"""
float3 p = P * 0.35;
float v = sin(p.x * 1.7 + sin(p.z * 0.9)) * sin(p.z * 1.3 + p.y) * 0.5 + 0.5;
return lerp(0.94, 1.04, v);
"""


def log(message):
    unreal.log_warning("GENESIS: " + message)


def fresh(name):
    path = FOLDER + "/" + name
    if eal.does_asset_exist(path):
        material = eal.load_asset(path)
        mel.delete_all_material_expressions(material)
        return material
    return asset_tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


def node(material, cls, x, y, **props):
    result = mel.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        result.set_editor_property(key, value)
    return result


def link(src, src_out, dst, dst_in):
    if not mel.connect_material_expressions(src, src_out, dst, dst_in):
        unreal.log_error("GENESIS: Verbindung fehlgeschlagen %s -> %s.%s" % (src.get_name(), dst.get_name(), dst_in))


def custom(material, x, y, code, inputs, output_type, description):
    result = node(material, unreal.MaterialExpressionCustom, x, y)
    result.set_editor_property("code", code)
    result.set_editor_property("description", description)
    result.set_editor_property("output_type", output_type)
    entries = []
    for name, source in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        entries.append(entry)
    result.set_editor_property("inputs", entries)
    for name, source in inputs:
        link(source, "", result, name)
    return result


def scalar(material, x, y, name, value):
    return node(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)


def vector(material, x, y, name, rgb):
    return node(material, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def overlay_base(material):
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("opacity_mask_clip_value", 0.5)
    material.set_editor_property("used_with_skeletal_mesh", True)
    pos = node(material, unreal.MaterialExpressionPreSkinnedPosition, -1600, 0)
    nrm = node(material, unreal.MaterialExpressionPreSkinnedNormal, -1600, 150)
    return pos, nrm


def finish_wpo(material, wpo_local):
    world = node(material, unreal.MaterialExpressionTransform, -300, 400,
                 transform_source_type=unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,
                 transform_type=unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    link(wpo_local, "", world, "")
    mel.connect_material_property(world, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)


def finish_mask(material, mask):
    # Die Referenzpose gibt es nur im Vertex-Shader: Maske dort rechnen und an die Pixel weiterreichen
    interp = node(material, unreal.MaterialExpressionVertexInterpolator, -300, 600)
    link(mask, "", interp, "")
    mel.connect_material_property(interp, "", unreal.MaterialProperty.MP_OPACITY_MASK)


def build_trousers():
    material = fresh("M_GEN_ScrubTrousers")
    pos, nrm = overlay_base(material)
    leg = scalar(material, -1600, 300, "LegRadiusCm", 8.2)
    waist = scalar(material, -1600, 400, "WaistCm", 97.0)
    hem = scalar(material, -1600, 500, "HemCm", 9.5)
    wpo = custom(material, -1200, 0, TROUSERS_WPO, [("P", pos), ("N", nrm), ("LegRadius", leg), ("Waist", waist), ("Hem", hem)],
                 unreal.CustomMaterialOutputType.CMOT_FLOAT3, "Hose: Röhre um die Beinachse")
    mask = custom(material, -1200, 500, TROUSERS_MASK, [("P", pos), ("Waist", waist), ("Hem", hem)],
                  unreal.CustomMaterialOutputType.CMOT_FLOAT1, "Hose: Maske")
    finish_wpo(material, wpo)
    finish_mask(material, mask)

    # Farbe gleich dem Kasack; leicht wolkig; Rauheit eines Poly-Baumwoll-Köpers
    color = vector(material, -900, -400, "Color", (0.030, 0.165, 0.185))
    var_src = node(material, unreal.MaterialExpressionVertexInterpolator, -1000, -250)
    var = custom(material, -1300, -250, FABRIC_VARIATION, [("P", pos)], unreal.CustomMaterialOutputType.CMOT_FLOAT1, "Stoff: Schwankung")
    link(var, "", var_src, "")
    tint = node(material, unreal.MaterialExpressionMultiply, -600, -350)
    link(color, "", tint, "A")
    link(var_src, "", tint, "B")
    # Stoff: Flaum, der an streifendem Licht aufhellt (Fresnel), heller und weniger gesättigt als die Grundfarbe.
    # So sieht Köper aus der Nähe aus – ohne diesen Saum wirkt er wie lackiert.
    fresnel = node(material, unreal.MaterialExpressionFresnel, -900, -150, exponent=3.0, base_reflect_fraction=0.0)
    sheen = node(material, unreal.MaterialExpressionMultiply, -700, -150)
    link(fresnel, "", sheen, "A")
    link(scalar(material, -900, -60, "Sheen", 0.10), "", sheen, "B")
    base = node(material, unreal.MaterialExpressionAdd, -400, -300)
    link(tint, "", base, "A")
    link(sheen, "", base, "B")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(material, -600, -200, "Roughness", 0.82), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(scalar(material, -600, -120, "Specular", 0.35), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Hose: %s" % material.get_path_name())


def build_gloves():
    material = fresh("M_GEN_NitrileGloves")
    pos, nrm = overlay_base(material)
    cuff = scalar(material, -1600, 300, "CuffCm", 6.5)
    wpo = custom(material, -1200, 0, GLOVE_WPO, [("P", pos), ("N", nrm), ("Cuff", cuff)],
                 unreal.CustomMaterialOutputType.CMOT_FLOAT3, "Handschuh: über der Haut")
    mask = custom(material, -1200, 500, GLOVE_MASK, [("P", pos), ("Cuff", cuff)],
                  unreal.CustomMaterialOutputType.CMOT_FLOAT1, "Handschuh: Maske")
    finish_wpo(material, wpo)
    finish_mask(material, mask)
    # Nitril „violet blue": sRGB etwa (70, 100, 200) -> linear. Halbmatt, glänzt an Knöcheln und Fingerkuppen.
    mel.connect_material_property(vector(material, -600, -300, "Color", (0.058, 0.120, 0.56)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(material, -600, -150, "Roughness", 0.40), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(scalar(material, -600, -60, "Specular", 0.5), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    log("Handschuhe: %s" % material.get_path_name())


if not eal.does_directory_exist(FOLDER):
    eal.make_directory(FOLDER)
build_trousers()
build_gloves()
