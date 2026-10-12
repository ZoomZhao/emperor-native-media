# 六类 FigureFSA 自造验证

只消费自写现代组件。输入由夹具独立写入，无原数据、原代码、工厂、GameData 或设备访问。最终 Release 22/0、UBSan 22/0；覆盖六类新对象、短／扩展 class tag、回引用环、每个截断点及失败不改缓存。来源和接收政策见 [合同说明](../../../docs/city19-figurefsa-new-payloads-20261012/README.zh-CN.md)。

从仓库根目录运行，输出目录必须尚不存在：

```sh
python3 file_io/tests/figurefsa_objects/run.py --output /absolute/new-output-directory
```

SDK 查询最多 10 秒，每次编译最多 40 秒，每个自写测试进程最多 20 秒。runner 保存命令、stdout／stderr 和源文件指纹。默认仅接受六个确切类的 schema 0、payload 自身版本 1；未知类型明确失败。测试结果不证明实际当前类型或完整加载。
