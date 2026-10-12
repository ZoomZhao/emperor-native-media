# FigureFSAState 自造 payload 验证

基于源链descriptor84d310/base850230/factory4db820/root7b0034/serializer4dbaa0，新增六组自造检查，并保留此前28组回归。两模式各34/0，输入、cache和字段值全部自造，无原指令或数据。

```sh
python3 file_io/tests/figurefsastate_payload/run.py --output /absolute/new-output-directory
```

输出目录必须不存在；SDK查询≤10秒、单次编译≤40秒、测试≤20秒。实际编译依赖来自Clang depfile。兼容性只沿已证明的FigureFSAState→FSAState方向，具体类型保留；未知类型不纳入目录。版本和真实条件边界见 [报告](../../../docs/city19-figurefsastate-payload-20261012/README.zh-CN.md)。
