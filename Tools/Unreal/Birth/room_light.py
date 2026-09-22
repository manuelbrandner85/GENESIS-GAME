# GENESIS – stimmt das Licht des Kreißsaals auf die Bildsprache des Covers ab (Docs/31_Bildsprache.md).
#
# Ausführung headless:
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Birth/room_light.py" -unattended
#
# Gemessen war die Szene: Leuchtdichte 0,28 (genau wie das Cover), aber 11,9 % der Fläche ausgebrannt (die
# Deckenfelder im Blick des Kindes) und praktisch kein kühler Anteil (0,3 % gegen 36 % auf dem Cover).
# Die Abhilfe ist die, die eine Hebamme auch tut: die Deckenfelder herunterdimmen und die warme Wandleuchte
# übernehmen lassen. Das Dämmerlicht am Fenster bleibt die kühle Gegenseite.
#
# Die Werte stehen auch in Tools/Unreal/Birth/delivery_room.py (dort entsteht der Raum von Grund auf).
# Dieses Skript ändert nur die vorhandenen Lichter, damit die Karte sonst unangetastet bleibt.

import unreal

MAP = "/Game/Genesis/Birth/Maps/L_GEN_Birth"

# Label -> (Intensität, Einheit, Kelvin)
LIGHTS = {
    "Daylight_Window": (700.0, unreal.LightUnits.CANDELAS, 8000.0),
    "WallLamp_Warm": (1250.0, unreal.LightUnits.LUMENS, 2700.0),
}
PANEL_LUMENS = 430.0

# Die leuchtenden Flächen selbst (Lumen nutzt sie als Lichtquelle): Leuchtdichte in cd/m², passend zum Lichtstrom
EMISSIVE = {
    "MI_GEN_Room_LedPanel": 382.0,
    "MI_GEN_Room_LampWarm": 12000.0,
    "MI_GEN_Room_Sky": 415.0,
}


def log(message):
    unreal.log_warning("GENESIS: " + message)


for name, luminance in EMISSIVE.items():
    path = "/Game/Genesis/Birth/Materials/Room/" + name
    material = unreal.EditorAssetLibrary.load_asset(path)
    if not material:
        unreal.log_error("GENESIS: Material fehlt: " + path)
        continue
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(material, "Luminance", luminance)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    log("%s: %.0f cd/m²" % (name, luminance))

unreal.EditorLoadingAndSavingUtils.load_map(MAP)
world = unreal.EditorLevelLibrary.get_editor_world()
changed = 0
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RectLight):
    label = actor.get_actor_label()
    component = actor.get_component_by_class(unreal.RectLightComponent)
    if label in LIGHTS:
        intensity, units, kelvin = LIGHTS[label]
    elif label.startswith("CeilingPanel"):
        intensity, units, kelvin = PANEL_LUMENS, unreal.LightUnits.LUMENS, 4000.0
    else:
        continue
    component.set_editor_property("intensity_units", units)
    component.set_editor_property("intensity", intensity)
    component.set_editor_property("temperature", kelvin)
    log("%s: %.0f %s, %.0f K" % (label, intensity, "lm" if units == unreal.LightUnits.LUMENS else "cd", kelvin))
    changed += 1

if changed:
    unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    log("Kreißsaal: %d Leuchten abgestimmt" % changed)
else:
    unreal.log_error("GENESIS: Keine Leuchten gefunden – Karte unverändert")
