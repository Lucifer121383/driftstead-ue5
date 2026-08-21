from __future__ import annotations

import json

import unreal


ITEM_ROWS = [
    {"Name":"Driftwood","ItemId":"Driftwood","DisplayName":"漂流木捆","Description":"风化木材，可用于扩建木筏。","Footprint":{"X":2,"Y":1},"Weight":2,"StackLimit":8,"Rarity":"Common","BaseValue":2,"WorldCollisionSize":{"X":90,"Y":45,"Z":42},"MeshStyle":"Bundle","PrimaryColor":{"R":0.55,"G":0.25,"B":0.08,"A":1},"SecondaryColor":{"R":0.82,"G":0.55,"B":0.22,"A":1},"SpawnWeight":10,"DriftSpeedRange":{"X":38,"Y":62}},
    {"Name":"Rope","ItemId":"Rope","DisplayName":"绳索卷","Description":"建造和修理用的耐盐绳索。","Footprint":{"X":1,"Y":1},"Weight":1,"StackLimit":12,"Rarity":"Common","BaseValue":2,"WorldCollisionSize":{"X":45,"Y":45,"Z":45},"MeshStyle":"Coil","PrimaryColor":{"R":0.68,"G":0.46,"B":0.22,"A":1},"SecondaryColor":{"R":0.95,"G":0.77,"B":0.42,"A":1},"SpawnWeight":9,"DriftSpeedRange":{"X":40,"Y":68}},
    {"Name":"ScrapMetal","ItemId":"ScrapMetal","DisplayName":"废铁块","Description":"制作工具和结构升级所需的金属。","Footprint":{"X":1,"Y":1},"Weight":2,"StackLimit":10,"Rarity":"Common","BaseValue":3,"WorldCollisionSize":{"X":48,"Y":48,"Z":48},"MeshStyle":"Metal","PrimaryColor":{"R":0.22,"G":0.32,"B":0.38,"A":1},"SecondaryColor":{"R":0.58,"G":0.72,"B":0.75,"A":1},"SpawnWeight":8,"DriftSpeedRange":{"X":34,"Y":55}},
    {"Name":"Cloth","ItemId":"Cloth","DisplayName":"布料卷","Description":"可用于床铺、风帆和温室。","Footprint":{"X":2,"Y":1},"Weight":2,"StackLimit":8,"Rarity":"Common","BaseValue":3,"WorldCollisionSize":{"X":86,"Y":44,"Z":44},"MeshStyle":"Roll","PrimaryColor":{"R":0.92,"G":0.62,"B":0.34,"A":1},"SecondaryColor":{"R":0.96,"G":0.86,"B":0.68,"A":1},"SpawnWeight":7,"DriftSpeedRange":{"X":40,"Y":66}},
    {"Name":"SeedCrate","ItemId":"SeedCrate","DisplayName":"种子箱","Description":"装有耐涝作物种子的珍贵木箱。","Footprint":{"X":2,"Y":1},"Weight":2,"StackLimit":6,"Rarity":"Uncommon","BaseValue":5,"WorldCollisionSize":{"X":86,"Y":46,"Z":48},"MeshStyle":"Crate","PrimaryColor":{"R":0.27,"G":0.65,"B":0.25,"A":1},"SecondaryColor":{"R":0.78,"G":0.89,"B":0.35,"A":1},"SpawnWeight":5.5,"DriftSpeedRange":{"X":36,"Y":58}},
    {"Name":"FoodCrate","ItemId":"FoodCrate","DisplayName":"食物箱","Description":"船员与动物所需的耐储食物。","Footprint":{"X":2,"Y":2},"Weight":4,"StackLimit":4,"Rarity":"Uncommon","BaseValue":7,"WorldCollisionSize":{"X":88,"Y":88,"Z":62},"MeshStyle":"Crate","PrimaryColor":{"R":0.8,"G":0.32,"B":0.16,"A":1},"SecondaryColor":{"R":0.95,"G":0.78,"B":0.28,"A":1},"SpawnWeight":4.5,"DriftSpeedRange":{"X":30,"Y":48}},
    {"Name":"SealedBarrel","ItemId":"SealedBarrel","DisplayName":"密封木桶","Description":"可在工作台打开，获得混合物资。","Footprint":{"X":2,"Y":2},"Weight":5,"StackLimit":3,"Rarity":"Uncommon","BaseValue":9,"WorldCollisionSize":{"X":80,"Y":80,"Z":90},"MeshStyle":"Barrel","PrimaryColor":{"R":0.18,"G":0.42,"B":0.58,"A":1},"SecondaryColor":{"R":0.82,"G":0.58,"B":0.19,"A":1},"SpawnWeight":3.5,"DriftSpeedRange":{"X":28,"Y":45}},
    {"Name":"MachineryCrate","ItemId":"MachineryCrate","DisplayName":"机械箱","Description":"高级动力设施所需的沉重零件。","Footprint":{"X":2,"Y":2},"Weight":6,"StackLimit":3,"Rarity":"Rare","BaseValue":12,"WorldCollisionSize":{"X":90,"Y":90,"Z":72},"MeshStyle":"Machinery","PrimaryColor":{"R":0.19,"G":0.26,"B":0.31,"A":1},"SecondaryColor":{"R":0.95,"G":0.57,"B":0.12,"A":1},"SpawnWeight":2.3,"DriftSpeedRange":{"X":24,"Y":40}},
    {"Name":"Electronics","ItemId":"Electronics","DisplayName":"电子元件盒","Description":"导航与自动化所需的干燥电路。","Footprint":{"X":1,"Y":1},"Weight":1,"StackLimit":8,"Rarity":"Rare","BaseValue":14,"WorldCollisionSize":{"X":48,"Y":48,"Z":48},"MeshStyle":"Electronics","PrimaryColor":{"R":0.18,"G":0.72,"B":0.78,"A":1},"SecondaryColor":{"R":0.72,"G":0.95,"B":1,"A":1},"SpawnWeight":2,"DriftSpeedRange":{"X":38,"Y":60}},
    {"Name":"AnimalCrate","ItemId":"AnimalCrate","DisplayName":"动物运输箱","Description":"安全装载着鸡或幼年山羊。","Footprint":{"X":2,"Y":2},"Weight":5,"StackLimit":2,"Rarity":"Epic","BaseValue":18,"WorldCollisionSize":{"X":96,"Y":96,"Z":82},"MeshStyle":"Animal","PrimaryColor":{"R":0.56,"G":0.24,"B":0.62,"A":1},"SecondaryColor":{"R":0.96,"G":0.7,"B":0.95,"A":1},"SpawnWeight":1.2,"DriftSpeedRange":{"X":25,"Y":42}},
]

RAFT_SIZES = [
    ((4,4),(0,0),(0,0)), ((6,5),(0,0),(0,0)), ((8,6),(0,0),(0,0)), ((9,7),(5,4),(0,0)),
    ((11,8),(7,5),(0,0)), ((13,9),(9,6),(0,0)), ((14,10),(10,7),(6,5)), ((16,11),(12,8),(8,6)),
    ((18,12),(14,9),(10,7)), ((20,14),(16,11),(12,9)),
]

RAFT_FACILITIES = [
    ["Workbench"],
    ["Workbench", "RainBarrel"],
    ["Workbench", "RainBarrel", "FarmPlot", "ChickenCoop", "CollectionNet"],
    ["Workbench", "RainBarrel", "FarmPlot", "ChickenCoop", "CollectionNet", "StorageLocker"],
    ["Workbench", "FarmPlot", "ChickenCoop", "StorageLocker", "TradingDock"],
    ["Workbench", "FarmPlot", "ChickenCoop", "StorageLocker", "WindTurbine", "RainBarrel"],
    ["Workbench", "FarmPlot", "StorageLocker", "WindTurbine", "TradingDock"],
    ["Workbench", "RainBarrel", "FarmPlot", "ChickenCoop", "CollectionNet", "StorageLocker", "WindTurbine", "TradingDock", "Desalinator", "AutoCrane"],
    ["Workbench", "RainBarrel", "FarmPlot", "ChickenCoop", "CollectionNet", "StorageLocker", "WindTurbine", "TradingDock", "Desalinator", "AutoCrane"],
    ["Workbench", "RainBarrel", "FarmPlot", "ChickenCoop", "CollectionNet", "StorageLocker", "WindTurbine", "TradingDock", "Desalinator", "AutoCrane", "Lighthouse"],
]


def _upgrade_cost(level: int) -> dict[str, int]:
    if level <= 1:
        return {}
    cost = {"Wood": 4 + level * 3, "Rope": 1 + level}
    if level >= 4:
        cost["Metal"] = level - 2
    if level >= 7:
        cost["Parts"] = level - 5
    return cost


def _ensure_table(name: str, struct_path: str, rows: list[dict]):
    directory = "/Game/Driftstead/Data"
    path = f"{directory}/{name}"
    table = unreal.load_asset(path)
    if not table:
        row_struct = unreal.load_object(None, struct_path)
        if not row_struct:
            raise RuntimeError(f"Row struct not found: {struct_path}")
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", row_struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, directory, unreal.DataTable, factory)
    if not table:
        raise RuntimeError(f"Failed to create Data Table {path}")
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows)):
        raise RuntimeError(f"Failed to fill Data Table {path}")
    unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)


def main() -> None:
    unreal.EditorAssetLibrary.make_directory("/Game/Driftstead/Data")
    _ensure_table("DT_Items", "/Script/Driftstead.DriftItemDefinition", ITEM_ROWS)
    raft_rows = []
    themes = ["漂流木台","拾荒甲板","萌芽木筏","高脚谷仓","海上家园","风帆牧场","潮汐农塔","蓝海庄园","方舟牧区","海上绿洲城"]
    for index, sizes in enumerate(RAFT_SIZES, 1):
        raft_rows.append({
            "Name": f"Level{index}",
            "Level": index,
            "ThemeName": themes[index - 1],
            "FirstFloor": {"X": sizes[0][0], "Y": sizes[0][1]},
            "SecondFloor": {"X": sizes[1][0], "Y": sizes[1][1]},
            "ThirdFloor": {"X": sizes[2][0], "Y": sizes[2][1]},
            "Facilities": RAFT_FACILITIES[index - 1],
            "UpgradeCost": _upgrade_cost(index),
        })
    _ensure_table("DT_RaftLevels", "/Script/Driftstead.RaftLevelDefinition", raft_rows)
    unreal.log("[Driftstead] Data Tables ready: 10 items and 10 raft levels")


if __name__ == "__main__":
    main()
