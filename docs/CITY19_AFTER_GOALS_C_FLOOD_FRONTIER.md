# 历史cFlood缺口：已由官方函数种子恢复

此前d4f71e6记录的是默认分析副本中的NoProcedure/undecodable。当前已通过安装版REA官方seed功能恢复同一入口，cFlood与cFestival连续解析到55822，未修改原PE或原工具。详见 [恢复报告](CITY19_FLOOD_FESTIVAL_SEEDED_READERS.md)。下面保留旧诊断快照，不能作为当前阻塞状态。

# 3736 后首个消费者 cFlood：对象来源已定位，读取布局未闭合

本轮没有新增实际字节解释。此前十成员终点仍为3736，已核11项结构记录、五种goal类型；未读取3736后的版本、长度、字段或对象tag。新增结果是把首个未知消费者从“调用方下一reader”定位为固定对象10af4e0的cFlood、Serialize槽4edda0，并核对它位于主池之前的同流顺序。

## 同一 archive 的连续顺序

核验此前完整官方52fda0来源的SHA与所选v19子段。ESI保持同一archive S，各站点PUSH ESI，再以getter返回的对象为ECX调用+8：

| 顺序 | caller站点 | 来源与当前状态 |
|---|---|---|
| 已完成十成员 | 53061d→55a430 | 到3736，record3/4与十成员结构已有资格 |
| 首个未知消费者 | 530625 getter425260；53062f +8 | getter固定返回10af4e0；cFlood/4edda0布局未闭合 |
| 第二消费者 | 530632 getter48df30；53063c +8 | getter固定返回c701d8；其实际读序与起点未知 |
| 第三消费者 | 53063f getter426a60；530649 +8 | 调用顺序已核，receiver/reader/字节位置未在本轮绑定 |
| 第四消费者 | 53064c getter4081f0；530656 +8 | 调用顺序已核，receiver/reader/字节位置未在本轮绑定 |
| 主池 | 53065f→42d790 | ECX8c7630，仍PUSH同一ESI；本样本池节/计数/类型尚未到达 |

共享S也共享archive上下文。十成员之后的条件cache next17只适用于已核前缀；cFlood与后续消费者对cache/当前schema/游标的作用尚未知，不能为主池重置cache或假定17保持到那里。goal class/object编号与主池槽号继续分开。

## cFlood 的静态对象来源

选定CRT表8144f4的四字节指向4ed100。该地址没有procedure元数据，确切native入口是CALL4ed110；仅使用这个调用锚点，不声称4ed100整个函数已闭合。完整4ed110将ECX置10af4e0并尾跳4ed240。完整constructor4ed240先调用4e07c0，随后写vptr7b2bb8，再调用4ed290。

完整reset4ed290写self+4/+8/+c/+10/+14，并以REP STOSD初始化self+18之后的数组；在正常DF0、有效不别名存储前提下保留偏移0。表的前三槽为4ed210/559150/4edda0。4ed210确切native为MOV EAX,84fab0/RET；descriptor84fab0的名字指针84fb1c对应静态cFlood，schema0、size字段cb28、base指针7cd140。这是源条件类型关联，不是原进程当前对象/vptr的观察。

size字段52008与reset数组大小都是内存结构信息，**不能作为wire长度**。本轮未按它们消费存档。class schema0也不能代替未知的独立节版本。

## 具体未闭合动作

对槽4edda0的procedure_assembly返回NoProcedure；inspect_native_instruction返回undecodable、没有bytes/length/指令；resolve_containing_procedure返回not_in_procedure。故尚无可证明的Serialize入口/读分支，无法确定版本、固定请求、动态计数或正常返回终点。没有实现假设布局的解析代码，没有跳过它去读取第二消费者。

官方会话正常关闭exit0，685.265秒，21 operations、19 Evidence、2 NoProcedure记录、4个native地址（含该undecodable观察）；输入身份及18对官方模块摘要一致、临时为空。两份NoProcedure记录及失败观察保留；不能把会话exit0写成reader通过，也没有新的审批拒绝。

本轮只变更文档。五个生产reader/codec、goal组件与五份既有CLI源码保持摘要；保存的旧四模式3472/3542/3581/3678及十成员3736完整统计与上批收据绑定。没有重跑旧套件或重读真实payload，既有Population各35/0、Yearly专项各8/0只作为已保存基线，未计作本轮新增测试。

下一步需要4edda0的有效官方静态函数/指令来源，才能闭合整个cFlood读段并一次消费其受限布局。普通文件权限并不会提供这条缺失来源。本轮未知布局停止符合当前范围；其它消费者和主池不能越过它。

整体加载区间、首个未知消费者与城市更新剩余依赖见 [路线](CITY19_ACTUAL_LOAD_ROUTE.md)，来源及基线摘要见 [证据](city19-postgoals-frontier-evidence.json)。
