"""Reproducible CC0 meshes, palette materials and animated ocean shader."""
from pathlib import Path
import json
import unreal

ROOT = Path(unreal.Paths.project_dir())
DEST = '/Game/Driftstead/Art'
PROPS = ['barrel', 'chest', 'crate', 'crate-bottles', 'bottle', 'bottle-large',
         'platform-planks', 'structure', 'structure-roof', 'structure-platform',
         'structure-fence', 'boat-row-small', 'mast', 'mast-ropes', 'flag',
         'palm-bend', 'palm-straight', 'rocks-a', 'rocks-b', 'patch-sand',
         'grass-plant', 'grass-patch', 'ship-wreck', 'tool-shovel', 'tool-paddle',
         'tower-complete-small', 'cannon']

def import_file(source, name, options=None):
    existing = unreal.load_asset(f'{DEST}/{name}')
    if existing:
        return existing
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DEST
    task.destination_name = name
    task.automated = True
    task.save = True
    if options:
        task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    result = unreal.load_asset(f'{DEST}/{name}')
    if not result and task.imported_object_paths:
        imported = task.imported_object_paths[0]
        if not unreal.EditorAssetLibrary.rename_asset(imported, f'{DEST}/{name}'):
            raise RuntimeError(f'Could not normalize imported asset name: {imported}')
        result = unreal.load_asset(f'{DEST}/{name}')
    if not result:
        raise RuntimeError(f'Import failed: {source} -> {name}; {task.imported_object_paths}')
    return result

def palette_material(pack, material_name):
    texture_path = ROOT / 'SourceArt' / pack / 'Pack/Models/FBX format/Textures/colormap.png'
    texture = import_file(texture_path, f'T_{material_name}')
    texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
    material = unreal.load_asset(f'{DEST}/M_{material_name}')
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            f'M_{material_name}', DEST, unreal.Material, unreal.MaterialFactoryNew())
        material.set_editor_property('two_sided', True)
        material.set_editor_property('used_with_instanced_static_meshes', True)
        sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -300, 0)
        sample.texture = texture
        unreal.MaterialEditingLibrary.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
        roughness = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 200)
        roughness.r = 0.82
        unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(material)
    # A restrained ambient contribution prevents unreadable black silhouettes
    # in the orthographic demo, without an expensive real-time GI dependency.
    lib = unreal.MaterialEditingLibrary
    lib.delete_all_material_expressions(material)
    sample = lib.create_material_expression(material, unreal.MaterialExpressionTextureSample, -500, 0)
    sample.texture = texture
    lib.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    ambient = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, 140)
    ambient_amount = lib.create_material_expression(material, unreal.MaterialExpressionConstant, -500, 160)
    ambient_amount.r = .14
    lib.connect_material_expressions(ambient_amount, '', ambient, 'B')
    lib.connect_material_expressions(sample, 'RGB', ambient, 'A')
    lib.connect_material_property(ambient, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    roughness = lib.create_material_expression(material, unreal.MaterialExpressionConstant, -200, 250)
    roughness.r = .82
    lib.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(material)
    return material

def make_ocean():
    path = f'{DEST}/M_Ocean'
    mat = unreal.load_asset(path)
    if not mat:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_Ocean', DEST, unreal.Material, unreal.MaterialFactoryNew())
    lib = unreal.MaterialEditingLibrary
    lib.delete_all_material_expressions(mat)
    world = lib.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -700, 0)
    time = lib.create_material_expression(mat, unreal.MaterialExpressionTime, -700, 180)
    custom = lib.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, 0)
    position_input = unreal.CustomInput()
    position_input.set_editor_property('input_name', 'P')
    time_input = unreal.CustomInput()
    time_input.set_editor_property('input_name', 'T')
    custom.set_editor_property('inputs', [position_input, time_input])
    custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property('code', 'float w=sin(P.x*.014+T*.7)*sin(P.y*.019-T*.4); float f=pow(saturate(sin(P.x*.008+P.y*.013+T*.25)),20); return lerp(float3(.018,.12,.17),float3(.045,.29,.32),(w+1)*.5)+f*.04;')
    lib.connect_material_expressions(world, '', custom, 'P')
    lib.connect_material_expressions(time, '', custom, 'T')
    lib.connect_material_property(custom, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 250)
    rough.r = 0.48
    lib.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(mat)

def main():
    unreal.EditorAssetLibrary.make_directory(DEST)
    pirate = palette_material('KenneyPirate', 'PiratePalette')
    character = palette_material('KenneyCharacters', 'CharacterPalette')
    entries = [(ROOT / 'SourceArt/KenneyPirate/Pack/Models/FBX format' / (name + '.fbx'), 'S_' + name.replace('-', '_'), pirate) for name in PROPS]
    entries.append((ROOT / 'SourceArt/KenneyCharacters/Pack/Models/FBX format/character-male-a.fbx', 'S_Sailor', character))
    report = []
    for source, name, material in entries:
        options = unreal.FbxImportUI()
        options.set_editor_property('import_mesh', True)
        options.set_editor_property('import_as_skeletal', False)
        options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
        options.set_editor_property('automated_import_should_detect_type', False)
        options.set_editor_property('import_materials', False)
        options.set_editor_property('import_textures', False)
        options.static_mesh_import_data.set_editor_property('combine_meshes', True)
        options.static_mesh_import_data.set_editor_property('auto_generate_collision', True)
        mesh = import_file(source, name, options)
        for slot in range(max(1, len(mesh.static_materials))):
            mesh.set_material(slot, material)
        box = mesh.get_bounding_box()
        report.append({'mesh': name, 'min': str(box.min), 'max': str(box.max)})
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    make_ocean()
    unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=True, recursive=True)
    (ROOT / 'Artifacts/ArtImportReport.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    unreal.log(f'[Driftstead] CC0 art ready: {len(entries)} real meshes and ocean material')

if __name__ == '__main__':
    main()
