# emperor-native-media：Emperor 复刻工程工作目录

本目录现在集中管理整个工程：当前维护的原生媒体组件，以及历史自写平台适配、运行时、状态协议、分析和验证工具。名称沿用 `emperor-native-media`。历史源码已按原相对路径归档，模块说明见 [legacy/INDEX.md](legacy/INDEX.md)，完整差距见 [GAPS.md](GAPS.md)。现有证据尚未证明完整游戏可以原生运行。

| 位置 | 用途与状态 |
|---|---|
| `surface_lab/`、`metal_lab/` | 当前独立像素表面、裁剪/缩放与离屏 GPU 实验 |
| `audio_lab/`、`video_lab/` | 当前离线 PCM 队列、帧时间轴和生命周期模型 |
| `av_lab/`、`stream_lab/`、`media_demo/` | 当前自造媒体解码、时钟与静音 App；保留原 Makefile 入口 |
| `legacy/src/samples/` | 历史平台 C/Objective-C 桥、测试模型与研究工具 |
| `legacy/src/stage74/` 至 `stage83/` | 历史内存/heap/thread/runtime、协议、差分与执行门源码归档 |
| `legacy/import-manifest.json`、`legacy/FILES.sha256` | 每份来源/副本哈希、脱敏记录、暂留清单 |
| `docs/MEDIA.md`、`VALIDATION.md` | 现有媒体使用入口与历史验收范围 |
| `docs/PLATFORM_AUDIT.md`、`docs/audit-evidence.json` | 本轮文档审计与证据定位/哈希 |
| `docs/DEPENDENCIES.md` | 当前与历史依赖、构建和执行边界 |
| `GameData/`、`.local/` | 忽略的本地素材与依赖路径；本轮未读取 GameData 内容 |

751 份历史自写文本源码已归档，其中 97 份副本将机器绝对路径换成占位符。41 份含原字节签名、生成绑定、私人记录或来源待核查内容的源码保留在旧目录。原 EXE、资源、存档、快照、提取物、生成游戏代码、构建产物、第三方工具和原始日志也保留原位；其绝对路径只记入忽略的本地配置。具体处置以迁移清单为准。文件归档完成不代表逻辑迁移或行为验证完成。

当前媒体组件需要 macOS 15+ 与已安装的 Apple 开发工具，使用系统框架及 Python 3。例如 `make fixtures`、`make build`、`make surface`、`make audio`、`make video`、`make metal`、`make av`、`make stream`；完整入口和约束见 [媒体说明](docs/MEDIA.md)。这些目标只引用仓库自造图片、PCM、MOV 和事件。App 默认静音；PCM 消费不连接输出设备。历史归档没有接入这些入口。

**本次整合只做文本复制、脱敏、哈希和静态语法/依赖核对，没有编译、运行测试或历史执行链，没有安装、联网、上传、操作 UI/键盘/音频设备或读取旧 Swift 实现。** 二进制转换/探测的既有平台安全拦截仍有效；移动目录、保存源码或旧批准记录不能解除拦截。不要从 `legacy/` 执行脚本、导入其运行时，或加载历史游戏衍生产物。

当前用户改动、媒体入口、MIT 许可证及本地 GameData 均保留；本轮没有提交或 push。下一步可在独立原生组件中用完全自造输入补齐验收，三个具体任务和通过标准见 [平台审计](docs/PLATFORM_AUDIT.md)。

当前自写媒体源码采用 [MIT 许可证](LICENSE)。历史归档的静态来源筛选不代替逐项许可确认；原游戏、第三方工具及 Apple SDK 的许可不因项目目录或顶层许可证改变，亦未随此次整合复制。
