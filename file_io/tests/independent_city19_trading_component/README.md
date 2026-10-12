# Trading 增量组件专项检查

这 8 项来自 `ebe76c166b83e1c7093f00b74a5e1da0b1849c33` 的独立审查补丁，验证默认/Homage-only 停止、Trading 12 字节固定字段、Yearly 未启用时的声明停止、两种引用、checkpoint 毒尾和 schema 范围。输入全部由测试自造，直接调用现代 header；不读取 GameData/EXE，不链接原游戏或其衍生库。

采用时只修改 runner 的明确源码目录与 Python 优化拒绝，以及测试失败时的索引保护。组件逻辑未修改。

在仓库根运行，输出目录须尚不存在：

```sh
python3 file_io/tests/independent_city19_trading_component/run_properties.py --source-root "$PWD" --output /tmp/emperor-trading-properties-new
```

runner 使用已安装 clang/macOS SDK，分别构建 Release 和 UBSan；总预算 180 秒，编译每次最多 60 秒，执行每次最多 10 秒。输出逐项结果及源码/命令/输出摘要；`python -O` 在建立输出前拒绝。

在 Yearly 提交 `48d40164ef514c8a7a9e2482c9f35093df733530` 的 header（SHA256 `f89731ce7014284385973c1079c0087d5d4acbbb640267dded51523b94d824a2`）下重新验收：Release 8/0、UBSan 8/0，耗时 2.986 秒。优化拒绝检查 exit1 且未建立输出目录。

原独审针对 ebe76c1：作者新增套件各 30/0、独立专项各 8/0、离线源码与收据 13/0，无新增阻断缺陷。该独审不覆盖随后新增的 Yearly 布局；Yearly 作者套件各 33/0 与本次旧增量专项验收不能代替对 Yearly 的独审。

这些检查验证现代结构组件的限定行为，不证明原进程 registry/vptr/cache/对象生命周期或完整保存兼容性。Yearly 开启后的真实结构边界为 3678 的 Population 声明，尚未读取其载荷。
