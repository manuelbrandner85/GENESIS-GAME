# GENESIS – die Fruchthöhle am Ende der vierten Woche (GENESIS-041 Teil 5): der Embryo im Amnion, der Dottersack mit
# seinen Gefäßen, Dottergang und Haftstiel, die ferne Wand der Chorionhöhle. Karte L_GEN_Fruchthoehle.
#
# Ausführung headless (nach setup_embryo.py, das Embryo und Organmaterialien anlegt):
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Embryogenesis/setup_fruchthoehle.py" -unattended
#
# Voraussetzung: Tools/Blender/Embryogenesis/build_fruchthoehle.py (export_all) hat die Umgebung exportiert.
# Umgebungsvariablen: GENESIS_SKIP_IMPORT=1, GENESIS_FOG (Dichte des Nebels in der Chorionhöhle).
#
# Maßstab der Mikrowelt: 1 µm = 1 Unreal-Einheit. Alle Teile liegen im selben Koordinatensystem wie der Embryo.

import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
os.environ["GENESIS_EMBRYO_LIB_ONLY"] = "1"
import setup_embryo as lib  # noqa: E402

eal = lib.eal
mel = lib.mel
MAP_PATH = lib.ROOT + "/Maps/L_GEN_Fruchthoehle"

# Teil -> (Nanite, Material, Übersetzungsname)
ENVIRONMENT = {
    "SM_GEN_Embryo28_Amnion": (False, "M_GEN_Amnion"),
    "SM_GEN_Embryo28_Dottersack": (True, "MI_GEN_Dottersack"),
    "SM_GEN_Embryo28_DottersackGefaesse": (True, "MI_GEN_Dottergefaesse"),
    "SM_GEN_Embryo28_Blutinseln": (True, "MI_GEN_Blutinseln"),
    "SM_GEN_Embryo28_Dottergang": (True, "MI_GEN_Stielgewebe"),
    "SM_GEN_Embryo28_DottergangGefaesse": (True, "MI_GEN_Dottergefaesse"),
    "SM_GEN_Embryo28_Haftstiel": (True, "MI_GEN_Stielgewebe"),
    "SM_GEN_Embryo28_HaftstielGefaesse": (True, "MI_GEN_Dottergefaesse"),
    "SM_GEN_Embryo28_Chorionplatte": (True, "MI_GEN_Chorionplatte"),
    "SM_GEN_Embryo28_ChorionplatteGefaesse": (True, "MI_GEN_Dottergefaesse"),
    "SM_GEN_Embryo28_Chorionhoehle": (True, "MI_GEN_Chorionplatte"),
}

# Weitere Gewebe als Instanzen des Organmaterials (Farbe, Durchschein, Rauheit, Streuung)
ENV_LOOKS = {
    # Dottersack: eine dünne, gelblich-cremefarbene Blase; im Leben durchscheinend, nicht kreidig
    "MI_GEN_Dottersack": ((0.20, 0.165, 0.13), (0.42, 0.30, 0.22), 0.40, 0.95),
    # Blutinseln: frühe Blutbildung – dunkelrote Zellhaufen auf dem Dottersack
    "MI_GEN_Blutinseln": ((0.13, 0.035, 0.03), (0.32, 0.07, 0.05), 0.45, 0.9),
    # Dottergefäße: Blut unter einer dünnen, gallertigen Wand – dunkler und stumpfer als das Blut im Herzen
    "MI_GEN_Dottergefaesse": ((0.07, 0.028, 0.026), (0.24, 0.07, 0.055), 0.42, 0.95),
    # Dottergang und Haftstiel: blasses, gallertiges Bindegewebe
    "MI_GEN_Stielgewebe": ((0.28, 0.22, 0.21), (0.46, 0.30, 0.27), 0.40, 0.95),
    # Chorionplatte (werdende Plazenta), von innen: eine dünne, rosig-weißliche Haut über dem dunkelroten Zottengewebe
    # und den Bluträumen der Mutter in der Gebärmutterwand dahinter
    # Nach den Referenzen (Doc 36): tiefrot, dunkel, nass – nicht hell-bunt. Vorher 0,24/0,10/0,085: Szene mit Sättigung
    # 0,63 gemessen (Cover 0,25).
    "MI_GEN_Chorionplatte": ((0.13, 0.075, 0.068), (0.30, 0.12, 0.10), 0.55, 0.9),
}


def create_env_instances(parent):
    instances = {}
    for name, (colour, through, rough, scatter) in ENV_LOOKS.items():
        path = lib.MATERIALS + "/" + name
        if eal.does_asset_exist(path):
            mi = eal.load_asset(path)
        else:
            mi = lib.asset_tools.create_asset(name, lib.MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, parent)
        mel.set_material_instance_vector_parameter_value(mi, "Farbe", unreal.LinearColor(*colour, 1.0))
        mel.set_material_instance_vector_parameter_value(mi, "Durchschein", unreal.LinearColor(*through, 1.0))
        mel.set_material_instance_scalar_parameter_value(mi, "Rauheit", rough)
        mel.set_material_instance_scalar_parameter_value(mi, "Streuung", scatter)
        eal.save_loaded_asset(mi)
        instances[name] = mi
    return instances


def create_living_shell(shell):
    """
    Die Haut des lebenden Embryos. Das Lookdev (Teil 4) war gegen Blender abgeglichen; die Referenzen (Doc 36) zeigen
    lebendes Gewebe aber rosiger und durchscheinender als jedes Präparat – ein frischer Fetus in intakter Fruchtblase
    ist rosa, die Gefäße schimmern durch. Eigene Instanz, damit das Grundmaterial unverändert bleibt.
    """
    name = "MI_GEN_EmbryoHuelle_Lebend"
    path = lib.MATERIALS + "/" + name
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else lib.asset_tools.create_asset(
        name, lib.MATERIALS, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, shell)
    mel.set_material_instance_vector_parameter_value(mi, "Farbe", unreal.LinearColor(0.80, 0.61, 0.61, 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "Weglaenge", 850.0)
    eal.save_loaded_asset(mi)
    return mi


def create_amnion_material():
    """
    Das Amnion: eine Zellschicht dick, im Fruchtwasser fast unsichtbar (auf beiden Seiten Flüssigkeit, kaum ein
    Brechzahlsprung). Man sieht es nur dort, wo der Blick streifend durch die Haut läuft – als zarter Saum.
    """
    material = lib.fresh_material("M_GEN_Amnion")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    material.set_editor_property("two_sided", False)
    colour = lib.vector_param(material, "Farbe", -600, 0, (0.80, 0.84, 0.88))
    fresnel = lib.expression(material, unreal.MaterialExpressionFresnel, -600, 200)
    fresnel.set_editor_property("exponent", 3.0)
    fresnel.set_editor_property("base_reflect_fraction", 0.0)
    face = lib.scalar_param(material, "DeckungFlaeche", -600, 300, 0.02)
    edge = lib.scalar_param(material, "DeckungSaum", -600, 400, 0.45)
    lerp = lib.expression(material, unreal.MaterialExpressionLinearInterpolate, -300, 250)
    lib.link(face, lerp, "A")
    lib.link(edge, lerp, "B")
    lib.link(fresnel, lerp, "Alpha")
    mel.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(lerp, "", unreal.MaterialProperty.MP_OPACITY)
    mel.connect_material_property(lib.scalar_param(material, "Glanz", -600, 500, 0.2), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.connect_material_property(lib.scalar_param(material, "Rauheit", -600, 600, 0.25), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)
    return material


def build_level(embryo_meshes, env_meshes, shell, organs, env_materials):
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Der Embryo im Szenen-Actor: Er skaliert mit der simulierten Länge und lässt das Herz schlagen
    scene = actors.spawn_actor_from_class(unreal.GenesisEmbryoScene, unreal.Vector(0, 0, 0))
    scene.set_actor_label("EmbryoScene")
    # Bildaufbau der Fruchthöhle: Embryo rechts oben, Dottersack links unten, der Dottergang als Bogen dazwischen.
    # Ein Embryoskop arbeitet weitwinklig und nah – und die Kamera muss in der Chorionhöhle bleiben (Radius 11 mm):
    # 18 mm Brennweite, 12,5 mm Bildbreite, Kamera ~8 mm vom Mittelpunkt. Vorher gegen die Geometrie durchgerechnet.
    scene.set_editor_property("focal_length_mm", 18.0)
    scene.set_editor_property("framing_width_um", 12500.0)
    scene.set_editor_property("focus_offset", unreal.Vector(-2400.0, 400.0, -900.0))
    slots = {
        "SM_GEN_Embryo28_Huelle": ("shell", shell),
        "SM_GEN_Embryo28_Herz": ("heart", organs["MI_GEN_Embryo_Blut"]),
        "SM_GEN_Embryo28_Gefaesse": ("vessels", organs["MI_GEN_Embryo_Blut"]),
        "SM_GEN_Embryo28_Neuralrohr": ("neural_tube", organs["MI_GEN_Embryo_Neuralgewebe"]),
        "SM_GEN_Embryo28_Somiten": ("somites", organs["MI_GEN_Embryo_Somiten"]),
        "SM_GEN_Embryo28_Leberanlage": ("liver", organs["MI_GEN_Embryo_Leberanlage"]),
    }
    for asset, (prop, material) in slots.items():
        comp = scene.get_editor_property(prop)
        comp.set_static_mesh(embryo_meshes[asset])
        comp.set_material(0, material)
    # Hülle zuerst, das Amnion darüber: Beide sind durchscheinend und liegen um denselben Mittelpunkt –
    # ohne Vorgabe wäre die Reihenfolge Zufall
    scene.get_editor_property("shell").set_editor_property("translucency_sort_priority", 0)

    for asset, (nanite, material_name) in ENVIRONMENT.items():
        mesh = env_meshes.get(asset)
        if mesh is None:
            continue
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
        actor.set_actor_label(asset.replace("SM_GEN_Embryo28_", ""))
        comp = actor.static_mesh_component
        comp.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
        comp.set_static_mesh(mesh)
        comp.set_material(0, env_materials[material_name])
        if asset.endswith("Amnion"):
            comp.set_editor_property("translucency_sort_priority", 1)
            comp.set_editor_property("cast_shadow", False)

    # Die Chorionhöhle ist mit Flüssigkeit gefüllt, im Randbereich trüb (Magma reticulare): Das Licht des
    # Embryoskops fällt zum Rand hin ab, die Ferne wird dunkel und kühl
    # Gemessen: Ohne Streuung war der Hintergrund reines Digitalschwarz (0/0/0) – ein CGI-Merkmal. Bei 0,03 hob der
    # Schleier den Hintergrund auf ~50/255 und nahm dem Embryo den Kontrast; 0,012 lässt einen Hauch Lichtkegel stehen.
    # Feine Schwebteilchen streuen kurzwellig stärker: die Albedo leicht kühl.
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -100000))
    fog.set_actor_label("Coelomfluessigkeit")
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_component.set_editor_property("fog_density", float(os.environ.get("GENESIS_FOG", "0.012")))
    fog_component.set_editor_property("fog_height_falloff", 0.00001)
    fog_component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    fog_component.set_editor_property("directional_inscattering_luminance", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
    fog_component.set_editor_property("enable_volumetric_fog", True)
    fog_component.set_editor_property("volumetric_fog_scattering_distribution", 0.55)
    fog_component.set_editor_property("volumetric_fog_albedo", unreal.Color(r=175, g=200, b=240, a=255))

    volume = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    volume.set_actor_label("FruchthoehlePostProcess")
    volume.set_editor_property("unbound", True)
    settings = volume.get_editor_property("settings")
    for name, value in (
        ("bloom_intensity", 0.06),
        ("lens_flare_intensity", 0.0),
        ("scene_fringe_intensity", 0.0),
        ("vignette_intensity", 0.3),
        ("film_grain_intensity", 0.0),
        ("motion_blur_amount", 0.2),
        # Farbabstimmung nach dem Cover (Doc 35: Szene 89 % warm, 0 % kalt, Sättigung 0,63 – Cover 26 / 33 / 0,25):
        # insgesamt etwas weniger Sättigung, die Tiefen leicht kühl. Wie bei der Filmentwicklung – das Leben leuchtet
        # warm von innen, das Dunkel um es herum wird blau-schwarz statt braun.
        ("color_saturation", unreal.Vector4(1.0, 1.0, 1.0, 0.82)),
        ("color_saturation_shadows", unreal.Vector4(1.0, 1.0, 1.0, 0.7)),
        ("color_gain_shadows", unreal.Vector4(0.93, 0.98, 1.10, 1.0)),
        ("color_offset_shadows", unreal.Vector4(0.0, 0.002, 0.006, 0.0)),
    ):
        settings.set_editor_property("override_" + name, True)
        settings.set_editor_property(name, value)
    volume.set_editor_property("settings", settings)

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    lib.log("Karte gespeichert: %s -> %s" % (MAP_PATH, saved))


def main():
    lib.ensure_folders()
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)
    organ_parent = eal.load_asset(lib.MATERIALS + "/M_GEN_EmbryoOrgan") or lib.create_organ_material()
    organs = lib.create_organ_instances(organ_parent)
    shell = create_living_shell(eal.load_asset(lib.MATERIALS + "/M_GEN_EmbryoHuelle") or lib.create_shell_material())
    env_materials = dict(organs)
    env_materials.update(create_env_instances(organ_parent))
    env_materials["M_GEN_Amnion"] = create_amnion_material()

    embryo_meshes = {asset: eal.load_asset(lib.MESHES + "/" + asset) for asset in lib.PARTS}
    env_meshes = {}
    for asset, (nanite, _) in ENVIRONMENT.items():
        if os.environ.get("GENESIS_SKIP_IMPORT"):
            env_meshes[asset] = eal.load_asset(lib.MESHES + "/" + asset)
        else:
            env_meshes[asset] = lib.import_part(asset, nanite)
    build_level(embryo_meshes, env_meshes, shell, organs, env_materials)


main()
