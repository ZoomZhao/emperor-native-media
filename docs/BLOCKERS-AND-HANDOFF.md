# 当前阻塞与接手说明

2026-10-10。目标是当前自写工程的真实集成与可复现验收，不恢复原二进制执行链，不虚构城市行为。当前 HEAD 为 `2f3378a91fb0987288e00c61befa0ddeacbe50be`；该版本此前已推送私有 main，之后的源码修复、runtime_core、runner 和报告仍在未提交工作树。本轮没有 commit/push、下载、联网或上传；原 GameData/二进制没有上传。

本轮新增完成项：修复仓库 runner 与实测入口不一致、固定本机路径、缺输入留存和进程期限的复现缺口；在项目外干净白名单源码导出中实际复现全部指定配置，并绑定当前仓库、冻结源码、实际输入、产物和结果。没有为了接入而把 heap/TLS/TLV 加入 App。

## 1. 已修复且当前实测通过

当前主要证据为 [portable-reproduction-receipt.json](portable-reproduction-receipt.json)。完整证据保留在本地任务输出目录；源码导出索引为 `clean-independent-export.json`，本仓库只保存脱敏凭据。只导出明确白名单自写源码，无 `.git`、GameData、legacy、凭据、旧二进制或历史产物。源码目录含空格，调用 cwd 是另一个无关目录；不用本机旧项目路径。

| 修复与源文件 | Release / UBSAN 当前结果 | 证据目录 ID |
|---|---|---|
| `runtime_core/src/heap_native.c`、`include/memory.hpp`、`include/thread_state.hpp`：有限 heap/私有内存/context 合同 | 各 91 断言通过 | `data-release-3tctrdrb`、`data-undefined-z95ed0fa` |
| `runtime_core/state_codec.py`：纯内存 TLV 类型/预算/失败合同 | 各 11 测试，0 失败/错误 | 同上，两次隔离 Python 执行 |
| `game_lab/wav_pcm.cpp`、`emperor_assets.mm`、`asset_pixels.cpp`、`resource_root.*`、`audio_policy.hpp`：WAV 完整扩展/JPEG 截断校验、RGB555/路径/静音 | 各 65,691 断言通过 | `media-release-1ioxhvyx`、`media-undefined-hfig8cs9` |
| `game_lab/app_media.*` 及真实 delegate 接线：WAV 在工厂前校验，失败替换退役，默认静音/显式播放/终止及图片路径 | 各 62 无 UI 集成断言通过，fake 音频后端 | `integration-release-wu7a4tfw`、`integration-undefined-b8962rzy` |
| `scripts/independent_runner.py`、三个 wrapper：明确相对源码白名单、冻结快照、留存实际输入、失败记录、每进程 60 秒 | 六个测试配置及只构建 App 均成功；最长进程 6.960 秒 | 上述六个及 `app-release-adc85o5e` |

实际 WAV/JPEG 失败回归及修复前失败记录另见 [INDEPENDENT_VALIDATION.md](INDEPENDENT_VALIDATION.md)。此次复现没有增加无关小样例，验证的是同一生产代码能通过可迁移的仓库入口执行。系统框架没有 UBSAN 插桩，未注入所有系统分配失败；断言数量不代替覆盖说明。

当前干净目录 App 只编译、链接、打包并通过 strict 签名校验，8 个自写编译源、系统框架/库。包内 `source-manifest.json` 保存 19 个生产源/头文件/构建元数据指纹。二进制 SHA-256 为 `3da77a3ee6e30112bb0dc7cd3b89bf141c43e16f3b38a5df73bb62b3e4b551f7`，与上一轮当前源码新包一致；原目录旧包保留。新包位置为证据根下 `app-release-adc85o5e/build/EmperorApp.app`。源码一致/签名成功不能证明 GUI、声音或游戏运行成功。

## 2. 已实现或仅构建，尚未运行验收

| 入口/对象 | 当前证据与状态 | 尚不能声称 |
|---|---|---|
| `app/main.mm` → AppKit/NSImage/选择器 | 当前真实源编译；控制调用点与 `AppMedia` 源码/测试绑定 | 未执行 GUI、NSImage 展示/缩放、选择器交互或包外资源读取 |
| `app/av_audio_backend.mm` → NSData/AVAudioPlayer | 当前源只编译，音频 bytes/策略通过 fake 工厂验证 | 未执行真实 MP3/WAV 解码、声音、硬件时钟、完成/错误回调或物理恢复 |
| SG3/map/文本预览入口 | 当前自写入口随 App 编译，使用明确资源根；没有读取真实资产 | 不证明所有图素/文本格式兼容、地图业务或城市逻辑 |
| graphics/audio/video/stream 的独立模型与旧报告 | 有既有有限证据，当前 App 不接入这些 presenter/队列 | GPU 读回不等于完整游戏；离线 PCM 不等于听觉；自造 MOV 不等于 Bink |

## 3. 工程缺失接口与依赖

| 阻塞：具体动作/入口 | 现状证据 | 为什么影响完整运行 | 最小解除条件 | 可并行替代与不等价之处 |
|---|---|---|---|---|
| E1：把 runtime_core heap/Memory 接入 App | App 缓冲使用 vector、对象使用 ARC；无逻辑地址/heap ID 的生产调用点。构建没有 `heap_native.c` | 数据模块通过不产生业务执行或状态消费者 | 提供真正需要逻辑地址/所有权映射的独立业务接口与有限合同，才实现适配器 | 当前内存/heap 合同已验收；添加未使用链接项没有整游戏价值 |
| E2：把 Contexts/TLS 接入 App | `Contexts.select` 只选择有限数据记录，不是 OS/CPU 线程；App 没有该 context 调度入口 | 无执行调度、线程状态/错误传播接口，不能取代业务运行时 | 明确上下文、调度、线程所有权及错误传播的消费者合同 | 可审阅 App 的主线程对象所有权；不能把虚构 context 切换记为引擎调度 |
| E3：将 TLV 用作 App 存档 | Python API 只接受内建数据、bytes；App 无状态 schema、保存/加载动作或恢复消费者 | 序列化器不知道什么业务状态需要保存，无法证明存档可继续运行 | 明确自写业务状态 schema、版本/校验/恢复规则与保存调用点 | 当前纯数据往返/失败测试已完成；不能兼容性推定历史任意快照或存档 |
| E4：把离线队列事件接到 AVAudioPlayer | `audio_lab::Player` 的事件由 offline consume/stop 产生；当前 AV 后端没有完成/错误事件发布接口。SFX 至多保留一个对象，替换/静音/终止清理；自然结束即时退役/状态更新未实现 | 离线事件不能描述真实播放完成、设备错误或物理恢复 | 先定义 native 事件来源、owner 线程交付、对象/ticket 失效和完成退役合同；随后明确真实后端验收范围 | 可审阅既有有界保留策略；fake 完成事件仍不能证明 AV 回调/物理声音，不能直接挪用离线队列装作接通 |
| E5：完整业务引擎与 App 状态/交互 | App 是资源浏览器，只有资源展示/播放动作；不存在可玩业务状态消费者、调度/update、命令/输入、保存/恢复接口 | 没有可驱动可玩流程的程序，素材/构建/平台小测试不能补齐 | 在合法独立实现范围内提供或决定业务接口/规格与消费者；当前不分析城市逻辑，不恢复原链 | 当前媒体接线和可复现构建已完成；它们不等价于启动→载入→交互→保存/退出的游戏验收 |
| E5a：业务状态与调度消费 | `NSApplication.run` 是 UI 事件循环；没有独立业务会话状态、advance/update、暂停/恢复或终止后的任务取消消费者 | 不知道帧/时间/输入应改变哪个业务状态，媒体时钟不能替代业务调度 | 确定合法独立状态合同、owner 线程、时间推进、命令和取消规则，再接 App | 可读现有媒体状态；不把 UI tab 或播放状态伪装成游戏状态 |
| E5b：保存/载入与恢复调用点 | App 无保存动作、状态导出、格式版本或恢复到业务消费者的入口；TLV 仅处理数据 bytes | 没有完整载入/继续/保存/退出闭环 | 明确待保存状态与版本/恢复事务、资源重新建立合同及生产调用点 | 当前 TLV 测试已完成；成功解码任意自造 dict 不能证明游戏可恢复 |
| E6：视频/Bink 接入 EmperorApp | 当前无影片页面、Reader/PTS/seek/EOF/取消/呈现消费者；有效 Bink 解码与 sound/EOB 生命周期没有实现证据 | 不能播放或驱动影片流程 | 明确影片消费者合同；有效 Bink 还需合法、可用实现和自造有效媒体验收，不从原 DLL/衍生库恢复 | 旧自造 MOV/帧模型可审阅；不能用 MOV 冒充 Bink 或用缺失文件失败宣称解码通过 |

没有为了制造消费者而添加 heap/TLS/TLV/离线事件。当前范围内可合理关闭的缺口是生产媒体接线和独立复现入口，已完成；上述缺少业务合同或真实后端验证条件，不能用更多颜色/队列小样例填空。

## 4. 原执行链的平台拒绝，与授权/预算的区别

| 阻塞：动作 | 现状证据与可信范围 | 影响 | 最小解除条件 | 可并行替代与不等价之处 |
|---|---|---|---|---|
| P1：原 EXE 转换/探测/反汇编、SR/Unicorn/Wine、原/衍生游戏库执行或变相恢复 | 最初 `<codex_delegation>` 明确记录原工作曾遭平台安全拦截，来源线程 ID `01a10af1-425e-75ae-bded-bf47f843a029`；[EXECUTION_SCOPE.md](EXECUTION_SCOPE.md) 保存该直接限制。本轮未重试 | 原链不能作为获得完整业务实现的后门，也不能转交另一代理执行 | 仅由平台正式支持核查原拒绝记录和适用范围；用户追加授权、剩余时间或换代理不能替代平台结论 | 当前独立源码/自造测试可继续，已完成本轮复现；不证明原行为等价，不解除原链 |
| P2：原拒绝报文、准确报错日期及工具调用 ID 缺失 | 当前可核对的委托只有上述来源线程和“曾拦截”的描述，没有完整拒绝报文/时间/call ID。`2026-10-05/task` 是资料目录名，不作为准确报错日期。既有 stage82/83 报告描述工程 preflight 拒绝，不当作平台安全拒绝原因 | 无法诚实给出精确平台根因/错误日期，更不能猜权限、许可、保护机制或违规类型 | 向官方核查提供已有拒绝原文/准确时间/相关任务或调用 ID；不重新触发原链来补证据 | 已整理可确认事实和范围；工程 gate、编译失败或普通文件错误不等价平台回绝 |

用户继续工作的授权已经存在；旧“文档审计期间不运行独立自造测试”已明确撤销，**不是当前阻塞**。本轮白名单测试和只构建 App 已实际执行。60 秒是每进程上限，不是等待用户批准的时间预算；本轮没有超时或独立测试自动审批拒绝。

GUI/真实音频设备/原 GameData 的使用仍在当前任务范围外，与 P1 平台拒绝分开记录。获准扩展独立自造 App/设备验收也不会解除 P1。

## 5. 工具能力与文件访问

| 阻塞：动作/入口 | 现状证据 | 影响 | 最小解除条件 | 替代与不等价之处 |
|---|---|---|---|---|
| T1：真实 App GUI/音频设备验收 | 当前只构建 App、fake 后端测试；未启动 UI/播放器、未尝试物理设备操作 | 真实交互、听觉、回调与设备恢复仍未知 | 下一阶段明确允许独立自造目录启动 App，以及所需 GUI/音频设备验证；限定时长/证据。原链保持禁止 | 当前无 UI 接线已通过；不能把没有声音设备的 PCM 核对称为听觉验证 |
| T2：真实资源兼容/旧包外读取异常 | GameData 目录本地被忽略，本轮未遍历或读取内容；未测试旧包文件选择/访问。没有可确认的系统权限拒绝原文 | 未知真实资源格式、完整性、读取环境和等待根因 | 合法授权的最小资源/明确错误记录及独立资产读取范围；先定位具体入口，不修改系统权限来猜测修复 | 自造路径/格式/尺寸/失败验收已完成；不等价全部资产兼容，也不证明旧问题是权限导致 |
| T3：Antigravity 代理消息或官方 CLI | 本机 `/Applications/Antigravity.app`，bundle `com.google.antigravity`，2.21.1；只读进程检查确认运行。常用 CLI 路径不存在；包里 language_server/webm_encoder 不当作代理消息 CLI。签名元数据 Team `EQHXZ8M8AV`，Authority unavailable，不自行认证发布者证书。GUI 代理入口/消息协议未验证 | 目前无法可靠投递具名代理，不能猜 RPC/CLI 或误投 | 用户确认准确代理会话或官方交互入口；当前最新要求无需实际发送，仅留草稿 | 项目外 `Antigravity-只读审查草稿.md` 已准备，未发送、未委派、没有传工程数据或全部对话。人工核对入口不等价已完成审查 |

Antigravity 核查证据在本任务目录的 `antigravity-readonly-evidence.json`。默认进程查询曾报 operation not permitted，授权的只读查询随后成功，已不构成阻塞；没有把这一观察扩展成 App UI 或文件访问的根因。未启动/更新该应用、安装工具或操作界面。

## 接手步骤与最小下一决定

1. 保留当前未提交工作树；先读本文件及 [APP_INTEGRATION.md](APP_INTEGRATION.md)。禁止 reset/覆盖修复、把旧包当当前源码或上传 GameData/原二进制/凭据。当前包是资源浏览器。
2. 仅对 receipt 白名单自写源码做干净导出；不要复制 GameData/legacy/旧 App 或历史引擎。从任意 cwd 调用导出目录中的三个 wrapper，明确给出新的绝对输出目录。现有 Apple 工具和标准 Python 足够，不需安装。

```sh
bash <clean-source>/runtime_core/run_boundaries.sh release --output <absolute-new-output>
bash <clean-source>/runtime_core/run_boundaries.sh undefined --output <absolute-new-output>
bash <clean-source>/game_lab/run_boundaries.sh release --output <absolute-new-output>
bash <clean-source>/game_lab/run_boundaries.sh undefined --output <absolute-new-output>
bash <clean-source>/game_lab/run_app_integration.sh release --output <absolute-new-output>
bash <clean-source>/game_lab/run_app_integration.sh undefined --output <absolute-new-output>
python3 -I -B <clean-source>/scripts/independent_runner.py app --output <absolute-new-output>
python3 -I -B <clean-source>/scripts/independent_runner.py verify --report <absolute-report-directory>
```

上述仅为允许的自造数据/无 UI/只构建入口。尖括号是待替换的路径；有空格时需要 shell 引号。`app` 不启动程序；不要使用 `make run-game`、全量扫描/音乐/UI脚本或任何原链入口。每个 runner 生成唯一目录，失败记录保留，验证器要求当前源、冻结源、输入、产物和结果一致。与本轮相同的断言数只应作为合同核对，遇到失败必须报告真实原因。

3. 下一项真正推进 App 行为的决定是：明确独立自造 GUI/真实音频后端验收范围，并先确定 E4 的 native 完成/错误生命周期合同。完整可玩目标另需 E5 的合法独立业务接口/规格；不能授权我们猜城市行为。P1/P2 则交官方核查，不能让 Antigravity 或其他开发者绕过。
4. 本轮没有合理、已明确范围而尚未完成的独立复现/接线任务；不继续无限追加微样例。若要委派 Antigravity，先核实官方投递入口，只发送最小只读审查任务，当前草稿尚未发送。
