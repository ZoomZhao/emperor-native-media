# 原版桥接的静态 ABI 描述与拒绝边界

描述来源是提交 `300a5a7945e4ac4c748cb61228e3857784f76446` 的固定文档与schema5快照。整合时已对照主线 `4eade8d` 的schema8及已发布更正：5个调用站点、14项字段的选定宽度/顺序事实仍成立。原补丁及二方复核的SHA、命令和测试收据保存在本地任务目录；本模块只消费这些静态描述，不读取原输入、GameData或旧Swift，不执行原指令或操作设备。

## 最小有用发现与本补丁

当前 [runtime.h:9](../legacy/src/stage82/tools/runtime.h#L9) 的 `S80Binding={uint32_t pc; void *target;}` 不携带receiver、当前vptr/表范围、ABI/所有权或实例代际。[runtime.c:52–62](../legacy/src/stage82/tools/runtime.c#L52) 的set_bindings仅查空目标/冲突，且原注释明确table membership不能授权动态调用；动态domain本身另有未闭合提供者。[native_driver.c:23–25](../legacy/src/stage82/tools/native_driver.c#L23) 连接这些表后激活一次性切片，没有稳定完整原tick消费者。这不是仅增加一个裸函数地址就能修好的接口。

可闭合且直接服务桥接的是**声明形状的旁路校验**：新增 `original_bridge/include/static_abi.h`、`src/static_abi.c`，用C11自有数据结构记录5个已证call-site和14项偏移/字段宽度；验证主体/profile/schema、32位载体、栈项数/位宽、明确literal实参、声明区间和槽边界，并输出具体拒绝原因。头兼容C++，不依赖legacy头、CPU/guest_memory、平台provider、原二进制或转换库。

`OAbiInput` 是**宿主诊断描述**，不是原对象/CPU/线程快照、文件格式或packed原内存布局。地址/实参先用uint64载体检查高位，不能将宿主指针截为guest32；对象字段只列局部读写事实，不补整类sizeof或业务名。C/R/P/O/Q/J角色及两种+8字段分开，选定聚合记录stride16不等于另一个记录对象的完整布局。本最小描述没有覆盖O+0x48或全部原字段，不借相邻偏移补猜未列字段及整类布局。

## 已记录的调用边界

| call-site | 声明事实 | 仍不能绑定 |
|---|---|---|
| `0x4aef47` | ECX=O，虚槽+c8；一项32位literal35，caller只用AL8 | 当前O、最终vptr、表范围、实际callee/清栈/副作用未闭合；局部候选RET4不能赋给该未知目标 |
| `0x4aef92` / `0x4aefaa` | 两个独立站点，顺序标签1/2，虚槽+264，各一项32位载体，使用EAX32位型 | 不合并、不要求跨调用参数保持相等，不发明返回signedness、缓存或完整目标ABI |
| `0x5fc139` | ECX=current payload，首虚槽，一项32位literal1，返回未用；前置guard记为signed decrement32≤0 | 不把基类0x5fb520当实际派生目标；不执行减引用、flags解码、释放或调度 |
| `0x5fbf47` | direct helper候选0x718850，两项32位栈载体，caller清8；返回用途未给定则Unknown | 局部normal copy不证明对象存活、异常回滚或完整引用转移；仍不可运行绑定 |

主体SHA/profile只是与**已有静态声明**对齐；没有读取输入文件来重新认证原文件身份。聚合实参只校验32位载体，不证明生产它的原控制流、实际取值域或合格自然输入。虚表字节区间来自caller声明：数值范围/槽边界可检查，但其真实性、完整表范围和方法归属仍Unknown；声明较大范围不能清除缺表/缺实例的原因位。

## 拒绝结果与消费者最小增量

API返回 `OAbiReport`，不是成功bool或目标函数指针：

```c
#include "static_abi.h"
OAbiReport report = oabi_validate_static_input(&description);
/* report.shape_status只能说明声明形状。 */
/* 本版所有report.binding_decision恒为OABI_BINDING_DENIED，不能构造S80Binding。 */
```

`OABI_STATIC_SHAPE_ONLY` 仅表示字宽/项数/literal/声明区间未发现冲突，**不表示目标获准、函数成功或原调用已发生**。未知site/超宽/literal错/声明槽越界返回shape rejected；目标未知时必有UNKNOWN_TARGET且binding denied。已知直接helper也只有STATIC_TARGET_ONLY。所有结果保留RUNTIME_FROZEN、LIFETIME_UNCLOSED、EXCEPTION_UNCLOSED；虚调用另保留TABLE_UNKNOWN/INSTANCE_UNBOUND。显式请求runtime binding直接拒绝。本API没有BindingAllowed状态、host code pointer、读取guest、dispatcher、注册、执行或成功stub。

未来真正接到现有原版消费者的最小证据/接口仍缺：

| 项目 | 需要的字段与来源 | 当前缺口 |
|---|---|---|
| 当前对象身份 | guest32 receiver、来源/对象代际、活状态/释放状态、消费时vptr；P/R/O或Q/J角色标签 | `S81DomainCase`只有site/reg/value/slot/target；同数值receiver或同池索引不足以证明同一活实例 |
| 虚表方法归属 | 当前表base、经证实的完整extent、槽offset、读得target及来源Evidence/subject/profile | 偏移字读数可落邻表；当前phase8.targets为空，构造候选不认证消费时表 |
| 实际call ABI | ECX/栈项、真实callee清栈、返回使用、每个站点顺序、异常合同 | 本模块只记录caller已证形状；不赋用候选callee合同，不实现caller/callee动作 |
| 索引与记录生存 | 32位登记索引→R+8低16→signed扩展的完整值域/别名规则、enabled后续写入、池backing边界及生存 | full_index_roundtrip_proved=false，later_record_writes_excluded=false；不能据low16相等绑定原32位对象 |
| worker释放 | 真实payload/派生首槽、listener/parent/节点/current引用转移、allocator模式/heap、SEH与异常 | 基类候选和normal retain/copy/release链未闭合全任务所有权；不能从RET4或flags1推出释放成功 |
| 持续原tick消费者 | 合格入口/完成边界、活状态owner、完整线程/可变opaque/事件/异常及下一tick协议 | 既有driver完成后输出诊断并退出；不能把诊断/初始sidecar当完整恢复状态 |

来源分别见 [REA_ABI_CONTRACTS](REA_ABI_CONTRACTS.md)、[候选清单](rea-abi-allowlist-candidates.json)、[原tick接线审计](ORIGINAL_TICK_WIRING_AUDIT.md)。没有这些闭合材料，本补丁不能安全实现live resolver或启用原版运行；不通过再造城市玩法填补。

## 300a5a7二方纠正的隔离

用户补充的 `rea-record-link-300a5a7-review/REVIEW.zh-CN.md` 和保存汇编/Listing已按原SHA核对并保存在 `evidence/original-abi-record-review-input`，包括原19组16PASS/3FAIL报告。三项应纠正为：0x5451a0测试第五项32位参数低字节（RET24清六项不等于测试第六）；0x546c40读取O+154中的指针再索引（旧文漏解引用）；5dc860调用在PUSH之前，预压索引只跨506240。没有重新解码或执行它们。

本版不注册上述helper/入口、不实现记录地址公式、参数序号或PUSH/CALL步骤；单测确保该前沿Unknown拒绝。主线负责修正文档及继续分析，本补丁不改原证据，不将审核错误抹成通过。选定记录的4/8偏移和16字节stride由二方明确确认，作为独立宽度描述保留；不据此恢复数组基址或取值算法。

## 只编译新增自有代码的验证

```sh
python3 scripts/original_abi_validation_runner.py --mode release --output /tmp/original-abi-release-NEW
python3 scripts/original_abi_validation_runner.py --mode undefined --output /tmp/original-abi-undefined-NEW
```

输出必须为未存在的绝对任务目录。runner只冻结本新增H/C/测试C三文件，系统clang以C11编译并链接这些自有描述校验函数与标准库；另以C++17仅检查新头。每子进程≤60秒，不include/link/load归档桥接/原派生目标，不启动REA/provider，不读取原输入或创建guest映射。自造地址/参数位型用于宽度、缺项、声明槽越界/最后4字节、Unknown目标、候选冒充绑定及新审核排除边界测试；没有虚拟CPU或原指令语义解释器。

最终Release/UBSan各 **167检查通过**，C++17新头独立语法检查通过。仅编译/链接新增C源和自造测试；先前151项通过的收据仍保留，追加二方纠正前沿Unknown拒绝后重跑167项。结果和源/命令/日志/二进制指纹见交付manifest。通过仅说明本自有描述校验合同，没有原游戏启动、完整tick、原行为对照或真实对象绑定证据。

## 2026-10-11整合与最新池边界

`OABI_SCHEMA_VERSION=5` 是本模块固定描述协议的版本，源于上述快照；它不是持续更新的研究清单schema版本，不应随清单schema8递增。新增池/清理证据未被自动登记为调用站点。代码和测试保持原补丁内容，整合仅更新本文的来源、适用范围和验证说明。

主线已确认：默认构造尝试建立4000槽，载入数量以32位输出参数读取；所选载入体没有4000或32767上限判断。4000不能作为全局合法索引上限。记录ID由低16位有符号扩展，1..32767仅是非负索引保持非零无别名往返的数学域，实际载入数量、O+b4槽身份和当前实例仍未证。本模块没有池数/记录ID校验器或载入算法，不能据静态shape检查恢复这些关系。

独立复核通过：Release/UBSan每种模式作者167项、自造描述输入51,396个case，另通过C11/C++17头独立检查；伪造虚表范围仍保留Unknown且拒绝运行绑定。整合逐个核对新增H/C/测试/runner与审核SHA一致，只改本文；未重跑或链接原消费者，也未宣称原行为通过。后续可以继续补自有数据结构、纯校验器及自造测试；live实例/实际目标、异常和生命周期闭合前，仍不能启用原版运行绑定。
