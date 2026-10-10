# 实际 App 静默连续预览

App 影片页已通过 `AppMedia::createMovieSession` / `MovieSession::request` 消费既有 `media_demo::Controller::perform`。保留 `moviePoster` 首帧加载，新增播放、暂停、停止与 seek。默认不自动打开文件或播放；输入经 ResourceRoot 解析，预览不输出声音。本项只接通既有生产接口，没有新增播放器、时钟或城市模型。

## 交付和生命周期

- Controller 只在专用串行 worker 使用。主线程 NSTimer 最多排队一个 Tick，现有 HostClock 决定实际媒体位置。
- 非 Tick 命令立即提升 revision 并取消前一 OpenControl。`processMovieDelivery` 先检查 revision，再触及 ImagePresenter，旧帧不能清除或替换新图。
- 会话析构先使独立生存令牌失效；已完成 worker、仍滞留主队列的交付不再访问会话、presenter 或用户回调。公开方法和交付须在主线程调用；异步 presenter 的生命须延续到交付或会话销毁。
- App 按命令数量统计在途请求，换片、首帧加载、清空和终止重置旧 tick/请求计数；先释放旧会话再打开新会话。控件查询使用 owner 缓存路径状态，不在发出 Stop/seek 取消前等待 decoder worker。
- Reader 成功关闭、坏输入、Stop、EOF及正常 close 路径已检查受控 Reader/Video/PCM/Presenter 资源归零。返回 Snapshot 的 BGRA 和 UI 图像不包含在 `resourcesZero()` 中；此结果不证明所有 native 清理失败或长期无泄漏。

## 像素与音频边界

当前固定验收输入为自造的8秒、64×48、30fps H264/LPCM MOV，解码 PCM 为48kHz双声道 S16。Reader 打开检查时长、尺寸和单视频/音频轨，实际消费时逐包核对 PTS/解码输出；没有通用 MOV/Bink 能力声明。

首帧入口返回64×48紧密 BGRA。连续预览沿用既有 EpochPresenter 的32×24、133字节行步长及 `{-1,-1,34,26}` 裁剪/缩放目标；共用交付每行取128字节，丢弃5字节 padding。该固定预览布局不等于可变窗口渲染合同。

PCM 由既有有界离线 Player 消费并丢弃，最多2个来源；不调用 App AudioFactory，不创建 DeviceMonitor。构建只保留 `device_monitor.hpp` 的事件类型，未编译原生监听翻译单元，也未新增 CoreAudio/QuartzCore 链接。

## 验证

独立消费器 Release/UBSan 各通过50项合同检查：真实自造 MOV 后续帧推进、行布局、暂停/seek/接近 EOF/停止、真实排队旧交付拒绝、两次重开/关闭、坏替换与恢复、转换失败清图、销毁后回调抑制及音频工厂零请求。首次受限环境均在 AVAssetWriter 启动夹具时失败；相同二进制在已授权正常系统环境通过，原失败日志保留。

无 Git、含空格路径、无原型头的发布导出，Release/UBSan 集成和 App 仅编译/打包/签名/系统链接通过。证据绑定冻结源码、自造 MOV 实际字节、产物与进程；每进程最多60秒。较大集成断言数主要来自已有逐像素检查，不代表功能数量。

本轮证据目录为 `app-movie-playback-followup-20261010/`，包含 source-before/author-source、independent-repair、actual-repair、owner-query-repair、own consumer accepted-binding、final-validation、movie-consumer-delta及发布/同步收据；详细中文审计报告保存在该任务目录。

## 仍缺少的消费者和验收

实际 EmperorApp 窗口、NSImage呈现、NSTimer体验未运行。完整有声播放仍缺实际 PCM 输出 sink、硬件时钟关联及设备事件到 App 会话的消费者；当前 HostClock 是单调系统时钟。离线 PCM 不证明听觉质量，离屏 Metal 读回不证明窗口/vblank或完整游戏通过。有效 Bink、原资产兼容和原城市逻辑/存档均未验证。

三个可继续推进的工程任务，应仍使用自造输入和同一生产接口：

| 任务 | 具体通过标准 |
|---|---|
| 换片/关闭异步化 | 复用现有 OpenControl；在无UI受控阻塞500ms时，owner的换片/停止请求在100ms内返回，迟到交付拒绝，完成关闭后资源归零；不另写时钟或播放器。当前 open/close仍以同步 fence结束worker。 |
| 实际消费者的错误理由合同 | 将资源路径拒绝、Reader解码/PTS失败及图像转换失败作为明确结果交付到同一App入口；用完全自造坏MOV/事件分别触发，替换清图、旧会话释放、恢复成功；不把已有模块错误测试当新接线通过。 |
| 输出布局参数化 | 将既有 Presenter 的尺寸/stride/矩形作为明确帧布局交付，替代适配层固定32×24；自造奇数尺寸/padding/alpha帧经同一消费者逐字节匹配独立标量期望，裁剪保留原映射，守卫不变，seek/关闭拒绝旧布局结果。 |

禁止恢复、变相续跑或绕过受阻原二进制执行/探测/城市差分路线。本轮未读取 GameData/EXE/反汇编/旧Swift，未执行原游戏或衍生库，未启动项目GUI、操作真实音频设备、安装或上传。
