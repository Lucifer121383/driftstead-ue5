# 漂海牧场：Driftstead

Unreal Engine 5.3.2 C++ 作品集 Demo：在固定斜俯视的海面上打捞补给，从小木筏开始，取水种植、登上二层，点亮信标完成第一段航程。另保留十级三层木筏供自由扩建与展示。

玩家界面、任务、物品、设施和反馈均为中文；内部稳定 ID 和自动化日志保留英文。

2026-09-11 工程版本：包含回程多物资钩取、前期航程、暂停返回主菜单、背包真实图标、中文清晰度修复和 F1 全资源补给。最新自动化 25/25 通过；完整航程与各轮打包验证的具体范围见[测试报告](Docs/TEST_REPORT.md)。

本机试玩入口：`Artifacts/Driftstead_Demo_Win64/Windows/Driftstead.exe`，本机分发包为 `Artifacts/Driftstead_Demo_Win64.zip`。附带[中文操作说明](Docs/PLAYTEST_GUIDE.md)。GitHub 仓库提供工程源码与素材，不包含此本机可执行包；下载工程后可按下文编译或打包。

## 操作

- `WASD`：相对固定镜头移动
- 鼠标：瞄准
- 按住/松开左键：蓄力抛钩
- 右键或 `Space`：提前收钩
- `E`：交互
- `U`：靠近工作台扩建（与 E 开桶分开）
- `Tab`：打开空间背包
- `R`：旋转物品；`Shift+R`：拆分堆叠；`B`：从临时回收篮取回
- `Esc`：暂停/关闭面板；暂停菜单点击“保存并返回主菜单”或按 `M`
- `F5`：保存航程；主菜单 `N` 新航程、`C/Enter` 继续、`H` 展示
- `F1`：游戏中每按一次，全部 9 种资源各 +100（不占背包格子）
- `F2–F4`、`F6`、`F9`：展示与开发验证快捷键

## 快速开始

```powershell
.\Tools\BuildEditor.ps1
.\Tools\GenerateAssets.ps1
.\Tools\RunTests.ps1
.\Tools\RunSmokeTest.ps1
.\Tools\RunOpeningTest.ps1
.\Tools\OpenEditor.ps1
```

## 获取项目

公开仓库：<https://github.com/Lucifer121383/driftstead-ue5>

```powershell
git clone https://github.com/Lucifer121383/driftstead-ue5.git
cd driftstead-ue5
.\Tools\BuildEditor.ps1
.\Tools\OpenEditor.ps1
```

仓库保留完整源码、Config、Content、Python 自动化、CC0 源贴图和文档。为避免 GitHub 仓库包含数百 MB 的可再生成文件，`Binaries`、`Intermediate`、`Saved`、日志与 Win64 打包产物未提交；运行 `.\Tools\PackageWin64.ps1` 可在本机重新生成 `Artifacts\Driftstead_Demo_Win64\Windows\Driftstead.exe`。

完整架构、自动化和测试证据见 `Docs`。项目协作约束见 `AGENTS.md`。

前期流程见 [航程设计](Docs/OPENING_JOURNEY.md)，模型及音效许可见 [资产来源](Docs/ART_CREDITS.md)。20–30 分钟是本轮节奏目标，不是未经验证的用户留存或游玩时长指标。

Build and verification commands are documented in `AGENTS.md` and `Docs/EDITOR_AUTOMATION.md`.
