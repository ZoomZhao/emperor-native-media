# 原生媒体工程进度

> 2026-10-10 自写驱动构建增量：显式 authored 作用域生成 build_scope.h，现成 native_driver.c 仅编译检查可在仓库及无 Git 导出中复现；关键入口／返回绑定／目标绑定仍未解析，未链接未执行。203 函数及配置／域文件在本机旧任务中存在，未收录发布仓库；后续有界控制器也已有实现，真实 App 接入及 phase8 行为仍未验收。见 [AUTHORED_DRIVER_COMPILE.md](AUTHORED_DRIVER_COMPILE.md) 和 [CITY_BUILD_HANDOFF.md](CITY_BUILD_HANDOFF.md)。

> 2026-10-10 启动入口增量：未配置资源目录时现有main调用明确目录选择器；取消正常退出，显式错误配置不回退。共享启动合同/集成两配置各135断言及App只构建通过，真实选择窗口未运行。见 [APP_LAUNCH.md](APP_LAUNCH.md)。原活动目录未同步。

> 2026-10-10 App反馈增量：播放状态通知和共享图像呈现失败合同已接入；集成两配置各118断言、生命周期及App只构建通过。见 [APP_FEEDBACK.md](APP_FEEDBACK.md)。真实GUI/设备仍未验收，原活动目录未同步。

> 2026-10-10 最新发布增量：当前提交已修复同步终止回调后 false 返回的空指针、状态覆盖与提前释放，最终源码所有相关回归和 App 只构建通过。见 [CALLBACK_REPAIR.md](CALLBACK_REPAIR.md)。以下失败记录保留为历史证据；原活动目录未同步，七个暂留路径未合入。

代码发布基线：[dd6d6fe8e8841c505669ca740a088214241f991d](https://github.com/ZoomZhao/emperor-native-media/commit/dd6d6fe8e8841c505669ca740a088214241f991d)，private main 已核对。该版本包含交接前独立修复，来源范围见 [COMMIT_SCOPE.md](COMMIT_SCOPE.md)。当前完整游戏尚未实现。

## 已做

- 已提交：WAV格式扩展与截断JPEG修复、RGB555像素转换、显式资源根、默认静音与失败清理、App共享媒体服务、有限数据模块、可迁移自造验证runner及源码/输入/产物/结果指纹。
- 工作区增量：完成/解码错误监听器和ticket、AVAudioPlayerDelegate桥接，以及地图/SG3/555/文本通过AppMedia统一读取。这些增量目前尚未发布。
- 首轮增量与代码基线的完整差异已审阅；新增集成测试只使用自造格式字节、PCM和fake事件，没有使用真实资源。
- 七个保留路径已做静态内容/来源核对，仍原位保留。它们未混入最新冻结验证和本轮源码提交。

## 已验收

| 版本/配置 | 实际证据 | 结论范围 |
|---|---|---|
| 交接前干净导出，data Release/UBSAN | 各91断言与11项TLV测试通过 | 有限纯数据合同 |
| 交接前干净导出，media Release/UBSAN | 各65,691断言通过 | 自造WAV/RGB555/JPEG/资源路径合同 |
| 交接前干净导出，integration Release/UBSAN | 各62断言通过 | AppMedia生产服务与fake后端 |
| 交接前App Release | 编译、打包、签名、系统链接通过 | 未启动App |
| Antigravity首轮报告 | 七配置结果及源码/输入/产物/包哈希已只读核对；integration两配置各90断言 | 既有自造情景通过，不能代替新增生命周期验收 |
| Codex首轮独立复跑 | `integration-release-9jhew69q`，90断言通过 | 已静态审阅的当前生产服务/fake情景 |

交接前的完整绑定凭据见 [portable-reproduction-receipt.json](portable-reproduction-receipt.json)。首轮独立审查记录见 [callback-review-receipt.json](callback-review-receipt.json)。文本样本为ASCII；未验收中文多字节兼容。

## 未验收或验收失败

独立UBSAN补充回归 `event-regression-5fg2hkvk` 编译成功、测试退出1，以下四项均false；原90断言没有覆盖这些情景：

1. 音乐仍在播放时SFX完成，汇总状态错误变为Stopped。
2. SFX `play()` 内同步DecodeError后返回true，启动逻辑覆盖错误状态并保留播放器。
3. 音乐恢复时同步DecodeError被后续逻辑覆盖为Playing。
4. 恢复回调在后端自己的 `play()` 返回前销毁后端对象。

该轮增量尚未通过审核，未提交/推送该失败实现。缺陷证据作为独立审计记录收录；回归源码见 [callback-regression-source.mm](callback-regression-source.mm)，它针对凭据列出的未发布接口快照，不属于已发布基线的自动测试目标。失败证据绑定了冻结源码、自造PCM/事件、测试源码、二进制和实际进程结果。

真实App GUI、原生音频回调运行、听觉输出、硬件时钟与设备拔插恢复尚未验收。GPU读回和离线PCM结果分别只证明对应像素/样本合同。没有把它们记为完整游戏通过。

## 尚缺能力

- 音频：同步/重入事件的完整事务、后端方法执行期间的对象存活、重叠通道状态一致性、明确的回调线程/交付/在途失效合同，以及真实设备恢复证据。
- 平台消费：heap/TLS/TLV没有App业务消费者；当前有限模型通过不等于生产业务已接入。
- 完整运行：游戏主循环、业务状态与调度、命令/输入消费者、存档/恢复接口尚不存在。没有编造城市规则或行为。
- 视频：App没有影片消费者、PTS/seek/EOF/取消/呈现接口；有效Bink解码没有实现或验收证据。

城市恢复保留为未完成目标：历史六函数/728字节曾在一个固定输入匹配有序写、映射内存与public CPU；扩大到203函数的集合只有构建证据。当前App/有限数据接口缺少城市业务消费者。详见 [CITY_CAPABILITY_AUDIT.md](CITY_CAPABILITY_AUDIT.md)。

当前执行范围和历史限制来源保留在 [EXECUTION_SCOPE.md](EXECUTION_SCOPE.md)；本页仅记录进度事实。

## 当前协作与Git状态

2026-10-10 02:11:01 UTC，专用会话仍显示Working，已读取进度文档与独立复现源码；当时五项增量尚未产生进一步修复差异，未出现完成回复。不能仅凭Working确认后台写入或修复完成。

后续只更新本页并审查实际产出，不发送新聊天任务。没有已验证的文档轮询/自动唤醒机制。当前文档写入不表示Antigravity已读或正在执行新事项。Codex负责独立验收与已授权提交推送；Antigravity不自行stage/commit/push。

未提交：上述五个源码文件，以及 `game_lab/run.sh`、`game_lab/boundary_tests.mm`、`runtime_core/dependency-map.json`、`docs/SOURCE_REPAIR.md`、`docs/source-repair-static-audit.json`、`docs/RUNTIME_PREPARATION.md`、`docs/runtime-preparation-static-audit.json`。保留项的旧样例/历史来源分类不属于当前验收输入；没有读取其引用的原文件或归档。
