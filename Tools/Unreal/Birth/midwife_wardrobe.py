# GENESIS – zieht der Hebamme das Standard-Kleidungsstück an (Kasack und Hose) und wählt Haare, wenn vorhanden.
# Danach midwife_build.py erneut ausführen.
import unreal

character = unreal.EditorAssetLibrary.load_asset("/Game/Genesis/Characters/Midwife/MHC_Midwife")
collection = character.get_editor_property("internal_collection")
instance = collection.get_editor_property("default_instance")
slots = [str(s) for s in collection.get_slot_names()]
unreal.log_warning("GENESIS_WARDROBE Slots: %s" % slots)

registry = unreal.AssetRegistryHelpers.get_asset_registry()
flt = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/MetaHumanCharacterPalette", "MetaHumanWardrobeItem")], recursive_classes=True)
items = [str(d.package_name) for d in registry.get_assets(flt)]
unreal.log_warning("GENESIS_WARDROBE Kleidungs- und Haarteile: %s" % items)


def dress(slot_hint, path):
    item = unreal.EditorAssetLibrary.load_asset(path)
    slot = next((s for s in slots if slot_hint.lower() in s.lower()), None)
    if not item or not slot:
        unreal.log_warning("GENESIS_WARDROBE nicht möglich: %s in %s" % (path, slot))
        return
    result = collection.try_add_item_from_wardrobe_item(slot, item)
    key = result[1] if isinstance(result, tuple) else result
    ok = instance.set_single_slot_selection(slot, key)
    unreal.log_warning("GENESIS_WARDROBE %s -> Slot %s: %s / %s" % (path, slot, result, ok))


dress("outfit", "/MetaHumanCharacter/Optional/Clothing/WI_DefaultGarment")
# Haare: kurz oder zusammengebunden – offenes langes Haar trägt niemand am Kreißbett
for path in items:
    lower = path.lower()
    if "hair" in lower and any(word in lower for word in ("bun", "short", "pony", "updo", "bob")):
        dress("hair", path)
        break
unreal.EditorAssetLibrary.save_loaded_asset(character)
unreal.log_warning("GENESIS_WARDROBE fertig")
