# cHero virtual+24 与后续 own6 前缀

真实解析边界保持 **1991913**，本轮新增真实已解释字节 **0**，未读取真实存档。原版载入依赖审计取得两个增量：完成 cHero 名义 virtual+24 的静态体和安装/销毁调用者核对；为外层下一项 `52cd90` 建立 own6 前缀的独立现代组件，Release/UBSan 合计 **24/0**。当前 cHero `+30` 的实际类型及新消费者的嵌入 reader 仍未绑定。

## +30 并非只默认置空

承接[前轮报告](../city19-init-and-later-source7-20261012/README.zh-CN.md)，`510d30 ->512230` 会分配 `0x2d8`，成功时调用 `514500`，在 `512291` 把返回值安装到 receiver `+30`；分配失败才写 null。本轮已识别的直接调用者：安装方法 `512230` 仅列出构造方法 `510d30`；销毁方法 `5122b0` 仅列出析构方法 `510de0`。这些是静态分析识别到的直接调用者，不排除未识别或间接调用，也不证明本次载入前的对象存续。

名义 receiver 虚表 `7b5b20` 的 virtual+24 为 **512a20**。前轮默认分析没有识别它为函数；本轮依据已保存虚表入口，用官方 seed 文件在新的任务临时数据库中标注该精确入口，取得完整静态体。输入 PE、已有数据库、工具实现及所有权保护均未改动。种子是入口断言，不是运行时发现；本轮分析 profile 为 `14a3741e33f4b0d3a3b99834b36c68dd78b98b5bcdba5d32d6d1cd169bd0a2d0`。

`512a20` 中 EBP 持有 receiver；完整体在 `512aef` 读取已有 `+30`，并调用 `4f2c40`。未见直接写 receiver `+30`，未见直接调用安装 `512230`、销毁 `5122b0` 或分配 `40ae80`。它在 `512ce8` 直接调用成员方法 **512630**；该方法完整体也未见上述直接写入或安装/销毁/分配调用。因此本轮线索不能称作直接 setter 或重新安装对象的方法。此结论只覆盖已查方法自身；其他 callee、outer 虚调用、全局对象和别名的副作用仍未排除，未解读业务分支含义。

reset `5149c0` 的 computed virtual+0x2c 已按名义子对象表绑定到 **4f28e0**。默认分析仍未识别完整函数，连续指令前缀在 `4f28f4` 调用 `4f8060`，涉及子对象 `+0x78` 的输入；不能把该槽当作 no-op。完整 `4f28e0`、`4f8060` 及 reset 其他 callee 的 root 保留仍是具体依赖。前轮所得 FigureFSA 元数据、名义 virtual+8 `4dad10` 也不能替代当前指针/root 的证明。

当前绑定仍要求分配成功、正常返回、有效存储和生命周期、reset/callee 保留 root、无后续替换或销毁，以及无同流/跨元素/全局别名副作用。**没有找到已证明的后续重新安装入口；不能由此断言不存在。**

[当前类型依赖清单](hero-current-type-dependencies.json) · [静态会话与证据 SHA](static-session-receipt.json)

## 外层下一消费者 52cd90：own6 前缀1459B+word2

version19 外层调用点 `5306d6` 以 receiver `c5cae0`、同一 archive 调用 **52cd90**。其读取路径先处理版本记录 `1158dc8`、调用 reset `52cb80`，再按 own version 选择分支。静态表把 own3/4/5/6 映射到 `52d1fc/52da0c/52d5fd/52d1fc`；其他值进入 `52de05` 的兼容分支，不能说原实现拒绝其他版本。

现代组件只接受 own6。从 `52d1fc` 到嵌入调用之前，完整连续路径只有 **66 个 raw 请求、合计1459B**；含版本 word 为 **1461B**。每个长度与调用点逐项绑定，无嵌套调用和分支；内存偏移、reset 的 REP 范围及任何对象大小都没有被用作线宽。

下一项是 **52d5f6：receiver+0x5c4 的 virtual+0**。它的当前 root 与线布局未证明。静态映像该槽为零，不能当成当前 null；精确槽地址的两个静态引用 `42e921/42edfe` 都只是 ECX 参数设置，不能称作 root 写入或构造。reset 也没有提供已证明的嵌入 root 初始化。嵌入调用正常返回后才进入 `52e409`，设置 own version6 并返回。

新增 `city19_next_owner6_prefix.hpp` 和 `city19_next_owner6_layout.hpp`，返回字段区间和明确的 `pending_embedded_reader=true`、`embedded_bytes_consumed=0`、`pending_source_call=52d5f6`。它不调用原函数、不重现 reset/状态副作用、不接入实际存档 CLI。4096B 有界视图、完整读取和限定版本6是现代组件策略，不代表原实现具有同样验证。**前缀完整不等于 52cd90 完整记录或真实数据验收。**

Release、UBSan 各 **12/0**：覆盖66项/1461B、连续区间与末项、嵌入调用保持未决、全部1461个不足前缀、非零起点、未知嵌入/后续字节未读、输入不变、其他版本拒绝、预算边界、越界及整数极值起点、null 非空视图。输入全部自造，最终结果与组件/布局/测试/驱动 SHA 绑定。

重现命令（从仓库根运行，输出目录须不存在）：

```sh
python3 file_io/tests/run_next_owner6_properties.py --source-root . --output /tmp/emperor-next-owner6-properties-fresh
```

[请求与外层图增量](next-owner6-prefix-source-binding.json) · [最终验证](synthetic-validation.json)

## 下一步与停止条件

1. 按已确定入口完成 `4f28e0` 与 `4f8060` 的 root 副作用证据，再检查 reset 其余传入 child/subobject 的 callee；未闭合的路径逐项列明。通过标准是明确 root 保留条件及排除项，不能以正常返回假定无写入。
2. 对 `512a20/512630` 余下 receiver、outer、全局 callee 查找具体 `+30` 安装/销毁路径及与 `51208b` 的先后关系。没有唯一或明确有条件的类型绑定时，继续保留真实边界，不猜读。
3. 为 `52cd90` 的嵌入 `+0x5c4 / virtual0` 追溯构造/root/读取来源。取得来源后再补现代组件，并用自造数据验证全记录；否则保持1461B前缀停止状态。

本轮静态会话 exit0，临时项目已清理，固定输入及已选工具模块指纹保持不变；未运行原游戏、衍生库或原 DLL/codec，未使用 SR/Unicorn/Wine，未访问旧 Swift、UI、键盘或音频设备。现有提交/推送授权持续有效，本轮无新增审批请求。
