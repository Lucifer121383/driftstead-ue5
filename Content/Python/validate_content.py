from __future__ import annotations

import json
from pathlib import Path

import unreal


REQUIRED_ASSETS = [
    "/Game/Driftstead/Maps/L_Demo",
    "/Game/Driftstead/Data/DT_Items",
    "/Game/Driftstead/Data/DT_RaftLevels",
    *[f"/Game/Driftstead/Materials/{name}" for name in ("M_Water","M_WoodLight","M_WoodDark","M_Metal","M_Rope","M_Cloth","M_Character","M_Crop","M_InteractionHighlight","M_RareItem","M_TransparentFloor")],
    *[f"/Game/Driftstead/Textures/CC0/T_{name}" for name in ("Wood_D","Wood_N","Wood_ARM","Metal_D","Metal_N","Metal_ARM","Fabric_D","Fabric_N","Fabric_ARM")],
    "/Game/Driftstead/Blueprints/Characters/BP_DriftsteadCharacter",
    "/Game/Driftstead/Blueprints/Gameplay/BP_DriftItem",
    "/Game/Driftstead/Blueprints/Gameplay/BP_Hook",
    "/Game/Driftstead/Blueprints/Raft/BP_RaftManager",
    "/Game/Driftstead/Blueprints/Raft/BP_Stair",
    "/Game/Driftstead/Blueprints/Facilities/BP_Facility",
    "/Game/Driftstead/UI/WBP_HUD",
    "/Game/Driftstead/UI/WBP_Inventory",
]


def main() -> None:
    missing = [asset for asset in REQUIRED_ASSETS if not unreal.EditorAssetLibrary.does_asset_exist(asset)]
    table_errors = []
    expected_rows = {"DT_Items": 10, "DT_RaftLevels": 10}
    expected_chinese_columns = {
        "DT_Items": ("DisplayName", {"漂流木捆", "绳索卷", "废铁块", "布料卷", "种子箱", "食物箱", "密封木桶", "机械箱", "电子元件盒", "动物运输箱"}),
        "DT_RaftLevels": ("ThemeName", {"漂流木台", "拾荒甲板", "萌芽木筏", "高脚谷仓", "海上家园", "风帆牧场", "潮汐农塔", "蓝海庄园", "方舟牧区", "海上绿洲城"}),
    }
    exported_tables = {}
    for table_name, expected_count in expected_rows.items():
        table = unreal.load_asset(f"/Game/Driftstead/Data/{table_name}")
        if not table:
            table_errors.append(f"{table_name}: could not load")
            continue
        row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
        exported_tables[table_name] = len(row_names)
        if len(row_names) != expected_count:
            table_errors.append(f"{table_name}: expected {expected_count} rows, found {len(row_names)}")
        column_name, expected_values = expected_chinese_columns[table_name]
        actual_values = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, column_name)
        serialized_column = "\n".join(actual_values)
        missing_values = sorted(value for value in expected_values if value not in serialized_column)
        if missing_values:
            table_errors.append(f"{table_name}.{column_name}: missing Chinese values {missing_values}")
        if table_name == "DT_RaftLevels":
            facilities = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "Facilities")
            upgrade_costs = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(table, "UpgradeCost")
            serialized_facilities = "\n".join(facilities)
            serialized_costs = "\n".join(upgrade_costs)
            required_facilities = ("Workbench", "RainBarrel", "FarmPlot", "WindTurbine", "Lighthouse")
            required_cost_resources = ("Wood", "Rope", "Metal", "Parts")
            missing_facilities = [value for value in required_facilities if value not in serialized_facilities]
            missing_cost_resources = [value for value in required_cost_resources if value not in serialized_costs]
            if len(facilities) != expected_count or missing_facilities:
                table_errors.append(f"{table_name}.Facilities: incomplete progression data; missing {missing_facilities}")
            if len(upgrade_costs) != expected_count or missing_cost_resources:
                table_errors.append(f"{table_name}.UpgradeCost: incomplete progression data; missing {missing_cost_resources}")
    report = {
        "required_asset_count": len(REQUIRED_ASSETS),
        "missing": missing,
        "data_table_rows": exported_tables,
        "table_errors": table_errors,
        "passed": not missing and not table_errors,
    }
    report_root = Path(unreal.Paths.project_dir()) / "Artifacts" / "Logs"
    report_root.mkdir(parents=True, exist_ok=True)
    report_path = report_root / "ContentValidation.json"
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log(json.dumps(report, indent=2))
    if missing or table_errors:
        raise RuntimeError(f"Content validation failed; see {report_path}")


if __name__ == "__main__":
    main()
