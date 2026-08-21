from __future__ import annotations

import unreal


MATERIALS = {
    "M_Water": ((0.02, 0.42, 0.56, 1.0), 0.20, 0.05),
    "M_WoodLight": ((0.72, 0.42, 0.14, 1.0), 0.78, 0.0),
    "M_WoodDark": ((0.30, 0.13, 0.04, 1.0), 0.85, 0.0),
    "M_Metal": ((0.22, 0.31, 0.36, 1.0), 0.38, 0.55),
    "M_Rope": ((0.68, 0.46, 0.20, 1.0), 0.92, 0.0),
    "M_Cloth": ((0.88, 0.55, 0.26, 1.0), 0.88, 0.0),
    "M_Crop": ((0.20, 0.68, 0.16, 1.0), 0.76, 0.0),
    "M_InteractionHighlight": ((1.0, 0.68, 0.12, 1.0), 0.35, 0.0),
    "M_RareItem": ((0.18, 0.72, 0.92, 1.0), 0.24, 0.22),
    "M_TransparentFloor": ((0.18, 0.38, 0.44, 0.38), 0.55, 0.0),
}


def _create_material(name: str, color, roughness: float, metallic: float):
    path = f"/Game/Driftstead/Materials/{name}"
    existing = unreal.load_asset(path)
    if existing:
        return existing

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(name, "/Game/Driftstead/Materials", unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError(f"Failed to create material {path}")

    color_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -420, -40)
    color_node.set_editor_property("parameter_name", "BaseColor")
    color_node.set_editor_property("default_value", unreal.LinearColor(*color))
    unreal.MaterialEditingLibrary.connect_material_property(color_node, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -420, 90)
    rough_node.set_editor_property("parameter_name", "Roughness")
    rough_node.set_editor_property("default_value", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -420, 190)
    metal_node.set_editor_property("parameter_name", "Metallic")
    metal_node.set_editor_property("default_value", metallic)
    unreal.MaterialEditingLibrary.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)

    if name in {"M_InteractionHighlight", "M_RareItem"}:
        unreal.MaterialEditingLibrary.connect_material_property(color_node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name == "M_TransparentFloor":
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        opacity_node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -420, 290)
        opacity_node.set_editor_property("parameter_name", "Opacity")
        opacity_node.set_editor_property("default_value", color[3])
        unreal.MaterialEditingLibrary.connect_material_property(opacity_node, "", unreal.MaterialProperty.MP_OPACITY)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def main() -> None:
    unreal.EditorAssetLibrary.make_directory("/Game/Driftstead/Materials")
    for name, (color, roughness, metallic) in MATERIALS.items():
        _create_material(name, color, roughness, metallic)
    unreal.log(f"[Driftstead] Materials ready: {len(MATERIALS)}")


if __name__ == "__main__":
    main()
