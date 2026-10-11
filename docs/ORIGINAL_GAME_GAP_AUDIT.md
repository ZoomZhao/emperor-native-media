# 完整原版游戏差距审计

2026-10-11。目标是完整原版《皇帝：龙之崛起》运行。当前仓库交付的是自写平台组件、有限资源解析和自写城市 App；还没有完整原版启动、城市周期、战役及原版存档的端到端通过证据。自写玩法、GPU 读回、离线 PCM 和编译成功均不能替代该目标。

初次审计只读工程源码/报告；后续已明确授权固定SHA原PE的REA/Ghidra纯静态检查，现已完成多批。旧“不读EXE/反汇编”是初次范围，不继续作为当前静态禁令。原IR/派生库/旧Swift/GameData未读，原/转换代码和被拒执行路线未运行。停止新增自写玩法、关卡及泛用媒体微测试。在途视口改动已安全收口，成果和其他草稿保留。

具体接线依赖见 [原版完整 tick 接线审计](ORIGINAL_TICK_WIRING_AUDIT.md)：首先缺完整 tick 边界及合格自然输入，其次缺当前入口/返回/条件目标绑定和未来回调契约。phase8 切片返回不能直接认定为完整 tick；legacy C heap/线程/异常合同与当前有限模型也须分别核对。该审计未运行原链或 UI，并记录了 AX 实际检测时点与影响。

## 逐模块事实

| 模块 | 当前实现及已验证范围 | 原版证据 | 完整原版仍欠缺 |
|---|---|---|---|
| 城市 tick 与业务 | CitySimulation 自写 8 类可建建筑、按月人口/粮钱/用工/服务、仓储路网及自写挑战；App 定时器调用该模型 | 旧 Stage73 只有六函数/728 字节、一个固定续接输入的局部匹配；扩大集合没有新增原行为执行证明 | 原版完整城市周期、全部对象和调度接线；原住房/生产链/行走者/贸易/军事/事件等覆盖与行为核对未闭合，不能用自写数值替代 |
| 原逻辑与执行入口 | 当前 App 明确编译自写源码与 Apple 框架，没有链接旧 203 函数库或 runtime_core | Stage81 记 203 函数、7346 条静态指令、20089 代码字节构建；Stage82 驱动已链接但未执行 | 入口/返回/动态目标绑定、完整状态及平台消费者、合格自然输入、一次完整原 tick 及后续连续运行都缺证据；203 是选定集合规模，不是剩余全部游戏函数数量 |
| 内存、heap、线程、异常和回调 | 有限 vector 内存、context/TLS 数据、TLV 以及自造原生 guest-memory 权限/清理验收；自写媒体迟到回调和关闭保护已接线 | 旧平台有局部 heap/线程/锁/时钟记录；Stage82 仍有 20 项 Win32 边界、20 处间接调用站点及未实现 SEH | 原对象图与地址/引用/分配归属、realloc/free/退役生命周期；真实调度、CPU/异常 dispatch 与 unwind、CRT/虚函数/User32 回调域和调用约定尚未闭合。有限 context 不是 OS 线程，有限 heap 退役不是完整 HeapDestroy 等价 |
| graphics/API 桥 | 当前 RGB555/565/BGRA、stride/padding、裁剪、最近邻缩放、重叠和生命周期有自造 CPU/离屏 Metal 证据；城市网格由自写图像显示 | 旧 DirectDraw 桥有有限转换/Blt/Lock 等记录；未形成完整原版呈现证明 | 全部所需 DirectDraw/呈现合同、色键/ROP/混合、失效恢复、drawable/vblank、输入与菜单/城市画面接线。旧移位扩展最大 248/252 与当前 surface 补位 255 须逐合同对齐，不能因两边都通过而认定等价 |
| 原资源解码 | 有界 WAV PCM16LE、JPEG/raw555、SG3 表、128×128 地块预览和 GB18030 文本接口，有限自造输入回归 | 旧 GameData 报告仅含有限样例；当前目录存在但本次没有检查内容或完整性 | 全部 SG3/伴随图素、压缩/未知布局、动画/遮挡/字体/调色与原版语义；当前 extractSprite 只接收 type16/24/32 的固定未压缩方块，不是完整图素解码。地图解压预览不是完整城市或战役载入 |
| audio | S16 离线队列、循环/格式拒绝、fake 恢复及 App 音乐/SFX 控制；默认静音 | 旧 AudioUnit 自造 FIFO 有系统回调记录；无完整原版 Miles/混音或听觉通过 | 原音乐/音效资源与触发、Miles 等所需 API/回调、真实混音/输出时钟和设备恢复。离线 PCM 逐样本一致不证明实际听到声音 |
| video | 自造 MOV Reader/PTS/seek/epoch/背压、异步取消/关闭和有限静默窗口播放有证据 | 旧 Bink 适配证明缺失输入/缺 factory 失败；没有有效原 Bink 解码通过 | 原 .bik、旧 DLL ABI、双缓冲/EOB/sound-open、原游戏触发和真实音画同步。最新通用集成在自造 MOV writer 启动失败，不能标为整体通过；自造 MOV 成功也不证明原影片兼容 |
| 菜单、输入与窗口 | Cocoa 资源页面、自写城市营建/挑战/存档按钮和有限自有窗口操作通过；正常零参数直接进入自写城 | 未见完整原版菜单/输入/窗口链验收 | 原主菜单、设置、任务/世界地图、原版 HUD、快捷键/输入路由、全屏/尺寸/焦点与保存菜单接线；Cocoa 按钮存在不代表原 UI 已恢复 |
| 战役与脚本 | 自写三关和官仓训练，自写期限/结果/收据 | 没有原战役脚本执行或原任务胜负通过证据 | 原任务装载、触发/事件/目标、世界进度、剧情/外交及跨关保存；新增自写关卡不能减少这项缺口 |
| 原版存档与恢复 | EMP1/ECG1/ECS1/ECA1 为自写格式，有原子错误拒绝及原生保存恢复证据 | 旧状态检查点不是普通玩家原版存档兼容证明 | 原版存档解析/写回、完整对象/脚本/资源状态、跨进程恢复后自然续跑和原版往返兼容；当前原生面板成功只证明自写格式 |
| 依赖及交付 | 当前 macOS15+、系统 clang/框架和 Python 构建已有证据；GameData/包/截图/存档被排除于推送 | 历史产物、配置与域文件在旧任务有记录；没有重新核验其二进制哈希 | 明确可使用/发布的来源与版本/ABI、生成绑定及完整原运行入口；归档或文件存在不证明已链接到 App 或可运行。当前包不是完整原版发行包 |

## 203 函数与旧报告哪些仍适用

- Stage81 结果状态为 FULL_FRESH_SR_GAME_OBJECT_CODE_DATA_SEPARATED_BUILD_ONLY。Stage82 结果为 DRIVER_BUILT_NOT_EXECUTED，original_game_functions_invoked=0。本次重读的是结果文本，未读取或重新运行那些产物；不能把“仓库未收录”改写成“本机不存在”，也不能把静态构建写成原版通过。
- Stage73 后续报告为 DIFFERENTIAL_MATCH_EXACT_SCOPE：49889 条有序写、297521152 字节映射内存与 public CPU 在一个固定输入匹配。其六函数旧库范围仍应保留；不适用于新 203 函数库、完整周期、一般 REP 故障、冷恢复或所有回调。
- Stage82 清单：23 项 API 中 3 项单线程锁仅在自造条件下通过，剩 20 项边界涉及 locale/编码/环境、模块与文件/进程元数据、动态加载/User32、虚拟内存和异常/终止；SEH 为 UNIMPLEMENTED_FAIL。20 处未知间接调用另分 11 虚调用、6 CRT 回调、3 动态 User32。它们是该选定集合的静态缺口，不证明每个自然输入都会走过，也不是全游戏 API 总数。
- Stage83 batch 原参考 A 在像素循环触及既有预算，B/C 未启动，没有合格自然 phase8 入口。后来 phase7-probe-v1 已实现有界控制器，并有自造停止/冻结和只读状态恢复证据，因此“没有执行器”过时；其报告仍明确新增原指令/SR 游戏函数执行为 0，B/C 格式适配与真实 phase8 差分未完成。初始空回调槽不能证明未来恒空。控制器自造全幅性能也触及时间界，不能承诺原路径在预算内完成。
- 旧 CITY_BUILD_HANDOFF/CITY_CAPABILITY_AUDIT 中的“App 只有资源预览、没有城市消费者”已经过时：当前确有自写城市消费者及存档。仍适用的是“没有原版城市消费者、没有完整原周期证据”。DEFAULT_RESOURCE_BROWSER 等历史页应按其冻结时点阅读。

## 通向完整原版的依赖顺序

1. **冻结原版垂直链的证据和接线合同。** 将启动→原菜单→原地图载入→完整城市 tick→原存档的入口、对象状态、API/动态目标、资源、脚本、回调、失败/退出依赖逐项映射到已有实现、当前消费者与精确报告。先确定选定集合覆盖何处、缺何处，不用成功 stub 填空，也不把 203 视为完整覆盖。
2. **闭合这条链实际需要的运行时与资源/API 依赖。** 首要是状态/heap 生命周期、真实调度与异常/回调边界；图形、输入、资源加载必须接到原消费者并对齐所需合同。单独模块通过不能替代原链。已知仍未解析或未支持的入口须明确停止。
3. **达到真正的原版执行关口，再扩展覆盖。** 首个行为关口应是合格自然输入驱动一完整原城市 tick：实际消费者运行，状态/有序写/对象和事件符合明确参考，未知 API/动态目标不被执行或伪造成功，受控退出与错误态完整记录。之后才是连续周期、完整原画面/输入、战役/脚本、原存档恢复续跑，以及原音视频与发布验收。这是拟定关口，当前没有完成，也不在本轮执行。

现阶段可立即交付的第一关是上面第 1 项的**静态依赖闭合清单**：每条依赖有来源、版本/ABI、提供者、真实调用者、验收范围；没有提供者的项明确标欠缺，冲突和旧结论标注时点。实际原执行关口受具体被拒路线和上述技术缺口共同约束，不能用增加自写 demo 功能替代。

## 三个最有用的后续工程任务及通过标准

| 任务 | 具体通过标准 |
|---|---|
| 原版最小垂直链依赖审计 | 启动/菜单/地图/完整 tick/原存档每一环都有当前消费者、来源与冻结证据；203 集合覆盖/未覆盖明确，所有未解析入口/API/目标逐项列出；旧六函数结果不转移给新库。只读先交清单，不执行原工具链 |
| 原链运行时与回调生命周期接口闭合 | 依据已有授权平台源码和明确合同补真实提供者；覆盖分配归属、对象生存/释放、调度与异常/回调退出，失败无假成功、无悬空引用，当前消费者实际接线；明确哪些只是自造合同、哪些仍无原版等价证据，不添加另一个替代城市模型 |
| 原版内容与兼容覆盖矩阵 | 原资源图素/动画/字体、输入/菜单、战役脚本和原存档分别记录所需格式/消费者/实现及已获证据；不支持格式明确拒绝；每项原版兼容结论需独立实际证据，GameData 存在、预览图或自写存档通过均不能代替。当前先用授权源码和既有报告，不读取未授权资源 |

## 技术缺口、具体限制和在途工作

技术缺实现/缺验证包括原逻辑消费者、完整 tick、上述 Win32/SEH/动态回调、资源/战役/存档/原音视频。可以继续讨论完整目标、审阅授权源码和报告、设计真实接线与错误合同、整理依赖和实施已授权的平台修复。

此前平台安全拦截的是具体原二进制转换/指令恢复/探测路线；本轮不恢复、包装、委托重试或换目录/工具绕过。不将该拒绝扩大成禁止讨论整个目标或所有平台工程。当前已批准的固定原PE静态读取与REA分析可以继续；旧Swift/GameData和原/派生游戏执行、SR/Unicorn/Wine不在本轮范围。这些范围记录不是新的技术兼容结论，也不禁止整个原版恢复目标。普通 Git 推送仅发布授权源码和说明。GameData 放入目录没有自动增加读取/执行/上传授权。

当前窗口辅助返回“existing AX permission unavailable; no request made”，未改系统权限；这是本次剩余自有窗口验收的具体条件缺失，不是新的一揽子安全拒绝。最新通用媒体集成的 MOV writer 失败属于实际测试失败，不与原执行安全拒绝混写。

最近完成完整首玩/原生存档路径验收的固定自写包对应 cd5fe0347d020fe2319fed06f4521b52ee8fb35a，位于 game_lab/build-playable/EmperorApp.app，它不是原版游戏。在途视口源码已收口至 72c8a14f07b81d32f1d43a4a236600296d80665e：几何/编译通过，默认 4x 部分窗口路径有证据，完整窗口验收及正常退出未闭合，未替换固定包。新道路搬粮规则、多仓转换和新增玩法均未继续。其他草稿保持原字节。

## 依据

当前源码依据：game_lab/app/build.sh、app/main.mm、city_simulation.hpp、city_game_state.hpp、emperor_assets.mm 及 runtime_core/README.md。工程证据摘要见 [CITY_BUILD_HANDOFF](CITY_BUILD_HANDOFF.md)、[CITY_CAPABILITY_AUDIT](CITY_CAPABILITY_AUDIT.md)、[AUTHORED_DRIVER_COMPILE](AUTHORED_DRIVER_COMPILE.md)、[NATIVE_GUEST_MEMORY](NATIVE_GUEST_MEMORY.md)、[PLATFORM_AUDIT](PLATFORM_AUDIT.md)、[RGB555_BOUNDARIES](RGB555_BOUNDARIES.md)、[APP_MOVIE_PLAYBACK](APP_MOVIE_PLAYBACK.md) 和 [MAP_VIEWPORT](MAP_VIEWPORT.md)。

旧文本定位（LOCAL_LEGACY_TASK 只是证据别名，不是运行入口）：stage81/builds/full-separated-v5/result.json、stage82/builds/original-v3/result.json、stage82/evidence/blockers-verified.json、stage83/batch-v1/reports/README-final.txt、stage83/phase7-probe-v1/reports/README.txt、evidence/stage73-differential-result.json。本地审计源指纹封存在任务目录 original-game-gap-audit-20261011/evidence-manifest.json；不上传旧产物、原始轨迹、图片、音频、存档或二进制。


## 原入口与实际对象来源增量

[对象工厂/登记/消费路线](ORIGINAL_OBJECT_FACTORY_ROUTE.md)补上CRT初始化表消费8c7630，以及固定selector a1的51c9a0构造、同池登记和按返回索引立即消费；载入版本站点与phase37共用主池。+94仍有完整Listing与transitive effects缺口，不能默认构造表稳定。批次因自定站点预算exit1，保存85Evidence/4NoProcedure及未执行请求，未标全通过。具体后续入口与精确阻碍见[原版handoff](ORIGINAL_ROUTE_HANDOFF.md)。静态工作未穷尽，原完整运行/动态绑定未通过，不转用自写媒体成果代替。

## 后续静态证据增量

[注册合同](ORIGINAL_POSTREGISTRATION_CONTRACT.md)已关闭428aa0本地Listing缺口，并连到注册期间的同池索引消费。新缺口具体是5268d0/418e80 transitiveeffects、766b16/88a3d4分配回调、旧对象销毁、registry与版本载入；静态工作未穷尽，实际实例/生命周期/完整运行没有通过。

## 后续原静态合同

[原生命周期/退出来源](ORIGINAL_LIFETIME_AND_EXIT_CONTRACT.md)连到真实CRT登记数组及正常主函数返回消费者，提供Q=O的明确条件而非自然实例证据；登记失败被忽略、回调数组/slot存活仍未证。[flag合同修正](ORIGINAL_FILTER_CONTRACT_CORRECTION.md)只接受1或3，5返回0。

## 回调及异常退出缺口更新（2026-10-11）

[新原源合同](ORIGINAL_CALLBACK_LIFETIME_CONTRACT.md)已补齐选定+130、d2+264/+270/+278的正常局部路径，以及备用块handler和所选异常free action。真正待证的是live O/P/Q、当前根/slot、Q+274与其它间接回调、特殊heap模式、OS异常和退出重入。不能把缺pin直接记为UAF，也不能把局部没有free记为全部callee安全。3a15dc7独立复核44/0，新增语义未复核；自然游戏graphics/audio/video及完整周期仍未通过。

## 文件成为局部可调度状态的来源（2026-10-11）

[版本载入合同](ORIGINAL_VERSIONED_CITY_LOAD_CONTRACT.md)补534a30实际文件读入口、城市19/池1/类schema0/基类字段5、构造默认值被输入覆盖及主池追加/条件hook。版本19读取调度phase和周期计数，不能无依据从0重新开始；serialized Ob4与slot对应仍未证明。下一步是534bf0/42da10读后作用、文件逻辑位置/buffered及双编号/append失败，见[handoff](ORIGINAL_ROUTE_HANDOFF.md)。这是获准原源纯静态证据，未执行原或派生游戏、未读GameData，不由自写测试替代实际加载或调度启用。

## 载入返回不等于新状态已调度

[读后原源](ORIGINAL_POSTLOAD_SCHEDULER_ENTRY_CONTRACT.md)表明534bf0已经直接消费4aeeb0并执行主池postload；下一次真正分派另需quota与gate/hold条件。42da10实际flag筛选1/3/6，phase reset外层周期为1，且reset有独立分支。写后传递覆盖、derived方法/槽存活、owner34两次guard仍缺；loaderEAX被忽略，mode1也不证明输入成功。当前为本地可复核增量，5587fb5精确八文档推送待批，不扩大运行范围或发布授权。

## 派生读后并非统一no-op，owner字段稳定仍缺

[owner34及派生源](ORIGINAL_OWNER34_DERIVED_POSTLOAD_CONTRACT.md)明确4fd600前置清零，第一guard前的非零生产依赖及两次guard间传递/alias仍Unknown。选定4f0150读O7/a/c/14并virtual+100，固定root候选4c69a0尚未展开；AL1与完整返回值、正常局部图与callee效果均不能混用。具体记录/字符串/观察状态写和新分配空结果解引用边已保留；不以无直接owner34 store证明全局不变。上一批nullowner速度分支遗漏已本地修正，原独立38/1保留、新增待复核。
