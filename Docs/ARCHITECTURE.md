# 架构说明

项目将权威玩法状态放在 C++。Blueprint 是原生类的薄子类，DataTable、材质和地图由 Unreal Editor Python 通过合法编辑器 API 生成；没有以文本方式修改 `.uasset` 或 `.umap`。

## 主要运行时模块

- `ADriftsteadCharacter`：输入、移动、楼层、交互、菜单和开发快捷键。
- `UHookComponent` / `AHookActor`：抛钩状态机与回收生命周期；HookActor 使用弱组件引用，切图时主动清理。
- `UInventoryComponent`：空间占用、堆叠、拆分、合并、交换、回收篮和事务资源。
- `ARaftManager`：按 `FRaftLevelDefinition` 生成三层木筏、楼梯和设施，运行时组件 mobility 一致。
- `AFacilityActor`：生产、农田、交易、储物、灯塔胜利反馈和设施级存档。
- `UDriftsteadQuestSubsystem`：线性教程状态与中文任务文本。
- `UDriftsteadGameInstance` / `UDriftsteadSaveGame`：普通、展示和自动化独立槽位，版本迁移与恢复。
- `ADriftsteadHUD`：只读取玩法状态并绘制中文 Canvas UI，不拥有背包或升级规则。

## 数据流

`漂流物 → HookComponent → InventoryComponent → Facility/Raft upgrade → SaveGame`

事件通过接口与委托连接：`ICatchableInterface` 隔离可捕获物，`IInteractableInterface` 统一设施与楼梯，HUD 只消费查询结果和短时反馈。

## 镜头与楼层

正交镜头保持每一级木筏的 2.5D 剪影和格位尺度一致。二、三层不是视觉贴片：它们有独立 Z 高度、碰撞地板、护栏、设施、楼梯链接以及按当前楼层执行的交互过滤。

## C++ / Blueprint 边界

C++ 负责状态机、事务、存档、生成和测试；BP/WBP 只提供可替换的美术与 UI 扩展点。当前可玩 UI 由 C++ Canvas 完整实现，8 个 WBP 是后续 UMG 美术迭代的薄壳。
