# 官方函数识别恢复 cFlood 与 cFestival，真实连续读到 55822

本轮从3736连续完成两段：cFlood 52003字节到55739；cFestival 83字节到55822。新增解释52086字节，剩4375173字节未解释。十个成员/11项goal记录和条件cache next17保持，完整136块、4430995解码字节以及输入/解码SHA一致。当前首个未知消费者是1312f08/cReligion的5a7b90；未读55822之后的字段。

## 默认分析缺口的实测诊断与恢复

原PE为固定4820992字节English x86-32目标。独立有界PE头解析确认ImageBase400000，4edda0为.text中可执行、文件支持的RVAedda0/文件偏移974240。7b2bb8虚表位于.rdata，原getter/CRT构造/虚表的+8指针与4edda0一致。官方read_bytes返回的64字节窗口，与直接PE映射窗口SHA完全一致。

已安装REA6.4.0本身支持REA_GHIDRA_SEED_FILE。官方ReaGhidraApplySeeds.java在默认分析前对明确入口调用disassemble/createFunction，并在握手报告结果；拒绝覆盖已有data/内部instruction等保护保持。采用单行 `0x4edda0\tfunction`、不加自造函数名，在本任务临时分析副本中创建1个函数、0失败。恢复入口为PUSH EBX，函数体241字节、非thunk，完整正常读分支和RET4闭合；起点未移动、原PE未修补。这支持“默认分析未建立该入口/指令”的诊断，不能把旧undecodable解释为x86字节非法或不可恢复。

同一官方流程另外分别恢复48ed30与嵌入成员4f8ca0。三个seed均为caller声明，不冒称Ghidra自主发现；不同seed/profile摘要分别保留。官方模块未修改，数据库只在任务独占临时根，readOnly/deleteProject与所有权清理保持，原数据库及PE不变。初始失败记录未删除。

## cFlood 完整限定段

完整4edda0先调用已核40cf80（RET4、无字节），用独立record10af4d0读取版本。4ed170初始化1；readmode signed版本1分支发出以下请求，最后设置record1并RET4。其它版本的现代拒绝是支持范围，不能冒称原函数拒绝。

| 解码范围 | 请求 | receiver偏移/source |
|---|---:|---|
| `[3736,3738)` | 2 | 独立节版本1 |
| `[3738,3742)` | 4 | +4 / 4ede36 |
| `[3742,3743)` | 1 | +8 / 4ede43 |
| `[3743,3747)` | 4 | +c / 4ede50 |
| `[3747,3751)` | 4 | +10 / 4ede5d |
| `[3751,3755)` | 4 | +14 / 4ede6a |
| `[3755,55739)` | 51984 | +18 / 4ede7a，字面请求cb10 |

段摘要 `9b130905c8b35c9471a2591a9781a0c95491087b14a2759030f39ffa2125e177`。52003来自2+4+1+4+4+4+51984请求，不来自内存sizeof52008或reset数组猜测。字段仅范围/摘要，不解释flood业务含义。

## cFestival 与嵌入成员

第二getter48df30固定返回c701d8；48deb0→48df40，constructor写根7adb88，+8为48ed30。完整reset48dff0/48dfd0与member+4的4f89b0在所选正常范围内保留两个根。runtime getter48a260指descriptor82bc58/name82c298=cFestival/schema0；schema不是独立节版本。

48ed30读取recordc702b8，48a300初始化4；随后以同一S调用member+self4的+8。成员4f8930写根7b3ee0，+8=4f8ca0；其独立record10bc148初始化1，source1分支调用4c95e0→完整42dc20四次，每次4字节，helper返回S而非字段值。Literal48f128四项表将外层version4指到48ee8e。

| 解码范围 | 已核序列 |
|---|---|
| `[55739,55741)` | cFestival外层节版本4 |
| `[55741,55743)` | 嵌入成员节版本1 |
| `[55743,55759)` | 成员四个DWORD，parent+8/+c/+10/+14 |
| `[55759,55791)` | 外层+28..+44，八个DWORD |
| `[55791,55822)` | 外层依序+18/1c/20/21/24/50/54/58/48/4c；4/4/1/1/4/1/4/4/4/4字节 |

段摘要 `0cc1c50d42f31a698d04cd19bf0f727120910e7e62fb3e08870ef0f8bda91c25`。83=2+2+16+63，与内存size92分开；外层1/2/3和嵌入其它版本未纳入此现代限定。完整原方法具有这些其它分支，未把限定域等同原版本拒绝门。

现代新header/工具只读取不可变字节与摘要。为固定cb10跨块请求建立独立65536字节私有保留上限，旧Goal视图仍16384，五个生产reader/codec及旧五CLI保持原样。本段无class/object tag消费，既有条件cache metadata保持；后续消费者的cache作用仍须单独核验。原instance/vptr/allocator/lifetime/别名条件和完整载入兼容性未由这些字段资格证明。

## 验证与下一段

初始Flood新增Release/UBSan各30/0保留；扩展嵌入成员与Festival后，最终各70/0，含完整字段摘要、各字段/数组截断、错版本、跨块scalar/byte、大未解释尾、SHA/Adler、十成员前置门及第三consumer毒尾。Python优化模式在建目录前拒绝。真实两个运行都绑定最终源/可执行摘要：Flood-only55739与连续55822的完整Flood段一致，goal全部items/headers、输入与解码身份逐项保持。旧整套未重跑。

acc03b7独审无阻断：Population各35/0、独立专项各9/0、来源/收据17/0。已纳入专项CPP、加明确源码目录/优化拒绝的runner和README，并在当前未改Goal header下重验各9/0；不把旧独审当新postgoal布局独审。

三个官方会话各exit0，分别598.397/576.856/914.515秒；每批保留8 procedure参数/48 native/80 operations/1024 selected bytes、1170秒分析截止/1200秒外层时限。每批23对官方模块SHA一致、输入身份不变、临时为空，均无审批拒绝。会话期间各保留一份后续地址NoProcedure记录，不把exit0当所有后续reader通过。

第三getter426a60返回1312f08，静态constructor5a7940写根7bc080，GetRuntimeClass关联85db40/name85dbb4=cReligion，+8=5a7b90已取得完整来源。它先读独立版本，再调用四个pointer子对象+8，之后才通过+c/+10取得self+48与长度20并读raw块。四个child布局尚未闭合，不能把后面的固定20字节当整个段长；55822之后未读。第四getter4081f0返回88f190，constructor40aaa0写根7a97e0、+8=40cf40，仍待官方入口识别核验，不判不可恢复。主池仍位于二者之后。

整体路线见 [路线](CITY19_ACTUAL_LOAD_ROUTE.md)；新profile、原始映射摘要、来源索引与实测收据见 [证据](city19-flood-festival-seeded-evidence.json)。
