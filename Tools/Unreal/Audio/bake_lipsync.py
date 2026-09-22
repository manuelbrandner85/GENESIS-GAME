# GENESIS: Der Kreislauf des Lebens
#
# Lippensynchronität: Erzeugt zu jeder Sprachzeile (/Game/Genesis/Audio/Speech/VO_<id>) die Gesichtsspur
# /Game/Genesis/Audio/LipSync/LS_<id> – mit dem StreamingADA-Modell aus der Aufnahme selbst.
# Aufruf (Editor geschlossen):
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Audio/bake_lipsync.py" -unattended -nosplash
# Optional nur einzelne Zeilen: Umgebungsvariable GENESIS_LIPSYNC_ONLY="H_Da,M_Hallo"

import os
import unreal

SPEECH = "/Game/Genesis/Audio/Speech"
TARGET = "/Game/Genesis/Audio/LipSync"
LOOKAHEAD_MS = 80.0

only = [s.strip() for s in os.environ.get("GENESIS_LIPSYNC_ONLY", "").split(",") if s.strip()]
registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path(SPEECH, recursive=False)
done, failed = 0, []
for data in assets:
    name = str(data.asset_name)
    if not name.startswith("VO_"):
        continue
    line = name[3:]
    if only and line not in only:
        continue
    sound = unreal.load_asset(f"{SPEECH}/{name}.{name}")
    if not isinstance(sound, unreal.SoundWave):
        continue
    if unreal.GenesisLipSyncLibrary.bake_lip_sync(sound, f"{TARGET}/LS_{line}", LOOKAHEAD_MS):
        done += 1
    else:
        failed.append(line)
unreal.log_warning(f"LipSync: {done} Spuren erzeugt, fehlgeschlagen: {failed}")
