# 编辑器自动化

所有 PowerShell 脚本从 `.uproject` 和注册表/Launcher manifest 解析真实 UE 安装目录，不硬编码或猜测版本。每次 C++ 编译前，公共脚本只关闭命令行明确指向本项目的 Unreal 进程。

| 脚本 | 用途 |
| --- | --- |
| `BuildEditor.ps1` | 关闭锁定进程并编译 Editor target |
| `OpenEditor.ps1` | 启动编辑器并打开 `L_Demo` |
| `RunTests.ps1` | 运行 `Driftstead.` 自动化测试并校验发现/完成数量 |
| `RunGame.ps1` | 用 Editor `-game` 启动可玩 Demo |
| `PackageWin64.ps1` | 干净归档 Build/Cook/Stage/Pak/Prerequisites；重打包前备份并校验原 Saved，完成后还原玩家进度 |
| `ArchiveDemo.ps1` | 生成经过容器检查的试玩 ZIP，排除测试存档/日志，保留上一版 ZIP 备份 |
| `GenerateAssets.ps1` | 执行 Python bootstrap，生成并校验资产 |
| `GenerateInventoryIcons.ps1` | 专用真实 RHI 场景拍摄 10 个模型图标，再完整校验内容；不能使用 NullRHI 拍摄 |
| `RunMenuTest.ps1` | 暂停/保存、四种钩子状态安全返回、多物资入包、新局与展示继续、10 图标加载；支持 `-Packaged` |
| `RunSmokeTest.ps1` | 编辑器游戏模式的端到端烟测 |
| `RunPackagedSmoke.ps1` | 启动最终归档 exe 并重复烟测 |
| `RunOpeningTest.ps1` | 真实打捞、三次付费升级、45 秒种植、信标结局与备份回退；加 `-Packaged` 验证归档版 |
| `DownloadDemoArt.ps1` | 下载两个 Kenney CC0 资源包，验证固定 SHA-256 后解包 |
| `CaptureScreenshots.ps1` | 离屏生成并验证 6 张演示截图 |
| `CleanProject.ps1` | 清理项目可再生构建目录 |
| `UECommon.ps1` | 路径解析、日志、进程隔离等共享函数 |

`Content/Python/bootstrap_project.py` 依次调用材质、Kenney 导入、原创钩爪/绳索/木梯几何、原创短音效、Blueprint、DataTable、地图与内容校验脚本。生成器可重复运行；现有资产会验证或更新，DataTable 导入问题和中文字段缺失会让命令失败。

`GenerateAssets.ps1` 在基础 bootstrap 之后独立执行 `create_inventory_icons.py`，避免同步 Python 循环阻塞真实渲染帧。图标及预览输出到 `/Game/Driftstead/UI/Icons`、`Artifacts/InventoryIcons`，最终内容校验必须包含 10 个 256×256 的 Texture2D。

自动化统一使用 `Driftstead_Automation` 及其备份槽。手工 GUI 回归应传入 `-DriftsteadManualQA` 使用同样的隔离槽，并与其他测试串行运行。不要在用户的普通航程里执行自动保存测试。
