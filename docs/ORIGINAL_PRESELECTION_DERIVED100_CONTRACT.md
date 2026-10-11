# 前置选择真实副作用与选定派生+100

固定原源 SHA `8a6d2df1…6753` / profile `e3402f2d…a7b2`。**没有找到owner+34非零store；两次guard值相等仍未证明。** 本批收敛了前置调用的确切写入、模态循环，以及一个构造表匹配方法的正常ABI；没有游戏执行、live绑定或调度启用验收。

## owner34生产源现在定位到哪里

三个已识别4fb530 caller都从408ac0取得固定owner10bc348，字段地址为10bc37c；这是调用源条件，不是当前运行观察。该字段静态xrefs为空，只表示没有ReferenceManager引用，不能证明无指针/别名store或恒零。

| 调用 | 已识别直接写 | 后续缺口 |
|---|---|---|
| 500400 | owner8非空时503260写linked+170=0。 | linked当前对象/别名未绑定。 |
| 5003a0 | 503210写linked+2c4=所读10de2d8。 | 接着4ff170(-2)构造/登记238记录；不是owner34 setter。 |
| 413a40 | 写owner+0=5。 | 仅确切偏移/值，不据查询标签命名类型或vptr。 |
| 4fca20 | 在4fd600中写owner+10=0，之后才最后清owner34。 | 后续选择/其它callee可覆盖；bool predicate与mask1谓词不同。 |
| 5032c0 | 4fb8c0成功分支写linked+2e0=1。 | 不直接写owner34，后续62e7f0有动态作用。 |

4ff170分配0x238、502a20在DF0下清0x238并构造record并经4ff590→58ee60登记；分配空分支后仍有ESI+10解引用的原源失败边。4ff760则读取linked并tail58ea10：分配0x18、构造4fa7e0后登记。源失败边不是实测崩溃，构造/容器/同步/异常作用尚未闭合。

## 4fb8c0确实进入当前虚方法循环

62e720的一个分支可调用当前virtual+4c后返回0；主要分支先改current1e55200的+58/+5c/+60，再取62d070所读global1e551e4，以它为receiver调用61b510。返回后重新读1e55200，再取当前+58，不把最早receiver或返回对象默认看成同一个。

61b510的46条完整native正常局部图显示：self+148为零早退-1；否则保存并清+dc、InterlockedIncrement(self+4)、修改+f8/+fc，循环调用**当前virtual+4**直到+f8>+fc。之后恢复+dc、递减+f8，先捕获self+100作为返回，再InterlockedDecrement(self+4)；其有符号结果<=0时可能调用当前virtual0(1)析构。循环目标/写集合、结束条件、析构和异常都未绑定；没有运行UI或OS API。

62e7f0成功流程写current1e55200+60=1，两子对象经66ce40的真实computed JMP到当前+8；后续62db60可tail到lookup对象的当前+44。resolver在这两个JMP处给not-call，不能解读为无调用作用。6249e0另经目标+28调用6ef160，传递alias也未知。

4fae70分配0xdc widget，6c4a80经baseconstructor后写root7c8344。模态61b510 receiver来自另一global，且访问+148；**不能直接把该widget根当模态current+4的实际目标**。下一生产源应追global1e551e4的来源及其current+4 handler，而不是把邻近factory或空xrefs补成writer。

owner34第一次检查前有以上不透明作用，之后两guard之间另有virtual+10、完整loader/reconcile、登记和消息作用。尚未找到明确使两值不同的store，亦没有完整无覆盖证明；因此保留两个独立读取，不宣称它们实际改变、同值或所有load都reset。既有条件reset0/0/0/1和首次quota/gate/hold门控保持。

## 只对所选根收敛派生+100

4c5dc0固定root7afbd8的+100=4c69a0，正常尾RET0x18确认六参数清理，只有AL1而非fullEAX1；FunctionManager猜测的零参数signature不替代原ABI。整体84条Listing/237字节/4块局部图已重建，另核23个关键native，**没有把84条说成全native闭合**。

同根+25c=4f13f0，四条完整native正常叶：signedword O14乘3，再作8倍索引读取8235a0，即24字节步长；输入和表范围未证明有效。同根+1e8=416b50，两条native返回O+c8，因此在各虚调用保持选定根的条件下，4c69eb写**O+128=当前101c500的低byte**。ReferenceManager的当前call依旧computed unresolved；固定槽字不是live绑定。

其它直接helper可见101c504/101c500、错误字段101c52c及索引数据数组的写入，本批仅保留字段/地址和ABI，不解释城市模拟逻辑或假定有效输入。对象/容器重定位、传递gate/quota/hold覆盖与异常仍Unknown，局部无free不当保活证明。

六参数postload的第五参数为0。选定原正常ABI下，保存四寄存器16字节、仍在栈上的cdecl参数16+32+16字节，使4c6a49的[ESP+0x64]正好对应原入栈+20，即arg5；随后ADD0x40清64、pop16、RET0x18。原helper所有正常RET清理形态与source ledger相符。故这个条件路径在4c6a52跳过两次+26c；一般arg5非零且O150非空的路径仍可调用它们，不能推广为统一启用规则。这里只做常数偏移算术，没有guest寄存器/内存解释器。

## 审核、验证与下一步

9beed22速度修正与82d9cf5已独立38组通过/0失败，旧b2 38/1收据保留，空owner条件失败闭合。描述符原字现明确为**0x928（2344字节）**；旧提交快照的待复核字段保留，由新增review update记最新结果。本批新结论尚未独立复核。

139请求/138Evidence/1NoProcedure，官方exit0/746.741秒，输入与18模块指纹不变、临时空、无活动分析会话。三完整native正常图52节点（modal46、25c4、1e8 2）；另84条Listing图仅23native锚点、80个native与Listing匹配。CALL只条件正常返回，不证明异常、循环终止或实际游戏运行；NoProcedure仅元数据缺口。

接下来优先：①global1e551e4来源与匹配的current+4 handler，找到owner34非零store及覆盖，或明确证明仍未绑定；②只沿所选4c69a0的helper/lookup按确切写集合核门控与alias，不能补造值域有效或开调度；③构造/登记与raw cursor异常/存活边，保持reset的分支前提。通过标准是固定源/receiver/ABI/确切store及正常和异常覆盖，运行事实仍需独立合格证据。

仅本地文档提交；不运行原/派生游戏或组件测试，不用GameData/旧Swift，不恢复转换/SR/Unicorn/Wine/历史被拒探测，不联网或操作UI/设备。旧558八文档发布仍待用户精确授权，新HEAD不自动包含。见[筛选证据](original-preselection-derived100-evidence.json)与[handoff](ORIGINAL_ROUTE_HANDOFF.md)。
