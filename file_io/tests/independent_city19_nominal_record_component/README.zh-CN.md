# 1479B 显式名义模型的独立验收

基线 `78f1b7ec4d141cb2008f8633a29b1fe083338dc5`，现代源码与 `40f0977e2587188f647591af4947aa34159764e5` 字节一致。此目录可作为新增测试补丁放入 `file_io/tests/independent_city19_nominal_record_component/`，不修改生产组件。

```sh
python3 file_io/tests/independent_city19_nominal_record_component/run.py --source-root . --output /tmp/emperor-independent-nominal-fresh
```

macOS、Xcode Command Line Tools；输出目录必须不存在。总预算180秒，单次编译60秒、执行10秒。Release/UBSan各9个案例，合计18次案例执行。只用一个源码include根，记录实际四个头文件、测试、驱动、期望与生成golden头文件的SHA256。输入全为主机自造内存，不读原EXE、真实payload/GameData/旧Swift，不启动REA或guest代码，不操作UI/设备。

独立golden66来自已审计的父请求operand，嵌入word+raw4/raw12来源为保存52c480的52c4cf/52c4dc；20B存储大小没有参与18B线宽。每个案例的期望来源在test-expectations.json。

覆盖显式contract不可省略、verifies_original_instance=false、旧API始终pending/嵌入消耗0、父子分段组合、66+2字段来源映射、prefix平移/opaque输入、最晚完整起点2617与4096边界、18个嵌入不足前缀、未知contract先拒绝、现代版本端序、失败后独立实例恢复和只读自造记录后接保护页。

成功只证明用户显式选择的名义1479B字节模型；不是当前原实例类型绑定。旧1461B API仍停在52d5f6前，原52c480未按输入版本拒绝的事实不被现代own1限定改变。当前+30、child+14和外部callback/root保留仍须另证，真实前沿1991913不变。
