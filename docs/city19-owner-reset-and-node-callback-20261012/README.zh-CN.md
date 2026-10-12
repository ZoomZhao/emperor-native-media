# 父 reset 范围与选定节点 callback 来源

真实边界保持 **1991913**，新增真实已解释字节 **0**，本轮未读取真实存档或执行原游戏。恢复后先核验保存结果与源码SHA，补齐报告，将1479B名义合同和两份独审测试纳入提交 **40f0977e2587188f647591af4947aa34159764e5**，已推送并直接核对远端。随后继续本轮精确来源核查。

## 已消除：4307f0 的父 root 副作用条件

4307f0完整体无callee，只有固定字段写入及4次固定循环。最晚写入是子对象+0x5c的一字节，结束于+0x5d；传入owner+0x404时，目的区间最晚止于 **owner+0x461**，早于嵌入root **+0x5c4**。

结合前轮52cb80直接体的写入范围和own6的66个目的区间，在有效存储、父REP所需DF0、正常返回及无别名条件下，选定父reset与已查字段请求不改写嵌入root。前轮尚未闭合的4307f0局部callee条件现已消除。实际构造到达、当前对象存续及archive/全局别名仍是独立条件，不能由此宣称实际实例已绑定。

[完整局部写入区间](parent-reset-root-preservation.json)

## 路径修正：+2d0 不是已证明的 +14 安装

既有5149c0路径把4db900的返回值存入child+0x2d0，再传给4f2aa0、4f2a80。完整体明确：

- **4f2aa0** 经名称/查找helper后，在插入分支把传入对象加入child **+0x18** 容器；没有取得直接安装+0x14的证据。
- **4f2a80** 总是写child **+0x8**，非null分支另写 **+0x10**，不直接写+0x14。
- **4f2ce0/4f2cc0** 分别操作child **+0x68/+0x78** 容器，其append helper的跨调用副作用仍未闭合。

因此不能把“同一+2d0指针传入这两个成员”升级成当前+0x14绑定。base构造4d7490继续调用 **4f2610** 和 **4d75d0**；+0x14初始来源仍须从这些精确依赖核查。

## 已缩小：一个 +18 节点 callback 候选

4db900构造在4db93e安装node root **7b0034**；其virtual+0x1c指向 **4f5490**。这提供+0x18 callback的一个具体候选，条件是节点分配成功、正常构造、插入分支实际加入该节点、节点未替换/销毁且无别名。84B分配是存储大小，不是线宽。

4f5490完整体显示：child参数非null时，以child为receiver调回 **4f2ff0**；随后对node+0x4容器元素以及node+0x14指针调用 **virtual+0x30**，并继续传递child参数。这两个外部target的root和参数写入仍未绑定，不能把4f5490当作无副作用setter，也不能以node自身表绑定其容器元素。

另外，4f2e90中4f2f94的直接写入按当前4f2ff0调用点追溯到caller栈计数器地址；在有效帧及无重叠条件下，它不构成已证明的child root写入。其他节点/helper副作用继续列为未决。

[选定节点、注册路径与剩余callback](selected-node-route-and-unresolved-callbacks.json) · [栈计数器写入范围](related-out-counter-write-scope.json)

## 验证口径与下一步

本轮只新增来源证据和文档，现代代码未变，没有重复运行测试。此前1479B显式名义模型 **28/0**、两套独审重放各 **20/0** 的结果与受保护源码SHA保持匹配。旧1461B API仍pending=true、嵌入消耗0；1479B模型仍不验证原实例。

[保存验证与源码不变](previous-component-validation-preserved.json) · [静态会话/证据SHA](static-session-receipt.json) · [1479B模型报告](../city19-root-effects-and-embedded-20261012/README.zh-CN.md)

下一步限定为：4f2610/4d75d0的+0x14来源；4f2b60/5f01f0的注册分支及成员关系；4f5490的node+4/node+14外部virtual30目标和child参数写入；其余已注册容器元素的构造/root。cHero当前+30类型、lifetime和完整FigureFSA读取合同仍未闭合，继续保留1991913，不猜读。没有全局不存在写入、唯一当前类型或完整游戏通过的结论。

静态会话exit0、临时项目清理完成，固定输入和已选安装模块指纹不变。未执行原游戏、衍生库、原DLL或codec，未使用SR/Unicorn/Wine，未读旧Swift或操作UI、键盘、音频设备。持续提交/推送授权有效，无新增审批请求。
