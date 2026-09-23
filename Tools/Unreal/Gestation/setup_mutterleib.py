# GENESIS – der Mutterleib aus Sicht des Kindes (GENESIS-044 Teil 1b): Import, Materialien, Karte L_GEN_Mutterleib.
#
# Ausführung headless (nach Tools/Blender/Gestation/build_mutterleib.py → export_all):
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Gestation/setup_mutterleib.py" -unattended
# Umgebungsvariablen: GENESIS_SKIP_IMPORT=1.
#
# Maßstab: 1 cm = 1 Unreal-Einheit, gebaut für 10 cm Innenradius; AGenesisWombScene skaliert mit der Woche.
# Materialien nach den Referenzen (Docs/36): Wand eine glänzende Haut, durch die vorne Rotlicht dringt; Kindseite der
# Plazenta bläulich-violett durchscheinend, Gefäße erhaben; Nabelschnur weißlich-bläulich, Gefäße dunkel darin.

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
const float Phase[3] = { 0.0, 2.1, 4.2 };
const float Width[3] = { 0.07, 0.07, 0.09 };
float Mask = 0.0;
for (int Index = 0; Index < 3; ++Index)
{
    float Distance = abs(frac(UV.y - Turns * UV.x - Phase[Index] / 6.2831853 + 0.5) - 0.5);
    Mask = max(Mask, 1.0 - smoothstep(0.1 * Width[Index], 1.6 * Width[Index], Distance));
}
return Mask;
"""

PARTS = {
    # Bauteil: (Nanite, Komponente)
    "SM_GEN_Womb_Wall": (True, "wall"),
    "SM_GEN_Womb_Placenta": (True, "placenta"),
    "SM_GEN_Womb_PlacentaVessels": (True, "placenta_vessels"),
    "SM_GEN_Womb_Cord": (True, "cord"),
    "SM_GEN_Womb_CordVessels": (True, "cord_vessels"),
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
    # Nabelgefäße darin: dunkler Schatten in der Sulze
    "MI_GEN_Womb_CordVessels": ((0.14, 0.05, 0.09), (0.30, 0.08, 0.12), 0.3, 0.9, 0.08, 0.0),
}


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
    mel.connect_material_property(lib.vector_param(material, "Farbe", -900, -200, (0.4, 0.3, 0.3)), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(lib.scalar_param(material, "Rauheit", -900, -100, 0.3), "", unreal.MaterialProperty.MP_ROUGHNESS)
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
    mel.connect_material_property(lit, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def build_level(meshes, wall_material, looks):
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    scene = actors.spawn_actor_from_class(unreal.GenesisWombScene, unreal.Vector(0, 0, 0))
    scene.set_actor_label("WombScene")
    materials = {
        "SM_GEN_Womb_Wall": wall_material,
        "SM_GEN_Womb_Placenta": looks["MI_GEN_Womb_Placenta"],
        "SM_GEN_Womb_PlacentaVessels": looks["MI_GEN_Womb_PlacentaVessels"],
        "SM_GEN_Womb_Cord": looks["MI_GEN_Womb_Cord"],
        "SM_GEN_Womb_CordVessels": looks["MI_GEN_Womb_CordVessels"],
    }
    for asset, (_, prop) in PARTS.items():
        component = scene.get_editor_property(prop)
        component.set_static_mesh(meshes[asset])
        component.set_material(0, materials[asset])

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
    build_level(meshes, wall, looks)


main()
