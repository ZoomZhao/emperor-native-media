# 最短初始化链：+14 来源与首次 reset 的集合边界

真实边界保持 **1991913**，新增真实已解释字节 **0**。本轮找到child+14的实际赋值链，并发现base初始化已注册多个节点；不能把首次reset当成仅访问一个新建空节点。现代生产头文件未变，纳入通过独审的名义模型测试补丁，针对当前源码重放 **Release9/0、UBSan9/0，合计18/0**。

## 可证伪的新结论

child初始reset在 **4f2852** 清零+0x14，但随后base root7affe8的virtual+0x34调用 **4d78d0**。该方法在 **4d9c61** 调4f5c30构造80B对象：分配成功时EBP为构造返回值，失败时EBP为null。**4d9c91→4f2d00** 把它写入child+0x14。因此，中间清零不是最终不变式；这个来源也不同于child+2d0注册路线和node+14字段。

4f5c30安装root **7b3d8c**，其virtual+0x30为 **4f5de0**。完整三指令路径读取自身vptr，调用virtual+0x10 **4f5e50**，再RET4。它未直接转发或解引用栈上的child参数；4f5e50的完整体以及可能的存储引用/全局副作用仍未核查，不能当作无写入getter。80B是分配大小，不能推算线宽。

选定初始化的+14候选是：分配失败的null；成功构造、存续及root保留条件下的7b3d8c对象。后续替换或别名来源尚未界定，不编造其他具体root。

## 有限对象来源与必要证明

容器constructor **554bc0** 将begin/end/capacity（offset4/8/12）置零；getter **4f8210/4f8200** 分别读取begin/end并RET。node base4f5340在 **4f53d2** 清零node+14。它们证明构造时初值，不自动覆盖之后的注册和关联调用。

base初始化4d78d0包含 **19处4db900构造、13处4f2aa0注册、14处4f5500和24处4f5520关联调用**，末尾还在4da93c调用virtual+0x2c。调用点数不是活对象或唯一对象数，但已足以否定“首次reset的+18只有derived新节点、容器全为空”的捷径。

选定+18候选来源包括base的4db900/root7b0034注册集合，以及derived5149c0的4db900/root7b0034插入分支。4f5490仍可重入4f2ff0，并对node+4容器及node+14指针调用virtual30。具体成员关系和相关写集合未闭合，不能跳过这些实际注册callback。

插入链也进一步追到 **5f01f0→5c1670→5c6260→525890→5258d0→42b5b0**：42b5b0返回给定目的地址，5258d0对该地址复制一个源dword，没有pointee虚调用。其他分配、复制、移除与别名条件仍保留，未作全局无写入结论。

读取类型需要证明的范围是：holder+30/root在选定入口的来源和存续；实际初始化callback中与child/holder/root有关的写入；相关版本和完整读取合同。无需证明后续所有游戏状态或规则目的，普通payload值仍保持opaque。但实际可能改指针/root的调用不能省略。

**下一条最有信息量的缺口：4f5e50完整体及其存储引用写集合。**

[初始化来源、候选与必要条件](minimal-initial-type-contract.json)

## 其他独立图项：493f00 的主调用合同

version19主caller在5306ff以receiver c75090、第二参数 **1** 调493f00。读模式先处理版本word，再在493fab调用 **this+4的virtual+4**；flag1跳至494027，跳过flag0的11B raw尾部、this+9d20的virtual+8和own2的491760补段，随后设置内部own3并返回。

这已闭合外层主调用的控制合同，避免未来多消费不属于该调用的尾部。this+4当前root和读取线宽尚未绑定，因此没有编造整段长度或新增猜读组件。精确use点439ee1/439ee6/439ee7设置内层receiver、传参并调用 **491890**；它是使用点，不能称构造。该项仍在真实未达后缀。

[493f00外层合同及下一依赖](consumer493f00-outer-contract.json)

## 验证与会话口径

独审补丁逐文件与私有交付字节匹配。新增测试验证显式contract、original-instance=false、66+2请求来源、旧API pending、18B子段截断、平移、预算、版本端序、失败隔离和只读保护页；只使用自造数据。旧1479B名义模型不等于原实例绑定，旧1461B API仍pending、嵌入消耗0。

```sh
python3 file_io/tests/independent_city19_nominal_record_component/run.py --source-root . --output /tmp/emperor-nominal-independent-fresh
```

[采用与18/0重放](independent-nominal-test-adoption-and-replay.json)

两次成功静态阶段均exit0并清理临时项目；中间一次因任务Unix socket路径过长失败，已核实身份后以官方取消流程清理，仅缩短任务TMPDIR重试。失败会话不计为源码结果，预算、工具实现和所有权保护未改。

[会话、指纹与路径修复](static-session-and-transport-receipts.json)

供独审的前两笔提交及本地报告：

- `40f0977e2587188f647591af4947aa34159764e5`：`/Users/zoomzhao/Workspace/emperor-native-media/docs/city19-root-effects-and-embedded-20261012/README.zh-CN.md`
- `78f1b7ec4d141cb2008f8633a29b1fe083338dc5`：`/Users/zoomzhao/Workspace/emperor-native-media/docs/city19-owner-reset-and-node-callback-20261012/README.zh-CN.md`

本轮未读真实存档、运行原游戏/衍生库/原DLL或codec、使用SR/Unicorn/Wine、读取旧Swift或操作UI/键盘/音频设备。当前唯一类型与完整FigureFSA合同尚未闭合，继续保持1991913，不猜读；无新增审批请求。
