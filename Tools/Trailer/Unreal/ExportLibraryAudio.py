# GENESIS Trailer - exportiert die gesampelten Instrumente der Unreal-Engine-Bibliothek (Harmonix-Plugin)
# als WAV, damit das Soul Theme daraus komponiert/gerendert werden kann.
# Quelle: /Harmonix/Examples/Samples/* (Epic Engine Content, nutzbar in Unreal-Projekten laut UE-EULA).
# Ziel:   <Repo>/Genesis/Saved/TrailerAudio/Library/<Instrument>/<Asset>.wav  (nicht versioniert)

import os
import unreal

INSTRUMENTS = ["Piano", "StringsLegato", "Horns", "Flute", "Vibraphone", "UprightBass", "StringsPizzicato"]
PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUT_ROOT = os.path.join(PROJECT_DIR, "Saved", "TrailerAudio", "Library")

registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(["/Harmonix/Examples/Samples"], True)

exported = 0
failed = 0
for instrument in INSTRUMENTS:
    package_path = "/Harmonix/Examples/Samples/" + instrument
    out_dir = os.path.join(OUT_ROOT, instrument)
    os.makedirs(out_dir, exist_ok=True)
    for asset_path in unreal.EditorAssetLibrary.list_assets(package_path, recursive=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(asset, unreal.SoundWave):
            continue
        task = unreal.AssetExportTask()
        task.object = asset
        task.filename = os.path.join(out_dir, asset.get_name() + ".wav")
        task.automated = True
        task.replace_identical = True
        task.prompt = False
        task.exporter = unreal.SoundExporterWAV()
        if unreal.Exporter.run_asset_export_task(task) and os.path.exists(task.filename):
            exported += 1
        else:
            failed += 1
            unreal.log_warning("GENESIS_TRAILER Export fehlgeschlagen: " + asset_path)

unreal.log("GENESIS_TRAILER exportiert: {} WAV, fehlgeschlagen: {} -> {}".format(exported, failed, OUT_ROOT))
if exported > 0:
    unreal.log("GENESIS_TRAILER_OK")
else:
    unreal.log_error("GENESIS_TRAILER_FAIL keine Samples exportiert")
