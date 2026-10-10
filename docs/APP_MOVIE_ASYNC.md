# 影片异步换片、关闭与错误交付

794b325 的同步换片、首帧读取和关闭会等待 worker/Reader。影片页现使用同一个 MovieSession 的 openAndPlay/requestPoster/closeAsync，ResourceRoot 与媒体读取在原有串行 worker 上执行；控件只读缓存。首帧仍为64×48，连续帧仍为32×24和133字节行步长，不泛化输出布局。

共享 worker 状态独立于前端对象生存。generation 立即取消旧 OpenControl，主队列交付先检查生存令牌与 generation；前端析构仅请求 worker 清理。队列最多一个 active、一个可覆盖 latest pending、一个 latest main delivery；被覆盖或过期请求不回调，App 使用最新请求标志。重复 close 幂等，资源标记在 worker 确认后更新；关闭失败保留原因并可重试。应用退出使用 applicationShouldTerminate 的异步回复，保留会话直到 close 完成；失败则拒绝退出并显示原因。此 Cocoa 接线仅编译验证。

MovieDelivery 携带错误类别和原始 detail，formatMovieStatus 同时供 App 与独立消费器使用。首帧 worker helper 保留 Reader 原因；兼容 moviePoster facade 仍返回无效图像。路径拒绝/资源缺失、metadata 超时、解码、图片转换、关闭均有原因；旧失败不能覆盖新结果。openMovie/performSync/close 是保留的 headless 同步辅助，生产 App 不调用。查询 hasActiveController/resourcesZero 读取原子缓存，不阻塞。

独立 Release/UBSan 消费器各通过127项合同检查：自造8秒64×48 H264/LPCM MOV，真实 Reader/Controller/离屏 Metal、离线PCM；500ms worker阻塞期间替换/关闭/前端析构调用均小于100ms；64次换片与32次Stop突发，实际最大active/pending/main-delivery各为1；加载取消、晚到交付、重复关闭、坏替换/恢复、首帧错误、2秒metadata超时、fake图片转换失败、关闭错误注入/重试，以及持有真实资源后析构的worker确认归零。Release/UBSan观察最长owner调用分别0.016125/0.098583ms，仅代表本次系统与输入。关闭故障由明确自写注入点触发，不代表真实GPU/系统关闭异常已验证。每进程60秒上限。最初受限环境在夹具writer启动前失败，失败记录保留；相同哈希二进制在已授权正常运行环境通过。

可重复命令（仅自造输入，无GUI或设备）：

```sh
python3 scripts/independent_runner.py movie-async --mode release
python3 scripts/independent_runner.py movie-async --mode undefined
python3 scripts/independent_runner.py app --mode release
```

runner冻结明确源码集，绑定实际生成的MOV/坏输入/观察数据、二进制与进程记录；verify校验同一源码、输入和产物。App只构建、打包、验签与系统动态库检查，禁止由此声称窗口/退出交互通过。

未验证：实际App窗口、NSImage/NSTimer/异步Cocoa终止体验、听觉播放、硬件时钟、物理设备恢复、有效Bink或一般影片格式、原资源/原城市/存档。resourcesZero是受控Reader/PCM/GPU计数，不包括返回像素/UI图片，不证明长期稳定。离线PCM不是听觉验证，GPU读回不是游戏通过。

后续三个独立工程任务：

1. 在既有接口补“关闭后重开/首帧后播放”的App事件消费合同。自造MOV与可控事件，主线程无等待；反复退出被拒绝后重试、首帧到播放、clear到播放均只消费最新结果；资源确认和错误文案与实际方法一致。当前GUI接线仅构建，事件合同不能冒充窗口验收。
2. 将既有离线PCM sink与设备变化事件做可替换消费者接口。自造PCM和注入设备事件，队列/循环/格式错误/恢复后时间关联可检查；重复/旧事件不能重建，关闭后队列和事件归零，默认不请求真实设备。实际声音与硬件恢复须另获操作设备的授权。
3. 给既有固定MOV路径补可控慢读取/失败恢复的真实消费验收。只用自造坏MOV、路径拒绝和取消，检查最新原因保留、旧帧不重现、资源上限及有限超时；不做无需求尺寸/stride泛化，不扩展原游戏或城市规则。

本轮不读取GameData/EXE/反汇编/旧Swift，不运行原游戏或衍生库，不恢复被阻断路线，不安装或上传。原型默认关闭，原有未提交修改独立保留。
