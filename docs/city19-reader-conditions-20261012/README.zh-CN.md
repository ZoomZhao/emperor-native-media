# 最小条件表：普通库合同与实际读取类型

基线 `e4ef565fe073c327c19cec6115c1131401b6aea4`。采用有限独审22/0及其收敛分类；本批只做已定位辅助的静态来源核对和已保存模型指纹检查。没有原/派生执行、转换、真实payload读取、城市玩法分析、旧Swift、UI/设备、网络或认证。来源见 [source-evidence.json](source-evidence.json)。

| 分类 | 最小条件或已证事实 | 结论边界 |
|---|---|---|
| 原来源已证 | 7656df在765705测试NULL，765707直接跳7657b9；跳过所有callee和heap路径，只做正常帧/SEH收尾 | NULL路径不访问应用对象/root；不证明非NULL或非法free |
| 原来源已证 | 47b3b0传flag0给408690，后者直接清零字符串receiver+4/+8/+c，无callee；receiver为global10bc088+4 | 字符串控制字段不是选定对象自身+14或vptr |
| 原来源已证 | 408660扫描源字节；408610复制指定字节；4087d0写字符串长度，经408800写buffer+n终止byte | buffer、长度与DF0合法性是下列合同前提 |
| 原来源已证 | 注册29函数闭合键比较/复制及树、迭代、tuple、栈的写集合；元素清理末端RET | 键可以重复引用同一对象；无需再证明输出容器为空 |
| 原来源已证 | 52c990在owner+5c4调用52c440，52c443安装7b6be0；选定reset不覆盖root0，parent raw请求与reset范围不覆盖+5c4 | 这是名义构造/局部写事实，不证明实际构造到达或当前实例身份 |
| 正常有效运行假设 | 合法对象/树/帧、成功正常返回、普通独立分配及持有期间有效；合法CString缓冲、容量、NUL源、DF0 | 不再递归证明整个CRT、任意损坏heap或异常字符串 |
| 精确存储假设 | 实际写入区域不覆盖保护的holder+30、对象root及必要字段 | 不是“所有指针都唯一”；允许键/引用指向同一pointee |
| 实质读取未知 | 51208b时holder+30指向哪个存续对象，51209d时实际root及其virtual+8 | allocator正确不能证明没有合法替换/重构 |
| 实质读取未知 | 可达应用callback经child/holder/存储引用进行的合法指针/root写入 | 不能用“无异常别名”排除真实程序引用和callback作用 |
| 实质读取未知 | 完整FigureFSA的base、own版本、字符串/计数和嵌入reader | 独立1479字节owner模型不能替代完整FigureFSA |

408920的procedure提取只到408970→408995，未取得完整尾部。因此缓冲准备/分配仍明确采用普通CString合同，**不称原实现已全部验证**。沿尾部的额外会话根据最新委派停止：只SIGINT经ownership和精确命令确认的owned Node，官方取消/清理完成；exit1是主动取消，零有效source artifact，不是安全或工具审批拒绝。不再因此阻塞读取类型主线。

已保存的实际入口来源：5122c0仅返回holder+30，无write/callee；511ea0在51208b调用它，51209d调用返回对象virtual+8。已有名义4dad10完整体先调用base4f4480，随后读取自身版本并分2..5路径，含可选字符串、计数和三处embedded virtual+8。下一主线应绑定当前holder/root及这些实际reader调用，避免把构造时空容器或+34初始0当作运行事实。

**具体条件化现代模型单列。** 若明确选择52cd90的own6路径，52d5f6接收者root为7b6be0、slot0为52c480，采用正常合法存储/完整读/保护字段分离合同，现代支持版本6/1，则现有 `walk_city19_next_owner6_nominal_record(..., nominal_52c480)` 消费1479字节（1461+18）。它始终 `verifies_original_instance=false`；不能靠版本字值推断实际类型，也不等于完整FigureFSA。子reader原源码不拒绝其他输入版本，1是现代选定子集。

保存的独立Release9/0、UBSan9/0合计18/0及当前源码SHA匹配；本批未改生产组件，没有重复编译/重跑或新真实文件解析。见 [conditional-modern-model-saved-validation.json](conditional-modern-model-saved-validation.json)。此证据只验证条件模型，不验证原实例。

**无条件真实前沿仍1991913，新增解释0B。** 条件、来源事实及实质未知的机器表见 [minimum-conditions.json](minimum-conditions.json)。普通库递归封账；读取类型与字节合同继续作为独立主线。选择性本地提交，推送等认证恢复，不重试认证或更改远端。
