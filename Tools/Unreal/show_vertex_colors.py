# GENESIS – Sichtprobe für Vertexfarben: legt eine Kopie einer Karte an, in der die gewählten Actor ihre
# Vertexfarben unbeleuchtet als Farbe zeigen. Damit sieht man im laufenden Spiel, was das Material bekommt.
#
# Die Zahlen dazu liefert Tools/Unreal/genesis_vertex_colors.py; dieses Skript liefert das Bild.
#
# Anlegen:
#   set GENESIS_VC_MAP=/Game/Genesis/Conception/Maps/L_GEN_OviductAmpulla
#   set GENESIS_VC_ACTORS=OviductWall
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/show_vertex_colors.py" -unattended
#   Tools\Build\Capture-GameScreenshot.ps1 -Map <Karte>_VC ...
#
# Aufräumen (Karte und Material wieder löschen – sie gehören nicht ins Spiel):
#   set GENESIS_VC_CLEANUP=1   und dasselbe Skript noch einmal starten.

import os

import unreal

SOURCE_MAP = os.environ.get("GENESIS_VC_MAP", "/Game/Genesis/Conception/Maps/L_GEN_OviductAmpulla")
ACTOR_PREFIX = os.environ.get("GENESIS_VC_ACTORS", "OviductWall")
HIDE_PREFIX = os.environ.get("GENESIS_VC_HIDE", "OviductCilia")
DEBUG_MAP = SOURCE_MAP + "_VC"
DEBUG_MATERIAL = "/Game/Genesis/Debug/M_GEN_VertexColorDebug"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def cleanup():
    for path in (DEBUG_MAP, DEBUG_MATERIAL):
        if eal.does_asset_exist(path):
            eal.delete_asset(path)
            unreal.log("GENESIS: entfernt %s" % path)
    if eal.does_directory_exist("/Game/Genesis/Debug"):
        eal.delete_directory("/Game/Genesis/Debug")


def build():
    folder = DEBUG_MATERIAL.rsplit("/", 1)[0]
    if not eal.does_directory_exist(folder):
        eal.make_directory(folder)
    if eal.does_asset_exist(DEBUG_MATERIAL):
        eal.delete_asset(DEBUG_MATERIAL)
    material = asset_tools.create_asset(DEBUG_MATERIAL.rsplit("/", 1)[1], folder, unreal.Material,
                                        unreal.MaterialFactoryNew())
    # Unbeleuchtet: Was im Bild steht, ist die Vertexfarbe selbst – kein Licht, kein Streuen, keine Deutung
    material.set_editor_property("used_with_nanite", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    color = mel.create_material_expression(material, unreal.MaterialExpressionVertexColor, -300, 0)
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    eal.save_loaded_asset(material)

    unreal.EditorLoadingAndSavingUtils.load_map(SOURCE_MAP)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    touched = 0
    for actor in actors.get_all_level_actors():
        label = actor.get_actor_label()
        if label.startswith(ACTOR_PREFIX):
            actor.static_mesh_component.set_material(0, material)
            touched += 1
        elif HIDE_PREFIX and label.startswith(HIDE_PREFIX):
            # Flimmerhärchen verdecken die Wand; für die Sichtprobe stören sie
            actor.set_actor_hidden_in_game(True)
    if not touched:
        unreal.log_error("GENESIS: kein Actor mit Präfix '%s' in %s" % (ACTOR_PREFIX, SOURCE_MAP))
        return
    unreal.EditorLoadingAndSavingUtils.save_map(unreal.EditorLevelLibrary.get_editor_world(), DEBUG_MAP)
    unreal.log("GENESIS: %d Actor auf das Debugmaterial gesetzt, Karte: %s" % (touched, DEBUG_MAP))


if os.environ.get("GENESIS_VC_CLEANUP"):
    cleanup()
else:
    build()
