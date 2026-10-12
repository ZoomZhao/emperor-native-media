# cReligion 与 cCommerce 连续完成：主池起点 56118

真实候选从55822新增296字节解释：cReligion完整294字节到56116，cCommerce独立版本节2字节到56118。十成员/11项goal、完整Flood/Festival统计、cache条件模型next17、完整136块/4430995解码字节和输入/解码SHA逐项一致。四个主池前置消费者现在全部闭合；尚未读取56118处的池节、计数或对象，剩4374877字节未解释。

## 四个子对象的来源与层级

完整父reader5a7b90先读独立record1312e48，再对parent+38/+3c/+40/+44四个pointer依序调用+8。5a7940的正常构造路径通过同一5a7010创建它们；5a7010写根7bc028，getter5a6f30返回descriptor85db28/name85dbc0=cWorshipType/schema0，+8=5a71b0。

子对象在+self28以765629创建三个stride18嵌入成员，constructor5156a0写根7b5c18，+8=510010；完整515700只写成员+4/+8。子对象自己的data getter5a71a0返回self+4，实际size槽为4f8920，确切MOV EAX,c/RET；它们解释构造时12字节清零区域，**不证明该区域有wire消费**。一次探测5a7190位于5a7140内部、并非size槽，保存但不用于资格。

官方任务副本先用40cf40单函数seed恢复第四reader，再用构造/虚表明确指向的5a71b0、510010两个function seeds恢复子reader，创建分别1、2函数，0失败。seed是caller入口声明；wire布局来自恢复后的完整指令/调用序列及此前指针来源。原PE/现有数据库和官方模块未改，默认readOnly/deleteProject、所有权与限额保持。所有旧NoProcedure记录保留。

## 完整限定读序与实测区间

1312e48由5a7870初始化1；1312f68由5a6e90初始化3；10bfdd8由50fef0初始化2。父source1读完四child后，在5a7c08读self48的raw20；完整所选+c/+10 getters分别返回self48、常数20。child source3先读版本，再调用三个嵌入成员，绕过版本<=2的旧数值调整分支，没有额外12字节raw读取。嵌入source2以4c95e0→42dc20读取五个DWORD，helper返回S而非字段值；原source1使用raw20、其它版本分支仍存在，现代仅支持此处选定1/3/2。

| 实际区间 | 限定 |
|---|---|
| `[55822,55824)` | cReligion独立节1 |
| `[55824,55892)` | child0节3 + 三个嵌入节2，每个2+5×4=22 |
| `[55892,55960)` | child1，同一68字节序列 |
| `[55960,56028)` | child2，同一68字节序列 |
| `[56028,56096)` | child3，同一68字节序列 |
| `[56096,56116)` | parent raw20，只作不可变范围/摘要 |
| `[56116,56118)` | cCommerce独立节1，无字段载荷 |

Religion294来自2+4×(2+3×22)+20，摘要 `ba443dc6a4bc5811cd4a3992541d98ffb29713cacdf0b903cdf8b30d194a6f0d`；raw20摘要 `55337594aa89ebe65975ff515b2b9221f72f112d4139bbb960ae17a12ce099a1`。四个父child、12个嵌入成员分别完整核验，不按sizeof、邻近字段或某个getter跳过回调。raw20仍是原内存表示的opaque字节，不据其中可能的pointer位模式建立host对象/调用关系。

第四getter4081f0固定返回88f190；constructor40aaa0写根7a97e0，reset40d170只写+33c/+340/+344，保留偏移0。getter40aa50对应descriptor8158e0/name815b80=cCommerce/schema0；读取槽40cf40调用无字节40cf80，读record88f180后设置1、RET4，没有field请求，也未使用返回版本选择分支。40a9b0初始化1；现代只接受1，不能冒称原拒绝其它版本。

## 新实现、验收与既有回归

新增独立research header/tool在65536字节现代预算内读取这两段，旧Goal/postgoal header、所有旧CLI及五个reader/codec核心不改。只记录版本、区间、固定计数与摘要；不创建原对象、不执行原回调，不复制旧版数值调整或解释业务字段。完整请求、有效原存储/正常callee及无同流别名副作用继续是source条件；运行时registry/vptr/allocator/lifetime并未观察。

Release、UBSan各71/0自造验收覆盖四child/12嵌入成员、各层版本域、所有嵌套切点和raw20/第四Word截断、任意原始位模式保持opaque、跨frame scalar、未解释主池尾、SHA/Adler与优化拒绝。两个真实运行绑定最终header/工具/可执行SHA，分别到56116/56118；旧goal全部items/headers和两个postgoal完整结构统计逐项相同。旧整套未重跑。

cfea23e独审通过：各70/0、独立专项各12/0、审计22/0，seed声明与旧虚表/PE映射分开核验。已纳入其专项CPP、明确source-root/优化拒绝与失败索引保护的runner、README；在当前未改两个旧header下重验各12/0。该独审不覆盖新增Religion/Commerce布局。

两源会话正常exit0，552.285/1097.467秒，各23对官方模块SHA一致、输入不变、临时为空；第一会话保留2个子入口NoProcedure，第二seed后全部查询成功。保持8 procedure参数、48 native、80 operations、1024 selected bytes、1170分析截止/1200外层时限；未因状态询问重启或放宽时限，无审批拒绝或等待。

## 下一精确依赖

原caller53065f现在以同一S和ECX8c7630进入主池42d790，实际起点56118。既有静态source1/计数/对象协议与Building schema0/字段段5只作为下一步来源，不能替代本样本池版本、计数、实际类选择与payload资格。cache不能重置为0/1；next17仍是前缀条件模型，主池class/object编号不能当池slot，已序列化字段也不能当slot重写。

主池对象及随后city state/phase字段、读后处理与实际更新入口条件仍未到达。整体路线见 [路线](CITY19_ACTUAL_LOAD_ROUTE.md)，来源、完整实测统计与摘要见 [证据](city19-religion-following-evidence.json)。
