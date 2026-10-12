# 独立 own6 前缀验收

期望基线 `9b7e975accdb6567f045b0e38809cb18953fa455`。本目录作为新增补丁放入 `file_io/tests/independent_city19_next_owner6_component/`，不改生产代码。只编译现代 C++ 并生成主机自造字节，不读取原 EXE、GameData、真实存档或旧 Swift；不新建 REA 查询/seed，不执行原方法。

从仓库根运行（macOS、Xcode Command Line Tools，输出目录必须不存在）：

```sh
python3 file_io/tests/independent_city19_next_owner6_component/run.py --source-root . --output /tmp/emperor-independent-owner6-fresh
```

总预算180秒，每次编译60秒、执行10秒。Release/UBSan各10个案例，合计20/0表示20次构建案例执行、10个独立案例。仅一个源码include根；结果绑定实际组件、布局、GoalCursor依赖和测试/驱动/期望/生成golden头文件摘要。失败保留日志并以非零退出。

66项golden来源是独立抽取**已保存**52cd90选定own6路径的PUSH长度与CALL地址，而非被测layout。每个期望来源在 `test-expectations.json`。1459B raw加word2为1461B；最后raw为52d5e4/1B。完成后必须保留pending=true、嵌入消耗0、pending_source_call=52d5f6。其他案例验证前缀平移和opaque值、最晚完整起点2635、完整视图预算含未读尾部、非null空视图、SIZE_MAX保护、逐字段平移截断、版本端序、失败后独立实例恢复、只读自造内存与后接保护页。

4096预算和版本6限定属于现代子集契约。原own3/4/5/6表、默认兼容路径不同，不能称原实现拒绝这些其他版本。raw长度来自请求operand，不来自对象内存偏移或reset REP范围。

此测试只验收own6前缀。52d5f6的+0x5c4虚调用未绑定，不消费未知嵌入字节，不代表完整52cd90记录、真实到达或游戏通过。当前+30类型/root仍未决，真实前沿1991913不动。
