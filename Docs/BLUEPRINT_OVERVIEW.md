# Blueprint 概览

生成的 `BP_` 和 `WBP_` 是薄层资产，不包含核心状态机、背包规则、资源事务、存档或木筏等级逻辑。

## Gameplay Blueprint

- `BP_DriftsteadGameMode`、`BP_DriftsteadCharacter`
- `BP_DriftItem`、`BP_DriftItemSpawner`、`BP_Hook`
- `BP_RaftManager`、`BP_Stair`、`BP_Facility`

这些资产继承对应的 C++ 类，为未来替换 Mesh、材质、粒子和可调参数保留入口。

## UI Blueprint

- `WBP_MainMenu`、`WBP_HUD`、`WBP_Inventory`、`WBP_Pause`
- `WBP_Facility`、`WBP_Upgrade`、`WBP_Showcase`、`WBP_Developer`

当前交付的可玩界面由 `ADriftsteadHUD` 以中文 Canvas 绘制，WBP 是已创建、可编译、可继续美术化的 UMG 扩展点。这样 C++ Demo 在没有手工接线的情况下也能完整运行。

## 数据资产

- `DT_Items`：10 种物品，中文显示名与说明、格位、重量、堆叠、稀有度、颜色和漂流参数。
- `DT_RaftLevels`：10 级主题、三层尺寸、设施集合和升级成本。
- `L_Demo`：唯一默认地图；运行时 GameMode 生成海面、角色、漂流物和木筏。
