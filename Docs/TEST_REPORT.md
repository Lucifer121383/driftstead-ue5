# 测试报告

更新：2026-09-11，菜单、背包图标与中文清晰度修复轮。以下只记录实际结果，不宣称“100% 无 Bug”。

## 最新追加：F1 全资源各 +100

- 游戏中按 F1 发放全部九类资源各 100，沿用 Enhanced Input Started（非每帧 Triggered）；主菜单、暂停及返回主菜单过程中不发放。背包打开可用，不增加格子物品。
- Editor 编译 PASS：BuildEditor-20260911-122745.log。
- 自动化 PASS 25/25：AutomationTestsConsole-20260911-122808.log，新增 SupplyAllNineBy100 检查全部九类非零起点、连续两次精确增量、满背包和回收篮不变。
- Win64 打包 PASS：PackageWin64-20260911-122819.log；包内核心烟测 PASS：PackagedSmokeConsole-20260911-122918.log。
- 新分发 ZIP：238,763,741 字节，SHA-256：3CCC5DAC0998933396E52A4542CDBC74BE308607E55EE8A0FFEF2B331C73D0EF；前版保留于 Artifacts/Backups/PreviousDemo-20260911-122916.zip。包内排除 Saved/Artifacts。
- 存档备份：Artifacts/Backups/BeforeRepackage-20260911-122819-341/Saved，打包后已恢复。
- 实际按键验证 PASS：F1SupplyManualQA.log；第一次 Wood=102 / Rope=101 / 其余七类=100，第二次 Wood=202 / Rope=201 / 其余七类=200（包含屏幕未显示的 Power）；主菜单及暂停按键不发放，恢复后无补发，画面正常光照且提示正确。四次实体按键仅游戏中两次产生发放记录。未用 GUI 模拟持续长按，该行为以 Started 绑定代码检查为依据。
- 测试窗口已关闭，用户普通/展示档未被补给测试修改；下方为上一轮菜单/图标修复的历史证据。

## 本轮修复

- Esc 暂停新增继续、保存、保存并返回主菜单；鼠标和快捷键在暂停时可用。
- 返回请求先取消蓄力或收回飞行中的钩子；携货完成入包后才保存，保存失败留在暂停场景。继续当前会话保留原模式、位置和进度。
- 清理移动按下状态、拖拽和其它面板；拦截菜单中的 R/B 与开发快捷键，防止误改物资。
- 快速拖拽保存按下事件坐标，避免 Enhanced Input 延迟回调读取到移动后的终点；菜单点击也采用按下快照，落点仍用实时位置。
- 10 种物资图标由真实网格/材质渲染，预载后再打开背包，解决首次显示短暂黑块。
- 中文改为按最终像素字号进行字形光栅化、整数像素定位，避免放大小字号缓存；启用游戏窗口 per-monitor DPI awareness。
- 暂停帮助明确岛与沉船目前是装饰，不具备登陆或搜刮玩法。
- 重打包前备份并校验原 Saved，完成后恢复用户进度和设置。

## 环境

UE 5.3.2（CL 29314046）、模块 Driftstead、Win64 Development、VS2022 / MSVC 14.36.32546 / Windows SDK 10.0.22621。

## 验收证据

日志均位于 Artifacts/Logs。

| 验证 | 当前结果与证据 |
| --- | --- |
| 最终 Editor 编译 | PASS：BuildEditor-20260911-121903.log |
| 10 种真实模型图标 | PASS：InventoryIcons-20260911-120201.log、InventoryIcons.json；10 个不同 ID、256×256；逐张 PNG 目视检查 |
| 磁盘图标重新加载 | PASS：InventoryIconReload.log；新进程重载全部 uasset，再导出 PNG 检查；不是仅检查拍摄源 |
| 完整内容校验 | PASS：ValidateIcons-20260911-120237.log、ContentValidation.json；84 项、10 图标、两张表各10行 |
| C++ 自动化 | PASS 24/24：AutomationTestsConsole-20260911-121931.log；最终快速拖拽版本重跑 |
| 菜单运行时回归 | PASS：MenuValidation-20260911-120302.log；真实 RHI 下 10 图标、暂停保存/继续、Idle/Charging/Flying/Attached 安全返回、多物资入包后再保存、新局状态清理、展示模式继续 |
| 包内核心烟测 | PASS：PackagedSmokeConsole-20260911-122141.log；多目标回收、重量/高度、楼层和设施存量、隔离存档 |
| 包内菜单回归 | PASS：PackagedMenu-20260911-122150.log；同上；不冒充鼠标拖放测试 |
| 包内完整首段航程 | PASS：PackagedOpening-20260911-122159.log；六件实际回收、付费升级、真实45秒种植、楼梯、信标12目标和异常主档回退 |
| 普通游戏窗口 GUI | PASS（已列操作）：MenuFontManualQA.log；鼠标继续读取三级测试档、Tab 打开非空背包、种子箱拖至空格并显示“物品已移动”、Esc 暂停、点击保存并返回、点击继续当前航程；未修改正常档 |
| 最终包 GUI 再验 | PASS：FastDragPackagedQA.log；实际鼠标继续、Tab 背包、种子箱快速从 (0,0) 拖至 (2,2)、Esc 暂停、点击保存并返回主菜单、Q 正常退出；图标与中文目视通过，使用隔离测试档 |
| 高 DPI 普通窗口 | PASS：MenuFontManualQA.log 中 Setting process to per monitor DPI aware；主菜单、暂停和游戏中文字实测，无需修改 Windows 缩放 |
| 预载后首开背包截图 | PASS：ScreenshotCaptureConsole-20260911-120946.log、Artifacts/Screenshots/03_Inventory.png；同帧截图已显示真实图标而非黑块 |
| 最终 Win64 构建/Cook/归档 | PASS：PackageWin64-20260911-122043.log |
| 编辑器内 PIE | PASS（启动和菜单进入）：EditorMenuManualQA.log；图标预载版本打开 L_Demo 后实际点击 Play/新航程/Stop；其它玩法以包内自动化及普通窗口回归为证据 |
| 用户存档保护 | 第一次备份 BeforeRepackage-20260911-120359-159/Saved；Normal 和 Normal_Backup 还原前后 SHA-256 一致 |

## 发现的问题与保留证据

1. 增加编译单元暴露了旧控制器头文件对 FInputKeyParams 完整类型的隐式依赖；完整首错日志 BuildEditor-20260911-115755.log，已显式包含 GameFramework/PlayerInput.h 后通过。
2. UE5.3 Python SceneCapture 属性只读限制、保存期间 Slate 回调重入导致重复/跳过物品；已使用公开组件函数、重入防护、ID 唯一与顺序校验修复。失败日志由图标生成阶段保留，最终脚本通过。
3. NullRHI 下 Texture2D 的 GPU 尺寸不可作为图标可用性的证明；首次 MenuValidation-FirstFailure-20260911.log 的玩法断言通过、图标断言失败。改为真实 RHI 测试后全部通过，没有弱化图标检查。
4. 首开背包当帧才 LoadObject，图标 GPU 资源尚未就绪；首次截图发现黑矩形，普通窗口稍后正常。现改为 HUD BeginPlay 预载，后续首开截图通过。
5. 上一轮窗口激活失败本次未复现，已完成实际鼠标操作。历史报告保留于 Artifacts/Backups/TEST_REPORT-before-menu-20260911.md。
6. 最终 GUI 的极短拖拽再次暴露输入时序：MouseDown 已记录，但 Enhanced Input Started 执行时实时鼠标已在终点。保存原始事件按下点后，FastDragManualQA.log 对应连续三次跨格拖放均成功、数量不变；不是靠放慢测试规避。

## 范围与边界

分发 ZIP：238,758,306 字节，SHA-256：42A85283589F4546462068E9A8F310142F53739567CC64437A629EFEA2144FD6。旧 ZIP 备份于 Artifacts/Backups/PreviousDemo-20260911-122215.zip。已对 ZIP 内 exe、ucas、utoc 与测试目录做逐文件 SHA-256 比较一致，ZIP 无 Saved/Artifacts 数据；普通存档及备份与本轮最初备份哈希一致。

- 自动化使用 Driftstead_Automation 及备份槽；GUI 用 DriftsteadManualQA 加载测试夹具，绝不拿普通/展示玩家存档做破坏性测试。
- 完整首段自动化预置资源，不能证明自然游玩必定在20–30分钟完成。
- Canvas 中文 HUD，不是完整 UMG 重构；角色仍为静态模型加轻微动态，绳索为轻量几何。
- 小岛和沉船无交互；种子箱/机械箱/动物箱共用木箱外观。
- 未做跨硬件、超宽屏、长时间压力、Shipping 与主观音效验收。
- 测试执行时尚未上传；2026-09-11 后续工程同步已完成，工程提交为 `2e311b0`，另有文档同步记录。此次未公开发布可执行包 Release；未重新运行或扩大上述测试范围。
