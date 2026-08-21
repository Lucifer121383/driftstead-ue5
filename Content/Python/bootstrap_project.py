from __future__ import annotations

import importlib
import sys
from pathlib import Path

import unreal


def main() -> None:
    script_root = Path(unreal.Paths.project_content_dir()) / "Python"
    if str(script_root) not in sys.path:
        sys.path.insert(0, str(script_root))

    modules = (
        "create_materials",
        "create_blueprints",
        "create_data_assets",
        "create_demo_map",
        "validate_content",
    )
    for module_name in modules:
        unreal.log(f"[Driftstead] Running {module_name}")
        module = importlib.import_module(module_name)
        importlib.reload(module)
        module.main()

    unreal.EditorAssetLibrary.save_directory("/Game/Driftstead", only_if_is_dirty=False, recursive=True)
    unreal.log("[Driftstead] Bootstrap completed successfully.")


if __name__ == "__main__":
    main()
