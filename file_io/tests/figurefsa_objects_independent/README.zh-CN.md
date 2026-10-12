# 六类新 payload 独立验证

固定审查基线：`c1c75e40f3da71746fa62f1f90305de90c40a4d4` 与 `b40718a44dea33a54a11e87c90f99ccbb30800bb`。三个生产头完全相同。本目录新增的 10 个测试组消费自造字节，使用现代 `walk_city19_figurefsa_exact_objects` API；与已采用的 `figurefsa_independent` 缓存分支测试不同。

每项期望来源见 `expectations.json`：六个 reader 的最小流宽度与字段调用点、class/object 登记顺序、16/17 新对象深度、128/129 新对象数量、精确缓存边界、包含嵌套引用的全局 1024/1025 预算、未知类/别名/schema/版本拒绝、晚期失败保留完整调用方输出，以及全图字节分割与嵌套区间。

默认 1024 字符串预算在其它上限下不可达：六类每个新对象最多一个名称，最多 128 新对象；外层最多 base 名称 1、current 名称 1、名称列表 256，总计 386。测试将上限降为 386/385 验证，不声称到达默认 1024 边界。上述资源限额是现代组件政策，不是原游戏数值。

```sh
python3 file_io/tests/figurefsa_objects_independent/run.py --source-root . --output /tmp/figurefsa-objects-independent-next
```

输出目录必须不存在。主机 Release、UBSan 两模式，各 10/0；总预算 180 秒、每次编译 60 秒、每次测试 10 秒。三个私有变异均在两模式捕获：拒绝正在读取对象的回引用、绕过嵌套引用的全局预算、把 Transition 的 8 字节字段错读为 4。

驱动按 Clang Make depfile 的转义读取依赖，固定 depfile target，保留文件名中的引号并还原 `$$`。没有把 Make 文件当作 shell 命令或 shell 引号语法。本补丁同样修补三个现有测试驱动的解析，生产组件不改。

没有读取原 EXE、GameData、旧 Swift 或实际 payload，没有原/转换代码执行、设备操作或 push。确切六类 schema0/own1 仅是条件合同；未知派生类、其它 schema/own version、实际 registry/factory、holder+30 身份和生命周期仍未由合成测试证明。真实边界 1991913，新增真实解释字节 0。
