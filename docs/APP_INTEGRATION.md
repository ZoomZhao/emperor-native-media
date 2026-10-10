# 当前 EmperorApp 实际集成验收

后续更新：真实 App 已增加受限的影片首帧预览页，接入共用的
`AppMedia::moviePoster`、现有 MovieReader 与 ImagePresenter。具体接口、
自造输入回归和限制见 [APP_MOVIE_POSTER.md](APP_MOVIE_POSTER.md)。
以下“没有影片页”等结论为先前音频/图片接线时点的记录；新入口不代表
影片播放、Bink、真实 GUI 或声音已经通过。

2026-10-10。本轮审阅现有 build 脚本，修复真实 App 调用接线，执行自造输入的无 UI 集成测试，并只编译、链接、打包与本地签名校验当前自写 App。没有启动 App、访问 GameData/legacy、操作设备、安装、联网或恢复原二进制链；新改动未 commit/push。

## 已关闭的具体缺口

原 App 音效直接交给 AVAudioPlayer，绕过已修复的 `decodeWAV`；新音效在路径/解码失败时可能让旧音效继续播放。新增 `app_media.*` 作为真实 delegate 和集成测试共用的控制实现，音效先做 64 MiB 有界读取和完整 PCM16 WAV 校验，再把同一份 bytes 交给工厂；音乐也先做 128 MiB 有界读取。`av_audio_backend.mm` 使用复制 NSData 的 `initWithData`，不重新按路径打开文件。

音乐/音效替换均先停止并清除旧对象；工厂失败、首次播放失败和恢复失败进入明确状态并释放对象。默认静音、零/非有限 gain、静音暂停音乐/清理音效、取消静音不自动播放、终止清理均进入同一个 `AppMedia` 路径，不再由 App 复制另一套策略。原 App 内选择器现在调用 `selectMusic`、`toggleMusic`、`playSfx`、`setMuted`、`setVolume`、`stopAll`，并统一刷新状态和按钮。

JPEG 和 raw555 同样由 AppMedia 解析明确资源根后调用已修复的生产 `loadJPEG` / `loadRaw555`；RGB555 经当前 `surface_lab` CPUStore 转换。主菜单图像失败有说明，raw555 的 NSImage 转换失败不会显示成功文字。这些显示调用点已审阅且编译通过；没有执行 NSImage/GUI 展示行为。现有 SG3/map/文本入口继续经同一个资源根，但没有运行、分析或扩展城市业务。

## 实际验证与包绑定

| 范围 | 当前结果 | 实际证明边界 |
|---|---|---|
| AppMedia 无 UI 集成 Release | 62 项断言通过 | 自造 WAV/JPEG/raw555 文件，经真实资源根、生产加载器与 AppMedia 控制，播放器工厂替换为 fake |
| 同一集成 UBSAN | 62 项断言通过 | 相同合同；只给此次编译的自写源码插桩，系统框架未插桩 |
| 当前 EmperorApp | 编译/链接、打包与 codesign strict 校验通过 | 8 个明确自写翻译单元，系统框架/库；未执行 App 或真实 AVAudioPlayer |

集成用例针对上述接线：默认与取消静音不创建播放器；music/SFX 的 bytes/gain/loops 送达 fake；静音暂停/释放；坏 WAV、丢失路径、目录逃逸、超限文件在工厂前失败；替换失败清理旧对象；工厂/首次启动/恢复失败退役；终止幂等；JPEG 截断拒绝及 raw555 实际像素加载。不是重复全色域或离线队列样例，也不是整游戏通过。

最终运行 ID 为 `release-l0ipf0gz`、`undefined-lww0x7ja`、`app-6n1t9bjq`。任务目录 `app-integration-validation/` 保留源码快照、实际自造文件/输入、完整命令与日志、二进制和结果。每个构建/验证进程有 60 秒期限；最终最长进程 5.082 秒。整个 build 脚本也受该期限约束。未运行 Makefile 或全量 runner。

`build.sh` 只使用 reviewed 的显式 clang++ 行、mkdir/mv/cp/printf、标准库 provenance helper 和 `codesign --sign - --timestamp=none`；没有启动、资源扫描、下载或音乐脚本。新增绝对 `EMPEROR_BUILD_DIR` 配置，已有 App 包时明确拒绝覆盖，旧包保留。当前包内 `Contents/Resources/source-manifest.json` 保存 19 个生产源码/头文件/构建元数据的 SHA-256、8 个编译源和签名前 binary hash，随后签名封存。外部 package-manifest 绑定签名后每个包文件与冻结源码清单；receipt 和复核器核对测试与 App 使用完全相同的当前源码。

签名后当前 App binary SHA-256：`3da77a3ee6e30112bb0dc7cd3b89bf141c43e16f3b38a5df73bb62b3e4b551f7`。现有旧包 binary SHA-256：`28c8d1ab4a3d19b231a089d3dda46a9e847ec726138f7f2b1e42eb2e4afe7869`。二者不同，旧包没有包内源码指纹，没有启动或覆盖旧包；不能从当前源码倒推旧包已包含哪些修复。新包可明确绑定共用 AppMedia、WAV/JPEG 修复、明确资源根及默认静音实现。

索引见 [app-integration-receipt.json](app-integration-receipt.json)。新包位于该 receipt 的 `final_app_relative_path`，相对于本任务目录；没有复制 GameData 入包。修改后的脚本需指定一个新输出目录，例如 `EMPEROR_BUILD_DIR=/absolute/fresh/build make game-app`；此说明不授权启动或读取真实资源。

## 真实依赖清单

以下是当前调用关系，不能以测试模块的存在替代 App 接线。

```text
AppKit delegate (main.mm)
  -> AppMedia (资源根、策略、播放对象所有权与状态)
     -> ResourceRoot.resolve -> 有界 file bytes
     -> jpeg -> loadJPEG/decodeJPEG -> ImageIO + CoreGraphics -> BGRA
     -> raw555 -> loadRaw555/decodeRaw555 -> surface_lab CPUStore -> BGRA
     -> playSfx -> decodeWAV -> AudioFactory(bytes)
     -> music -> 有界 bytes -> AudioFactory(bytes)
     -> AudioPolicy -> gain/静音/显式播放/清理
  -> AVAudioFactory -> NSData copy -> AVAudioPlayer [仅编译，没有实际播放]
  -> imageFromPixels -> NSImage -> NSImageView [仅编译，没有 GUI 验收]
  -> 现有 SG3/map/text 预览入口 [只检查资源根接线，不运行或扩展]

runtime_core::Memory -> heap_native callback model [独立数据测试已通过]
runtime_core::Contexts [独立数据测试已通过]
runtime_core/state_codec.py [独立纯内存 TLV 测试已通过]
  -> EmperorApp: 没有 include、链接、Python 调用或生产消费者

audio_lab/video_lab/metal_lab/av_lab/stream_lab/media_demo
  -> EmperorApp: 没有队列、时钟、帧、Metal presenter 或 Controller 接入
视频/Bink/业务状态/游戏调度/保存载入 -> EmperorApp: 未实现接口/消费者
```

当前 App 的对象与图像缓冲依赖 ARC、std::vector 和系统框架，未要求 runtime_core 的逻辑地址 heap、context 或 TLV。不能通过添加未使用的头文件/库、虚构 CPU/城市行为或制造游戏绑定来宣布接入。此次选择并关闭的真实依赖是 App 音效到当前 WAV 解析器、策略到播放器生命周期、图片/路径到生产加载器；runtime_core 没有可据此补接的真实 App 消费合同。

剩余实际阻塞：当前范围只允许不启动的 App 构建与无设备集成，因此真实 GUI/NSImage、AVAudioPlayer 解码/播放/回调/设备恢复不能验收；自造 WAV 不证明真实 MP3/资产兼容；有限 PCM16 合同会拒绝其他 WAV 格式。App 本身仍是资源浏览器，没有业务引擎、游戏状态/调度/交互/存档接口，也没有影片页或有效 Bink 实现。本报告当轮没有读取旧Swift或执行城市业务；当前普通城市接口/报告分析已允许，具体受阻原指令恢复/差分路线仍不重试或绕过，见 [EXECUTION_SCOPE.md](EXECUTION_SCOPE.md)。完整可玩目标在此存在真实缺口，不再把更多颜色/队列小样例记为整游戏进度。

## 干净目录复现更新

同一生产 App 源码通过新的仓库 portable runner 在含空格的干净源码导出/无关cwd复现。AppMedia 两配置仍各62断言通过；App只构建/签名通过，签名后二进制仍为3da77a3ee6e30112bb0dc7cd3b89bf141c43e16f3b38a5df73bb62b3e4b551f7。当前复现入口、native事件和runtime_core需求复核、逐项阻塞见 [BLOCKERS-AND-HANDOFF.md](BLOCKERS-AND-HANDOFF.md)。没有扩大GUI/设备/原链范围或新增业务行为。

本页记录交接前版本验收；该源码已发布为dd6d6fe8e8841c505669ca740a088214241f991d。后续事件接口增量尚未发布，四项补充回归失败见 [BLOCKERS-AND-HANDOFF.md](BLOCKERS-AND-HANDOFF.md)。
