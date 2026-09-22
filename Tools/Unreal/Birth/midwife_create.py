# GENESIS – die Hebamme als MetaHuman (im offenen Editor ausführen: Konsole `py "<Pfad>"`).
#
# Ein neu angelegter MetaHuman beginnt mit dem Grundgesicht – deutlich anders als das der Mutter – und trägt das
# Standard-Kleidungsstück (Oberteil mit Rundhals und Hose). Genau das trägt eine Hebamme im Kreißsaal:
# Kasack und Hose; die Farbe (Blaugrün) setzt der Szenenaufbau über eine Materialinstanz.
#
# Schritte: 1 anlegen + Haut/Körper setzen, 2 Texturen und Gesichts-Rig aus der Cloud anfordern (nicht blockierend),
# 3 bauen (midwife_build.py, wenn beides fertig ist). Die Quellassets bleiben wie bei der Mutter außerhalb von Git
# (Genesis/Content/Genesis/Characters ist ausgeschlossen, das Repository ist öffentlich).
import unreal

PATH = "/Game/Genesis/Characters/Midwife"
NAME = "MHC_Midwife"
eal = unreal.EditorAssetLibrary
sub = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)

if not eal.does_directory_exist(PATH):
    eal.make_directory(PATH)
full = PATH + "/" + NAME
character = eal.load_asset(full) if eal.does_asset_exist(full) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    NAME, PATH, unreal.MetaHumanCharacter, unreal.MetaHumanCharacterFactoryNew())
sub.try_add_object_to_edit(character)

# Haut: Mitte fünfzig – ein älterer Hauttyp (höherer Texturindex = mehr Falten und Poren im Katalog), etwas
# hellerer, rosigerer Ton als die Mutter, Hände gepflegt, Nägel kurz und ohne Lack (Hygiene im Kreißsaal).
skin = character.get_editor_property("skin_settings")
props = skin.get_editor_property("skin")
props.set_editor_property("u", 0.24)
props.set_editor_property("v", 0.58)
props.set_editor_property("face_texture_index", 140)
props.set_editor_property("body_texture_index", 5)
props.set_editor_property("roughness", 1.1)
props.set_editor_property("show_top_underwear", True)
props.set_editor_property("fingernail_tint_intensity", 0.0)
skin.set_editor_property("skin", props)
res = skin.get_editor_property("desired_texture_sources_resolutions")
R = type(res.get_editor_property("face_albedo"))
for name in ("face_albedo", "face_normal", "face_cavity", "face_animated_maps"):
    res.set_editor_property(name, R.RES4K)
skin.set_editor_property("desired_texture_sources_resolutions", res)
sub.commit_skin_settings(character, skin)

# Körper: 1,66 m, kräftiger – eine Frau, die seit dreißig Jahren im Schichtdienst steht
constraints = sub.get_body_constraints(character)
for c in constraints:
    name = c.get_editor_property("name")
    if name == "Height":
        c.set_editor_property("is_active", True)
        c.set_editor_property("target_measurement", 166.0)
    elif name == "Fat":
        c.set_editor_property("is_active", True)
        c.set_editor_property("target_measurement", 0.6)
sub.set_body_constraints(character, constraints)
try:
    sub.commit_body_state(character)
except Exception as error:
    unreal.log_warning("GENESIS_MIDWIFE Körper: %s" % error)

eal.save_loaded_asset(character)

# Cloud: Texturen (Gesicht 4K – sie ist das Erste, was das Kind aus 30–40 cm sieht) und volles Gesichts-Rig
tex = unreal.MetaHumanCharacterTextureRequestParams()
tex.set_editor_property("blocking", False)
tex.set_editor_property("report_progress", True)
sub.request_texture_sources(character, tex)
rig = unreal.MetaHumanCharacterAutoRiggingRequestParams()
rig.set_editor_property("blocking", False)
rig.set_editor_property("report_progress", True)
rig_enum = type(rig.get_editor_property("rig_type"))
for candidate in ("JOINTS_AND_BLEND_SHAPES", "JOINTS_AND_BLENDSHAPES"):
    if hasattr(rig_enum, candidate):
        rig.set_editor_property("rig_type", getattr(rig_enum, candidate))
        break
sub.request_auto_rigging(character, rig)
unreal.log_warning("GENESIS_MIDWIFE angelegt, Texturen und Rig angefordert: %s" % full)
