# REA 安装、使用结果与原版执行路线

2026-10-11，仓库审阅基线 `8e17cb4f2e861021254ef8898a5f252d44ad7c4e`。REA 6.4.0 已隔离安装并实际用于无目标诊断、能力查询和自写接口源码索引。没有导入原 EXE、恢复原指令、执行原/派生游戏库或新增原行为对照；当前仍没有完整原版 tick 通过证据。

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

## REA 实际使用及其结论

所有命令使用固定 Node 和本地 `scripts/rea.mjs`，临时环境设置 `REA_ANALYSIS_PROVIDER=ghidra`、`GHIDRA_INSTALL_DIR=/opt/homebrew/opt/ghidra/libexec`、`JAVA_HOME=/opt/homebrew/opt/openjdk@21/libexec/openjdk.jdk/Contents/Home`，未持久化。

| 实际命令 | 结果与可推断范围 |
|---|---|
| `--version`、`--help` | exit0；版本6.4.0，CLI 能加载。不是目标分析或原版运行证明 |
| `doctor --provider ghidra --json` | exit0，选定 scope `healthy=true`：Node、Ghidra路径/平台/架构/版本/headless/native decompiler/JDK 均通过。Ghidra 实际12.1.2，REA判为12.1.x兼容，官方已验证构建标为12.1.4；JDK实际21.0.12。没有运行目标分析或自动升级 |
| 同一次 doctor 的全环境结果 | `environment_healthy=false`；Hopper未发现、IDA未配置、REA技能/若干客户端注册未对齐。它们是未选择的集成诊断，不使已通过的 Ghidra scope 变成失败，也没有据此自动安装/注册 |
| `capabilities --json` | exit0，Ghidra availability为available；`open=false`、`analysis_run=null`、`analysis_activity.status=not_observed`，没有绑定或分析任何目标。列出的能力是提供者声明，不能当作已实测原目标支持 |
| `import-reference-source <冻结接口目录> --json` | exit0；`HistoricalSourceGraph`、authority=`historical-reference`、inventory_state=`partial`。九份授权自写 C/H 接口及三份既有审计的指纹与冻结来源逐项一致；relationships为空、parse_failures为空，不能推断没有C调用关系 |

导入仅包括 native_driver/runtime/platform_native/heap_connection、heap_native/thread_native 的自写接口及现有审计；没有遍历整个仓库或 GameData，没有复制原 domains/IR/目标字节。REA 此命令的关系解析只支持 JS/TS/JSX/TSX，C 在本次仅被识别和哈希。结果另保留 Node 路径竞争限制；provenance.importer_version 为 null，版本由独立 `--version` 记录绑定，不能擅改工具输出。图根 SHA256 为 `44fce8668b1e38f8d70398e548ac984adab6c61fefc2aed86ca3e65543a4a691`。

因此 REA 本轮新增的是工具就绪、能力/副作用声明与接口来源核对。以下原执行路线来自既有报告和授权接口人工审阅，并非 REA 对原二进制重新产生的反编译或调用图。

## 原入口、调度与平台边界

| 环节 | 已有证据 | 尚未建立的连接 |
|---|---|---|
| 原启动、菜单、地图载入 | 当前 App 零参数启动进入自写 CitySimulation；未见完整原主菜单/地图消费者证据 | 原启动到调度入口的完整连接未知；本轮没有分析入口点或原资源 |
| 调度到选定切片 | 历史报告给调度入口 `0x4ac2b0`，tid1自然CALL `0x4ac338`→`0x4aeeb0`→返回 `0x4ac33d`，限定Watch实际7→8、无wrap、真实栈 | 这是phase8切片合同，不是完整城市tick。原参考batch A触及前缀像素预算，B/C未开始；较晚控制器新增原执行仍为0，没有合格自然活动输入 |
| 切片消费者与绑定 | 自写 native_driver 导入私有内存→四线程→heap/锁/API站点→返回/条件目标表→入口/完成栈守卫→调用 s82_entry→完成后诊断/清理/退出 | 当前入口、completion、return bindings/count及structural bindings尚未解析；一次性驱动没有已验证的外层原城市连续调度消费者，App未链接它 |
| 平台 dispatch | platform_native仅有限Heap/TLS/LastError/线程ID、三项单线程临界区及memcpy/memmove/memset；未知API明确停止 | 选定集合仍列20项Win32边界、20处未知非IAT调用；动态receiver、注册/释放、callee ABI、SEH展开、真实锁wait/wake和完整恢复欠缺。静态表成员不授权任意对象上的调用 |
| graphics | 自造RGB555/565/BGRA、stride/裁剪/缩放/重叠/生命周期有CPU及离屏Metal证据 | 原DirectDraw消费链、色键/ROP/混合、表面失效及drawable/vblank等没有完整原画面证明；旧移位与当前补位扩展不同。GPU读回不证明游戏通过 |
| audio | 离线S16队列/循环/格式拒绝/fake设备恢复；旧自造AudioUnit报告仅有限系统回调证据 | 原Miles等API、触发/混音/时钟、真实输出与设备恢复未闭合；队列0遍循环含义及格式必须按具体合同，离线PCM不能当听觉验证 |
| video | 自造MOV的Reader/PTS/seek/epoch/取消等局部证据；旧Bink适配仅缺输入/缺factory明确失败 | 有效原Bink、DLL ABI、双缓冲/EOB/sound-open及原消费者无通过证据。最新通用媒体集成在自造MOV `writer start failed`，不能说整体通过 |

接口、20项API及heap/回调/线程细节见 [原版完整tick接线审计](ORIGINAL_TICK_WIRING_AUDIT.md)，模块差距见 [完整原版差距](ORIGINAL_GAME_GAP_AUDIT.md)，媒体验证时点见 [PLATFORM_AUDIT](PLATFORM_AUDIT.md) 和 [RGB555_BOUNDARIES](RGB555_BOUNDARIES.md)。旧203函数为选定构建集合，Stage82原函数调用为0；Stage73六函数一个固定输入的局部匹配不能扩展为203函数或完整游戏通过。

## 能继续推进的三个工程任务

| 任务 | 具体通过标准 |
|---|---|
| 明确原链提供者与消费者合同 | 启动/完整tick/返回/内存/线程/heap/回调/媒体每项标明来源、版本/ABI、状态owner、真实调用者和缺失符号；phase8与完整tick明确分开。未知项留欠缺，不填空表或成功stub；不读取原EXE即可先完成接口工作 |
| 修正自写heap提交及恢复格式 | 将权限与fill+copy总预算在提交前统一规划；失败不假称回滚成功；诊断和可重新导入状态分型且版本明确，包含所声明handles/regions/live/CPU/TLS/锁所有权。只用自造状态验证实际适配器，不称原行为等价；原状态续跑仍待原证据 |
| 完成共享媒体接口的独立验收 | 完全自造图片/PCM/事件验证当前实际消费者：graphics各自像素合同、padding/裁剪/缩放/旧句柄及迟到GPU完成；audio样本/游标/循环、失效不消费、恢复续接、失败无状态污染及事件恰好一次；video先解决自造MOV writer失败，再验seek/epoch/取消/EOF与资源上限。记录限定结果，不转成原画面、听觉、物理设备或Bink通过 |

## 当前不能执行的部分与真正通过关口

此前被拦截的是原二进制转换/指令恢复/探测及原城市执行对照的具体路线；现有报告只保留“possible cybersecurity risk”描述，没有足够原始拒绝信息重建每条命令。本轮没有发生新的此类审批拒绝，也不把旧拒绝扩大为禁止一切原版讨论。更换为 REA 不授权恢复、包装、委托或绕过同一路线；当前不读取原EXE/反汇编/旧Swift/GameData，不调用SR/Unicorn/Wine，不执行原/派生游戏库。

Ghidra具备跨宿主PE/x86静态分析能力，与执行Windows游戏不同；本轮没有导入原PE，原目标支持/分析完整性未知。REA原生运行观察主要是Mach-O/LLDB路线，并未提供本次已验证的Windows Emperor执行器。工具安装成功不能消除原消费者、完整tick和对照证据缺口。

窗口验收另有历史AX守卫失败：2026-10-11 08:34:33两次助手在 `AXIsProcessTrusted()` 停止，未请求权限；08:29:45的 `save panel required` 是另一个条件失败。本轮没有UI重测或权限修改，不推断全系统/当前GUI均不可用，不借REA UI工具绕过守卫。真实设备听觉/断连恢复也不在本轮范围。

真正的完整原tick关口必须同时具备：明确覆盖全部必要阶段的自然输入与完成边界；来源/ABI一致且实际调用的消费者、返回及动态对象目标；对应原状态/对象/有序写/事件证据；未知API明确停止且不伪造成功；完整状态恢复、连续调度与受控退出。之后才验原菜单/画面、战役、原存档和音视频。当前没有满足，不给完成百分比。

本地可追溯证据位于任务目录 `rea-binary-route-20261011/`：源码获取及tar完整性凭据、安装失败/修复/最终锁文件、五项命令stdout/stderr与哈希、冻结接口manifest、结果摘要。原始诊断、依赖树、工具包和二进制不纳入Git；本次只发布此说明，现有工程草稿保持原字节。
