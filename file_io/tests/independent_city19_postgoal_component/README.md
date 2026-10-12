# Flood/Festival 独立专项

来自cfea23e独审的12项自造组件检查，覆盖变动goal终点、Flood固定51984数组、Festival嵌入顺序/稀疏byte、尾部不读、旧goal不可变、精确65536边界与前置状态。只给runner加明确源码目录/优化拒绝，给稀疏字段断言加索引保护，未改生产header。

```sh
python3 file_io/tests/independent_city19_postgoal_component/run_properties.py --source-root "$PWD" --output /tmp/emperor-postgoal-properties-new
```

输出目录须不存在；总180秒、编译60秒、命令10秒。当前Release/UBSan各12/0，3.333秒。仅自造输入，不读原数据/PE、不启动分析器。原独审70/0、专项12/0、审计22/0通过；此专项不替代新Religion/Commerce独审。
