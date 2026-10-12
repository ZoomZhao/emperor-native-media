# v19 真实候选的结构载入路线

最新结构解释终点为1991878。完整外框已由现代组件核验：165223输入字节、136块、4430995解码字节；剩余2439117字节未解释。完整城市载入、业务值及原游戏运行未验证。

| 解码区间 | 实际已验证结构 |
|---|---|
| `[0,2)` | 城市版本19 |
| `[2,45)` | plain owner节6与41字节字段 |
| `[45,95)` | owner1158e18节2与48字节字段 |
| `[95,3355)` | helper节3、直接owner节4、3256字节固定字段 |
| `[3355,3736)` | 十个Goal成员，11项记录；五种源映射 |
| `[3736,55739)` | cFlood/source1，52003B |
| `[55739,55822)` | cFestival/source4及嵌入source1，83B |
| `[55822,56116)` | cReligion/source1，四个source3子对象及12个source2成员，294B |
| `[56116,56118)` | cCommerce/source1，版本word2、无字段载荷 |
| `[56118,792136)` | mainpool1/count4000；Building schema0、字段段5，各182B |
| `[792136,1592210)` | Figure list1/count2000；1979条Figure7/398B，21条Pheasant own2→Figure7/400B |
| `[1592210,1646216)` | GoodsControl list1/count200；内联source1，各270B |
| `[1646216,1653822)` | cEmpire3/count190；EmpireObject own2，各40B |
| `[1653822,1991874)` | 22个grid15，每个22个child5；grid各15366B、child各629B |
| `[1991874,1991878)` | 父cEmpire/source3 self+4/raw4 |
| `[1991878,4430995)` | 后续尚未解释 |

主池一个Building类声明、3999次class引用，无object引用，条件cache从next17至4018；Figure/Pheasant两类声明、1998次class引用，无object引用，至next6020。GoodsControl、Empire内联与两层grid不增加标签或编号。所有计数都是样本结构段，不能当活动建筑、人物数量或原唯一实例。

各宽度来自请求：Building word2+180=182；Figure word2+395+presence1=398；Pheasant自身word2后直接调用Figure=400；GoodsControl word2+268=270；EmpireObject word2+38=40。本次grid为word2+1192+22*(child word2+627)+334=15366，22个grid加父raw4为338056B。descriptor schema、自身版本、目的sizeof/步长、cache编号、池槽号和+b4字段分别保留。

480945外层receiverc07b80、步长3c00、exclusive limitc5a380给出22次；43e935内层字面16h给出22次。原3c00=15360与274=628都是目的步长，均不等于本次流长度。前一36次计算错误已更正为22，未影响前序读取。

当前连续终点1991878；父选定分支的直接字节请求已完成。原480a5b→480f60 postload未执行，调用者后续消费者尚未资格化，未知处停止。详见 [grid与父尾报告](CITY19_EMPIRE_GRIDS15_COMPLETE.md)、[机器证据](city19-empire-grids15-evidence.json) 及 [此前内联前缀](CITY19_EMPIRE_INLINE_PREFIX.md)。

父1/2/3、内联子explicit2、grid15与grid子5是当前现代选定范围。其它版本在准确word边界停止，不猜未知载荷。负数、kind与预算拒绝是现代政策，不表示原有对应校验。原存储有效、选定readmode/版本、完整读、正常返回、reset的DF0及分配/销毁/postload没有同一S或跨元素别名改写等条件保留；当前registry、vptr、活cache、factory及生命周期未实测。

当前grid属性Release/UBSan各17/0、CLI各9/0；实际先一个grid再22个grid与父尾全通过。前序19项完整身份/统计在两次探测中保持；旧十二个研究头、五个核心、十七个旧CLI与用户已有变更保持。115e138和c9daab7独审均已通过；本次原样采用c9五个测试文件，重放连续链每模式14/0、最终CLI每模式6/0，来源20/0的独审包含22次更正。当前grid增量仍待独审，不以发布核验代替独审。

后续核1991878之后的载入消费者，再抵达state/phase及更新gate/quota/clock。条件静态来源不提供原运行实例，完整载入和一次城市更新仍未证明。本次复用保存来源，无新REA会话、后台分析器或审批等待；原游戏、codec、构造、销毁及hook未执行。
