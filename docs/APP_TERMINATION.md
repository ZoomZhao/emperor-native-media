# App影片退出协商

基线6edb89987d733cba37cace3b488d4d97e1067172的影片关闭已异步化，但真实applicationShouldTerminate:只内嵌完成/错误分支，没有退出期限；close回调到达时pump可能还在结束，前端析构也会再次排清理任务。

真实入口现在调用MovieSession::requestTermination(5000, completion)，与无UI消费器使用同一代码。只复用已有Controller/Reader/OpenControl、共享worker、生存令牌和1active/1latest pending/1latest main delivery队列；没有新播放器、模拟器或第二worker。单个owner计时器检查单调期限及实际关闭确认。Completed/Failed/TimedOut都有明确结果和原因；permitsExit只在Completed、close已观察、资源归零及worker无媒体工作/队列为空时为true。

pending重复请求返回Pending，保留原callback和deadline，不追加任务/timer；Completed重复请求返回Completed，不回调、不排工作，App可直接返回NSTerminateNow。失败/超时允许显式重试，每次有新generation，旧close/timer不改变新结果。超时报告当时资源与worker状态；后台仍可清理，不把超时当资源归零。owner延迟消费到超过截止时间时也报告Timeout。pending/Completed期间媒体命令不替换退出请求。

真实App保留会话直到结果；Completed给Cocoa YES，Failed/TimedOut给NO并显示共用formatTerminationResult原因，失败恢复控件。没有影片会话直接Now。成功后的前端析构看到同一资源/worker状态，跳过再次排空清理。owner计时器在完成、错误、超时和析构时取消，旧事件先检查生存令牌及generation。

完成时的workerIdle表示实际媒体工作active=0、pending=0、待交付mailbox=0、pump已报告finished，并且Controller不存在、受控Reader/PCM/GPU已归零。它不保证dispatch内部空尾栈/闭包对象已析构：正在交付的安全尾闭包或已取消timer事件可以短暂存在，但不持有原生媒体资源，且在会话销毁后不访问front或用户回调。结果callback可销毁会话；finish之后不再访问成员。

独立Release/UBSan实际消费器各133项检查：自造固定8秒H264/LPCM MOV与真实Controller/Reader/离屏GPU；已有活资源/500ms加载延迟/实际metadata hold/排队旧画面中退出；64次重复退出；关闭错误注入/重试；40ms超时、晚到清理后状态保持与重试；owner延迟消费过deadline；pending析构取消timer、抑制交付并确认后台清理；Completed析构不再enqueue。最大worker/pending/mailbox/timer各1，最长owner调用0.028917/0.111709ms；原40ms期限实测42.7998/42.8034ms，不被重复请求的60秒参数重置。只说明本次输入/环境；检查数包含重复循环。自写关闭故障注入不等于真实系统关闭异常覆盖。

复核命令（每进程60秒，明确源码/输入/产物绑定）：

```sh
python3 scripts/independent_runner.py movie-termination --mode release
python3 scripts/independent_runner.py movie-termination --mode undefined
python3 scripts/independent_runner.py app --mode release
```

新增退出消费器之外，既有movie-async suite仍用于异步媒体兼容合同；不重跑旧全时序/耐久套件当新成果。首次受限环境的writer启动失败在夹具之前，失败保留；相同哈希二进制在已授权正常运行环境通过。无Git/含空格/无原型头导出和App构建/签名/系统链接仅证明源码与接线可构建。

本轮未启动App窗口、未运行真实Cocoa退出交互、未操作音频设备或录制声音；后台清理限于影片Controller的受控资源，不代表App所有音频设备资源已经验收。没有读取GameData/原EXE/旧Swift或执行原游戏、衍生库与受阻的原执行/探测链。现有源码、接口和依赖的静态审阅仍可做；不把具体受阻路线扩大为整个城市工程分析禁令。

## Cmd+Q 不退出：主菜单缺少 terminate: 键等价（2026-10-22）

用户报告：影片播放时在原生 App 按 Cmd+Q 不退出。复现与定位后，真正原因不在影片视图、窗口、模态循环或关闭回调：

- Cmd+Q 不是窗口 key event，而是 `NSApplication` 主菜单的 key equivalent。App 的 `main()` 只创建 `NSApplication`、设置 delegate 并 `[app run]`（`game_lab/app/main.mm:2340`），从未设置 `NSApp.mainMenu`，也没有任何 `terminate:`＋`"q"` 菜单项。没有该菜单项时 AppKit 不会调用 `applicationShouldTerminate:`，`MovieSession::requestTermination`、`_movieTerminating` 和 5 秒期限根本不会开始。
- 影片播放本身不参与该路径：菜单键等价先于 key window/first responder 匹配。用同一 keyDown 事件验证：无主菜单时 delegate 收到 `applicationShouldTerminate:` **0** 次；安装菜单项后为 **1** 次。播放中与空闲时行为一致。
- 应用内「❌ 退出应用」按钮调用 `[NSApp terminate:]`，因此该入口一直可用；这遮蔽了缺失的 Cmd+Q 绑定。

修复：

- 新增 `game_lab/app/movie_quit_menu.hpp`，构造 App 主菜单：应用菜单含「关于」和「退出 皇帝：龙之崛起」，后者 `action=terminate:`、`keyEquivalent=@"q"`（Command）。`main()` 在 `[app run]` 之前调用 `game_lab::installApplicationMainMenu(...)`；App 与回归测试编译同一个构造。
- `applicationShouldTerminate:` 保留原 5 秒 `requestTermination` 契约与 `NSTerminateLater` 回复；新增有界兜底：pending 超过 5 秒＋回复宽限（6 秒）或上次退出被拒绝（Failed/TimedOut）后，用户再次按 Cmd+Q 直接以 `NSTerminateNow` 结束并先在 stderr 写明原因，不再出现「按了没反应、App 仍开着」。新的媒体命令（Play/Pause/Stop/Seek、打开首帧、清空）会清除拒绝状态，下一次退出重新走完整协商。
- 失败/超时仍先回复 `NSTerminateNO`、恢复控件并在状态栏显示共用 `formatTerminationResult` 原因；stderr 同时输出「退出被拒绝：…（再次按 Cmd+Q 直接退出）」。

回归测试 `movie-quit`（25 项检查，自造合成 MOV，无原版资源）：

- 菜单形状：存在 `terminate:` 项，keyEquivalent 为 `"q"`，修饰键仅为 Command。
- 无主菜单时同一 Cmd+Q keyDown 不进入 delegate；安装后进入一次；单独 `q` 不触发。
- 子进程端到端：真实 8 秒合成 H264/LPCM MOV＋真实 Controller/Reader＋`NSApp.run`；无菜单基线在 4 秒 watchdog 内不会退出（exit 4，仅 `playback-live=1`），有菜单时经事件队列 Cmd+Q → `requestTermination` Pending → `permitExit/resourcesZero/workerIdle/noController` 全 1 → `replyToApplicationShouldTerminate:YES` → `applicationWillTerminate` → exit 0。合成音频工厂拒绝设备输出（`audio_devices:false`）。

真实 App 实测（自造合成 MOV，`--data-root` 只指向临时目录，未读取 GameData）：

- 构建产物 `/…/EmperorApp.app` 启动后注入仅用于本次观察的探针，把影片页路径设为 `Movies/own.mov` 并播放；状态栏为「播放中」。向 App 事件队列投递 Command-q：进程 **exit 0**，自启动到退出 5.35 秒，Cmd+Q 到退出约 0.35 秒。
- 同一探针在投递前清空主菜单（模拟修复前状态）：影片仍在「播放中」，Cmd+Q 后 12 秒 watchdog 触发仍存活（exit 9）。这直接证明缺失主菜单项是原因，而不是影片播放阻塞退出。
- 未手动操作可见窗口，未使用辅助功能/自动化权限；强制退出分支（失败后再次 Cmd+Q）未在真实 App 中注入关闭故障，仅由代码审阅与既有失败/超时合同覆盖。

复核命令：

```sh
python3 scripts/independent_runner.py movie-quit --mode release
python3 scripts/independent_runner.py movie-quit --mode undefined
python3 scripts/independent_runner.py movie-termination --mode release
python3 scripts/independent_runner.py movie-async --mode release
python3 scripts/independent_runner.py app --mode release
```

`app` suite 在基线上因 `scripts/independent_runner.py` 的 APP 源清单缺少 `original_scene_timeline.hpp`、`city19_goods1_layout.hpp` 等 12 个既有构建输入而编译失败；本轮把这些文件补入清单（并让新头文件同时进入 `build_provenance.py` 嵌入清单），与本 bug 无关，失败记录保留在此。
