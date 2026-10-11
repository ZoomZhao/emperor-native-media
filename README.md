# emperor-native-media

当前维护自写原生媒体组件、资源加载器与城市营建 App。ang 的城市模型已纳入默认构建，支持建造、拆除、按月推进和自写存档；具体规则与独立验收见 [CITY_MODEL.md](docs/CITY_MODEL.md)。尚未证明原版行为一致或完整 Emperor 可玩。

| 位置 | 用途 |
|---|---|
| `game_lab/` | AppKit 城市营建、资源加载、AppMedia 和静音 MovieSession；[入口与支持范围](game_lab/README.md) |
| `surface_lab/`、`metal_lab/` | CPU 像素转换、裁剪/缩放和离屏 GPU |
| `audio_lab/`、`video_lab/`、`stream_lab/`、`media_demo/` | 离线队列、时间轴、媒体生命周期与自造素材验收 |
| `runtime_core/` | 有限 heap/私有内存/context 与纯内存 TLV；尚无 App 消费者 |
| `legacy/` | 历史源码归档，不接入当前维护入口 |
| `GameData/`、`.local/` | 忽略的本地资源与配置，不随源码提交 |

需要 macOS 15+ 和已安装的 Apple 开发工具。`python3 scripts/independent_runner.py city --mode release --output /absolute/new-output` 验收自写城市；同一入口提供 surface、metal、media、integration、movie-async、movie-termination、data 与仅构建 app，并支持测试的 undefined 配置。入口保存明确源码/输入/二进制/结果凭据，每个子进程最多 60 秒。`make game` 是读取本地 GameData 的旧样本入口，本轮没有运行；`make run-game` 会启动 GUI，也没有调用。

城市与媒体测试使用自造输入，Release/UBSan 均通过；App 编译、链接、打包和本地签名校验通过。未启动 GUI、操作真实声音设备或读取 GameData。App 默认静音、无启动自动播放；播放影片走静音媒体路径。声音、设备恢复、真实资产和原版兼容仍未验证。GPU 读回不代表完整视觉验收，离线 PCM 不代表听觉通过。RGB555 随机边界与有界图形入口的实际增量见 [验收说明](docs/RGB555_BOUNDARIES.md)。

能力缺口和可继续工作见 [GAPS.md](GAPS.md)，当前允许范围与原任务具体受阻路线见 [EXECUTION_SCOPE.md](docs/EXECUTION_SCOPE.md)。[平台审计](docs/PLATFORM_AUDIT.md)及其他历史报告保留各自时点证据；旧“资源浏览器未接城市”“增量未发布”状态已由当前源码与 CITY_MODEL 更新。

当前自写源码采用 [MIT 许可证](LICENSE)，App 包内保留许可证与生产源码哈希；原游戏、第三方工具和 Apple SDK 的许可不因本仓库改变。归档和资源目录存在不能代替行为迁移或执行验收。
