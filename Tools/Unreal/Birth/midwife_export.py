# GENESIS – exportiert Körper und Standard-Kleidungsstück der Hebamme (LOD0, mit Skelett) als FBX nach
# ArtSource/Generated/Birth/Midwife – Grundlage für die in Blender simulierte Kasackhose (Tools/Blender/Birth/build_scrub_trousers.py).
# Die Dateien enthalten MetaHuman-Daten und bleiben lokal (ArtSource/Generated ist nicht im Repo).
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Birth/midwife_export.py" -unattended -nosplash
import os
import unreal

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(REPO, "ArtSource", "Generated", "Birth", "Midwife")
os.makedirs(OUT, exist_ok=True)
BUILT = "/Game/Genesis/Characters/Midwife/Built/Midwife"
ASSETS = {
    "Body": BUILT + "/Body/SKM_MHC_Midwife_BodyMesh",
    "Outfit": BUILT + "/Clothing/MHC_Midwife_Outfits",
}
for name, path in ASSETS.items():
    asset = unreal.load_asset(path)
    options = unreal.FbxExportOption()
    options.set_editor_property("level_of_detail", False)
    options.set_editor_property("export_morph_targets", False)
    options.set_editor_property("collision", False)
    options.set_editor_property("fbx_export_compatibility", unreal.FbxExportCompatibility.FBX_2020)
    task = unreal.AssetExportTask()
    task.set_editor_property("object", asset)
    task.set_editor_property("filename", os.path.join(OUT, name + ".fbx"))
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("options", options)
    ok = unreal.Exporter.run_asset_export_task(task)
    unreal.log_warning("GENESIS: Export %s -> %s: %s" % (path, name, ok))

# Im Editor gestartet (-ExecutePythonScript): danach wieder schließen
if os.environ.get("GENESIS_QUIT_AFTER"):
    unreal.SystemLibrary.quit_editor()
