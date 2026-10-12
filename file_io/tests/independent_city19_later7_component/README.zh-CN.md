# 独立 source7 组件验收

期望基线为 `36bd99090fe2e28e2ccd1aa42956bc0f8e82592d`。测试只编译自写现代 C++ 并生成自造内存字节，不读原 EXE、GameData 或真实存档，不启动 REA、原游戏或衍生游戏。此目录可作为新增测试补丁放入 `file_io/tests/independent_city19_later7_component/`。不改核心读取代码。

从仓库根运行（macOS、Xcode Command Line Tools；输出目录必须不存在）：

```sh
python3 file_io/tests/independent_city19_later7_component/run.py --source-root . --output /tmp/emperor-independent-later7-fresh
```

驱动只有一个源码 include 根 `--source-root/file_io/include`，测试与期望使用本目录文件。结果记录这三份实际编译头文件、测试、驱动、期望 JSON 和生成的 golden header 的 SHA256；复跑集成版本时要对照这些摘要。编译总预算180秒，各次编译60秒、运行10秒，Release/UBSan 各10个案例。失败或构建/运行异常以非零状态退出并保留日志。

10个案例的真实期望来源见 `test-expectations.json`。245个请求的 golden 来源为此前独立核对过的源 operand 账本，本次再与保存的静态证据核对；包括共享尾部 `59a107` 的4字节。12105是 raw 请求之和，加2字节版本为12107，均不是游戏经济数值。版本7限定及16384视图预算属于现代子集契约，不表示原实现拒绝其他版本或采用此预算。

额外案例验证任意前缀平移、原始请求值不改变范围、最晚完整起点4277、整视图预算含未读尾部、非 null 空视图、SIZE_MAX 在解引用前拒绝、逐字段边界截断、失败后重跑隔离，以及只读自造映射后接保护页。一个案例内的参数枚举不另计断言数量。只读保护页是主机自造内存测试，不操作设备，不等同 ASan。

此测试通过只证明独立 own7 字节请求组件。前置当前 `+30` 类型未决，真实前沿1991913不动；不能据此认为已进入 `593140`、完整存档可读或游戏可玩。
