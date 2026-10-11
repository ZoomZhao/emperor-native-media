# REA 固定 PE 静态关口、安装与原版接线缺口

2026-10-11，补充基线 `852aea89f94226a901ff5ec98f4b7a6fec83b15b`。固定 PE 的身份/头部/导入表核对已完成。原先被拒的无目标 REA 自检在新授权及 macOS 分支源码证据支持下，原命令的一次标准复审获准并实际执行，确认 Java 配置成功、Swift5.10 与继承的SDK15.2不兼容。仅在子进程配对已有SDK14.5后，实际 REA/Ghidra inspect 两次均到45秒启动期限；自有日志证明原PE已导入、自动分析已开始，但尚无完成握手、函数图或调用点结果。没有执行原程序或新增原行为对照，完整原版tick仍未通过。

## 安装来源与复现

本地工程：`/Users/zoomzhao/Workspace/emperor-native-media`。CLI 安装在 `.local/rea-cli-6.4.0/node_modules/rea-agents`，没有替换全局 Node/npm/REA，没有运行 `rea setup`、安装分析器或写入持久 MCP/技能配置。已有全局 REA 保留原状。

| 项目 | 固定来源或本次实际结果 |
|---|---|
| 官方源码 | [morluto/rea 固定提交](https://github.com/morluto/rea/tree/a56f7bb668ab23fec8d33f7af353ac7ed2fe1895)，gitHead `a56f7bb668ab23fec8d33f7af353ac7ed2fe1895` |
| npm 发布 | [rea-agents 6.4.0 元数据](https://registry.npmjs.org/rea-agents/6.4.0)；名称、版本、repository、dependencies、engines 与固定源码 package.json 一致。浮动 main 已有依赖差异，不能作为完全相同的发布凭据 |
| 发布 tarball | [官方 npm tarball](https://registry.npmjs.org/rea-agents/-/rea-agents-6.4.0.tgz)；SHA256 `59eb1604b8db75f522d4a975c36ce6b4a2ad465e43bad165f9a8f728391db9b8` |
| npm integrity | `sha512-Vj1zWfSlna5bphGoaoI/TcVBuYvze9u+OyPC3Shz8YkBqaCTqB4COnnRoX7XDwMpwI24ZlFEel2yEj0k8LTWEQ==`，下载内容与 registry 值一致；未验证发布签名/attestation，不把一致性核对称为签名认证 |
| 实际运行时 | Node `/Users/zoomzhao/.nvm/versions/node/v24.14.0/bin/node`，v24.14.0；配套 npm 11.9.0。默认 Homebrew Node v25.8.2 不满足该包 `^22.19.0 \|\| ^24.11.0 \|\| >=26.0.0` 要求，未改全局默认 |
| 安装锁文件 | `.local/rea-cli-6.4.0/package-lock.json`；SHA256 `53731622932f39150d6f01438ef247072207b57f63836adf66df2dd32ef0199f`；REA 版本/integrity 匹配固定发布，resolved 下载地址均属 registry.npmjs.org |
| 实际 CLI 入口 | `scripts/rea.mjs`；SHA256 `3554288a08c455fa419dad662d73710fb17debe94b9e71dd1282761117c5981e` |

实际安装使用上述 Node 执行同版本目录下 `lib/node_modules/npm/bin/npm-cli.js`，参数为 `install --prefix <隔离目录> --ignore-scripts --no-audit --no-fund --omit=dev --save-exact --registry=https://registry.npmjs.org/ --fetch-retries=0 --fetch-timeout=20000 rea-agents@6.4.0`。每次进程限时60秒；使用隔离 npmrc/global-npmrc/cache，子进程移除 npm token 和 NODE_OPTIONS，不读取或打印个人 npm 凭据，不改 HOME/CODEX_HOME。

已静态审阅发布包生命周期脚本：REA 无 preinstall/install/postinstall，prepare 在限定条件下可能初始化 Husky，prepack 用于打包。本次全部依赖生命周期脚本均禁用，没有执行 curl 管道安装器或更新/集成命令。有限遥测关键词扫描没有发现初始化代码，但不构成全依赖安全审计。REA 包含浏览器、网络、MCP 与运行观察能力，不能笼统承诺其所有功能都离线；本次对外只获取公开源码/npm 依赖和推送选定文档，没有上传原程序、GameData、资源或分析输出。

安装过程保留失败：首次下载/展开达到60秒限时；继续安装虽 exit0，但一个 MCP client 文件展开不完整，CLI 五项调用均启动失败。离线 `npm ci` 又因锁文件缺少其他平台的可选 PTY 条目而拒绝。随后将该次 node_modules/lock 保留为 `interrupted-node_modules`/`interrupted-package-lock.json`，从缓存与官方源重新安装；最终版本及以下实际调用均通过。没有启用安装脚本来解决问题，旧失败不会改写成通过。最终锁文件尚未再次用 `npm ci` 验证跨平台复现，PTY/调试/运行观察也没有验收。

## 第一阶段：无目标 REA 使用（已完成）

所有命令使用固定 Node 和本地 `scripts/rea.mjs`，临时环境设置 `REA_ANALYSIS_PROVIDER=ghidra`、`GHIDRA_INSTALL_DIR=/opt/homebrew/opt/ghidra/libexec`、`JAVA_HOME=/opt/homebrew/opt/openjdk@21/libexec/openjdk.jdk/Contents/Home`，未持久化。

| 实际命令 | 结果与可推断范围 |
|---|---|
| `--version`、`--help` | exit0；版本6.4.0，CLI 能加载。不是目标分析或原版运行证明 |
| `doctor --provider ghidra --json` | exit0，选定 scope `healthy=true`：Node、Ghidra路径/平台/架构/版本/headless/native decompiler/JDK 均通过。Ghidra 实际12.1.2，REA判为12.1.x兼容，官方已验证构建标为12.1.4；JDK实际21.0.12。没有运行目标分析或自动升级 |
| 同一次 doctor 的全环境结果 | `environment_healthy=false`；Hopper未发现、IDA未配置、REA技能/若干客户端注册未对齐。它们是未选择的集成诊断，不使已通过的 Ghidra scope 变成失败，也没有据此自动安装/注册 |
| `capabilities --json` | exit0，Ghidra availability为available；`open=false`、`analysis_run=null`、`analysis_activity.status=not_observed`，没有绑定或分析任何目标。列出的能力是提供者声明，不能当作已实测原目标支持 |
| `import-reference-source <冻结接口目录> --json` | exit0；`HistoricalSourceGraph`、authority=`historical-reference`、inventory_state=`partial`。九份授权自写 C/H 接口及三份既有审计的指纹与冻结来源逐项一致；relationships为空、parse_failures为空，不能推断没有C调用关系 |

导入仅包括 native_driver/runtime/platform_native/heap_connection、heap_native/thread_native 的自写接口及现有审计；没有遍历整个仓库或 GameData，没有复制原 domains/IR/目标字节。REA 此命令的关系解析只支持 JS/TS/JSX/TSX，C 在本次仅被识别和哈希。结果另保留 Node 路径竞争限制；provenance.importer_version 为 null，版本由独立 `--version` 记录绑定，不能擅改工具输出。图根 SHA256 为 `44fce8668b1e38f8d70398e548ac984adab6c61fefc2aed86ca3e65543a4a691`。

这些结果属于先前无目标阶段，不能拿其中 `target_open=false` 描述后来 inspect 的尝试，也不能把 doctor 健康当作目标分析已通过。后续新增证据如下；原调度/运行事实仍只引用既有报告。

## 第二阶段：固定 PE 的新证据（只读元数据）

当前授权允许固定文件的纯静态检查，先前“本轮不读取EXE”仅是当时审计范围，已被这次明确授权替代；不将旧转换/执行路线的拒绝扩大为所有 PE 静态工作都禁止。

输入为 `/Users/zoomzhao/Workspace/lzjq/Exe/ghidra/input/EmperorEN.exe`，SHA256 `8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753`，4,820,992字节，路径/祖先无符号链接。每次 inspect 前后及只读元数据核对前后，文件哈希与身份未变。没有读取旧 Swift、GameData 或执行此文件。适用输入目录 AGENTS 已完整阅读；固定文件的新授权优先于通常先查既有 corpus 的指引。

| 新结果 | 证据级别与限制 |
|---|---|
| i386 PE32；image base `0x400000`；入口 RVA `0x3688ba`，VA `0x7688ba`，位于 `.text` | `PE_METADATA_ONLY`：自写只读 PE 头部/导入表解析器的结果，不是 REA/Ghidra 反汇编；没有入口指令、到调度的调用链或可达性结果 |
| `.text`、`.rdata`、`.data`、`.rsrc` 四节；15个导入DLL、433个导入项（包括ordinal项） | 仅声明的静态依赖；不是433个实际调用、所有运行时动态依赖或必要API的总表 |
| DDRAW.dll：`DirectDrawCreate` 一项 | 能确定导入工厂名字；工厂产生的接口、vtable槽位、对象生命周期、调用路径、色键/ROP/表面恢复仍未知 |
| mss32.dll：54个 Miles 导入 | 名字涉及 startup/shutdown、waveOutOpen、sample/stream/3D provider、状态/释放、速率/音量/声像/循环、文件与reverb/filter。不是54个已实现API；参数、callback、循环含义和实际触发未获证 |
| binkw32.dll：14个 Bink 导入 | 包括 `_BinkOpen@8`、`_BinkDoFrame@4`、`_BinkCopyToBuffer@28`、`_BinkWait@4`、`_BinkService@4`、`_BinkOpenMiles@4`、`_BinkSetSoundSystem@8` 等。修饰名仅是ABI线索，不证明结构布局、参数合同、音视频时钟或原播放通过 |
| KERNEL32/USER32/GDI32 等 | 声明有线程/事件/等待、内存/heap、加载器、异常及 `RtlUnwind` 等依赖；现有有限平台子集不能据此升级为完整Win32提供者 |

完整导入数量按DLL为：KERNEL32 152、USER32 115、GDI32 51、comdlg32 1、ADVAPI32 5、SHELL32 1、ole32 3、DDRAW 1、WINMM 1、WSOCK32 28、mss32 54、binkw32 14、ijl10 3、COMCTL32 1、WINSPOOL 3。ordinal项保持ordinal，不猜名字。完整元数据只保存在本地 `rea-static-gate-20261011/pe-metadata.json`；Git只发布此筛选结论，不上传原文件或原始分析输出。

入口 `0x7688ba` 与历史报告的调度地址 `0x4ac2b0` 是两个不同的已知坐标；目前没有把二者连接起来的静态图，更没有自然执行证据。

## 有界 REA 关口的实际尝试与停止原因

固定 Node/REA 使用以下相同命令，没有启动游戏、调用 SR/Unicorn/Wine 或使用替代执行器：

```text
node <本地 rea-agents/scripts/rea.mjs> inspect <上述固定 EmperorEN.exe> --provider ghidra --json
```

REA 启动限45秒，外层总限60秒；没有提高预算、增加分析seed/extensions或改 ownership guard。原目标只作为静态导入文件，临时分析项目使用 readOnly/deleteProject 配置。

| 实际尝试 | 实际结果 |
|---|---|
| 普通沙盒，09:30:45 UTC | exit1，12.288秒；`execution_failure`、operation=`list_documents`、`GhidraLaunchError: Ghidra headless launch failed`、failure_kind=`start`、retryable=false。不是超时 |
| 标准 `require_escalated` 获准，09:32:08 UTC | 相同命令实际执行，exit1，12.131秒，同一启动错误；不是审批拒绝，也不是目标分析已通过 |
| 原先无目标自检的首次审批 | 固定 `diagnose_startup.mjs` 在 CreateProcess 前被拒，未执行；该拒绝凭据保留 |
| 同一无目标自检的一次标准复审 | 基于新用户授权与macOS分支不调用ps的源码证据，原命令/脚本指纹不变，标准 `require_escalated` 获准并实际执行；Java配置ok，helper准备失败并给出准确Swift/SDK冲突。脚本exit0仅说明保存诊断完成，不能称helper通过 |
| 配对已有SDK后的 `inspect final`，09:54:31 UTC | 子进程使用CLT Swift5.10/SDK14.5；exit1，47.053秒，`provider_timeout`、operation=`list_documents`、timeout_ms=45000。外层60秒未触发 |
| 同参数有自有日志记录的 `inspect observed`，09:57:49 UTC | exit1，47.790秒，同一45秒provider_timeout。自有ownership manifest的parent_pid匹配本次CLI；保留了PE导入和自动分析阶段日志；没有完成握手或查询结果 |

最初两次失败记录的target hash均为上述SHA，provider_version=12.1.2，profile_digest=`e3402f2d7c487875edf3c6215fa95d67ee1fa133ba68c2390a9338b38656a7b2`。当时stdout只暴露包装错误；新的获准自检才取得明确SDK错误，不能改写旧日志。所有inspect前后目标哈希/身份未变；结束时自有临时根均空，没有报告cleanupIncomplete，不推断全系统残留进程为零。

自检的具体错误为：`this SDK is not supported by the compiler`。CLT编译器实际Apple Swift5.10，继承的SDKROOT指向MacOSX15.2.sdk，其Swift接口由6.0.3构建，报unknown attribute `_extern`及Swift模块构建失败。原包的helper编译使用同一继承环境；这一具体阻塞已经复现。随后只为本次CLI子进程设 `DEVELOPER_DIR=/Library/Developer/CommandLineTools`、`SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX14.5.sdk`，与已有Swift5.10接口配对；没有安装、更改xcode-select全局选择或编辑REA源码。后续运行越过launch error并到达Ghidra导入/分析，是该修正的实际证据。此前doctor健康不覆盖这个helper编译器/SDK组合。

观察日志记录Ghidra `Headless startup complete (1673 ms)`、`IMPORTING: Loaded 0 additional files`、`ANALYZING all memory and code`；同一目标临时副本已导入，没有加载额外程序文件。日志时间为宿主本地格式，原样保留，CLI status另记明确UTC。只在ownership manifest的parent_pid匹配本次Popen-owned CLI后记录该analyzer的ghidra.log/script.log/ownership；没有增加ps或任何替代查询，也没有读取其他analyzer日志。REA未在45秒内收到post-analysis握手，CLI返回provider_timeout；不能从少量日志确定具体哪个analyzer最慢，更不能称反编译/调用关系查询已完成。

原始审批拒绝理由保留如下（后来同一动作的标准复审已获准）：

> 该自检会调用 REA 的 ownership helper，而其实现包含 `ps` 进程表查询；这属于此前明确禁止重试的被拒绝 `/bin/ps` 路线，不能以间接 helper 方式绕过，即使不提供目标也不应执行。

原拒绝要求不得通过替代或间接执行绕过，也允许先核对授权/低风险证据。随后将新授权、固定macOS分支证据和同一精确命令交标准审批，仅复审一次并获准；不是换命令/执行器绕过。原拒绝与复审结果分别保留。历史“possible cybersecurity risk”的原转换/执行拒绝是另一组证据，不由这次自检复审扩大或解除。

只读源码补充核对发现：被拒脚本在 macOS 的 `prepare` 分支只编译 REA 自带 helper，Java LaunchSupport 通过普通 execFile 查询配置；该脚本并不调用 ps、执行 native reader 或启动 Ghidra。整个正常 ownership 生命周期另包含全系统 `ps -axo pid=,ppid=,pgid=,uid=,stat=,command=`，以及 native reader 的全系统内核身份快照和选定成员 token 检查。因此不得写成“自检已经读取进程表”或“前两次启动错误一定由ps导致”。这个区别提交了标准复审；获准后才执行同一无目标脚本，SHA256仍为 `659a5f2cc71d6d689bbd2925f8071148bb53c26cba3e0799aeea4309e9ca9e98`。

正常 lifecycle 的读取与清理范围：ps列出系统可见进程元数据，command可能含其他任务私密参数；macOS reader用KERN_PROC_ALL取身份快照，再对所选自有组成员读取KERN_PROCARGS2，仅输出runId/身份/失败原因，不输出其他环境值。清理可请求本次bridge退出，对本次根组/后代组的全部成员验证随机token并在发SIGKILL前复核；Ghidra不启用全系统token sweep，身份不可读/不匹配时停止清理。未改系统设置或权限。精确自检只涉及自有编译子进程的取消、临时/cache写入。随后获准的正常静态provider生命周期沿用原guard及查询、管理本次自有analyzer；没有用户其他进程终止。

用户已明确授权使用REA及“完全访问”。实际收到的环境仍标workspace-write/auto_review，没有工具或系统确认模式变更；没有声称已full access或修改模式。依当前允许的标准复审机制提交新证据后获准；用户话语本身不是放行证明，实际工具创建进程才确认这次获准。未重复向用户要求同一授权，未移除guard、伪造token、换ps/sysctl查询或执行器。

当前配置/launcher使用默认自动分析，源码中没有可直接缩小到四函数的自动分析范围开关。官方seed文件只补充caller-declared函数/代码/标签，并不限制全映像自动分析；本轮没有用seed断言伪装发现、禁用analyzer或改bridge完成守卫来取得结果。四函数/深度1和两个站点约束适用于完成启动后的查询，不能缩短当前全映像导入/自动分析阶段。

后续深度1/最多4函数及最多2调用点仍未启动，原因是实际inspect未成功、没有完整握手；不能记为“空图”“目标已解析”或通过。没有自动扩大45秒启动/60秒外层预算。下一步须获授权调整一次纯静态启动预算，或采用已验证的官方范围控制方案，先完成同一固定输入的inspect，再执行下列限定查询：

```text
trace-native-values <固定目标> 0x4ac2b0 --provider ghidra --max-depth 1 --max-functions 4 --max-call-sites 64 --max-nodes 1000 --max-edges 2000 --limit 1000 --json
resolve-native-call-targets <固定目标> 0x4ac338 --provider ghidra --json
resolve-native-call-targets <固定目标> 0x5fc139 --provider ghidra --json
```

这些坐标来自既有报告，`max-call-sites 64` 是图内资源上限，独立解析仍仅两个指定站点；任何输出都须标明缺覆盖、置信度与截断，不能由静态推测升级为原调用ABI或行为等价。

## 对当前原版接线的实际影响

| 环节 | 目前能据证说什么 | 仍缺什么 |
|---|---|---|
| 原入口到调度 | 新PE入口元数据已定；原运行链仅有历史坐标 | 入口到 `0x4ac2b0` 的静态/自然运行连接、启动/原菜单/地图消费者未知 |
| 调度及phase8 | 历史 tid1 CALL `0x4ac338`→`0x4aeeb0`→返回 `0x4ac33d`；限定Watch7→8、无wrap/真实栈的切片合同 | phase8不是完整tick。Stage83 batch A碰前缀像素预算，B/C未开始；较晚controller新增原执行为0，无合格自然活动输入 |
| 切片消费者及绑定 | 自写驱动导入memory/threads/heap/锁/API/目标表并检查入口/完成栈后调用入口，完成后诊断/清理/退出 | 当前 `_s82_entry`、`_s82_completion`、返回表/count和structural bindings仍未解析；App未链接原消费者、timer仍是自写CitySimulation；一次性驱动不证明连续原调度 |
| 动态callback | 历史 worker4 `0x5fc139`，ECX来自ESI对象、整数1、返回 `0x5fc13b`；初始槽为NULL | 初始NULL不是恒空；工厂/注册/释放、目标身份、栈清理/flags/x87及生命周期均未补上。这次仍没有新调用点结果 |
| 平台及状态 | 当前有限15项dispatch，heap/TLS/LastError/线程ID、三种有限锁与memops；未知明确失败 | 旧选定集合20项Win32边界、20个非IAT目标、SEH/wait/wake/运行后冷恢复仍缺。新导入表扩大的是声明依赖视野，不证明这些接口已调用或通过 |
| graphics | 自造RGB555/565/BGRA的stride、裁剪、缩放、重叠及生命周期已有限定CPU/离屏Metal证据 | DirectDrawCreate到原接口消费者未连通；旧移位最大248/252与当前补位255不同。GPU读回不等于原画面或全游戏通过 |
| audio | 自造S16队列/循环、格式拒绝和fake设备恢复有离线证据，历史AudioUnit仅有限回调证据 | 54个Miles导入的合同/触发/混音/clock、真实输出与设备恢复未闭合；离线PCM不能当听觉验证，fake恢复不能当物理设备通过 |
| video | 自造MOV reader/PTS/seek/epoch/取消有局部证据；旧Bink仅无输入/无factory的失败证据 | 14个Bink导入与Miles音轨连接未实现/未验；自造MOV的最新集成仍 `writer start failed`，不证明原Bink播放、EOB或双缓冲通过 |

旧203函数/7346条静态指令/20089字节属选定构建证据，旧Stage82原函数调用为0。Stage73六函数、一个固定输入的有限匹配不能推广到203函数、原完整tick或游戏。更详细的提供者、API、heap及callback缺口见 [原版tick接线审计](ORIGINAL_TICK_WIRING_AUDIT.md)、[完整原版差距](ORIGINAL_GAME_GAP_AUDIT.md)；媒体验证范围见 [PLATFORM_AUDIT](PLATFORM_AUDIT.md)、[RGB555_BOUNDARIES](RGB555_BOUNDARIES.md)。

## 先前接口审计确定的独立工作（本轮未执行）

| 任务 | 具体通过标准 |
|---|---|
| 将新导入依赖落实到提供者/消费者合同 | 启动/完整tick/返回/线程/heap/callback及DDraw/Miles/Bink逐项标来源、版本/ABI、状态owner、实际消费者与缺符号；导入名、静态猜测、历史运行分别标证据级别；未知目标/ABI保持明确失败，不补空表或成功stub |
| 修正自写heap提交计划与可恢复状态格式 | fill+copy总预算/权限在提交前规划；失败不假称回滚。诊断与可导入快照分型并版本化，明确handles/regions/live及CPU/TLS/锁所有权；完全自造状态覆盖拒绝路径和重导入合同，只记适配器验证，不称原续跑等价 |
| 验收现有媒体消费者的可声明子集 | 用自造图片逐合同检查555/565舍入、padding/裁剪/缩放、旧句柄与迟到GPU完成；自造PCM/事件核对队列样本/游标/0循环语义、失效不消费、恢复续接及事件恰好一次；先修自造MOV writer失败，再核对seek/epoch/取消/EOF和资源上限。原Miles/Bink、听觉/物理设备、原画面分别留待专门证据 |

上述是先前接口审计列出的独立任务。本轮实际工作集中于REA对固定原PE的自检复审、编译环境诊断和有界静态inspect，没有转去执行这些媒体/适配器测试。

窗口验收仍有独立的历史AX守卫失败：08:34:33两次助手在 `AXIsProcessTrusted()` 停止且未请求权限；08:29:45 `save panel required` 是另一个条件失败。本轮没有UI/键盘/音频设备操作或权限修改，也不通过REA UI功能绕过该守卫。

当前REA阻塞是45秒内未完成默认原PE自动分析并取得握手，已经不是“自检无法获得审批”。budget/guard保持原约束，尚无函数图和两个站点结果。原二进制转换/恢复指令执行/原城市动态对照的旧拒绝路线没有重启；新工作仅纯静态REA，不执行原或派生游戏库。真正完整tick通过仍需自然合格输入、完整阶段/完成边界、来源/ABI一致且实际被调用的消费者/目标、有对应原状态/对象/有序写/事件证据、完整恢复和连续调度/受控退出；静态分析和安装本身不能证明这些行为。

## 可追溯凭据

本地任务根目录 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2`：

- `rea-binary-route-20261011/`：固定发布来源/integrity、失败/修复/最终安装锁、五项无目标调用、冻结接口manifest及结果摘要；原证据保持原样。
- `rea-static-gate-20261011/`：input identity、最初两次inspect失败、PE_METADATA_ONLY、ownership源码/精确拒绝及历史 `APPROVAL_SCOPE.zh-CN.md`；旧证据未覆盖。新增 `selfcheck-review-request.json`/`selfcheck-review-receipt.json`、`startup-selfcheck.json`/`startup-root-cause.json`、匹配SDK及观察runner指纹、`inspect-final.*`/`inspect-observed.*`及自有provider日志、`review-and-inspect-result.json`。后者代表最新状态：复审获准，SDK修正后仍45秒provider_timeout。
- `rea-static-review-20261011/`：本次简洁结果、文档候选/选择发布计划及提交/远端验证凭据。原始工具输出与Ghidra日志仅本地留存。

仅提交此文档更新。原EXE/依赖树/工具包/原始分析输出不入Git，GameData不读取/上传，既有工程草稿与授权接口来源指纹保持原字节。
