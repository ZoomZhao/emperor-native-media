# cEmpire 的22个grid及父尾：连续前缀至1991878

在c9daab7的1653822终点后，现代有界读取器先验证首个grid，再沿相同的已核对布局完成22个grid和父source3尾raw4。实际全部grid为版本15，各有22个自身版本5子段，共484个子段；连续终点1991878，新增338056字节，剩余2439117字节未解释。前序19项完整身份与统计在两次探测中逐项保持，条件archive cache仍为next6020。

| 解码区间 | 实际结构 |
|---|---|
| `[1653822,1669188)` | 首grid own15，22个child own5，15366B |
| `[1669188,1976508)` | ordinals1..20，均15366B |
| `[1976508,1991874)` | ordinal21，15366B |
| `[1991874,1991878)` | 父source3 self+4/raw4，不解释字段值 |
| `[1991878,4430995)` | 后续未解释 |

## 长度来自实际请求

480800/source3的480945循环以receiverc07b80开始，每次目的地址加3c00，比较c5a380并JL。`(c5a380-c07b80)/3c00=22`；这是调用次数来源。前一报告36已更正，3c00仅作目的步长。

43e370先用a4f560读取自身word，按signed版本减1、CMPe/JA及43fdb0/43fd90两级表选择分支。版本15指向43e78e；本现代实现只资格化这一明确路径。原其它版本有独立分支或默认路径，现代在未知版本word后停止不读载荷，不冒称原错误拒绝。

版本15先有30次raw请求，共1192B；43e935的字面计数16h控制22次43e941→43da30，目的步长274；再有28次raw请求，共334B。共享尾43fd52的版本15条件跳过旧版本字段回填，仅设置当前版本并返回，不另消费字段。

43da30从父参数取得15，因大于2而经a4f570读取子自身word。子word5选择43ddf9路径：首raw1的参数在分支前43dbfc/43dbfe已入栈；self+78/raw4从43dea7..43deaa跳入43df5b/43df5d；尾self+264及+268各raw4由43dfd5..43dff0消费。24次请求共627B，加自身word2为629B。没有用274=628目的步长推流长度。

因此每grid为`word2 +1192 +22*(word2+627) +334 =15366B`，22个grid加父raw4为338056B。grid目的3c00=15360也不等于流长度。两层版本word、调用次数、请求表、目的布局分别保留；这些内联段不增加class/object标签或cache编号。

## 验收与停止边界

新增布局表、独立研究组件与CLI；旧十二个研究头、五个核心、十七个旧CLI及用户九项已有变更的SHA保持。增量属性Release/UBSan各17/0，包括15366个proper grid截断、父尾四个截断、两层未知版本停止、source1/2/3、预算及checkpoint；新CLI两模式各9/0，涵盖跨frame的grid/child word、压缩/raw混合块、未知版本与截断拒绝。最终实际结果绑定同一Release构建和完整输入/解码SHA。没有无关重跑旧整套测试。

仅复用保存的完整官方来源及表证据，没有新REA会话、安装或原执行。c9daab7独审已通过：来源20/0，独立连续链每模式14/0、最终CLI每模式6/0，36→22更正及copy/assignment根保留条件均获核实。五份测试文件原样纳入，当前候选重放同样通过14/0及6/0每模式。当前grid增量仍待独审；机器证据见 [请求与验收记录](city19-empire-grids15-evidence.json)。

`parent_byte_requests_complete=true`只表示选定父分支的直接读请求闭合。原480a5b→480f60 postload未执行，当前registry、vptr、活cache、分配、销毁、别名及生命周期不因此获证；依旧要求原存储有效、读mode与版本选定、完整读取、正常返回、无同一S或跨元素别名改写，前序reset的DF0条件保留。字段业务值、城市逻辑与原运行状态未解释，完整载入或城市更新仍未通过。

下一步从1991878核对调用者后续消费者及其版本/准确请求，同时独审82项请求、共享尾和22×22循环。未知处停止；不得把postload、后续载入或更新状态当本次完成项。当前无后台分析器或审批等待。

```sh
python3 file_io/tests/run_empire_grids_properties.py --source-root "$PWD" --output /tmp/city19-grid-properties-new
python3 file_io/tests/run_empire_grids_cli_checks.py --source-root "$PWD" --output /tmp/city19-grid-cli-new
```

输出目录必须不存在。CLI沿用完整身份/外框参数，末尾可选`MAX_POOL_ITEMS MAX_FIGURE_ITEMS ENABLE_PHEASANT MAX_GOODS_ITEMS MAX_EMPIRE_ITEMS MAX_GRIDS`；最后参数1..22是现代批次预算。未知版本停止点以pending报告，记录数仅表示结构段。
