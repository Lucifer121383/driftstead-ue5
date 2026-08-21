# 编辑器自动化

所有 PowerShell 脚本从 `.uproject` 和注册表/Launcher manifest 解析真实 UE 安装目录，不硬编码或猜测版本。每次 C++ 编译前，公共脚本只关闭命令行明确指向本项目的 Unreal 进程。

| 脚本 | 用途 |
| --- | --- |
| `BuildEditor.ps1` | 关闭锁定进程并编译 Editor target |
| `OpenEditor.ps1` | 启动编辑器并打开 `L_Demo` |
| `RunTests.ps1` | 运行 `Driftstead.` 自动化测试并校验发现/完成数量 |
| `RunGame.ps1` | 用 Editor `-game` 启动可玩 Demo |
| `PackageWin64.ps1` | 干净归档 Build/Cook/Stage/Pak/Prerequisites |
| `GenerateAssets.ps1` | 执行 Python bootstrap，生成并校验资产 |
| `RunSmokeTest.ps1` | 编辑器游戏模式的端到端烟测 |
| `RunPackagedSmoke.ps1` | 启动最终归档 exe 并重复烟测 |
| `CaptureScreenshots.ps1` | 离屏生成并验证 6 张演示截图 |
| `CleanProject.ps1` | 清理项目可再生构建目录 |
| `UECommon.ps1` | 路径解析、日志、进程隔离等共享函数 |

`Content/Python/bootstrap_project.py` 依次调用材质、Blueprint、DataTable、地图与内容校验脚本。生成器可重复运行；现有资产会验证或更新，DataTable 导入问题和中文字段缺失会让命令失败。
