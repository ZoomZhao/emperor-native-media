# 自写现代文件读取基础验收

已实现独立 `file_io/`：有界 +15 zlib 适配器、显式 profile 的单块/多块读取器，以及只消费自造夹具的驱动。实现使用 host z_stream，不复制原 56 字节 ABI、原 codec 或原对象；没有接入游戏执行候选。

格式依据来自固定 `79726e8` / codec `bc10de8` 的独立保存来源审核，**64 组通过、0 失败**，FIRST-TAG-ZERO-01 闭合；原 f9d56f8 的 56/1 失败记录保留。压缩块支持完整 marker / LE32 N / 未知四字节 word / N payload；未知 word 不当产量。原 wrapper 未要求 codec 吃完声明 N，因此 payload 内 tail 被记录，第二 member 不合并解码。来源审核只认证限定的静态合同，不认证真实文件或本组件运行。

实测记录见 [machine-readable 结果](native-file-reader-acceptance.json)。Release 与 UndefinedBehaviorSanitizer 分别执行 **1,215 项检查，均零失败**；本机系统 zlib 运行版本为 **1.2.12**。生成器使用系统 Python/zlib；没有安装或联网。此前第一轮 Release 1,204/0，以及修正字典需求消费诊断前的两次 1,215/0 均保留，不能作为当前代码版本的证据。

复查旧结果发现：系统库返回 NEED_DICT 时，total_in 尚为零，但已消费 zlib 两字节头和四字节 dictionary ID。适配器已改以本次输入量减 avail_in 记录实际 consumed，并补充 FDICT 的六字节消费与尾部计数断言；该问题只影响失败诊断，没有发布失败输出。修正后重新完成两构建验收，不把先前绿色计数当作这项语义已正确。

| 验收范围 | 已观察结果 |
|---|---|
| 单块 | 独立夹具 151 个可审预期，各以完整、1/2/3/4/7/4096 交付，共 1,057 次检查。合法 stored/fixed/dynamic、原样、空、恰容量 bytes/count 与参考一致。 |
| 字段与长度 | 覆盖 77 个 frame 逐字节截断、31 个 codec 前缀截断，以及 N=0/36144/36145/80000000/FFFFFFFF。字段/payload 不完整或输入超限均在 codec 前拒绝，实际调用次数验证。 |
| codec 与输出隔离 | 60 个直接 adapter/guard 检查。坏 CMF/FCHECK/window/Adler、字典、reserved BTYPE、raw-DEFLATE/gzip 与超容量不发布输出。坏 Adler 实际产生私有前缀后失败，目标与 guard 保持不变。 |
| tail 与 opaque | 五种 opaque 值均不影响有效产量；payload 内零/垃圾/第二 stream 与恰输入容量 tail 均结束于第一 stream，分别记录 frame 消耗与 codec consumed。 |
| 流政策 | 11 个原 pending 案例各七种交付，共 77 次现代政策检查：相邻/混合块、空块后正常块、128 空块预算、好块后损坏块、总输出预算、未知 tag 与块外垃圾。先前成功块可已交付，坏块不发布；终止结果重复稳定。 |
| I/O 和 API | 2 个外层合成容器检查、16 个不同位置/异常 mask 的 I/O 注入、2 个 EOF/truncation 异常 mask 检查、1 组 null/host 整数上限拒绝；合计 21。I/O 异常未被变成 EOF。 |

原始独立夹具 **166 案例：151 单块预期、15 pending** 没有被改写为全部通过。新增流结果只验证明确的现代 frame-event API 政策。两项零字节请求、恰请求量后的预读、外部初始化 near-U64 累计计数仍待相应 API；本组件尚无该接口，四项不计 PASS。原零/等量 buffered request 可能 fill 的来源事实也没有被套用成新 API 行为。

实际原文件兼容仍未验收：没有真实文件/存档/GameData 输入，没有认证当前 marker/root、真实外层头/定位、所有块终止协议或未知 word 意义。纯 EOF/未知 marker 的现代处理不是原终止规则的证明。旧 Huffman/distance/history、异常/callback/alias 与实际对象生命周期极限保持。没有原 EXE/衍生游戏执行、运行绑定、转换、原 codec 测试、UI/音频设备操作或上传/push。

接下来最有用的三项工程工作与通过标准：

1. **独立实现复审与复跑**：另一作者核对源合同和本组件，复跑两构建的自造输入；通过标准是 bytes、actual count、codec 前拒绝、私有失败输出和重复终止结果均独立复核，无 sanitizer 报告，保留任何失败快照。
2. **限定真实外层格式配置**：只沿已获准原来源闭合 marker 初始化/当前资格、打开和定位、块序列与终止；通过标准是每个 profile 字段都有固定来源，构造常量与实际当前对象值严格区分，未知尾部不猜恢复，不读取真实文件来填静态资格。
3. **若需要，再设计字节请求 API**：先明确零请求、恰边界预读、空块进展、后块失败时累计产量与事务范围；通过标准是独立定义 policy 后才关闭四项 pending，用自造源验证计数上限/无进展/错误隔离，不能声称等价于原 unchecked 行为。

本轮只有本地实现与提交；远端具体载荷审批仍独立待定，旧审批不自动覆盖本轮 HEAD。
