# FigureFSA 独立条件分支验证

基线 `46893b8fa44a78fbc5e7a920ca03a2541a42b480`。本目录只有自造输入、测试与期望来源摘要。没有读取原程序或实际 payload，没有执行原代码，也不证明当前 holder+30 的身份或生命周期。缓存 family/class 关系由调用者事先验证；不同 ID 不表示物理指针必定不同。

12 个测试组的具体期望与来源见 `expectations.json`。新增重点包括：跨多个合法列表的 1024/1025 总引用预算、非零 presence 的全部代表值、INT32_MIN、8192/256/1024 精确上限、零预算、完整调用方输出在晚期异常后的保留、每个消费字节的区间分割，以及保存来源中的类型包装与调用顺序。8192/256/1024/65536 是现代安全子集，不能声称原实现使用这些上限。

在已包含这两个头文件的源码快照根目录复跑：

```sh
python3 file_io/tests/figurefsa_independent/run.py --source-root . --output /tmp/figurefsa-independent-new-run
```

输出目录必须不存在。总预算 180 秒；SDK 查询 10 秒、每次编译 60 秒、每次执行 10 秒。仅使用主机 Clang C++17 的 Release 和 UBSan；没有 ASan/TSan 证明。结果记录编译命令、日志哈希、所选源码根与实际 Clang 依赖路径/哈希。源码根只有一个，不把测试旁的缓存头误认作指定根中的文件。

在该基线上 Release 12/0、UBSan 12/0。移除累计预算、把 presence 改成 `==1`、把 State 包装改成 Transition 三种私有快照变异均在两模式被对应测试捕获。变异结果不计入正常通过数。

本补丁另外修补已有 `figurefsa_conditional/run.py` 的来源记录：`--dependency-root` 作为后备搜索目录，可能被 FigureFSA 头文件旁的带引号 include 覆盖；驱动原来却记录后备 Goal 文件哈希。修补后按 Clang depfile 记录实际被编译的头文件，并单列所请求的后备根。没有修改生产头文件。

新类声明仍需独立的 schema/name、类查找/兼容性、类与对象缓存注册顺序、工厂及各动态 serializer 合同。已有 class tag 新建对象同样会消费 payload、添加对象缓存项，并可能通过注册前后的引用形成对象图；不能以 cache hit 或内存 class size 推断字节数。当前组件拒绝这两条路径，实际边界仍为 1991913，新增真实解释字节为 0。
