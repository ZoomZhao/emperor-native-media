# v19 主池：4000 条 Building v5 结构记录

同一真实候选的结构解释终点从56118推进到 **792136**，新增736018字节。池节版本1、signed32计数4000；一个`Building`新类声明（schema0）之后是3999个同类引用，4000条独立载荷均为字段段5、182字节。没有对象引用或未知类跳过。外框资格仍为165223输入字节、136块、4430995解码字节；余下3638859字节尚未解释。

这是限定范围的自写现代读取器对流结构的资格核验。原游戏、原codec、构造、补空分配、读后hook均未执行；不能据此宣称原生实例、完整城市载入或游戏通过。机器证据见 [增量证据](city19-mainpool-building-evidence.json)。

## 原来源与现代模型

本轮复用固定English PE SHA256 `8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753` 的官方REA/Ghidra完整证据（profile `e3402f2d7c487875edf3c6215fa95d67ee1fa133ba68c2390a9338b38656a7b2`），没有新增分析器或数据库修改。证据SHA、ID、procedure、来源性质列在JSON中。

53065f以同一ESI=S、receiver8c7630调用42d790。自身记录8c7648通过41fc10/41fbf0读取word，选定mode1/section1分支。42d8cc调用42dc20向local[esp2c]写入4字节计数；helper返回S，不能把EAX当计数。signed N>0进入对象循环。本地未见计数上限检查；42d91f的4000比较控制小计数的默认对象补空，77fcfd的3ffffffe是archive编号限制，都不是池计数上限。原本地未显式拒绝负数，现代子集单独拒绝。

池容器8c7634与archive cache S34不同。选定清理调用没有传入S，本地未见S34/S30重置；仍需正常callee、有效存储及无同S回调别名的前提。77fe3f只在缓存未建时初始化。现代元数据保留前序null0、五类/11记录与next17，新Building类编号17，条件对象编号18..4017，最终next4018。原当前cache、registry、vptr未实测。

42d0e0向77fd90传expected descriptor817890。新类和缓存类引用在选定原路径构造新实例并调用当前+8；缓存对象引用复用对象，不消费新payload或给新ID。原缓存是未分种类pointer array，现代kind防护属于自写政策。前序Goal类的已知基链不满足expected Building，现代模型拒绝把旧编号套为Building。缓存编号、池ordinal和字段receiver+b4不能等同。

实际新类名字精确匹配静态literal `Building`（8178a8），映射descriptor817890/schema0/factory42d050、所选ctor426c90/426e60、root7ab59c、+8 reader427430。这是流名字与选定静态描述的匹配，当前原registry唯一解析、活实例和vptr仍是条件。

reader自身记录8c7620的字段版本独立为5。保存的完整请求顺序有83项、共180字节，加2字节word得到182。所有offset、宽度和source call逐项核对，包括+70先于+6f的非单调顺序。sizeof200、构造清理和GUID生成均不用于推导流长度；字段、b4、GUID及指针位保持不透明，只输出范围和SHA。

| 解码区间 | 实际结构 |
|---|---|
| `[56118,56124)` | 池节word1及signed32计数4000 |
| `[56124,56138)` | Building新类声明，schema0，8字节名字 |
| `[56138,56320)` | 第0条182字节载荷，条件对象编号18 |
| `[56320,792136)` | 3999次2字节class17引用，各跟182字节载荷 |
| `[792136,4430995)` | 下一个消费者及余下内容，未解释3638859字节 |

连续长度为`6+14+182+3999*(2+182)=736018`。原字段方法未逐项检查780533返回数；现代要求完整有界输入，不能改写为原游戏逐字段严格检查。原循环还有基于O+4的+0xc0读后hook、append/分配及生命周期，现代组件均不调用，也不据不透明字段推导分支效果。

## 范围与验收

现代政策：前缀1048576字节、池计数8192、每批4096条、cache8192；pool section只接1、Building schema0/字段段5。ASCII名字、非null、cache种类、负计数、完整字节及预算防护是当前限定子集。批量上限在下一tag前checkpoint；未知新类只核声明、停在lookup/payload前。本样本4000条在既定范围内，没有放宽范围或跨未知载荷。

新增自造fixture的Release/UBSan各62项通过，覆盖182宽度、class/object及扩展tag、旧cache连续/不兼容、未知类停车、批量checkpoint、4000连续记录、版本/schema、截断、哈希与codec拒绝。初跑每模式1项失败是案例名含`object2`误触发“对象引用无载荷”断言；编号和范围原已正确。只改为两个明确对象引用案例，解析器未改，初失败回执保留。

9d343cb独审通过（各71项原suite、各10项独立专项、18项来源审计，无阻断）。本轮纳入其四文件独立测试，要求显式source root、拒绝优化Python并补child访问保护；未改Religion组件，Release/UBSan各10项通过。它们验收前序组件，本轮主池尚待独审。

24项增量连续性检查全通过：十成员各项/headers、Flood/Festival、Religion/Commerce及完整输入/解码身份与9d343cb一致；五个核心、所有旧CLI和Goal/postgoal/Religion三个头文件字节未改。旧完整suite未无关重跑。自造fixture不读原EXE、存档或游戏库；实际核验使用相同限定输入与系统zlib，不保存原始/解码载荷。

```sh
python3 file_io/tests/city19_mainpool_tests.py --source-root "$PWD" --output /tmp/city19-mainpool-checks-new
python3 file_io/tests/independent_city19_religion_component/run_properties.py --source-root "$PWD" --output /tmp/city19-religion-checks-new
```

在仓库根运行，输出目录必须不存在。新CLI沿用外框参数，末尾可选`MAX_POOL_ITEMS`，默认4096；全部解码资格不等于全部结构解释。

## 下一处来源

792136后原调用点53066a以同一S调用4e1e40(receiver10315d8)。本轮未解释该消费者的版本、宽度或载荷，下一步先闭合其独立来源。后续state/phase/gate/quota、生命周期及读后处理仍未证明，不以默认值、sizeof、槽号或未知长度替代。原运行/转换链、旧Swift、游戏库执行及设备/UI操作未恢复。
