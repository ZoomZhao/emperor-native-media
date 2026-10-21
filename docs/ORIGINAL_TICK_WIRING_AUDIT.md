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
