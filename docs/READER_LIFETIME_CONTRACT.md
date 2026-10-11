# 文件读取器的输入生命周期

独立审核固定 `16b4c8c8489b9003db8b31e1a66f9275bf52e91b`：在 source 与 output 存储独立且有效的支持域，Release/UBSan 各1,215/0，独立手工/分配/状态测试各44/0。FDICT consumed6、直接C适配器四种 slice重叠、错误隔离和真实151-case执行均独立核定；发现 **READER-BORROWED-ALIAS-01：P2公开生命周期前提缺口**。

如果 custom streambuf 借用 vector X 的 storage，同时调用 `reader.next(X)`，成功 `output.swap(decoded)` 会把 X 的旧 buffer 交给 local decoded，返回时释放；下一次 stream read 将仍引用过期 buffer。独立全局delete监测确认旧存储释放，第二次read前的guard拦截并抛异常，reader返回IoError。**没有实际过期内存读取，也没有ASan UAF报告。** 原独立证据、最小复现和补丁SHA保留在本批记录，不将正常输入绿色结果推广为任意借用alias安全。

本轮补齐公开header/README约束：借用 `FramedReader` 的 source及backing storage必须覆盖reader生命周期、每次调用期间有效，且不能与任何 `next(output)` 的output vector共享存储。这个模式明确不支持；opaque `std::istream` 无法一般地检查底层存储地址，文档修正没有伪装成自动alias检测或修复所有借用源。独立释放监测负例保持，用来验证风险与拦截过程，不计作“借用alias已可用”。

另新增 [OwnedFramedReader](../file_io/include/owned_reader.hpp)，复用同一个核心parser：

- 输入参数按值接收：lvalue复制；`std::move(bytes)`转移，随后用private vector支撑private stream。
- 原vector可以复用作output；复制路径也允许原输入vector修改或销毁，不影响owned source。移动后调用者仍须遵守存储转移约定。
- 对象禁copy/move以保持stream/reference/get-area地址稳定；需要转移对象可用unique_ptr。
- 输入物化/参数复制可能在构造前抛bad_alloc；parser限额不约束调用者先前物化的完整输入，也不等于encoded+decoded+scratch+hostwindow等整个进程的heap上限。

直接 `emfr_decode_zlib` 在private scratch完成读取和cleanup后才提交目标，支持域内的重叠input/output slices已被独立验证。不能把这个单次slice事务结论套到长期借用流存储。

本轮真实验证结果见 [记录](reader-lifetime-contract-evidence.json)：

| 测试 | Release / UBSan |
|---|---|
| 原151独立case×7交付及其余核心检查 | 各1,215/0；151-case部分各1,057次符合预期。 |
| 合入的独立reviewer44项回归 | 各44/0，allocator拒绝、FDICT、直接slice alias和sticky状态均保持。 |
| unsupported borrowed alias释放监测 | 已观察释放；guard在过期read前拦截，返回IoError。 |
| 新owned入口作者回归 | 各6/0：copy/move并复用原vector、原输入修改/销毁、后块截断、累计预算、empty container End。新wrapper尚待独立复核。 |

ASan环境仍不可用：未链接reader/codec的空main在runtime初始化失败，exit−6/`asan_init_is_running`。保留stderr；**不称ASan通过，不称这是组件内存诊断**。本轮使用UBSan及安全释放监测，没有通过变更保护或执行过期读取绕过该限制。

复跑只构建自写组件与系统zlib、使用自造输入，输出须为全新绝对目录：

```sh
python3 -B file_io/run_independent_review.py --output /tmp/emperor-independent-NEW
python3 -B file_io/run_owned_review.py --output /tmp/emperor-owned-NEW
```

两核心CPP保持16b4c8c原字节；新入口是header包装，未改原格式/错误映射、未扩展字节请求API。原15pending和四项API未关闭；4ac4fc8的外层profile文档保持冻结，待独立源审核。本轮不读原EXE、GameData或真实存档，不执行原/派生游戏，不作新REA查询、UI/声音设备、安装、网络/上传/push。
