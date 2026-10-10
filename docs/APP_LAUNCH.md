# 未配置资源目录的启动入口

2026-10-10。旧main只接受参数/环境配置，缺配置立即返回2；Finder双击没有选择资源目录的入口。这是已有资源浏览器的实际启动缺口，独立数据测试不能代替该接线。

新增 app_launch.hpp::prepareLaunch，主入口实际使用同一启动决策。命令行--data-root优先于EMPEROR_GAMEDATA_ROOT；二者均未配置时调用明确目录选择器。AppKit只允许单选目录、不创建目录。取消选择正常退出0，不进入delegate/资源加载/播放；明确配置无效仍返回2，不静默回退到另一个目录。help和非法参数在选择器之前处理，没有cwd搜索、自动猜测资源目录、保存偏好或权限改动。

无UI集成Release/UBSAN各135断言通过。新增验收用fake选择器和自造目录/PCM：只选择一次、选择后的目录真正进入AppMedia加载、默认静音不自动播放、命令行/环境优先、含空格目录、参数缺值/重复/未知、空/相对/缺失显式目录拒绝、help不选目录、取消不重试、非目录选择拒绝。既有播放通知和呈现失败用例继续执行并通过。当前App只构建、签名和系统链接检查通过，包内源码指纹含app_launch.hpp。

精确源码/输入/构建/结果绑定见 [app-launch-receipt.json](app-launch-receipt.json)，入口仍是scripts/independent_runner.py integration --mode release|undefined或app，显式指定新的绝对输出目录。每进程60秒，无超时。

原四项/六false场景/重入生命周期的生产及测试源码均与452f9e9逐字相同，相关已发布验收通过摘要按哈希复核，没有重复运行或伪称新测试。新启动代码没有修改播放器控制实现。

原生NSOpenPanel、Finder启动、实际AppKit呈现和音频设备仍未运行；fake启动合同及构建不证明原生窗口/声音通过。没有读取GameData、执行原链或增加城市引擎、视频/Bink、存档消费者。原活动目录与索引仍未同步；当前提交在独立checkout完成。
