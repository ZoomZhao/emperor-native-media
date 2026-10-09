# Game Lab: Emperor 原版资产引擎与原生运行器

本模块负责与 `GameData/` 原版游戏资产对接，实现资产解析、格式转换、内存管理及原生 macOS 交互运行。

## 架构

- **`emperor_assets.hpp` / `emperor_assets.mm`**
  - **JPEG 解码**: 基于系统 `ImageIO` 框架，解码 1024×768 高清国风界面（如 `China_FE_MainMenu.jpg`）。
  - **Raw 555 贴图**: 基于 `surface_lab` 的 `RGB555LE` 格式，将 640×480 原版贴图（`China_DEMO1.555` 等）转换为 32 位 `BGRA8`。
  - **SG3 精灵归档**: 解析 Impressions Games SG3 格式（magic 213、位图表、64 字节精灵记录），支持 16×16、24×24、32×32 及任意尺寸图素提取。
  - **RIFF WAV 音效**: 解析 736 个 22.05kHz 16-bit 线性 PCM 音效（单声道/双声道）。
  - **城市地图**: 解析 128×128 城市地形图（`.map`），解压 zlib 地块 ID 网格。
  - **中文文本库**: 使用 CoreFoundation 将原版 GBK/GB2312 编码的 `EmperorText.txt` 无损转码为 UTF-8 文本。

- **`tests.mm` / `run.sh`**
  - 自动化无头回归测试，验证真实资产读取与解码。
  - 集成进根目录 `Makefile` (`make game`) 与 `scripts/run.py`，受 55 秒超时硬性约束守护。

- **`app/main.mm` (`EmperorApp.app`)**
  - 原生 macOS AppKit + Metal 交互运行器。
  - 包含 5 个交互式工作台：
    1. **主界面 (Main Menu)**: 1024×768 经典界面与交互式菜单选项。
    2. **城市地图 (City Maps)**: 浏览与渲染 19 个原版城市地图地形（长乐、安阳、八达岭、成都等）。
    3. **图素与贴图 (Sprite & Texture Explorer)**: 实时浏览 SG3 精灵包与全屏 555 贴图。
    4. **国风音乐台 (Jukebox)**: 播放 20 首原版古典中国民乐（三弦、二胡、古琴、琵琶等）与触发 736 个 WAV 原版音效。
    5. **游戏文本资料库 (Game Text)**: 搜索与查阅 7,599 条原版汉化游戏对话与提示。

## 构建与运行

```bash
# 运行资产回归测试
make game

# 构建原生应用
make game-app

# 启动原生应用
make run-game
```
