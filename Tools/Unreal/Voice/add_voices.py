# GENESIS: Der Kreislauf des Lebens
#
# Gibt der Geburtsszene zwei Stimmen: die des Kindes und die der Mutter.
# Beide erzeugen nichts von selbst – sie folgen der Simulation (schreit das Kind? spricht jemand?).
#
# Aufruf (Editor geschlossen):
#   "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
#       "Genesis\Genesis.uproject" -run=pythonscript -script="Tools\Unreal\Voice\add_voices.py"

import unreal

MAP_PATH = "/Game/Genesis/Birth/Maps/L_GEN_Birth"


def log(message):
    unreal.log("[GENESIS Stimme] %s" % message)


def find_actor(actors, label):
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    return None


def add_voices():
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    if not world:
        log("Karte nicht gefunden: %s" % MAP_PATH)
        return

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Das Kind: Die Stimme sitzt dort, wo das Kind nach der Geburt liegt (Maßstab 1 mm = 1 uu).
    for label, role, location, loudness in (
        ("ChildVoice", unreal.GenesisVoiceRole.NEWBORN, unreal.Vector(160.0, 0.0, 0.0), 1.0),
        ("MotherVoice", unreal.GenesisVoiceRole.MOTHER, unreal.Vector(420.0, 60.0, 160.0), 0.85),
    ):
        actor = find_actor(actors, label)
        if actor is None:
            actor = actors.spawn_actor_from_class(unreal.GenesisVoiceActor, location)
            actor.set_actor_label(label)
            log("Stimme angelegt: %s" % label)
        else:
            actor.set_actor_location(location, False, False)
            log("Stimme aktualisiert: %s" % label)

        actor.set_editor_property("voice_role", role)
        voice = actor.get_editor_property("voice")
        if voice:
            voice.set_editor_property("loudness", loudness)

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    log("Geburtsszene gespeichert: %s" % saved)


add_voices()
