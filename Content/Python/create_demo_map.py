from __future__ import annotations

import unreal


MAP_PATH = "/Game/Driftstead/Maps/L_Demo"


def main() -> None:
    unreal.EditorAssetLibrary.make_directory("/Game/Driftstead/Maps")
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        if not level_editor.load_level(MAP_PATH):
            raise RuntimeError(f"Failed to load {MAP_PATH}")
    else:
        if not level_editor.new_level(MAP_PATH):
            raise RuntimeError(f"Failed to create {MAP_PATH}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world:
        world_settings = world.get_world_settings()
        world_settings.set_editor_property("force_no_precomputed_lighting", True)
    labels = {actor.get_actor_label() for actor in actor_subsystem.get_all_level_actors()}
    if "PlayerStart" not in labels:
        start = actor_subsystem.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 125), unreal.Rotator())
        start.set_actor_label("PlayerStart")
    level_editor.save_all_dirty_levels()
    unreal.EditorAssetLibrary.save_asset(MAP_PATH, only_if_is_dirty=False)
    unreal.log(f"[Driftstead] Demo map ready: {MAP_PATH}")


if __name__ == "__main__":
    main()
