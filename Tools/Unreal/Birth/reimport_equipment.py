# GENESIS: Der Kreißsaal – nur die Ausstattung (SM_GEN_DeliveryEquipment) und die Raum-Materialien neu übernehmen,
# ohne die ganze Szene neu aufzubauen. Vorher: Tools/Blender/Birth/build_delivery_room.py -- --export
#   UnrealEditor-Cmd.exe Genesis.uproject -run=pythonscript -script="Tools/Unreal/Birth/reimport_equipment.py" -unattended -nosplash

import os
import sys
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import delivery_room  # noqa: E402

REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
SOURCE = os.path.join(REPO, "ArtSource", "Generated", "Birth")
MESHES = delivery_room.MESHES
eal = unreal.EditorAssetLibrary

unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX false")
options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_as_skeletal", False)
options.set_editor_property("import_materials", False)
options.set_editor_property("import_textures", False)
options.set_editor_property("import_animations", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
data = options.get_editor_property("static_mesh_import_data")
data.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
data.set_editor_property("combine_meshes", True)
data.set_editor_property("auto_generate_collision", False)
data.set_editor_property("generate_lightmap_u_vs", False)
data.set_editor_property("build_nanite", False)
data.set_editor_property("remove_degenerates", False)
data.set_editor_property("convert_scene_unit", True)
data.set_editor_property("compute_weighted_normals", True)

task = unreal.AssetImportTask()
task.set_editor_property("filename", os.path.join(SOURCE, "SM_GEN_DeliveryEquipment.fbx"))
task.set_editor_property("destination_path", MESHES)
task.set_editor_property("destination_name", "SM_GEN_DeliveryEquipment")
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("options", options)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

mesh = eal.load_asset(MESHES + "/SM_GEN_DeliveryEquipment")
materials = delivery_room.build_materials()
delivery_room.assign_materials(mesh, materials)
for material in materials.values():
    if material:
        eal.save_loaded_asset(material)
unreal.log_warning("GENESIS: Ausstattung neu übernommen: %s" % mesh.get_path_name())
