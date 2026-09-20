# GENESIS: Der Kreislauf des Lebens
#
# Gibt den Szenen ihren Klang: die Eileiter-Ampulle, den Mutterleib von innen und den Kreißsaal.
#
# In der Geburtsszene stehen zwei Orte gleichzeitig: Der Mutterleib wird von innen gehört, der
# Kreißsaal von außen – also vor der Geburt durch Bauchdecke und Fruchtwasser gedämpft und
# nach dem ersten Atemzug klar. Denselben Sprung erlebt das Kind.
#
# Aufruf (Editor geschlossen):
#   "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
#       "Genesis\Genesis.uproject" -run=pythonscript -script="Tools\Unreal\WorldSound\add_places.py"

import unreal

BIRTH_MAP = "/Game/Genesis/Birth/Maps/L_GEN_Birth"
OVIDUCT_MAP = "/Game/Genesis/Conception/Maps/L_GEN_OviductAmpulla"


def log(message):
    unreal.log("[GENESIS Ort] %s" % message)


def find_actor(actors, label):
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def place(actors, label, location, place_value, loudness, activity, digestion=0.5, follow_birth=True):
    actor = find_actor(actors, label)
    if actor is None:
        actor = actors.spawn_actor_from_class(unreal.GenesisWorldSoundActor, location)
        actor.set_actor_label(label)
        log("Ort angelegt: %s" % label)
    else:
        log("Ort aktualisiert: %s" % label)

    params = actor.get_editor_property("params")
    params.set_editor_property("place", place_value)
    params.set_editor_property("loudness", loudness)
    params.set_editor_property("activity", activity)
    params.set_editor_property("digestion", digestion)
    actor.set_editor_property("params", params)
    actor.set_editor_property("follow_birth", follow_birth)
    return actor


def build(map_path, entries):
    world = unreal.EditorLoadingAndSavingUtils.load_map(map_path)
    if not world:
        log("Karte nicht gefunden: %s" % map_path)
        return

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for entry in entries:
        place(actors, *entry)

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, map_path)
    log("%s gespeichert: %s" % (map_path, saved))


build(BIRTH_MAP, [
    # Label, Ort, Lautstärke, Betrieb, Verdauung, folgt der Geburt
    ("WombTone", unreal.Vector(0.0, 0.0, 0.0), unreal.GenesisPlace.WOMB, 0.9, 0.3, 0.6, True),
    ("DeliveryRoomTone", unreal.Vector(420.0, 0.0, 200.0), unreal.GenesisPlace.DELIVERY_ROOM, 0.85, 0.5, 0.5, True),
])

build(OVIDUCT_MAP, [
    ("OviductTone", unreal.Vector(0.0, 0.0, 0.0), unreal.GenesisPlace.OVIDUCT_AMPULLA, 0.9, 0.1, 0.5, False),
])
