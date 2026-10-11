# Game Lab：自写城市与媒体

默认 EmperorApp 包含 `CitySimulation`，可在空城建造、拆除、推进月份、调整速度、保存及载入。模型规则与 EMP1 合同见 [CITY_MODEL.md](../docs/CITY_MODEL.md)；没有原版行为对比，不能称为完整复刻。

| 源码 | 支持范围 |
|---|---|
| `city_simulation.hpp`、`city_state_io.hpp` | 128×128 自写模型、严格候选状态验证、有界普通文件读取和同目录原子保存 |
| `city_simulation_tests.cpp` | 生产模型/共用文件接口的自造 Release/UBSan 回归 |
| `wav_pcm.cpp` | RIFF/WAVE PCM16LE、1–8 声道、8000–192000Hz；拒绝范围、格式、帧及重复块错误，不重采样 |
| `asset_pixels.cpp` | 精确 RGB555LE → BGRA8；当前补位扩展合同 |
| `emperor_assets.mm`、`asset_limits.hpp` | 有界资源读取；JPEG 元数据/完整解码、有限 SG3 原始图素、128×128 map 和 GB18030 非空文本预览；不保证全部原资产兼容 |
| `resource_root.*`、`app_launch.hpp` | 明确绝对目录，拒绝相对子路径逃逸、非普通文件等；无配置时选择目录，取消退出 0，明确无效配置退出 2 |
| `app_media.*`、`app/av_audio_backend.*` | 真实 App/fake 测试共用控制路径，默认静音，无启动自动播放，音乐和音效共用 gain/静音策略 |
| `movie_session.*` | 静音 Reader/Controller、异步状态、seek/取消与有界终止；自造 MOV 验收 |

SG3 提取只支持 type 16/24/32 的精确原始 RGB555 方块，flags 为 0，uncompressedLength 为 0 或预期值；未知/压缩布局拒绝。路径过滤属于应用合同，不是 OS sandbox 或并发文件系统安全保证。

```sh
python3 scripts/independent_runner.py city --mode release --output /absolute/new-output
python3 scripts/independent_runner.py city --mode undefined --output /absolute/new-output
python3 scripts/independent_runner.py media --mode release --output /absolute/new-output
python3 scripts/independent_runner.py integration --mode undefined --output /absolute/new-output
python3 scripts/independent_runner.py movie-async --output /absolute/new-output
python3 scripts/independent_runner.py movie-termination --output /absolute/new-output
python3 scripts/independent_runner.py app --output /absolute/new-output
```

这些入口只用自造输入、每个子进程最多 60 秒，保留源码/实际输入/二进制/结果哈希。`app` 仅构建、打包和签名校验，不启动。当前两配置均通过；GUI、听觉、真实设备和 GameData 未验收。此前本地 `boundary_tests.mm` 是早期修复草稿，仍保留；当前入口明确使用 `pure_boundary_tests.mm` 与 App 集成测试。

用户运行 App 时可传 `--data-root /absolute/self-authored-resources` 或 `EMPEROR_GAMEDATA_ROOT`；命令行优先，也可选择自己的空目录先营建空城。构建不会复制或读取 GameData。`tests.mm`/`run.sh` 是旧 GameData 样本入口，本轮未运行；`make run-game` 会打开 GUI。只编译/签名通过不能替代启动、选择目录、点击、可见影片或声音验证。
