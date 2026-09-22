# GENESIS – speichert die gebaute Hebamme und listet die Teile ihres Blueprints (für Kleidung und Rig).
import unreal

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
for data in registry.get_assets_by_path("/Game/Genesis/Characters/Midwife/Built", recursive=True):
    name = str(data.asset_name)
    kind = str(data.asset_class_path.asset_name)
    if kind in ("Blueprint", "SkeletalMesh", "ChaosClothAsset", "ChaosOutfitAsset", "GroomAsset", "MaterialInstanceConstant") and not name.startswith("T_"):
        unreal.log_warning("GENESIS_MIDWIFE_ASSET %s [%s]" % (data.package_name, kind))
bp = unreal.EditorAssetLibrary.load_asset("/Game/Genesis/Characters/Midwife/Built/Midwife/BP_Midwife")
if bp:
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in sub.k2_gather_subobject_data_for_blueprint(bp):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if obj:
            mats = []
            if isinstance(obj, unreal.MeshComponent):
                try:
                    mats = [str(m.get_name()) if m else "-" for m in obj.get_materials()]
                except Exception:
                    pass
            unreal.log_warning("GENESIS_MIDWIFE_COMP %s [%s] %s" % (obj.get_name(), obj.get_class().get_name(), mats))
unreal.log_warning("GENESIS_MIDWIFE_PROBE fertig")
