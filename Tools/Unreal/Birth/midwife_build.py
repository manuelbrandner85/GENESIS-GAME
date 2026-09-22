# GENESIS – baut die Hebamme (Kino-Pipeline), sobald Texturen und Rig aus der Cloud da sind.
import unreal

character = unreal.EditorAssetLibrary.load_asset("/Game/Genesis/Characters/Midwife/MHC_Midwife")
sub = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
if not sub.is_object_added_for_editing(character):
    sub.try_add_object_to_edit(character)
unreal.log_warning("GENESIS_MIDWIFE baubar: %s" % sub.can_build_meta_human(character, True))
params = unreal.MetaHumanCharacterEditorBuildParameters()
params.set_editor_property("pipeline_type", unreal.MetaHumanDefaultPipelineType.CINEMATIC)
params.set_editor_property("pipeline_quality", unreal.MetaHumanQualityLevel.CINEMATIC)
params.set_editor_property("name_override", "Midwife")
params.set_editor_property("absolute_build_path", "/Game/Genesis/Characters/Midwife/Built")
params.set_editor_property("common_folder_path", "/Game/Genesis/Characters/Common")
sub.build_meta_human(character, params)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
for data in registry.get_assets_by_path("/Game/Genesis/Characters/Midwife/Built", recursive=True):
    unreal.log_warning("GENESIS_MIDWIFE_BUILT %s [%s]" % (data.package_name, data.asset_class_path.asset_name))
