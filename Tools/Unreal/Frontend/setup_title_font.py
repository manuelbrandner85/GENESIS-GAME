# GENESIS – übernimmt die Titelschrift (Cinzel, SIL Open Font License) als Font-Asset für Startbildschirm und Kapitel.
#
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Frontend/setup_title_font.py" -unattended
#
# Die Schrift liegt als Quelle in ArtSource/Fonts/Cinzel (mit OFL.txt). Cinzel ist eine Kapitälchen-Antiqua nach
# römischen Inschriften – dieselbe Anmutung wie der Schriftzug auf dem Cover (Docs/31_Bildsprache.md).

import os
import unreal

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SOURCE = os.path.join(REPO, "ArtSource", "Fonts", "Cinzel", "Cinzel[wght].ttf")
DEST = "/Game/Genesis/UI/Fonts"

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(message):
    unreal.log_warning("GENESIS: " + message)


if not eal.does_directory_exist(DEST):
    eal.make_directory(DEST)

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE)
task.set_editor_property("destination_path", DEST)
task.set_editor_property("destination_name", "FF_GEN_Cinzel")
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
tools.import_asset_tasks([task])
face = eal.load_asset(DEST + "/FF_GEN_Cinzel")
if not face:
    unreal.log_error("GENESIS: Import der Titelschrift fehlgeschlagen: " + SOURCE)
else:
    log("Schriftschnitt: %s (%s)" % (face.get_path_name(), face.get_class().get_name()))
    font_path = DEST + "/F_GEN_Title"
    font = eal.load_asset(font_path)
    if not font:
        font = tools.create_asset("F_GEN_Title", DEST, unreal.Font, unreal.FontFactory())
    # Laufzeit-Schrift: Slate rastert sie in jeder Größe frisch – kein vorgebackener Bildsatz
    font.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)
    data = unreal.FontData()
    data.set_editor_property("font_face_asset", face)
    entry = unreal.TypefaceEntry()
    entry.set_editor_property("name", "Default")
    entry.set_editor_property("font", data)
    typeface = unreal.Typeface()
    typeface.set_editor_property("fonts", [entry])
    composite = unreal.CompositeFont()
    composite.set_editor_property("default_typeface", typeface)
    font.set_editor_property("composite_font", composite)
    eal.save_loaded_asset(font)
    log("Titelschrift: %s" % font.get_path_name())

# Der Schrift-Import braucht die Oberfläche (Slate); headless bricht er ab. Deshalb im vollen Editor starten:
#   UnrealEditor.exe Genesis.uproject -ExecutePythonScript="Tools/Unreal/Frontend/setup_title_font.py"
# mit gesetztem GENESIS_QUIT_AFTER schließt er sich danach wieder.
if os.environ.get("GENESIS_QUIT_AFTER"):
    unreal.SystemLibrary.quit_editor()
