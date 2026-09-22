# GENESIS – importiert die gesprochenen Sätze (Mutter, Hebamme) als SoundWaves: /Game/Genesis/Audio/Speech/VO_<ID>
#
# Quelle: ArtSource/Generated/Audio/SpeechNormalized (erzeugt mit Tools/Audio/speech_lines.py → kie_audio.py →
# normalize_speech.py). Aufruf headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Audio/import_speech.py" -unattended
import os
import unreal

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SOURCE = os.path.join(REPO, "ArtSource", "Generated", "Audio", "SpeechNormalized")
DESTINATION = "/Game/Genesis/Audio/Speech"
eal = unreal.EditorAssetLibrary

tasks = []
for filename in sorted(os.listdir(SOURCE)):
    if not filename.lower().endswith(".wav"):
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, filename))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", "VO_" + os.path.splitext(filename)[0])
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

count = 0
for task in tasks:
    path = DESTINATION + "/" + task.get_editor_property("destination_name")
    sound = eal.load_asset(path)
    if sound:
        # Sprache wird im Spiel gefiltert (durch die Bauchdecke gedämpft) – keine eigene Raumwirkung
        sound.set_editor_property("looping", False)
        eal.save_loaded_asset(sound)
        count += 1
unreal.log_warning("GENESIS: %d Sprachaufnahmen importiert nach %s" % (count, DESTINATION))

# Klänge (Kind, Raum) – Suno „sounds" (Tools/Audio/sound_lines.py). Importiert werden die WAV-Fassungen
# (Tools/Audio/mp3_to_wav.py → SoundsWav), NICHT die MP3: Unreal 5.8 las die Suno-MP3 nur zu ~45 % – gemessen war
# der Raumklang 6,7 s statt 14,7 s lang, jeder Schrei nach 6 s abgeschnitten. Nur die Namen aus Sounds (v1/v2), die
# v3/v4 sind dieselbe Aufnahme in anderem Behälter.
SOUND_SOURCE = os.path.join(REPO, "ArtSource", "Generated", "Audio", "Sounds")
SOUND_WAV = os.path.join(REPO, "ArtSource", "Generated", "Audio", "SoundsWav")
SOUND_DESTINATION = "/Game/Genesis/Audio/Sounds"
tasks = []
for filename in sorted(os.listdir(SOUND_SOURCE)) if os.path.isdir(SOUND_SOURCE) else []:
    if not filename.lower().endswith(".mp3"):
        continue
    wav = os.path.join(SOUND_WAV, os.path.splitext(filename)[0] + ".wav")
    if not os.path.exists(wav):
        unreal.log_error("GENESIS: WAV fehlt (erst mp3_to_wav.py): %s" % wav)
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", wav)
    task.set_editor_property("destination_path", SOUND_DESTINATION)
    task.set_editor_property("destination_name", os.path.splitext(filename)[0])
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
count = 0
for task in tasks:
    name = task.get_editor_property("destination_name")
    sound = eal.load_asset(SOUND_DESTINATION + "/" + name)
    if sound:
        # Der Raumklang läuft als Schleife, alles andere einmal
        sound.set_editor_property("looping", name.startswith("SFX_Kreisssaal"))
        eal.save_loaded_asset(sound)
        count += 1
unreal.log_warning("GENESIS: %d Klänge importiert nach %s" % (count, SOUND_DESTINATION))
for task in tasks:
    name = task.get_editor_property("destination_name")
    sound = eal.load_asset(SOUND_DESTINATION + "/" + name)
    if sound:
        unreal.log_warning("GENESIS: %s %.2f s" % (name, sound.get_editor_property("duration")))
