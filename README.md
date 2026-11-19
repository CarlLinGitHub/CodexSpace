# Stellar Thunder

《Stellar Thunder》是一款用 Unreal Engine 5.8 和 C++ 制作的《雷电》风格 3D 太空弹幕游戏。镜头从玩家战机的后上方俯视并跟随移动。战役共五关，每关四波敌机和一个两阶段 Boss。

## 操作

| 键盘 | Xbox 手柄 | 操作 |
| --- | --- | --- |
| `WASD` / `方向键` | 左摇杆 / 十字键 | 移动 |
| `空格` | `A` / `RT` | 开始；按住持续射击 |
| `Shift` | `LT` | 降低移速并缩小受击范围 |
| `Z` | `LB` | 清除当前敌弹，每局两次 |
| `R` | `Menu` | 返回标题画面 |

战机可移动到游戏画面的四边和四角，边缘留有少量机身空间。

每局 3 条命，初始为单线黄弹（0 级）。击落普通敌机有 32% 概率掉落一种随机颜色的补给。拾取同色补给升一级，最高 5 级；拾取异色补给会清空此前积攒的升级，并切换到新颜色的 1 级。切回旧颜色也从 1 级开始。

| 弹药 | 每级散射角度增量（单侧） | 每级单目标伤害增量 |
| --- | ---: | ---: |
| 黄 | 7° | 0 |
| 红 | 2° | 3 |
| 蓝 | 4° | 1 |

每次齐射对同一目标只结算一次，因此黄弹增加覆盖范围，不靠重叠弹线增加伤害。过关保留生命、分数和当前弹色等级，并补充一发清弹脉冲（最多两发）。

## 关卡

1. **Outer Rim**：基础队形、瞄准扇射。
2. **Asteroid Belt**：纵深队形、环弹与扇射交替。
3. **Ion Nebula**：左右轮流进场、摆动扫射。
4. **Siege Line**：更多重型敌机、双炮位交叉弹幕。
5. **Dreadnought Core**：高速密集编队、环弹与偏转瞄准混合。

武器取舍与关卡节奏参考 [Raiden IV](https://shmups.wiki/library/Raiden_IV)：宽覆盖和集中火力服务于不同场景，先引入单一弹型，再在后段组合。没有复刻原作关卡或美术。

## 仓库内容

这是**仅源码仓库**。`CodexSpace.uproject` 是 Unreal 项目描述文件；`Source/` 包含 C++ 源码及 Unreal Build Tool 构建定义。不包含项目关卡、模型、材质、原始配置或 Windows 可执行文件。**克隆后不能直接游玩。**

要构建源码，在 Windows 上安装 Unreal Engine 5.8、MSVC 和 Windows SDK，打开 `CodexSpace.uproject` 生成项目文件，再构建 `CodexSpaceEditor`（Win64 Development）。要在编辑器中游玩，还需在本地加入关卡和代码引用的 `/Game/SpaceShooter` 素材，并将关卡的 GameMode 设为 `SpaceGameMode`。

## 素材来源

原版使用的飞船模型来自 [Quaternius Ultimate Space Kit](https://quaternius.com/packs/ultimatespacekit.html)，采用 [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)；模型未随本仓库分发。
新版玩家战机使用蓝色 Omen 涂装，敌机和 Boss 同样来自 Quaternius 的 [Ultimate Spaceships Pack](https://quaternius.com/packs/ultimatespaceships.html)（CC0 1.0）。

星云背景来自 Screaming Brain Studios 的 [Seamless Space Backgrounds](https://opengameart.org/content/seamless-space-backgrounds)，同样采用 CC0 1.0。
陨石模型为 Game Piggs 的 [Asteroid 2a](https://game-piggs.com/models/asteroid-2a/)（CC0）。
