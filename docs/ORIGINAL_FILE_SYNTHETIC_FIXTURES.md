# 严格块读取器的独立合成夹具

固定来源：`79726e882de14e6195816a806cef93b5aedb5ac1` 的选定codec、fill错误传播、缓冲短读与城市流入口合同。`tests/original_file_fixtures/source_contract.json`列七份Markdown/JSON的SHA和预期依据。只读已发表合同，没有读取主线的新读取器实现，没有实现/复制parser、适配器、旧codec、原指令或原对象。全部payload和marker均为本批自造，不包含原EXE、GameData或真实存档。

这是**现代安全解码政策的合成资料与参考观察**，不能据此宣称原存档兼容、原wrapper错误码一致或完整文件已载入。生成器使用系统Python自带zlib；参考观察和自检通过只验证夹具，不是Native读取器通过。新代码没有游戏/UI、guest内存、原代码/派生原代码执行、安装、上传或真实声音设备操作。

## 明确边界与等待证据

| 项目 | 本资料的处理 |
|---|---|
| 完整首marker | 强制提供四字节；分别覆盖0/1/2/3字节。原首tag未预置零，短读不能零填后假定tag。 |
| 压缩块 | 按marker、LE32 N、额外LE32 opaque word、N payload构造；固定名义input/output上限36144。 |
| 原样块 | 按marker、LE32 N、N原样bytes构造；严格短读/上限拒绝是现代政策建议，原fill没有逐项检查。这里的raw块不是raw-DEFLATE。 |
| opaque word | 已消费，语义Unknown。变体0/1/实际产量/36144/FFFFFFFF不得当作预期输出长度或要求与产量相等。 |
| zlib terminal与产量 | 需要+15包装、完整stream结束/Adler、无需字典、产量不超上限；成功available=actual produced，坏块available=0且不消费部分输出。 |
| payload内部尾随数据 | 完整第一stream后tail不单独导致失败；分别记录整个frame N与codec consumed。第二个zlib member是第一stream的tail，不能自动合并产量。 |
| frame外尾部/多frame/空块/物理EOF | 整文件结果全部保留pending。`79726e8`没有闭合终止marker、跨块请求、空块跳过、预读及末块规则。 |
| 零请求 | 原缓冲路径可能fill；原raw helper提前返回不能推广到新API。无副作用no-op是否为新政策仍待明确，所以两案例pending。 |
| 资源与累计计数 | 单块36144界有合同；可配置零/64输出容量是现代建议。总输出、块数和累计计数接口未定，pending案例携带配置提案/逻辑边界，不分配巨大物理输入。 |

默认marker是ASCII `SZL1`、`RAW1`，明确为**合成配置**；不是原F+14/F+18常量，也不是外层文件头`fedcbaaa`。支持通过CLI指定两个不同四字节marker，以便主线使用新适配器的显式配置；这不认证实际文件值或外层物理起始偏移。

`expected.status=ready_block_assertion`仅表示可审查的**单块**预期；`basis`区分来源文档提出的Native拒绝政策、本批原样/可配置资源政策提案、以及等待证据。即使单块accept，`whole_file_termination`仍为pending。`pending_evidence`案例不可被测试报告计成PASS，须先补来源/明确Native API政策；不能通过随意选EOF值使它们变绿。

## 文件与生成方式

```sh
python3 -B -m unittest discover -s tests/original_file_fixtures -p 'test_fixture_generator.py' -v
python3 -B tests/original_file_fixtures/generate_fixtures.py --output /tmp/emperor-file-fixtures-NEW
```

输出必须是尚不存在的绝对目录，不覆盖已有资料。每个case有`input.bin`、`case.json`；单块接受案例另有`expected-output.bin`。总`manifest.json`保存case内容SHA、字节数、预期、构造字段位置、交付读分片建议、参考codec观察、系统Python/zlib版本和本生成器/来源元数据SHA。同一zlib运行版本内生成内容可复现；跨zlib版本不假定压缩字节SHA相同，须重新生成并保存运行版本。

参考观察只对自造payload调用`zlib.decompressobj(+15)`。输出最多capacity+1，用额外一字节发现溢出；capacity=0时仍传max_length=1，避免Python的max_length=0变成无限输出。没有调用无界flush，没有解码1MiB炸弹的全部输出。逐字节输入使Adler失败前已返回前缀可见；观察中的错误文本只作诊断，不用其英文文本或现代错误数字强配旧wrapper状态。参考库的stream eof也不等于整个framed文件EOF。[Python zlib接口](https://docs.python.org/3/library/zlib.html)、[zlib格式RFC1950](https://www.rfc-editor.org/info/rfc1950/)、[DEFLATE格式RFC1951](https://www.rfc-editor.org/info/rfc1951/)只支持标准格式/参考API说明，不补原游戏规则。

## 覆盖与主线接线设计

- 合法stored/fixed/dynamic、原样、terminal零产量、恰36144产量，opaque word与产量不相等。
- 两个规范frame每个字节位置的物理截断；完整frame中的codec每个前缀；缺marker/长度word/opaque word/payload与缺codec stream-end分开。
- 原样/压缩声明N比实际payload大、声明N截短stream、N=0/36144/36145/80000000/FFFFFFFF；声明上限在读取/分配/codec前拒绝。
- 坏FCHECK、方法、window、Adler、FDICT、reserved BTYPE，以及gzip/raw-DEFLATE包装拒绝；坏Adler已有部分输出仍不发布。
- output恰上限/多一字节/1MiB膨胀；输入恰上限带tail和多一字节；零/64配置产量预算。
- payload tail零字节/垃圾/第二stream；frame外尾部；原样/压缩混合多块、好块后坏块、空块后正常块、128空块；零请求、等量请求边界、累计/块数预算等pending设计。

主线应写自己的adapter测试包装，读取`case.json`作为**fixture声明**而非格式parser，配置synthetic marker并传`input.bin`。完成一个块后对照expected bytes/actual available，另断言错误分类可见、坏块available为0、输出guard不变、声明界先检查、输入只消费声明N、codec consumed与frame consumed分开。生成器不能代替这些断言，也没有在本批执行它们。

对每个就绪case，重复传输最大正分片1/2/3/4/7/4096及一次完整交付；先用完整逻辑输入证明“正短读后仍有剩余”与永久EOF/截断不同。严格`read_exact`如何重试、底层错误怎样传播应由新API明确，不复制原fill的未核查传播。另设计断言：负/超请求返回计数在消费前失败；I/O异常不能变成EOF；重试次数/无进展循环受限；小型输入中逻辑计数接近SIZE_MAX/UINT64_MAX时先做算术检查。此处是接线测试设计，尚无被测API，因此没有假造计数回调或默认通过stub。

多块原子性尤其需要补充：好块后坏块时，是否允许调用者已经消费首块前缀、是否返回累计短量、是否回滚本次请求；块级available=0不能擅自推出整流事务回滚。零块是否继续读取与exact request是否预读也应留待主线的新规则。完成这些决定后，另提交带来源的新expected版本，保留本批pending快照。

## 验证交付

自检覆盖生成结构、所有截断位置、参考库stream-end/尾随/损坏/容量观察、未知word不作产量、pending未被转成成功、落盘SHA及不覆盖旧目录。生成**166案例：151项就绪单块预期、15项等待证据**；不是151项Native测试通过。两个规范frame的每个截断点共77项，完整frame中codec前缀截断31项。生成器14项自检通过，结果和命令记录在独立交付manifest；无主线读取器、原存档或原游戏通过结论。补丁只添加本资料、生成器、自检与来源元数据，不修改城市核心、main.mm、legacy、现有codec合同或主目录/index。
