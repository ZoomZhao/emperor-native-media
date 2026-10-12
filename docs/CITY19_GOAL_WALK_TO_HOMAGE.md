# 有界Goal组件：回归3399/3427，继续到3472的Homage声明

新自写组件把成员头、compact/extended tag、条件class/object缓存和已匹配Housing的20字节字段读序集中到不可变字节view。旧工具与五份生产reader/codec保持原样。结果只存偏移和metadata，不返回原字节指针或对象字段值；异常不会向调用者发布部分修改的模型。component没有原registry查询、factory、虚方法调用或城市规则实现。

本轮真实继续到3472。相同165223字节候选仍完整136块/4430995解码字节，完整输入/解码SHA一致。两项真实checkpoint分别停3399和3427，首/第二20字节摘要及条件cache next3/4与已发布1defa69/65091b5的收据精确一致；没有改变旧闭包语义。之后继续的具体顺序如下。

| 范围 | 来源与实测结构 |
|---|---|
| `[3427,3433)` | 成员2版本1，signed count为2 |
| `[3433,3435)` | class引用1，对应新Housing instance |
| `[3435,3455)` | Housing payload20，两节版本1、四个DWORD字段，仅摘要 |
| `[3455,3461)` | 该成员第二项word0xffff、新class schema0、nameLen11 |
| `[3461,3472)` | 名字摘要匹配静态字面量cHomageGoal，payload未读 |

已完成三个已匹配Housing项，前两个成员正常完成，当前成员2的ordinal1停在新类声明之后。cache条件模型为slot0null、class1Housing/schema0、object2/3/4均关联class1，next5。Homage class5与instance6只是假设其lookup/schema/factory正常完成后的安排，尚未登记为活跃metadata条目。本轮未把声明名字匹配当作原runtime registry选择或其serialize reader成立。

pending名字SHA为 `1d97a2fcfe52b702e5517b850d067ca0d09378bf1c57416ee887da4012b3da4b`，精确匹配已保存官方静态字符串859658的cHomageGoal。解释终点3472，余4427523字节未解释；16384只是私有保留预算，不是解释终点，也不是原archive容量上限。新类出现后不按sizeof或猜测长度推进，先查有限descriptor/base/schema/factory/constructor/vptr/Serialize来源。

源码顺序继续严格区分class与object：class声明通过lookup/schema检查后先登记descriptor，再检查expected；创建instance时先登记object，再调用其payload reader。object reference复用合格、存活、expected-compatible缓存对象，不消费新payload或新增id；class reference创建新instance并读已匹配payload。source cache数组无显式kind标签；现代metadata的kind错配拒绝为安全限定。原registry当前选择、vptr/对象/cache活状态、clear callback/allocator/异常与同流别名仍未证明。

现代限制为前缀16384字节、128个处理项、256个metadata cache条目；source原计数没有这些上限。成员/基类/Housing节版本仅接受已选择的1；原方法读分支不按取回节版本选择字段，不能写成原程序版本拒绝门。负计数、空/非ASCII或embeddedNUL名字也是现代范围。输入仍由自写OwnedFramedReader+系统zlib核整流，失败不输出成功统计。原字节、GUID、字段值及编译二进制不上库，无原/转换/衍生game执行。

Release、UBSan各40/0自造验收，覆盖两个Golden checkpoint、重复新类与instance/cache注册顺序、10成员循环、空成员、同成员/跨成员引用、object引用零payload、class引用20字节、unknown类声明停止、cache kind/索引/null/version/截断/名称/Adler/SHA及item/cache预算拒绝。真实checkpoint另有独立旧收据比较，不把合成通过当作原程序通过。

已审阅并合入独审提供的独立member-progress测试脚本；只把review frozen默认目录改成三个显式必填参数，并加自身优化启动拒绝。该脚本原76属性、8优化及24源收据检查由独审已通过，本轮未重复；仅检查新增启动门和语法。新Goal组件仍待追加独审。证据及准确执行边界见 [清单](city19-goal-walk-evidence.json)。
