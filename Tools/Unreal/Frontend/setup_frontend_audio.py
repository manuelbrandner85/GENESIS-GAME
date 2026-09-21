# GENESIS: Der Kreislauf des Lebens
# Importiert die Töne des Spielrahmens nach /Game/Genesis/Frontend/Audio (kopflos, idempotent).
#
# Quellen: ArtSource/Generated/Frontend/Audio (erzeugt von Tools/Frontend/generate_frontend_audio_kie.py
# und Tools/Frontend/generate_ui_sounds.py; die Erzählersätze stammen aus den Trailer-Aufnahmen).
#
# Aufruf:
#   UnrealEditor-Cmd.exe Genesis\Genesis.uproject -run=pythonscript -script=Tools\Unreal\Frontend\setup_frontend_audio.py

import os

import unreal

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SOURCE = os.path.join(REPO, "ArtSource", "Generated", "Frontend", "Audio")
DESTINATION = "/Game/Genesis/Frontend/Audio"

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(message):
    unreal.log("GENESIS: " + message)


def import_sound(filename):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE, filename))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", os.path.splitext(filename)[0])
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])
    return eal.load_asset(DESTINATION + "/" + os.path.splitext(filename)[0])


def main():
    if not os.path.isdir(SOURCE):
        unreal.log_error("GENESIS: Quellordner fehlt: " + SOURCE)
        return

    for filename in sorted(os.listdir(SOURCE)):
        if not filename.lower().endswith((".wav", ".mp3")):
            continue
        sound = import_sound(filename)
        if not sound:
            unreal.log_error("GENESIS: Import fehlgeschlagen: " + filename)
            continue
        name = os.path.splitext(filename)[0]
        # Nur das Hauptthema läuft in Schleife – das Menü darf nicht plötzlich verstummen
        sound.set_editor_property("looping", name == "MX_Frontend_Theme")
        # Der Rahmen soll auch in der Pause und unabhängig vom Spielgeschehen hörbar sein
        eal.save_loaded_asset(sound)
        log("Ton %-28s %6.2f s  Schleife %s" % (name, sound.get_editor_property("duration"), sound.get_editor_property("looping")))
    log("Rahmen-Töne importiert")


main()
