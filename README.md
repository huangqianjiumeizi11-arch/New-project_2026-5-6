# MonsterDodge EasyX 小游戏

这是一个 Visual Studio + C++ + EasyX 制作的俯视角闪避闯关小游戏。

## 玩法

- 玩家通过键盘移动角色，鼠标控制攻击方向。
- 地图中有障碍物、追击怪、射击怪、Boss、随机刷新的药包和能量补给。
- 空格冲刺会消耗能量，并提供很短的无敌时间，用来穿过危险弹幕。
- 连续击败怪物会显示连击，并提升得分。
- 击败所有怪物后进入升级界面。
- 选择升级后进入下一关。
- 共 3 个关卡，第三关击败 Boss 后胜利。

## 初始菜单

游戏窗口打开后会先进入主菜单，可用鼠标移动光标并用鼠标左键选择：

- 开始游戏：进入第 1 关
- 游戏设置：开启 / 关闭背景音乐，调整音量 0~100
- 操作说明：查看移动、攻击、冲刺和暂停按键
- 退出游戏：关闭程序

设置界面也支持 `↑` / `↓` 调整音量，背景音乐开关通过鼠标左键点击。

背景音乐使用 miniaudio 播放，文件名为 `bgm.mp3`，可放在 exe 同目录或项目 `Release` / `Debug` 目录。

## 操作

- `WASD`：移动
- 鼠标移动：瞄准
- 鼠标左键：攻击
- `Space`：冲刺 / 短暂无敌
- `Esc`：游戏中打开暂停菜单
- `R`：失败或胜利后重新开始
- 失败或胜利后按 `Esc`：返回主菜单
- 暂停菜单可用鼠标左键选择“继续游戏”或“返回主菜单”


## 源码结构

- `MonsterDodge.cpp`：程序入口和主循环
- `Game.h` / `Game.cpp`：游戏状态、关卡、菜单、绘制和更新逻辑
- `MusicManager.h` / `MusicManager.cpp`：背景音乐播放和音量控制
- `Types.h`：基础结构体和枚举
- `third_party/miniaudio/miniaudio.h`：第三方音频库
## 用 Visual Studio 运行

1. 安装 EasyX：打开 EasyX 官网下载安装包，并选择安装到 Visual Studio。
2. 用 Visual Studio 打开 `MonsterDodge.sln`。
3. 选择 `Debug x86` 或 `Release x86`。
4. 按 `F5` 运行。

如果你的 EasyX 是 64 位环境，也可以切换到 x64；如果编译报库文件不匹配，优先使用 x86。

## 也可以手动创建工程

如果 `.sln` 无法适配你的 Visual Studio 版本：

1. 新建一个 C++ 空项目。
2. 把 `MonsterDodge.cpp`、`Game.cpp`、`MusicManager.cpp` 加入源文件，并保留对应头文件和 `third_party` 目录。
3. 确认 EasyX 已安装。
4. 编译运行。












