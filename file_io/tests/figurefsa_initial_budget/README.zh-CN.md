# 初始追加缓存边界与原子性

只验证现代资源合同，不读取原二进制、真实输入或设备。76字节输入完全自造，含NULL引用且不追加对象：分别检查prefix1、prefix6020下初始追加1024接受、1025拒绝，以及失败赋值保留原输出/cache。

```sh
python3 file_io/tests/figurefsa_initial_budget/run.py --source-root . --output /absolute/new-output-directory
```

输出目录须不存在。旧基线每模式2/4，修复后每模式6/0；旧失败保留在任务目录。1024/8192是现代合同，不是原实现数值。驱动使用固定Clang target与Make转义解析，不使用shell引号语法。
