# GENESIS – Augenbrauen, Wimpern, Flaum für die Hebamme (ohne sie wirkt ein Gesicht aus der Nähe künstlich).
import unreal

character = unreal.EditorAssetLibrary.load_asset("/Game/Genesis/Characters/Midwife/MHC_Midwife")
collection = character.get_editor_property("internal_collection")
instance = collection.get_editor_property("default_instance")
registry = unreal.AssetRegistryHelpers.get_asset_registry()
flt = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/MetaHumanCharacterPalette", "MetaHumanWardrobeItem")], recursive_classes=True)
items = sorted(str(d.package_name) for d in registry.get_assets(flt))
for slot, folder, prefer in (("Eyebrows", "/Eyebrows/", ("natural", "thin", "_m_", "_s_")),
                             ("Eyelashes", "/Eyelashes/", ("natural", "_m_", "_s_")),
                             ("Peachfuzz", "/Peachfuzz/", ("",))):
    options = [p for p in items if folder.lower() in p.lower()]
    unreal.log_warning("GENESIS_BROWS %s: %s" % (slot, [p.split("/")[-1] for p in options]))
    choice = next((p for word in prefer for p in options if word in p.lower()), options[0] if options else None)
    if choice:
        item = unreal.EditorAssetLibrary.load_asset(choice)
        result = collection.try_add_item_from_wardrobe_item(slot, item)
        key = result[1] if isinstance(result, tuple) else result
        instance.set_single_slot_selection(slot, key)
        unreal.log_warning("GENESIS_BROWS gewählt %s -> %s" % (slot, choice.split("/")[-1]))
unreal.EditorAssetLibrary.save_loaded_asset(character)
