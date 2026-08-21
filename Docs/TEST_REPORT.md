# 测试报告

测试日期：2026-08-21。以下只记录实际执行并观察到的结果。

## 环境

- Unreal Engine：5.3.2（CL 29314046）
- Editor target：`DriftsteadEditor Win64 Development`
- Game target：`Driftstead Win64 Development`
- 编译器：Visual Studio 2022 / MSVC 14.36.32546
- Windows SDK：10.0.22621

## 最终结果

| 验证 | 结果 | 证据 |
| --- | --- | --- |
| Editor 编译 | PASS | `BuildEditor-20260821-192847.log` |
| Python 内容生成与校验 | PASS | 31 个必需资产；两个 DataTable 各 10 行；设施解锁/升级成本完整；导入 `0 Problems`；`ContentValidation.json` 为 `passed: true` |
| C++ 自动化测试 | PASS 22/22 | `AutomationTests.log`；新增背包缩容保全物品用例 |
| 编辑器运行时烟测 | PASS | `RuntimeSmoke.log`；800 cm Z 高差平面捕获、雨水桶计时产出均通过 |
| 截图采集 | PASS 6/6 | `Artifacts/Screenshots`；木筏、钩爪和漂流物已显示新 PBR 材质 |
| Win64 Build/Cook/Stage/Pak/Archive | PASS | `PackageWin64-20260821-193426.log`，最终 Development 包 `BUILD SUCCESSFUL`；Shipping 流程亦于 `PackageWin64-20260821-192323.log` 成功完成 |
| 打包版烟测 | PASS | `PackagedSmoke.log`，从最终归档 exe 重复完整烟测并验证被动设施生产 |
| 打包版 GUI 中文检查 | PASS | 实际观察主菜单、HUD、任务、帮助和背包，无乱码或缺字 |
| F5 保存冲突回归 | PASS | 新打包版实测显示“游戏已保存。”，画面保持 Lit，不再进入 Shader Complexity 棋盘格视图 |

## 覆盖范围

- 抛钩蓄力、发射、命中指定漂流木、回收、冷却和失败路径。
- 背包 First-Fit、旋转、拆分、合并、交换、回收篮消费与回填。
- 背包缩容时的重新布局与溢出回收，确保物品总量不丢失；点击选中后的定向旋转/拆分有明确高亮反馈。
- 资源事务成功/失败原子性、存档输入清理和版本检查。
- 木筏 1–10 级、4 级二层、7 级三层、正反楼梯链接。
- 设施产出与储物柜状态保存；强制降为 1 级后加载，精确恢复 10 级、输出、绳索数量和设施状态。
- Cook 后启动 `Artifacts/Driftstead_Demo_Win64/Windows/Driftstead.exe`，重复完整烟测。
- 钩爪局部胶囊 Sweep 按 X/Y 平面命中，并以 800 cm Z 高差目标验证忽略视觉高度。
- F5 保存与 UE 默认调试键解绑回归；保存提示、中文 HUD 和正常光照画面同步验证。
- 主菜单、暂停面板和背包输入隔离；暂停时世界停止，`Esc` 输入可在暂停状态恢复。
- 损坏存档中的异常楼层、NaN/无限值或远离木筏的坐标会被规范化到安全位置。
- 9 张 CC0 1K PBR 源贴图导入后，木筏实例化网格材质使用标志、贴图绑定和 Cook 均验证通过。

## 日志扫描

最终日志没有 `Fatal error`、`Unhandled Exception`、`Ensure condition failed` 或项目资产缺失。打包版仍可能输出 UE 5.3 的非阻断警告：可选 `Cloud/IoStoreOnDemand.ini` 不存在，以及一个未使用的 Sequencer CoreStyle brush；均不影响本 Demo。

## 尚未执行

- 未做跨硬件正式 60 FPS 基准。
- Shipping Build/Cook/Stage/Archive 已执行，但 Shipping 默认关闭项目日志，未使用当前日志型烟测脚本验证；当前最终 ZIP 为带 prerequisites 且完整烟测通过的 Development 演示包。
- 未做商店发布代码签名或安装器认证。
