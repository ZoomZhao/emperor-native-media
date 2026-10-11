# 原版完整 tick 接线依赖审计

2026-10-11，只读审阅授权平台源码、接口定义与既有文本/JSON报告。审阅基线为 af588457fcd9115a7084621c65d6bc93b806a47e。本轮没有编译/测试、运行历史工具或原/派生游戏库，没有读取 EXE、原 IR、旧 Swift 或 GameData，没有操作或重测 UI/权限。报告中的原现场、入口与 ABI 元数据取自既有报告，不进行新的原指令分析。运行源码保持不变。

## 最先挡住完整 tick 的四项依赖

| 依赖 | 已知事实与当前文件 | 直接缺口 | 哪部分能独立推进 |
|---|---|---|---|
| T0 完整 tick 的边界 | Stage82 报告定义 tid1 在 4ac338 调用 4aeeb0，返回 4ac33d；较晚报告给出调度入口 4ac2b0。当前 native_driver.c 用 S82_ENTRY/S82_RETURN 与完成时栈位置约束一个切片 | 这只是 phase8 捕获/返回合同；没有完整 tick 的全部阶段、对象、完成条件和外层继续调度已通过证明。不能把一切片返回或一个月推进称为完整原 tick | 先做入口/状态/返回/调用者依赖表；完整 tick 的原行为边界和参考必须等待相应原证据 |
| T1 合格自然输入 | Stage83 batch A 在前缀像素循环触及限额，B/C 未启动；phase7-probe-v1 实现控制器，但 postflight 记新增原指令、reference、SR_game_calls 均为0 | 没有同一自然调度产生且合格的活动输入/入口，不能证明实际需调用哪些 API/对象。空活动记录不能推广为所有输入恒空，不能按猜测造对象或改 PC 到入口 | 可核对 qualified/diagnostic 的字段及消费者兼容；取得真实活动输入、执行和对照不在本轮范围，被拒路线不重启 |
| T2 入口/返回/目标绑定与消费者 | 当前仅编译凭据明确 _s82_entry、_s82_completion、stage72_return_bindings/count、stage77_structural_code_bindings 仍未解析；原 domains.c 在 import-manifest 的 held 项，本机有旧产物但未作为当前构建输入。App build.sh 未链接此链 | 新仓库不能靠补 include 自动变成原 tick 消费者；缺来源/版本/ABI一致的绑定和真实调用者。原绑定未提供、只编译对象未链接；旧203函数驱动已链接但原函数调用为0 | 自有适配器的提供者清单、版本/布局检查、所有权与明确失败接口可以开发；不能写空目标表或成功 stub 来“闭合”原绑定 |
| T3 未来动态回调 | phase7-v2 报告的 worker4 调用点 5fc139：ECX 来自 ESI 对象，传入整数1，返回现场 5fc13b；当前队列对象槽为空，未来对象/注册来源/表目标不明。Stage82 另列20处未知非IAT调用 | callee 的栈清理、flags/x87、所有注册对象类型及释放后调用禁用没有证明。表里有地址不等于对该 receiver 的调用获证；初始NULL不能当永久NULL | 可以设计带来源/代际/生存约束、未知即失败的 resolver 接口；目标身份、工厂/注册/释放闭包和原调用约定必须等待对应原行为证据 |

这些项按依赖关系排列，不声称它们是原执行时的固定先后顺序；原参考未走到合格入口，实际第一处动态阻塞仍未知。

## 已知链与当前接口，不能把 phase8 当完整 tick

历史 Stage82 的 A/B/C 是：A 从原检查点自然推进并停在所定义入口之前；B 使用冻结 A 输入执行所选 phase8 到原返回；C 使用同一输入做原生对照。该定义本身不证明三者成功。batch 已发生的是 A 失败，B/C 未启动。较晚控制器允许自然调度 tid1/2/4，检查 Watch 实际7→8、调用/返回栈及无 wrap，但新增原执行为0；其 qualified/diagnostic 格式到旧 B/C 的完整适配仍未验收。以上是历史依赖说明，不是运行建议或本轮命令。

现有自写 C 驱动源码的接线顺序可直接核对：

1. native_driver.c 检查已有执行门、输入及 limits，调用 s79_memory_open 建立私有区域；调用 s80_import_threads 导入四线程初始记录。
2. s80_connect 连接权限/写记录；s81_import_heap、s82_import_locks 和 s82_import_api_sites 导入 heap、锁与命名API站点；s80_set_bindings 接收返回/目标表。
3. 校验 S82_ENTRY、真实栈返回 S82_RETURN，设置 complete_pc、complete_sp、complete_target、allowed PCs，激活 runtime 后调用尚需绑定的 s82_entry。
4. step/guard/write hook、返回 resolver、平台 dispatch 参与该有限切片；s82_complete 检查 pending write 和 complete_sp 后 longjmp 停止。
5. 输出诊断结果后关闭映射、释放对象并退出。这是一次性切片驱动，没有已证明的外层原城市连续调度消费者。App timer 仍调用自写 CitySimulation，不能补上该缺口。

源码：[native_driver.c](../legacy/src/stage82/tools/native_driver.c)、[runtime.h](../legacy/src/stage82/tools/runtime.h)、[runtime.c](../legacy/src/stage82/tools/runtime.c)。只编译的未解析符号证据见 [AUTHORED_DRIVER_COMPILE](AUTHORED_DRIVER_COMPILE.md) 和 [凭据](authored-driver-compile-receipt.json)。本轮未运行这些接口，未读原绑定或生成游戏代码。

## 选定集合中尚缺的20项平台API

Stage82 的 blockers-verified.json 原有23项清单，其中 InitializeCriticalSection、EnterCriticalSection、LeaveCriticalSection 只在自造的单线程/锁owner条件下通过。剩下20项如下。清单适用于选定203函数集合，不是全游戏API总表；是否在合格自然输入上经过仍为 UNOBSERVED。

| API | 缺少的合同 | 当前提供者/证据与推进边界 |
|---|---|---|
| CompareStringA、CompareStringW、GetCPInfo、GetStringTypeA、GetStringTypeW、MultiByteToWideChar、WideCharToMultiByte、SetEnvironmentVariableA、GetTimeZoneInformation（9） | 原locale/码页/flags、长度与缓冲、转换失败及LastError、环境/时区来源；不能默认等同宿主Foundation行为 | 旧清单分类 locale_time_environment，均BOUNDARY；platform_native.c 的未实现入口明确停止。能先写有来源的参数/缓冲和错误接口，具体原路径值/副作用必须对照 |
| LoadLibraryA、GetProcAddress（2） | 模块/导出到受控guest调用者的身份、句柄生存、失败与回调调用约定 | 分类 loader_and_dynamic_User32；另有 GetActiveWindow、MessageBoxA、GetLastActivePopup 三处动态结果未获证。不能转成任意宿主函数指针、返回假窗口或静默成功；本轮不操作UI |
| VirtualAlloc、VirtualFree（2） | guest地址/大小/flags、reserve/commit/decommit、权限和释放生命周期 | 分类 CRT_virtual_memory。现有 guest_memory 可验证有限映射/权限；vector/bump模型不能直接替代完整Win32语义。可开发明确子集的私有映射适配，原调用域及行为仍须对照 |
| GetModuleFileNameA、GetStdHandle、WriteFile（3） | guest模块路径/编码/缓冲、句柄归属与生命周期、写入结果/错误/实际副作用 | 分类 CRT_files_and_process_metadata。宿主进程路径或stdout不能随意当作原结果；可先定义受控提供者，不能把未定义句柄写成成功 |
| GetCurrentProcess、RaiseException、ExitProcess、TerminateProcess（4） | 进程身份、异常dispatch/unwind、回调/对象清理、退出结果与错误态 | 分类 SEH_fatal_termination，SEH_dispatch_unwind=UNIMPLEMENTED_FAIL。受控停止是失败边界，不是这些API已实现；不真实终止其他进程，不伪造 RaiseException 成功返回 |

当前 C 的 [platform_native.c](../legacy/src/stage82/tools/platform_native.c) 只 dispatch id1..15：HeapAlloc/ReAlloc/Free/Size，thread ID/LastError/TLS，三项有限临界区和 memcpy/memmove/memset。命名站点 guard 核对当前slot/thunk及寄存器来源，不能证明未列出的 API 或动态域。s82_unimplemented 调用 s82_stop(500,id)，保留明确失败。缺实现的项必须有真实契约和调用者后才接入，不能只改返回值。

## callback、heap、SEH及调度的精确缺口

| 契约 | 已有接口及源码事实 | 仍需要什么 |
|---|---|---|
| 动态目标域 | runtime.c 的 stage72_lookup_return 只解已给定返回表；s80_bound_target 只检查表成员，源码明确“table membership alone cannot authorize a target”；stage77_lookup_target 的条件域另需提供者，原 domains.c 未复制 | 全部receiver/不可变表及每次目标、工厂/注册/释放来源、调用ABI和异常行为。旧20站点分11 virtual、6 CRT callback、3 User32动态结果；不能据此推定覆盖前缀 worker 回调或全游戏所有调用 |
| heap导入与所有权 | heap_connection.c 的 s81_import_heap 接收 S77H v1，heap handles/options、区域、live allocations、next/mapped；S77Memory check/map/fill/copy 接到 guest权限与trace | 原状态需和对象图/引用/所有权一致，不把宿主地址当guest地址。当前legacy模型flags有限、固定bump策略；HeapFree只退休live记录，不收回bump字节，HeapDestroy明确 S77_BOUNDARY |
| realloc与写预算 | legacy heap_native.c 先 s77_alloc（映射/填充并提交新记录），再复制保留前缀，最终退役旧记录；heap_connection.c fill/copy 各自 reserve | 若采用新版控制器报告的“整API提交前预留”合同，C侧也需填充+保留复制的总预算计划；当前分段检查不能从源码推导整调用失败原子性。失败可poison，不假称旧字节/元数据已回滚；本轮没有执行反例或改归档源码 |
| heap恢复输出 | native_driver.c 输出 heap-state.bin 的next/mapped/live/count和live三字段行；s81_import_heap要求带S77H/version及handles/options/regions的另一格式 | 诊断输出不能直接当可重新导入的heap快照。要恢复续跑必须明确版本、完整ownership/regions与提交语义，不能仅靠live count一致 |
| CPU/context/调度 | runtime.c 的 s80_import_threads 恢复四条初始 S80T 记录；s80_save_core 更新public core，明确opaque为immutable initial sidecar。thread_native.c 仅选择已runnable且public/cache已知的记录；switch更新FS/TEB | 更新后的完整CPU/架构状态、真实wake/wait/事件/时钟/锁及可变HostStore同步。初始roundtrip不证明运行后冷恢复。旧选定集合无x87/GS变化的声明不适用于更大完整tick；GS_BASE旧报告为unknown |
| 锁与SEH | platform_native.c 他线程持锁时停止，不进行阻塞/唤醒/调度；thread_contract.py 捕获 RaiseException 参数后产生 UNIMPLEMENTED_SEH_DISPATCH_STOP，没有handler/unwind/栈清理 | 跨线程锁语义、异常注册链/handler/展开/继续/退出与回调顺序，不能以有限FS访问或longjmp代替完整SEH |
| 活状态、停止与消费者 | native_driver.c completion后输出current core/TLS/locks/heap/memory诊断并退出；App不链接该runtime | 不仅是链接：还需稳定活状态owner、完整tick调用/完成及外层调度接口、错误退出/释放和下一tick。不能通过加入未使用库或再造城市模型宣称接入 |

对应源码：[heap_native.c/h](../legacy/src/stage77/tools/heap_native.c)、[heap_connection.c](../legacy/src/stage82/tools/heap_connection.c)、[thread_native.c/h](../legacy/src/stage79/tools/thread_native.c)、[thread_contract.py](../legacy/src/stage79/tools/thread_contract.py)。runtime_core 的有限heap退役和Contexts是另一套自写合同，没有原消费者；不能替换上述原桥结论。归档保持 inert，来源见 [legacy/INDEX](../legacy/INDEX.md) 与 import-manifest.json。

## 可开发与必须等待对照的分界

并非所有源码工作都依赖原执行。已有接口足以支持三个直接服务接线的工作：显式provider/ABI与状态所有权清单；整API预算/权限的提交前计划及失败诊断；有版本的qualified/diagnostic和完整状态导入导出接口。完成标准分别是依赖/消费者无歧义、声明失败路径不假成功、恢复格式包含声明状态且诊断不能冒充合格输入。它们只能记为适配器合同，不记为原tick通过。

原完整tick边界、自然活动输入、动态对象注册与callee ABI、20项API在真实调用域的返回/副作用、完整线程/异常/heap及恢复续跑等价仍需要对应原行为证据。当前缺口不是只补一个函数或更换启动参数；本轮没有能直接据证宣布“完整tick已打通”的源码修正。因此只提出上述有明确接口来源的修正方向，不改历史源指纹，不制造微型测试、玩具模型或成功stub作为进展，也不重启被拒执行路线。

## AX实际检测与影响

本轮仅读取以下既有动作记录，没有重新调用窗口助手：

| UTC时间及动作 | 实际结果 | 可以推断的范围 |
|---|---|---|
| 2026-10-11 08:29:45.906386，action-021 setName | exit1，stderr为 save panel required | 保存面板条件未满足；不能据此认定是AX权限导致，也不能记为保存通过 |
| 08:34:33.270292，action-022 windows | exit1，stderr为 existing AX permission unavailable; no request made | 当次助手的AXIsProcessTrusted()守卫失败 |
| 08:34:33.976883，action-023 inspect viewportRecoveryState | exit1，同上 | 第二次只读检查在同一守卫停止，未产生窗口快照 |

窗口助手 permissions() 在第39行先执行 AXIsProcessTrusted()；inspect/windowMetadata 在访问自有App或AX窗口之前调用它。记录绑定driver SHA256 23456ee3a2d682e8a32bcfbdecc0b1e831372d5eec1cfb838e797cbf64ff20f5、源码SHA256 80be8924ac53ec29cad9f2feb8187cad0699eda6f0373bfa39f6cd09019f8890。未请求权限或修改系统设置。

其影响是候选剩余窗口/保存/拖动/resize验收与正常退出未闭合。不能推断谁撤销权限、当前系统全部GUI不可用、屏幕/事件权限也失败，或候选当前仍在运行；不存在本轮新的AX检测或恢复尝试。本次源码/报告审计不需要AX权限，也不通过其他接口绕过该守卫。

## 本地证据与首个关口

历史文本别名 LOCAL_LEGACY_TASK：stage82/reports/README.txt，stage82/evidence/blockers-verified.json，stage83/batch-v1/reports/README-final.txt，stage83/phase7-v2/reports/README.txt，stage83/phase7-probe-v1/reports/README.txt及evidence/postflight.json。当前源码及历史文本SHA、筛选后的依赖/API/调用元数据、AX status/stdout/stderr的SHA封存在任务目录 original-tick-wiring-audit-20261011/evidence-manifest.json；不上传原始字节、绑定表、快照、日志、资源或二进制。

当前已完成的关口是对上述已知入口和桥接代码的静态审计：把phase8与完整tick分开，把缺提供者与缺原对照分开，把AX守卫与历史原执行拒绝分开。下一个真正的行为关口仍须合格自然输入完成明确界定的原完整tick，实际状态/对象/有序写/事件及受控退出有对应证据；本轮未执行、未通过，不给完成百分比。
