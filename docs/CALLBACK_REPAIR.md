# 当前音频事件与生命周期修复

2026-10-10。本次提交合入已审查的播放器 Finished/DecodeError 事件、ticket 失效与资源根接线，并修复音乐启动/恢复和音效启动时同步终止回调后 play() 返回 false 的空指针及状态覆盖。

返回值只更新仍持有同一 ticket 的播放器；回调、停止或替换已产生的新状态保留。对象在最外层 play() 返回后才释放，内层播放与 stopAll 不会提前销毁外层播放器。没有终止回调的 false 返回仍正确进入 StartFailure。共享退役实现也用于替换、静音音效和自然结束。

最终提交源码已重新验证：未改动的原四项和六场景反例通过；生命周期 Release/UBSAN 均通过，包括有/无终止回调的重入替换与停止；当前 AppMedia 集成两配置各90断言通过。当前 App 编译/链接、打包、签名与系统链接检查通过，未启动。每进程期限60秒，源码、输入、构建与结果哈希见 [callback-release-receipt.json](callback-release-receipt.json)。

可迁移复现：用已有 python3 -I -B 调用 scripts/lifecycle_runner.py --mode release 或 undefined，并指定 --output /absolute/fresh-output；集成与仅构建入口为 scripts/independent_runner.py integration 或 app。仅使用自造字节/PCM/fake事件，没有真实资产、UI或音频设备输入。

地图/SG3/图素/文本现在均经 AppMedia 的资源根与生产解析器，新增集成只验证自造资源读取。地图仍是地块预览，没有城市更新引擎。AVAudioPlayer 设备运行、听觉、回调线程及恢复未验收；本次同步控制回归不证明并发安全。视频/Bink、业务状态、调度和存档消费者仍缺失。

本次从既有远端基线建立独立发布 checkout；原活动目录及索引没有写入，也没有同步为本提交。七个暂留路径及其他无关修改未合入，legacy 保持基线内容且没有检出或执行。播放状态到界面通知与图像呈现失败合同仍是下一增量，不能宣称此处已完成。
