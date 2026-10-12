# Population 12 字节闭包与十个成员实际终点 3736

同一真实存档候选的十个成员已全部完成结构读取：此前已完成5个，本轮完成成员5与四个空成员，共新增5个完成成员。Population payload到3690，其后的已知 Yearly class10引用到3712，最后四个空成员头到3736。共11项已核结构记录、5种已匹配类型；当前没有未读取的 goal 类型声明。3736之后仍有4427259解码字节未解释。

Population 默认关闭，显式 enable_population=true 开启。新类依据独立 source 映射：名字85970c→descriptor8590f0（schema0/base8590d8/factory5591c0/size字段12）；factory请求12并调用559220，构造先调用已核base559120，再写vptr7b813c、调用5601a0。reset只写self+4/+8，前三槽5592f0/5592d0/560280；getter确切entry为MOV descriptor/RET。

完整 Serialize560280先调用base55feb0，然后用独立record12a5a40读取节版本，readmode以4字节读取self+8（5602c6）。559fe0将own record初始化1。没有内部计数/容器，也没有按取回版本选择字段的分支。wire12来自2+4+2+4请求；类schema0、base节1、Population节1分开。现代版本1限定不能解释为原程序另有拒绝门限。

| 实际范围 | source 与限定 |
|---|---|
| `[3678,3680)` | base55feb0节版本1 |
| `[3680,3684)` | base55fef6，self+4四字节 |
| `[3684,3686)` | Population record12a5a40，节版本1 |
| `[3686,3690)` | 5602c6，self+8四字节 |
| `[3690,3692)` | 已知Yearly class10引用，新条件object16 |
| `[3692,3712)` | Yearly已核20字节布局 |
| `[3712,3718)` | 成员6，版本1/count0 |
| `[3718,3724)` | 成员7，版本1/count0 |
| `[3724,3730)` | 成员8，版本1/count0 |
| `[3730,3736)` | 成员9，版本1/count0 |

Population12摘要 `26989234d58cdbb1eefd5774033642437115ac5d1cbbab1f3bc74529e591e42c`，后续Yearly20摘要 `8390833fb77e718b42bac826733b01286fdf4575471a29447d5d2a78ed023c69`。末项不是Housing；两者虽都是20字节，实际class10的独立类型映射决定其读序。条件cache为class1Housing/class5Homage/class8Trading/class10Yearly/class14Population，11个object元数据，next17，没有pending class。

Trading与Population共用一DWORD字节helper，仍保留不同descriptor、own record、callsite与错误信息。默认Housing-only、Homage、Trading、Yearly四个旧模式，在最终header下的真实完整统计分别保持3472/3542/3581/3678，旧CLI源未改，旧整套测试未重跑。外框仍136块、4430995解码字节，完整输入/解码SHA一致。

新增自造验收Release、UBSan各35/0：Population字段、各截断、错误版本/schema、compact/extended class与object引用、重复声明、与Trading/Housing的不同类缓存、非空/kind/预算/SHA/Adler门限、跨块，以及十成员完成后留下下一owner毒尾。python -O在建立输出前拒绝。最初34/0结果保留；实际发现十成员终点后，增加毒尾用例，最终35/0。源码会话exit0，319.061秒，13 operations、5个完整procedure参数、2个native地址、36 selected bytes；输入与18对官方模块摘要一致，临时为空、errors为空。Yearly上一增量独审已通过：新套件各33/0、独立专项各8/0、源码与收据14/0。已纳入Yearly专项测试，在当前Population header下再次验收各8/0，优化拒绝门通过；这不代替新增Population及helper重构的独审。

十成员来自已核55a7a0的readmode/version4循环：55a8bf调用当前成员槽0，步长0x18，次数10；循环结束后才在55a8ce设置record4，并于55a8d8返回。外层55a430对所选version3调用该reader，随后设置record3并返回。因此3736是此owner完成后调用方下一消费者的起点。此前Population未知载荷阻断整个loop返回；本轮消除该结构缺口。

后续缺口位于3736：调用方的下一receiver、实际静态类型关联、节版本与字段读序尚未在本轮绑定，未读取或猜跳这些字段。十成员完成不等于完整城市载入；原进程实例/registry/current vptr/cache仍没有运行时证据。下一步应从调用方已有来源定位下一消费者，再核验其reader布局，保持当前摘要及成员统计。

工具只输出结构范围、计数与摘要，无字段值或资源/目标规则解释。证据见 [清单](city19-population-walk-evidence.json)。
