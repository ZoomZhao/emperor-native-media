# 数据资格工具的未消费长度诊断修复

独立审查在提交 ca8956c 找到 `QUALIFIER-UNPARSED-N-01`（P2）：12 字节自造输入中的未知 marker 后四字节没有被 parser 消费，旧工具仍把它们输出为声明长度 N。这既伪造解析事实，也违反失败尾部仅输出摘要的约定。原始合成结果 42/10（每种 native 构建四个 N 诊断失败，另有两个 Python 优化门禁失败）保留在独立审核包。

N 现在缺省为 JSON null。只有已识别的 compressed/raw marker，且 parser 确认消费了完整八字节 marker+N 时，才输出已消费的 LE32 长度。未知 header/marker、block limit、长度截断不再从未解析尾部取数；完整已读 N 的 payload 截断、encoded limit 保留真实声明值。后续 JSON 消费者须接受 nullable N。

独立提供的 52 例在整合副本通过：Release 24/0、UBSan 24/0，加四个共享只读/门禁检查，合计 52/0。五个入口的启动门禁回归仍为 6 个 unittest 方法、13 个离散情形通过。Python 工具保留 47dd940 已加的优化拒绝与显式哈希/路径条件，没有重复套用较弱的建议补丁。

修复后工具对同两份精确输入只读复核：地图 15 块、471164 字节；dat 14 块、430024 字节。所有有效帧 N、输入 SHA、累计解码 SHA、块数、产量、现代 End 与空尾部均与此前收据一致；EOF 事件 N 现在为 null。此前真实 29 块证据结论保留，本次重算与保存证据审计明确区分。

五个生产 reader/codec 文件与外层 profile 不变。没有原程序/原 codec/衍生游戏执行、输入写回、原始或解码字节入库、设备操作、联网或推送。现代块 End 不证明原始加载器整体成功或城市/存档兼容。详见 [证据](qualification-unparsed-length-fix-evidence.json)。
