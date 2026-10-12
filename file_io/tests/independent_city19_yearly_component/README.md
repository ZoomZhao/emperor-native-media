# Yearly 增量组件专项检查

这8项来自 eca8ca7 的独立审查补丁，验证旧flags停止、Yearly20的固定opaque字段、未启用Population时停止、Housing/Yearly缓存类型分离、引用消费、专属版本错误、tight item预算与截断后新调用状态。输入完全自造，直接调用现代header；不读取真实保存或EXE。

纳入时只要求明确源码目录、增加Python优化拒绝，以及测试失败时的索引保护；未修改组件逻辑。

在仓库根运行，输出目录须不存在：

```sh
python3 file_io/tests/independent_city19_yearly_component/run_properties.py --source-root "$PWD" --output /tmp/emperor-yearly-properties-new
```

使用已安装clang/macOS SDK，Release与UBSan两种构建；总预算180秒，每次编译60秒、执行10秒。输出逐项结果及源码/命令/输出摘要；python -O在建立输出前拒绝。

原独审针对 eca8ca7（Yearly实现48d4016）：新套件各33/0、独立专项各8/0、源码与收据14/0，无新增阻断缺陷。本次在Population最终header SHA256 `ba28a2f5a601613ac230465104999e0e37579c16be048ad6a8e8bd0a95f9ad0b` 下重新验收专项各8/0、共16/0，耗时 2.983 秒。优化拒绝 exit1，输出目录未建立。这8项不代替Population新映射的独立审查。
