# emperor-native-media：Emperor 工程工作目录

本目录集中维护自写原生媒体组件、资源浏览器与历史自写源码归档。现有证据没有证明完整游戏可玩；资源浏览器不包含完整游戏业务或执行引擎。当前差距见 [GAPS.md](GAPS.md)。

| 位置 | 用途与状态 |
|---|---|
| `surface_lab/`、`metal_lab/` | RGB565/BGRA8/RGB555LE、裁剪/缩放与离屏 GPU；有限数值验收记录 |
| `audio_lab/`、`video_lab/` | 离线 S16 队列、帧时间轴和生命周期模型 |
| `av_lab/`、`stream_lab/`、`media_demo/` | 自造媒体、时钟与默认静音的 NativeMediaDemo |
| `game_lab/` | 资源加载与 AppKit 资源浏览器；解析和真实控制路径的无 UI 两配置通过；App 已只编译/签名，GUI/真实资产未运行 |
| `runtime_core/` | 有限 heap/私有内存/context 与纯内存 TLV；自造 Release/UBSAN 验收通过 |
| `legacy/src/`、`legacy/INDEX.md` | 历史自写平台/运行时/协议源码归档，保留原相对路径；不接入维护入口 |
| `legacy/import-manifest.json`、`legacy/FILES.sha256` | 来源、副本哈希、脱敏及暂留记录 |
| `docs/PLATFORM_AUDIT.md`、`VALIDATION.md` | 既有证据与实际覆盖边界 |
| `docs/SOURCE_REPAIR.md`、`docs/source-repair-static-audit.json` | 初次源码修复与静态检查快照 |
| `docs/DEPENDENCIES.md`、`runtime_core/dependency-map.json` | 当前依赖和历史缺口分类 |
| `GameData/`、`.local/` | 忽略的本地素材与配置；本轮没有读取 GameData 内容 |

需要 macOS 15+ 与已安装的 Apple 开发工具。`make fixtures/build/surface/audio/video/metal/av/stream` 使用自造素材；`make game` 是读取本地 GameData 的旧资源样本测试，两者输入范围不同。`make game-boundaries` 是资源自造边界入口，`make runtime-boundaries` 是精确有限数据/TLV 入口；本轮均未运行，后续范围核查见 [EXECUTION_SCOPE.md](docs/EXECUTION_SCOPE.md)。`make game-app` 构建资源浏览器，`make run-game` 构建并打开它，同时显式传递仓库内 GameData 的绝对路径；本轮均未调用。更多配置见 [game_lab/README.md](game_lab/README.md)。

两种 App 都默认静音。EmperorApp 不在启动或取消静音时自动播放；音乐与音效共用静音策略，实际声音仍未验证。它以 AppKit/NSImage 呈现资源，没有将离屏 Metal 测试接入完整游戏渲染。

751 份历史自写文本源码归档保持哈希不变，其中 97 份副本做了机器路径脱敏；41 份来源/内容不适于此次迁移的源码留在旧目录。EXE、原资源、快照、生成游戏代码、衍生库、第三方工具和原始私人记录未随归档复制。文件归档或路径修正不构成行为迁移完成。

2026-10-10，交接前独立修复已提交并推送为 `dd6d6fe8e8841c505669ca740a088214241f991d`。后续五项源码增量仍在工作区：现有90断言通过，补充回归发现四项音频状态/生命周期失败，尚未发布。当前验收与缺陷证据见 [BLOCKERS-AND-HANDOFF.md](docs/BLOCKERS-AND-HANDOFF.md)。城市目标与历史局部证据见 [CITY_CAPABILITY_AUDIT.md](docs/CITY_CAPABILITY_AUDIT.md)，具体受阻执行路线见 [EXECUTION_SCOPE.md](docs/EXECUTION_SCOPE.md)；普通城市工程分析可以继续。

当前自写媒体源码采用 [MIT 许可证](LICENSE)。归档来源筛选不代替逐项许可确认；原游戏、第三方工具和 Apple SDK 的许可不因本目录改变。

来源明确的自造作用域与现成自写驱动现在有[仅编译检查入口](docs/AUTHORED_DRIVER_COMPILE.md)：生成一个对象，并要求入口与绑定保持未解析；不执行归档工具或驱动。归档源码本体不改。[城市构建交接](docs/CITY_BUILD_HANDOFF.md) 区分发布仓库未收录、本机已有产物和真实运行缺口。

后续数据模块准备与当前版本静态证据见 `RUNTIME_PREPARATION.md`（本地历史报告；未纳入本次提交） 和 `runtime-preparation-static-audit.json`（本地历史报告；未纳入本次提交）。

当前自造运行结果、修复的实测缺陷及哈希凭据见 [INDEPENDENT_VALIDATION.md](docs/INDEPENDENT_VALIDATION.md)。旧通用 Makefile/runner 没有作为本次受限输出入口运行。

## 当前 App 实际集成与仅构建验收

2026-10-10：AppMedia 已成为真实 App 与无 UI 测试共用的控制实现，补齐 App 音效到当前 WAV 校验、失败替换清理、默认静音/gain/显式播放/终止，以及 JPEG/raw555 的资源路径接线。Release/UBSAN 集成各 62 断言通过；当前 EmperorApp 编译、链接、打包和本地 codesign strict 校验通过，包内保存生产源码指纹，旧包保留。未启动 GUI、真实 AVAudioPlayer 或设备，未读取 GameData/legacy。

真实依赖清单、当前包证据和限制见 [APP_INTEGRATION.md](docs/APP_INTEGRATION.md)。runtime_core 仍没有 App 消费者；音频队列/视频/Metal demo 未被接入 EmperorApp，完整业务引擎与状态/调度/交互/存档合同仍缺失。先前“只静态/未构建”是当时记录；当前新包与无 UI 集成已实际验证，不能推广为完整游戏或声音/GUI通过。这些交接前修复已发布；后续增量的验收状态以进度页为准。

当前独立入口已修复并在干净白名单目录实际复现；实质发现、可接手命令与逐项阻塞见 [BLOCKERS-AND-HANDOFF.md](docs/BLOCKERS-AND-HANDOFF.md)。数据/TLV、媒体与 App 无 UI 集成 Release/UBSAN 均通过，当前 App 只构建/签名通过。交接前源码已发布；本轮审计记录收录新增失败证据，没有发布失败的源码增量。
