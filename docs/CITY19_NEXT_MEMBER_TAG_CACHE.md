# 同流下一成员：class引用1与第二个Housing payload，到3427

在1defa69已核验首个Housing payload到3399后，同一完整136块候选的结构解释继续到3427。完整输入/解码SHA保持不变。首成员计数恰为1，所以3399首先是下一成员头，不是首成员的第二对象tag。

实际成员1在`[3399,3401)`读版本1、`[3401,3405)`读四字节signed count，计数关系也恰为1。3405处word为0x8001，即compact class reference，掩去class标志后引用条件模型的class1。此路径解析出class descriptor后会创建新instance，而非复用既有object；所选cHousingGoal/schema0 reader因此继续消费20字节。

| 范围 | 字节 | 本轮结果 |
|---|---:|---|
| `[3399,3405)` | 6 | 成员1版本1、count恰为1 |
| `[3405,3407)` | 2 | class reference1，word0x8001 |
| `[3407,3409)` | 2 | base节版本1 |
| `[3409,3413)` | 4 | base self+4字段，仅摘要 |
| `[3413,3415)` | 2 | housing节版本1 |
| `[3415,3427)` | 12 | housing self+8/+c/+10，各4字节，仅摘要 |

第二payload SHA256为 `850e83029b5521094a70a51d6096b8b91db8ec10b5f6c1364b07fd770c00645f`。解析止于3427，余4427568字节未解释。第二成员计数关系恰为1，所以这一项之后该成员正常完成；下一实际消费者应为成员2的版本/计数头，尚未读取。若计数更大，下一消费者仍是同成员的对象tag，工具的合成案例分别覆盖这两种分支。

本轮复用完整77ffc8/77fd90、77bbc0/77bb42、55a7a0/560040及55feb0→560ef0来源。当前条件metadata模型为cache0=null、class1=静态859198/schema0、object2=首个已核字段的Housing对象，next3。所选class-reference1经schema/expected检查后，factory正常非空返回时，原代码先登记object3，再调用虚槽+8读20字节，正常返回恢复S+c；本工具只建立metadata/字节资格模型，没有执行原factory或创建原实例。此前没有schema override、无cache预seed及同流别名、正常有效指针/资源/生命周期等前提继续保留。

**免读payload的是合格object reference，而不是所有reference。** 同流object reference2在index有效、目标非空/存活、其GetRuntimeClass满足expected cGoalBase等条件下复用已有对象，不调用该项schema reader，不增加cache id。合格class reference1则创建新instance、增加object cache id并消费20字节。源cache是共享无类型指针数组；现代metadata模型明确区分类/对象种类并拒绝错配，这项安全限制不是从源推导出的原始显式kind guard。

现代工具对未seed或没有合格metadata的引用停止。generic null reference虽然源协议支持，但560040随后直接使用object+4；本正计数成员场景将null列为不支持，不称原程序有额外null拒绝门。newclass声明只在原始word0xffff时选择；名字/schema/长度可有界核验，但没有合格lookup/schema reader时停在声明之后，不跳过payload。pending class3/object4仅是之后正常解析/factory完成时的条件安排，不把未完成声明登记成活跃对象。

限定新工具保留上限3522字节，是十成员布局下下一个tag/最长63字节声明的安全范围，不是本样本解析终点。整流身份、Adler、EOF等先核验再输出统计。字段值和原始名字字节不输出，不分析城市规则，也不把缓存编号当主池槽位。actual原registry选择、当前vptr、活对象/cache状态及callback/异常完整效果均未证明。

Release、UBSan各47/0自造验收：下一成员与同成员下一对象分支，空成员推进及十成员结束，compact/extended object/class引用，object引用不消费伪payload尾部，class引用必须有20字节/两个节版本，现代kind错配、未知引用、null场景、负计数、字段/标签/声明截断、名字63/64、标准zlib、Adler与SHA拒绝。优化Python在建目录前拒绝。新添后续计数分类后保留首次收据、复验最终源码和同一实际边界，未重复旧闭合批次冒充新增验收。

本轮没有新的原始静态查询或后台分析器；源引用均为先前完整Listing及明确的native版本/表槽观察。f14193d独审19/0、各36/0及P3短前缀48/0已确认，本新增仍待追加独审。证据见 [清单](city19-next-member-tag-cache-evidence.json)。继续应从3427成员2头开始，在本条件cache模型class1/object2/object3/next4基础上核验下一tag；先匹配类型/reader，再解释字段，保持原registry选择未证。
