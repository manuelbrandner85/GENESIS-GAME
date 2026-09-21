"""
GENESIS – die Mutter im Kreißsaal (GENESIS-036).

Setzt die MetaHuman-Figur (BP_Mother, gebaut im MetaHuman Creator) halb sitzend ins Entbindungsbett
und stellt den GenesisMotherRig daneben, der ihr Körper-Skelett übernimmt (Haltung, Atem, Blick,
Hände am Kind). Der Platzhalterkörper aus Blender wird ausgeblendet; die Decke aus der
Stoffsimulation bleibt – sie lag schon über diesem Körper.

Geometrie (Kreißsaal-Koordinaten, 1 mm = 1 Einheit, Y gegenüber Blender gespiegelt):
  Rückenteil 45°, Gelenk Sitz/Rücken bei (-450, 0, -40), Sitzfläche Oberkante z = -40.
  Hüftgelenk ~10 cm über der Sitzfläche und so weit vor dem Rückenteil, dass der Rücken
  (13 cm hinter dem Hüftgelenk) am Polster liegt → (-366, 0, 60).

Aufruf im Editor:  py "Tools/Unreal/Birth/mother.py"
"""
import math

import unreal

BIRTH_MAP = "/Game/Genesis/Birth/Maps/L_GEN_Birth"
MOTHER_BP = "/Game/Genesis/Characters/Mother/Built/Mother/BP_Mother"

HINGE = unreal.Vector(-450.0, 0.0, -40.0)
BACK_ANGLE = math.radians(45.0)
# Ihre Achsen in der Welt: vorn = Normale des Rückenteils, oben = das Rückenteil hinauf
FORWARD = unreal.Vector(math.sin(BACK_ANGLE), 0.0, math.cos(BACK_ANGLE))
UP = unreal.Vector(-math.cos(BACK_ANGLE), 0.0, math.sin(BACK_ANGLE))
# Hüftgelenk (Mitte zwischen den Oberschenkelköpfen) in der Referenzpose der Figur, cm
HIP_LOCAL_CM = unreal.Vector(0.0, 2.3, 84.0)
SEAT_HIP_HEIGHT_MM = 100.0
BACK_BEHIND_HIP_MM = 130.0
SCALE = 10.0   # Figur in cm, Szene in mm


def log(message):
    unreal.log("GENESIS Mutter: " + message)


def hip_world():
    """Hüftgelenk: SEAT_HIP_HEIGHT über der Sitzfläche, BACK_BEHIND_HIP vor der Ebene des Rückenteils."""
    z = HINGE.z + SEAT_HIP_HEIGHT_MM
    # (p - HINGE) · FORWARD = BACK_BEHIND_HIP  →  x auflösen (y = 0)
    x = HINGE.x + (BACK_BEHIND_HIP_MM - (z - HINGE.z) * FORWARD.z) / FORWARD.x
    return unreal.Vector(x, 0.0, z)


def body_world_transform():
    rotation = unreal.MathLibrary.make_rot_from_yz(FORWARD, UP)
    hip_offset = rotation.quaternion().rotate_vector(HIP_LOCAL_CM * SCALE)
    location = hip_world() - hip_offset
    return unreal.Transform(location, rotation, unreal.Vector(SCALE, SCALE, SCALE))


def find_component(actor, prefix):
    for component in actor.get_components_by_class(unreal.SkeletalMeshComponent):
        if component.get_name().startswith(prefix):
            return component
    return None


def build(actors=None):
    actors = actors or unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors.get_all_level_actors():
        label = actor.get_actor_label()
        if label in ("Mother", "MotherRig"):
            actors.destroy_actor(actor)

    blueprint = unreal.EditorAssetLibrary.load_blueprint_class(MOTHER_BP)
    if not blueprint:
        log("BP_Mother fehlt – erst im MetaHuman Creator zusammensetzen")
        return None
    mother = actors.spawn_actor_from_class(blueprint, unreal.Vector(0.0, 0.0, 0.0))
    mother.set_actor_label("Mother")

    body = find_component(mother, "Body")
    if not body:
        log("Körperkomponente nicht gefunden")
        return None
    # Die Figur so stellen, dass ihr Körper (nicht die Wurzel des Blueprints) im Bett liegt
    relative = body.get_relative_transform()
    actor_world = relative.inverse().multiply(body_world_transform())
    mother.set_actor_transform(actor_world, False, False)

    rig = actors.spawn_actor_from_class(unreal.GenesisMotherRig, hip_world())
    rig.set_actor_label("MotherRig")
    rig.set_editor_property("mother_actor", mother)

    # Der Platzhalterkörper aus GENESIS-035 hat seinen Dienst getan: Die Decke liegt, die Figur ist da
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == "MotherBody":
            actor.set_actor_hidden_in_game(True)
            actor.set_is_temporarily_hidden_in_editor(True)

    hip = hip_world()
    log("gesetzt – Hüfte (%.0f, %.0f, %.0f) mm, Körper %s" % (hip.x, hip.y, hip.z, body.get_world_location()))
    return mother, rig


if __name__ == "__main__":
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorLevelLibrary.get_editor_world().get_path_name().split(".")[0] != BIRTH_MAP:
        level.load_level(BIRTH_MAP)
    build()
    level.save_current_level()
