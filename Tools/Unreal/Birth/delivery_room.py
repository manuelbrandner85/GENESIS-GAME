# GENESIS: Der Kreislauf des Lebens
# Kreißsaal für L_GEN_Birth: Meshes, Materialien (Master + Instanzen), Licht. Wird von
# setup_birth_scene.py aufgerufen. Maßstab 1 mm = 1 Unreal-Einheit, Ursprung = Ausgang des Kanals.
#
# Licht nach der Geburt, wie es in einem deutschen Kreißsaal üblich ist – hier am Abend: Die
# Deckenfelder sind heruntergedimmt, damit das Kind die Augen öffnet; durch die halb geschlossene
# Jalousie fällt das letzte Dämmerlicht; über dem Kopfende brennt eine warme Wandleuchte. Jede
# Lichtquelle ist im Raum zu sehen.

import unreal

MESHES = "/Game/Genesis/Birth/Meshes"
MATERIALS = "/Game/Genesis/Birth/Materials/Room"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

FLOOR = -800.0
CEILING = FLOOR + 2900.0
LED_PANELS = [(-162.5, -937.5), (-162.5, 937.5), (1087.5, -937.5), (1087.5, 937.5)]
# Achtung Achsen: Der FBX-Weg Blender → Unreal spiegelt Y. Was in build_delivery_room.py bei +Y liegt
# (Fenster), liegt in Unreal bei −Y. Ein erster Aufbau hatte das Tageslicht deshalb an der Wand ohne
# Fenster – ein Licht ohne Herkunft, das die fensterlose Wand grell ausleuchtete.
WINDOW_CENTER = (-400.0, -2300.0, FLOOR + 1550.0)
WINDOW_SIZE = (1350.0, 1250.0)   # Glasfläche ohne Rahmen

# Werkstoffe: (Grundfarbe linear, Rauheit, Rauheitsschwankung, Farbschwankung, metallisch, Rauschmaßstab mm)
# Albedo-Werte aus üblichen Messbereichen: Wandfarbe hell 0,65–0,75, PVC-Boden 0,35–0,45,
# Edelstahl gebürstet F0 ~0,56, Haut 0,35–0,55 (rot höher), Gummi < 0,05.
SURFACES = {
    "Wall": ((0.70, 0.68, 0.64), 0.86, 0.05, 0.035, 0.0, 420.0),
    "WallAccent": ((0.33, 0.39, 0.33), 0.86, 0.05, 0.04, 0.0, 420.0),
    "Floor": ((0.40, 0.38, 0.35), 0.42, 0.12, 0.09, 0.0, 55.0),
    "FloorSeam": ((0.27, 0.26, 0.24), 0.50, 0.05, 0.02, 0.0, 30.0),
    "CeilingTile": ((0.76, 0.76, 0.74), 0.95, 0.03, 0.03, 0.0, 30.0),
    "CeilingGrid": ((0.80, 0.80, 0.78), 0.40, 0.08, 0.02, 0.0, 200.0),
    "WindowFrame": ((0.78, 0.78, 0.76), 0.35, 0.08, 0.02, 0.0, 150.0),
    "WindowSill": ((0.66, 0.64, 0.60), 0.30, 0.10, 0.06, 0.0, 40.0),
    "BlindSlat": ((0.70, 0.70, 0.68), 0.40, 0.08, 0.03, 0.0, 200.0),
    "BlindCord": ((0.80, 0.79, 0.76), 0.92, 0.05, 0.02, 0.0, 10.0),
    "Door": ((0.44, 0.32, 0.21), 0.45, 0.10, 0.10, 0.0, 28.0),
    "Steel": ((0.56, 0.57, 0.58), 0.30, 0.12, 0.03, 1.0, 20.0),
    "Mattress": ((0.10, 0.23, 0.26), 0.35, 0.10, 0.05, 0.0, 80.0),
    "BedPlastic": ((0.78, 0.78, 0.76), 0.40, 0.08, 0.02, 0.0, 120.0),
    "Rubber": ((0.03, 0.03, 0.03), 0.80, 0.10, 0.02, 0.0, 20.0),
    "Blanket": ((0.62, 0.58, 0.50), 0.95, 0.03, 0.06, 0.0, 18.0),
    "Underpad": ((0.12, 0.28, 0.55), 0.90, 0.05, 0.05, 0.0, 12.0),
    "DeviceWhite": ((0.79, 0.79, 0.77), 0.35, 0.08, 0.02, 0.0, 120.0),
    "DeviceGrey": ((0.30, 0.31, 0.32), 0.45, 0.08, 0.03, 0.0, 120.0),
    "MattressInfant": ((0.12, 0.30, 0.45), 0.35, 0.08, 0.04, 0.0, 60.0),
    "Towel": ((0.80, 0.79, 0.76), 1.00, 0.02, 0.03, 0.0, 6.0),
    "Paper": ((0.82, 0.80, 0.74), 0.85, 0.05, 0.03, 0.0, 20.0),
    "ClockFace": ((0.85, 0.85, 0.83), 0.25, 0.05, 0.01, 0.0, 50.0),
    # Ungefärbte Baumwolle. Ein rotbraunes Tuch las sich aus Sicht des Kindes wie ein Arm – Hautfarbe ist hier tabu.
    "RopeCloth": ((0.56, 0.52, 0.45), 0.95, 0.03, 0.06, 0.0, 15.0),
    "BallVinyl": ((0.20, 0.30, 0.45), 0.40, 0.08, 0.04, 0.0, 90.0),
    "StoolVinyl": ((0.10, 0.12, 0.15), 0.45, 0.08, 0.03, 0.0, 40.0),
    "Cabinet": ((0.75, 0.75, 0.72), 0.40, 0.06, 0.02, 0.0, 200.0),
    "Worktop": ((0.35, 0.35, 0.34), 0.35, 0.10, 0.05, 0.0, 30.0),
}

# Selbstleuchtend: (Farbe, Leuchtdichte in cd/m²). Werte aus Lichtstrom und Fläche gerechnet.
EMISSIVE = {
    "Sky": ((0.72, 0.82, 1.10), 320.0),           # Himmel in der Dämmerung, kurz nach Sonnenuntergang
    "LedPanel": ((1.00, 0.96, 0.90), 640.0),      # 3600 lm auf 20 % gedimmt, 0,36 m²: 720/(π·0,36)
    "LampWarm": ((1.00, 0.78, 0.52), 7700.0),     # 800 lm auf 660 × 50 mm Diffusor
    "HeaterGlow": ((1.00, 0.28, 0.08), 25.0),     # Heizstab eines Wärmestrahlers: kaum sichtbar dunkelrot
    "ScreenGreen": ((0.35, 1.00, 0.55), 180.0),   # CTG-Bildschirm
    "ScreenAmber": ((1.00, 0.70, 0.30), 180.0),
}

NOISE_CODE = """
// Wertrauschen über die Weltposition (mm), vier Oktaven. Keine Textur, keine Kachelung.
float3 p = WP / max(Scale, 0.001);
float value = 0.0;
float amplitude = 0.5;
for (int octave = 0; octave < 4; octave++)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = frac(sin(dot(i + float3(0,0,0), float3(12.9898, 78.233, 37.719))) * 43758.5453);
    float n100 = frac(sin(dot(i + float3(1,0,0), float3(12.9898, 78.233, 37.719))) * 43758.5453);
    float n010 = frac(sin(dot(i + float3(0,1,0), float3(12.9898, 78.233, 37.719))) * 43758.5453);
    float n110 = frac(sin(dot(i + float3(1,1,0), float3(12.9898, 78.233, 37.719))) * 43758.5453);
    float n001 = frac(sin(dot(i + float3(0,0,1), float3(12.9898, 78.233, 37.719))) * 43758.5453);
    float n101 = frac(sin(dot(i + float3(1,0,1), float3(12.9898, 78.233, 37.719))) * 43758.5453);
    float n011 = frac(sin(dot(i + float3(0,1,1), float3(12.9898, 78.233, 37.719))) * 43758.5453);
    float n111 = frac(sin(dot(i + float3(1,1,1), float3(12.9898, 78.233, 37.719))) * 43758.5453);
    float nx00 = lerp(n000, n100, f.x);
    float nx10 = lerp(n010, n110, f.x);
    float nx01 = lerp(n001, n101, f.x);
    float nx11 = lerp(n011, n111, f.x);
    value += amplitude * lerp(lerp(nx00, nx10, f.y), lerp(nx01, nx11, f.y), f.z);
    p = p * 2.03 + 17.1;
    amplitude *= 0.5;
}
return value / 0.9375;
"""


def log(message):
    unreal.log("GENESIS: " + message)


def ensure_folder():
    if not eal.does_directory_exist(MATERIALS):
        eal.make_directory(MATERIALS)


def fresh_material(name):
    path = MATERIALS + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    material = asset_tools.create_asset(name, MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    # Alle Raum-Meshes sind Nanite-Meshes. Ohne diese Kennung setzt Unreal in der gebauten Fassung
    # stillschweigend das graue Standardmaterial ein – im Editor fällt das nicht auf.
    material.set_editor_property("used_with_nanite", True)
    return material


def node(material, cls, x, y, **props):
    result = mel.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        result.set_editor_property(key, value)
    return result


def link(src, src_out, dst, dst_in):
    if not mel.connect_material_expressions(src, src_out, dst, dst_in):
        unreal.log_error("GENESIS: Verbindung fehlgeschlagen %s -> %s" % (src.get_name(), dst_in))


def noise_node(material, x, y, world, scale_node):
    custom = node(material, unreal.MaterialExpressionCustom, x, y)
    custom.set_editor_property("code", NOISE_CODE)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    inputs = []
    for name in ("WP", "Scale"):
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        inputs.append(entry)
    custom.set_editor_property("inputs", inputs)
    link(world, "", custom, "WP")
    link(scale_node, "", custom, "Scale")
    return custom


def create_surface_master():
    """
    Ein Master für alle festen Oberflächen im Raum. Keine Fläche ist gleichmäßig: Die Farbe wolkt
    leicht, die Rauheit schwankt – beides aus einem Rauschen über die Weltposition, damit es keine
    Kachelung gibt. Die Instanzen stellen nur Werkstoffwerte ein.
    """
    material = fresh_material("M_GEN_RoomSurface")
    world = node(material, unreal.MaterialExpressionWorldPosition, -1400, 0)
    scale = node(material, unreal.MaterialExpressionScalarParameter, -1400, 150, parameter_name="NoiseScale", default_value=100.0)
    noise = noise_node(material, -1100, 0, world, scale)
    # Zweites, gröberes Rauschen für die Rauheit (andere Struktur als die Farbe)
    scale2 = node(material, unreal.MaterialExpressionMultiply, -1250, 300, const_b=2.7)
    link(scale, "", scale2, "A")
    noise2 = noise_node(material, -1100, 300, world, scale2)

    base = node(material, unreal.MaterialExpressionVectorParameter, -900, -300, parameter_name="BaseColor",
                default_value=unreal.LinearColor(0.7, 0.7, 0.7, 1.0))
    color_var = node(material, unreal.MaterialExpressionScalarParameter, -900, -150, parameter_name="ColorVariation", default_value=0.03)
    centered = node(material, unreal.MaterialExpressionSubtract, -800, 0, const_b=0.5)
    link(noise, "", centered, "A")
    scaled = node(material, unreal.MaterialExpressionMultiply, -650, -100)
    link(centered, "", scaled, "A")
    link(color_var, "", scaled, "B")
    factor = node(material, unreal.MaterialExpressionAdd, -500, -100, const_b=1.0)
    link(scaled, "", factor, "A")
    tinted = node(material, unreal.MaterialExpressionMultiply, -350, -250)
    link(base, "", tinted, "A")
    link(factor, "", tinted, "B")
    mel.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = node(material, unreal.MaterialExpressionScalarParameter, -900, 450, parameter_name="Roughness", default_value=0.5)
    rough_var = node(material, unreal.MaterialExpressionScalarParameter, -900, 600, parameter_name="RoughnessVariation", default_value=0.08)
    centered2 = node(material, unreal.MaterialExpressionSubtract, -800, 300, const_b=0.5)
    link(noise2, "", centered2, "A")
    rough_delta = node(material, unreal.MaterialExpressionMultiply, -650, 400)
    link(centered2, "", rough_delta, "A")
    link(rough_var, "", rough_delta, "B")
    rough_sum = node(material, unreal.MaterialExpressionAdd, -500, 450)
    link(rough, "", rough_sum, "A")
    link(rough_delta, "", rough_sum, "B")
    # Kein Clamp nötig: Rauheit ± Schwankung bleibt in [0, 1], die Engine begrenzt ohnehin
    mel.connect_material_property(rough_sum, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metallic = node(material, unreal.MaterialExpressionScalarParameter, -350, 650, parameter_name="Metallic", default_value=0.0)
    mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def create_emissive_master():
    """Leuchtende Flächen (Himmel, Leuchten, Bildschirme): Leuchtdichte in cd/m² als Parameter."""
    material = fresh_material("M_GEN_RoomEmissive")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    tint = node(material, unreal.MaterialExpressionVectorParameter, -600, 0, parameter_name="Color",
                default_value=unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    luminance = node(material, unreal.MaterialExpressionScalarParameter, -600, 200, parameter_name="Luminance", default_value=100.0)
    product = node(material, unreal.MaterialExpressionMultiply, -300, 100)
    link(tint, "", product, "A")
    link(luminance, "", product, "B")
    mel.connect_material_property(product, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def create_glass():
    """Isolierglas: fast farblos, leicht grünlich an den Kanten, dünn durchscheinend mit Spiegelung."""
    material = fresh_material("M_GEN_RoomGlass")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    material.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    mel.connect_material_property(node(material, unreal.MaterialExpressionConstant3Vector, -400, 0,
                                       constant=unreal.LinearColor(0.82, 0.88, 0.86, 1.0)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(node(material, unreal.MaterialExpressionConstant, -400, 150, r=0.08), "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(node(material, unreal.MaterialExpressionConstant, -400, 250, r=0.03), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(node(material, unreal.MaterialExpressionConstant, -400, 350, r=0.5), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def create_skin():
    """
    Haut der Mutter: streuend (Subsurface), rote Tiefe, leicht fleckig. Sie liegt beim Kind nur
    wenige Zentimeter vor der Linse und ist damit immer unscharf – was bleibt, ist die Farbe und das
    weiche Durchscheinen an den Rändern. Genau daran erkennt man Haut auch verschwommen.
    """
    material = fresh_material("M_GEN_MotherSkin")
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
    world = node(material, unreal.MaterialExpressionWorldPosition, -1100, 0)
    scale = node(material, unreal.MaterialExpressionConstant, -1100, 150, r=22.0)
    noise = noise_node(material, -850, 0, world, scale)
    base = node(material, unreal.MaterialExpressionConstant3Vector, -700, -200, constant=unreal.LinearColor(0.53, 0.35, 0.28, 1.0))
    blotch = node(material, unreal.MaterialExpressionConstant3Vector, -700, -350, constant=unreal.LinearColor(0.56, 0.31, 0.26, 1.0))
    mix = node(material, unreal.MaterialExpressionLinearInterpolate, -450, -250)
    link(base, "", mix, "A")
    link(blotch, "", mix, "B")
    link(noise, "", mix, "Alpha")
    mel.connect_material_property(mix, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(node(material, unreal.MaterialExpressionConstant3Vector, -450, 0,
                                       constant=unreal.LinearColor(0.75, 0.20, 0.12, 1.0)), "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    mel.connect_material_property(node(material, unreal.MaterialExpressionConstant, -450, 150, r=0.45), "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(node(material, unreal.MaterialExpressionConstant, -450, 250, r=0.48), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(node(material, unreal.MaterialExpressionConstant, -450, 350, r=0.35), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def instance(parent, name, scalars, vectors):
    path = MATERIALS + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mi = asset_tools.create_asset(name, MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for key, value in scalars.items():
        mel.set_material_instance_scalar_parameter_value(mi, key, value)
    for key, value in vectors.items():
        mel.set_material_instance_vector_parameter_value(mi, key, unreal.LinearColor(value[0], value[1], value[2], 1.0))
    eal.save_loaded_asset(mi)
    return mi


def build_materials():
    ensure_folder()
    surface = create_surface_master()
    emissive = create_emissive_master()
    materials = {"Glass": create_glass(), "IvBag": None, "Skin": create_skin()}
    materials["IvBag"] = materials["Glass"]
    for name, (color, rough, rough_var, color_var, metallic, scale) in SURFACES.items():
        materials[name] = instance(surface, "MI_GEN_Room_" + name,
                                   {"Roughness": rough, "RoughnessVariation": rough_var, "ColorVariation": color_var,
                                    "Metallic": metallic, "NoiseScale": scale}, {"BaseColor": color})
    for name, (color, luminance) in EMISSIVE.items():
        materials[name] = instance(emissive, "MI_GEN_Room_" + name, {"Luminance": luminance}, {"Color": color})
    log("Raum-Materialien: %d" % len(materials))
    return materials


def assign_materials(mesh, materials):
    missing = []
    for index, slot in enumerate(mesh.get_editor_property("static_materials")):
        name = str(slot.get_editor_property("material_slot_name"))
        material = materials.get(name)
        if material is None:
            missing.append(name)
            continue
        mesh.set_material(index, material)
    eal.save_loaded_asset(mesh)
    if missing:
        unreal.log_warning("GENESIS: ohne Material: %s in %s" % (", ".join(missing), mesh.get_name()))


def spawn_mesh(actors, mesh, label, cast_shadow=True):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0))
    actor.set_actor_label(label)
    component = actor.static_mesh_component
    component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    component.set_static_mesh(mesh)
    component.set_editor_property("cast_shadow", cast_shadow)
    return actor


def rect_light(actors, label, location, rotation, width, height, intensity, units, kelvin, attenuation=6000.0):
    light = actors.spawn_actor_from_class(unreal.RectLight, unreal.Vector(*location))
    light.set_actor_label(label)
    # Ein Rechtecklicht strahlt entlang seiner X-Achse. rotation = (Neigung, Gier) in Grad.
    light.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=rotation[0], yaw=rotation[1]), False)
    component = light.get_component_by_class(unreal.RectLightComponent)
    component.set_editor_property("intensity_units", units)
    component.set_editor_property("intensity", intensity)
    component.set_editor_property("source_width", width)
    component.set_editor_property("source_height", height)
    component.set_editor_property("attenuation_radius", attenuation)
    component.set_editor_property("use_temperature", True)
    component.set_editor_property("temperature", kelvin)
    component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    return light


def build(actors, import_mesh):
    """Raum, Bett, Tücher, Körper, Ausstattung und Licht in die geöffnete Karte setzen."""
    materials = build_materials()
    for asset in ("SM_GEN_DeliveryRoom", "SM_GEN_BirthBed", "SM_GEN_BirthTextiles", "SM_GEN_MotherBody", "SM_GEN_DeliveryEquipment"):
        # Raum und Ausstattung enthalten Durchscheinendes (Glas, Infusionsbeutel). Nanite kann das nicht
        # und zeigt dann das stark vereinfachte Ersatz-Mesh – Hocker und Ball wurden achteckig. Die beiden
        # sind klein (unter 11 000 Flächen), ohne Nanite verliert man nichts.
        mesh = import_mesh(asset + ".fbx", asset, nanite=asset not in ("SM_GEN_DeliveryRoom", "SM_GEN_DeliveryEquipment"))
        if not mesh:
            continue
        if asset in ("SM_GEN_DeliveryRoom", "SM_GEN_DeliveryEquipment"):
            # Ein Neuimport in ein vorhandenes Asset behält dessen alte Nanite-Einstellung – also ausdrücklich aus
            settings = mesh.get_editor_property("nanite_settings")
            settings.set_editor_property("enabled", False)
            mesh.set_editor_property("nanite_settings", settings)
            eal.save_loaded_asset(mesh)
        assign_materials(mesh, materials)
        # Der Himmel vor dem Fenster wirft keinen Schatten – er ist das Licht
        spawn_mesh(actors, mesh, asset.replace("SM_GEN_", ""), cast_shadow=True)

    # Tageslicht durch das Fenster: Die Glasfläche selbst ist die Lichtquelle (Fläche = Fenster),
    # knapp vor der Jalousie, damit deren Lamellen das Licht in Streifen schneiden.
    # Abenddämmerung: Himmel ~320 cd/m² × 1,69 m² Glas ≈ 540 cd, bläulich (8000 K). Viele Geburten sind
    # abends oder nachts; dann tragen die gedimmten Deckenfelder, die warme Wandleuchte und die
    # Bildschirme das Bild. Mit Tageslicht (~6800 cd) war die Wand gegenüber dem Fenster so hell,
    # dass das CTG darin unterging – gemessen: Bildmedian 0,79.
    rect_light(actors, "Daylight_Window", (WINDOW_CENTER[0], WINDOW_CENTER[1] + 5.0, WINDOW_CENTER[2]), (0.0, 90.0, 0.0),
               WINDOW_SIZE[0], WINDOW_SIZE[1], 540.0, unreal.LightUnits.CANDELAS, 8000.0, attenuation=9000.0)

    # Gedimmte LED-Felder (4000 K, je 720 lm)
    for index, (cx, cy) in enumerate(LED_PANELS):
        rect_light(actors, "CeilingPanel_%d" % index, (cx, cy, CEILING - 8.0), (-90.0, 0.0, 0.0),
                   590.0, 590.0, 720.0, unreal.LightUnits.LUMENS, 4000.0)

    # Warme Wandleuchte über dem Kopfende (2700 K, 800 lm), leuchtet in den Raum und an die Decke
    rect_light(actors, "WallLamp_Warm", (-1915.0, 0.0, FLOOR + 1970.0), (10.0, 0.0, 0.0),
               660.0, 50.0, 800.0, unreal.LightUnits.LUMENS, 2700.0)
    log("Kreißsaal gesetzt")
