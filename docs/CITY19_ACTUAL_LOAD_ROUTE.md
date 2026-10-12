# v19 真实候选的结构载入路线

最新结构解释终点为1991913。完整外框已由现代组件核验：165223输入字节、136块、4430995解码字节；剩余2439082字节未解释。完整城市载入、业务值及原游戏运行未验证。

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
| `[1991878,1991913)` | 511ea0首项word4、32B字段、presence1；可能扩展未资格化 |
| `[1991913,4430995)` | 可能virtual+8载荷及后续未解释 |

主池一个Building类声明、3999次class引用，无object引用，条件cache从next17至4018；Figure/Pheasant两类声明、1998次class引用，无object引用，至next6020。GoodsControl、Empire内联与两层grid不增加标签或编号。所有计数都是样本结构段，不能当活动建筑、人物数量或原唯一实例。

各宽度来自请求：Building word2+180=182；Figure word2+395+presence1=398；Pheasant自身word2后直接调用Figure=400；GoodsControl word2+268=270；EmpireObject word2+38=40。本次grid为word2+1192+22*(child word2+627)+334=15366，22个grid加父raw4为338056B。descriptor schema、自身版本、目的sizeof/步长、cache编号、池槽号和+b4字段分别保留。

480945外层receiverc07b80、步长3c00、exclusive limitc5a380给出22次；43e935内层字面16h给出22次。原3c00=15360与274=628都是目的步长，均不等于本次流长度。前一36次计算错误已更正为22，未影响前序读取。

当前连续终点1991913。480f60→481f10的完整链为条件状态写入，没有archive读取；原postload未执行。后续同一S调用顺序为510e60→564e30→593140。实际首项非零presence停在5122c0返回及可能virtual+8读取之前，不能把35B当完整记录。后续12项列表及27个向量段未实际完成；未知payload不按目的步长跳过。详见 [Empire之后的消费者](CITY19_AFTER_EMPIRE_CONSUMERS.md) 和 [新增机器证据](city19-following-consumers-evidence.json)。

父1/2/3、内联子explicit2、grid15与grid子5是当前现代选定范围。其它版本在准确word边界停止，不猜未知载荷。负数、kind与预算拒绝是现代政策，不表示原有对应校验。原存储有效、选定readmode/版本、完整读、正常返回、reset的DF0及分配/销毁/postload没有同一S或跨元素别名改写等条件保留；当前registry、vptr、活cache、factory及生命周期未实测。

前序grid独审3c17a26已通过，四份测试原样纳入，重放独立专项每模式15/0、来源审计18/0。本次后续模型属性每模式18/0、最终CLI每模式9/0，实际35B前缀通过，前序20项与cache6020保持。旧十四个研究头、五个核心、十八个旧CLI及用户已有变更保持。当前后续消费者增量仍待独审，不以发布核验代替独审。

后续核1991913的可能虚调用消费者，再抵达向量、state/phase及更新gate/quota/clock。未知状态/析构/别名效果与未知流长度分列于新增报告。条件静态来源不提供原运行实例，完整载入和一次城市更新仍未证明。两次来源会话已正常关闭并清理；5122c0新来源阶段另行继续，不当作此35B增量的已验证扩展。原游戏、codec、构造、销毁及hook未执行。
