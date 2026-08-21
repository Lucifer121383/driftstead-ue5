from __future__ import annotations

from pathlib import Path

import unreal


TEXTURE_DIRECTORY = "/Game/Driftstead/Textures/CC0"
SOURCE_DIRECTORY = Path(unreal.Paths.project_dir()) / "SourceArt" / "PolyHaven"

TEXTURE_FILES = {
    "Wood_D": ("T_Wood_D.jpg", "color"),
    "Wood_N": ("T_Wood_N.jpg", "normal"),
    "Wood_ARM": ("T_Wood_ARM.jpg", "masks"),
    "Metal_D": ("T_Metal_D.jpg", "color"),
    "Metal_N": ("T_Metal_N.jpg", "normal"),
    "Metal_ARM": ("T_Metal_ARM.jpg", "masks"),
    "Fabric_D": ("T_Fabric_D.jpg", "color"),
    "Fabric_N": ("T_Fabric_N.jpg", "normal"),
    "Fabric_ARM": ("T_Fabric_ARM.jpg", "masks"),
}

MATERIALS = {
    "M_Water": ((0.02, 0.42, 0.56, 1.0), 0.20, 0.05, None),
    "M_WoodLight": ((0.95, 0.78, 0.56, 1.0), 0.95, 0.0, "Wood"),
    "M_WoodDark": ((0.42, 0.25, 0.12, 1.0), 1.0, 0.0, "Wood"),
    "M_Metal": ((0.72, 0.80, 0.82, 1.0), 0.95, 0.62, "Metal"),
    "M_Rope": ((0.92, 0.72, 0.42, 1.0), 1.0, 0.0, "Fabric"),
    "M_Cloth": ((0.95, 0.62, 0.30, 1.0), 0.92, 0.0, "Fabric"),
    "M_Character": ((1.0, 1.0, 1.0, 1.0), 0.78, 0.0, None),
    "M_Crop": ((0.20, 0.68, 0.16, 1.0), 0.76, 0.0, None),
    "M_InteractionHighlight": ((1.0, 0.68, 0.12, 1.0), 0.35, 0.0, None),
    "M_RareItem": ((0.18, 0.72, 0.92, 1.0), 0.24, 0.22, None),
    "M_TransparentFloor": ((0.18, 0.38, 0.44, 0.38), 0.55, 0.0, None),
}


def _import_texture(asset_name: str, filename: str, texture_kind: str):
    source_file = SOURCE_DIRECTORY / filename
    if not source_file.is_file():
        raise RuntimeError(f"Missing CC0 source texture: {source_file}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_file))
    task.set_editor_property("destination_path", TEXTURE_DIRECTORY)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(f"{TEXTURE_DIRECTORY}/T_{asset_name}")
    if not texture:
        raise RuntimeError(f"Failed to import texture T_{asset_name} from {source_file}")
    if texture_kind == "normal":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif texture_kind == "masks":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def _texture_parameter(material, name: str, texture, x: int, y: int):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, x, y
    )
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", texture)
    return node


def _create_material(name: str, color, roughness: float, metallic: float, texture_set: str | None, textures):
    path = f"/Game/Driftstead/Materials/{name}"
    material = unreal.load_asset(path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, "/Game/Driftstead/Materials", unreal.Material, unreal.MaterialFactoryNew()
        )
    if not material:
        raise RuntimeError(f"Failed to create material {path}")

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("used_with_instanced_static_meshes", name in {"M_WoodLight", "M_WoodDark"})

    color_node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -620, -120
    )
    color_node.set_editor_property("parameter_name", "BaseColor")
    color_node.set_editor_property("default_value", unreal.LinearColor(*color))

    rough_node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -620, 100
    )
    rough_node.set_editor_property("parameter_name", "Roughness")
    rough_node.set_editor_property("default_value", roughness)

    metal_node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -620, 230
    )
    metal_node.set_editor_property("parameter_name", "Metallic")
    metal_node.set_editor_property("default_value", metallic)

    if texture_set:
        diffuse = _texture_parameter(material, "SurfaceTexture", textures[f"{texture_set}_D"], -620, -320)
        tint = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -280, -220
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(diffuse, "RGB", tint, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(color_node, "", tint, "B")
        unreal.MaterialEditingLibrary.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)

        normal = _texture_parameter(material, "SurfaceNormal", textures[f"{texture_set}_N"], -620, 360)
        unreal.MaterialEditingLibrary.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

        arm = _texture_parameter(material, "SurfaceARM", textures[f"{texture_set}_ARM"], -620, 520)
        rough_multiply = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -260, 430
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(arm, "G", rough_multiply, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(rough_node, "", rough_multiply, "B")
        unreal.MaterialEditingLibrary.connect_material_property(
            rough_multiply, "", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            arm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
        )
    else:
        unreal.MaterialEditingLibrary.connect_material_property(
            color_node, "", unreal.MaterialProperty.MP_BASE_COLOR
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS
        )

    unreal.MaterialEditingLibrary.connect_material_property(
        metal_node, "", unreal.MaterialProperty.MP_METALLIC
    )

    if name in {"M_InteractionHighlight", "M_RareItem"}:
        unreal.MaterialEditingLibrary.connect_material_property(
            color_node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
    if name == "M_TransparentFloor":
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        opacity_node = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionScalarParameter, -420, 340
        )
        opacity_node.set_editor_property("parameter_name", "Opacity")
        opacity_node.set_editor_property("default_value", color[3])
        unreal.MaterialEditingLibrary.connect_material_property(
            opacity_node, "", unreal.MaterialProperty.MP_OPACITY
        )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def main() -> None:
    unreal.EditorAssetLibrary.make_directory("/Game/Driftstead/Materials")
    unreal.EditorAssetLibrary.make_directory(TEXTURE_DIRECTORY)
    textures = {
        asset_name: _import_texture(asset_name, filename, texture_kind)
        for asset_name, (filename, texture_kind) in TEXTURE_FILES.items()
    }
    for name, (color, roughness, metallic, texture_set) in MATERIALS.items():
        _create_material(name, color, roughness, metallic, texture_set, textures)
    unreal.log(
        f"[Driftstead] Materials ready: {len(MATERIALS)}; CC0 textures: {len(textures)}"
    )


if __name__ == "__main__":
    main()
