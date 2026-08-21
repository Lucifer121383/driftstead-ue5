# 漂海牧场：Driftstead

原创 Unreal Engine 5.3 C++ 作品集 Demo：在明亮的 2.5D 洪水世界中打捞物资，把 4×4 小木筏扩建成 10 级、3 层的海上牧场。

玩家界面、任务、物品、设施和反馈均为中文；内部稳定 ID 和自动化日志保留英文。

## 操作

- `WASD`：相对固定镜头移动
- 鼠标：瞄准
- 按住/松开左键：蓄力抛钩
- 右键或 `Space`：提前收钩
- `E`：交互
- `Tab`：打开空间背包
- `R`：旋转物品；`Shift+R`：拆分堆叠；`B`：从临时回收篮取回
- `Esc`：暂停/关闭面板
- `F1–F6`、`F9`：展示与开发验证快捷键

## 快速开始

```powershell
.\Tools\BuildEditor.ps1
.\Tools\GenerateAssets.ps1
.\Tools\RunTests.ps1
.\Tools\RunSmokeTest.ps1
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

Build and verification commands are documented in `AGENTS.md` and `Docs/EDITOR_AUTOMATION.md`.
