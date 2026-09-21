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


def create_blastomere_material():
    """
    Furchungszelle: Das Zytoplasma stammt aus der Eizelle, ist also ebenso körnig –
    nur verteilt es sich mit jeder Teilung auf mehr und kleinere Zellen.
    """
    material = load_or_create_material("M_GEN_Blastomere")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)

    uv = expression(material, unreal.MaterialExpressionTextureCoordinate, -1700, 0)
    grain = custom(material, -1400, 0, "Zytoplasma-Korn", FBM2D_CODE, ["UV", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(uv, "", grain, ["UV"])
    connect(constant(material, -1700, 180, 42.0), "", grain, ["Freq"])

    coarse = custom(material, -1400, 300, "Plasmaschlieren", FBM2D_CODE, ["UV", "Freq"], unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(uv, "", coarse, ["UV"])
    connect(constant(material, -1700, 380, 6.5), "", coarse, ["Freq"])

    # Per-Instance-Daten aus der Simulation: 0 = Farbton der Zelle, 1 = Embryoblast, 2 = Fragmentierung
    tint = expression(material, unreal.MaterialExpressionPerInstanceCustomData, -1400, 520, data_index=0)
    inner = expression(material, unreal.MaterialExpressionPerInstanceCustomData, -1400, 620, data_index=1)

    structure = add(material, -1100, 150,
                    multiply(material, -1250, 60, grain, constant(material, -1400, 120, 0.25)),
                    multiply(material, -1250, 320, coarse, constant(material, -1400, 400, 0.30)))
    shade = multiply(material, -900, 300, structure, tint)

    dark = color(material, -900, -200, 0.045, 0.034, 0.028)
    light = color(material, -900, -80, 0.155, 0.12, 0.10)
    base = lerp3(material, -650, -140, dark, light, shade)

    # Der Embryoblast – aus ihm wird der Mensch – ist dichter gepackt und dadurch etwas wärmer und heller
    inner_color = color(material, -900, 40, 0.20, 0.14, 0.105)
    final_base = lerp3(material, -420, -80, base, inner_color, inner)
    mel.connect_material_property(final_base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    subsurface = color(material, -420, 200, 0.22, 0.10, 0.07)
    mel.connect_material_property(subsurface, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    opacity = expression(material, unreal.MaterialExpressionScalarParameter, -420, 320,
                         parameter_name="ScatterAmount", default_value=0.14)
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

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
