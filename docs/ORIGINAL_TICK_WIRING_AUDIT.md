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

## Original city update tick (exe map)

### 口径与证据边界

本节只描述地址、调用顺序、全局与被调者，不粘贴反汇编或反编译正文。主体仍是 SAV.md 记录的同一份自有 EXE，SHA-256 `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`，image base `0x400000`，代码 VA 对应文件偏移 VA−`0x400000`（[SAV.md](SAV.md) 第794行）。分析用 listing 只留在外部任务目录，没有进入仓库。

- 调用面：父入口 `0x5371A0` 的 8 个直接项（7 个 CALL 加 1 个尾跳），见 Table A；每个直接项再展开一层，见 Table B。Table B 共 81 行（按 caller-callee 对计；`0x554C00`、`0x47F1B0` 等在不同 caller 下重复出现），去重后二级地址 63 个；连同 8 个直接项，全表 89 行、71 个不同地址。
- “全局”只指函数体内以绝对地址出现的 `dword/word/byte ptr [imm]` 操作数，不跟随被调者，也不含 `[ecx+disp]`、`[eax+disp]` 这类对象字段。因此某行的“全局”列是下界，不是该子树访问的全集。
- 本 build 几乎没有可用 ASCII 字符串，无法用字符串引用命名函数。身份判断只来自 caller/返回点、被调者集合、全局与相邻常量；“unclear”表示这些结构证据不足以确定业务子系统，不表示该阶段无作用。
- 状态只有四类：`ported` 必须能在 [SAV.md](SAV.md) 或 [SAV_FIGURES.md](SAV_FIGURES.md) 中找到该地址作为“已实现/已计算”的证据（行号见“依据”）；`partial` 表示该行为只作为已移植 caller 的一部分存在，或只有合同/命名 blocker、尚未形成可运行实现；`not ported` 表示无实现；`empty` 表示该 phase 在跳表中直接落到 `0x4AC4C8` 公共推进，没有本地 handler。
- SAV19 恢复状态来自 `0x5311DC..0x531873` 内“`push imm` 紧接 `call 0x780533`”的 89 个请求点。该窗口不是唯一读取点（例如版本化加载合同记录的 `0x5306BA→0xC82EEC`），所以“未 restored”只表示“不在本窗口”，不能说该字段从不被加载。反向也一样：本图只有 7 个全局同时出现在这 89 个请求点里，其余 82 个的消费者在更深的子树或别的代码路径中，不能据此判定它们未被使用。

### Table A — `0x5371A0` 的 8 个直接项（调用顺序）

| # | 地址 | 顺序/条件 | 一句话描述（子系统） | 依据 | 状态 | 直接全局（SAV19 恢复） |
|---:|---|---|---|---|---|---|
| A1 | `0x4189B0` | 首调 `0x5371A0` | 存档 PRNG：先把上一 7-bit 值写入 100 项环 `0x10C6F90[cursor]`，前移/回绕游标 `0x10C6F8C`，再把两个 31-bit 寄存器各推进 31 次（engine RNG） | leaf；`0x5371A0` 直接返回 `0x5371A5`；[SAV_FIGURES.md](SAV_FIGURES.md):766、788 | partial：寄存器已移植，环与游标未移植 | `0x10C7148`、`0x10C714C`；两者均 restored |
| A2 | `0x529990` | 第二 | 十枚 modulo 计数器 `0x10C71D4..0x10C71F8`，周期依次为 4/8/16/24/32/64/128/256/384/768；逐个 +1，到周期归零（calendar/periodic clocks） | leaf；[SAV.md](SAV.md):798 称 modulo clocks；[SAV_FIGURES.md](SAV_FIGURES.md):788 记录 `0x529A81` 复位 | not ported | `0x10C71D4..0x10C71F8`；均未 restored |
| A3 | `0x5EF190` | 第三；返回值为下一项 ECX | 返回单例 `0x1E53538`（16 字节 leaf）（event/UI object root） | leaf；`0x5371AA` 把 EAX 送入 ECX | not ported | 无 |
| A4 | `0x5EFF20` | 第四 `0x5371B1` | 单例对象 tick：字节 +17/+16 双门，`+24` 倒计时递减，类型 `+20` 过滤 23/22/129/123/202/113/115/111，registry 扫描，最后可把暂停字节 `0x10DE036` 置 1（events/UI pause） | 调用点 `0x5371B1`；9 个直接被调者见 Table B1 | not ported | `0x10DE036`；未 restored |
| A5 | `0x4AC2B0` | `0x5371BF`，仅 `[0x88EC38]==0` | 城阶段调度：三段 precheck（`0x408AC0→0x4FAE50`、`0x55BCB0→0x55B540`、`0x55CE80→0x55CEE0`）；三项依次满足（非零/真）时在 `0x4AC4E7` 直接 RET，任一不满足则进入分派。读 `0xC82EF8`，1..48 查 `0x4AC4E8` 跳表，每次只执行一个阶段；第 51 次归零并尾跳 `0x4AC650`（calendar/scheduler） | `0x5371B6` gate；[ORIGINAL_SCHEDULER_CONTRACT.md](ORIGINAL_SCHEDULER_CONTRACT.md) 第15/19-75行；[SAV.md](SAV.md):798-799 | not ported | `0xC82EF8` restored；`0x13F8E78`、`0x1312564`、`0x1312870`、`0x130F967` 未 restored |
| A6 | `0x466E90` | `0x5371C9`，ECX=`0xA98370` | 场景事件槽：先 `0x492430(0xC75090)`，清 `+259952`，对 8 个槽 `[ECX+259920..+25993C]` 逐个 `0x468030(0)` 清标志，未完成者跑 `0x467680`，任一进展置 `+259952`（scenario/events） | `0x5371C4` 装 ECX；8 次循环 | not ported | `0xA98370` 是对象基址，不是全局数组；无 restored 全局 |
| A7 | `0x4E27E0` | `0x5371CE`，figure pass | Figure 池调度：升序 slot 1..1999，处理 reciprocal `+92/+90`、age `+114`、leader `+66`、post-return `+190`、cell list、cargo、destination RNG/route-owner；virtual+28 在 `0x4E2909`（figures） | [SAV_FIGURES.md](SAV_FIGURES.md):688、768 | ported（bounded ordered implementation） | `0x10AEEE0`、`0x10AEEE4`；均未 restored |
| A8 | `0x564B50` | `0x5371DC`，重新读 `[0x88EC38]==0` 时尾跳 | 周期 wildlife 扫描：`0x12A6E78` 自增并与 `0x859DA0` 比较；到期后 registry 扫描 type 76..268、live 1/3，跑 `0x5653C0/0x56C880/0x56D170/0x564EA0/0x567610`，写 `0x10DE0D0/0x10DE045`（wildlife/figures periodic） | `0x5371D3` gate；9 个直接被调者见 Table B5 | not ported | `0x12A6E78`、`0x859DA0`、`0x10DE0D0`、`0x10DE045`；均未 restored |

边界：`0x4AC650`、`0x42DA70` 等第三层及以下的被调者不在本节展开；`0x4AC650` 的完整依赖仍以调度合同为准。

### Table B — 一层展开（81 行）

#### B1. `0x5EFF20` 的 9 个直接被调者

| 地址 | 一句话描述 | 依据/调用点 | 状态 | 直接全局（SAV19） |
|---|---|---|---|---|
| `0x53C720` | getter `byte[ecx+17]`（event 就绪/停止门） | `0x5EFF25` | not ported | 无 |
| `0x53C710` | getter `byte[ecx+16]`（第二道门） | `0x5EFF34` | not ported | 无 |
| `0x5F0250` | getter `byte[ecx+40220]`，ECX=`0xC75090` | `0x5EFF51` | not ported | 无 |
| `0x4F8210` | vector begin：返回 `[ecx+4]` | `0x5EFFD9`、`0x5F0013` | partial：作为已移植遍历的 begin 存在，无独立地址证据 | 无 |
| `0x4F8200` | vector end：返回 `[ecx+8]` | `0x5EFFE2`、`0x5F001C`、`0x5F007E` | partial：同上 | 无 |
| `0x47F1B0` | registry slot：`begin+4*id`，RET4 | `0x5EFFF5`、`0x5F002F`；[SAV.md](SAV.md):625 用于已移植 wall 邻接解析 | ported | 无 |
| `0x408E20` | setter `byte[ecx+17]=0` | `0x5EFFC1`、`0x5F008D` | not ported | 无 |
| `0x5F00B0` | 带 bool 参数的递归/遍历清扫 | `0x5F009D`（push 1） | not ported | 无 |
| `0x554C00` | count：`([ecx+8]-[ecx+4])/4`，空指针返回 0 | `0x5EFFB6` | partial：作为已移植循环长度存在 | 无 |

#### B2. `0x4AC2B0` 的 48 个阶段行（跳表 `0x4AC4E8`，phase 1..48）

| phase | 目标 | 一句话描述（子系统） | 依据 | 状态 | 直接全局（SAV19） |
|---:|---|---|---|---|---|
| 1 | `0x4F1560` | 经济/会计：调 `0x4F1590/0x4F1A70/0x4F1A80/0x4F1A90/0x5919D0/0x4AF2D0/0x4AEDF0/0x54CC60`，尾跳 `0x4AFFA0→0x44BAB0` | `0x4AC2FA`；[SAV.md](SAV.md):827 命名 `city_accounting_4f1560_4ac2fa` 为 blocker | not ported | 无 |
| 2 | `0x4AC4C8` | 无本地 handler，直接公共推进 | `0x4AC4C8` | empty | — |
| 3 | `store32(0x13F8E78)=1` | 未明示语义的刷新/脏标志写入 | `0x4AC42D` | not ported | `0x13F8E78`；未 restored |
| 4 | `0x4819F0` | 城市统计：`0x591AA0(0)`（ECX=`0x130F960`）返回 ≥0 时清 `0x131262C/0x1312630` | `0x4AC304` | not ported | `0x131262C`、`0x1312630`；均未 restored |
| 5 | `0x54C6F0(0)` | unclear：0x54 子系统模式 0 链 `0x54C730→0x54CC60→0x54D450→0x54C8D0→[0x54C970]→0x54D3A0→0x54EF10→0x5206F0` | `0x4AC30E`；push 0 | not ported | 无 |
| 6 | `0x5185C0` | 住房：经 getter `0x10BFEF0`，住宅倒计时/劳动字节，virtual +B8/+1E4 | `0x4AC31D`；[SAV.md](SAV.md):804 仅描述 | not ported | 无 |
| 7 | `0x4AF350` | 道路/路线：清 `0xC81B70..`、`0x101D0C0..`，调 `0x4AF310/0x4AF490`；`0x534EBA→0x4AF350/0x4AF490` 重建 `EC5A10` | `0x4AC32E`；[SAV_FIGURES.md](SAV_FIGURES.md):44 | partial：路线重建存在，完整阶段未移植 | `0xC81B70/74/78`、`0x101D0C0/C4/C8/CC`、`0x101C560`；均未 restored |
| 8 | `0x4AEEB0` | 建筑池消费：`0x506240`(C=`0x10BC910`)、`0x413A50(1)`、`0x4F8200`、`0x47F1B0`、`0x426D10`、`0x5DB530`×2；清 `0xC82FFC/0xC82F6C/0xC830C8` | `0x4AC338`；[REA_ABI_CONTRACTS.md](REA_ABI_CONTRACTS.md):11、276 | partial：ABI/过滤已合同化，未形成实现 | `0xC830C8`；未 restored |
| 9 | `0x40FC20` | 已静态核实为 `RET` 的空 handler | `0x4AC342`；合同第77行 | not ported（no-op） | 无 |
| 10 | `0x4AEB30` | unclear：清 `0xC82EE0`；registry 遍历（`0x4F8210/0x4F8200`），alive `[+4]`，读 `[obj+180]`，`0x506240`、`0x554C00` | `0x4AC34C` | not ported | `0xC82EE0`、`0xC82ED8`、`0xC82EDC`；均未 restored |
| 11 | `0x590A50` | 人口峰值：`max(0x130F988,0x131257C)` 并存回 | `0x4AC356`；[SAV_FIGURES.md](SAV_FIGURES.md):985，997→1007 已断言 | ported | `0x130F988`、`0x131257C`；均未 restored |
| 12 | `0x4ACE60` | unclear：类型过滤的建筑遍历；`0x413B40(1)`、live `0x426D10`、类型谓词 `0x562F70`、`0x567540/0x42B720/0x42B730` | `0x4AC360` | not ported | 无 |
| 13 | `0x591EC0→0x5925A0` | 城市统计重置/汇总：`0x1312564==0` 时写 `0x1312870/0x130F967=ff`，`0x591EC0`(ECX=`0x130F960`) 清 `+11816` 区块，`0x5925A0` 恒调 | `0x4AC36A`；合同第77行 | not ported | `0xC82F68`；未 restored |
| 14 | `0x4AC4C8` | 无本地 handler | `0x4AC4C8` | empty | — |
| 15 | `0x4AC4C8` | 无本地 handler | `0x4AC4C8` | empty | — |
| 16 | `0x5D3CB0` | unclear：清 `0x13123D0..0x1312444`（stride 4，字段 +0/+652/+796），再 registry 遍历 `0x506240/0x413A50/0x554C00/0x47F1B0/0x426D10/0x5D61C0/0x418D60` | `0x4AC398` | not ported | 无（基址为立即数） |
| 17 | `0x4AEDF0` | unclear：registry 遍历 `0x413B40(1)/0x554C00/0x426D10`，调 `0x591490/0x40E680` | `0x4AC3A2` | not ported | `0xC5CDA0`、`0x1312528`、`0x131205C`、`0x1312128`；均未 restored |
| 18 | `0x5A8170` | unclear（owner 向量通行）：`0x426A60` 返回 `0x1312F08`，caller 预压 0，`0x5A8170` 消费 | `0x4AC3AC`；合同第77行 | not ported | 无 |
| 19 | `0x4AC4C8` | 无本地 handler | `0x4AC4C8` | empty | — |
| 20 | `0x51E4A0` | 生产/作物：`0x4C1A90` prepass、registry walk、`0x426D10` live、`+47`、virtual+C8(−4)、`0x4C0F60` 作物、`0x4AE560`、+208/+74/+204；随后 `0x4BC440` 地形资源生长 | `0x4AC3BF`；[SAV.md](SAV.md):800、827 | partial：作物子域已算，production virtual 仍拒 | 无 |
| 21 | `0x4ACFC0` | 道路刷新：清 runtime `+2A/+2C`，逐 live building `+84`；crop/wall `0x51CE50→0x426DF0→0x4BAF40` 重写 road distance/+24；用 `0x1391FE0` 与 ranked networks | `0x4AC3CE`；[SAV.md](SAV.md):801、827 | partial：命名 blocker，未形成实现 | `0xC5CDFC/FE`、`0xC5CE00/02`、`0x1192A2C`、`0xD62400`、`0x101D0C8`；均未 restored |
| 22 | `0x4AD3D0` | 住房自由空间/模型容量：`0x590DD0`、`0x40FC20`、owner 向量遍历、`0x44CC80(model,17)` | `0x4AC3D8`；[SAV.md](SAV.md):804 | not ported | `0x130F994`、`0x130F998`；均未 restored |
| 23 | `0x4AD4A0` | 劳动/产业分配：`0xD62418-=51`（clamp 0），`0x5917E0`、`0x4ADA10`、`0x4ADC90`、`0x590E00`、`0x590EC0`、`0x4AD850`、`0x4138F0`、`0x546EA0`；唯一读 `0x1192B04` 区块的阶段 | `0x4AC3E2` | not ported | `0x1192B04` restored；`0x1192A2C`、`0x1192B05..B0D`、`0xD62418`、`0x130F988`、`0x1311F80..B8` 未 restored |
| 24 | `0x4AE1A0` | 住房迁出：owner 向量遍历 + `0x4AE150` | `0x4AC3EC`；[SAV.md](SAV.md):804 | not ported | 无 |
| 25 | `0x4AE220→0x4AE7F0→0x4F19A0` | 就业：清 `0x1312138..0x1312208` 数组（stride 20）、registry walk；`0x4F19A0` 以类别总量/余数两次派发 `+18C=0x51E310`，写 workers`+44` | `0x4AC3F6`；[SAV.md](SAV.md):802 明写 “Not yet ported” | not ported | 无 |
| 26 | `0x4AC4C8` | 无本地 handler | `0x4AC4C8` | empty | — |
| 27 | `0x5253A0` | unclear：`0x525540`、`0x524310`×2、owner 向量遍历、每槽 `0x525470`×4 与 `0x524310` | `0x4AC40A` | not ported | `0x101C504`；未 restored |
| 28 | `0x4ACE30` | 水/服务：清 `0xC82F14`，`0x5177B0` 后尾跳 `0x5179B0` 派生 water state | `0x4AC414`；[SAV.md](SAV.md):804 | not ported | `0xC82F14`；未 restored |
| 29 | `0x54C6F0(1)` | 同 phase5 的模式 1 链 | `0x4AC41E`；push 1 | not ported | 无 |
| 30 | `store32(0x13F8E78)=1` | 同 phase3 | `0x4AC42D` | not ported | `0x13F8E78`；未 restored |
| 31 | `0x416D20` | 建筑钩子：`timeGetTime` profiling，升序 live building virtual+20；crop bit80 经 `0x4C1040→0x4C12E0`；well/kiln 仍拒 | `0x4AC43C`；[SAV.md](SAV.md):803 | partial：作物子域已算，Well/Kiln 拒绝 | `0x1032658` restored；`0xC82EE0` 未 restored；IAT `0x7A9534/0x847398/0x88F720` 不是 SAV 状态 |
| 32 | `0x5DBCC0→0x4B02A0` | unclear：清 `0x13124B2..B8` 四字，调 `0x480740/0x481F10/0x441690/0x443E20/0x441620/0x443E50/0x441590`，随后 `0x4B02A0` | `0x4AC446` | not ported | `0x13124B2/B4/B6/B8`、`0xA5B044`、`0xC5CE24/26`；均未 restored |
| 33 | `0x4AF230` | 每类型标志复位：清 `0xC82DB0/0xC82E40` 各 30 dword、`0xA5AF64` 起 269 字节、`0x131249C..` 计数；`0x413B40(1)`、`0x4F8200` | `0x4AC452` | not ported | `0x131249C/A0/A4/A8/AC/BE`；均未 restored |
| 34 | `0x4AC4C8` | 无本地 handler | `0x4AC4C8` | empty | — |
| 35 | `0x517B40→0x517280` | 住房服务衰减：12 个 service 字节（runtime `+2A..+2E`、`+32..+34` 及序列化省略的 padding）饱和到 0；virtual +B8/+1E4 | `0x4AC45E`；[SAV.md](SAV.md):804、817 明写 “this subpass is ported” | ported | 无 |
| 36 | `0x517AD0→0x518D60` | 服务聚合：`+218` 派生 aggregates | `0x4AC46C`；[SAV.md](SAV.md):804 | not ported | 无 |
| 37 | `0x53C870→0x44F1D0` | unclear（容器/异常路径）：`0x53C870` 返回 `0x115F6EC`；`0x44F1D0` 用 `0x413B40(0x8C7630)`、`0x413B70/0x44CC50/0x44EB20/0x44EA70/0x44ECD0`、virtual +168/+16C/+170 | `0x4AC47A`；ORIGINAL_PHASE37 合同 | not ported | 无 |
| 38 | `0x4ACD10` | unclear：registry 遍历 `0x413B40(1)/0x554C00/0x426D10`，逐 live slot `0x53C870→0x44F180` | `0x4AC488` | not ported | 无 |
| 39 | `0x519120` | 住房演化：`0x4ACE20` prepass、清 `0xC82CF4`、owner 向量遍历、`0x5AD8F0`；`+200=0x518EF0` 演化/模型/所有权 | `0x4AC48F`；[SAV.md](SAV.md):804 | not ported | `0xC82CF4`；未 restored |
| 40 | `0x4ACC40` | unclear：registry 遍历；`0x4BCE40`、`0x4B67B0`、`0x5AD8F0/0x5AD960`、`0x5EF190`、`0x4DF630`、`0x5EF560` | `0x4AC496` | not ported | `0x101D0FC`；未 restored |
| 41 | `0x4AC4C8` | 无本地 handler | `0x4AC4C8` | empty | — |
| 42 | `0x4AC4C8` | 无本地 handler | `0x4AC4C8` | empty | — |
| 43 | `0x4AF9B0` | 火灾/蔓延：清 `0xC82F20/24/28/2C`、`0x413B40(1)`、`0x4F8200`、`0x426D10`、`0x5418D0`、`0x42B6C0`、`0x4189A0`、`0x47F1B0`、`0x4AF970` | `0x4AC49D`；[SAV.md](SAV.md):805 | not ported | `0xC82F24`、`0xC82F28` restored；`0xC82F20/2C`、`0xC5CD58` 未 restored |
| 44 | `0x4AF5D0` | 风险/火灾：ECX=`0x8C7630` 尾跳 `0x42D9A0`；`d62408=1` 可跳过；IDIV `0x817748`；owner+4→`0x4F8210/0x4F8200`；slot virtual+30；`0x5AD8F0/0x5AD960→0x522810` | `0x4AC4A4`；[SAV.md](SAV.md):805 | not ported | 无 |
| 45 | `0x5F02C0→0x4B94A0` | 纪念物/城市对象：ECX=`0x1E535F8`；`0x517CC0<350` 返回；owner 向量；`0x426A60/0x5A8420(5)`；`0x4189A0`；`0x5F03F0`；随后 `0x4B94A0` | `0x4AC4AB`；合同第77/121行 | not ported | `0x875C88`；未 restored |
| 46 | `0x4AC4C8` | 无本地 handler | `0x4AC4C8` | empty | — |
| 47 | `0x4AFD20` | 计数复位：清 `0xC82D48/44/34/30` 后 RET | `0x4AC4BC`；合同第77行 | not ported | `0xC82D30/34/44/48`；均未 restored |
| 48 | `0x4AE8B0` | 建筑倒计时：registry 遍历 `0x413B40(1)/0x4F8200`；live1 时 runtime`+52` 减一 | `0x4AC4C3`；[SAV.md](SAV.md):805 | not ported | 无 |

#### B3. `0x466E90` 的 4 个直接被调者

| 地址 | 一句话描述 | 依据/调用点 | 状态 | 直接全局（SAV19） |
|---|---|---|---|---|
| `0x492430` | 场景/事件管理步，ECX=`0xC75090`；`0x494250`、`0x425260/0x425250`×2、`0x4ED3B0`；读 `0x88EC38` | `0x466E9B` | not ported | `0x88EC38`；未 restored（邻接的 `0x88EC3C` 是 restored，不能替代） |
| `0x468030` | setter `byte[ecx+56]=arg` | `0x466EBC`、`0x466EF4` | not ported | 无 |
| `0x468040` | getter `byte[ecx+56]` | `0x466ED2` | not ported | 无 |
| `0x467680` | 场景槽步：`0x467660`、`0x549E30`、virtual+64、`byte+47`、`0x4B3BD0/0x4B3DA0`、`0x467F60/70/80/90` | `0x466EDD` | not ported | 无 |

#### B4. `0x4E27E0` 的 11 个直接被调者

| 地址 | 一句话描述 | 依据/调用点 | 状态 | 直接全局（SAV19） |
|---|---|---|---|---|
| `0x42B6B0` | 返回常量 `0x10BC950`（record-manager 邻接对象） | `0x4E27E9` | not ported | 无 |
| `0x506C70` | 工业缓存 `0x10BC928[36]` 九 dword 清零，仅 saved phase==2 | `0x4E27F0`；[SAV_FIGURES.md](SAV_FIGURES.md):159、323 | ported | `0xC82EF8` restored |
| `0x5D2B60` | 返回 storage vector 基址 `0x1E4EE30` | `0x4E27F5`；[SAV_FIGURES.md](SAV_FIGURES.md):158 | partial：加载路径已记录，vector/cache 由 caller 提供 | 无 |
| `0x5D2B70` | storage 缓存 `0x1E4EE08[36]` 九 dword 清零，仅 phase==2 | `0x4E27FC`；[SAV_FIGURES.md](SAV_FIGURES.md):158、323 | ported | `0xC82EF8` restored |
| `0x5E3530` | 清 `0x1E50CE8..F4` 四计数，遍历 `0x10315DC` 调 `0x4E5310`（combat 类别计数） | `0x4E2801`；[SAV_FIGURES.md](SAV_FIGURES.md):768 | not ported | `0x1E50CE8/EC/F0/F4`；均未 restored |
| `0x554C00` | registry count | `0x4E2815`、`0x4E2947` | partial：作为已移植循环长度存在 | 无 |
| `0x47F1B0` | registry slot(id) | `0x4E282F`、`0x4E2862`、`0x4E28E5` | ported | 无 |
| `0x4D6A60` | 识别 type19/21/22，供 cart family 选择 | `0x4E28C3`；[SAV_FIGURES.md](SAV_FIGURES.md):651 | partial：类型识别已用于已移植选择器 | 无 |
| `0x4E26C0` | profiling 计数：写 `0x10323A0/A4`、`0x10AEEF4`，读 IAT `0x7A917C/0x7A9120` | `0x4E28FD`；[SAV_FIGURES.md](SAV_FIGURES.md):768 | partial：instrumentation 未携带 | `0x10323A0/A4`、`0x10AEEF4` 未 restored；`0x7A917C/0x7A9120` 是 IAT |
| `0x4E26F0` | profiling 计数/耗时：同组加 `0x10323A8/B0/B8`、`0x1031938` | `0x4E2916`；同上 | partial | 同上加 `0x10323A8/B0/B8`、`0x1031938`；均未 restored |
| `0x4EA080` | 生命周期/移除：`0x4E1FA0`(`0x10315D8`)、`0x47F1B0`(`0x10315DC`)、`0x5C4EB0/0x5C4D00/0x5C4EE0` | `0x4E292D`；[SAV_FIGURES.md](SAV_FIGURES.md):768、989 | not ported：当前明确 withheld | 无 |

#### B5. `0x564B50` 的 9 个直接被调者

| 地址 | 一句话描述 | 依据/调用点 | 状态 | 直接全局（SAV19） |
|---|---|---|---|---|
| `0x554C00` | registry count | `0x564B77`、`0x564C1D` | partial | 无 |
| `0x47F1B0` | registry slot(id) | `0x564B8C` | ported | 无 |
| `0x562F70` | 类型谓词：`type-76` 落在 `[0,192]` 时以 `0x562F9C` 的 193 字节表取 0/1，再查 `0x562F94` 的两项跳表，返回 bool | `0x564B98` | not ported | 无 |
| `0x426D10` | live 谓词：`byte[ecx+4]∈{1,3}`，RET4 | `0x564BAF`；[SAV_FIGURES.md](SAV_FIGURES.md):786、875 | ported | 无 |
| `0x5653C0` | 按 id 扫描 `0x12A5DD0` 起的 144 字节记录数组，计数取 `0x12A6E7C` | `0x564BBB` | not ported | `0x12A6E7C`；未 restored |
| `0x56C880` | unclear：`0x56A910/0x564E80/0x56A940`、`0x47F1B0`、registry 向量遍历、`0x56BB40/0x563550`；读 IAT/vtable `0x7AC8C0` | `0x564BDB` | not ported | 无 SAV 全局；`0x7AC8C0` 是 IAT/vtable |
| `0x56D170` | unclear（SEH 帧）：`0x47F1B0`、`0x56CF40/0x554C00/0x4F8210/0x4F8200/0x56F9E0/0x56F9B0/0x56D0F0/0x4F7A10` | `0x564BE3` | not ported | 无 |
| `0x564EA0` | unclear：`0x480740/0x4812E0`、`0x47F1B0`、`0x564E90`、`0x567610`、`0x5635A0`、`0x7650FC`、`0x402A50`；vtable `0x7B8A80..` | `0x564C01` | not ported | 无 SAV 全局；`0x7B8A80/88/90/98`、`0x7ACA28`、`0x7AE110` 是 IAT/vtable |
| `0x567610` | 按 id 扫描 `0x12A5DD4` 起的 144 字节记录数组，计数 `0x12A6E7C` | `0x564C0B` | not ported | `0x12A6E7C`；未 restored |

### Table C — 本图触及的 SAV19 全局

`0x5311DC..0x531873` 窗口共恢复 89 个全局；本图 89 行的直接操作数只命中其中 7 个。其余 82 个虽然被加载恢复，但不在这 89 个函数体的直接操作数里（消费者在更深层或别的代码路径），不能据此说它们未使用。

| 全局 | 触及条目 | 在窗口内 restored? | 请求点 |
|---|---|---|---|
| `0xC82EF8` | `0x4AC2B0`（phase 计数）；`0x506C70`、`0x5D2B70`（phase==2 门） | 是 | `0x531427` |
| `0x10C7148` | `0x4189B0`（RNG 寄存器） | 是 | `0x53145F` |
| `0x10C714C` | `0x4189B0` | 是 | `0x531451` |
| `0x1192B04` | `0x4AD4A0`（劳动/产业块） | 是 | `0x53153A` |
| `0x1032658` | `0x416D20`（建筑钩子） | 是 | `0x53166F` |
| `0xC82F24` | `0x4AF9B0`（火灾/风险） | 是 | `0x531572` |
| `0xC82F28` | `0x4AF9B0` | 是 | `0x531564` |

未 restored 但会被未移植阶段读写的代表项：`0x13F8E78`（phase3/30）、`0x131262C/0x1312630`（phase4）、`0xC82F68`（phase13）、`0xC5CDA0`（phase17）、`0x130F994/0x130F998`（phase22）、`0x101C504`（phase27）、`0xC82F14`（phase28）、`0xC82CF4`（phase39）、`0x101D0FC`（phase40）、`0xC82F20/0xC82F2C`（phase43）、`0x875C88`（phase45）、`0xC82D30/34/44/48`（phase47）、`0x12A6E78/0x10DE0D0/0x10DE045`（`0x564B50`）、`0x10DE036`（`0x5EFF20`）、`0x88EC38`（`0x492430`）。这些是进程期/scratch 状态；加载一个已用过的进程时不能假定为零，原生实现必须显式提供初值或有条件复位。

### 计数与移植顺序

| 层 | 行数 | ported | partial | not ported | empty |
|---|---:|---:|---:|---:|---:|
| Table A 直接项 | 8 | 1 | 1 | 6 | 0 |
| B1 `0x5EFF20` | 9 | 1 | 3 | 5 | 0 |
| B2 phase 1..48 | 48 | 2 | 5 | 32 | 9 |
| B3 `0x466E90` | 4 | 0 | 0 | 4 | 0 |
| B4 `0x4E27E0` | 11 | 3 | 5 | 3 | 0 |
| B5 `0x564B50` | 9 | 2 | 1 | 6 | 0 |
| 合计 | 89 | 9 | 15 | 56 | 9 |

89 行/71 个不同地址（8 直接 + 63 个不同二级地址）；`ported` 9 行、`partial` 15 行、`not ported` 56 行、`empty` 9 行。

按依赖与收益排序的移植顺序（≤15 项）：

1. `0x4AC2B0` + `0x4AC4C8`/`0x4AC650`：phase 计数、48 项跳表、三段 precheck、51 次归零与周期尾。没有它就只能是单阶段切片，不能称为城市 tick。
2. `0x4189B0` 全量：两个寄存器加 100 项环 `0x10C6F90` 与游标 `0x10C6F8C`；所有依赖 RNG 的路径都需要同一写入顺序。
3. `0x529990`：十枚周期计数器；后续 periodic 分支共享这组时钟。
4. `0x5EF190` + `0x5EFF20`：单例事件倒计时与暂停字节 `0x10DE036`，是 `0x5371A0` 第四步的真实消费者。
5. `0x4E27E0` 收尾：`0x4EA080` 移除/生命周期、`0x4E26C0/0x4E26F0` 计数、`0x42B6B0/0x5E3530/0x5D2B60/0x4D6A60`，把 49/192 的 ordered 结果补完整。
6. `0x564B50` + `0x562F70/0x5653C0/0x56C880/0x56D170/0x564EA0/0x567610`：周期 wildlife 扫描。
7. `0x590A50` 的 phase11 接线：函数已移植，缺的是调度器中的一次调用与 `0x130F988` 输入。
8. 住房服务链 `0x517B40/0x517280`（已移植）→ `0x517AD0/0x518D60` → `0x519120/0x518EF0` → `0x4ACE30/0x5179B0`（phase35/36/39/28）。
9. 生产/道路/建筑钩子收尾：`0x51E4A0`(+`0x4BC440`)、`0x4ACFC0`、`0x416D20`（phase20/21/31）。
10. 就业/劳动：`0x4AE220/0x4AE7F0/0x4F19A0` 与 `0x4AD4A0`（phase25/23，含 `0x1192B04` 块）。
11. 道路/对象池：`0x4AF350` 与 `0x4AEEB0`（phase7/8）。
12. 城市统计/纪念物：`0x591EC0/0x5925A0` 与 `0x5F02C0/0x4B94A0`（phase13/45）。
13. 火灾/风险：`0x4AF9B0` 与 `0x4AF5D0/0x42D9A0`（phase43/44）。
14. 数组复位/池清扫：`0x5D3CB0`、`0x4AEDF0`、`0x4AEB30`、`0x5DBCC0`（phase16/17/10/32）。
15. 其余复位与遍历：`0x4AFD20`、`0x4AE8B0`、`0x4ACE60`、`0x5253A0`、`0x4ACD10`、`0x4ACC40`、`0x4AD3D0`、`0x4AE1A0` 与 `0x4F1560`（含已命名的会计 blocker）。

以上只是静态优先级：第 1-5 项是“能开始按原顺序推进”的最小集合，不代表第 6 项之后无依赖；每项的完成标准仍是该阶段在原返回点正常返回且所需全局/对象状态有来源，而不是返回 0 或空 stub。

### City modulo clocks

`2026-10-21` re-verified this row directly against the same listing of the owned executable, SHA-256 `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`, image base `0x400000`, code VA = file offset + `0x400000`. The leaf `0x529990` runs to `0x529A75` and returns at `0x529A76`; `0x5371A0` calls it second (`0x5371A5`, after `0x4189B0` at `0x5371A0`). The ten globals are one contiguous dword array `0x10C71D4..0x10C71F8`, advanced in **descending** address order; every slot has the same shape (load u32 `0x529990` for slot 0, `inc`, store the incremented value, `cmp` against the period literal, and a final `store 0` only when the incremented value is `>=` the period). The reset is therefore taken at the third store, and a stored value equal to or above the period also collapses to 0.

| Index | Counter global | Period | Load / cmp / store / reset store |
|---:|---|---:|---|
| 0 | `0x10C71F8` | 4 | `0x529990` / `0x529998` / `0x52999B` / `0x5299A2` |
| 1 | `0x10C71F4` | 8 | `0x5299A8` / `0x5299AE` / `0x5299B1` / `0x5299B8` |
| 2 | `0x10C71F0` | 16 | `0x5299BE` / `0x5299C4` / `0x5299C7` / `0x5299CE` |
| 3 | `0x10C71EC` | 24 | `0x5299D4` / `0x5299DA` / `0x5299DD` / `0x5299E4` |
| 4 | `0x10C71E8` | 32 | `0x5299EA` / `0x5299F0` / `0x5299F3` / `0x5299FA` |
| 5 | `0x10C71E4` | 64 | `0x529A00` / `0x529A06` / `0x529A09` / `0x529A10` |
| 6 | `0x10C71E0` | 128 | `0x529A16` / `0x529A1C` / `0x529A21` / `0x529A28` |
| 7 | `0x10C71DC` | 256 | `0x529A2E` / `0x529A34` / `0x529A39` / `0x529A40` |
| 8 | `0x10C71D8` | 384 | `0x529A46` / `0x529A4C` / `0x529A51` / `0x529A58` |
| 9 | `0x10C71D4` | 768 | `0x529A5E` / `0x529A64` / `0x529A69` / `0x529A70` |

Per slot the net effect is `next = previous + 1; value = next >= period ? 0 : next`, so a slot cycles `1,2,...,period-1,0` from zero. The ten globals are disjoint, so the descending order is unobservable; no slot reads another. Table A row A2's periods (4/8/16/24/32/64/128/256/384/768) are confirmed, with the global-to-period mapping above. The `0x529A81` reset recorded in [SAV_FIGURES.md](SAV_FIGURES.md):788 belongs to the separate leaf that starts at `0x529A80` (writes 100 to the ring cursor `0x10C6F8C`, then calls `0x4189B0` 100 times); the clocks' resets are the ten `0x5299A2..0x529A70` stores.

**Serialization: not restored (no T-relative offset exists).** The SAV19 branch `0x5311DC..0x531873` contains 89 `push size; push address; mov ecx, esi; call 0x780533` read requests; none pushes any of `17592788/17592792/17592796/17592800/17592804/17592808/17592812/17592816/17592820/17592824`. The reads around the neighbouring RNG state are `0x10C714C` at call `0x531451`, `0x10C7148` at `0x53145F`, then `0x101D0AC` at `0x53146D` and `0x101D0B0` at `0x53147B`; a serialized clock array would have to appear between those, and it does not. Across the whole listing each of the ten addresses occurs exactly three times (load, increment store, reset store), all inside `0x529990`, so no other code path — including the versioned load at `0x5306BA` — references them either. A loaded save therefore cannot supply these values; a native city tick must give them explicit initial values (process start, or an authored re-entry point) and must not derive them from save bytes.

Implementation: [original_city_clocks.hpp](../game_lab/original_city_clocks.hpp) holds the ten-slot aggregate, the address/period tables and `advanceOriginalCityClocks`; [original_city_clocks_tests.cpp](../game_lab/original_city_clocks_tests.cpp) checks every slot's exact wrap, order independence and a 1000-step table against an independent re-derivation; [scripts/city_clocks.py](../scripts/city_clocks.py) builds and runs it in release and `--mode undefined` (UBSan). Current result: `{"passed":true,"authored_checks":5008}` in all four combinations (with and without `EMPEROR_GAMEDATA_ROOT`), identical because this pass consumes no game data. This closes the row's behavioural question only; it moves no original-data metric and its clock values remain unobservable until a phase that reads them is ported.

## Phase 调度器复验：48 个 case 的接线、计数语义与两个 blocker（2026-10-21）

本节把 Table B2 从“每个阶段叫什么”推进到“每个阶段在 `0x4AC4E8` 跳表里落到哪个 case、第一件事调谁、在绝对地址上写了什么”，并给出计数推进、三段 precheck、两个 named blocker 的输入序列化与推荐移植顺序。复验使用同一份 listing：SHA-256 `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`，image base `0x400000`，code VA = file offset + `0x400000`。

**本节不新增测量。** 状态列仍只由 [SAV.md](SAV.md)、[SAV_FIGURES.md](SAV_FIGURES.md) 与本文件的地址检索决定；判定口径是该地址在三份文档里的措辞，不是本节的重新测量。凡是文档里查不到语义的阶段，描述一律写 `unclear`。

### Table D — phase 1..48：case 目标、首个被调、写全局、状态

“首个被调”按 case 块内的实际调用顺序列出；`→` 表示前一个调用的返回值被搬到 ECX 再调下一个。“写全局”是**同一深度的下界**：只统计 case 块内联写、以及每个直接被调者函数体首层的绝对操作数写（含 `[4*reg+imm]` 的数组基址），不含 `[reg+disp]` 的对象内写，也不含第二层以下被调者。

| phase | case 目标 | 首个被调函数 | 一句话描述 | 写全局（绝对地址，深度 1） | 状态 |
|---:|---|---|---|---|---|
| 1 | `0x4AC2FA` | `0x4F1560` | 城市会计；8 个直接调用后尾跳 `0x4AFFA0→0x44BAB0` | 自身无；下游见 D3.1 | not ported（named blocker） |
| 2 | `0x4AC4C8` | — | 空 case，直接进推进块 | — | empty |
| 3 | `0x4AC42D` | —（内联写） | 未明语义的全局置 1 | `0x13F8E78` | not ported |
| 4 | `0x4AC304` | `0x4819F0` | 城市统计：`0x591AA0(0)` 返回 ≥0 时清两个计数 | `0x131262C`、`0x1312630` | not ported |
| 5 | `0x4AC30E` | `0x54C6F0`（push 0） | unclear：`0x54` 子系统模式 0 链 | 无 | not ported |
| 6 | `0x4AC31D` | `0x5177B0` → `0x5185C0` | 住房倒计时 / 劳动字节 | `0x131289C` | not ported |
| 7 | `0x4AC32E` | `0x4AF350` | 路线重建 / 清 road cache | `0xC81B70`、`0xC81B74`、`0xC81B78`、`0x101C560`、`0x1312588`、`0x131258C` | partial |
| 8 | `0x4AC338` | `0x4AEEB0` | 建筑池消费 | `0xC82FFC`、`0xC830C8` | partial |
| 9 | `0x4AC342` | `0x40FC20` | 空 handler，函数体只有 RET | 无 | not ported（no-op） |
| 10 | `0x4AC34C` | `0x4AEB30` | registry 遍历 + 清计数 | `0xC82EE0`、`0xC82ED8`、`0xC82EDC` | not ported |
| 11 | `0x4AC356` | `0x590A50` | 人口峰值 `max(0x130F988,0x131257C)` 存回 | `0x131257C` | ported |
| 12 | `0x4AC360` | `0x4ACE60` | unclear：类型过滤的建筑遍历 | 无 | not ported |
| 13 | `0x4AC36A` | 内联测 `0x1312564`，再 `0x591EC0`、`0x5925A0` | 城市统计重置 / 汇总 | `0x1312870`、`0x130F967`、`0xC82F68` | not ported |
| 14 | `0x4AC4C8` | — | 空 case | — | empty |
| 15 | `0x4AC4C8` | — | 空 case | — | empty |
| 16 | `0x4AC398` | `0x5D3CB0` | unclear：清 `0x13123D0..0x1312444` 区块后遍历 | 无（基址为立即数） | not ported |
| 17 | `0x4AC3A2` | `0x4AEDF0` | unclear：registry 遍历 + `0x591490`/`0x40E680` | `0x1312128`、`0x1312528` | not ported |
| 18 | `0x4AC3AC` | `0x426A60` → `0x5A8170`（push 0） | unclear：owner 向量通行 | `0x85D720`、`0x1312F78` | not ported |
| 19 | `0x4AC4C8` | — | 空 case | — | empty |
| 20 | `0x4AC3BF` | `0x51E4A0` → `0x4BC440` | 生产/作物 + 地形资源生长 | `0xD62438`–`0xD6243D`、`0x101C560`、`0xD5A6FC`、`0xFE9880`、`0xE92DD0` | partial |
| 21 | `0x4AC3CE` | `0x4ACFC0` | 道路刷新 | `0x1192A2C`、`0xD62400`；另有对象内 `+0x2A`/`+0x2C` | partial（named blocker） |
| 22 | `0x4AC3D8` | `0x4AD3D0` | 住房自由空间 / 模型容量 | `0x130F994`、`0x130F998` | not ported |
| 23 | `0x4AC3E2` | `0x4AD4A0` | 劳动/产业分配 | `0xD62418`、`0x1192A2C`、`0x1192B04`–`0x1192B0D`、`0x1311F7C`、`0x1311F80`、`0x1311F84`、`0x1311F88`、`0x1311FB0`、`0x1311FB4`、`0x1311FB8` | not ported |
| 24 | `0x4AC3EC` | `0x4AE1A0` | 住房迁出 | 无 | not ported |
| 25 | `0x4AC3F6` | `0x4AE220` → `0x4AE7F0` → `0x4F19A0` | 就业派发 | `0x1312138`、`0x1312140`、`0x131220C`、`0x1192A84`、`0x1192A2C` | not ported |
| 26 | `0x4AC4C8` | — | 空 case | — | empty |
| 27 | `0x4AC40A` | `0x5253A0` | unclear：每槽 4 次 `0x525470` | `0x101C504` | not ported |
| 28 | `0x4AC414` | `0x4ACE30` | 水/服务状态派生 | `0xC82F14` | not ported |
| 29 | `0x4AC41E` | `0x54C6F0`（push 1） | 同 phase 5 的模式 1 链 | 无 | not ported |
| 30 | `0x4AC42D` | —（内联写） | 同 phase 3 | `0x13F8E78` | not ported |
| 31 | `0x4AC43C` | `0x416D20` | 建筑钩子：profiling + 作物 bit80 | `0x1032658`；IAT `0x88F720` | partial |
| 32 | `0x4AC446` | `0x5DBCC0` → `0x4B02A0` | unclear：清四字后遍历 | `0x13124B2`、`0x13124B4`、`0x13124B6`、`0x13124B8`、`0x1192A2C` | not ported |
| 33 | `0x4AC452` | `0x4AF230` → `0x4AF2D0` | 每类型标志复位 | `0x131249C`、`0x13124A0`、`0x13124A4`、`0x13124A8`、`0x13124AC`、`0x13124BE` | not ported |
| 34 | `0x4AC4C8` | — | 空 case | — | empty |
| 35 | `0x4AC45E` | `0x5177B0` → `0x517B40` | 住房服务衰减（12 字节饱和到 0） | 无 | ported |
| 36 | `0x4AC46C` | `0x5177B0` → `0x517AD0` | 服务聚合 `+218` | 无 | not ported |
| 37 | `0x4AC47A` | `0x53C870` → `0x44F1D0` | unclear：容器/异常路径 | 无 | not ported |
| 38 | `0x4AC488` | `0x4ACD10` | unclear：registry 遍历 + `0x44F180` | 无 | not ported |
| 39 | `0x4AC48F` | `0x519120` | 住房演化 | `0xC82CF4` | not ported |
| 40 | `0x4AC496` | `0x4ACC40` | unclear：registry 遍历 | `0x101D0FC` | not ported |
| 41 | `0x4AC4C8` | — | 空 case | — | empty |
| 42 | `0x4AC4C8` | — | 空 case | — | empty |
| 43 | `0x4AC49D` | `0x4AF9B0` | 火灾/蔓延 | `0xC82F20`、`0xC82F24`、`0xC82F28`、`0xC834B6` | not ported |
| 44 | `0x4AC4A4` | `0x4AF5D0` | 风险/火灾；ECX=`0x8C7630`，尾跳 `0x42D9A0` | 无（深度 1） | not ported |
| 45 | `0x4AC4AB` | `0x5F02C0`（ECX=`0x1E535F8`）→ `0x4B94A0` | 纪念物 / 城市对象 | `0x101D12C` | not ported |
| 46 | `0x4AC4C8` | — | 空 case | — | empty |
| 47 | `0x4AC4BC` | `0x4AFD20` | 计数复位 | `0xC82D30`、`0xC82D34`、`0xC82D44`、`0xC82D48` | not ported |
| 48 | `0x4AC4C3` | `0x4AE8B0` | 建筑倒计时 | 无；函数返回后落进 `0x4AC4C8` | not ported |

关于本表的读法：

- 48 个跳表项一一对应 phase 1..48，顺序由 `0x4AC2F3` 的 `jmp dword ptr [4*eax + 0x4AC4E8]` 决定；跳表本身在文件偏移 `0xAC4E8`，192 字节。9 个空 case（phase 2、14、15、19、26、34、41、42、46）的表项都指向 `0x4AC4C8`，所以“空”不是解析失败。
- 只有 phase 1、3/30、13 没有“先调用再返回”的形状：phase 1 是单调用加尾跳；phase 3/30 是纯内联写 `0x13F8E78`；phase 13 先内联读 `0x1312564` 再决定是否调 `0x591EC0`，`0x5925A0` 恒调。
- “写全局”与 Table B2 的“直接全局（SAV19）”列口径不同：B2 只列 SAV19 相关项，本表列出该深度上所有绝对写。因此本表多出 `0x1312588/0x131258C`（phase 7）、`0xD5A6FC`/`0xFE9880`/`0xE92DD0`（phase 20）、`0xC834B6`（phase 43）、`0x101D12C`（phase 45）等项。B2 里出现过而本表未复现的少数项（phase 8 的 `0xC82F6C`、phase 43 的 `0xC82F2C`、phase 45 的 `0x875C88`）位于第二层以下被调者，不在本列深度内，两者不矛盾。
- 落在代码段（`0x401000..0x560000`）的索引基址是跳表而不是全局，已从“写全局”列剔除；读侧同理。
- phase 48 的 case 块没有自己的 `jmp 0x4AC4C8`，它在 `0x4AC4C3` 调用 `0x4AE8B0` 后**顺序落入** `0x4AC4C8`。这是 48 个 case 里唯一的 fall-through；其余 38 个非空 case 都在自己的最后一条指令跳回 `0x4AC4C8`。

### D1. 计数如何推进，一次 admitted update 跑几个 phase

计数只有一个 dword：`0xC82EF8`（listing 里打印为十进制 `13119224`）。整个推进链是：

| 指令 | 语义 |
|---|---|
| `0x4AC2E4` | 读 `[0xC82EF8]` |
| `0x4AC2E9` | 自减 1 |
| `0x4AC2EA` | 与 47 比较（无符号） |
| `0x4AC2ED` | 大于 47 时跳到 `0x4AC4C8`，本次不跑任何 phase |
| `0x4AC2F3` | `jmp [4*eax + 0x4AC4E8]`，用“计数−1”索引跳表 |

所以 **counter 值 1..48 分派 phase 1..48，counter 49、50 以及 0 都是空转**（`0−1` 会回绕成 `0xFFFFFFFF`，无符号比较同样大于 47）。推进块在 case 之后：

| 指令 | 语义 |
|---|---|
| `0x4AC4C8` | 读 `[0xC82EF8]` |
| `0x4AC4CD` | 加 1 |
| `0x4AC4CE` | 与 51 比较 |
| `0x4AC4D1` | 先写回 `[0xC82EF8]` |
| `0x4AC4D6` | 小于 51 就 `ret`（`0x4AC4E7`） |
| `0x4AC4D8` | 否则写 0 |
| `0x4AC4E2` | 跳 `0x4AC650`（周期尾） |

结论：

1. **一次 admitted update 只跑一个 phase，或者一个都不跑，绝不跑两个。** 39 个非空 case 里有 38 个以 `jmp 0x4AC4C8` 结束，phase 48 以 fall-through 进入 `0x4AC4C8`；case 内部没有任何回跳 `0x4AC2F3` 的路径，所以不存在“一次调用连跑多阶段”。
2. **周期长度 = 51 次 admitted update。** counter 从 0 出发时：第 1 次空转（0 写 1），第 2..49 次依次跑 phase 1..48，第 50 次空转（49→50），第 51 次把 50→51 并立刻回写 0、进入 `0x4AC650`。51 次之后 counter 回到 0、48 个 phase 各跑一次、周期尾跑一次，状态与起点同构。以后每 51 次 admitted update 重复一次。
3. **被拒绝的 update 不推进计数。** 拒绝路径是 `0x4AC2DE` 直接跳到 `0x4AC4E7` 的 `ret`，既不读也不写 `0xC82EF8`，因此拒绝可以无限重复而不会“跳过一个 phase”。这一点对 native 端很关键：只要某个 phase 还拒，counter 就永远停在同一格，重试不会掩盖问题。
4. phase 进度与“世界时间”是两套计数：`0x4AC650` 在 `0x4AC66E` 自增 `0xC82EF4`、在 `0x4AC674` 自增 `0xC82EF0`；后者在 `0x4AC6B8` 读回、`0x4AC6BD` 与 15 比较，`0x4AC6C4` 在 ≤15 时跳过整段月度/季度分支，否则在 `0x4AC716` 把 `0xC82EF0` 钉回 15。`0xC82EEC` 只在 >15 的分支里推进（`0x4AC6CA` 读、`0x4AC7A3` 写）。也就是说“51 次 admitted update 的 phase 周期”和“15 个周期尾的月周期”是两个不同长度，不能互相换算成一次。

### D2. 三段 precheck 做什么

三段都在分派之前、都在 `0x4AC2B0..0x4AC2DE` 内，短路求值，任意一段“放行”就跳去分派：

| 段 | 取对象 | 判据 | 放行条件 | 指令 |
|---|---|---|---|---|
| P1 | `0x408AC0` 返回 `0x10BC348`，作为 ECX 传给 `0x4FAE50` | `0x4FAE50` 返回 `[ecx+16] != 0`（`setne`） | 返回 0 | `0x4AC2B0`、`0x4AC2B7`、`0x4AC2BC`、`0x4AC2BE je` |
| P2 | `0x55BCB0` 返回 `0x12A4BA8`，作为 ECX 传给 `0x55B540` | `0x55B540` 返回 al 布尔；内部经 `0x480740`/`0x481F10`/`0x443FC0` 并检查 `[ecx+2124]` | 返回 0（al=0） | `0x4AC2C0`、`0x4AC2C7`、`0x4AC2CC`、`0x4AC2CE je` |
| P3 | `0x55CE80` 返回 `0x12A3D70`，作为 ECX 传给 `0x55CEE0` | `0x55CEE0` 返回非 0 即“要拦住” | 返回 0 | `0x4AC2D0`、`0x4AC2D7`、`0x4AC2DC`、`0x4AC2DE jne` |

三条的短路方向不对称：P1、P2 是 `je 0x4AC2E4`（判据为假 → 放行），P3 是 `jne 0x4AC4E7`（判据为真 → **拒绝**）。因此只有“P1 非 0 且 P2 非 0 且 P3 非 0”同时成立时整次 update 才被拒绝；任何一段为假都直接分派。

**拒绝也有副作用。** `0x55CEE0` 自身在 `0x55CEE0..0x55D1B5` 内写 `0x10DE043`、`0x10DE044`、`0x10DE3F4`、`0x1192A2C`、`0x12A53F4`、`0x13125E0`、`0x13125E4`、`0x88EBDC`，并读 `0x10DE043/44`、`0x12A4BB0`、`0x12A53F4`、`0x12A544C`、`0x13124F8`、`0x13125DC`、`0x13125E8`。也就是说：**phase 一次都没跑，precheck 也已经把若干全局改掉**。native 端如果先实现 precheck 而不同时实现这些写，第一次拒绝之后的全局状态就会和原版不同。

调用者侧：`0x5371B6` 读 `0x88EC38`，为 0 时在 `0x5371BF` 调 `0x4AC2B0`，随后 `0x5371C9` 调 `0x466E90`（ECX=`0xA98370`）、`0x5371CE` 调 `0x4E27E0`。`0x88EC38` 非 0 时 `0x5371BD` 直接跳到 `0x5371CE`，跳过调度器与 `0x466E90`，`0x5371D3` 再读同一个地址后 `0x5371DA jne` 直接 RET，连 `0x564B50` 也跳过——**但 `0x4E27E0` 仍然会跑**。这一点在 native 侧容易被简化成“整段跳过”。

### D3. 两个 named blocker 的输入序列化

两个 blocker 在 48 阶段序列里都只有一个入口阶段，所以“少一个就整条 tick 停住”是结构性的，不是保守估计。

#### D3.1 `0x4F1560`（phase 1，case `0x4AC2FA`）

- **谁到达它**：48 阶段序列里只有 phase 1——`0x4AC2FA` 调用 `0x4F1560`，`0x4AC2FF` 跳回推进块。另有一个序列外调用点 `0x406D91`：返回后写 `0x88EC40`、`0x10DE014`、`0x10DE045`，并把 `0x13F8DD0` 置 6、`0x115F720` 置 1 后 RET，是一条整体重同步路径。两者共享同一份会计状态，所以“只给 phase 1 接线”不足以覆盖所有写者。
- **直接被调者（按顺序）**：`0x4F1590`（`0x4F1560`）、`0x4F1A70`（`0x4F1565`）、`0x4F1A80`（`0x4F156A`）、`0x4F1A90`（`0x4F156F`）、`0x5919D0`（`0x4F1574`）、`0x4AF2D0`（`0x4F1579`）、`0x4AEDF0`（`0x4F157E`）、`0x54CC60`（`0x4F1583`），随后 `0x4F1588` 尾跳 `0x4AFFA0`（该函数体只有 1 条指令，再尾跳 `0x44BAB0`）。
- **自身直接读写**：`0x4F1560` 的 9 条指令内没有绝对操作数读，也没有绝对操作数写。所有状态都在被调者里。
- **被调者写（绝对地址）**：
  - `0x4F1590`：`0x1312200`、`0x1312204`、`0x1312208`、`0x1312210`、`0x8501FC`、`0x8501FD`
  - `0x5919D0`：`0x1312484`
  - `0x4AEDF0`：`0x1312128`、`0x1312528`
  - `0x54CC60`：`0x1312002`、`0x1312004`、`0x1312568`、`0x131256B`、`0x131256C`、`0x131256D`、`0x131256F`、`0x1312798`、`0x131279C`、`0x13127A0`、`0x13127A4`、`0x13127CC`、`0x13127D0`、`0x13127D4`、`0x13127D8`、`0x10AEEC0`、`0x10AEEC4`、`0x10AEEC8`、`0x10AEECC`、`0x11A28D8`
- **被调者读（绝对地址）**：`0x1312134`、`0x1312200`、`0x1312204`、`0x1312208`、`0x1312210`、`0x8501FC`、`0x8501FD`（`0x4F1590`）；`0x1312484`、`0x1312514`（`0x5919D0`）；`0x131205C`、`0x1312128`、`0x1312528`、`0xC5CDA0`（`0x4AEDF0`）；`0x10AEEC0`、`0x10AEEC4`、`0x10AEEC8`、`0x10AEECC`、`0x11A28D8`、`0x11A2B08`、`0x1312002`、`0x1312004`、`0x1312568`、`0x131256B`、`0x131256C`、`0x131256D`、`0x131256F`、`0x1312798`、`0x131279C`、`0x13127A0`、`0x13127A4`、`0x13127CC`、`0x13127D0`、`0x13127D4`、`0x13127D8`（`0x54CC60`）。
- **SAV19 恢复情况**：写侧共 29 个不同地址，只有 `0x10AEEC0`、`0x10AEEC4`、`0x10AEEC8`、`0x10AEECC` 在窗口内（请求 `0x531653`、`0x531645`、`0x531637`、`0x531629`，各 4 字节），其余 25 个不在。读侧共 34 个不同地址，同样只有这 4 个在窗口内（其余 30 个不在）。**因此会计表的主体（`0x1312xxx`、`0x8501FC/FD`、`0x11A28D8`）必须由 native 侧显式给初值，不能从存档字节推出。**

#### D3.2 `0x4ACFC0`（phase 21，case `0x4AC3CE`）

- **谁到达它**：48 阶段序列里只有 phase 21——`0x4AC3CE` 调用 `0x4ACFC0`，`0x4AC3D3` 跳回推进块。另有一个序列外调用点 `0x534EBF`：该处按 `0x4AF350 → 0x4ACFC0 → 0x4AEEB0` 的顺序直接调三个 phase 体（前接 `0x4E8AC0`），是 phase 7/21/8 的手工重放路径。序列里和序列外的两条路径会写同一批道路缓存，任何一条漏接都会让道路状态分叉。按 listing 的函数边界（入口到 ≥2 个 `nop` 的填充前），`0x4ACFC0` 覆盖 `0x4ACFC0..0x4AD257`，其中 `0x4AD186`、`0x4AD1A6`、`0x4AD212` 是同一个函数内的共享出口块（`0x4AD20D` 等处跳进来），不是外部被调者。
- **直接被调者（按顺序）**：`0x5AE140`、`0x413B40`、`0x554C00`、`0x426D10`、`0x554C00`、`0x547E60`×2、`0x46A1C0`、`0x425260`、`0x425250`、`0x5AE380`、`0x5AD8F0`、`0x5AE140`、`0x5AE380`、`0x5AD8F0`、`0x5AE140`、`0x4AD260`、`0x5AD960`、`0x5ADD40`、`0x4B67B0`、`0x5403C0`、`0x4138F0`、`0x546EA0`、`0x5EF190`、`0x408E20`、`0x4138F0`；另有一次**间接调用** `call dword ptr [edx+132]`（`0x4AD029`，vtable 槽 `+0x84`）。
- **自身写**：`0x1192A2C`（`0x4AD1AA`、`0x4AD216` 两处置 1）、`0xD62400`（`0x4AD207` 置 0），以及每个 live 对象的 `word[+0x2A]`、`word[+0x2C]`（`0x4AD021`、`0x4AD025`，清零）。
- **自身读**：`0xC5CDFC`、`0xC5CDFE`、`0xC5CE00`、`0xC5CE02`、`0xD62400`、`0x101D0C8`、`0x1192A2C`，以及 `0x1391FE0` 的 `[4*index + 0x1391FE0]` 三处（`0x4AD063`、`0x4AD10F`、`0x4AD16A`）。
- **间接调用不是直接调用**：`0x4AD029` 的槽 `+0x84` 在 `.rdata` 的 71 张 vtable 里都指向 `0x51CE50`（出现处如 `0x7A99D8`、`0x7A9C54`）。`0x51CE50` 只有 8 条指令、唯一一次调用是 `0x51CE55` 调 `0x426DF0`；`0x426DF0` 再调 `0x4BAF40`。`0x426DF0` 在整份 listing 里只有 `0x51CE55` 这一个直接调用点。所以 `0x4ACFC0 → 0x426DF0` 是“虚表槽 + 一层转发”，把它当作 `0x4ACFC0` 的直接被调者会画错依赖图。
- **间接链的状态**：`0x4BAF40` 写 `0x10C72A8`、`0x10C72AC`，读 `0x10C72A8`、`0x10C72AC`、`0x101D0C8`、`0x1391FE0`、`0x820038`、`0xEC5A10`、`0xF6A9E0`、`0xFC3750`（173 条指令）。`0x10C72A8/AC` 是 road distance 缓存；两者都不在 SAV19 窗口内。
- **SAV19 恢复情况**：`0xC5CDFC`、`0xC5CDFE`、`0xC5CE00`、`0xC5CE02`、`0x101D0C8`、`0x1391FE0`、`0x1192A2C`、`0xD62400`、`0x10C72A8`、`0x10C72AC` **全部不在**窗口内。窗口里有 `0xC5D0D8`（请求 `0x5314C1`）和 `0x101D0AC`/`0x101D0B0`（`0x53146D`/`0x53147B`），都是相邻但不同的全局，不能替代。
- **`0x1192A2C` 是跨阶段脏标志**：phase 21、23（`0x4AD5BA`）、25（经 `0x4AE7F0` 的 `0x4AE892`）、32（`0x4B02A0` 的 `0x4B03A3`）都写它，precheck `0x55CEE0` 也写它，而它不在 SAV19 窗口内。它决定了“劳动/道路是否重算”，因此是 phase 21/23/25/32 之间唯一的显式耦合点；native 端如果把它当常量，四个阶段会同时错。

**拒绝分区的实测口径**（[SAV.md](SAV.md):827）：N10/N100 是原子拒绝，构成 `1,048 city_accounting_4f1560_4ac2fa + 15 road_refresh_4acfc0_426df0_1391fe0 + 2 production_virtuals_51e4a0_74_208 = 1,065/1,065`。即 10 次与 100 次 admitted update 各拒绝 1,065 次，其中 1,048 次来自 phase 1、15 次来自 phase 21、2 次来自 phase 20；phase 1 的拒绝发生在任何未移植的 phase 1 producer 之前，是保守拒绝而不是“存档缺字段”的证明。

### D4. 对 Table C 的修正

Table C 原来只列了 7 个全局，且漏掉了调度器在周期尾恢复的 `0xC82EF0`、`0xC82EF4`、`0xC82EEC` 与另外几项。按同一窗口重扫（91 个请求、89 个不同地址；另有 2 个请求压的是寄存器而不是立即数，不计入）：

| 全局 | 写入者 | restored? | 请求点 |
|---|---|---|---|
| `0xC82EF8` | `0x4AC2B0`/`0x4AC4C8`（phase 计数） | 是 | `0x531427`（size 4） |
| `0xC82EF0` | `0x4AC650`（周期尾，月计数） | 是 | `0x531435`（size 4） |
| `0xC82EF4` | `0x4AC650`（周期尾，总周期计数） | 是 | `0x531443`（size 4） |
| `0xC82EEC` | `0x4AC650`（仅 >15 分支） | 是 | `0x5312AF`（size 4） |
| `0xC82DB0` | phase 33（B2 记为 `0x4AF230` 链清 30 dword） | 是 | `0x5314E3`（size 144） |
| `0xC82E40` | phase 33（同上） | 是 | `0x5314D2`（size 144） |
| `0xD623FC` | `0x4AC650` | 是 | `0x5312A1`（size 4） |

即：**`0xC82EF0` 与 `0xC82EF4` 也在窗口内**，`0xC82EEC` 同样在；B2 记为 phase 33 清零的两个 30 dword 数组以 144 字节整块恢复（覆盖 36 dword，比 B2 记录的 30 dword 更宽）。Table C 的行应以此表为准。

先前“phase 33 的 `0xC82DB0/0xC82E40` 未 restored”的说法不成立，应改为“`0xC82DB0`/`0xC82E40` restored（各 144 字节），只有 `0x131249C..AC` 与 `0x13124BE` 未 restored”。

### D5. 状态计数与推荐移植顺序

| 状态 | 行数 | phase |
|---|---:|---|
| ported | 2 | 11、35 |
| partial | 5 | 7、8、20、21、31 |
| not ported | 32 | 1、3、4、5、6、9、10、12、13、16、17、18、22、23、24、25、27、28、29、30、32、33、36、37、38、39、40、43、44、45、47、48 |
| empty | 9 | 2、14、15、19、26、34、41、42、46 |
| 合计 | 48 | — |

48 行里 `ported` 2 行（4.2%）、`partial` 5 行（10.4%）、`not ported` 32 行（66.7%）、`empty` 9 行（18.8%）。`ported` 的两行分别有 [SAV.md](SAV.md):804 与 [SAV_FIGURES.md](SAV_FIGURES.md):1007 的断言支撑；`empty` 的 9 行不需要实现；其余 37 个非空阶段都还没有在原返回点正常返回过。

要让 **10 次** admitted update 连续不拒绝（counter 从 0 起算，即覆盖 phase 1..10），最小集合是：调度器计数链 + 三段 precheck + phase 1..10。phase 9 只需一个立即 RET 的空 handler。

要让 **100 次** admitted update 连续不拒绝，必须把 48 个 phase 与周期尾全部具备：100 次里 counter 走完近两整圈（第 51 次触发 `0x4AC650`），任何一格还拒，计数就停在那格，后面的 phase 根本不会被执行。建议顺序：

1. **调度器核心**：`0x4AC2B0..0x4AC2DE` 三段 precheck、`0x4AC2E4..0x4AC2F3` 计数与跳表、`0x4AC4C8..0x4AC4E7` 推进与拒绝、`0x4AC4E8` 的 48 项表。判据是 counter 轨迹 `0,1,2,…,48,49,50,0` 与“拒绝不动 counter”。
2. **phase 1（`0x4F1560`，blocker）**。它是第一个阶段，也是 1,048/1,065 次拒绝的来源；不给它显式初值，10 次都到不了。
3. **phase 2..10**，其中 phase 9 是空 handler、phase 3/30 是内联写。到这里 **10 次 admitted update** 成立。
4. **phase 11..20**。phase 20 的 production virtuals（`0x51E4A0` 的 `+74`/`+208`）是 2/1,065 次拒绝的来源，必须在进入 20 之前定好。
5. **phase 21（`0x4ACFC0`，blocker）**。先按虚表槽 `+0x84 → 0x51CE50 → 0x426DF0 → 0x4BAF40` 建对依赖图，再给 `0xC5CDFC..0xC5CE02`、`0x1391FE0`、`0x1192A2C`、`0xD62400`、`0x10C72A8/AC` 定初值所有权；这 15 次拒绝是 process-state，不是存档缺字段。
6. **phase 22..48**，按 22→48 的自然顺序，前置是 `0x1192A2C` 的四写者（21/23/25/32）与 `0x1032658`（phase 31，已 restored）。
7. **周期尾 `0x4AC650`** 与 `0xC82EF0`/`0xC82EF4`/`0xC82EEC` 的推进（含 `0x4AC716` 的 15 上限）。没有它，counter 到 50 就无处可去，100 次里第 51 次必然断。

每一步的完成标准都是“该阶段在原返回点正常返回”，而不是“返回 0 或空 stub”。按这个顺序，第 3 步结束时 10 次可跑，第 7 步结束时 100 次可跑；中途任何一格拒绝都会让 counter 停住，这一点本身就是最好的回归信号。

最后一条不属阶段序列但会影响验收：两个 blocker 都各有一个序列外调用点（phase 1 的 `0x406D91`、phase 21 的 `0x534EBF`）。native 侧如果只按跳表顺序接线，会漏掉这两条重同步路径；它们与阶段序列共享同一批全局，验收时至少要在“序列内”和“序列外”两种入口下得到一致的会计/道路状态。

## 剩余两步的深度映射：`0x466E90` 与 `0x564B50`（2026-10-21）

本节把 Table A 的 A6（`0x466E90`，场景/事件槽）与 A8（`0x564B50`，figure 之后的周期步骤）从“一句话描述 + 一层被调者”推进到：对象布局、一次 tick 的精确分支与写入、槽的创建路径、直接被调者的全局、以及 SAV19 读窗口对这些状态的实际覆盖。复验使用同一份 listing：SHA-256 `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`，image base `0x400000`，code VA = file offset + `0x400000`。

**本节不新增测量、不执行原程序。** 状态列仍只由 [SAV.md](SAV.md)、[SAV_FIGURES.md](SAV_FIGURES.md) 与本文件的地址检索决定；描述里的 `unclear` 表示结构证据不足以确定业务语义。本节不含反汇编文本，只有地址、字段偏移与行为描述。列出的一层被调者即 Table B3/B5 的 4 + 9 行，本节再加两侧的叶函数与被调者首层全局。

### 1. `0x466E90` — 场景/事件槽 tick

#### 1.1 调用位置与门

`0x466E90` 在整份 listing 里只有一个调用点：`0x5371C9`，紧接 `0x4AC2B0` 返回之后，`ECX` 由 `0x5371C4` 装成立即数 `0xA98370`（=11,109,232）。函数体到 `0x466F03` 的 `ret` 结束（`0x466F04..0x466F0F` 是 `nop` 填充），共 54 条指令，**自身没有任何绝对地址操作数**：读和写全部落在 `ECX` 指向的对象上，以及被调者内部。

它只在 `[0x88EC38]==0` 时被调用：`0x5371B6` 读该全局，非 0 时 `0x5371BD` 直接跳到 `0x5371CE`，跳过调度器与 `0x466E90`。这个门和 A5 共用同一个字节，D2 已记 `0x88EC38` 不在 SAV19 窗口内（窗口里的邻接项是 `0x88EC3C`，请求 `0x531489`）。

#### 1.2 对象 `0xA98370` 的布局

`0xA98370` 不是指针也不是数组基址，而是一个 **259,960 字节（`0x3F778`）的静态对象**，下一个对象的基址正好是 `0xAD7AE8`。已证实的成员：

| 偏移 | 绝对地址 | 大小 | 含义与证据 |
|---|---:|---:|---|
| `+0` | `0xA98370` | 51,984 字节（12,996 dword） | 索引表 #1；`0x466950` 清零、`0x4680C0`/`0x4680D0` 以 `byte[obj+id]` 读写 |
| `+51984` | `0xAC4EE0` | 207,936 字节（51,984 dword） | 索引表 #2；`0x466E60` 写 `[obj+4*id+51984]`、`0x468110` 读同一位置 |
| `+259920` | `0xAD7AC0` | 8 × dword = 32 字节 | **8 个场景槽指针**；`0x466E90` 两遍循环、`0x4669B0`、`0x466F10`、`0x466E00` 都按 `[obj+4*i+259920]` 访问 |
| `+259952` | `0xAD7AE0` | 1 字节 | **本 tick “任一槽有进展”的聚合标志**；唯一写者是 `0x466E90`，唯一读者是 getter `0x4EBCB0`（由 `0x4EA3B0` 的 `0x4EA3CD` 调用） |
| `+259956` | `0xAD7AE4` | 1 dword | **指向 207,936 字节临时表的指针**；`0x467800` 用 `0x77BB08` 分配 207,936 字节并存入，`0x467830` 清零 51,984 dword，`0x467850` 用 `0x77BB31` 释放后置 0 |

`+259940..+259951` 这 12 字节在整份 listing 里没有任何引用（被 `0x466950` 的清零范围覆盖，但无显式读写点），本节不为其分配语义。`0x466950` 的清零范围到 `+259951` 为止，所以 `+259952` 与 `+259956` 不由构造函数清零，依赖静态零初始化；native 侧必须显式给这两个字段初值，不能依赖分配器返回零页。

#### 1.3 一次 tick 的精确算法

| 阶段 | 指令 | 行为 |
|---|---|---|
| 前置 | `0x466E96`、`0x466E9B` | `ECX=0xC75090`，调 `0x492430`（场景/事件管理器步），返回值不用 |
| 清聚合 | `0x466EA6` | `byte[obj+259952] = 0` |
| 第一遍 | `0x466E AC..0x466EC5` | 8 次：`slot=[obj+4*i+259920]`；非空则 `0x468030(slot, 0)` → `byte[slot+56]=0` |
| 第二遍 | `0x466EC7..0x466EFD` | 8 次：`slot=[…]`；为空跳过；`byte[slot+56]!=0` 跳过；否则 `r = 0x467680(slot)`，`byte[obj+259952] \|= r`，再 `0x468030(slot, 1)` → `byte[slot+56]=1` |
| 返回 | `0x466F03` | `ret`，无返回值 |

两点容易读错：

1. **`+56` 不是“完成”标志，而是“本 tick 已尝试”闩锁。** 第一遍无条件清 0，第二遍跑完无条件置 1，与 `0x467680` 的返回值无关。因为没有别的东西在这两遍之间写 `+56`，实际效果是**每个非空槽每 tick 恰好跑一次**；“跳过”分支只在槽指针被同一 tick 内新建（由 `0x49CE90` 路径）时才对下一次 tick 生效。native 侧若省掉第一遍清零，第二个 tick 起所有槽都会被跳过。
2. **聚合字节是“本 tick 有槽报告进展”**，只在 `0x466E90` 内被清零与置位。它在 tick 之外被 `0x4EA3B0` 读：`0x4EA3B0`（29 个调用者，如 `0x417A60`、`0x4CDFC4`、`0x4E3F10`、`0x5E41A0`）先按 id 取 registry 对象，若该字节非 0 则调 `0x4EA530(id)`，再按 `[obj+40]` 与 `[obj+18]` 分派。所以 `0x466E90` 的可见输出不是返回值，而是这个字节；把它当局部变量会让所有消费者读到 0。

#### 1.4 槽对象与状态存储

槽对象由 `0x4669D0` 创建（`0x466FC0` 是唯一调用点），构造函数取决于 `0x49CE90` 传下来的模式字节：

| 模式 | 分配大小 | 构造函数 | 备注 |
|---:|---:|---|---|
| 3 | 76 字节 | `0x5A1150` | `0x466B7F` |
| 4 | 60 字节 | `0x467C70` | `0x466BF4` |
| 5 | 60 字节 | `0x4ECB30` | `0x466BB9` |
| 其他 | — | 不创建，返回 0 | `0x466C65` |

基类构造 `0x467020`（4 个栈参数，`ret 16`）设置：`+0` = vtable `0x7AC92C`，`+4`=0，`+8`=0，`+12/+16/+20/+24` = 四个参数，`+28`=0，`+32`/`+33` 两字节 0。tick 里实际用到的字段：

| 字段 | 读者 / 写者 | 语义 |
|---|---|---|
| `+0` | `0x467680`、`0x466F10` | vtable `0x7AC92C`（`+8`=id/类型比较用，`+16`=“可用”判定，`+64`=`0x467130`） |
| `+4`、`+8` | `0x467130`、`0x467700` | 内部记录容器；`+8` 为 0 时 `0x467130` 直接返回 |
| `+24` | `0x467660`（返回它）、`0x467130`（重写它）、构造参数 4 | 当前记录指针；`0x467680` 的第一道门就是它指向的记录 |
| `+40` | `0x467700` | 作为取模上界（`[esi+40]-1` 与 `[esi+28]` 取较小/较大后 `idiv`） |
| `+47`、`+48` | `0x467680` 清零、`0x467FF0` 置位 `+47` | 两个一次性闩锁；非 0 时先调 `0x4B3BD0(0)` / `0x4B3DA0(0)` 再清零。二者读 `0x101D0C4`，其余目标（`0x4B8000`、`0x4B72B0`、`0x408170`）语义 unclear |
| `+56` | `0x468030` / `0x468040` | 本 tick 已尝试闩锁（见 1.3） |

`0x467680`（`0x467680..0x4676FB`，55 条指令）的顺序是：

1. `P = [slot+24]`（`0x467660`），若 `byte[P+24]==0` 立即返回 0（`0x549E30`）。**这是“槽是否有活对象”的总门**：未激活的槽每 tick 都返回 0，且不会执行后面的 vtable 调用。
2. `virtual slot+64`（`0x46769A`）= vtable `0x7AC92C` 第 16 项 = `0x467130`：当 `[slot+4]!=0` 时推进内部记录、重写 `+24` 并对 `+16` 起的结构做三次调用（`0x467CD0`、`0x467D60`×2、`0x467D40`）。
3. `+47` 非 0 → `0x4B3BD0(0)`、清零；`+48` 非 0 → `0x4B3DA0(0)`、清零。
4. `virtual slot+68`（`0x4676C3`）：**目标不可静态解析**。按文件镜像，vtable `0x7AC92C` 在 `+68`（`0x7AC970`）处不是代码地址（值为 `0xEB1C432D`），说明真实槽对象在构造后会用派生类 vtable 覆盖 `[slot]`；`0x467020` 只是基类构造。此处写 `unclear`，不猜目标。
5. 返回 `state(P) ∈ {2,3,4,5}`：`0x4680A0` 取 `[P+20]`，`0x467F60/70/80/90` 分别比较 2/3/4/5。**“进展”就是本 tick 结束时 `P` 的状态落在 2..5。** 状态 7 与 0 都算“未进展”（`0x467FB0` 用 `!=0 && !=7` 判“槽占用”）。

**谁能创建槽**：`0x49CE90`（场景槽所有者步，唯一调用者 `0x49D5C0`）在 `0x49CF57` 用 `ECX=0xA98370` 调 `0x466F10(type=word[obj], mode=byte[obj+2], lo, hi)`；返回索引 `<0` 时把 `word[obj+48]` 置 2（视为失败），否则置 `byte[obj+50]=1`、`word[obj+48]=1`、`word[obj+56]=index`，模式 4 时再走 `0x4BFDB0→0x4C2010` 并写 `byte[0xC7EDAE]`（= `0xC75090+40222`，该字节在 SAV19 窗口里由 `0x493F00` 恢复）。`0x4669B0(obj, index)` 是“该槽是否空闲”的查询（转 `0x467FB0`）。

`0x466F10`（4 参数，`ret 16`）本身是带随机采样的分配器：`arg3<=0` 或 `arg4>=8` 直接返回 -1；否则最多 10,000 次用 `0x4189A0` 取随机数、对候选跨度取模，用栈上 8 字节位图避免重复尝试；命中空槽时调 `0x4669D0` 构造并写入槽数组，命中已有槽时比较 `vtable+8` 与 `arg4` 再调 `0x467510`。返回槽索引或 -1。**`arg3`/`arg4` 的精确语义（下界 / 上界 / 类型）在 `0x49CE90` 的调用点与函数体的 `arg4 - arg3 + 1` 之间不自洽，本节记 unclear。** `0x4189A0` 展开为“推进 `0x4189B0` 后返回 `0x10C7138`”，即槽创建会消耗引擎 RNG 抽取次数——这是 `0x466E90` tick 本身不消耗 RNG、但槽创建消耗 RNG 的分界。

`0x4669D0` 还读取 `0xC82EEC`（月计数，SAV19 已恢复）与地图数组 `0xF6A9E0`（请求 `0x531327`，207,936 字节整块恢复），并在 `0x466B77..0x466B29` 用 228×228 网格做落点搜索。它写回的是 `0x466F10` 的槽位，不写对象 `A` 的其它成员。

#### 1.5 被调函数表

“直接全局”只列函数体内的绝对地址操作数；`[reg+disp]` 对象内写不在此列。依据列给出 `0x466E90` 的调用点或该函数的首个调用者。

| 地址 | 调用者 | 一句话描述 | 直接全局（读 / 写） | 状态 |
|---|---|---|---|---|
| `0x466E90` | `0x5371C9`（唯一） | 8 槽两遍 tick；清 `+259952`、逐槽清/置 `+56`、跑 `0x467680` 并 OR 聚合 | 无 | not ported |
| `0x492430` | `0x466E9B`（唯一） | 场景/事件管理器步：`[0x88EC38]==1` 直接返回；否则按 268 字节步长遍历 `0xC75090+10` 的记录，模式 5 且 `0x425250` 为真时调 `0x4ED3B0`，命中则写 `byte[0xC75090+40220]=1` | 读 `0x88EC38`；写 `[0xC75090+40220]`（对象内） | not ported |
| `0x494250` | 59 处，含 `0x492441`、`0x49247B` | 记录数 getter：`[manager+40208]` | 无 | not ported |
| `0x425260` | 30 处，含 `0x492452`、`0x492462` | 单例 getter，返回 `0x10AF4E0` | 无 | not ported |
| `0x425250` | 13 处，含 `0x492459` | 该单例的 `byte[+8]` getter | 无 | not ported |
| `0x4ED3B0` | `0x492469`（唯一） | unclear：读写 `0x10BC008`；调 `0x408DB0`、`0x4EEB40`、`0x4EEBD0`、`0x4ED5A0`、`0x5403C0`、`0x5AD8F0`、`0x4ED4C0` | 读/写 `0x10BC008` | not ported |
| `0x468030` | `0x466EBC`、`0x466EF4` | setter `byte[ecx+56]=arg`，`ret 4` | 无 | not ported |
| `0x468040` | `0x466ED2` | getter `byte[ecx+56]` | 无 | not ported |
| `0x467680` | `0x466EDD`（唯一） | 单槽步：`P=[slot+24]` 门、`virtual+64`、清 `+47`/`+48`、`virtual+68`、状态 2..5 判定，返回 bool | 无（写 `+47`/`+48`） | not ported |
| `0x467660` | 41 处，含 `0x467686` | getter `[ecx+24]`（记录指针） | 无 | not ported |
| `0x549E30` | 6 处，含 `0x46768D` | getter `byte[ecx+24]`（记录激活标志） | 无 | not ported |
| `0x4B3BD0` | `0x4676A3`（唯一调用） | unclear：读 `0x101D0C4`；调 `0x4B8000`、`0x4B72B0`、`0x408170` | 读 `0x101D0C4` | not ported |
| `0x4B3DA0` | 4 处，含 `0x4676B4` | unclear：同组，调 `0x4B8000`/`0x408170`/`0x4B72B0` 13 次 | 读 `0x101D0C4` | not ported |
| `0x467F60` | 3 处，含 `0x4676C8` | 状态 `==2` 谓词（`0x4680A0`） | 无 | not ported |
| `0x467F70` | 2 处，含 `0x4676D3` | 状态 `==3` 谓词 | 无 | not ported |
| `0x467F80` | 2 处，含 `0x4676DE` | 状态 `==4` 谓词 | 无 | not ported |
| `0x467F90` | 5 处，含 `0x4676E9` | 状态 `==5` 谓词 | 无 | not ported |
| `0x4680A0` | 9 处，含 `0x467F60` | 状态 getter：`[0x467660(ecx)+20]` | 无 | not ported |
| `0x466F10` | `0x49CF5C`（唯一） | 槽分配器：随机采样 + 8 字节栈位图 + 10,000 次上限，返回索引或 -1 | 无（RNG 在 `0x4189A0` 内） | not ported |
| `0x4669D0` | `0x466FC0`（唯一） | 槽负载工厂：228×228 落点搜索后按模式 3/4/5 分配 76/60/60 字节并构造 | 读 `0xC82EEC`、地图基址 `0xF6A9E0` | not ported |
| `0x466950` | 经 `0x466910` 尾跳（构造） | 构造/清零：`+0` 12,996 dword、`+51984` 51,984 dword、`+259920` 8 dword | 无 | not ported |
| `0x466E00` | `0x466C9F`（析构路径） | 同范围清零后释放 8 个槽（`0x468020`、`0x664C70`、`virtual [eax]` push 1），槽指针置 0 | 无 | not ported |
| `0x467800` | `0x46783E` | 分配 207,936 字节到 `[0xAD7AE4]`，返回是否成功 | 写 `0xAD7AE4` | not ported |
| `0x467830` | `0x467839` | 确保临时表存在后清零 51,984 dword | 读 `0xAD7AE4` | not ported |
| `0x467850` | 2 处，含 `0x466DDF` | 释放临时表并把 `0xAD7AE4` 置 0 | 读 `0xAD7AE4` | not ported |
| `0x49CE90` | `0x49D5C0`（唯一） | 场景槽所有者步：按 `word[obj]`/`byte[obj+2]` 申请槽、写 `+48/+50/+56`、模式 4 时写 `byte[0xC75090+40222]` | 写 `0xC7EDAE`（`+40222`） | not ported |

`0x492430` 的 268 字节记录步长来自 `0x492475` 的 `add edi, 268`，记录 0 从 `0xC75090+10` 开始，计数来自 `[0xC75090+40208]`；`10 + 150×268 = 40210`，所以最多 150 条记录，`+40212` 之后才是 `0x493F00` 序列化的 5 个标量字段。这一层只描述结构，不推断记录字段语义。

#### 1.6 SAV19 是否恢复 `0xA98370`

结论：**不恢复。** 三条独立证据：

1. **读请求窗口内没有该范围。** `0x5311DC..0x531873` 共 91 次 `call 0x780533`，其中 89 次带 `push size; push imm` 立即数对（另 2 次压寄存器，不计）。89 个立即数地址里没有一个落在 `[11109232, 11369191]`；窗口中最接近的低位请求是 `0x5315AD` 的 8,960 字节 @ `0xA4F590`，高端请求是 `0x531327` 的 207,936 字节 @ `0xF6A9E0`，都不覆盖 `0xA98370..0xAD7AE7`。
2. **整份 listing 没有指向槽数组或聚合字节的立即数。** `0xAD7AC0`（=11,369,152）到 `0xAD7ADF`（含 8 个槽指针）以及 `0xAD7AE0`（聚合字节）作为立即数地址出现 **0 次**。对象体内只有两个绝对引用点：`0xA98370` 本身 18 处（构造/析构/`cmp ecx,` 判定/6 个业务入口），`0xAD7AE4` 7 处（全在 `0x467800`/`0x467830`/`0x467850` 与 `0x467900` 内）。也就是说：即便存档想恢复槽，也没有任何代码路径以立即数地址点名这些字段。
3. **窗口调用集到引用者的可达性为空。** 窗口的 25 个直接调用目标是 `0x4081F0`、`0x408C60`、`0x408C70`、`0x425260`、`0x426A60`、`0x42D790`、`0x480740`、`0x4825F0`、`0x482600`、`0x482680`、`0x48DF30`、`0x493F00`、`0x4E07C0`、`0x4E1E40`、`0x506240`、`0x510E60`、`0x52C110`、`0x52CD90`、`0x52E7B0`、`0x52FA30`、`0x52FD90`、`0x5501B0`、`0x55A430`、`0x593140`、`0x780533`。从这 25 个目标出发做直接调用 BFS 到深度 3（深度 1 新增 66 个、深度 2 新增 36 个、深度 3 新增 192 个函数），**没有一条路径命中**引用 `0xA98370` 的 16 个函数入口（`0x4668E0`、`0x4668F0`、`0x466900`、`0x466910`、`0x466980`、`0x466C80`、`0x467900`、`0x46F9B0`、`0x4715C0`、`0x49CE90`、`0x4EA3B0`、`0x4ECD90`、`0x534410`、`0x534BF0`、`0x5355F0`、`0x5371A0`）。

**唯一未排除的路径**是窗口里的 7 次间接分派：`0x531213` 的 `call [edx]` 与 `0x53122A/0x531237/0x531244/0x531251/0x531274/0x531281` 的 6 次 `call [edx+8]`。它们的接收者是 BSS 单例（`0x1158E18`、`0x10AF4E0`、`0xC701D8`、`0x1312F08`、`0x10BC910`、`0xC058D0` 等），vtable 由构造时写入，文件镜像只到 `0x499000`，无法静态解析。严格的表述是：**槽状态在窗口内未被恢复；除非某条未解析的虚分派间接把 `0xA98370` 交给反序列化器，否则它完全是进程期状态。** 走 `0xA98370` 的 6 个业务入口里没有序列化形状的函数（`0x49CE90` 是场景槽所有者 tick、`0x4EA3B0` 是查询、`0x534xxx` 是 UI/查询路径），所以这条残余路径的可能性很低，但本节不把它写成“已证明为 0”。

**相邻但不同的对象：`0xC75090` 场景管理器确实被恢复。** 窗口在 `0x5312EC` 用 `ECX=0xC75090` 调 `0x493F00(flag, stream)`，该函数同时是读出与写入两个方向（`0x493F34` 判 flag 后分成 `0x780642` 写分支 `0x493F40` 与 `0x780533` 读分支 `0x493FB6`），按顺序处理 6 个成员：

| 成员 | 大小 | 读请求 | 说明 |
|---|---:|---|---|
| `+40212` | 4 | `0x493FC1` | dword |
| `+40216` | 4 | `0x493FD1` | dword |
| `+40220` | 1 | `0x493FE1` | 字节；`0x492430` 在 `0x492488` 写它 |
| `+40221` | 1 | `0x493FF1` | 字节 |
| `+40222` | 1 | `0x494001` | 字节；`0x49CE90` 在 `0x49CF93` 写它 |
| `+40224` | — | `0x494013` | 子对象，调用其 `[vtable+8]`（未解析） |

这 6 个请求用的是 `lea reg,[edi+disp]` 形式，**不在这 89 个 `push imm` 请求里**，这正是 D2 提醒过的“窗口不是唯一读取点”。它同时说明：`+40208` 记录计数与 `+10..+40207` 的 150×268 记录数组**不是** `0x493F00` 的直接成员；记录数组唯一的整块搬运是赋值运算符 `0x494040`（`rep movsd` 10,050 dword，从源 `+4` 搬到目标 `+8`，并把 `[src+0]` 写入 `[dst+40208]`）。记录数组到底由谁序列化，本节记 **unclear**。

#### 1.7 能否只靠六份 SAV19 输入跑

**不能。** 缺的是这些输入，且没有一项能从存档字节推出：

1. 8 个槽指针、聚合字节 `+259952`、临时表指针 `+259956`——窗口零覆盖（1.6 第 1、2 条）。
2. 门字节 `0x88EC38`——不在窗口内；它是会话模式，全 listing 至少 14 个写者（`0x401E24`、`0x42E473`、`0x42E508`、`0x42F713`、`0x430EFD`、`0x431372`、`0x475765`、`0x4EC2C6`、`0x4FB60C`、`0x51686C`、`0x5168D9`、`0x536DAF`、`0x536DF3`、`0x586B86`），其中 `0x431372`、`0x4EC2C6`、`0x51686C`、`0x5168D9`、`0x536DF3` 写 0。native 侧必须由“进入城市模式”这件事显式给 0，不能假设存档给了它。
3. 槽创建所需的 RNG 输出 `0x10C7138`（`0x4189A0` 的返回值）与引擎 RNG 状态——只有 `0x10C7148`/`0x10C714C` 两个寄存器在窗口内，`0x10C7138` 不在。因此**槽的索引分配是不可从存档重建的**：同一份存档在 RNG 不同时会把事件放进不同槽位，进而改变 `0x467680` 的执行顺序与 `+259952` 的取值。
4. 管理器 150×268 记录数组的序列化责任不明（1.6 末段）。若它来自存档，槽内容可恢复；若不来自，则连“哪些事件存在”都只能由场景脚本重建。

可跑的下限：给对象 `A` 一个全零实例（8 个空槽）时，`0x466E90` 本身可以在每个 tick 正常返回：`0x492430` 只需要 `0x88EC38` 与已恢复的管理器标量，两遍循环都空转，`+259952` 恒 0。这只能证明“空槽 tick 的边界”，**不能当作该步已移植**——它没有跑过任何一个槽。

### 2. `0x564B50` — figure 之后的周期步骤

#### 2.1 调用位置与门

`0x564B50` 的唯一入口是 `0x5371DC` 的尾跳（`jmp`，不是 `call`），因此它继承 `0x5371A0` 的返回地址，自身 `ret` 即返回 `0x536BC0` 的调用者。`0x5371D3` 再次读 `[0x88EC38]`，非 0 时 `0x5371DA` 直接 `ret`，**连这一步都不进**；`0x5371CE` 的 figure pass 则不受这个门影响。函数体是 `0x564B50..0x564C50`（96 条指令），`0x564C51..0x564C5F` 为填充。

[SAV.md](SAV.md):798 把这一跳命名为 “post-figure monument work”，A8 沿用该命名。**业务身份仍记 unclear**：函数体给出的硬事实是“按类型白名单的周期池扫描”，见 2.3；本节不去证明它就是纪念物。

#### 2.2 精确算法

| 阶段 | 指令 | 行为 |
|---|---|---|
| 计数 | `0x564B50`、`0x564B55`、`0x564B5B`、`0x564B5C`、`0x564B5E` | `n = [0x12A6E78] + 1`；`[0x12A6E78] = n`；若 `n <= [0x859DA0]` 则 `ret`（`0x564C50`） |
| 初始化 | `0x564B69..0x564B7E` | `bl=0`（进展标志）、`edi=1`（槽索引）、`count = 0x554C00(0x8C7634)`；`count <= 1` 时跳到收尾 |
| 循环 | `0x564B86..0x564C24` | `edi` 从 1 到 `count-1`，见下 |
| 收尾 | `0x564C2D..0x564C44` | `[0x12A6E78]=0`；`[0x859DA0] = bl ? 50 : 30`；`ret` |

循环体（每个槽）：

1. `obj = *0x47F1B0(0x8C7634, edi)`（升序槽）。
2. `type = (i16)[obj+20]`（`movsx`）。
3. `0x562F70(type)` 为假 → 跳到下一个槽（2.3 的白名单）。
4. `(i16)[obj+22] != 0` → 跳过。
5. `0x426D10(obj, 0)` 为假 → 跳过（live 必须 ∈ {1,3}）。
6. `rec = 0x5653C0(obj, 1)`：按 `type` 在 `0x12A5DD0` 起的 144 字节记录数组里查第一条 `[rec+4]==type` 的记录，返回记录基址或 0。
7. `rec != 0` 且 `byte[0x10DE0D0]==0` 且 `byte[0x10DE045]!=0x56` 时：调 `0x56C880(rec,1)`；再调 `0x56D170(rec,1)`，返回真则 `bl=1`。否则整段跳过。`0x10DE0D0` 是暂停字节、`0x10DE045` 是模式字节（[SAV_FIGURES.md](SAV_FIGURES.md):710），即**暂停或模式为 86 时只跳过实体更新，不跳过计数与白名单**。
8. `type == 76 || type == 77` → 调 `0x564EA0(obj, 1)`。
9. `edi += 0x567610(type)`：按 `type` 在 `0x12A5DD4` 起的同一数组里查 `[rec+0]==type`，返回 `[rec+12]`；循环尾部再 `inc edi`，净效果是**按该类型的计数整段跳过**。

第 9 步是一个实现风险点：`0x567610` 返回 0 时 `edi` 不前进，原版会原地重复。原版不挂起说明“凡通过白名单的类型在记录数组里必有 `[rec+12] >= 1` 的条目”，但这是数据前提而不是代码保证。native 侧如果照抄 `edi += k` 而不对 `k==0` 做显式失败，遇到缺表就会死循环；正确做法是把 `k==0` 当成命名拒绝（cannot index type），而不是 `k=1` 的静默修正。

#### 2.3 类型白名单 `0x562F70`

`eax = type - 76`；`>192`（即 `type<76` 或 `type>268`）返回假；否则取 `byte[0x562F9C + (type-76)]` 作为索引跳 `0x562F94` 的两项表：**索引 0 → `0x562F8D` 返回真，索引 1 → `0x562F90` 返回假**。表 193 字节，只有 0/1 两种值，0 出现在 29 个位置：

| 通过（返回真）的类型 | 数量 |
|---|---:|
| 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86 | 11 |
| 92, 93 | 2 |
| 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268 | 16 |
| **合计** | **29** |

被拒的是 87..91（5 个）与 94..252（159 个）。所以这一步的扫描面是 **29 个类型、268 个槽位中的一段**，不是“所有 figure”。`76/77` 在第 8 步还有单独的 `0x564EA0` 处理，与白名单一致（可被到达）。

#### 2.4 直接被调函数表

| 地址 | 调用者 | 一句话描述 | 直接全局（读 / 写） | 状态 |
|---|---|---|---|---|
| `0x564B50` | `0x5371DC`（尾跳，唯一） | 周期池扫描：30/50 两档周期、29 类型白名单、按类型整段跳过 | 读 `0x12A6E78`、`0x859DA0`、`0x10DE0D0`、`0x10DE045`；写 `0x12A6E78`、`0x859DA0` | not ported |
| `0x554C00` | 389 处 call + 9 处 jmp，含 `0x564B77`、`0x564C1D` | 向量 count：`([ecx+8]-[ecx+4])/4`，空指针返回 0 | 无 | partial（仅作为已移植循环长度） |
| `0x47F1B0` | 4,107 处，含 `0x564B8C` | registry 槽：`begin + 4*id`，`ret 4` | 无 | ported（[SAV.md](SAV.md):625） |
| `0x562F70` | 58 处，含 `0x564B98` | 29/193 类型白名单（2.3） | 无（表是立即数基址） | not ported |
| `0x426D10` | 207 处，含 `0x564BAF` | live 谓词：`byte[ecx+4] ∈ {1,3}`，`ret 4` | 无 | ported（[SAV_FIGURES.md](SAV_FIGURES.md):786、875） |
| `0x5653C0` | 13 处，含 `0x564BBB` | 按 owner 类型查 `0x12A5DD0` 的 144 字节记录数组，匹配 `[rec+4]`，返回记录基址或 0 | 读 `0x12A6E7C` | not ported |
| `0x56C880` | `0x564BDB`（唯一调用） | unclear：`0x56A910`/`0x564E80`/`0x56A940` 后遍历 registry 向量，含 `0x56BB40`、`0x563550`、`0x4F8200`/`0x4F8210`；读 IAT/vtable `0x7AC8C0` | 无 SAV 全局；`0x7AC8C0` 是 IAT |
| `0x56D170` | `0x564BE3`（唯一调用） | unclear（含 SEH 帧）：`0x47F1B0`、`0x56CF40`、`0x56F9E0`、`0x56F9B0`、`0x56D0F0`、`0x56B8C0..0x56B910`、`0x563550`；返回 bool | 无 | not ported |
| `0x564EA0` | `0x564C01`（唯一调用） | unclear：`0x480740→0x4812E0`、`0x564E90`、`0x567610`、`0x5635A0`、`0x7650FC`；对类型 76/77 且 `[rec+20]==4` 才继续 | 无 SAV 全局；`0x7ACA28`、`0x7AE110`、`0x7B8A80..0x7B8A98` 是 IAT/vtable | not ported |
| `0x567610` | 28 处，含 `0x564C0B` | 按类型查 `0x12A5DD4`，匹配 `[rec+0]`，返回 `[rec+12]`（跳过计数） | 读 `0x12A6E7C` | not ported |

#### 2.5 全局写与序列化

| 全局 | 角色 | 写者 | 读者 | SAV19 | 说明 |
|---|---|---|---|---|---|
| `0x12A6E78` | 距上次扫描的 tick 计数 | `0x564B50`（`0x564B5E` 自增、`0x564C2D` 归零） | 同函数 `0x564B50` | 窗口内无请求 | 全 listing 只有 3 处引用，全在本函数内；纯进程期计数器 |
| `0x859DA0` | 当前周期（30 或 50） | `0x564B50`（`0x564C39` 写 30、`0x564C46` 写 50） | 同函数 `0x564B55` | 窗口内无请求 | 同上，只有 3 处引用 |
| `0x10DE0D0` | 暂停字节 | 多处（如 `0x42E51C`、`0x4761C3`、`0x476347`、`0x477256`、`0x534F1A`） | 本函数 `0x564BC6`、`0x53A198` 等 | 窗口内无请求 | 会话状态，不是存档字段 |
| `0x10DE045` | 模式字节（`0x56`=86 时跳过实体更新） | 多处（`0x4012BD`、`0x40153E`、`0x401BEA`、`0x401C1C`、`0x401E1E`…） | 本函数 `0x564BCF`、`0x53A173` 区段 | 窗口内无请求 | 会话状态，不是存档字段 |
| `0x12A6E7C` | 144 字节记录数组的条数 | `0x56488B` 写 0、`0x567683` 写回 | `0x563630`、`0x5653D4`、`0x567610`、`0x567650` | 窗口内无请求 | 表由 `0x561B31`/`0x561B6A` 一带构建，基址 `0x12A5DD0` 亦无请求 |
| `0x12A5DD0` | 144 字节记录数组基址 | 表构建路径 | `0x532036`、`0x564E36`、`0x564E61`、`0x5653E2`、`0x567695` | 窗口内无请求 | 每类型一条，`+0` 与 `+4` 分别是两套键，`+12` 是跳过计数 |

**序列化结论：`0x564B50` 自己写的两个全局（`0x12A6E78`、`0x859DA0`）在整个 listing 里各只出现 3 次，全部在本函数体内，SAV19 窗口没有任何请求指向它们。** 读侧的两个会话字节（`0x10DE0D0`、`0x10DE045`）同样不在窗口内。所以这一步**没有任何“本步私有”的存档状态**：它的全部输入是 (a) 两个进程期计数器、(b) 两个会话字节、(c) registry `0x8C7634` 的对象、类型与 live 标志、(d) 两张 `0x12A5DD0` 记录表。其中 (c)(d) 的所有权属于 figure/建筑与场景表，不属于本步。

#### 2.6 能否只靠六份 SAV19 输入跑

**不能只靠存档，但缺口比 `0x466E90` 小，且缺口都在“会话初始化”一侧而不是“本步状态”一侧。**

- 缺 `0x88EC38`：与 1.7 同一问题，必须先由会话给 0，否则这一步根本不被进入。
- 缺 `0x10DE0D0`/`0x10DE045` 的初值：两者不在窗口内。若不显式给，暂停/模式判定会取进程里的残留值，扫描可能被整段跳过。
- 缺 `0x12A6E78`/`0x859DA0` 的初值：二者在 BSS 里天然为 0，于是加载后第一次 admitted tick 就有 `1 > 0`，扫描立即发生；这与原版“距上次扫描 30 tick”的关系不同。必须由会话决定“加载后第一次扫描在第几 tick”，不能靠零初值默认成立。
- 缺 `0x12A6E7C` 与两张记录表：它们由 `0x561B31` 一带的构建路径填充，来源是 figure/场景数据而不是本步。若这部分数据已经从存档恢复（figure 池 `0x4E27E0` 一侧的口径），本步可以直接消费；若没有，本步会扫到 `rec==0` 而全部跳过。
- 有机部分：registry `0x8C7634`、`0x47F1B0`、`0x426D10`、`0x554C00` 都已有 ported/partial 实现，`0x562F70` 是纯常量表，`0x5653C0`/`0x567610` 是纯查表。**这意味着 `0x564B50` 的骨架可以在没有任何原存档状态的前提下先跑起来并正常返回**，只是三个 unclear 子步（`0x56C880`、`0x56D170`、`0x564EA0`）必须显式拒绝而不是返回“成功”。

### 3. 状态计数

| 步骤 | 函数行 | ported | partial | not ported | 判据 |
|---|---:|---:|---:|---:|---|
| `0x466E90` | 26 | 0 | 0 | 26 | 该子图在 SAV.md / SAV_FIGURES.md 中除 `0x466E90` 被点名为“被隔离前提排除”外无任何实现证据 |
| `0x564B50` | 10 | 2 | 1 | 7 | ported：`0x47F1B0`（[SAV.md](SAV.md):625 “resolved by 0x47F1B0”，与 `game_lab/original_building.hpp` 的 owner-slot 解析一致）、`0x426D10`（[SAV_FIGURES.md](SAV_FIGURES.md):910 “building live1/3 (`0x426D10`)” 与 :999 “validates home+64 via `0x426D10`(live1/3)”）；partial：`0x554C00`（仅作为已移植循环长度） |

两个步骤合计 36 行：**ported 2（5.6%）、partial 1（2.8%）、not ported 33（91.7%）**。`0x466E90` 一侧是 0/26——它的 26 个函数里没有任何一个能在三份文档里找到实现证据；`0x564B50` 一侧的 2 个 ported 项都是它调用的通用 registry/谓词叶函数，不是它自己的语义。

### 4. 推荐移植顺序

先做 `0x564B50`，后做 `0x466E90`。理由是缺口结构而不是重要性：前者没有本步私有的存档状态、子调用里已有 2 个 ported + 1 个 partial，骨架先能正常返回；后者需要先自造一个“零槽对象”才能返回，而且槽内容的创建依赖不可从存档重建的 RNG。

1. **`0x564B50` 骨架 + 计数器**：两档周期（30/50）、`+1 > period` 的判定、收尾归零。判据是 n 从 0 起算的轨迹 `1,2,…,30,1,…`，以及 `bl=1` 后周期变 50。此时白名单与实体更新都还没接，允许显式拒绝。
2. **`0x562F70` 白名单**：193 字节表 + 2 项跳表，用独立重算断言 29 个通过类型与 165 个拒绝类型（`type-76` 的 193 个索引全部覆盖）。
3. **`0x5653C0` / `0x567610`**：两张查表。必须显式处理 `0x567610` 返回 0：报命名拒绝，**不要**把 `edi += 0` 静默改成 `+1`，否则会掩盖数据缺口并改变扫描顺序。
4. **`0x564EA0`（类型 76/77）**：它是白名单里唯一有独立分支的类型，且内部还要求 `[rec+20]==4`。先把 `0x480740→0x4812E0` 与 `0x5635A0` 的语义查清再接线，否则保持拒绝。
5. **`0x56C880` / `0x56D170`**：两者的返回语义（尤其 `0x56D170` 的 bool 决定周期 30/50）必须先确定，否则周期会永远停在 30。`0x56D170` 带 SEH 帧与 `0x56F9xx` 一族，属于最重的一块，建议最后接。
6. **`0x466E90` 常量骨架**：`0x468030` / `0x468040`（各 2 条指令的 setter/getter）+ 两遍 8 槽循环 + `+259952` 聚合。在 8 个槽全空时应当每 tick 正常返回且聚合恒 0。这是唯一能“无槽先跑”的边界。
7. **`0x467680` 的只读部分**：`0x467660`、`0x549E30`、`0x4680A0`、`0x467F60/70/80/90`。此时可以判定“槽是否激活 + 状态是否 2..5”，还不涉及 `virtual+64/+68`。
8. **`0x492430`**：它只需要已恢复的管理器标量（`+40208`、`+40212`、`+40216`、`+40220`、`+40221`、`0xC7EDAE`）与会话字节 `0x88EC38`，是 `0x466E90` 一侧最容易闭合的子步。`0x4ED3B0` 必须一起接，否则模式 5 的记录会漏掉 `0x10BC008` 的写。
9. **`0x467680` 的写部分 + `0x4B3BD0`/`0x4B3DA0`**：`+47`/`+48` 两个闩锁与它们对 `0x101D0C4` 的读；这一块必须先确认 `0x4B8000`/`0x4B72B0`/`0x408170` 是否只读 UI 状态。
10. **槽创建链 `0x466F10` / `0x4669D0` / `0x49CE90`** 最后接。它同时需要：引擎 RNG（`0x4189A0`→`0x10C7138`）、地图块 `0xF6A9E0`、月计数 `0xC82EEC`、三个模式对应的负载类（`0x5A1150`/`0x4ECB30`/`0x467C70`）。在这条链接通之前，任何“`0x466E90` 已移植”的说法都不成立，因为它从未跑过非空槽。

每步的验收标准仍是“在原返回点正常返回且状态可解释”，而不是返回 0 / 空 stub。第 5 步之前不建议宣布 `0x564B50` 完成，因为 `0x56D170` 的返回值直接改变周期长度；第 10 步之前不建议宣布 `0x466E90` 完成，因为空槽 tick 不覆盖该函数的任何一个真实分支。

### 5. 本节保留的 unclear

| 项 | 为什么不能确定 |
|---|---|
| 槽对象 `virtual+68` 的目标 | vtable `0x7AC92C` 在 `+68`（`0x7AC970`）处不是代码地址，真实 vtable 是构造后由派生类写入；`0x467020` 只是基类构造，文件镜像里没有第二个写 `[obj]` 的点 |
| `0x466F10` 的 `arg3`/`arg4` 语义 | 调用点按 `word[obj]`、`byte[obj+2]`、两个 `0x4A9B90` 输出压栈，函数内既当索引跨度又当 `vtable+8` 比较值，两种读法不自洽 |
| `0x4669D0` 的落点搜索与三个负载类 | 228×228 网格 + `0xF6A9E0` 地块过滤可读，但“找到落点后负载对象代表什么”没有结构证据 |
| `0x492430` 模式 5 分支与 `0x4ED3B0` 的作用 | 只能确认“模式 5 且 `0x425250` 真时调 `0x4ED3B0`，返回真则写 `+40220`”，业务含义不清 |
| `0x56C880` / `0x56D170` / `0x564EA0` | 三者都有大量间接调用与 SEH 结构；`0x56D170` 的 bool 决定 30/50 周期，其判据未定 |
| 29 个通过类型的业务身份 | 白名单是硬事实，`76/77` 有独立分支也是硬事实；但“这些类型是纪念物/动物/其它”没有代码证据，SAV.md 的 “monument” 命名未被本节证实 |
| 150×268 记录数组的序列化责任 | `0x493F00` 只序列化 6 个标量/子对象；记录数组唯一的整块搬运是赋值运算符 `0x494040`；谁把它接到流上没有静态证据 |
| 窗口内 7 次间接分派 | 接收者是 BSS 单例，vtable 运行时写入，文件镜像止于 `0x499000`，无法静态解析 |

## `0x564B50` port status（2026-10-21）

本节只记录“剩余两步的深度映射”§4 第 1、2 步的落地结果：`0x564B50` 骨架（30/50 计数）与 `0x562F70` 白名单。第 3–10 步（`0x5653C0`/`0x567610`/`0x564EA0`/`0x56C880`/`0x56D170` 与 `0x466E90` 一侧）没有实现，5 个未移植被调者在骨架里是调用者提供的 provider；provider 缺失即命名拒绝，不猜返回值。本节不新增测量、不执行原程序，也不移动任何数据指标。

**白名单表（已逐字节核对）。** 表在 VA `0x562F9C` = 文件偏移 `0x162F9C`（VA−`0x400000`），193 字节，覆盖类型 76..268。读回的值只有 0 与 1：**29 个 0（通过）、164 个 1（拒绝）**。两项分派表在 `0x562F94`：项 0 指向 `0x562F8D`（`al=1` 返回），项 1 指向 `0x562F90`（`al=0` 返回）。判定链为 `0x562F74`（减 76）、`0x562F77`（与 192 无符号比较）、`0x562F7C ja`（越界直接拒绝）、`0x562F80`（取表字节）、`0x562F86`（分派）。通过类型是 76..86、92..93、253..268；拒绝类型是 87..91 与 94..252。§4 第 2 步的“165 个拒绝类型”是算术笔误，193−29 = 164。

**计数与循环（指令地址）。** `0x564B5B` 增 1、`0x564B5E` 先写回、`0x564B5C`/`0x564B63` 用**有符号** `jle` 与 `0x859DA0` 比较，因此 `0xFFFFFFFF+1`、`0x7FFFFFFF+1` 以及最高位为 1 的 period 都按补码语义判定。收尾是 `0x564C2D` 归零、`0x564C39` 写 30、`0x564C46` 在进展标志为真时改写 50。周期语义是“距上次扫描的调用数 + 1”：period 30 时第 31 次调用扫描，period 50 时第 51 次；每次 due 的扫描都会重写 30/50，所以初值 50 在无进展时会塌回 30。循环边界来自 `0x564B77`/`0x564C1D`（`0x554C00` 计数）、`0x564B7C`/`0x564B7E jbe`（count ≤ 1 直接走收尾，但仍归零并写周期）、`0x564C22`/`0x564C24 jb`。单槽顺序与两类分支见 §2.2：白名单、`(i16)[obj+22]`、live∈{1,3} 三种拒绝跳到 `0x564C17`（只 +1，不做按类型跳过）；记录缺失、暂停字节 `0x10DE0D0`、模式 `0x10DE045`=86 跳到 `0x564BEE`，仍会跑 `0x564EA0` 与 `0x567610`。`0x567610` 返回 0 时原版会原地重复，骨架把它当作命名拒绝，而不是静默 +1。

**会话输入（均非存档字段）。** `0x12A6E78` 与 `0x859DA0` 在全 listing 各只出现 3 次，且全在本函数体内；`0x10DE0D0` 16 次、`0x10DE045` 391 次、门字节 `0x88EC38` 182 次引用，均是多写者的会话状态。SAV19 读窗口 `0x5311DC..0x531873` 共 91 次 `call 0x780533`，对上述五个地址的检索结果为 0。因此这五项（门字节、暂停字节、模式字节、两个计数器初值）是输入结构的必填字段、没有默认值，缺失即 `refused_missing_session_input`；库内不读环境、时钟或文件。

**命名拒绝。** `skipped_gate_88ec38`（门非 0，调用者 `0x5371D3` 本就不进入）、`refused_missing_session_input`、`refused_null_registry_slot`（`0x564B91` 无检查解引用）、`refused_unported_5653c0`（`0x564BBB`）、`refused_unported_56c880`（`0x564BDB`）、`refused_unported_56d170`（`0x564BE3`）、`refused_unported_564ea0`（`0x564C01`）、`refused_unported_567610`（`0x564C0B`）、`refused_567610_zero_advance`（`0x564C13` 的零跳过）。拒绝发生在计数写回之后、收尾之前，所以计数器保留已增的值、周期不变；重复调用仍然 due，不会掩盖缺口。

**实现与验收。** [original_post_figure_pass.hpp](../game_lab/original_post_figure_pass.hpp) 含 193 字节 `constexpr` 表（注释给出 VA 与文件偏移）、`originalPostFigureWhitelisted`、30/50 计数骨架与全部拒绝状态；[original_post_figure_pass_tests.cpp](../game_lab/original_post_figure_pass_tests.cpp) 覆盖 29/164 计数、每个通过区间边界、越界与类型 0/负值、计数器回绕与有符号比较、两条 30/50 轨迹、每条拒绝路径与必填字段缺失；[scripts/post_figure_pass.py](../scripts/post_figure_pass.py) 以 release 与 `--mode undefined`（UBSan）构建运行。当前结果：两种模式均为 `{"passed":true,"authored_checks":656}`，不读游戏数据。这只关闭 §4 第 1、2 步；本步仍会在第一个白名单内的 live 对象上按 provider 缺失拒绝，`0x466E90` 完全未动，因此不能记为该函数或该阶段已移植。

### `0x5653C0` / `0x567610` / `0x564EA0` / `0x56C880` / `0x56D170` 的落地（2026-10-22）

本节继续记录 `0x564B50` 骨架的 5 个命名拒绝，但不新增测量、不执行原程序、不移动数据指标；证据仍来自同一个自有可执行文件（SHA-256 见文件头，镜像基址 `0x400000`，VA = 文件偏移 + `0x400000`）。

**`0x5653C0` 与 `0x567610`：整体移植。** 两者只读同一张表：记录基址 `0x12A5DD0`，记录步长 144 字节，键在 `+4`（`0x12A5DD4`），跳过计数在 `+12`（`0x12A5DDC`），项数在 `0x12A6E7C`。项数按有符号判定，至多 0 时分别回答空指针（`0x565401`）与 0（`0x567635`）。`0x5653C0` 用对象的类型词作键，命中即停、回答第一条匹配记录；`0x567610` 用同一个键回答 `[rec+12]`。表的容量是 27（`0x567675` 拒绝第 28 条），由运行期注册构造（`0x567650` 复用同键记录、`0x56B270` 初始化新记录；`0x56488B` 先把项数归零），因此存档不能提供这张表。SAV19 读窗口 `0x5311DC..0x531873` 的 91 次读取请求中，没有任何一次点名 `0x12A5DD0`、`0x12A5DD4`、`0x12A5DDC` 或 `0x12A6E7C`。新增 `OriginalPostFigureRecord` / `OriginalPostFigureRecordTable` 与两个函数 `originalPostFigureFindRecord0x5653C0`、`originalPostFigureSkipAdvance0x567610`；空表就是“项数至多 0”，返回 0 或空，负的 `+12` 原样返回。

**跳过计数是有符号的。** `0x564C13` 把 `0x567610` 的答案减 1，`0x564C1C` 的公共步进再加 1，净效果就是答案本身，所以负答案让游标向回走。游标按 `uint32_t(int32_t(cursor) + advance)` 更新：负值回退后必然落到大于等于槽数的无符号区间，由 `0x564C22` 的边界判定结束扫描；答案为 0 时保持 `refused_567610_zero_advance`，不把缺表静默改成步进 1。

**`0x564EA0`：控制流整体移植。** 入口是 `index <= 0`（有符号，`0x564EDB`）直接离开；相对索引是 `index - word[obj+22]`（`0x564EF0`）；随后依次要求链接对象的状态字节 `+96` 为 0（`0x564F1A`）、`0x564E90` 为真（`0x564F27`）、链接对象类型为 76 或 77（`0x564F31`）、状态 dword `+20` 等于 4（`0x564F4D`），任一不满足都不写任何字段。候选数量来自 `0x567610(链接类型)`（`0x564F57`），至多 0 时直接进入提交判定，只写标记。候选扫描覆盖 `index .. index+count-1`，五种 `(dword+4, dword+8)` 形状对应资源 42/44、226、46、47、43 与类型码 21..26；两个失败分支的提交表在 `0x565384` 与 `0x56539C`，类型码 21→`+76`、22/23→`+64`、24→`+68`、25→`+72`、26→`+80`。转换常量从文件读回：`0x7B8A98`=700.0、`0x7B8A90`=1100.0、`0x7ACA28`=400.0、`0x7AE110`=1000.0、`0x7B8A88`=600.0、`0x7B8A80`=900.0；“类型 76”分支始终读参数对象自身的类型（`0x564FB2`、`0x565154`、`0x5651BE`），链接类型只负责入口门与候选数量。比较是严格有符号，所以平局由先到者胜；只要出现过不可用候选就优先提交不可用组，否则提交可用组；任何提交（含无类型码路径 `0x565371`）都写链接状态的 `+96` 字节为 1。五个外部问题（vtable 槽 `+0x1EC` 的状态、`0x5635A0` 的邻居记录、`0x4812E0` 标志、`0x564E90` 门、`0x402A50` 可用性）都是显式必填 provider，没有默认值；`OriginalPostFigureType7677State` / `Neighbour` / `Inputs` 记录这些字段。

**`0x56C880` 与 `0x56D170`：精确前缀移植，容器遍历仍拒绝。** `0x56C880` 先无条件写 `0x56A910` 的 `+16..+40` 七元组 `2,3,8,7,5,3,3`（`0x56C88B`），`0x564E80` 为真时改写为 `0x56A940` 的 `4,6,16,14,10,6,6`（`0x56C89B`），然后才读 `registry[index]`（`0x56C8AA`）并计算相对索引 `index - word[obj+22]`；至多 0 直接在 `0x56C8BD` 离开，答案是完整结论。`0x56D170` 有相同的入口（`0x56D19E`、`0x56D1A5`），早退路径把答案写成假并在 `0x56D4AB` 返回，除此之外不写任何字段。两者的 `+44/+48/+64/+96/+128` 容器遍历及其下层调用仍然语义不明，保持命名拒绝；由于骨架在 `0x564BA4` 已要求 `word[obj+22]==0`，从骨架进入时相对索引恒为正，早退分支只能由直接调用触发。新增 `OriginalPostFigureRecordScalars`、`originalPostFigureUpdateRecord0x56C880`、`originalPostFigureStepRecord0x56D170`，以及窄化后的状态 `refused_56c880_containers` / `refused_56c880_contract` / `refused_56d170_containers` / `refused_56d170_contract`；`updateRecord0x56C880` 的 provider 与 `stepRecord0x56D170` 的 provider 仍然优先，provider 与移植输入都不在时保留旧名 `refused_unported_56c880` / `refused_unported_56d170`。

**骨架新增必填输入。** `OriginalPostFigureRecordTable`（`0x12A5DD0` 数组；只建模键与 `+12` 两个字段）、`OriginalPostFigureType7677Inputs` 的 5 个 provider（均无默认值）、`updateProfile0x56C880`（含 `0x564E80` 门与记录 `+16..+40` 标量目标，均非存档字段）、以及 `stepPrefix0x56D170` 选择开关（`0x56D170` 前缀只读 registry，因此它是接线开关而不是存档状态）。骨架仍不读环境、时钟或文件。

**实现与验收。** [original_post_figure_pass_tests.cpp](../game_lab/original_post_figure_pass_tests.cpp) 新增记录表、`0x564EA0` 与两条前缀的检查：查表首命中与空表、负 `+12`、五类候选形状与两张提交表、平局、不可用优先、`+96` 标记、两组七元组与严格倍增、相对索引 0/负/正三态、越界与空槽、五个缺失 provider 的分支、以及骨架在窄化状态下的计数/周期保持。两种模式（release 与 `--mode undefined`）均为 `{"passed":true,"authored_checks":919}`，不读游戏数据。仍未移植：`0x56C880` 与 `0x56D170` 的容器遍历及其下层调用、`0x56C880` 之后的 `0x56D170` 整段答案来源（`+80`/`+48` 计数由未移植的 `0x56B8E0`/`0x56B8C0` 产生），以及 `0x466E90` 一侧的全部内容。
