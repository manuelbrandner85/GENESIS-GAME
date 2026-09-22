# GENESIS – legt die gemeinsame Farbstimmung des Covers auf alle Szenen (Docs/31_Bildsprache.md).
#
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Frontend/apply_cover_look.py" -unattended
#
# Am Cover gemessen: Die Welt ist kühl (36 % der Fläche), das Lebendige warm (27 %). Deshalb bekommen die Tiefen einen
# Hauch Blau und die Lichter einen Hauch Gold – sehr wenig, sonst wäre es ein Filter statt einer Stimmung. Belichtung,
# Nebel und Lichtfarben bleiben, wie sie in den Szenen gemessen wurden.

import os
import unreal

MAPS = [
    "/Game/Genesis/Conception/Maps/L_GEN_OviductAmpulla",
    "/Game/Genesis/Implantation/Maps/L_GEN_UterineCavity",
    "/Game/Genesis/Birth/Maps/L_GEN_Birth",
]

# (Eigenschaft, Wert) – Vier-Kanal-Werte sind (R, G, B, Gesamt)
GRADE = [
    ("color_gain_shadows", unreal.Vector4(0.96, 0.99, 1.07, 1.0)),
    ("color_gain_highlights", unreal.Vector4(1.05, 1.00, 0.93, 1.0)),
    ("color_saturation", unreal.Vector4(0.98, 0.98, 0.98, 1.0)),
    ("color_contrast", unreal.Vector4(1.02, 1.02, 1.02, 1.0)),
]


def log(message):
    unreal.log_warning("GENESIS: " + message)


def apply_to_map(path):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_error("GENESIS: Karte fehlt: " + path)
        return
    unreal.EditorLoadingAndSavingUtils.load_map(path)
    world = unreal.EditorLevelLibrary.get_editor_world()
    volumes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PostProcessVolume)
    if not volumes:
        unreal.log_error("GENESIS: Kein PostProcessVolume in " + path)
        return
    for volume in volumes:
        settings = volume.get_editor_property("settings")
        for name, value in GRADE:
            settings.set_editor_property("override_" + name, True)
            settings.set_editor_property(name, value)
        volume.set_editor_property("settings", settings)
        log("Farbstimmung auf %s (%s)" % (volume.get_actor_label(), path))
    unreal.EditorLoadingAndSavingUtils.save_map(world, path)


unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Genesis"], True)
for map_path in MAPS:
    apply_to_map(map_path)
