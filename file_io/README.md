# 独立有界文件读取组件

自写 C/C++17 组件，使用已安装系统 zlib 的 host ABI。只实现来源支持的选定块结构与明确的新安全政策；没有旧 codec、原指令、guest 对象、游戏运行绑定或 GameData 依赖。

`include/strict_zlib.h` 提供一个 +15 zlib stream 的 C 接口。输入、输出均有调用者限额；所有失败保持目标字节不变并返回 `output_available=0`。`output_produced` 只诊断私有失败前缀，不可供 consumer 使用。成功必须 stream-end 和 cleanup 成功；报告实际 consumed、produced 与 payload 内 tail，不自动解码第二个 member。需要字典、raw-DEFLATE、gzip、坏校验、截断与超容量均明确失败。

`include/framed_reader.hpp` 提供 `FramedReader(std::istream&, FrameProfile, StartPosition)`。marker 与限额必须由调用者显式提供，组件没有原文件 magic 常量。压缩块为 marker4 / LE32 N / opaque4 / N payload；原样块为 marker4 / LE32 N / N bytes。opaque word 只保留为元数据，不当产量。输入使用阻塞 `std::istream` 的完整读取语义；非阻塞源应先另写合格适配器。

`StartPosition::RequiredFrame` 从已限定的块起点开始，首 marker 不完整（含空输入）即失败。默认 `ContainerHeader` 首先要求 profile 中的完整四字节外层头；完成头后允许零块。**这套外层政策用于自造容器验收，尚未认证真实原文件。** 接入实际文件还需独立确认当前 marker、外层序列和文件打开/定位条件。

每次 `next(output)` 明确返回 `Frame` / `End` / 错误：

- 每个完整块（包括零产量块）返回一次 `Frame`，不会把空块误当 EOF。
- 仅完整块边界的底层 EOF 返回 `End`；未知 marker、部分字段/payload、I/O 异常都是错误。不会猜原终止 magic 或错误恢复位置。
- 单块与总输入、总输出、块数均有限额；N 在分配前检查。累计加法由剩余预算限制，不从文件声明分配无界内存。
- 块成功后才交换调用者 output。后块失败不会撤销已经返回的前块；失败和 End 均保留调用者 output，终止结果可重复且不继续读。
- `input_offset` 是已确认交付的字节计数；I/O 异常时底层可能先消费部分字节，不能把该诊断值当可恢复的物理 seek 位置。

原 wrapper 错误数字和 advertised length 的未检查传播没有被复制；这里的失败隔离是新的修复政策。没有按请求字节数读取的 API，因此零请求、恰请求量后的原预读与外部 near-U64 counter 注入仍未实现。

验收命令（输出目录必须是尚不存在的绝对目录）：

```sh
python3 -B file_io/run_acceptance.py --output /tmp/emperor-file-io-release-NEW --mode release
python3 -B file_io/run_acceptance.py --output /tmp/emperor-file-io-undefined-NEW --mode undefined
```

命令只构建本组件与夹具驱动，调用另一线程提供的生成器构造全部合成数据，不运行根 Makefile 或游戏/media 目标。七种交付分片（完整、1/2/3/4/7/4096）由测试 streambuf 实现；注入 I/O 异常覆盖默认和抛异常的 istream。测试专用 symbol indirection 记录实际 codec 调用次数，不改变生产源码逻辑。

151 个单块预期的原始夹具与 15 个 pending 条目保持原样；11 个流案例另按本组件明确的现代政策验证，四项字节请求/外部计数语义继续 pending。具体运行证据与限制见 [验收记录](../docs/NATIVE_FILE_READER_ACCEPTANCE.md)。夹具作者独立于实现；当前组件验证由实现作者执行，尚待独立代码/运行复审。
