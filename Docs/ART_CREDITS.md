# 美术与音效来源

## 新增模型

- Kenney Pirate Kit 2.1 — https://kenney.nl/assets/pirate-kit
  - 许可：CC0，原包 `SourceArt/KenneyPirate/Pack/License.txt`。
  - ZIP SHA-256：`667ED2CAF92954DDB98F7B7CEDE831FE99AB75063C26B25E23D32715BEE9C943`。
  - 已选用 27 个模型：木桶、箱子、瓶子、木板、平台、围栏、建筑构件、小船、桅杆、棕榈、岩石、沙洲、植物、沉船、工具、塔等。
- Kenney Mini Characters 1.0 — https://kenney.nl/assets/mini-characters
  - 许可：CC0，原包 `SourceArt/KenneyCharacters/Pack/License.txt`。
  - ZIP SHA-256：`9E1D48E6D7B8479EBBE84DF71EB5BD8E1B3F0DA546DEA641890DCCC8A02D0999`。
  - 选用一名角色模型及原包调色板贴图。当前导入为静态模型，移动采用轻微摇摆，不宣称已实现骨骼行走动画。

两包均明确允许个人、教育及商业用途，不要求署名；在此保留作者及许可便于追溯。可与源码一起公开分发。下载脚本为 `Tools/DownloadDemoArt.ps1`，导入脚本为 `Content/Python/import_demo_art.py`。

## 本项目生成的资产

- 三爪钩、绳索卷、带扶手木梯：`create_signature_meshes.py` 生成原创几何 OBJ，再经 Unreal 导入静态网格。不是更名复制网上模型。
- 海面：缓慢运动的程序化颜色波纹，不使用实时水体模拟。
- 抛钩、收获、扩建、播种、信标：`create_demo_audio.py` 生成简短原创合成音效；非商业歌曲或网络采样。
- 原始输出位于 `SourceArt/Original`，游戏资产位于 `/Game/Driftstead/Art`。

## 既有材质

之前下载的 Poly Haven 木板、金属和织物贴图继续保留在 `SourceArt` 与 `Content/Driftstead/Textures/CC0`。新 Kenney 模型使用其配套调色板，以统一视觉语言；未将纹理强行覆盖到不匹配的 UV 上。

## 美术边界

目前是统一低多边形风格的作品集 Demo，不是完整商业美术交付。工作台/鸡舍等设施由现有模型构件组合；高级设施仍可继续定制专属外观。完整角色骨骼动画、昼夜天气、专业海浪与最终配乐未包含。
