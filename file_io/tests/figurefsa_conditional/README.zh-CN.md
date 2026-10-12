# 自造 FigureFSA 条件组件验证

夹具由本测试独立写入自造字节；不读取 GameData、原二进制或实际文件，不运行原代码或工厂。结果适用于显式选择的 NULL／既有缓存对象回引用合同，不支持新类／新对象 payload，不证明实际动态类型。

已保存 Release 14/0、UBSan 14/0，包含全部版本组合、每个截断点、预算、缓存关系及只读输入等检查。冻结记录见 [验证摘要](../../../docs/city19-figurefsa-conditional-reader-20261012/validation.json)。本次文档收口没有重跑。

在测试执行得到明确授权的环境中，从仓库根目录运行以下命令，输出目录必须尚不存在：

```sh
python3 file_io/tests/figurefsa_conditional/run.py --output /absolute/new-output-directory
```

SDK 查询最多 10 秒；每次编译最多 40 秒；每个自写测试进程最多 20 秒。runner 保存 stdout、stderr、命令及源码 SHA。它编译新组件与现有自写 GoalCursor；任何新类／建对象输入都会明确拒绝，不借原工厂补全。
