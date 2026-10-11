# phase37：原虚调用+170的条件目标、receiver与返回合同

阻碍的是44f1d0在44f2d2的computed CALL、返回44f2d8，随后44ecd0七参数消费，最终主调度return4ac486再进入commonadvance。官方resolver仍unresolved/targets[]；没有实际实例绑定或原代码执行。

已有四个构造首word写与固定主体表字证据共同给出：426c90→7ab59c、42d480→7aba38、5630b0→7b887c、56fb50→7b8ad4，各自+168/+16c/+170字均为413a00/416ac0/416ad0。构造最终首word不证明当前槽vptr；完整表范围也未证，不因读到目标字而宣称数组范围或实际动态类闭合。上述四个+170字地址亦见未分类xref清单；xref本身不等于运行绑定。

416ad0/416ac0没有procedure或containing procedure。五次原函数查询失败原样保存，未强建函数。官方Listing把416ad0处连续6字节解为清零EAX、读取ECX+7字节到AL、普通RET。因此**在实际控制转移到该入口且receiver可读的条件下**，EAX为unsigned8零扩展0..255，清参0，四个预压值保留；没有CALL、写或释放。416ac0首3字节仅RET16，不写输出指针。不能用短前缀结论冒充函数管理器注册、完整vtable范围或异常安全。

413a00已有完整短体XOR AL,AL/RET；若两次vptr读取保持选定表、对应表字保持固定主体所见值，+168 false路径会选+170而非+16c。caller从当前raw slot加载O到ESI，426d10读O+4并RET4；四次44cc50使用selector1..4的路径只读表值。此选定前缀没有本地release、slot替换或vptr写，未见retain/pin。读取O+4/+14/+3c/+7及后续+a/+c依赖存活内存；当前类、两次vptr、并发/重入写及载入身份仍未知。局部未失效不等于生命周期通过。

最终44ecd0的七项32位栈序为signed16[O+a]、signed16[O+c]、candidate170的zeroextended8[O+7]、四个已保存selector1/2/3/4表值。四值先压、跨虚调用，再压前三项；44ecd0 RET28消费全部，虚调用普通RET保留原帧。这补上88515658的正常平衡推导所需候选原指令依据；实际computed站点仍未解析，其他子类/覆盖不能默认同样清参。

后续直接callee正常清参已闭合：44ed90五个32位值RET20（EBP恢复对齐栈）；44d090三个32位值含outparamRET12；44cf00/44cf20单值RET4；44f180单值RET4且只AL定义byte，caller显式MOVSX AL，不能把EAX当标准化32位结果；44f190/44f1b0两值RET8；44e770两double64 RET16，内部44e4d0单doubleRET8；4bb390五值普通RET由caller清20。44cf20按signed count递归，原存储/输入域和资源深度未证；x87条件/异常环境仍未知。没有实现这些原算法或新增原运行绑定。44eb20最终链与47f1b0索引槽helper复用既有证据，普通返回合同不证明其自然输入、存储或阶段副作用完成。

本批36操作：31 Evidence与5项保留NoProcedure错误；12个选定地址、5个Listing锚、64显式数据字节。453.227秒exit0、官方close、临时根空、输入身份及18份选定官方模块成对SHA不变。新语义未独立复核；自有校验器/测试未改未重跑。机器合同见original-scheduler-contract.json的phase37_virtual170_conditional_contract，完整调度仍未通过。

## 后续原静态合同

426d10门槛精确为O+4字节等于1或3，完整EAX0/1、RET4，不能推广奇数；flag5返回0。该[更正](ORIGINAL_FILTER_CONTRACT_CORRECTION.md)不改变当前vptr、slot、实例及存活未知。
