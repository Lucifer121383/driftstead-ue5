from __future__ import annotations

import unreal


BLUEPRINTS = {
    "/Game/Driftstead/Blueprints/Characters": {
        "BP_DriftsteadCharacter": "/Script/Driftstead.DriftsteadCharacter",
    },
    "/Game/Driftstead/Blueprints/Gameplay": {
        "BP_DriftItem": "/Script/Driftstead.DriftItemActor",
        "BP_Hook": "/Script/Driftstead.HookActor",
        "BP_DriftItemSpawner": "/Script/Driftstead.DriftItemSpawner",
    },
    "/Game/Driftstead/Blueprints/Raft": {
        "BP_RaftManager": "/Script/Driftstead.RaftManager",
        "BP_Stair": "/Script/Driftstead.StairActor",
    },
    "/Game/Driftstead/Blueprints/Facilities": {
        "BP_Facility": "/Script/Driftstead.FacilityActor",
    },
    "/Game/Driftstead/Blueprints/Core": {
        "BP_DriftsteadGameMode": "/Script/Driftstead.DriftsteadGameMode",
    },
}

WIDGETS = ("WBP_MainMenu", "WBP_HUD", "WBP_Inventory", "WBP_Pause", "WBP_Upgrade", "WBP_Facility", "WBP_Showcase", "WBP_Developer")


def _create_blueprint(directory: str, name: str, class_path: str) -> None:
    asset_path = f"{directory}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return
    parent_class = unreal.load_class(None, class_path)
    if not parent_class:
        raise RuntimeError(f"Native class not found: {class_path}")
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, directory, unreal.Blueprint, factory)
    if not asset:
        raise RuntimeError(f"Failed to create Blueprint {asset_path}")
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)


def _create_widget(name: str) -> None:
    directory = "/Game/Driftstead/UI"
    asset_path = f"{directory}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.UserWidget)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, directory, unreal.WidgetBlueprint, factory)
    if not asset:
        raise RuntimeError(f"Failed to create Widget Blueprint {asset_path}")
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)


def main() -> None:
    for directory, assets in BLUEPRINTS.items():
        unreal.EditorAssetLibrary.make_directory(directory)
        for name, class_path in assets.items():
            _create_blueprint(directory, name, class_path)
    unreal.EditorAssetLibrary.make_directory("/Game/Driftstead/UI")
    for widget_name in WIDGETS:
        _create_widget(widget_name)
    unreal.log(f"[Driftstead] Thin Blueprints ready: {sum(len(v) for v in BLUEPRINTS.values())}; widgets: {len(WIDGETS)}")


if __name__ == "__main__":
    main()
