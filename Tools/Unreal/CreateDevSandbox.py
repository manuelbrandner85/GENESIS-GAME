# GENESIS – legt die Entwickler-Sandbox-Map an (idempotent).
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/CreateDevSandbox.py" -unattended -nullrhi

import unreal

SOURCE_MAP = "/Engine/Maps/Templates/Template_Default"
TARGET_MAP = "/Game/Genesis/Maps/L_DevSandbox"
FOLDERS = [
    "/Game/Genesis/Maps",
    "/Game/Genesis/Data",
    "/Game/Genesis/Characters",
    "/Game/Genesis/Environments",
    "/Game/Genesis/Materials",
    "/Game/Genesis/FX",
    "/Game/Genesis/Audio",
    "/Game/Genesis/Cinematics",
    "/Game/Genesis/UI",
]

for folder in FOLDERS:
    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.EditorAssetLibrary.make_directory(folder)
        unreal.log("GENESIS: Ordner angelegt " + folder)

# Im Commandlet ist das Asset Registry für Engine-Inhalte nicht vollständig gescannt
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Engine/Maps/Templates", "/Game/Genesis"], True)

if unreal.EditorAssetLibrary.does_asset_exist(TARGET_MAP):
    unreal.log("GENESIS: " + TARGET_MAP + " existiert bereits")
else:
    duplicated = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, TARGET_MAP)
    if duplicated is None:
        unreal.log_error("GENESIS: Duplizieren fehlgeschlagen: " + SOURCE_MAP)
    else:
        unreal.EditorAssetLibrary.save_asset(TARGET_MAP, only_if_is_dirty=False)
        unreal.log("GENESIS: Map angelegt " + TARGET_MAP)
