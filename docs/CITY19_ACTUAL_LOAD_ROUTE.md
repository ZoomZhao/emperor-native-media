# v19 真实候选的结构载入路线

最新结构解释终点为1653822。完整外框已经现代组件核验：165223输入字节、136块、4430995解码字节。剩余2777173字节未解释；完整城市载入、字段业务值及原游戏运行未验证。

| 解码区间 | 实际已验证内容 |
|---|---|
| `[0,2)` | 城市版本19 |
| `[2,45)` | 首个plain owner节6与41字节字段 |
| `[45,95)` | owner1158e18节2与48字节字段 |
| `[95,3355)` | 外层helper节3、直接owner节4、3256字节固定字段 |
| `[3355,3736)` | 十个成员全部完成，11项结构记录；Housing/Homage/Trading/Yearly/Population五种源映射 |
| `[3736,55739)` | cFlood/source1，52003字节 |
| `[55739,55822)` | cFestival/source4及嵌入source1，83字节 |
| `[55822,56116)` | cReligion/source1，四个source3子对象及12个source2嵌入成员，294字节 |
| `[56116,56118)` | cCommerce/source1，2字节版本节、无字段载荷 |
| `[56118,792136)` | 主池source1/count4000；Building schema0、字段段5，4000条182字节载荷 |
| `[792136,1592210)` | Figure list1/count2000：1979条Figure7/398B，21条cPheasant own2→Figure7/400B |
| `[1592210,1646216)` | cGoodsControlList source1/count200；200条内联source1，各270B |
| `[1646216,1653822)` | cEmpire source3/count190；190条EmpireObject own2，各40B，仅内联前缀完成 |
| `[1653822,4430995)` | 首个43e370 grid及后续尚未解释 |

主池声明一个Building类、3999次缓存class引用，无object引用；同一S的现代条件cache从next17至4018。随后Figure/Pheasant两类声明、1998次class引用、无object引用，至next6020。GoodsControl与Empire内联读取不增加archive标签或编号。以上计数都是样本结构记录，不能当活跃建筑、人物数量或唯一原实例。

各流宽度来自准确请求：Building word2+180=182，Figure word2+395+presence1=398，Pheasant自身word2后直接调用Figure=400，GoodsControl word2+268=270，EmpireObject自身word2+21请求38=40。descriptor schema、对象自身版本、内存sizeof、目的步长、cache编号、池槽号和+b4字段分别保留；不互相代替。

当前首个未知实际消费者是1653822的43e370，首receiverc07b80；父source3静态接收者循环为22次，随后还有父self+4/raw4。grid版本与载荷尚未消费，3c00目的步长不作为流长度。未知布局或来源缺口停止，不跨过未知长度。详见 [Empire内联前缀](CITY19_EMPIRE_INLINE_PREFIX.md) 及 [机器证据](city19-empire-inline-prefix-evidence.json)。

父1/2/3与子explicit2是当前现代选定范围；子legacy1虽有raw40来源，本实现不资格化。负数、其它版本、kind及预算拒绝是现代独立政策，不表示原游戏已有对应校验。valid原存储、完整读、正常返回、reset的DF0及分配/销毁/读后处理没有同一S或跨元素别名改写等条件保留。当前registry、vptr、活cache、factory、生命周期与postload未实测。

本增量最终头文件Release/UBSan各18/0，早期CLI共6项及2个预算拒绝；最终Release重构建smoke和实际190条通过。20项前序身份、统计、源码与布局连续性通过。115e138独审已通过，并原样采用五个测试文件；其属性每模式18/0、CLI每模式7/0、来源22/0。该独审不覆盖当前Empire增量，当前增量仍待独审。

后续先逐段闭合grid与父尾，再抵达state/phase字段及更新gate/quota/clock消费者。条件静态合同只给来源关系；原完整载入、一次城市更新及游戏通过仍未证明。当前来源会话均已关闭并清理，无后台分析器或审批等待。原游戏/codec/构造/销毁/hook未执行。前序报告保留历史检查点；[GoodsControl报告](CITY19_GOODSCONTROL_COMPLETE.md) 中36次的计算错误已更正为22次。
