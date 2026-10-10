# 播放通知与图像呈现失败接口

2026-10-10。音频自然结束/解码错误原先只修改 AppMedia 状态，界面要等下一次用户动作才刷新。本次增加 setStatusChangedCallback，所有状态入口统一通知；App delegate 用弱引用订阅并刷新标签/按钮，销毁前取消订阅。回调和状态都用独立副本，允许在回调内取消订阅及重入，析构清理不再通知已结束的消费者。

原生 delegate 将事件投递到主队列，投递时重新检查弱 helper 及 listener/ticket，已退役对象不会保留一个可调用的旧 listener。AppMedia 的合同仍是单一串行控制上下文，未支持任意并发调用。

新增 ImagePresenter / presentImage，把像素范围验证、清除旧图、转换失败清除部分结果与成功结果统一为明确合同。JPEG背景、RGB555贴图、地图和SG3图素均实际调用同一接口；地图成功说明只在转换成功后设置，图素序号无效会清图并显示失败。接口用 fake 转换器注入失败，不为了验证而启动 AppKit。

最终源码验收：无设备集成 Release/UBSAN 各118断言通过，含完成/错误通知恰好一次、旧ticket不通知、另一轨道继续Playing、取消订阅、自取消和重入状态快照、析构先注销；四类自造像素均验证转换失败清除旧/部分图像、后续有效输入恢复、非法尺寸/短缓冲不调用转换器。原四项、六 false 场景和重入生命周期仍通过，两配置六终止场景均只通知一次。App仅构建、签名和系统链接检查通过，包内源码指纹包含新增呈现头文件。

完整源码/输入/构建/结果摘要见 [app-feedback-receipt.json](app-feedback-receipt.json)。复现仍使用 scripts/independent_runner.py integration --mode release|undefined 和 scripts/lifecycle_runner.py；输出显式指定绝对新目录，每进程60秒。

原生主队列投递、NSImageView/文字实际呈现和真实AVAudioPlayer只检查接线与构建，本次未运行。因此fake回归不证明GUI、听觉或设备恢复。未读GameData或原二进制，没有新增城市更新、视频/Bink或存档能力。本次在独立发布checkout继续实现，原活动目录和索引没有更新。
