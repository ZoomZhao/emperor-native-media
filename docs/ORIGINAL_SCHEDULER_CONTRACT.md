# 原版城市完整调度合同与接口准备

本批静态合同基于固定subject/profile及已保存REA证据；没有执行原/转换代码、重新转换、加载衍生库、读取GameData或操作设备。已核51项主分派、时钟/额度/阻塞门、父调用者及周期尾的局部顺序；不是完整原行为通过。准确机器可读描述见 [original-scheduler-contract.json](original-scheduler-contract.json)。

## 实现者首先要保留的边界

| 层次 | 原入口与返回标签 | 完成含义及缺口 |
| --- | --- | --- |
| 刷新调用 | 536fe0 PUSH1→536b20，返回536fe7，caller POP清4 | 可能0/1/多次父调度；还含呈现/状态切换并可能递归536b20(0)，不等于一个城市周期 |
| 父调度 | 5371a0依次4189b0→529990→5ef190→5eff20；仅88ec38=0时CALL4ac2b0@5371bf，return5371c4后466e90；再4e27e0；重新检查gate后尾跳564b50 | 返回/计数不能替代实际phase推进；需保留两次gate读取与前后依赖 |
| 单步分派 | 4ac2b0，phase8只为CALL4aeeb0@4ac338/return4ac33d | precheck可停留；正常体只选择一个phase，不从phase8返回推出整轮 |
| 完整51步周期 | 支持的正常phase0..50；phase50 commonadvance到51后phase=0，4ac4e2 JMP4ac650 | phase已归零但tail尚未返回时仍未完成；尾跳复用原returnframe，不能新增CALL帧或仅看归零字段 |
| 更慢周期 | tail更新c82ef4及c82ef0；signed sub>15的periodic分支把sub暂置15再0，并递增c82eec，signed>11归0 | 16及12是数值周期，未用它们冒充已验证日/月/年或具体业务 |

4ac2b0前置链为408ac0（返回owner地址10bc348）→4fae50（测试owner+10），满足后55bcb0（owner12a4ba8）→55b540（AL），再55ce80（owner12a3d70）→55cee0。三项均非零时在4ac4e7直接RET，不进入分派或selectedadvance。55cee0有全局写入、间接调用，最终EAX为signedbyte10de044；不能把这个副作用函数简化为无副作用bool暂停检查。这里的停留与外层阻塞flag分开。

正常完成仅指所选调用链正常回到commonadvance；该处重新读c82ef8，再INC并signed比较51。不是必然递增入口时保存的phase：callee、别名、线程或递归是否改变phase未排除。异常、异步任务真正完成及具体对象/实际动态目标仍未知；各行不能理解成native空成功stub。

## 全部主阶段与原调用顺序

所有行先受父gate、quota和上述precheck约束。空行仍进入commonadvance；有调用行需其原链正常返回后才能advance。case13仅591ec0及两byte写有独立条件，5925a0始终调用。阶段内部条件未因本表而闭合。

| phase（十进制） | dispatch block | 本地动作，顺序不可调换 |
| --- | --- | --- |
| 0 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 1 | 0x4ac2fa | 0x4f1560 |
| 2 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 3 | 0x4ac42d | store32(13f8e78)=1 |
| 4 | 0x4ac304 | 0x4819f0 |
| 5 | 0x4ac30e | 0x54c6f0 |
| 6 | 0x4ac31d | 0x5177b0 → 0x5185c0 |
| 7 | 0x4ac32e | 0x4af350 |
| 8 | 0x4ac338 | 0x4aeeb0 |
| 9 | 0x4ac342 | 0x40fc20 |
| 10 | 0x4ac34c | 0x4aeb30 |
| 11 | 0x4ac356 | 0x590a50 |
| 12 | 0x4ac360 | 0x4ace60 |
| 13 | 0x4ac36a | 0x591ec0（仅1312564=0） → 0x5925a0 |
| 14 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 15 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 16 | 0x4ac398 | 0x5d3cb0 |
| 17 | 0x4ac3a2 | 0x4aedf0 |
| 18 | 0x4ac3ac | 0x426a60 → 0x5a8170 |
| 19 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 20 | 0x4ac3bf | 0x51e4a0 → 0x4bc440 |
| 21 | 0x4ac3ce | 0x4acfc0 |
| 22 | 0x4ac3d8 | 0x4ad3d0 |
| 23 | 0x4ac3e2 | 0x4ad4a0 |
| 24 | 0x4ac3ec | 0x4ae1a0 |
| 25 | 0x4ac3f6 | 0x4ae220 → 0x4ae7f0 → 0x4f19a0 |
| 26 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 27 | 0x4ac40a | 0x5253a0 |
| 28 | 0x4ac414 | 0x4ace30 |
| 29 | 0x4ac41e | 0x54c6f0 |
| 30 | 0x4ac42d | store32(13f8e78)=1 |
| 31 | 0x4ac43c | 0x416d20 |
| 32 | 0x4ac446 | 0x5dbcc0 → 0x4b02a0 |
| 33 | 0x4ac452 | 0x4af230 → 0x4af2d0 |
| 34 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 35 | 0x4ac45e | 0x5177b0 → 0x517b40 |
| 36 | 0x4ac46c | 0x5177b0 → 0x517ad0 |
| 37 | 0x4ac47a | 0x53c870 → 0x44f1d0 |
| 38 | 0x4ac488 | 0x4acd10 |
| 39 | 0x4ac48f | 0x519120 |
| 40 | 0x4ac496 | 0x4acc40 |
| 41 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 42 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 43 | 0x4ac49d | 0x4af9b0 |
| 44 | 0x4ac4a4 | 0x4af5d0 |
| 45 | 0x4ac4ab | 0x5f02c0 → 0x4b94a0 |
| 46 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 47 | 0x4ac4bc | 0x4afd20 |
| 48 | 0x4ac4c3 | 0x4ae8b0 |
| 49 | 0x4ac4c8 | 无本地handler，进入commonadvance |
| 50 | 0x4ac4c8 | 无本地handler，进入commonadvance；达到边界时尾跳4ac650 |

phase5/29分别PUSH32(0/1)到54c6f0、caller ADD ESP4；所选callee还尾跳4ea8e0，因此不能用当前body结尾伪造完整callee RET合同。phase6/35/36先5177b0、ECX=EAX后5185c0/517b40/517ad0；phase18先PUSH0，426a60仅返回owner1312f08且RET，ECX=EAX后5a8170消费预压参数，所选RET4；不是426a60先消费0。phase37先53c870，ECX=EAX后44f1d0；phase45 ECX=1e535f8→5f02c0→4b94a0。phase13若1312564=0，写1312870/130f967为ff并591ec0，然后ECX130f960→5925a0。phase1有九项后续链末尾JMP4affa0；phase44装ECX8c7630后JMP42d9a0；phase47清四个32位字段后RET。phase9的40fc20已静态核为RET，仍不能把任意未知handler换成它。

## 共享时钟、暂停/模式门与额度

536b20入口采timeGetTime(IAT7a9534)→1156dec；53a130另采同源时钟，与115f540作32位减法后signed判断，负值替换为10000。speed派生index为signed32的(100−10de0dc)/10向零取整；index≥10返回0，负index在后续被夹到0。十项阈值为0/20/35/55/80/110/160/240/350/500，加2后与elapsed比较。不能额外假定速度输入0..100或把host64时钟直接截断后称为同一epoch。

byte10de0d0、word32 88ebd8、word32 10de038任意非零都可阻止正额度；5369a0的所选mode允许集合为1/6/18/35/39/72/73/86，但18/35在53a130另要求10be9dc=1。未证明这些地址的完整UI暂停语义；这里只把暂停/阻塞/模式入口的精确读取条件交给接口，不发明用户动作。

正额度返回10000并更新115f540，否则0。536b20在额度>0时循环5371a0，但continuebyte10de3f4=0或所选取消消息会把quota清0；每轮仍调用535770，再递增loopindex。PeekMessageA只是静态所见调用，本批没有操作消息泵。115f550计数父调用而非phaseadvance；10000是最多循环额度，既不是已经完成的phase数，也不是完整周期或执行器wallclock预算。536b20还有mode=0早返、1156df8=0早返及load/state-change分支；后半呈现/加载合同未被简化或执行。

## 周期尾的依赖和完成

4ac650先更新total/sub，再590d40→468160→468210；AL条件下另走468160→468350→468160→4684a0，随后42da70。sub>15进入大周期分支：按outercounter筛选4bfdb0→4bff20，再4bfdb0→4bfe10；暂置sub15、累积字段转存/清零，依次592060→590db0→4ae900→4ae950→4ac5b0→5930b0→5177b0→518690→55af70→591db0→426a60→5a8620→547670→4b6d00→5ad8f0→547440→4eb850；随后sub0、outer+1，outer>11分支还有592360→480740→481f10→445130。之后还有outer匹配、total模16/48、55cee0结果等条件链，均保留在源Evidence与JSON地址序列；地址序列不能当成无条件调用序列。

共同593090后，10de044非零可提前RET，前面的counter与副作用不回滚。非停留尾部继续sub0对象扫描与条件链，sub0/8、6、4、outer1且sub2、sub7/2/3/12、sub低两位0分别有依赖，最后40fc20两次→4bfdb0→4c0570→RET。仅注册4aeeb0或4ac2b0地址不能覆盖这些依赖。实际callee约定/对象、所有权、线程、SEH和有序写尚未全闭合；机器清单只保存条件敏感的原地址顺序，未替实现者执行业务。

## 当前转换覆盖与可落实的接口

本仓库legacy/INDEX、import-manifest、static-dependencies和build_drivers源码共同表明：归档工具历史上读取stage81/builds/full-separated-v5/game.ll，期待2020个结构目标，但当前归档没有此IR、original.json、build_scope.h、domains.c或original-v3/native-driver。未重新转换、编译/加载原产物。对主调度全部阶段及tail函数，**当前可核验已转换集合为空，转换覆盖未证**；不能把“迁移仓库缺文件”改写成“历史从未转换”。历史配置/报告引用与当前可用产物要分开。native_driver的完成PC/SP只约束切片，不能提升为整轮消费者。

当前最关键可落实接口是 [scheduler_shape.h](../original_bridge/include/scheduler_shape.h) 的`OAbiSchedulerInput/Report`：独立schema1，64位地址/phase载体先验宽度，51项不可调用的整数阶段描述，固定主体/profile，支持的静态单步入口4ac2b0/return5371c4。报告保留FRAME_UNBOUND/TARGET_UNBOUND/EFFECTS_UNCLOSED/CYCLE_UNPROVED，所有binding denied且cycle_completion_proved=0；phase8切片入口/返回冒充单步会拒绝，任何单步fullcycle claim拒绝。它不读取guest、不推进phase、不实现原指令、不注册目标或生成successstub。现有5站点ABI preflight未改变或重复测试。

未来真正的原桥接需显式携带phase读取时刻、advance读取时刻、三个周期counter、clock epoch/last sample、阻塞flag和pending原returnframe；区分Held/Skipped/StepReturn/TailPending/TailReturn及Unsupported。phase归零时不能提前宣布完成；caller/callee真正完成与整轮有序trace/上下文闭合前，不启用运行绑定。完整性与资源预算若由适配层加严，必须标成适配层政策，不照搬负数量、短读未检或截断别名作为兼容算法。

新增自有校验器Release/UBSan各503自造检查、C++17头独立检查通过；源码阶段描述与已保存跳表/调用点逐项核对。它仅证明声明校验合同。静态批次47操作、20查询函数（17新地址）及3个复用主体，543.984秒exit0/官方close/临时根空，显式数据读592字节，18份选定官方模块成对SHA及输入身份不变。上轮6c13ce7已二方22组通过；本批新语义/新校验器尚无二方复核。所有原执行/转换/运行绑定仍关闭。

## 三个阶段尾部的实际返回合同（后续静态批次）

phase1：4f1560尾跳4affa0，再尾跳44bab0；后者CALL44b9b0并JMP44ba30，最终44baa1普通RET。两个叶函数各有7次本地表访问、没有新的CALL或间接调用。各JMP复用当前returnframe。只关闭这段尾链；4f1560前面的其他依赖仍需独立合同。

phase5/29：54c6f0先恢复ESI，再JMP4ea8e0；后者4eac49普通RET，主调用者仍清理原0/1参数的4字节。88ec38=1时可跳过尾部扫描；其本地外循环99次，所选内部搜索最多16次。这些静态局部界限不约束子调用的运行时间或证明返回。4ea050复制7个32位参数到4e1420，后者正常出口RET28清理副本，4ea050普通RET后其调用者另清原参数。4e1420有构造选择、allocator依赖、虚调用+4/+18/+e8/+ec和FS:[0]异常帧（handler78614a）；尚未绑定实际类，也未闭合异常路径，不能把普通RET改成无条件完成。

phase44：4af5d0设置ECX=8c7630，尾跳42d9a0，正常出口42da05普通RET；d62408=1可跳过。本体用有符号IDIV读取817748作为除数，未见本地非零保护；运行时取值未证。owner+4交给4f8210/4f8200，故begin/end分别读取owner+8/+12。当前槽对象ECX上虚调用+30，压参顺序0、0、余数，随后槽指针加4并重新读取end；真实callee清理、对象存活、回调是否改end及循环收敛均未证。返回AL聚合非零时依次5ad8f0、5ad960尾跳522810；前者再调用5ad440（5参数、caller清20），后者调用5223b0（4参数、caller清16）。这两个依赖又含虚调用+cc/+264/+270以及+1ec/+cc。不能把顶层RET地址当成这些动态调用已正常返回的证据。

本批31次查询、14个选定新地址均有Evidence且无错误；357.910秒exit0、官方close及临时根清空、输入身份和18份选定官方模块SHA成对不变；未额外显式读取数据。此更新仅补充静态合同，scheduler校验器、preflight及其测试源码未改变，未重跑旧检查。Release/UBSan各503检查是上批自造声明结果，本批新语义没有独立复核。原/转换代码执行和运行绑定仍关闭；现有证据不能证明完整城市周期通过。
