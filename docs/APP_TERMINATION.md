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
