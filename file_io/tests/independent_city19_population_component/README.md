# Population 十成员边界专项

来自acc03b7独审的9项自造组件检查，覆盖准确十成员、有效/损坏第十一成员尾部不读、item checkpoint与空成员头、精确预算、字段类型/cache及截断。CPP保留原补丁，只给runner增加明确source-root和优化拒绝。

在仓库根运行，输出目录须不存在：

```sh
python3 file_io/tests/independent_city19_population_component/run_properties.py --source-root "$PWD" --output /tmp/emperor-population-properties-new
```

总180秒，编译60秒/命令10秒；Release与UBSan各9/0，耗时3.907秒。仅自造输入，不读取保存或EXE。旧独审35/0、专项9/0、源码/收据17/0通过；本次专项不代替新postgoal映射独审。
