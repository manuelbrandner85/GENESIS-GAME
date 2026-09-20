# GENESIS: Der Kreislauf des Lebens
#
# Setzt die Zahl der Zellen im Schwarm der Eileiter-Ampulle.
#
# Eine Spermienzelle hat 43 120 Dreiecke. Bei 4 500 Zellen sind das rechnerisch 194 Millionen –
# sichtbar ist davon nur ein Bruchteil, weil die Kamera in einem Kanal von 3 mm Länge steht und
# die Engine alles andere verwirft. Gemessen: 138 FPS bei 4 500 Zellen (Frame 7,2 ms, Spiel-Thread
# 5,6 ms, GPU 5,4 ms). Erst ab etwa 8 000 Zellen wird der Spiel-Thread zum Engpass (10,4 ms).
#
# Reduktionsstufen (LODs) wären der nächste Hebel, sind hier aber bewusst nicht gesetzt: Die Geißel
# ist an ihrem Ende 0,12 µm dünn, und eine automatische Reduktion zerlegt genau diese Fläche –
# denselben Fehler („Geißel zerfällt in Punkte") gab es in GENESIS-024 schon einmal.
#
# Aufruf (Editor geschlossen):
#   "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
#       "Genesis\Genesis.uproject" -run=pythonscript -script="Tools/Unreal/Conception/tune_swarm.py"

import os
import unreal

MAP_PATH = "/Game/Genesis/Conception/Maps/L_GEN_OviductAmpulla"
CELL_COUNT = int(os.environ.get("GENESIS_CELL_COUNT", "4500"))


def log(message):
    unreal.log("[GENESIS Schwarm] %s" % message)


def set_cell_count():
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    if not world:
        log("Karte nicht gefunden: %s" % MAP_PATH)
        return

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.GenesisSpermSwarm):
            actor.set_editor_property("cell_count", CELL_COUNT)
            log("Schwarm %s: %d Zellen" % (actor.get_actor_label(), CELL_COUNT))

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH)
    log("Karte gespeichert: %s" % saved)


set_cell_count()
