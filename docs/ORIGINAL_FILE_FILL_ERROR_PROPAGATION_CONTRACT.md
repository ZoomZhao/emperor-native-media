# fill、短读与错误传播的原静态合同

固定原English x86源SHA `8a6d2df1…6753` / profile `e3402f2d…a7b2`。**已补齐447d30 fill及所选6c4930 handler的正常局部图；fill后续读取与transform状态存在未检查传播边界。** bulk archive可短返回，严格word读取不足则请求异常，不能合为一种完整读规则。新语义待独立复核；这些都是原x86 Listing/静态指令，不是arm64或原游戏运行。[来源索引](original-file-fill-error-propagation-evidence.json)区分完整native图、Listing与关键锚点。

## 447d30不再只是八条前缀

匹配构造根7ac7d4+60的候选447d30已取75个完整正常局部native节点（旧8+新67），两个RET在447dcc/447dff；六处直接CALL77c2c1和一处CALL5f8070。正常边仅在callee正常返回/原ABI有效时成立，当前F/root、输入、存储/计数/alias和异常仍未绑定。

Fbyte+1e=0或byte+1f非零时直接退，不生产新的available；不能把该flag直接命名为“本次已观测EOF”。其余路径先读4字节至预置零的local。只有该次返回量为4且tag等于F+14才进入transform分支；否则再比较tag与F+18，可能进入原样分支。字段14/18初始化常量不是所有当前对象值的证明。

| 局部路径 | 保存源的实际行为与边界 |
|---|---|
| tag匹配F+14 | 后续读4到F+11a8c、另4到local，再读F+11a8c字节到F+8d58；三次返回量未在fill核对。将F+11a88预置0x8d30（36144），调用四参数5f8070，然后忽略状态码，设置F+20=F+28、byte+1d=0。 |
| tag匹配F+18 | 读4到F+11a88，按该值请求payload到F+28；两次实际返回量都未核对。随后设置byte+1d=1、F+20=F+28，**没有将available直接缩至实际payload返回量**。 |
| 不匹配或首tag短读后未被接受 | 没有新的available直接store。结合447e00先清available、有效存储/非alias正常callee等条件，remaining>0时可返回已累计短量；不能据此证明发生真实EOF。 |

buffered等量/零可用量仍先fill后看remaining，原138节点合同不改；raw零请求的提前返回不能消除这个副作用。后续循环正相等也先fill，不把末段`remaining<=available` guard推广到所有派发。没有消费GameData或补造长度、字节内容、文件模式。

## 四参数transform的长度与错误状态

5f8070 whole Listing为52条/156字节/6块，核10个原native锚点；不称全52native。原cdecl ABI参数为destination、out-count指针、source、source-length，正常RET不清参数；447db9的ADD0x10由caller清四参数。声明常数栈偏移账目见本地ABI ledger，不执行或解释guest寄存器/内存。

5f89e0→5f88d0初始化有版本前缀/0x38尺寸等检查和-6/-2/-4正常错误出口，默认零alloc/free槽写5fa020/5fa040；这两个wrapper分别直调7675ee/7656df。当前heap/回调生命周期与内部5f9100等传递效果未闭合。5f8a00捕获398条Listing，只作为库内部来源库存，不冒充完整native、跳表case或递归callee证明。

wrapper只在step结果为1时，5f80fc显式写`*out-count=local stream+14`，随后返回cleanup 5f8880的结果。初始化非零错误直接返回，step为0的非成功分支返回-5，其它非1分支返回相应step状态；这些分支没有该直接out-count store。**fill忽略wrapper EAX，不能保证错误时预置的0x8d30已变成实际有效产量。** 这指出来源检查缺口，不断言某个输入实际失败或宣称有效解码；codec与alias/缓冲范围仍需独立资格来源。

## raw、bulk与严格读取是不同接口

77c2c1的22个原native正常节点已闭合：N=0提前返回0、不调用ReadFile；OS成功时返回实际out-count槽，未强制返回请求量。失败则GetLastError→77caa3(errorCode,0)。77caa3的9个原native正常节点已闭合：errorCode=0普通RET8，raw随后返回out-count槽；非零先经77cbcf映射cause，再进入77cb64(cause,errorCode,0)。实际OS输出/错误码没有运行验证。

查询780625得到的是包含该地址的**真实入口780533**，271字节/109条whole Listing、RET8，不能把780625叫独立函数。它按已有available复制、按底层current+34实际量累计，在零量或未填足整段时可停止并返回`requested−remaining`；S+8非零还有current+50 mapping分支，目标/root未绑定。N=0直接返回0；正整段路径可能在remaining已零时仍预读。DIV之前S+1c非零、有效cursor/存储/计数/ABI等都是未提供的运行前提。

严格reader **41fcb0实际读取两字节word**，不是查询标签中的single-byte。其22条Listing判断cursor+2是否越end；不足时调用78074e所缺量，随后读word、cursor+2、RET4。78074e最终available<need时调用78090e(3,0)，因此这里不同于bulk的短量正常返回。不能凭底层允许短返回就说所有archive读取静默接受，也不凭一个throw helper就认定全读档错误都同样捕获。

77cb64的0x14记录和78090e的0x10记录分别经76501e请求**RaiseException**。该transport有24条Listing/1native调用锚点，固定32字节模板确认exception code0xe06d7363、flags=1、三参数；参数为0x19930520、传入object/pointer-slot地址、传入descriptor地址。源码仍有正常RET8尾，但实际OS投递、catch、跨帧展开、恢复和异常对象生命周期未证明；不能把请求异常等同已验证throw/恢复。

## handler余路与owner34静态极限

所选root7c82fc+4=6c4930候选现在完整24原native正常节点（旧16+新8），两RET；直接调用GetMessageA后仅分返回值零/非零。零时写self+fc=0；非零时TranslateMessage→DispatchMessageA，均以API正常返回为局部后继前提，没有独立负值检查。

该局部图没有直接的owner34非零store；唯一字段store仍是self+fc=0。但DispatchMessageA的实际message/窗口procedure目标、root/对象寿命与writer alias未绑定，**不能静态消除所有owner34覆盖，更不能证明两guard同值或实际modal终止**。下一来源必须是同窗口注册/对象链的确切callback与alias证据，或者明确留下这个外部输入极限；不无界扫描邻近地址或借执行补值。

## 验证、审核与下一缺口

138请求/138Evidence/0NoProcedure，13procedure参数地址、117新native锚点、32选定data字节；官方exit0/775.37秒，固定目标与18组选定官方模块指纹不变/tmp空/0活动会话。四完整正常native图共130节点（旧24+新106：fill75、handler24、raw22、mapper9）；transform52Listing/10native、Raise24Listing/1native、bulk109Listing、strict22Listing及codec398Listing均单独计，不混成全部native或传递闭包。

e950895已二方46/0，9aac717已二方44/0，原失败和partial快照保留。旧静态拒绝已解除，只作历史记录；本批新语义仅作者来源/文档一致性检查通过，**待独立复核**。runtime schema1与全部候选对象/执行DENIED保持，没有游戏/派生/arm64游戏执行、转换、组件测试、目标OS API调用、联网/上传/安装或UI/设备操作；不用旧Swift或城市仿真解释。

下一可验证缺口：①按当前load/open原来源建立F标记、两个长度与实际out-count的有效域和缓冲范围，保留本文所有未检查return；②将codec wrapper的out-count/store与内层case/默认alloc/free正常/错误效果分别闭合，不能以398条库存代替证明；③source限定的window callback/alias及RaiseException catch/frame/lifetime。实际文件、首tick或完整游戏通过仍需相应合格运行证据，不能由本批静态结果推出。

仅本地提交。push仍独立待用户精确目的地/载荷授权，旧5587fb5八文档审批不覆盖新HEAD；没有推送重试或替代上传。
