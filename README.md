# Stellar Thunder

《Stellar Thunder》是一款用 Unreal Engine 5.8 和 C++ 制作的《雷电》风格 3D 太空弹幕游戏。玩家驾驶战机穿越滚动星空，迎战 8 波敌机和一艘两阶段 Boss 战舰。

## 操作

| 按键 | 操作 |
| --- | --- |
| `WASD` / `方向键` | 移动 |
| `空格` | 开始；按住持续射击 |
| `Shift` | 降低移速并缩小受击范围 |
| `Z` | 清除当前敌弹，每局两次 |
| `R` | 返回标题画面 |

每局有 3 条命，拾取敌机掉落的道具可将火力提升至 3 级。

## 仓库内容

这是**仅源码仓库**。`CodexSpace.uproject` 是 Unreal 项目描述文件；`Source/` 包含 C++ 源码及 Unreal Build Tool 构建定义。不包含项目关卡、模型、材质、原始配置或 Windows 可执行文件。**克隆后不能直接游玩。**

要构建源码，在 Windows 上安装 Unreal Engine 5.8、MSVC 和 Windows SDK，打开 `CodexSpace.uproject` 生成项目文件，再构建 `CodexSpaceEditor`（Win64 Development）。要在编辑器中游玩，还需在本地加入关卡和代码引用的 `/Game/SpaceShooter` 素材，并将关卡的 GameMode 设为 `SpaceGameMode`。

## 素材来源

原版使用的飞船模型来自 [Quaternius Ultimate Space Kit](https://quaternius.com/packs/ultimatespacekit.html)，采用 [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)；模型未随本仓库分发。
