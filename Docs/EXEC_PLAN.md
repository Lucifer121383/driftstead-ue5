# 执行计划

最终状态：**全部阶段完成（2026-08-21）**

## Phase 0 — 环境与工具链

- [x] 检测到 Unreal Engine 5.3.2：`C:\Program Files\Epic Games\UE_5.3`，没有猜测版本。
- [x] 确认项目模块 `Driftstead`、VS 2022、MSVC 14.36.32546、Windows SDK 10.0.22621。
- [x] 建立 `Tools` 自动化脚本、C++ targets、Config、Docs 和 Git 仓库。
- [x] 编译 `DriftsteadEditor Win64 Development` 并打开 `L_Demo`。

## Phase 1 — 核心垂直切片

- [x] 固定正交 2.5D 镜头、WASD 移动、鼠标瞄准。
- [x] C++ 抛钩状态机：Idle → Charging → Flying → Attached → Returning → Cooldown。
- [x] 漂流物生成、命中、回收、重量限制和落水救援。

## Phase 2 — 物品与空间背包

- [x] 10 种数据驱动物品及中英文无关的稳定内部 ID。
- [x] 6×4 空间背包、First-Fit、旋转、拆分、合并、交换、临时回收篮。
- [x] 资源事务消费、容量边界、损坏存档规范化。

## Phase 3 — 十级木筏与真实楼层

- [x] 1–10 级尺寸、成本、主题和设施配置。
- [x] 4 级开放二层，7 级开放三层；楼层拥有真实碰撞、楼梯和交互过滤。
- [x] 运行时程序化重建与关卡切换验证。

## Phase 4 — 设施、任务与存档

- [x] 工作台、雨水桶、种植田、鸡舍、收集网、储物柜、风力机、自动吊机、交易码头、淡化器和灯塔。
- [x] 教程任务链、普通/展示独立存档、版本 2 存档及版本 1 兼容。
- [x] 设施产出、农田状态、储物柜背包与资源一并保存和恢复。

## Phase 5 — 内容、表现与中文界面

- [x] Python 生成地图、10 个材质、8 个 Blueprint、8 个 WBP、2 个 DataTable。
- [x] 主菜单、HUD、帮助、背包、暂停、任务、设施和反馈全部改为中文。
- [x] 使用 UE 默认复合字体的 CJK fallback；编辑器截图和打包版窗口均无缺字、方框或乱码。

## Phase 6 — 验证与交付

- [x] 内容校验通过：21 个必需资产、10 个物品行、10 个木筏等级行、中文字段齐全。
- [x] 自动化测试 21/21；编辑器运行时烟测通过；6 张截图通过。
- [x] Win64 Build/Cook/Stage/Pak/Prerequisites/Archive 成功。
- [x] 打包版烟测和 GUI 主菜单/HUD/背包验证通过。
- [x] 文档、打包说明、ZIP 与本地 Git 基线完成。
