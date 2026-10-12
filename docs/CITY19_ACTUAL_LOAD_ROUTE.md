# v19 真实候选的结构载入路线

最新结构解释终点为1646216。完整外框已经现代组件核验：165223输入字节、136块、4430995解码字节；这些数字表示外框与解码资格，不表示所有城市内容已解释。

| 解码区间 | 实际已验证内容 |
|---|---|
| `[0,2)` | 城市版本19 |
| `[2,45)` | 首个plain owner节6与41字节字段 |
| `[45,95)` | owner1158e18节2与48字节字段 |
| `[95,3355)` | 外层helper节3、直接owner节4、3256字节固定字段 |
| `[3355,3736)` | 十个成员全部完成，11项结构记录；Housing/Homage/Trading/Yearly/Population五种源映射 |
| `[3736,55739)` | cFlood/source1，52003字节，完整固定请求 |
| `[55739,55822)` | cFestival/source4及嵌入source1，83字节 |
| `[55822,56116)` | cReligion/source1，四个source3子对象及12个source2嵌入成员，294字节 |
| `[56116,56118)` | cCommerce/source1，2字节版本节、无字段载荷 |
| `[56118,792136)` | 主池source1/count4000；Building schema0、字段段5，4000条182字节载荷，新增736018字节 |
| `[792136,1592210)` | Figure list1/count2000全完成：1979条Figure7/398B，21条cPheasant own2→Figure7/400B，两类schema0 |
| `[1592210,1646216)` | cGoodsControlList source1/count200；200个内联cGoodsControl source1，各270B |
| `[1646216,4430995)` | 尚未解释；2784779字节 |

实际主池声明一个Building类、3999次缓存class引用，没有object引用；现代条件cache沿同一S从next17连续到4018。83个字段请求的180字节加自身word2构成182，未用sizeof200推宽度。源码4000是默认补空阈值，未见池计数拒绝上限；现代计数8192/批4096/cache8192/前缀1048576是独立资源政策。

当前首个未知实际消费者是1646216的cEmpire（getter480740、receiverc058d0、选定root7acd58/+8 reader480800、caller530686）。实际word/count/payload尚未资格化。Building4000、Figure列表2000及GoodsControl200均为本样本结构记录数，活动状态/数量和记录业务值未解释。shared archive条件cache保持next6020，不是原运行cache证明。

后续依赖按顺序是：

1. 主池4000条结构载荷已全部完成；4e1e40/list1的2000项及两种具体布局已闭合，cGoodsControlList200项亦完整闭合，接下来核cEmpire与内联/后续层级的准确请求。未知布局或来源缺口停止，不跳未知长度。
2. 原当前registry、vptr、活cache、factory、分配、alias、生命周期及读后hook仍未实测。条件静态链与现代编号不能当原实例证明；字段+b4、cache编号与池槽号不能等同。
3. 完成后续state消费者，取得实际phase/周期字段。已知source中c82ef8、c82eec、c82ef0、c82ef4来自流，不能以构造或另一reset默认值替代本次字节。
4. 连接完整载入结果、生命周期及读后处理，再核实际更新入口gate/quota/clock与current state。既有条件静态调度合同只给来源关系；结构尚未到达这些消费者，不能据主池完成宣称一次城市更新通过。

新增Pheasant Release/UBSan各23属性组及CLI共6项通过，23项连续性检查通过；此前Figure456及主池独立20项每模式通过的证据保留。旧前缀、五个核心、旧CLI及三个前序头文件保持一致；d596187主池独审通过，本GoodsControl增量尚待独审。见 [GoodsControl报告](CITY19_GOODSCONTROL_COMPLETE.md)、[完整列表报告](CITY19_PHEASANT_FIGURE_LIST_COMPLETE.md)、[Figure报告](CITY19_FIRST_FIGURE7_FRONTIER.md)、[主池报告](CITY19_MAINPOOL_BUILDING_BATCH.md) 和 [Religion/Commerce报告](CITY19_RELIGION_COMMERCE_TO_MAINPOOL.md)。

本段来源会话均已关闭并完成清理；后续来源不改变此验收边界。原游戏/codec/构造补空/hook未执行；剩余2784779字节及当前原运行状态保持未证明。
