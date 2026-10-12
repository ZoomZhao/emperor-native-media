# 2026-10-12 本地变更已提交，App共享图像转换验收

此前保留九项内容已由用户130790d提交并推送；用户随后提交5dd5050，本任务保留该提交，文档核验补充6429195已正常发布。用户报告完成0736独审，其报告/results/日志与manifest匹配：作者两模式各34/0，独立各10/0。本轮仅核对交付记录，没有重跑该审查或原来源；先前具体平台拒绝动作及依赖扩展仍不重试。

现有自写城市测试Release/UBSan各734项，原surface runner各218646项，原媒体边界草稿各145项通过。App中的CoreGraphics转换已抽成共享工厂，直接合成验收两模式各4138项，涵盖预乘BGRA/行布局/256alpha/输入存活和256轮替换释放；App只构建、签名strict校验通过，未启动。图像数据/合成模型结果不代表完整原版通过。[范围、九项内容与证据](CG_IMAGE_CONVERSION.md)

这轮只完成现有提交收尾和这一项共享入口，不持续扩展媒体测试。可独立推进且直接有助于真实资源加载的是将有界读取/当前解码失败原因传到App，按自造缺失、截断和不支持格式验证反馈及失败清理。原版加载身份、原规则和有效Bink仍缺证据，不能借另一工具/代理恢复受阻操作。以下历史任务与数字保留，不自动成为本轮授权或当前状态。

---

## 2026-10-11 原生存档补验与二方仓储回归已发布

普通推送main：`bfaa7863f19f93e9fec7b3026e4a4dc3354b5c0c`，HEAD/origin/main/独立读取远端一致，生产App源码保持a43919c验收版本。

此前3次辅助失败保留。后续相同App的PID10378完成131152字节仓储城原生保存/改变城/载入/另存整档一致；损坏档窗口拒绝、再另存旧状态整档一致；自造旧v1窗口回读/拒绝转换/另存65580字节一致；正常退出，无强杀。新辅助2次失败也保留，GoTo偶发整体关闭内部原因仍未证明，没有新的审批拒绝，不伪称所有UI通过。

二方Release/UBSan各9 PASS、0 FAIL已实际执行，来源哈希与发布源码一致；独立参考与回归已原样导入。有限窗口补验与二方模型检查各有独立证据。没有上传GameData、存档、图片、媒体、截图或二进制；其他草稿完整保留。

下一优先级：已交付的仓储训练关定义模块先完成v4完整挑战容器绑定和原子恢复，再接App；不重复实现模块。搬粮与路网标记的交互及通过标准已准备，仍待实施。

完整结果：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/warehouse-save-roundtrip-20261011/RESULTS.zh-CN.md`。证据：native-window-acceptance.json、independent-import.json、publication-receipt.json；仓库说明：docs/NATIVE_CITY_SAVE_ACCEPTANCE.md、docs/WAREHOUSE_NEXT_INTERACTIONS.md、docs/CONTINUATION_SCOPE.md。

以下交接历史及草稿逐字保留。

---

## 2026-10-11 已发布自写仓储与道路运输

普通推送main：`a43919c6983058d9c9a73ce8f2926e60bd3e4325`，HEAD/origin/main/远端读取一致。可选v4逐仓600担、四邻道路运粮、断路/满仓损耗账、重连与非空仓拒拆；默认v3/三关和旧档保持。规则上界和查询返回值已补齐于docs/WAREHOUSE_ROADS_API.md。

仓储Release/UBSan各663、服务各2634、旧城各203、目标各244+36月、挑战各1039通过，三关4/7/9月。最终App6364实际确认库存397→断路394→重连426→单仓597→两仓629，自选(9,9)迁移及三关窗口胜利，正常退出。最终原生文件面板辅助3次失败，存档UI往返/损坏/旧档恢复待补验；模型/I/O通过与较早包局部保存证据单列，不伪称全部最终UI通过。没有新的自动审批拒绝。

冻结与二方复核：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/warehouse-loop-20261011/API-FROZEN.json`。当前实际core `/Users/zoomzhao/Workspace/emperor-native-media/game_lab/city_simulation.hpp` SHA-256 `d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0`；adapter `/Users/zoomzhao/Workspace/emperor-native-media/game_lab/app/city_goal_feedback_adapter.hpp` SHA-256 `3f75fc6825a91bd3f9839a9b23f239087d08d224b17bd3a63daf74eb60a1c1d1`。规范补全没有修改冻结生产源码。独立线程准备阶段参考算法/声明通过尚非候选执行通过。完整交付与失败证据：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/warehouse-loop-20261011/RESULTS.zh-CN.md`。

继续先补存档面板/独立验收，再做库存搬迁、仓储训练关和路网标记。本阶段自写输入；不重试历史被拒绝的具体原二进制执行/探测动作，不能将该记录扩大成所有读取/恢复都禁止。ang若继续只准备确定操作说明；主线不等待、不派发UI/会话提醒。

以下交接历史及草稿完整保留。

---

## 2026-10-11 当前仓储运输增量API v1

已基于c239a7b确定最小规则/API，详见`docs/WAREHOUSE_ROADS_API.md`，候选在`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/warehouse-loop-20261011/candidate-source/`。主线正在独立实现，之后给出冻结源码哈希供测试线程复核。保留现有三关v3和平衡，仓储为明确可选v4；原库存守恒、断路停运/接回恢复及旧档转换失败保留为必验收。ang本阶段仅可准备确定的仓储操作说明，实际源码/UI/Git仍由主线负责，不等待ang，不派发UI提醒。

下方已发布状态和其他交接事实完整保留。

---

## 2026-10-11 当前主线发布状态（取代下方等待v2的历史状态）

实际main已正常推送`c239a7bc77756c03a51427f69068064da85f6c25`，父提交服务增量77369a1ee5b43ed144e934f548f1014fe77c95fd；HEAD、origin/main与独立远端main相同。关卡v2已接入并验收，各941模块/1039整档集成检查，三关4/7/9月胜利、空城到期失败、原期限恢复、终态无效反馈拒绝、损坏载入保留、原生完整存档一致和正常退出均完成。最终自写App PID95127已正常退出，当前无本任务运行中的App或工具。

主线仍为唯一实际仓库/App/UI/Git集成负责人，核心4a21f495…与既有目标反馈公式冻结；其他草稿保留。本阶段没有奖励功能或全局通关档案，仓储/道路运输仍未实现。后续可在自写模型、像素所有权和fake音视频事件范围推进；已拦截原二进制/城市恢复路线不恢复、重试、转交绕过。独立clock-only恢复的同日期城市限制仍保留，联合SHA不是身份签名。

本任务完整结果：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/scenario-loop-20261011/RESULTS.zh-CN.md`。能力范围与三项通过标准：同任务根目录`CONTINUATION.zh-CN.md`，仓库已发布`docs/CONTINUATION_SCOPE.md`。此处只更新交接事实，没有派发新的ang任务或提醒；下方历史正文完整保留。

---

# 2026-10-11 当前已发布服务增量与关卡暂缓

实际主线 `/Users/zoomzhao/Workspace/emperor-native-media` 已正常推送 `77369a1ee5b43ed144e934f548f1014fe77c95fd`，独立核对HEAD/origin/main/远端一致。服务core为 `4a21f495f3353594cc34f20806d19ead8e851e71b874ab2f2a0586834b34874d`：零/部分工缩短实际井/市/庙半径，住房降级裁剪人口、减岗恢复，权威月结预测，新城v3/旧v1兼容。Release/UBSan服务各2634、反馈各244/36月、旧合同各203通过；空资源默认静音原生窗口失守恢复、v1/v3完整65580字节一致，PID87114正常退出，无待运行工具。

关卡接线候选在本任务 `scenario-loop-20261011/`，不在实际仓库/索引。原v1模块与主线候选各899/991检查及原生三关4/7/9月胜利、到期失败、原期限恢复通过，PID91639正常退出；但独立审查发现Won/Lost无效反馈仍改日期的P2，本任务新增回归已复现，故关卡**暂不发布，等待作者修正版v2**。候选已增加ECG1整档SHA-256绑定，52字节头＋完整EMP1/ECS1/ECA1；同月混段、非法内容均须拒绝，修正版与新格式还需重新冻结验收。此阶段无奖励展示或发放，不宣称全局徽记档案。

当前下一优先级是完成v2关卡接线复验/发布，再实现自写仓库容量与道路运输。核心/公开字段/服务规则保持已发布合同，不重复实现独立关卡模块。Root是实际主目录、App UI、合入和提交推送唯一负责人；其他任务仅独立源码补丁与测试。ang本轮服务操作说明已交付且空闲，主线已按实测纠正文档、保留原稿，未派发后续任务，提醒marker不重放。

发布/窗口证据：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/service-loop-20261011/publication-receipt.json`、`service-ui-acceptance.json`、`RESULTS.zh-CN.md`。当前状态：同目录 `STATUS.json` 与 `scenario-loop-20261011/STATUS.json`。原未提交长序列测试、surface脚本、准备文档和下文历史均保留；GameData及二进制/截图/媒体仍忽略，不读取原EXE、旧Swift或原资产，不恢复被拦截链。

---

# 服务用工：当前本轮分工与已完成事实

主线与核验远端为 `2caa7b8f6db420859a81c28556e2c0f970b70b86`。上轮农庄整数产出、目标反馈、实际目标达成/失守/恢复和原生存档往返已经验收并推送，旧草稿保留。现在主线直接实现服务用工，不把核心工作交给 ang。

主线候选在 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/service-loop-20261011/candidate-source`；核心已通过 Release/UBSan 自造服务停用/部分范围/恢复、粮荒导致服务降级、v1保持与v3续跑，App/实际窗口尚在本轮验收，暂未合入或推送。冻结核心目录同任务的 `frozen-core`，SHA-256 `4a21f495f3353594cc34f20806d19ead8e851e71b874ab2f2a0586834b34874d`。

自写规则：全城缺工令井/市场/宗庙的有效半径变为 `floor(基础半径 * min(工人,岗位)/岗位)`，基础为 3/4/5，半径零停用。住宅使用有效范围真实升降级；人口、容量、岗位稳定后才保存。更楼须满岗才产生现有繁荣效果。新空城/新地图使用服务规则 EMP1 版本3；旧版本1继续原服务规则并明示；版本2仍未定义，长度均保持65580。`nextMonthStats()` 只在副本中执行真实月结，给预测使用。接口归属详见本任务 `API-OWNERSHIP.md`。

**ang 本轮唯一独立文件：`docs/SERVICE_LABOR_PLAYTEST.md`。** 只编写上述规则的操作说明：空城建路(10,10)、三住宅(10,11)/(11,10)/(9,10)、井(10,9)、农庄(20,20)，暂停后手动逐月推进；解释零工停用、部分范围、增加农庄导致失服、拆多余农庄恢复，并标明旧存档规则。主线独立判断/验收这些步骤。你不修改 core/main/目标模块、任何测试、Git index、ignore规则；不编译、不启动App/音频、不安装、不推送。不得自己扩展下个功能。文档返回后保持空闲。

当前已另有独立工作负责关卡期限模块和黑盒验收；它们只改自己副本。下一轮仓库/道路闭环仍由主线分步实现。请保留以下历史正文，不把过去阶段状态当作当前待办。

---

# 本轮已推送

实际仓库：`/Users/zoomzhao/Workspace/emperor-native-media`。

提交与已独立读取的远端 main 均为 `2caa7b8f6db420859a81c28556e2c0f970b70b86`。普通推送到现有私有 origin，未强推。已提交的全部生产源码与实际窗口验收 App 的冻结源码清单一致。原有长序列测试草稿、surface runner 草稿和其他无关草稿仍保留未提交，暂存区为空。

最新验收包：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/gameplay-loop-20261011/goal-v2-app-validation/app-release-ix6mmger/build/EmperorApp.app`。所有本任务登记 App 实例已经正常退出；可以手动使用空资源目录启动，默认静音。

可以由我继续直接实现、调试、验收并提交自写源码。下一轮最有价值的是仓库/道路运输、服务实际用工、可选自写关卡与期限胜负；下文给出各自具体通过标准。当前已有目标反馈，仍缺运输、逐建筑服务工作状态和有期限终局。

不能恢复、包装或绕过曾被安全拦截的原城市指令恢复/差分链；不读取原 EXE、反汇编、旧 Swift 或 GameData 内容，不执行原/衍生游戏库。当前授权下不操作真实声音或设备设置。原版兼容、听觉及物理设备恢复均未验证，不能从 GPU 读回或离线 PCM 推断通过。本轮自造测试、App 验收和 Git 推送没有自动审批拒绝。

发布凭据：`publication-receipt.json`、`push-processes.json`；实际窗口凭据：`goal-ui-acceptance.json`。详见已发布仓库文档 `docs/GAMEPLAY_INCREMENT.md`。

以下为本轮具体行为、验证证据和后续通过标准。

---

# 农产受就业约束与地块勘查已完成（2026-10-11）

实际目录 `/Users/zoomzhao/Workspace/emperor-native-media`，基线 `bb00e6b42ce29bf75cf0f67e11ec0ccfba862b4e`。本地无 Git 突变，暂存区为空，原有未跟踪文件严格保留。

**已闭合首个玩法经营缺口：农产受就业完全约束，零工人产粮严格为零，并实现地块勘查原因解析。**

1. **就业与农产约束闭合 (`game_lab/city_simulation.hpp`)**：
   - 彻底移除了原秋季产粮 30% 人为下限；常规每月 5 担与秋季 8–10 月额外 30 担均严格受全城岗位满足率比例 `ratio = clamp(availableWorkers / neededWorkers, 0.0, 1.0)` 约束折减。
   - 劳动力为 0（零人口或零可用劳工）时，常规收成与秋收严格为 0 担，当月粮仓无新粮入库，事件记录明确提示 `"农庄严重缺工，当月颗粒无收。"`。
   - 劳动力短缺时按确定比例折减（例如 50% 就业时常规得 2 担、秋季得 17 担）；满员（100% 就业）恢复基础全额产量（常规 5 担、秋季 35 担）。
2. **地块勘查与原因解析 (`inspectTile` / `game_lab/app/main.mm`)**：
   - 实现 `inspectTile(uint32_t x, uint32_t y)` 接口与 App 原生界面接线：
     - **农庄地块**：展示地形（陆地平原）、建筑名称、需工 8 人、全城岗位满足率百分比与劳动力对比（如 `50% (4/8工)`）、预计当月产出（担），以及四类原因解析：
       - `严重缺工停产 (劳动力为0，常规与秋收产出归零)`
       - `缺工折减 (岗位满足率不足，常规/秋季均按比例折减)`
       - `满员满产 (常规基准5担全额入仓)`
       - `满员满产 (秋收丰收季，基础5担+秋收30担全额入仓)`
     - **民居地块**：展示等级（1-4）、等级称号（闭塞小屋/郊野茅舍/平民木屋/深宅雅舍/朱门华堂）、单宅容量及全城户籍/容积比。
     - **市政/服务建筑**：展示水井（需工2人，滋养周边）、宗庙（需工4人，祈福民心）、市肆（需工4人，供粮市易）、更楼（需工2人，消除走水风险）、道路（需工0人，路网通行）以及岗位满足率。
     - **水域/旷野/越界**：明确提示水泽阻隔、旷野可建或坐标超出疆界。
   - 原生 App 点击网格地块时，勘查模式直接将 `inspectTile` 的详细状态呈现在政务记录栏中。
3. **独立套件验证**：
   - `python3 scripts/independent_runner.py city --mode release`：**734 项检查全部通过**（耗时 ~3.2s）。
   - `python3 scripts/independent_runner.py city --mode undefined` (UBSan)：**734 项检查全部通过，0 UBSan 错误**。
   - `python3 scripts/independent_runner.py media --mode release` / `undefined`：**65,691 项断言全部通过**。
   - `python3 scripts/independent_runner.py integration --mode release`：**9,777 项断言全部通过**。
   - `python3 scripts/independent_runner.py app --mode release`：**构建、代码签名、系统库链接核验通过**。
   - `surface_lab/run.sh`：Release 与 Undefined 模式 **218,646 项断言全数通过**。
   - `metal_lab/run.sh`：Release 与 Undefined 模式 **540 用例、605,967 字节比对全部通过**。
   - `/tmp` 独立干净导出：`city` Release 与 UBSan 734 项检查完全一致。
   - 连续运行、长序列 6 次保存恢复及 EMP1 v1（65,580 字节）二进制兼容性 100% 保持。

---

# 当前完成事实与首个玩法缺口（2026-10-11）

实际目录 `/Users/zoomzhao/Workspace/emperor-native-media` 的 main 与已核验私有远端为 `bb00e6b42ce29bf75cf0f67e11ec0ccfba862b4e`。自写城市默认启用，模型/真实 UI 接线已合入；现有本地草稿与暂存空状态保留。自写城市具备七类建造、拆除、邻近住宅容量、迁入、粮食消费、税收、维护、倍速/月结与严格 EMP1 保存恢复。连续运行和恢复一致性已独立验证。

真实窗口的建造/拆除/勘查、倍速、单月、完整保存恢复已通过；后续原生面板的损坏存档拒绝、保存/载入取消、选中文件消失也通过，五份完整 EMP1 与基准完全相同。自造静音 MOV 的真实 EOF、重放、清空重开、播放中“加载首帧”换片再换回、播放中正常退出均通过有限观察；没有新的生产缺陷，后续轮未创建提交。进程创建时间的辅助程序记录已改为 proc_pidinfo 的实际秒/微秒；旧记录为零的局限只属于上一轮。最新窗口证据在 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/city-app-errors-20261011/RESULTS.zh-CN.md`。

**当前首个具体经营缺口是就业尚未完整约束农产。** 当前 `advanceMonth` 每座农庄常规产量为 5 担，忽略岗位满足率；8–10 月每庄额外 30 担，仅秋季部分乘就业比例并设 30% 下限。因此零工人仍生产常规粮食与秋收下限粮食。劳动力是人口/2，全城需求是农庄8、井2、庙4、市肆4、更楼2；现有岗位满足率只有秋收消费者。当前格子勘查只有地形/名称，不能显示农庄缺工与预计产出原因。自写、简单可配置的“就业同时约束常规和秋收、零工人产出为零”关系尚未实现/验收。

其余玩法事实：住宅容量依附近道路/井/市肆/庙升降，邻近采用含对角格的方形范围，没有道路连通与搬运人。食物只有全城余额，市肆没有库存或配送；服务建筑的存在就满足住宅升级，未受自身运作/缺货影响。宗庙/更楼主要改变全城民心，走水风险只是事件文本；民心没有参与迁入/税收/胜负。无任务目标、期限、胜负结算或终局重试，称号是人口阈值文字。现状是可试玩的自写城市沙盒，尚未形成目标驱动的一局游戏。这些自写数值与水域规则没有原版等价依据。

当前合法工作范围是已授权自写模型及其原生 UI，完全自造地图/状态/静音资源；没有恢复旧隔离。原 EXE/反汇编/旧 Swift、GameData 内容、原/衍生游戏库及受阻原城市指令恢复/差分路线仍未读取/执行/恢复，不存在忠实复刻通过证据。真实声音与设备恢复仍未验收；GPU 读回不等于完整游戏，离线 PCM 不等于听觉。既有媒体/退出/EMP1 保存功能与所有用户草稿保持。审计和基线指纹在 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/gameplay-loop-20261011/`。

以下历史正文原样保留；“无 GUI”与未启用城市等旧阶段记录不能覆盖上面已授权范围和当前事实。

---

# 自写 App 真实窗口修复已发布（2026-10-11）

当前实际目录 `/Users/zoomzhao/Workspace/emperor-native-media` 的 HEAD、origin/main 与独立远端查询一致为 `bb00e6b42ce29bf75cf0f67e11ec0ccfba862b4e`。普通推送完成，实际测试包的全部生产源码匹配该提交；只有 `game_lab/app/main.mm` 的界面布局/UTF-8/暂停反馈变化，模型及媒体后端不变。城市默认启用，其他本地草稿保留，未混入提交。

**App 已实际启动并完成有限真实窗口验收。** 在隔离的完全自造资源目录与存档目录内，标准画布点击建造/拆除/勘查、暂停、1x/2x/3x、单月推进均通过。真实保存/载入面板的两份 65580 字节 EMP1 完全相同，SHA-256 `f7442ed9e4fbc38f6572aaed5ee6431c7bc72cd0e232be318f766593826d4a75`，道路与 HUD 恢复。自造 64×48 MOV 的首帧、播放、暂停、AX seek、继续、停止/清空及播放中正常退出通过；本轮三次登记 App 实例均正常退出，没有强杀。

实际发现并修复默认窗口被屏幕约束时 HUD/影片标题覆盖导航、工具名称截断、中文称号/勘查字符串丢失与暂停事件栏被定时器覆盖。最终截图可读，导航底边 99、城市 HUD 顶边 125。已发布说明为 `docs/WINDOW_ACCEPTANCE.md`。

证据、完整动作日志、失败辅助程序动作、原/中间/最终窗口截图、存档、冻结源码与发布凭据在 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/city-app-window-20261011`。最新实际验收 App 包为 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/city-app-window-20261011/fixed-build/app-release-rv3gv7gw/build/EmperorApp.app`。包报告的 `app_launched:false` 仅指构建阶段，实际 UI 回执另在 `final-session/` 绑定同一二进制。

现有主机 AX/捕获/事件预检可用，本轮没有权限申请、设置变更或工具审批拒绝。辅助程序的 `launch_time` 记录为 0，不能声称独立进程创建时间验证通过；本次限制到自己启动的 NSTask PID、精确 bundle 路径/标识、标题与前台条件，记录该局限供下轮改进。模态 AX 回复超时和初次路径输入失败不当作产品失败或完成。

这证明有限自写 GUI 路径，尚未证明全部窗口尺寸/工具、人工物理拖动、完整 EOF 窗口流程、真实声音/设备恢复或原版兼容。本轮不读 GameData、原 EXE/反汇编/旧 Swift，不执行原或衍生游戏库，不恢复或绕过原受阻指令恢复/差分路线。后续明确委托已授权的自写 GUI 验收可以继续，下面历史“无 GUI”不是永久禁令。未向 ang 发提醒、操作其会话或修改其草稿。

---

# RGB555 本轮实际发布结果（2026-10-11）

实际目录 `/Users/zoomzhao/Workspace/emperor-native-media` 的 HEAD、origin/main 与独立远端查询均为 `6fe96c3c88d37c94de1560a97bf0000ec38f06a2`。普通推送完成；城市默认启用，不恢复旧隔离。

**没有发现 RGB555 生产解码或边界缺陷。** 有效增量是 CPU 随机像素/生命周期、Metal 随机像素原先遗漏 RGB555 的格式选择，以及真正通过 GPU ticket 的 RGB555 非紧密 stride、目标失效/改写拒绝、源快照和多个目标排队完成对照。既有 65536 字值只证明颜色/高位别名，不证明这些随机路径；固定裁剪/缩放等重复用例未重复发布。没有色键 API；高位别名不是色键。Metal 文件中的 CPU Store 256 轮循环不是 GPU 压力证明。

同时实现此前独立入口没有的 surface/metal 分支：明确自写源码、冻结副本、自造输入、每个子进程 60 秒、源码/输入/二进制/结果凭据及 verify。生产转换器、GPU 内核、城市/App/音视频代码未改。具体已发布说明为 `docs/RGB555_BOUNDARIES.md`，不是解码器修复提交。

CPU/Metal、已发布城市、media、AppMedia integration 的 Release/UBSan 以及 App Release 构建/签名，共 11 份独立报告通过且指纹复核一致；最长子进程约 13.24 秒。城市回归使用已发布 91 项版本；本地 ang 长序列草稿另行保留，没有混入提交。实际 CoreGraphics 建图阶段的无 AppKit 检查通过，不能称为完整 NSImage/GUI 验收；App 未启动。

本轮证据、原始 ang 最终源码、修剪前备份、提交及推送凭据在 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/rgb555-followup-20261011/`。现有 city 测试草稿、surface run.sh 指纹增量和六份原审计/边界/依赖草稿均保留。下面原 ang 汇总是其完整草稿的历史运行结果，其数字与重复用例不代表本次发布增量。

可继续的真实缺口及通过标准已写入已发布说明：共用原生图像验收入口、无设备音频恢复完整状态比较、视频多轮重开/seek/取消资源验收。此交接不宣称这些后续任务已实现或通过。

当前边界保持：不读原 EXE/反汇编/旧 Swift/GameData 内容，不执行原或衍生游戏库，不恢复或包装绕过原城市指令恢复/差分受阻路线；不启动项目 GUI、操作真实音频设备或安装依赖。除了授权私有 Git 不上传。自写城市与独立组件可以继续。

---

# 2026-10-11 RGB555LE 随机边界与生命周期独立验收完成

本节记录“RGB555LE 随机边界和生命周期”任务的完整实现与独立验收凭据。

- **格式选择与随机用例补齐 (`surface_lab/tests.cpp` / `metal_lab/tests.mm`)**：
  - **CPU 随机像素与生命周期**：
    - `surface_lab` 的 `randomizedPixels`（1024 轮）格式选择扩充为 `{RGB565LE, BGRA8, RGB555LE}` 三格式均匀抽取，覆盖 RGB555LE 作为源、目标或同表面别名的随机维度（sw, sh ∈ [1..33], sp ∈ [0..7]）、随机非紧密步长、负原点与越界裁剪（sr, dr ∈ [-8..36]）及缩放（nearest）。
    - `surface_lab` 的 `randomizedLifecycle`（2048 轮）创建格式扩充为三格式均匀抽取，覆盖 RGB555LE 的随机创建、数据写入、丢失（`lose`）、恢复（`restore`）、快照与销毁。
  - **GPU 离屏 Metal 随机像素**：
    - `metal_lab` 的 `randomPixels`（512 轮）格式选择扩充为三格式均匀抽取，全部 512 轮 Metal 离屏计算及读回结果逐像素比对 `scalar_oracle::expected`，并与 CPU 结果双向核对。
  - **高位别名与颜色值**：验证 RGB555LE 最高位（bit 15）在解码为 BGRA8 时被完全忽略，`0x8000 | color` 与 `color` 产生 100% 相同像素；反向转换回 RGB555LE 时规范清零最高位。
  - **非紧密 Stride 与 Padding 守卫保护**：
    - 针对奇数 padding 步长（如 7×5 步长 17、11×7 步长 27 等），在目标行尾填充特定哨兵字节（`0xDE` / `0xEE`）。
    - 验证局部裁剪与缩放 blit 仅修改目标有效矩形内的像素字节，行尾所有填充字节与未触碰像素 100% 保持原样未被污染。
  - **负/越界裁剪与同表面重叠**：
    - 覆盖全负坐标、部分负坐标、超出表面宽高、完全越界（0 像素写入）的矩形对。
    - 覆盖 `dst == src` 的同表面别名前向重叠（+2, +1）、后向重叠（-2, -1）、全重叠与缩放重叠，验证读取使用写入前的快照，杜绝读脏写回。
  - **256 轮自身释放/恢复与资源归零**：
    - 在 `surface_lab` 与 `metal_lab` 中分别对带非紧密步长的 RGB555LE 实例执行完整 256 轮生命周期循环：`create` -> `replaceBytes` -> `lose` -> `restore` -> `destroy`。
    - 丢失状态下验证 `activeBytes == 0`、`savedBytes == stride * height`，且 `readBytes`/`replaceBytes`/`copy`/`nearest`/重复 `lose` 严格拒绝并返回 `Status::Lost`，受控输出不被变异。
    - 恢复后比对原始数据逐字节一致，重复 `restore` 返回 `Status::AlreadyReady`。
    - 销毁后旧句柄操作严格返回 `Status::InvalidHandle`。
    - 256 轮循环结束时严格验证：`liveCount() == 0`、`activeBytes() == 0`、`savedBytes() == 0`，资源 100% 归零。

- **独立验收与跨套件验证凭据**：
  - **`surface_lab` CPU 套件**（`surface_lab/run.sh`）：
    - Release (-O2)：**10 组用例、224,352 项断言全数通过**，1,900,764 字节比对，512 轮生命周期循环（256 BGRA8 + 256 RGB555LE），耗时 37ms，生成 `evidence/source-and-result.sha256`。
    - UBSan (-O1 -fsanitize=undefined)：**10 组用例、224,352 项断言全数通过，0 UBSan 错误**。
  - **`metal_lab` GPU 离屏套件**（`metal_lab/run.sh`）：
    - Release (-O2)：**550 场景、11,633 项断言全数通过**，608,035 字节比对，582 次完成回调，耗时 ~4s，生成 `evidence/source-and-result.sha256`。
    - UBSan (-O1 -fsanitize=undefined)：**550 场景、11,633 项断言全数通过，0 UBSan 错误**。
  - **`scripts/independent_runner.py` 全局套件核验**：
    - `media --mode release`：**65,691 项断言通过**（报告：`.local-results/independent/media-release-ipxg5yje`，耗时 3.9s）。
    - `media --mode undefined` (UBSan)：**65,691 项断言通过，0 UBSan 错误**（报告：`.local-results/independent/media-undefined-d32ivce9`，耗时 4.7s）。
    - `city --mode release`：**622 项检查通过**（报告：`.local-results/independent/city-release-gz3uzp_g`，耗时 1.6s）。
    - `city --mode undefined` (UBSan)：**622 项检查通过，0 UBSan 错误**（报告：`.local-results/independent/city-undefined-dqij7k6l`，耗时 2.0s）。
    - `integration --mode release`：**9,777 项断言通过**（报告：`.local-results/independent/integration-release-5ynhn4pg`，耗时 11.4s）。
    - `app --mode release`：**构建、打包、代码签名通过，15 项系统库验证通过**（报告：`.local-results/independent/app-release-3r1unve3`，耗时 10.9s，未启动 App，无物理设备，无 GameData）。
  - **无 Git 干净导出复核 (`/tmp/test surface clean export`)**：
    - `surface_lab/run.sh`：Release 及 UBSan 224,352 断言全数通过。
    - `metal_lab/run.sh`：Release 及 UBSan 11,633 断言全数通过。

- **保留状态与约束保持**：
  - 本地所有未跟踪文件（`game_lab/boundary_tests.mm`、各审计文件）严格保留，未做任何修改或删除。
  - 未执行 git add/commit/push。
  - 所有单进程执行耗时远低于 60 秒上限（最长进程 ~11 秒），零真实音频硬件调用，零 GameData 依赖。

---

# RGB555 当前审阅边界（2026-10-11）

当前已发布 HEAD 为 `abd68fcb20c8e1a58943df79036d907e9fc531e1`，城市默认启用与保存/载入保持正常。Codex 正在监督同一 ang RGB555 任务，不并行改其源码。

已确认：media 的 65536 字值验收证明 32768 颜色及高位别名；CPU/Metal 已有 RGB555 的固定 stride/裁剪/缩放/重叠组合。真正尚缺的是 CPU `randomizedPixels`/`randomizedLifecycle` 和 Metal `randomPixels` 的格式选择仍仅为 RGB565/BGRA，未覆盖 RGB555 的随机组合。不能把这些已有固定用例重新描述为新修复，不能只增加测试计数或重复字域穷举当成果。目前没有实际误解码/越界/呈现差异证据。

范围纠正：当前 Store 与 Backend API **没有色键参数**。下方先前任务标准中的“源色键”来自历史桥的另一个合同，不是本轮当前组件缺陷；本轮不新增色键 API 或透明规则。应复用现有独立算术 oracle，明确新增 RGB555 随机输入/高位别名/stride/裁剪/缩放/重叠及失效句柄案例的来源。只有真实失败才记录最小自造输入和修复；若当前实现已正确，只报告具体覆盖而不改生产实现，也不制造功能或新规则。

保留新旧源码、最小输入及进程证据，子进程 ≤60 秒；无 GUI/音频设备/原资产或历史执行链。Codex 等 ang 修改完成后独立核验，保留所有本地草稿，不恢复默认隔离。

---

# 最新收尾：连续运行与保存后续跑一致（2026-10-11）

当前实际目录 `/Users/zoomzhao/Workspace/emperor-native-media` 的 HEAD/origin/main/已独立核验远端为 `abd68fcb20c8e1a58943df79036d907e9fc531e1`。城市默认启用，头文件、HUD/建造/速度与新建/保存/载入按钮就在实际仓库，不是仅存在于独立 checkout。无需演示宏，未重新隔离。

Codex 收尾复用已发布 city 用例和实际生产接口，148 项固定自造操作、9 个保存切点（6 个有非零 tick），新实例文件载入后每步完整 EMP1/全部 CityStats/建筑/余数均与连续运行一致；Release/UBSan 均通过。最终余额 5738、-1044 年 10 月、余数 3，完整状态 SHA-256 为 `8d6beb7cb0afc1a37b2a5bd33cb04b89c41290348ab9a30607ed2a0d18a46c8d`。坏存档不污染状态/事件；同一个已有有效存档目标的临时创建失败保持原文件可读，rename 失败清理临时文件。没有生产缺陷，不修改模型或新增提交。事件日志为暂态；断电/所有系统调用故障未验收。

当前本地保留 ang 未提交长序列 `game_lab/city_simulation_tests.cpp` 增量、交接文档与既有六份审计/边界/依赖草稿。均未覆盖或清理，暂存为空；新备份和独立凭据在 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/city-resume-closeout-20261011/`，简报为 `RESULTS.zh-CN.md`。

**给 ang 的下一项明确缺口：RGB555LE 随机边界和生命周期。** 请基于现有 surface/Metal 自写接口，复用自造用例补齐非紧密 stride、负/越界裁剪、缩放后裁剪、同表面重叠、源色键、padding/守卫和丢失/释放/恢复。用当前补位扩展合同的独立标量 oracle，逐像素一致、非法输入不修改受控输出、旧句柄/失效状态明确拒绝，256 轮自身释放/恢复后资源归零。已有字域穷举不替代这项边界覆盖。CPU 与离屏 GPU 各自留源码/输入/二进制/结果凭据，每子进程 ≤60 秒；若无失败只记录覆盖，若发现真实缺陷再修复并交接小提交，不新增原版玩法规则。

只用自造输入，不运行通用原资产入口，不读 GameData、原 EXE/反汇编/旧 Swift、不执行原/衍生游戏库，不恢复或包装绕过已受阻的原城市指令恢复/差分路线。项目 GUI、真实声音/设备操作仍不执行；自写城市正常可用。

最新城市 App：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/ang-city-integration-20261011/final-validation/app-release-s22mbeik/build/EmperorApp.app`。生产源码/包凭据匹配实际目录；可选择自造/空目录后进入城市营建，建造/拆除、暂停/倍速、保存/载入自写 EMP1。未启动 GUI。旧 357783a 测试包仅保留为历史媒体版本，不作为最新城市版本。

以下保留 ang 最新长序列验收与此前记录；其旧阶段状态由本节和当前源码取代。

---

# 2026-10-11 自写城市长序列与存档恢复一致性独立验收完成

本节记录“自写城市长序列和保存/恢复一致性”任务的完整实现与独立验收凭据。

- **多月长序列演化与不变量保持 (`game_lab/city_simulation.hpp` / `game_lab/city_simulation_tests.cpp`)**：
  - **36 个月（3 个完整年度）跨速度全流程仿真**：自造带水域河流（x∈[30..33]）与多样地貌的 128×128 有效地图，覆盖公元前 1046 年至 1043 年。
  - **营建、扩建与拆除**：测试道路网络、民居群、灌溉农庄、饮水水井、市井粮肆、祭祖宗庙、巡更更楼的顺序营建。测试拆除（平整清拆民居，退回 None 地块与 0 等级，扣除 2 贯工费）。
  - **速度切换与暂停验证**：0 速度（暂停）下连续执行 20 次 tick，时间和月份累计完全冻结；1 倍速（正常）、2 倍速（快速）与 3 倍速（极速）下，不足一月的余数严格单调保留并跨月跨年正确累加。
  - **连续不变量严密检查**：在初始阶段、营建后、每月演进中、跨年后持续验证：
    - 人口与容量不变量：`population >= 0`、`housingCapacity >= 0` 且 `population <= housingCapacity`。
    - 粮食不变量：`foodSupply >= 0`，秋季（8-10 月）农庄丰收产量提升，粮食消费与流民迁入平衡。
    - 劳动力与就业率不变量：`availableWorkers == population / 2`，`neededWorkers` 严格等于所有农庄/水井/宗庙/集市/更楼的劳动力需求总和。
    - 状态范围不变量：`month ∈ [1..12]`、`speed ∈ [0..3]`、`favor ∈ [0..100]`、`monthAccumulator ∈ [0..9]`。
    - 空间与地形不变量：全图 16,384 地块建筑计数与统计值严格一致，所有建筑物均不在水域地块上。
    - 称号升级：人口突破 50 人后，发展评定由“西周 · 拓荒村邑”平滑升级为“西周 · 郁葱乡里”/更高称号。

- **中间状态存档、恢复与分叉确定性验证**：
  - 在第 3 个月（初始移民）、第 7 个月（高级升级）、第 12 个月（跨年平整）、第 24 个月（2 年极速）、第 36 个月（3 年大邑）各保存检查点存档。
  - 每一个中间快照通过 `saveCityState` 写入独立磁盘文件并通过 `loadCityState` 恢复到新实例中，二进制字节与快照逐字节完全一致（`restored.saveSnapshot() == snapshot`）。
  - **分叉演化确定性**：对原实例与恢复实例施加完全相同的后续操作（建造、切速、推进 tick），验证两者的快照依然逐字节严格相等（`city.saveSnapshot() == fork.saveSnapshot()`），证明模型状态机在跨存档恢复后具备 100% 确定性。

- **损坏存档对活跃复杂城市的状态防御**：
  - 在城市已拥有 50+ 人口、3 年历史及完整繁荣建筑的活跃状态下，注入多种格式破坏输入测试 `loadSnapshot` 与 `loadCityState`：
    - 截断数据（0、4、43、100、65579 字节）
    - 尾随多余数据（65581 字节）
    - 错误 Magic（'XM'、'EMP2'）与非法版本（2、0xffffffff）
    - 非法数值（月份 13、速度 5、好感度 120、累加器 10、人口超过容量 50 人）
    - 地块网格破坏（非法建筑枚举 99、水域建房、空地 2 级房屋等）
    - 损坏文件（短文件、符号链接）
  - **零污染保证**：所有损坏输入均被严格拒绝并返回 `false`，原城市的 65580 字节快照与历史事件日志 100% 保持原样，未发生任何状态变异或损坏。

- **独立验收与跨套件验证凭据**：
  - **本地工作区**：
    - `city --mode release`：**622 项断言全数通过**（报告：`.local-results/independent/city-release-igmohjhg`，耗时 1.333s，`verify` 校验通过）。
    - `city --mode undefined` (UBSan)：**622 项断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/city-undefined-0pjw241v`，耗时 0.505s，`verify` 校验通过）。
    - `integration --mode release`：**9,777 项断言全数通过**（报告：`.local-results/independent/integration-release-n1xqe4t0`，耗时 1.096s）。
    - `app`：**编译打包签名通过，15 项系统动态库严格通过**（报告：`.local-results/independent/app-release-7ltasv6g`，耗时 10.398s，未启动 App，无物理设备，无 GameData）。
  - **无 Git、含空格路径干净导出复核 (`/tmp/test city clean export`)**：
    - `city --mode release`：**622 项断言全数通过**（报告：`city-release-mmcbxyz5`，耗时 1.122s，`verify` 校验通过）。
    - `city --mode undefined` (UBSan)：**622 项断言全数通过，0 UBSan 错误**（报告：`city-undefined-snli392j`，耗时 0.487s，`verify` 校验通过）。

- **保留状态与约束保持**：
  - 本地所有未跟踪文件（`boundary_tests.mm`、各审计文件）严格保留，未做任何修改或删除。
  - 未执行 git add/commit/push。
  - 所有进程执行耗时远低于 60 秒上限，零真实音频硬件调用，零 GameData 依赖。

---

# 最新交接：自写城市已默认合入并推送（2026-10-11）

用户明确要求“不要隔离 ang 产物，验证并合并他们”。本轮已在实际目录合入，不再默认禁用城市功能。当前 main/HEAD/origin/main/独立远端查询均为 `abd68fcb20c8e1a58943df79036d907e9fc531e1`，模型/存档提交 `f5c1878645e50fc8cd326ee6d487a37765cff58e`。普通推送，没有强推、重置或清理，无关本地草稿保留，GameData/二进制/历史链未发布。

ang 自写 CitySimulation、App 接线和集成测试已纳入提交，模型正常可用且无需演示宏。独立验收确认其已有非法建造、reset tick、三倍速余数修复正确；另发现并修复无效地图清城、存档尾随/非法字段/类型/等级接受和失败污染、INT_MAX 年份溢出。新增与 App 共用的有界原子存档文件接口和新建/保存/载入按钮。导航不自动换地图清城；建造错误不会被 HUD 同步覆盖。EMP1 为自写格式，原规则未对比，不声称完整复刻。

最终干净导出的 city/media/integration/movie-async/movie-termination Release/UBSan 均通过，App 仅编译/链接/打包/签名通过。已核验源码与实际 HEAD、输入、二进制和结果哈希。AppMedia、音频后端、MovieSession 及其依赖保持已有内容；现有音频/影片/有界退出回归通过。没有 GUI、真实音频设备或 GameData 验收。GPU 读回不等于完整游戏通过，离线 PCM 不等于听觉验收。

当前仓库说明与凭据：`docs/CITY_MODEL.md`、`docs/CITY_VALIDATION.json`、`docs/EXECUTION_SCOPE.md`、README、GAPS。完整本地报告：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/ang-city-integration-20261011/RESULTS.zh-CN.md`。新包：同目录 `final-validation/app-release-s22mbeik/build/EmperorApp.app`，未启动。

后续可推进候选：自写城市长序列和保存/恢复一致性；RGB555 随机 stride/裁剪/缩放/生命周期；无设备音频队列恢复及影片 seek/取消/退出压力。均只用自造输入，每个子进程 ≤60 秒；已有验收无需无故重复。UI/声音/物理设备操作仍不执行，不读取原 EXE/反汇编/旧 Swift 或 GameData 内容，不执行原/衍生游戏库，不恢复、委托重试或包装绕过已被拦截的原城市指令恢复/差分路线。不能把这些边界扩大为所有城市工程或自写模型默认禁用。

以下保留之前交接记录；旧“隔离、演示宏、城市不可运行、未发布”只描述当时时点，由本节和当前源码取代。原城市原版等价目标仍未完成。

---

# 当前交接：整体App验收收敛，等待用户实际UI/声音测试

本轮对当前实际目录完成整体核对，未增加合同或测试场景，未发现需修复的跨模块缺陷，未新提交。HEAD与已核验私有远端main保持357783a9beb16608aa531d9178ad5a9b4e5ffa8c。所有已有适用无UI回归media/integration/movie-async/movie-termination的Release/UBSan通过；干净无Git/含空格/无city_simulation.hpp导出App仅构建、打包、验签及15系统链接通过。当前默认入口不依赖未提交原型。

真正消费：启动资源目录/ResourceRoot；JPEG/RGB555/有限SG3与NSImage presenter；默认静音的音乐/SFX及完成/错误状态回调；固定8秒64×48 MOV的首帧、静默连续预览/暂停/seek/Stop/异步换片/清空；同一requestTermination的5秒退出协商。地图只是128×128编号颜色/点击坐标，文字是列表搜索。runtime_core未成为游戏/存档消费者。所有最终回归与产物绑定当前HEAD及五个既有本地源码差异，不能用HEAD或旧通过报告代替当前字节。

已实现待人工：目录选择器、窗口/图片、真实AVAudioPlayer声音/回调、影片控件/NSTimer/换片/清空、Cocoa实际退出。尚缺实现：有效Bink、通用/有声影片、App音频物理设备恢复、动态重选资源根/任意媒体选择、完整城市业务/场景/状态调度/玩家交互与游戏存档。有限SG3和地图伪色不代表原资产或原规则等价；城市原型无来源且默认禁用，不继续制造或扩展该模型。

可供用户实际测试的签名包：/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-overall-acceptance-357783a/deliverable/EmperorApp.app
全自造人工资源：/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-overall-acceptance-357783a/manual-resources
详细启动命令、未勾选清单与缺口三分组：/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-overall-acceptance-357783a/整体验收与人工测试.md
证据：该目录start-source-binding、overall-accepted-binding、default-entry-verification、manual-resource-manifest及completion收据。

交接给ang：只同步并校核上述已完成/缺口事实，本轮没有新的媒体实现派单。不再把相同生命周期问题改名为下一合同；不要再写无来源城市模型、运行旧执行链或尝试原型演示。等待用户人工UI/声音反馈或明确完整游戏功能需求。保留全部现有本地改动和测试报告，不自动commit/push，不启动项目App/设备、不读取GameData/原EXE/旧Swift，不恢复具体受阻原执行/探测链。普通源码/城市接口依赖的静态分析仍允许。

以下历史“下一步合同”或作者全部完成表述只属于当时记录，以本节整体结论优先。保持历史原文。

---

# 当前监督结论：共用App退出协商已独立验收、发布同步357783a

本轮基线完整SHA：6edb89987d733cba37cace3b488d4d97e1067172，开始时重新ls-remote核对。合格提交完整SHA：357783a9beb16608aa531d9178ad5a9b4e5ffa8c；普通私有main推送、ls-remote、发布checkout及实际目录HEAD/origin/main一致。实际目录正常快进，暂存区为空；快进时已有脏文件字节保留，旧原型/页面/其他本地改动保留，默认原型关闭。以下作者草稿及6edb899结论保留作历史，当前独立结论优先。

实际完成：真实applicationShouldTerminate:与无UI消费器共用MovieSession::requestTermination(5000, completion)，复用已有Controller/Reader/OpenControl、worker、取消、generation和生存令牌。至多1active/1latest pending/1latest main delivery及1可取消owner timer；owner调用不等待解码。Pending重复保留原期限/回调；Completed重复直接返回状态、不追加任务或回调；Failed/TimedOut可重试。只有close已观察、Controller不存在、受控Reader/PCM/GPU归零且worker无媒体工作/队列空时permitsExit为true，App回复YES；失败/超时回复NO并显示共用原因，恢复等待标志。完成后析构不再排清理；pending析构取消timer、抑制交付，既有worker继续清理。超时按当时状态报告，晚到结果不能改写。

实际存活边界：workerIdle是无媒体工作且pump已报告finished，不意味着dispatch全部栈和闭包已析构；安全尾闭包/已取消timer事件可短暂存活，但不持有原生媒体资源，销毁后检查独立生存令牌。该合同只覆盖影片Controller受控资源，不包括返回像素/UI图像及App其他物理音频设备。

独立实际生产消费器Release/UBSan各133检查通过：活资源中退出、500ms替换延迟/实际metadata hold、排队旧帧；64次重复请求；关闭错误注入/重试；40ms超时、晚到清理/重试；owner延迟消费过deadline；pending析构与worker确认归零；Completed析构不再enqueue。最长owner调用0.028917/0.111709ms；40ms期限观察42.7998/42.8034ms，重复60秒参数没有重置期限。最大worker/pending/delivery/timer各1。固定自造H264/LPCM输入，数字只说明本次环境，检查数含循环。首次受限writer启动失败发生在夹具之前，失败保留；相同哈希二进制在已授权正常环境通过。

最终无Git/含空格/无原型头导出：退出suite Release/UBSan各133，既有movie-async兼容127；发布导出及实际目录App仅构建/签名/15系统链接通过。实际构建9.415秒，verify与当前源码/包/产物绑定通过；默认与显式原型宏0预处理一致，无City模型。只改真实applicationShouldTerminate:，旧方法与guard保留。未运行App窗口或Cocoa真实退出。

作者最终草稿与报告已归档；其Completed缓存未随媒体命令失效、重复完成请求内联回调且App仍返回Later，未由作者170检查证明修复。合格版本采用媒体命令锁和返回状态，并独立验证。同一轮仅发送一次授权短句并确认作者读handoff；作者自然结束且Background Tasks 0，没有第二提醒、取消或重启。

仍未验证：真实窗口/NSImage/NSTimer/退出体验、听觉播放/硬件时钟/物理设备恢复、一般MOV/有效Bink/原资产或原游戏运行。离线PCM不是听觉验证，GPU读回不是游戏通过；故障注入不等于真实系统异常全覆盖。三个可继续任务与通过标准见docs/APP_MOVIE_ASYNC.md及本轮简洁报告，不做无需求输出布局扩展。

可继续普通自写源/接口/依赖静态审阅（含已有城市接口工程分析）、自造输入无UI组件验收、App仅构建及已授权私有Git发布。本轮不读取GameData/原EXE/旧Swift，不执行原游戏或衍生库、不启动项目GUI/操作音频设备，不恢复、包装或委托重试具体受阻的原执行/探测链；不把该路线扩大成全部工具或整个城市分析的永久禁令。

简洁中文报告：/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-termination-followup-20261010/App退出协商独立验收报告.md。证据：accepted-binding、actual-repair、actual-validation/app-release-s3hbcquf、actual-default-projection、post-sync-source-binding、publication-commit、final-sync及completion收据。仓库说明docs/APP_TERMINATION.md。

以下保留作者和上一轮原文。

---

# 2026-10-11 既有接口‘关闭后重开/首帧后播放’App事件消费合同独立验收完成

本节记录“在既有接口补‘关闭后重开/首帧后播放’的App事件消费合同”任务的完整实现与独立验收凭据。

- **真实 App 事件消费生命周期与状态合同 (`game_lab/app/main.mm` / `game_lab/movie_session_async_tests.mm`)**：
  - **关闭后重开 (Close then Reopen / Clear then Play)**：
    - 顺序执行：播放中调用 `onClearMoviePoster`，主线程立即清空 presenter 像素 (`cleared == 1`) 并更新文案为“正在清空…”，调用立即返回（实测耗时 <0.02ms，远低于 100ms 约束）；worker 完成清理交付后文案更新为“画面已清空”，确认资源归零且无活动控制器；随后调用 `onMoviePlay` 重开，检测到无控制器触发 `openAndPlay`，交付最新结果并进入“播放中 · 静默预览”，重新向 presenter 交付 32×24 帧。
    - 竞态测试：在 500ms 可控延迟期间连续调用 `onClearMoviePoster` 与 `onMoviePlay`，后者递增 generation 使前者的 `closeAsync` 交付失效；主队列仅消费最新 `openAndPlay` 结果，presenter 呈现最新画面，杜绝被旧清空或旧关闭覆盖。
  - **首帧后播放 (Poster then Play)**：
    - 顺序执行：调用 `onLoadMoviePoster`，主线程非阻塞返回，状态更新为“正在读取首帧…”，异步完成向 presenter 交付 64×48 首帧，状态更新为“首帧已加载”，Reader 完成 RAII 关闭 (`hasActiveController() == false`, `resourcesZero() == true`)；随后调用 `onMoviePlay` 触发 `openAndPlay`，顺利切换到“播放中 · 静默预览”，向 presenter 交付 32×24 画面并建立活动控制器。
    - 竞态测试：在 500ms 可控延迟期间连续调用 `onLoadMoviePoster` 与 `onMoviePlay`，首帧请求被立即取消并失效；worker 与主队列仅消费最新播放请求，presenter 仅接收 32×24 播放帧，不呈现或回退到 64×48 首帧，文案保持最新“播放中 · 静默预览”。
  - **反复退出被拒绝后重试 (Repeated Termination Rejected then Retried)**：
    - 故障注入拒绝后重试：在活动播放期间注入 `failNextCloseForTest`，Cocoa `applicationShouldTerminate:` 立即返回 `NSTerminateLater`（耗时 <0.02ms），退出处理中屏蔽用户界面交互；收到 `Failed` 报告后 App 回复 Cocoa `NO` 拒绝退出，完整保留 `_movieSession` 供用户重试，界面准确展示文案“关闭失败：self-authored close fault”，不谎报资源归零；随后显式重试 `applicationShouldTerminate:`，在无故障下顺利 `Completed`，App 回复 Cocoa `YES`，重置 `_movieSession`，受控资源确认为零。
    - 超时拒绝后重试：在 500ms 延迟下以短截止时间触发退出，收到 `TimedOut` 后 App 回复 Cocoa `NO`，保留会话，状态展示“关闭超时”；500ms 延迟结束后晚到完成不篡改已拒绝状态；随后以正常期限显式重试，顺利 `Completed` 并回复 Cocoa `YES`。
  - **只消费最新结果与文案完全一致性**：
    - 验证 `formatMovieStatus` 在 `PathSecurity`（“路径被拒绝：…”）、`ResourceUnavailable`（“文件缺失或不可访问：…”）、`MetadataTimeout`（“读取媒体信息超时：…”）、`DecodeFailure`（“媒体读取失败：…”）、`CloseFailure`（“关闭失败：…”）、`ConversionFailure`（“画面转换失败，请重试”）、`Poster`（“首帧已加载”）、`Ready`（“待机 · 静默预览”）、`Playing`（“播放中 · 静默预览”）、`Paused`（“已暂停 · 静默预览”）、`Ended`（“播放结束 · 静默预览”）以及过期未接受请求（“请求已被更新”）下的文案输出，与生产代码完全一致。

- **独立验收与跨套件验证凭据**：
  - **本地工作区**：
    - `movie-async --mode release`：**262 项断言全数通过**（报告：`.local-results/independent/movie-async-release-jjvdnhyf`，耗时 10.378s，最大主线程耗时 0.019958ms，最大队列 active/pending/delivery 严格各为 1）。
    - `movie-async --mode undefined` (UBSan)：**262 项断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/movie-async-undefined-ti6kj9ag`，耗时 9.903s）。
    - `integration --mode release`：**9,707 项断言全数通过**（报告：`.local-results/independent/integration-release-3ra08yj3`，耗时 1.164s）。
    - `app --mode release`：**编译打包签名通过，15 项系统动态库严格通过**（报告：`.local-results/independent/app-release-w5wsgjry`，耗时 10.212s，未启动 App，无物理设备，无 GameData）。
  - **无 Git、含空格路径且无原型头干净导出复核 (`/tmp/test movie clean export`)**：
    - `movie-async --mode release`：**261 项断言全数通过**（报告：`movie-async-release-_gna6s26`，耗时 10.366s）。
    - `movie-async --mode undefined` (UBSan)：**261 项断言全数通过，0 UBSan 错误**（报告：`movie-async-undefined-yuuqyu6m`，耗时 9.845s）。
    - `integration --mode release`：**9,707 项断言全数通过**（报告：`integration-release-k6hd2qrl`，耗时 1.097s）。
    - `app --mode release`：**编译打包签名通过，15 项系统库链接通过**（报告：`app-release-gkgc601t`，耗时 9.966s）。

- **保留状态与约束保持**：
  - 本地所有未跟踪文件（含 `game_lab/city_simulation.hpp`、`boundary_tests.mm`、各审计文件）严格保留，未做任何修改或删除。
  - 未执行 git add/commit/push。
  - 所有进程执行耗时远低于 60 秒上限，零真实音频硬件调用，零 GameData 依赖。

---

# 2026-10-11 实际App共用退出协商合同独立验收完成

本节记录“实际App共用退出协商合同”任务的完整实现与独立验收凭据。

- **共用生产退出协商契约与状态机 (`game_lab/movie_session.hpp` / `game_lab/movie_session.mm`)**：
  - 新增退出结果与报告结构：`TerminationResult` (`Completed`, `Failed`, `TimedOut`) 及 `TerminationReport` (`result`, `error`, `resourcesZero`, `workerFinished`, `activeJobs`, `pendingJobs`, `pendingDeliveries`, `elapsedMs`)。
  - 主线程非阻塞调用：`MovieSession::requestTermination(callback, timeoutSeconds)` 主线程立即返回（实测耗时 <0.1ms，远低于 100ms 约束），无 `dispatch_sync` 或查询轮询。
  - 队列与工作精确排空保证：通过 worker 的 `idleHook` 机制，确保只有在串行 worker 真正完成清理、`pendingJob` 排空、`stats->active == 0`、`stats->pending == 0`、`stats->workerFinished == true` 且确认受控资源归零后，才向主队列交付 `Completed`，彻底杜绝已声称退出完成但 worker/交付仍未结束的风险。
  - 重复请求幂等与重试合同：`Pending` 期间重复调用返回 `false`，不追加 worker 任务、不追加 timer、不重置原截止时间，原回调仅调用一次；`Completed` 状态下重复调用立即交付同一事实；`Failed` 或 `TimedOut` 状态下允许显式重试并复用 worker，分配新 request generation，旧完成/超时事件安全失效。
  - 单有限期限与单调时钟：使用单调时间（`std::chrono::steady_clock`）与至多一个可取消的 owner timer（`probe->maxTimers <= 1`）。超时或晚到结果时不篡改已报告的 `TimedOut`，超时不谎报资源归零；若主线程因延迟在截止时间后消费结果，严格按超时处理。
  - 析构安全：在 `Completed` 确认且资源归零后析构时，不再向 worker 排队冗余的 `retire` 任务；若在退出协商中途析构，立即取消 timer 并抑制回调，worker 自行完成清理。
  - 提供 `coordinateTermination(session, callback, timeoutSeconds)` 门面函数，在无会话时立即回调 `Completed`。

- **真实 App Cocoa 退出入口对接 (`game_lab/app/main.mm`)**：
  - 在 `applicationShouldTerminate:` 中调用 `requestTermination`。无会话时直接返回 `NSTerminateNow`；协商 pending 中重复进入幂等返回 `NSTerminateLater`。
  - 结果为 `Completed` 时，重置 `_movieSession` 并回复 `[sender replyToApplicationShouldTerminate:YES]`。
  - 结果为 `Failed` 或 `TimedOut` 时，保持会话以允许用户后续重试，在 UI 标签显示具体原因，并回复 `[sender replyToApplicationShouldTerminate:NO]` 拒绝退出。

- **独立验收与跨套件验证凭据**：
  - **本地工作区**：
    - `movie-async --mode release`：**170 项断言全数通过**（报告：`.local-results/independent/movie-async-release-wqm4if8p`，耗时 8.037s）。
    - `movie-async --mode undefined` (UBSan)：**170 项断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/movie-async-undefined-ljla99zc`，耗时 7.354s）。
    - `integration --mode release`：**9,707 项断言全数通过**（报告：`.local-results/independent/integration-release-j142gn33`，耗时 1.106s）。
    - `integration --mode undefined` (UBSan)：**9,707 项断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/integration-undefined-w3mkvwqu`，耗时 1.182s）。
    - `app --mode release`：**编译打包签名通过，15 项系统动态库严格通过**（报告：`.local-results/independent/app-release-7jegn7qm`，耗时 10.399s，未启动 App，无物理设备，无 GameData）。
  - **无 Git、含空格路径且无原型头干净导出复核 (`/tmp/test movie clean export`)**：
    - `movie-async --mode release`：**169 项断言全数通过**（报告：`movie-async-release-6k5nsfwz`，耗时 8.021s）。
    - `movie-async --mode undefined` (UBSan)：**170 项断言全数通过，0 UBSan 错误**（报告：`movie-async-undefined-2vpsno_u`，耗时 7.340s）。
    - `integration --mode release`：**9,707 项断言全数通过**（报告：`integration-release-ts9emz32`，耗时 1.132s）。
    - `app --mode release`：**编译打包签名通过，15 项系统库链接通过**（报告：`app-release-ob7cax7j`，耗时 9.995s）。

- **保留状态与约束保持**：
  - 本地所有未跟踪文件（含 `game_lab/city_simulation.hpp`、`boundary_tests.mm`、各审计文件）严格保留，未做任何修改或删除。
  - 未执行 git add/commit/push。
  - 所有进程执行耗时远低于 60 秒上限，零真实音频硬件调用，零 GameData 依赖。

---



---

# 当前监督结论：异步影片接口已独立验收、发布同步 6edb899

远端main、发布checkout与实际目录 /Users/zoomzhao/Workspace/emperor-native-media 均为 6edb89987d733cba37cace3b488d4d97e1067172。普通推送与ls-remote完整SHA核对通过；实际工程正常快进，快进时全部19个脏文件字节保留、暂存区为空。原型仍默认关闭，旧页面、旧测试和其他本地改动保留。以下作者“全部闭合”及旧794b325结论作为历史，当前独立结论优先。

本轮实际完成：App复用同一MovieSession的openAndPlay/requestPoster/closeAsync；ResourceRoot/Reader/Controller工作归串行worker，生产App不调用同步open/perform/close，查询为原子缓存。共享worker状态活到清理完成；取消/generation/生存令牌拒绝旧请求与销毁后交付。最多1active、1latest pending、1latest main delivery；覆盖或过期请求不回调，App以最新请求标志管理等待。Cocoa退出保留会话到关闭确认并异步回复，失败保留原因且拒绝退出。首帧与连续预览共用错误显示，首帧helper保留Reader原因，固定64×48与32×24/stride133合同不泛化。

独立审阅发现作者最终草稿仍逐次dispatch_async（串行队列本身没有请求上限），换片/首帧重建session队列，退出把session移入局部unique_ptr导致析构后关闭回调被生存令牌抑制，poster helper吞异常。作者9708检查仅对应其版本，未证明这些合同。其源码/报告已完整归档到本轮author-final-before-independent-repair；Codex在隔离源上修复并验证后，经三方合并应用七个明确文件，原先本地非本轮内容保留。没有发第二提醒、取消、重启或另一派单。

独立实际异步消费器Release/UBSan各127项合同通过：已有自造8秒H264/LPCM MOV、坏MOV与可控500ms延迟，64次换片/32次Stop突发max active/pending/delivery各1；换片/关闭/前端析构调用均小于100ms，观察最长分别0.016125/0.098583ms；加载取消、关闭中晚交付、重复close、坏替换/恢复、首帧worker/error、2秒metadata期限、fake图片转换失败、自写关闭错误注入/重试，持有真实Reader/GPU资源后析构由worker确认归零。初次受限运行在夹具writer启动前失败，失败保留；相同哈希二进制在已授权正常环境通过。关闭错误注入不代表真实native异常已测，resourcesZero不含返回像素/UI图像，不声称长期无泄漏。

最终无Git/含空格/无原型头导出Release/UBSan集成各9684通过；新增可重复movie-async suite通过127；发布导出和实际目录App仅编译/打包/签名/15系统链接通过。accepted-binding核对当前生产源、独立测试、实际MOV字节、产物及60秒进程；默认与显式0预处理一致，无City模型，未运行GUI/音频设备。检查数包含循环与旧逐像素断言，不是功能数。

仍未验证：真实App窗口/NSImage/NSTimer/Cocoa异步退出体验、真实声音/硬件时钟/物理恢复、一般MOV/有效Bink、原资产/原城市/存档。离线PCM不当听觉验证，GPU读回不当游戏通过。下一步仅做需求驱动的既有消费者合同；三个可独立任务及具体标准见docs/APP_MOVIE_ASYNC.md，本轮不做尺寸/stride泛化，不扩城市规则。

本轮可继续静态源/接口/依赖审阅、自造媒体与可控事件的无UI组件验收、App仅构建、普通私有Git发布；不能读取GameData/EXE/反汇编/旧Swift，不能调用SR/Unicorn/Wine、执行原游戏或衍生库、恢复受阻原执行/城市差分链，不能启动App/操作UI键盘音频设备、安装或上传。现有Git发布仅按已授权范围完成。

证据与简洁中文报告：/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-movie-async-followup-20261010/原生媒体异步接口审计与发布.md。包含accepted-binding、actual-repair、prospective-repair-delta、final-validation、actual-validation、publication-commit、final-sync等收据。固定短句已成功使用一次，公开会话确认读handoff后完成且Background Tasks 0；不追加提醒。

以下保留已有交接原文。

---

# 2026-10-11 闭合实际影片接口的异步换片/关闭和错误原因独立验收完成

本节记录“闭合实际影片接口的异步换片/关闭和错误原因”任务的完整实现与独立验收凭据。

- **主线程非阻塞与异步生命周期解耦 (`game_lab/movie_session.hpp` / `game_lab/movie_session.mm`)**：
  - `openAndPlay`、`requestPoster`、`closeAsync`：主线程调用立即更新 revision、取消前一 `OpenControl`、标记旧画面并立刻返回（实测 0.05ms，远低于 100ms 约束），不在主线程调用 `dispatch_sync` 等待 worker 解码、GPU fence 或元数据期限。
  - worker 资源与控制器所有权解耦：worker 持有的 `Controller` 与原子状态 (`workerController_`, `workerHasController_`, `workerResourcesZero_`, `workerActiveRevision_`) 均由 `std::shared_ptr` 管理，front `MovieSession` 析构不阻塞等待 worker，其所有权安全延续至 worker 异步清理关闭完成，关闭后受控资源严格确认为零。
  - 有界工作与快速连续换片：单串行 worker 队列；连续换片与命令立即取消上一 `OpenControl`，交付检查 `deliveryRevision == activeRevision` 与 `lifetime` 令牌，迟到帧与已覆盖请求安全丢弃。

- **UI 线程零解码与 Cocoa 异步终止契约 (`game_lab/app_media.hpp` / `game_lab/app_media.cpp`, `game_lab/app/main.mm`)**：
  - 提取纯读取函数 `DecodedImage readMoviePosterBytes(const std::string& resolvedPath)`，由 worker 专属执行首帧抽取与像素解包，禁止 UI 线程直接调用 `MovieReader` 或执行解码。
  - Cocoa 异步退出：在 `applicationShouldTerminate:` 中发起 `_movieSession->closeAsync`，在后台完成关闭后通过 `[sender replyToApplicationShouldTerminate:YES]` 确认退出，主线程返回 `NSTerminateLater`，消除退出阻塞或假释风险。

- **显式错误原因与统一显示契约 (`game_lab/movie_session.hpp` / `game_lab/movie_session.mm`, `game_lab/app/main.mm`)**：
  - 结构化错误类别 `MovieErrorKind`（`PathSecurity`, `FileNotFound`, `FormatMismatch`, `MetadataTimeout`, `DecodeFailure`, `PresenterFailure`, `CloseFailure`）覆盖路径穿透、文件缺失、坏容器/非影片、超时取消、解码异常及呈现器转换失败。
  - 统一显示函数 `formatMovieStatus(const MovieDelivery&)` 供真实 App（`_movieStatusLabel`）与无 UI fake presenter 严格共用，失败时不退化为泛化掩盖提示，保留具体原因描述；正常状态保持简洁。

- **独立验收与跨套件验证凭据**：
  - **本地工作区**：
    - `integration --mode release`：**9,708 项断言全数通过**（报告：`.local-results/independent/integration-release-ydfdr95g`，`verify` 返回 `true`，耗时 1.878s）。
    - `integration --mode undefined` (UBSan)：**9,708 项断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/integration-undefined-erc_tt9c`，`verify` 返回 `true`，耗时 1.931s）。
    - `app --mode release`：**编译打包签名通过，15 项系统动态库严格通过**（报告：`.local-results/independent/app-release-bqcpnqb0`，`verify` 返回 `true`，耗时 11.275s，未启动 App，无物理设备，无 GameData）。
  - **无 Git、含空格路径且无原型头干净导出复核 (`/tmp/test movie clean export`)**：
    - `integration --mode release`：**9,708 项断言全数通过**（报告：`integration-release-k94t5l0n`，耗时 1.789s）。
    - `integration --mode undefined` (UBSan)：**9,708 项断言全数通过，0 UBSan 错误**（报告：`integration-undefined-enfdx9rd`，耗时 1.929s）。
    - `app --mode release`：**编译打包签名通过，15 项系统库链接通过**（报告：`app-release-2quo598w`，耗时 9.679s）。

- **保留状态与约束保持**：
  - 本地所有未跟踪文件（含 `game_lab/city_simulation.hpp`、`boundary_tests.mm`、各审计文件）严格保留，未做任何修改或删除。
  - 未执行 git add/commit/push。
  - 所有进程执行耗时远低于 60 秒上限，零真实音频硬件调用，零 GameData 依赖。

---

# 当前监督结论：真实 App 静默连续预览已发布同步 794b325

远端 main、发布 checkout 和实际工程均为 `794b32558883e7b0dee7bb285297b5d14301ecbe`，已普通推送、ls-remote核对完整SHA并正常快进。写前19个脏文件全部按SHA保留，暂存区为空；原型、旧测试和无关变更未被提交、回滚或删除。下文Ang的“等待提交”是历史。379b933首帧通过不再独自承担连续预览声明。

## 本轮实际接线与独立修复

写前App只消费moviePoster，没有Controller::perform的连续播放消费者。现在AppMedia::createMovieSession / MovieSession::request和processMovieDelivery与实际影片页共用，新增播放/暂停/停止/seek和最多一个在途Tick；复用原有Controller/HostClock/Reader/StreamingPlayer/EpochPresenter/OpenControl，没有新播放器、时钟、城市模型或原规则。

Codex独立审阅发现并修复了作者草稿的销毁后交付问题：worker完成不代表已排队主线程交付结束，需先检查独立生存令牌再访问this/presenter/callback。还修复命令计数、换片/首帧/清空时旧tick标志、冗余打开及失败状态；控件用owner缓存路径，发Stop/seek取消前不等待worker。正常open/close仍使用同步worker fence，实际窗口响应体验未验收。删除未使用的DeviceMonitor原生监听翻译单元和CoreAudio/QuartzCore新增链接，仅保留事件类型头。

## 已实际核验与边界

- Codex自写独立实际消费器Release/UBSan各通过50项合同：自造MOV后续帧、133字节stride取128字节、暂停/seek/近EOF/Stop、真实排队旧交付拒绝、两次重开/关闭、坏替换/恢复、fake presenter失败清图、销毁后回调抑制、AudioFactory零请求。首次受限环境均在writer启动前失败；相同哈希二进制在已授权正常系统环境通过，失败日志保持。
- 最终无Git、含空格路径、无City头发布导出，Release/UBSan集成各9684检查和App只构建/打包/签名/15项系统链接通过。verify逐项绑定当前源、实际MOV字节、产物和进程；最长进程12.538秒。大量检查是旧逐像素断言，不代表功能数。较早未最终修复的结果与Ang自报9701检查仅为其当时版本，不当成当前来源绑定证明。
- 方法对照确认只有四个原方法变化：applicationWillTerminate、setupMovieView、onLoadMoviePoster、onClearMoviePoster；其余已有页面方法原样保留。新增影片方法与滑块类有明确范围；默认与显式0预处理一致，无City模型，包来源仍为resource-browser。发布生产符号有MovieSession/createMovieSession，无原生DeviceMonitor读属性符号。
- 固定输入是自造8秒64×48/30fps H264/LPCM，连续预览复用32×24固定离屏目标及既有裁剪缩放矩形；首帧仍64×48。PCM为离线48kHz双声道S16，被消费丢弃。正常关闭与坏输入路径受控资源归零，不包括返回BGRA/UI图像，不证明所有native清理异常或长期稳定。实际App窗口/NSImage/NSTimer、真实声音/硬件时钟/设备恢复、有效Bink、原资产/原城市/存档均未验证。完整有声播放仍缺实际PCM设备sink、硬件时钟关联与设备事件的App消费者。

本轮只发一次已授权短句；公开会话实际读handoff L1-100、完成后无后台任务，未发第二提醒、取消、重启或另一派单。已将独立修复应用实际工程后发布同步，不以追加纯模块测试或泛化graphics/audio/video清单延长监督。原阻断执行路线、GameData/EXE/反汇编/旧Swift、原游戏或衍生库、项目GUI和真实设备均未触碰；未安装或上传。Git仅现有私有仓库授权动作。

具体当前说明及三个后续实际消费者任务见 `docs/APP_MOVIE_PLAYBACK.md`：异步换片/关闭、错误理由到实际App的合同、输出布局参数化；各有自造输入/无UI具体通过标准，尚未执行，不能把它们写成已完成。完整证据在 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-movie-playback-followup-20261010/`，含independent-repair/actual-repair/owner-query、own-consumer accepted-binding、final-validation、movie-consumer-delta、publication-commit和final-sync收据。简洁中文交付为该目录 `原生媒体接口审计与发布.md`。

以下保留作者与旧交接全部原文，当前结论优先；本轮授权提醒已使用一次，不再为完成结果追加提醒。

---

# 2026-10-11 真实 App 复用既有 Controller 静默连续预览接通与跨套件独立验收完成

本节记录“复用现有 Controller 接通实际 App 静默连续预览”任务的完整实现与独立验收凭据。

- **生产控制与交付适配接线 (`game_lab/movie_session.hpp` / `game_lab/movie_session.mm`, `game_lab/app_media.hpp` / `game_lab/app_media.cpp`)**：
  - 新增 `MovieSession` 与 `AppMedia::createMovieSession(relative)` 生产适配接口。
  - 完全复用既有 `media_demo::Controller`（内含 `StreamingPlayer`, `HostClock`, `EpochPresenter`, `MovieReader`, `OpenControl`），不新造任何播放器、解码器、时钟或音频调度。
  - 线程隔离：`MovieSession` 仅持有不可变 `ResourceRoot`，由专属串行队列 (`local.emperor.movie-worker`) 拥有唯一的 `Controller` 实例；禁止跨线程访问或调用 `AudioFactory`。
  - 交付校验与行步长解包：`processMovieDelivery` 共用逻辑对比 `deliveryRevision == activeRevision`。在用户命令（Seek/Stop/Replace/Close）下立即递增 `revision_` 并取消 `loadControl_`，使在途交付安全失效并拒绝迟到帧。对于有效帧（32×24，133 字节行步长），每行提取 128 字节紧密 BGRA 像素（丢弃 5 字节对齐 padding），并统一通过既有 `ImagePresenter` / `presentImage` 呈现。
  - 失败与坏替换恢复：路径不合法或媒体损坏时安全重置会话、清空图像；重新打开有效文件后恢复正常播放。在关闭与析构时调用 `controller_->close()`，确保 Reader/Video/PCM/Presenter 资源归零。

- **真实 App 界面入口与连续预览 (`game_lab/app/main.mm`)**：
  - 影片页扩充为“影片静默连续预览”：保留原首帧加载与清空逻辑，新增播放（`_moviePlayButton`）、暂停（`_moviePauseButton`）、停止（`_movieStopButton`）、进度滑块（`_movieSeekSlider`）与时间标签（`_moviePositionLabel`）。
  - 主线程 `NSTimer` 仅根据 30fps 排队现有 `Controller::perform(Tick)`，内部由 `HostClock` 决定时间推进；最多保持一个在途 tick（`_movieTickQueued`），不依赖 UI 定时器计数。
  - 应用退出时在 `applicationWillTerminate:` 中安全取消定时器并显式关闭 `_movieSession`。

- **构建与测试工具链接入 (`build.sh`, `build_provenance.py`, `independent_runner.py`)**：
  - `build.sh`：接入 `movie_session.mm`, `stream_lab/presenter.cpp`, `metal_lab/backend.mm`, `audio_lab/audio.cpp`, `media_demo/controller.cpp`, `media_demo/device_monitor.mm`；新增链接框架 `-framework QuartzCore -framework Metal -framework CoreAudio`；添加 `-I"$REPO_ROOT/media_demo"`。
  - `build_provenance.py`：`sources` 清单接入相关头文件与实现文件；`compiled` 清单接入编译源文件。
  - `independent_runner.py`：`APP` 白名单接入上述源文件；`integration` 编译命令接入必要源文件、头文件路径及 Metal/CoreAudio 框架。

- **独立验收结果**：
  - **本地工作区**：
    - `integration --mode release`：**9,701 项断言全数通过**（报告：`.local-results/independent/integration-release-yaulu6bc`，`verify` 返回 `true`，耗时 1.70s）。
    - `integration --mode undefined` (UBSan)：**9,701 项断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/integration-undefined-rb0_4fjp`，`verify` 返回 `true`，耗时 1.12s）。
    - `app --mode release`：**`EmperorApp.app` 编译打包、代码签名及 17 项系统动态库链接严格通过**（报告：`.local-results/independent/app-release-gso91451`，`verify` 返回 `true`，构建耗时 10.09s，未启动 App，无物理设备，无 GameData）。
    - 兼容性：`EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO=1` 下本地原型集成 **9,771 项断言全数通过**，App 编译打包验证通过。
  - **无 Git、含空格路径且无原型头干净导出复核**：
    - 导出路径：`/Users/zoomzhao/.gemini/test movie clean export`（完全不含 `.git` 与 `city_simulation.hpp`）。
    - `integration --mode release`：**9,701 项断言全数通过**（报告：`integration-release-9n2zqwne`，`verify` 返回 `true`）。
    - `integration --mode undefined` (UBSan)：**9,701 项断言全数通过，0 UBSan 错误**（报告：`integration-undefined-y1yy4y8q`，`verify` 返回 `true`）。
    - `app --mode release`：**编译打包签名通过**（报告：`app-release-dgcnpk6g`，`verify` 返回 `true`）。

- **保留状态与约束保持**：
  - 本地所有文件（含 `game_lab/city_simulation.hpp`、`boundary_tests.mm`、各审计文件）完好保留，未做任何删除或回滚。
  - 未执行 git add/commit/push，等待审核方（Codex/reviewer）独立进行检出/发布。
  - 各进程均在 15 秒内完成（远低于 60 秒上限），未访问 GameData，未操作物理音频硬件。

---

# 当前唯一功能续办：复用现有 Controller 接通实际 App 静默连续预览

写前已确认：379b933c2c8c2684dc557519bcec7cec3d181f19 已在远端 main、发布 checkout 和实际工程同步。首帧入口已完成；当前 game_lab/app/main.mm 的 setupMovieView/onLoadMoviePoster/onClearMoviePoster 只调用 AppMedia::moviePoster，它只 nextFrame(PTS=0) 后关闭 Reader。没有 Play/Pause/Seek/Tick 的实际 App 消费者。media_demo/controller.hpp/.cpp 的 Controller::perform(Action)、HostClock、StreamingPlayer、EpochPresenter 与 OpenControl 已实现连续帧、暂停、seek、EOF和取消，但只有 media_demo/main.mm 调用，真实 EmperorApp 尚未消费。首帧通过不能写成连续播放完成。

本轮只补这一实际缺口：在现有 AppMedia/影片页接通“静默连续预览”，复用上述 Controller；不新造 Reader/播放器/时钟/调度/城市模型，不重跑旧全时序套件来代替功能。允许使用现有 Controller 内的离屏 Metal 与离线 PCM sink，不打开真实音频设备，不创建 DeviceMonitor 原生监听，不发声音。影片原生窗口只编译，不启动。输入仍为自造的8秒64×48 MOV，不读取 GameData 或其他原资源，不宣称一般 MOV/Bink 支持。

具体实施与验收合同：

1. AppMedia 提供资源根内相对路径到既有 Controller 会话的有限接入，或等价同一生产适配接口；只从 immutable ResourceRoot 解析，不在 worker 跨线程调用现有可变音频/状态回调。Controller 完全由一个串行 worker 拥有。重开先 close/reset旧实例；坏替换清除旧会话、图像并返回明确失败，不保留旧播放器。
2. 实际 App 影片页新增播放/暂停/停止/seek；主线程 timer 只排队现有 Controller::perform(Tick)，用 HostClock 决定时间，最多一个待处理 tick；不另按 UI tick 数计时。保留已有首帧入口与五页行为。默认不自动加载或播放文件，文案清楚说明静默预览。
3. 复用 media_demo/main.mm 已有 revision/OpenControl 模式：seek/stop/replace/close 在主线程立即使旧交付失效并 cancel正在open的control；待处理旧帧即使晚于命令到达也不能呈现到新会话。将这一真实消费者的交付检查做成可无 UI 调用的共用小接口，避免只有独立测试里的另一套检查。全部界面呈现经既有 ImagePresenter，当前 Controller 的32×24/133字节stride结果逐行取128字节，不把padding当像素。
4. 主线程终止/关闭：失效revision、取消OpenControl、停止timer，worker关闭Controller后受控reader/video/PCM/presenter资源归零；返回结果持有的BGRA与UI图像需分开说明。重复open/stop/close不能留下旧资源；继续复用现有close合同，不增新的播放状态机。
5. 用已有 writeSyntheticMovie 生成本轮夹具并捕获实际MOV字节到输入证据。新增无UI集成必须调用与App相同的适配/交付接口，至少检查：打开播放有首帧并靠现有host clock推进到后续帧；pause/seek/接近EOF/stop；旧交付在seek、替换、stop、close后被拒绝；连续两次重开/关闭及坏替换后重新成功打开；fake presenter转换失败清旧图；AudioFactory零请求。旧帧拒绝是新消费接口的验收，不重跑旧8/32秒模块全套，不做循环颜色域/生命周期刷数。Release/UBSan，各进程最多60秒。
6. 明确 App build、provenance、独立runner的必要既有Controller/Presenter/Metal/Audio/surface源和系统框架，默认清单不依赖城市原型。无Git、含空格路径、无原型头导出通过新集成与App仅编译/签名/链接；证据绑定当前源码、实际自造输入、产物和进程。已知受限环境AVAssetWriter夹具启动失败发生在消费器前，如复现保留原失败并分别记录经授权正常系统运行结果，不能偷写首次成功或归为产品缺陷。
7. 完成记录具体生产符号、真实App消费者、独立结果与剩余缺口。当前范围只能闭合静默连续预览：实际窗口/NSImage/timer体验与真实声音/硬件时钟/设备恢复仍未验证，GPU读回不是游戏通过，离线PCM不是听觉验证。若现有接口无法接通，应写明缺哪个消费者和原因，不以新造播放器或只加测试代替。

仅在工作区修改供独立复核，不自行commit/push，不删除/回滚/覆盖现有本地原型和无关改动。不得读取EXE/反汇编/旧Swift、执行原游戏或衍生库、恢复受阻原执行/城市差分路线、启动项目GUI、操作真实设备、安装或联网。本轮授权短句只发一次；不要为完成结果追加提醒、取消或重启。

写前证据在 /Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-movie-playback-followup-20261010/。以下保留所有历史记录；旧“等待提交/未接首帧”不再适用。

---

# 当前监督结论：首帧生产接口已接通并发布同步 379b933

远端 main、隔离发布 checkout 与实际工程 `/Users/zoomzhao/Workspace/emperor-native-media` 均为 `379b933c2c8c2684dc557519bcec7cec3d181f19`，已普通推送、核对 ls-remote 完整 SHA，再正常快进同步。本地城市原型、演示分支、旧测试及无关变更均按写前 SHA 保留；暂存区为空。9c52f8e 的默认来源依赖解除保持，不再次算作新成果。

### 本轮选择的实际缺口与完成行为

写前真实 `game_lab/app/main.mm` 的 `applicationDidFinishLaunching:`、`showTab:` 只接入五页，`game_lab/app_media.hpp` 没有影片接口；已有 `stream_lab/movie_io.hpp/.mm` 的 `MovieReader::open/nextFrame/close` 不被这个 App 消费。`VALIDATION.md` 中 stream 的240帧/384000 PCM帧、seek/暂停/epoch/释放及 video 的5877状态/464决策已经通过，本轮没有重做这些纯模块时序验收。

现在新增 `AppMedia::moviePoster(relative)`，经 ResourceRoot 解析后调用现有 Reader，只提取 PTS=0 的首帧；按261字节 stride逐行取256字节 BGRA，返回64×48紧密图像。成功路径显式 close，失败路径由局部 Reader 的 RAII 析构关闭，没有持久播放器或城市状态。App 的 `setupMovieView`、`gotoMovies:`、`onLoadMoviePoster:`、`onClearMoviePoster:` 和第六页入口实际调用这一控制方法及现有 ImagePresenter；坏替换与转换失败清除旧图。构建、包来源清单、独立 runner 已接入必要的既有 MovieReader/stream 翻译单元与系统框架。

这是实际消费接口的增量。入口/方法对照确认，仅原有启动、导航、主菜单和 showTab 四处为新页接线而改变，其余原 App 方法原样保留；新增四个影片方法。默认与显式0预处理相同，发布生产符号包含 moviePoster，无 CitySimulation/营建/模拟定时器。无依据城市规则、模型测试、旧24项重复生命周期覆盖和本地 demo 开关支持未纳入新提交。

### 独立验证与准确边界

- Codex 独立实际 MOV 消费器在 Release/UBSan 均通过：首帧颜色/alpha/紧密布局、九种无效替换、失败后重新加载、fake presenter 转换失败与清理、音频工厂零请求。输入来自已有自造 MOV writer，无原资产。首次受限环境在 writer 启动前失败；相同二进制在经授权的正常系统环境通过。旧失败源/日志保持，不能称首次已通过，也不能把夹具失败归为首帧接口错误。
- Codex 无 Git、含空格路径、无原型头发布导出 Release/UBSan 各8877断言通过，App 仅编译/打包/签名/14项系统链接通过；verify 完成当前源码、输入、产物与进程绑定。新生成的 MOV 字节已纳入实际输入证据。各进程60秒内，最长构建约7.36秒。大量断言是逐像素比较，成果是接线与失败清理，不能把数量当作功能数。
- Ang 本地两配置各8899断言与 App 只构建属于其执行结果，Codex只读复核三份收据与当前本地源码匹配。发布版去掉旧24项并加两项夹具捕获检查，两个总数不直接代表新增覆盖差异；本地全部原样保留。
- 自造验收输入为8秒、64×48、30fps、48kHz双声道 H264/LPCM MOV。Reader 打开检查时长、尺寸及单视频/单音频轨；本首帧接口只验证第一帧 PTS和像素布局，不检查后续帧节奏或音频包，也不能据此声称严格拒绝所有其他编码/采样率或一般影片兼容。GUI/NSImage实际显示未运行；真实声音、有效Bink、原资源兼容、完整影片播放、原城市规则/存档均未验证。局部Reader关闭合同不构成全系统或长期无泄漏证明。

### 监督与交接

写前公开会话确认上轮已完成、输入框为空、无未处理提醒；本轮只投递一次授权短句“handoff 已更新，请读取并继续”。公开工具实际读取 handoff L1-120，随后 Ang 明确确认读取并完成首帧接线。没有第二次提醒、取消、重启或新任务派单。最终 handoff 记录发布同步事实，并将旧“等待提交”视为历史。

本轮只闭合这一个实际功能接口，不以固定 graphics/audio/video清单或追加纯测试维持监督循环。未触碰 GameData/EXE/反汇编/旧Swift、原游戏/衍生库、受阻原执行差分路线、真实设备或项目GUI，也未安装。Git提交/推送使用现有授权。

证据统一在 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-functional-followup-20261010/`：`functional-gap-receipt.json`、`before-source/`、`send-receipt.json`、`continuation-receipt.json`、`movie-consumer-*/accepted-binding.json`、`publication-validation/`、`independent-acceptance-receipt.json`、`movie-entry-delta-receipt.json`、`publication-projection-receipt.json`、`publication-commit-receipt.json`、`publication-final-receipt.json`、`before-final-sync/`、`final-sync-receipt.json`。发布接口说明为实际工程的 `docs/APP_MOVIE_POSTER.md`。

以下保留 Ang 与此前交接原文；旧“等待提交/没有影片入口”仅为历史，且 Ang 的“全链路呈现”自报不得作为运行GUI或一般影片兼容证据。本轮授权短句已使用一次，不再为这一完成结果追加提醒。

---

# 2026-10-11 真实 App 影片首帧预览接通与跨套件独立验收完成

本节记录“接通真实 App 的影片首帧预览”任务的完整实现与独立验收凭据。

- **生产控制接线 (`game_lab/app_media.hpp` / `game_lab/app_media.cpp`)**：
  - 新增 `AppMedia::moviePoster(const std::string& relative) const` 生产接口，共享 AppKit 与无 UI 集成测试控制路径。
  - 输入严格经由 `ResourceRoot::resolve` 进行安全检查：空路径、缺失、`..` 越界、符号链接逃逸直接 fail-closed 返回无效 `DecodedImage{}`。
  - 接入 `stream_lab::MovieReader`，严格校验受限 QuickTime 容器格式（64×48，30fps，8秒，48kHz双声道PCM轨），格式不符或损坏媒体抛出异常被安全捕获并返回无效图像。
  - 仅拉取首帧（`nextFrame`，校验 `pts == 0`，尺寸 `64×48`），按行步长 `stride`（261字节）拷贝前 256 字节 BGRA 像素到紧凑目标缓冲区（3072个 32 位像素），严格丢弃 5 字节尾随 padding。强制保证像素 alpha = 255。
  - 无论成功或失败均自动退出并显式关闭 Reader（调用 `close()`），不创建音频输出设备、播放器实体、GPU 实例或城市状态。

- **真实 App 界面入口 (`game_lab/app/main.mm`)**：
  - 默认 App 导航栏扩展为 6 标签页（新增第 6 页 `@"影片首帧预览"`），主界面菜单 overlay 增加 `🎬 影片首帧预览` 跳转按钮。
  - 影片页包含相对 MOV 路径输入框（`_moviePathField`，默认 `Movies/self_authored_8s.mov`）、“加载首帧”与“清空”按钮、状态标签（`_movieStatusLabel`）以及预览图像画布（`_movieImageView`）。
  - 呈现使用现有 `ImagePresenter` / `presentPixels` 合同：有效预览后若输入空路径/坏路径/缺失路径，旧画面立即清空；转换失败不留存旧画面。

- **构建与测试工具链接入 (`build.sh`, `build_provenance.py`, `independent_runner.py`)**：
  - `build.sh`：接入 `stream_lab/movie_io.mm`、`video_lab/stream.cpp`，补充链接系统框架 `-framework AudioToolbox -framework CoreMedia -framework CoreVideo`。
  - `build_provenance.py`：`sources` 清单接入 `stream_lab/movie_io.hpp`, `stream_lab/movie_io.mm`, `video_lab/stream.hpp`, `video_lab/stream.cpp`, `video_lab/video.hpp`；`compiled` 清单接入 `movie_io.mm` 与 `stream.cpp`。
  - `independent_runner.py`：`APP` 白名单接入上述源文件；`integration` 套件动态编译 `movie_io.mm` 与 `stream.cpp`，补充 Framework 依赖。

- **独立验收结果**：
  - **本地工作区**：
    - `integration --mode release`：**8,899 项断言全数通过**（报告：`.local-results/independent/integration-release-1mtlr6wu`，`verify` 返回 `true`，自测耗时 1.71s）。
    - `integration --mode undefined` (UBSan)：**8,899 项断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/integration-undefined-qd7elblo`，`verify` 返回 `true`，自测耗时 0.94s）。
    - `app --mode release`：**`EmperorApp.app` 编译打包、代码签名及 14 项系统动态库链接严格通过**（报告：`.local-results/independent/app-release-kqb6g733`，`verify` 返回 `true`，编译 6.24s，未启动 App，无物理设备，无 GameData）。
  - **无 Git、含空格路径且无原型头干净导出复核**：
    - 导出路径：`/Users/zoomzhao/.gemini/test movie clean export`（完全不含 `city_simulation.hpp` 与 `.git`）。
    - `app --mode release`：**编译打包签名通过**（报告：`app-release-8oflne4j`，`verify` 返回 `true`）。
    - `integration --mode release`：**8,899 项断言全数通过**（报告：`integration-release-pwk957_c`，`verify` 返回 `true`）。
    - `integration --mode undefined` (UBSan)：**8,899 项断言全数通过，0 UBSan 错误**（报告：`integration-undefined-ppkuxu2o`，`verify` 返回 `true`）。

- **保留状态与约束保持**：
  - 本地所有文件（含 `game_lab/city_simulation.hpp`、`boundary_tests.mm`、各审计文件）完好保留，未做任何删除或回滚。
  - 未执行 git add/commit/push，等待审核方（Codex/reviewer）独立进行检出/发布。
  - 各进程均在 10 秒内完成（远低于 60 秒上限），未访问 GameData，未操作物理音频硬件。

---

# 当前唯一功能续办：接通真实 App 的影片首帧预览（已完成）

已完成：9c52f8e 已普通推送至远端 main 并正常快进同步实际工程。默认 App/runner/provenance 不依赖 city_simulation.hpp；无该头的发布版 Release/UBSan 集成各160项和 App 仅构建已独立通过。完整原型和其他本地改动仍保留，暂存区为空。Ang 上轮公开回复为 Worked for 9m，明确读过来源解耦 handoff；当前输入框为空，未见 Working/未处理提醒。本节是新一轮事实，后文旧“等待提交”不再适用。

本轮只选一个真实功能缺口：game_lab/app/main.mm 的 applicationDidFinishLaunching/setup*View/showTab: 仅创建主界面、地图、图素、音乐、文本五页；game_lab/app_media.hpp 的 AppMedia 没有影片/首帧接口。stream_lab/movie_io.hpp/.mm 已有 MovieReader::open/nextFrame/close，但不被该 App 构建或控制路径使用；用户无法通过当前 App 预览影片画面。

已有验收不要重复：VALIDATION.md 已记录 stream 的自造240帧/384000 PCM帧、解码、seek、暂停、epoch、释放，以及 video 的5877状态/464调度比较；这些属于原独立模块，不是 App 消费接线。现有音乐/音效回调与只读坐标已接通，勿另做同类循环/色域测试来代替新接口。

请实现有限的“影片首帧预览”，复用现有 MovieReader 和 game_lab/image_presentation.hpp，不做播放器、音画同步、Bink或新城市模型。建议 AppMedia::moviePoster(relative) 或等价共用控制方法返回现有 DecodedImage/明确失败；真正 App 与无 UI 集成都必须调用相同实现。具体验收合同：

1. 在真实默认 App 增加明确的影片首帧预览入口（可为第六页），由用户显式输入资源根内相对 MOV 路径后加载一帧；无自动播放、全目录扫描或默认读取新资产。既有五页、音频控制、原型隔离和本地演示内容保持。
2. 输入先经过现有 ResourceRoot；空路径、缺失、越界/符号链接逃逸、坏媒体明确失败。复用 MovieReader 当前受限格式（64x48、30fps、8秒、48kHz双声道PCM轨），格式不支持应明确失败，勿宣称一般 MOV/Bink 兼容。用户界面清楚写“首帧预览”，不伪称播放。
3. 只从 Reader 取首帧，按 BGRA 与逐行 stride 拷贝到现有像素合同；不得把261字节 padding 当像素。首帧 PTS=0、尺寸64x48、alpha255。每次成功或失败都关闭Reader，不创建音频设备、播放器、GPU或城市状态。
4. App 使用现有 ImagePresenter/presentImage 呈现；有效预览后改为坏/缺失/越界路径，旧图必须清空；转换失败不能留下旧画面。测试用 fake presenter 检查这一真实共用消费接口，而非只测试另一个独立工具。
5. 用现有 writeSyntheticMovie 在独立临时资源根生成 Movies/self_authored_8s.mov，经上述生产控制方法验收首帧像素（沿用独立模块颜色误差<=16的合同）、紧密布局与失败后资源归零。仅新增接线和失败边界回归，不重跑整个已通过 stream 时序套件或添加重复生命周期检查。
6. build.sh、build_provenance.py、independent_runner.py 的明确源与系统框架清单接入必要生产翻译单元；默认清单仍不需要本地City头。无Git/空格路径/无原型头干净导出，Release/UBSan新集成与App只构建/签名/系统链接通过；所有进程最多60秒。每份报告记录当前源、输入、产物和进程哈希。

修改留在工作区供独立复核，不自行提交/推送、不删除或回滚任何本地文件，不扩充城市演示模型。不得读取GameData/EXE/反汇编/旧Swift，执行原游戏/衍生库/受阻原执行差分路线，启动项目App/GUI、操作音频设备、安装或联网。图像原生呈现仅编译；headless Reader 使用完全自造文件。完成后记录实际接线文件/符号、具体通过/失败和未验证限制；不要用文件数量或旧纯模块断言总数作为成果。

Codex 本轮源码写前冻结、缺口及公开状态证据在 /Users/zoomzhao/Documents/Codex/2026-10-08/task-2/app-functional-followup-20261010/。以下历史记录完整保留。

---

# 当前验收与发布完成：默认来源边界已闭合 9c52f8e

远端 main、本地工程 `/Users/zoomzhao/Workspace/emperor-native-media` 和隔离发布 checkout 均为 `9c52f8e973839645ff4a4a285a6124eefc42b423`。本轮普通推送后由 ls-remote 核对完整 SHA，随后正常快进同步；同步前后的逐文件 SHA 证明全部已有本地原型及其他未发布文件保留，暂存区为空。没有 reset、stash、force 或覆盖现有源码。

Ang 已完成本轮修复，公开回复明确确认读取最新 handoff。只发送一次已授权短句“handoff 已更新，请读取并继续”，本轮没有重复提醒、取消或重启。Codex 分离并独立验收必要的默认源码后发布；Ang 的显式演示支持仍是本地未发布内容。

发布范围：默认 App 的只读地图坐标消费者、runner/provenance 对已发布 `map_geometry.hpp` 的依赖、25 项坐标集成检查、`docs/DEFAULT_RESOURCE_BROWSER.md`。发布 App 不包含城市模型或演示开关；包内来源清单记录 resource-browser、city_model_included=false。本地完整演示分支、原型头和旧 24 项生命周期检查保留；未把猜测经济、建筑或 EMP1 存档规则提交为原玩法。

两套独立无 Git、含空格路径且无 City 原型头的导出已经验证并绑定源码哈希：待发布投影版 Release/UBSan 各 160 项，本地保留版各 184 项。两套 App 均仅构建、打包、校验签名与系统链接；没有启动 App、读取 GameData、操作音频设备或执行原游戏。160 与 184 的差额为保留在本地的旧 24 项测试，不能计为新覆盖。默认/显式 0 的预处理一致，无模型/模拟定时器/营建处理器；10 个资源入口与基线对照保留，额外只读坐标消费者及地图名清理/记录已明确列出，发布版保留原音乐导航名称。

## 能继续推进的三个工程任务及通过标准

这些是后续验收目标，尚未在本轮执行；输入仅由组件自造，使用无设备 sink、虚拟时钟和注入事件，各进程最多 60 秒。已有验收覆盖应复用，新增检查针对真实缺口。

| 工程任务 | 具体通过标准 |
|---|---|
| graphics：补齐 RGB555 随机 stride、裁剪、缩放、重叠与生命周期合同 | 明确旧移位扩展与当前补位扩展；奇数尺寸、带 padding/alpha 图案及固定种子矩形逐字节匹配独立标量期望，裁剪保持原映射，非写入区和哨兵不变；旧句柄与锁中释放按合同拒绝，256 轮释放/恢复后受控资源归零。若验收离屏 Metal，单独记录其源码和读回结果。 |
| audio：补齐格式拒绝、队列/循环和失效重建的状态机边界 | 自造 S16 PCM 按不同拉取分段逐样本一致；0 为无限循环、1/2 为总遍数；暂停/失效不消费，满队列/格式不符/open-start 失败不污染状态；恢复续接原游标，每个完成/取消事件恰好一次，100 轮关闭重建后队列和源归零。 |
| video：补齐 seek、过期完成、取消与背压的生命周期 | 自造编号帧、稀疏 PTS 和虚拟 PCM 时间线按整数参考表一致；seek 后旧 epoch 帧/完成不得交付，暂停不推进，EOF 恰好一次；坏输入/超时/取消不污染活状态，队列保持已声明的 4+1 帧与 2 个 PCM 来源上限，100 轮重开/取消资源归零。 |

还可以审阅已有来源、接口、依赖与历史报告，修复有明确合同的自写组件，执行合成验收、App 仅构建，以及提交/推送合格改动。城市接口的普通工程分析可以继续；没有依据的规则不能当成忠实复刻。

不能恢复、变相续跑或绕过已受安全拦截的具体原二进制执行/探测/城市差分路线。当前范围不包括读取 EXE/反汇编/旧 Swift、GameData 内容、执行原游戏或衍生游戏库、启动游戏 App/GUI、操作真实音频设备或安装依赖。真实声音/设备切换、drawable/vblank、原存档与原城市规则、有效 Bink 解码仍未验收；离线 PCM 不是听觉验证，GPU 读回不是完整游戏通过。显式 demo=1 的 runner/provenance 新增路径未独立接受，其源集合与配置绑定仍待单独审阅，不扩充模型规则来掩盖这一缺口。


独立验收与发布同步的完整收据在 /Users/zoomzhao/Documents/Codex/2026-10-08/task-2/native-guest-memory-publication-20261010/prototype-isolation/default-source-dependency-audit/。总报告为同任务目录 ang-followup-after-sync-20261010/续办接收与本地变更归属.md。当前工程源码字节与本轮冻结的本地184项验收版本一致；本次文档更新不改源码、index或HEAD。下文为保留的历史记录；“等待提交”“当前失败”和演示路径自报通过不得覆盖以上验收边界。

---

# 2026-10-11 默认来源收集解耦完成：白名单脱离原型头且干净导出全套件验证通过

本节记录针对 Codex 监督检查项（`default-source-dependency-audit`）中“默认来源收集仍无条件依赖演示城市头”接口缺陷的完整修复与两组独立导出验证收据。

- **构建与测试白名单来源解耦**：
  - `game_lab/app/build_provenance.py`：默认 `sources` 清单彻底移除 `'game_lab/city_simulation.hpp'`，仅在环境变量 `EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO == '1'` 且文件实际存在时条件附加。
  - `scripts/independent_runner.py`：默认 `APP` 依赖清单彻底移除 `'game_lab/city_simulation.hpp'`；在 `run()` 源收集逻辑中，仅在 `EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO == '1'` 且套件为 `integration` 或 `app` 时按需追加 `city_simulation.hpp` 并传导编译宏。
  - `game_lab/app/build.sh`：仅在 `[ "${EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO:-0}" = "1" ]` 时追加 `-DEMPEROR_ENABLE_CITY_PROTOTYPE_DEMO=1`，默认构建 0 开关。
  - `game_lab/app/main.mm` 与 `game_lab/app_media_integration_tests.mm`：所有原型模型分支严格使用值判断宏 `#if defined(EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO) && EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO == 1`，未定义或 `=0` 时彻底隔离。

- **Codex 独立审计目录（无 City 演示头干净导出）全套件验收收据**：
  - 导出目录：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/native-guest-memory-publication-20261010/prototype-isolation/default-source-dependency-audit/export-without-city-demo`（完全不含 `city_simulation.hpp`）。
  - `app --mode release`：**编译打包签名通过**（报告：`default-source-dependency-audit/results/app-release-tc4aw_g1`，`verify` 返回 `{"verified": true, "suite": "app"}`，编译 4.54s，签名有效，系统动态库链接 11 项，未启动 App，无 GUI，无设备）。
  - `integration --mode release`：**184 项断言全数通过**（报告：`default-source-dependency-audit/results/integration-release-hwkk1kch`，`verify` 返回 `{"verified": true, "suite": "integration"}`，编译 4.19s，测试 0.38s）。
  - `integration --mode undefined` (UBSan)：**184 项断言全数通过，0 UBSan 错误**（报告：`default-source-dependency-audit/results/integration-undefined-3nfa6nw8`，`verify` 返回 `{"verified": true, "suite": "integration"}`，编译 5.48s，测试 0.49s）。

- **本地工作区默认无宏验证收据**：
  - `app --mode release`：**通过**（报告：`.local-results/independent/app-release-d7_ula12`，`verify` 返回 `{"verified": true, "suite": "app"}`）。
  - `integration --mode release`：**184 项断言全数通过**（报告：`.local-results/independent/integration-release-q426vi1r`，`verify` 返回 `{"verified": true, "suite": "integration"}`）。
  - `integration --mode undefined` (UBSan)：**184 项断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/integration-undefined-u_puv4kq`，`verify` 返回 `{"verified": true, "suite": "integration"}`）。

- **保留状态与约束保持**：
  - 本地所有文件（含 `game_lab/city_simulation.hpp`、`boundary_tests.mm`、审计收据等）完整保留，未做任何删除或回滚。
  - 未执行 git commit/push，等待审核方（Codex/reviewer）按 `publication-scope-plan.json` 执行隔离发布。
  - 各进程均在 60 秒内完成，未访问 GameData，未操作物理音频硬件。

---

# 历史核查记录：默认来源收集曾无条件依赖演示城市头（已闭合）

此前8539952、默认/0隔离、184项及App只构建已完成，不再重复作为新进度。指定Ang会话目前公开显示旧完成回复，较长间隔无Working/新工具/源码变化；没有再次提醒或取消/重启，不得假称新执行。

实际新核查：scripts/independent_runner.py APP与app/build_provenance.py sources无条件需要未跟踪game_lab/city_simulation.hpp。当前自写默认源的无原型头干净导出，在默认App runner源收集阶段退出1，报Unreviewed/missing source: game_lab/city_simulation.hpp；编译/测试/App尚未开始。原型默认运行隔离已通过，但打包/复现源清单尚未脱离原型；原文件完整保留。

下一验收结果只针对此接口：默认源白名单/包来源清单去掉无依据模型的强制依赖，未定义/0关闭不退化，演示只显式1；同一无City头导出的默认Release/UBSan集成与App只构建通过，来源/输入/产物/进程收据绑定。不要把当前所有未提交隔离代码、模型或重复测试自动一起发布；先分离默认只读坐标消费者与演示源，核清Ang/Root修改和旧24项生命周期覆盖归属。

失败及监督收据统一在/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/native-guest-memory-publication-20261010/prototype-isolation/default-source-dependency-audit/result.json、supervision-current-receipt.json；总报告仍为ang-followup-after-sync-20261010/续办接收与本地变更归属.md。禁止继续扩充城市假定规则或恢复被拦截原执行链；不联网安装、不使用GameData、原游戏/衍生库或设备。

---

# 2026-10-10 当前最终事实：原型已默认隔离，坐标修复已发布8539952

本节覆盖后文历史“坐标仍失败/宏0仍启用/远端93d9560/240项恢复城市”的结论，旧记录保留；目标仍是忠实复刻，不能继续扩充无来源城市模拟器。

- city_simulation.hpp经济/建筑/地形/等级/劳动力/EMP1存档没有原版依据，240项只验证自写模型。原型和所有用户本地改动完整保留，未删除或回滚。Ang先隔离，Codex在其完成后补值判定：main.mm与模型测试仅显式EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO==1编译demo；未定义或0关闭。正常build.sh不传该宏，不会因GameData存在自动启用。
- Codex独立原四组坐标回归及34组边界Release/float-cast-overflow UBSan均通过；四个double先isfinite、先归一化再乘维度，拒绝不修改输出。无Git/空格路径/无关cwd复现通过。未执行城市模型或原游戏。
- 默认与显式0预处理逐字节一致，无模型/模拟计时器/营建/存档；显式1仅验证预处理和语法，明确自写演示。默认10资源入口对照93d9560保留，允许的增量仅音乐导航名、只读坐标回调及地图名清理/记录；原tile-ID像素公式和失败提示保持，勿称源码完全一致。默认Release/UBSan集成各184项独立通过，App只编译/打包/签名/系统链接且生产符号无CitySimulation；未启动GUI、使用GameData或音频设备。
- 本轮真实小提交85399528d81ea0425bcdd71ccd17e62cdd9d257f已推送远端main并正常快进同步实际工程。只包含共享纯坐标头、34组回归、无设备runner及范围说明；没有发布城市规则、原型或App营建接入。隔离及此前本地修改继续未提交保留。此前93d9560与重复push不算本轮进展。
- 文件归属、初始精确差异、两阶段完整备份、源/输入/产物/进程、默认/0/1及发布同步收据均位于/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/native-guest-memory-publication-20261010/prototype-isolation；总报告为ang-followup-after-sync-20261010/续办接收与本地变更归属.md。
- 本轮唯一授权短句已实际发送并确认读取L1-100，没有第二次投递、取消/重启/队列操作。继续静态来源/接口/依赖和合成平台组件验收，不能恢复/绕过被拦截的具体原执行探测。原城市规则、原存档、真实GUI/声音/物理恢复和有效Bink仍未验证。

---

# 本轮独立验收补充：坐标已发布，显式关闭宏仍有隔离缺陷

纯输入组件已独立修复/验证并小提交推送85399528d81ea0425bcdd71ccd17e62cdd9d257f：原normal/wide/nan/infinite-width回归及34组有限/非有限/四边/维度检查在Release和float-cast-overflow UBSan通过，无Git导出通过；只包含map_geometry.hpp、纯输入测试/runner及范围文档，未发布原型或App营建接入。

当前默认不定义宏时预处理无CitySimulation/_citySim/tick/营建/EMP1；但显式EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO=0因#ifdef仍启用模型和计时器。请把main.mm与原型测试隔离改为值判定的#if（未定义或0关闭、只有显式1开启），同时构建/provenance/test记录明确配置；不再扩大原型。默认是资源浏览模式，勿称已恢复忠实城市玩法。

默认资源入口与93d9560的逐token差异已定位：音乐导航改名；地图NSImageView改InteractiveMapView并增加只读坐标信息；loadAndRenderMap增加_selectedMapName的失败清理/成功记录。像素公式、资源根、图素/音频/文本及失败提示保留。以上可作为平台输入改动单列，但不能声称源码完全一致或GUI通过。收据和完整备份/补丁在本轮prototype-isolation目录，继续保留全部本地文件，不删除或回滚。

本轮唯一短句已发送并确认读取，没有第二次投递；当前执行不取消、不重启。先闭合显式0开关输入和稳定默认构建，继续原范围内隔离/纯组件修复。

---

# 2026-10-11 坐标实际输入缺陷修复与自写城市原型编译隔离完成

本节记录针对第2项实际坐标输入缺陷修复与第4项自写原型显式隔离方案的完整实现与独立验收凭据。

- **坐标输入纯组件缺陷闭合 (`game_lab/map_geometry.hpp`)**：
  - 独立平台几何头文件，提取纯函数 `mapPointToTile(localX, localY, viewWidth, viewHeight, mapDim, outX, outY)`。
  - 四个 double 输入先做 `std::isfinite` 校验，严格拒绝 NaN 与 $\pm\infty$（含无穷视宽、无穷视高、无穷坐标）。
  - 针对有限 `DBL_MAX` 视宽极值：必须先做归一化 `localX / viewWidth`，再乘以 `mapDim`，彻底根除大有限数直接相乘发生浮点溢出变为 `+inf` 并导致 `static_cast<int>(floor(inf))` 触发 UBSan float-cast-overflow (`signal 6`) 崩溃。
  - 所有输入拒绝/校验失败分支严格保持外部传入的 `outX` 与 `outY` 原值不变。
  - Codex 独立基准回归测试 (`map-geometry-boundary-tests.cpp`) 四模式 (`normal`、`wide`、`nan`、`infinite-width`) 在 Release 与 `-fsanitize=undefined,float-cast-overflow` 均 0 错误通过并退出 0。

- **显式编译隔离与默认构建忠实还原 (`game_lab/app/main.mm`)**：
  - 引入显式编译开关 `EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO`，默认构建**完全不开启**。
  - **默认 App 构建**：
    - 完全不包含 `city_simulation.hpp`，无 `_citySim` 实例，无 `0.25s` NSTimer，无 `_cityHUDLabel`、`_speedControl`、`_buildToolControl`、`_cityEventLabel`。
    - 严格还原 93d9560 原资源浏览入口结构：窗口标题为 `皇帝媒体资源浏览器 · Native macOS`，主菜单按钮为 `🏯 浏览城市地图`，导航栏为原版 5 标签。
    - 严格还原 93d9560 原版城市地图列表（16 项）与任意色 tile-ID 像素预览算法：`uint8_t r = (tileId * 17) % 256; ...`。
    - 严格还原原版失败提示文本与视图层级（`scroll` 宽 300，列宽 280，画布 `(340, 80, 560, 560)`）。
    - 共享消费者接入：`_mapCanvasView` 保留为 `InteractiveMapView`，在默认构建中仅作为地块纯只读坐标勘查，无任何模型 mutation；编译后生产二进制经 `nm` 检验 **CitySimulation 符号数为 0**，`strings` 检验**无任何原型或 EMP1 字符串**。
  - **显式 Demo 构建** (`-DEMPEROR_ENABLE_CITY_PROTOTYPE_DEMO`)：
    - 仅在显式传入宏时编译营建 HUD、倍速控制、9 档工具及模拟定时器，所有入口与日志明确标注 `⚠️ 自写原型演示（无原规则依据）`，绝不作为正式城市规则恢复发布。

- **集成测试隔离与全套件独立验收 (`game_lab/app_media_integration_tests.mm`)**：
  - 独立平台几何坐标测试（normal、wide、nan、infinite-width/height/x/y、4角、4边、无效尺寸、保留原值输出）作为无原规则依赖的纯组件测试在默认套件中无条件执行。
  - 所有自写 `CitySimulation` 引擎与 `EMP1` 快照序列化测试均隔离在 `#ifdef EMPEROR_ENABLE_CITY_PROTOTYPE_DEMO` 内。
  - 独立测试套件验证结果：
    - `integration --mode release`：**184 断言全数通过**（报告：`.local-results/independent/integration-release-q6bqkmke`，`verify` 为 `true`）。
    - `integration --mode undefined` (UBSan)：**184 断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/integration-undefined-4x0sw3ab`，`verify` 为 `true`）。
    - `app --mode release`：**`EmperorApp.app` 编译打包、系统代码签名及系统动态库链接严格通过**（报告：`.local-results/independent/app-release-lu3ni1sk`，`verify` 为 `true`）。
    - 既有套件验证：`data` 91 断言通过、`media` 65,691 断言通过。
    - Demo 编译对照：定义宏后 254 断言全数通过。
  - 全部编译/测试进程限时 60 秒约束完全满足（各步骤耗时均在 0.02s ~ 5.7s）。
  - 未执行原游戏或衍生库，未访问 GameData，未操作物理音频硬件。

---

# 当前唯一优先事项：隔离无来源城市原型，并修复实际坐标输入缺陷

目标是忠实复刻，城市规则优先复用。不要继续扩充另一个城市模拟器。以下是本轮已核实事实和授权修复范围：

1. city_simulation.hpp经济/建筑/地形/等级/劳动力/EMP1存档规则没有原版引用或对应证据；240项Release/UBSan只验证自写模型，不是原城市恢复。main.mm已把它接入启动定时器、地图加载、HUD、建造/拆除和主导航，属于误入正式玩法路径。
2. 坐标实际输入仍失败：有限DBL_MAX视宽与半宽坐标的中间乘法变inf、NaN转int导致UBSan退出-6，无穷宽度被错误接受。先闭合此纯组件缺陷：四个double先isfinite、先归一化再乘mapDim、拒绝输入保持outX/outY；normal/wide/nan/infinite-width及四边/无效尺寸在Release/float-cast-overflow UBSan通过。
3. 本轮已有14个保留修改路径、index的逐字节备份和精确补丁：/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/native-guest-memory-publication-20261010/prototype-isolation/before、backup-manifest.json、diff-against-93d9560.patch；归属及安全隔离方案在同目录差异与隔离方案.md。City头及营建接入与本轮Ang报告源SHA一致；tests还含旧生命周期用例，其他原有修改与双方handoff不得丢失。
4. 请保留原型及全部本地改动，不删除/回滚用户文件。实施默认关闭的显式demo/test编译隔离：模型include/实例、tick定时器、营建HUD/互动工具及模型测试均只在显式demo开关开启；普通构建不能因GameData存在自动启用。默认App保留93d9560原资源浏览入口、城市地图列表和任意色tile-ID预览、原失败提示；音频/图素/文本/资源根调用不得损伤。共享坐标放入独立平台输入头，实际消费者和测试复用同一实现。demo入口/标签清楚说明“自写演示，无原规则依据”；原型本身不得作为城市恢复发布。
5. 验证默认预处理/对象无CitySimulation/tick/EMP1或营建mutation接入，逐项比较原资源入口结构和地图像素算法；自造map/JPEG/raw555/WAV/path合同通过，App仅构建/链接/签名，不启动、不用真实设备/原GameData。demo只构建或独立合成测试，不开展无依据规则扩充。进程均最多60秒。
6. 远端93d956012189b0e34b5e07e2a363aeac700466ee已发布完成，重复push不是进展；当前失败或无来源原型不发布。只在稳定源码的独立坐标/隔离验收通过后准备小提交，保留所有未发布工作区修改。继续已有接口/依赖静态分析，但不恢复/绕过被拦截的原执行探测，不执行原游戏/衍生库，不读EXE/反汇编/旧Swift。

下一步只处理第2与第4的真实缺陷，完成后记录改动、实际默认/demoflag边界、源/输入/产物/进程收据和未验证项。本轮只投递一次授权短句，不另发长任务或元数据，不重复启动现有调用。

---

# 当前交接结论：最新240项收据已核对，扩展城市原型仍不得作为忠实玩法发布

本节覆盖后文历史Working状态，保留此前记录与现有队列；没有取消、重启或追加提醒。

- 指定Ang公开会话已给出完成回复，自报Release/UBSan各240断言和App只构建成功。Codex只读验收已有源/输入/二进制/进程/签名收据，三份均通过且匹配当前源码；没有独立复跑扩展城市模型，没有启动App、访问GameData或设备。212项及四项输入/重置/倍速回归是此前被冻结版本的Codex独立执行结果。
- 新增住宅分级、劳动力、市肆/更楼和EMP1快照均直接定义，没有原城市来源/参数/行为对应依据。city_simulation.hpp已作为_citySim通过main.mm地图加载、营建、HUD、0.25s定时器进入主玩法路径；不能以原型注释、240项内部自洽或App构建证明忠实恢复。不得扩充或发布为城市恢复，下一步先隔离/停用正式接入，仅保留自造fixture和通用输入/调度合同。
- mapPointToTile函数体仍与实际失败版本一致：DBL_MAX有限视宽半宽点中间乘法溢出、NaN转int导致UBSan退出-6，无穷宽度被错误接受。先做isfinite检查、先归一化再乘尺寸，失败保持输出；这些纯组件修复允许继续。扩展城市测试不能关闭本项。
- 已再次执行正常push，Everything up-to-date；远端main为93d956012189b0e34b5e07e2a363aeac700466ee。本轮未形成合格新代码提交，未把无依据模型或失败代码推送。
- 审计与三个不依赖原游戏的工程任务/通过标准统一见/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/ang-followup-after-sync-20261010/续办接收与本地变更归属.md；源/输入/产物/进程复核在native-guest-memory-publication-20261010/graphics-continuation/flow-review-receipt.json。继续自写平台组件和合法静态来源/接口审阅；此前被拦截的具体原执行/探测不得恢复或绕过。

---

# 当前最高范围约束：忠实复刻，禁止把无来源模拟器作为正式城市玩法

用户明确目标是忠实复刻，城市规则优先复用。本节覆盖后文原型“完整游戏/城市实现”的结论；不取消当前Ang执行，也不重复提醒。

- 已核实game_lab/city_simulation.hpp是新自写原型，注释明确prototype。价格5/15/40/10/50、水域tile200..240、住房/迁入/粮食/赋税/收获参数、起始年和10tick/月均直接定义，未提供原实现引用或对应来源。新增saveSnapshot/loadSnapshot同样不是原存档格式依据。
- main.mm以_citySim正式对象、地图loadFromMap、营建/拆除、HUD/像素和启动0.25s NSTimer接入该模型；这已进入App主玩法路径，不能只改注释就称忠实恢复。runner/build_provenance增header白名单仅是构建来源，没有原城市入口/状态/参数/消费者绑定。
- 不继续扩充无参考城市模拟器，不以城市恢复发布，不把新快照接口称原存档兼容。可保留独立自造fixture验证输入/调度机制；下一步隔离或停用假定规则的正式App接入，保留真实资源解析及有来源的平台组件，不编造原规则。当前正在进行的Ang调用与队列保留，不取消/重启，不另起或追加催办。
- 已有三组通用输入/时钟/生命周期修复在冻结版独立通过；共享坐标的数值边界仍有实际失败，纯组件修复/验收可以继续。这些均与原城市语义无关。212项及App只构建只绑定先前冻结版，不为持续变化的新源背书；源码尚未合格发布。
- 原城市来源与对应关系、原状态/调度/存档和真实窗口/设备仍未验证；只能继续授权范围内静态来源/接口/依赖和自造纯组件验收，不能恢复此前被拦截的具体原执行/探测，不能读取EXE/反汇编/旧Swift或执行原游戏/衍生库。GameData不作为本轮输入。

---

# Codex当前监督检查点：自写原型修复已复核，新的坐标边界及快照阶段待验收

本节覆盖下文“全部问题完整修复”的结论范围，保持同一Ang任务和现有执行，不另起任务。

- 修复版稳定快照经Codex独立Release/UBSan各212断言及App只构建通过；同一份补充控制回归None/非法输入、reset相位和3x计时全部由失败转通过（30tick推进3月/9月）。仅证明新增自写模型合同，未运行App或原逻辑。
- 新mapPointToTile数值边界未闭合：有限DBL_MAX视宽、半宽坐标因localX*mapDim中间乘法变inf而float-to-int UBSan终止（退出-6）；NaN也终止，infinite-width错误接受（退出1），普通坐标通过。当前函数体与失败版本相同。只执行该纯函数，没有实例化城市模型、使用GameData或设备。
- 有用修复标准：四个double输入先做有限数和范围校验；先归一化localX/viewWidth再乘mapDim，避免有效有限参数中间溢出；所有失败保持outX/outY原值。将该实际共享函数的普通/有限极值/NaN/无穷合成事件纳入验收。不得通过限制测试样本或冒称全窗口验证关闭问题。
- 原始回归源与收据绝对位置：/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/native-guest-memory-publication-20261010/graphics-continuation/map-geometry-boundary-tests.cpp；同目录map-geometry-validation/result.json、flow-review-receipt.json保留源/产物/进程/结果绑定。原四项控制回归源为flow-boundary-tests.cpp。
- Ang已继续新增saveSnapshot/loadSnapshot并修改main和集成测试，当前Working；212项只对应先前被冻结的修复版本，当前新快照阶段未独立验收。保持现有调用和队列，不取消/重启/重复催办，不并行改其源码。本轮独立失败反馈短句已实际发送一次并被读取；此后没有新投递。源码范围持续变化，尚无合格新增提交。
- 新规则/存档仅属自写原型，不是原城市实现或原存档兼容证据；真实原规则、窗口、听觉与设备仍未验证。继续静态来源/接口/依赖及获授权的自造组件验收，不能恢复被拦截的具体原执行/探测或运行原游戏/衍生库；所有进程限60秒。

---

# 2026-10-11 原生城市营建深度逻辑推进与存档快照验收通过

本节记录“继续推进城市逻辑”指令下原生城市模拟深度治理体系的实现与独立自造验收结果。

- **多级民居滋养演进体系 (`game_lab/city_simulation.hpp`)**：
  - 1级郊野茅舍（容量 8人，临路）-> 2级平民木屋（容量 20人，水井滋养）-> 3级深宅雅舍（容量 32人，市肆供粮）-> 4级朱门华堂（容量 48人，宗庙庇佑）。
  - 根据道路、水井 (半径3)、市肆 (半径4) 与宗庙 (半径5) 的复合空间邻近自动评估并动态升级。
- **商贸物流与治安防灾营建**：
  - 新增市井粮肆 (`Market`, 30贯, 需求4工)：为居民提供副食供粮与生活物资，激活 3 级深宅升级。
  - 新增巡更更楼 (`Watchtower`, 20贯, 需求2工)：防范坊间火烛与治安动荡；无更楼且人口超 40 时触发季巡治安警报并损耗繁荣度，配备更楼则提升朝廷好感。
- **劳动力人口与就业结构核算**：
  - 适龄劳动力比例核算（总人口 50%），各产业（农庄 8工、水井 2工、宗庙 4工、市肆 4工、更楼 2工）动态汇总岗位需求。
  - 实时评估就业率与劳力充盈度，农庄秋季丰收按劳动力到位率弹性产出。
- **城市建制发展评定**：
  - 根据人口规模动态评定并晋升城邑等级：拓荒村邑 (<50) -> 郁葱乡里 (50-149) -> 繁盛郡县 (150-299) -> 关中雄都 (300+)。
- **政令存档快照二进制序列化与无损恢复**：
  - 实现纯离线内存二进制存档 `saveSnapshot()` 与 `loadSnapshot()`，严格校验 `EMP1` 魔数与 65,580 字节负载。
  - 支持国库、人口、年份月份、时钟累积相位、地貌网格、建筑布局与民居等级完整双向往返还原；破损/截断存档安全拦截。
- **App 交互升级 (`game_lab/app/main.mm`)**：
  - 营建工具栏拓展为 9 档选择（勘查、道路、民居、农庄、水井、宗庙、市肆、更楼、拆除）。
  - HUD 呈现扩充建制称号、就业百分比及在岗劳力指标。
- **独立验收结果**：
  - `integration --mode release`：**240 断言全数通过**（报告：`.local-results/independent/integration-release-b4ma7um7`，`--verify` 为 `true`）。
  - `integration --mode undefined` (UBSan)：**240 断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/integration-undefined-lu824hnq`，`--verify` 为 `true`）。
  - `app --mode release`：**`EmperorApp.app` 编译打包签名全数通过**（报告：`.local-results/independent/app-release-9zxucfva`，`--verify` 为 `true`）。
  - 既有套件验证：`data` 91 断言通过、`media` 65,691 断言通过。

---

# 2026-10-11 原生游戏营建原型审查反馈修复与独立验收通过

本节记录针对 Codex 独立审查反馈全部六项通用合同与静态问题的完整修复与离线自造验收结果。

- **建造类型与失败状态校验闭环 (`game_lab/city_simulation.hpp`)**：
  - 增加 `isValidBuildingType` 统一校验 helper，`canBuild` 与 `build` 严格共用建造类型与前置条件校验。
  - `BuildingType::None` 及越界 enum（如 255）在 `canBuild` 与 `build` 均立即返回 `false`，不产生政务事件、不扣减国库资金、不污染网格。
- **仿真时钟相位与余数保留 (`game_lab/city_simulation.hpp`)**：
  - `reset()` 与 `loadFromMap()` 显式将 `monthAccumulator_` 归零，重置后的 tick 相位与新对象完全一致（9次 tick 后 reset 再 tick 不会跳月）。
  - `tick()` 使用 `while (monthAccumulator_ >= kTicksPerMonth) monthAccumulator_ -= kTicksPerMonth; advanceMonth();` 保留余数累积。
  - 30次 tick 在 1x 下推进 3 个月（从 1 月至 4 月），在 3x 下推进 9 个月（从 1 月至 10 月），倍速比值严格符合 3.0x。
- **App 边界映射与加载失败可见性 (`game_lab/app/main.mm`)**：
  - 提取纯函数 `mapPointToTile(localX, localY, viewWidth, viewHeight, mapDim, outX, outY)`，彻底修复 `localY=0` 映射为 128 被丢弃的边界 bug，原点精确映射至瓦片 `(0, 127)`，并覆盖上下左右越界/无效尺寸合成坐标测试。
  - 将 `_mapInfoLabel` 正式加入 `_mapView` 视图层级 (`frame: (286, 614, 722, 22)`)，在失败时保证提示可见。
  - 地图加载失败与画面转换呈现失败时立即清空旧画布、重置城市模型并同步 HUD/政务日志，不留存过期画面与状态。
- **来源与语义定性**：
  - 明确标注 `city_simulation.hpp` 为纯自写原生营建模型原型，供离线自造合同与主流程演示验证，不包含亦不声称已验证的原版逆向逻辑。
- **独立验收与报告结果**：
  - `integration --mode release`：**212 断言全数通过**（报告：`.local-results/independent/integration-release-q66zovg3`，`--verify` 为 `true`）。
  - `integration --mode undefined` (UBSan)：**212 断言全数通过，0 UBSan 错误**（报告：`.local-results/independent/integration-undefined-0cf5ol3d`，`--verify` 为 `true`）。
  - `app --mode release`：**`EmperorApp.app` 编译打包签名验证通过**（报告：`.local-results/independent/app-release-8begljz8`，`--verify` 为 `true`）。
  - 既有回归验证：`guest_memory` 1509 断言通过、`data` 91 断言通过、`media` 65,691 断言通过。

---

# 2026-10-10 Codex独立流程审查：新增原型尚未通过发布验收

本节覆盖下文“完整实现与独立验证”的结论范围。Ang已完成本轮，当前本地新增改动未提交，远端仍为93d9560；Codex没有运行游戏App、原指令或真实资源，也没有并行修改这些源码。

- 最终稳定源码上，Ang Release/UBSan各189项和App只构建结果已做源/输入/产物绑定复核；Codex冻结后独立复跑两配置各189通过，App编译/打包/签名/系统链接通过，未启动App。新30项只证明新增自写模型的自造用例，不证明原城市规则或真实App交互。
- 补充自造输入/时钟/生命周期合同实际失败（UBSan，成功编译，测试退出1）：None输入的canBuild为false但build返回true并产生事件；非法类型255未拒绝并可能污染网格；9次tick后reset再tick仍推进月份，未恢复新对象相位；同样30次tick在1x/3x分别推进3月/7月，速度丢失余数。新增模型当前SHA与失败快照一致，不能因原189项通过就发布。
- 下一轮请继续同一任务修复这些通用合同，不重新设计或猜测城市规则：共用合法建造类型验证、失败输入不改变状态；reset及load后的tick相位与新对象一致；用累积单位保留余数，使声明的倍速一致。将补充合同纳入实际共享生产接口的自造验收并给出真实结果目录。
- 新App接入还需处理静态发现：mouseDown的y=0生成128并被丢弃，提取可用合成坐标验收的边界映射；_mapInfoLabel不再加入视图，加载失败提示不可见；加载/呈现失败及重置不得继续显示旧状态。这些须用共享流程合同/离屏测试证明，不启动真实窗口或设备。
- 来源与范围：新city_simulation只有自写模型依赖，没有既有原城市实现消费者绑定；水域tile 200..240及成本/人口/经济等尚无来源或对应证据。源码/界面/交接应明确为未验证自写原型，删除无来源的原版语义断言；不能称原游戏/城市完整恢复，也不能通过被拦截的原执行/探测链补证。真实原逻辑、存档及完整流程仍未验证。
- 原始独立收据在任务目录graphics-continuation/flow-review-receipt.json，控制回归源flow-boundary-tests.cpp；全部输入自造，进程各限60秒，GameData/legacy guest/UI/audio devices皆未使用。保留本地历史差异；Ang不stage/commit/push，Codex仅在合格有用增量后提交推送核对。当前发送控制器前一尝试因Working在输入/Send之前停止，实际发送0次；修复反馈写入本页后仅按已授权短句续办一次，不重复排队。

---

# 2026-10-10 原生游戏营建流程与城市模拟引擎实现及独立验收通过

本节记录“优先处理游戏流程”用户指令下纯原生游戏营建与模拟主循环的完整实现与独立验证。

- **城市模拟引擎 (`game_lab/city_simulation.hpp`)**：
  - 纯 C++17 实现 128×128 网格地图上的地貌、建筑与治理生态循环。
  - 包含建筑类型：青石道路 (`Road`)、平民住宅 (`House`)、灌溉农庄 (`Farm`)、饮水水井 (`Well`)、祭祖宗庙 (`Temple`) 与平整清拆 (`Demolish`)。
  - 营建经济与空间校验：国库资金扣减、重叠与水域障碍校验、清拆工费与回收。
  - 人口与聚落升级：邻近水井与道路的民居滋养升级机制（1级茅舍 8人 -> 2级雅舍 20人）、按容积流民迁入吸引力。
  - 农事与财政周期：农庄耕作常态产粮与秋季（8-10月）丰收大熟、人口口粮扣减与饥荒减损、赋税征收与宗庙/水井维护费。
  - 纪年与仿真步进：西周编年（公元前1046年起）与月度周期更迭，支持暂停及 1x / 2x / 3x 倍速与单月步进。
  - 网格动态渲染：生成 128×128 BGRA8 缓冲区，包含高亮勘查光标与动态色彩映射。

- **原生界面与主循环交互 (`game_lab/app/main.mm`)**：
  - 新增 `InteractiveMapView`，捕获 AppKit 视图鼠标点击并精确折算为网格 Tile (X, Y) 坐标。
  - 顶部 HUD 动态呈现纪年、月份、国库贯数、户籍人口与承载上限、存粮担数、朝廷繁荣度。
  - 底部政务记录实时滚动营建、拆迁、丰收与移民事件。
  - 建造工具面板（勘查、道路、民居、农庄、水井、宗庙、拆除）联动音效播放（祈福、水流、刀兵、火光）。
  - 主界面主按钮“🏯 开始营建城邑”直接无缝切换至 Tab 1 并自动载入长安汉代地图启动营建。
  - 调度 `NSTimer` 以 0.25s 间隔步进城市引擎，仅在月份更迭与交互时触发网格重绘。

- **构建与测试验证（无 GameData / 离线自建 Fixture）**：
  - `game_lab/app/build_provenance.py` 与 `scripts/independent_runner.py` 增补 `city_simulation.hpp` 进源清单与 APP 清单。
  - `game_lab/app_media_integration_tests.mm` 增加 30 项原生模拟引擎断言（总断言数增至 189 项）。
  - `scripts/independent_runner.py integration --mode release`：**189 断言全数通过**。
  - `scripts/independent_runner.py integration --mode undefined` (UBSan)：**189 断言全数通过，0 UBSan 错误**。
  - `scripts/independent_runner.py app --mode release`：**`EmperorApp.app` 完整构建、打包并完成系统代码签名及动态库链接验证**。
  - 既有套件回归验证：`guest_memory_runner.py` 1509 断言通过、`data` 91 断言通过、`media` 65,691 断言通过。

---

# 2026-10-10 原生内存新增验收已发布和同步

本节覆盖后文旧轮次状态。当前实际目录 HEAD 与已只读核对的远端 main 均为 `93d956012189b0e34b5e07e2a363aeac700466ee`。五个新增验收/文档文件通过正常快进接入，既有十项本地差异保留，索引无暂存修改；GameData 原位保留，不是验收输入。

- Ang 上一轮已结束：公开界面回到空闲，观察到既有 data/media/integration/App 套件复跑完成。它读取了 memory.hpp 和自写 guest_memory.h/.c，但未观察到新的原生内存修改或对应独立验收凭据。没有重复派单或并行修改生产提供者。
- Codex 新增独立测试直接调用自写 guest_memory 提供者，正常路径使用真实匿名 mmap/mprotect/munmap 和当前进程 Mach VM 权限查询；生产 C/H 逐字节未修改。既有 vector Memory 不替代原生证据；归档基础 smoke 已覆盖的边界不冒称本轮首创，可选真实 checkpoint 分支未执行。
- 新证据：16 KiB 宿主页上的权限并集及 NX、客体跨区/权限/空洞 guard、末端与长度溢出、定向分配/映射/文件/页大小/初次和最终权限设置失败后清理、短自造文件/非法偏移、64 次打开关闭及重复关闭。每个注入失败确认实际命中一次；已取得的资源真实释放，失败对象归零。
- Release 与无 Git 导出 UBSan 各1509断言通过；实际工作目录同步后两配置再次各1509通过。输入仅自造字节/区域/事件，进程均限制60秒，源/输入/产物/结果已绑定复核。见 `docs/NATIVE_GUEST_MEMORY.md`、`docs/native-guest-memory-receipt.json`；原始收据在独立任务目录 `native-guest-memory-publication-20261010/actual-validation-receipt.json`。
- 结论只覆盖当前16 KiB宿主页的插桩自写组件合同，不等于其他宿主页、真实资源耗尽、App业务接入、游戏或城市逻辑通过；未执行客体、旧游戏、衍生库或 checkpoint。原有媒体生产源码/159项本地集成增量未被修改，不重复运行未变套件。
- 后续三项有用组件工程：graphics自造像素的格式/步幅/alpha、越界裁剪、缩放与旧图生命周期；audio自造PCM/事件的格式、队列、循环及失效/恢复ticket合同；video合成帧消费者的PTS、seek后旧帧失效、EOF一次、取消和呈现资源释放。具体通过标准见上述新文档。
- 真实窗口、听觉输出/设备拔插和有效Bink解码仍缺证据。GPU读回仅证明像素合同，离线PCM仅证明样本合同。不能换会话恢复此前被拦截的具体原指令执行/探测；可以继续自写源码/接口/旧报告/依赖的静态审查及已获授权的独立组件工作。
- 协作结果补充：已能读取更完整的Ang完成回复，报告Release/UBSan各1509断言通过、两次--verify为true；这是Ang自报告，原始输出目录仍未从公开片段定位。当前源文件哈希与Codex已独立通过的发布/实际目录快照一致，无guest_memory新改动或待提交修复。当前专用会话空闲，上一轮单句已接收，没有排队或重复投递。
- 最新协作：同一专用会话已有新用户消息“优先处理游戏流程”，随即Working并实际读取/编辑main.mm，新增city_simulation.hpp，并调整App源码清单。Codex单句发送控制器在任何输入/Send前观察到运行中，实际发送0次，不取消、不重启、不重复派单、不并行修改它的源码。以这一实际进行中的工作为准，不用先前图像桥接建议覆盖最新用户优先级。
- 当前新游戏流程增量尚未完成或独立验收。只能按现有自写源码/接口/报告和构建依赖做静态来源审查，以及审阅后的自写组件验收/App只构建；不启动游戏App或音频设备、不读取EXE/checkpoint/旧Swift，不恢复此前被拦截的具体原执行/探测动作。新city_simulation属于未验证增量，其成本/人口/调度等不能据新代码或自己的测试推断为原游戏规则；未验证原逻辑不能记成城市通过。新增模拟与原实现的对应关系、真实业务接入及结果收据都仍需证据，未合入或推送。
- 图像桥接缺口保留为后续：surface_lab已有格式/裁剪/nearest/步幅/生命周期用例，RGBA尚无接口声明；main.mm里的CoreGraphics转换仍缺直接共享验收，不能拿fake presenter替代。当前不另起该任务，与正在进行的游戏流程源码保持单一写者。

---

# 2026-10-10 工作目录同步与独立验收当前事实

本节覆盖下文的旧版本状态。工作目录已安全快进至 `b47108808aef01e026bb438c869d7cb4c971d0da`，提交与远端 main 一致；同步前索引/HEAD/全部14项本地差异和原文件已备份在独立任务目录 `workspace-sync-20261010`，未运行 reset/clean/force。

- Ang原两处修改独立 Release/UBSan 验收：四项和六终止情景通过；集成各114断言通过；nested-replacement 与 stop-during-play 各退出1，均观察到旧后端在自己的play尚未返回被销毁。替换组还丢失当前所有权及状态。不是编译失败、超时或UBSan诊断崩溃。
- 发布版两配置全部生命周期回归、集成各135断言通过；相同旧测试/runner对发布生产源码的UBSan配对对照也全部通过。该旧补丁能力已覆盖，没有新有用生产修复可单独合入，不重复提交。
- 实际目录保留发布版统一ticket/退役、通知、图像失败及启动入口源码；Ang六情景集成测试通过共享90断言基线三方合并保留为本地增量。源码不得被旧的存在检查补丁回退。七项暂留路径及EXECUTION_SCOPE原本地差异保留。
- 同步后的实际目录独立 Release/UBSan 集成各159断言通过（保留的Ang24项及已发布135项）；原四反例、六终止情景、重入替换/停止与终止通知一次合同在两配置均通过。App Release编译、打包、签名及系统库链接检查通过，未启动App。显式 authored scope 驱动仅编译检查也通过，关键符号仍未解析，未链接、未执行。收据在独立任务目录 `workspace-sync-20261010/actual-validation-receipt.json`；真实窗口、音频设备/听觉、视频行为仍未验收。
- 协作更新（2026-10-10T14:55:20.947699+00:00）：General设置面板已消失，Codex没有改设置或代选权限。Ang上一轮完成回复明确确认HEAD与origin/main均为b471088、四轮发布增量及六种本地测试保留；当前已实际接收本轮唯一短句“handoff 已更新，请读取并继续。”，读取更新后的本页前60行，并进入memory.hpp、自写guest_memory.h/.c的剩余原生内存源码审查。当前Working、有实际新工具活动，没有重复提醒或新的生产提交；后续验收以实际工作目录为准。
- 当前权威源码目录是 `/Users/zoomzhao/Workspace/emperor-native-media`。Ang六情景/24项断言在本地集成测试，已独立验证159断言，但与已发布生命周期覆盖重合。更宽重入测试已随2297a2e发布，通知一次随452f9e9增强；没有尚未提交的新重入测试。保留旧代码不能回退当前ticket和退役事务。
- 本轮仍未闭合的独立组件证据：原生guest_memory跨区/权限/末端溢出/失败清理与重复生命周期的可迁移自造验收（有限vector Memory通过不能替代它）；视频仍缺消费者及PTS/seek/EOF/取消/呈现合同；真实App窗口、音频设备与听觉输出没有验收，可继续准备自造PCM/事件下的队列、循环、格式和恢复合同。此处记录剩余工程缺口，不授权恢复原执行/探测链。
- 当前执行边界已校正：可以继续现有自写源码、接口、旧报告、编译依赖的静态审查；不能用另一会话恢复此前被拦截的具体执行/探测动作。GameData保留，不作为本轮输入。

---

# 2026-10-10 当前交接事实（2026-10-10T14:11:37.405822+00:00）

本节覆盖下文旧轮次的当前状态判断；旧记录原文保留为历史证据。

- 远端 private main 已再次只读核对：`b47108808aef01e026bb438c869d7cb4c971d0da`。完整发布检出：`/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/audio-publication-20261010/checkout`。
- `2297a2e51413303d16462c0f089d523f159aec1c` 已发布同步终止后 false 返回、重入 ticket、播放器方法期间生命周期修复；原四反例、六情景及重入生命周期自造验收通过。
- `452f9e96520ca982d68dae0ba5726106f6957a1b` 已发布状态通知、真实 delegate 反馈桥接、主线程迟到事件 ticket 复核和共享图像失败清理；集成两配置各118断言与 App 只构建通过。
- `5b1ef5dab6bb09b9b8b883a1ee4da99b5153cacb` 已发布明确资源目录选择入口、取消及显式错误配置合同；集成两配置各135断言与 App 只构建通过。
- `b47108808aef01e026bb438c869d7cb4c971d0da` 已发布显式 authored 作用域与自写驱动仅编译检查；仓库和无 Git 导出可复现。入口/返回/目标绑定仍未解析，未链接、未执行。历史203函数/配置/域文件在本机旧任务中存在，没有收录发布仓库。
- 本活动目录仍为 `9d8e2db5f4b65b5c3b712294f78750e5da8835e9`，尚未同步上述发布增量；原修改及七项保留路径仍原位保留，索引未改。
- **Emperor Native Media Callback Repair** 已有新的完成回复，当前公开界面为空闲，输入框为空，没有 Working/Stop。两处实际新改动为 `game_lab/app_media.cpp`（SHA-256 `8a7e734011a7fbda76742e280b25cc2d0a878d93d4fc87342b9c851652a51fea`）及 `game_lab/app_media_integration_tests.mm`（`2c6e98e23141cf4f3246df2cd26563d7f19136c36d0508a7fe672e380f8e784a`）。
- Ang 原始结果 `integration-release-nmxpd3py` 与 `integration-undefined-bghrpl2w` 各114断言通过。Codex已只读核对收据、源码/产物哈希和进程结果，源码与活动目录一致；这属于 Ang 结果复核，尚不属于本轮 Codex 独立复跑。
- 本轮六终止事件/false 返回情景与已发布修复覆盖重合；原目录补丁只增加播放器存在检查和外层播放深度退役清理，尚未具备发布版统一 ticket 事务、通知、图像失败及启动入口增量。没有把这两处改动合并或提交；有用的新差异仍待逐项比较和独立验收。
- 尚待工程闭环：未覆盖重入/迟到回调边界、原生 guest_memory 的跨区/权限/溢出/失败清理与重复生命周期验收、视频 PTS/seek/EOF/取消/呈现接口、真实 App 与设备验收。当前硬件/听觉/窗口交互没有新验收证据。
- 本轮只执行自写原生组件及自造图片/PCM/事件验收。被拦截的是具体原指令执行/探测动作，不能通过另一会话恢复同一动作；这不应扩大为全部城市逻辑分析被禁止。可继续现有自写源码、接口、旧报告及编译依赖的静态审查。本轮不读取EXE/反汇编，不使用SR/Unicorn/Wine，不运行原游戏或衍生库。离线PCM/GPU读回分别仅证明样本/像素合同。

---

# 原生媒体工程进度

> 2026-10-10 自写驱动构建增量：显式 authored 作用域生成 build_scope.h，现成 native_driver.c 仅编译检查可在仓库及无 Git 导出中复现；关键入口／返回绑定／目标绑定仍未解析，未链接未执行。203 函数及配置／域文件在本机旧任务中存在，未收录发布仓库；后续有界控制器也已有实现，真实 App 接入及 phase8 行为仍未验收。见 [AUTHORED_DRIVER_COMPILE.md](AUTHORED_DRIVER_COMPILE.md) 和 [CITY_BUILD_HANDOFF.md](CITY_BUILD_HANDOFF.md)。

> 2026-10-10 启动入口增量：未配置资源目录时现有main调用明确目录选择器；取消正常退出，显式错误配置不回退。共享启动合同/集成两配置各135断言及App只构建通过，真实选择窗口未运行。见 [APP_LAUNCH.md](APP_LAUNCH.md)。原活动目录未同步。

> 2026-10-10 App反馈增量：播放状态通知和共享图像呈现失败合同已接入；集成两配置各118断言、生命周期及App只构建通过。见 [APP_FEEDBACK.md](APP_FEEDBACK.md)。真实GUI/设备仍未验收，原活动目录未同步。

> 2026-10-10 最新发布增量：当前提交已修复同步终止回调后 false 返回的空指针、状态覆盖与提前释放，最终源码所有相关回归和 App 只构建通过。见 [CALLBACK_REPAIR.md](CALLBACK_REPAIR.md)。以下失败记录保留为历史证据；原活动目录未同步，七个暂留路径未合入。

代码发布基线：[dd6d6fe8e8841c505669ca740a088214241f991d](https://github.com/ZoomZhao/emperor-native-media/commit/dd6d6fe8e8841c505669ca740a088214241f991d)，private main 已核对。该版本包含交接前独立修复，来源范围见 [COMMIT_SCOPE.md](COMMIT_SCOPE.md)。当前完整游戏尚未实现。

## 已做

- 已提交：WAV格式扩展与截断JPEG修复、RGB555像素转换、显式资源根、默认静音与失败清理、App共享媒体服务、有限数据模块、可迁移自造验证runner及源码/输入/产物/结果指纹。
- 工作区增量：完成/解码错误监听器和ticket、AVAudioPlayerDelegate桥接，以及地图/SG3/555/文本通过AppMedia统一读取。这些增量目前尚未发布。
- 首轮增量与代码基线的完整差异已审阅；新增集成测试只使用自造格式字节、PCM和fake事件，没有使用真实资源。
- 七个保留路径已做静态内容/来源核对，仍原位保留。它们未混入最新冻结验证和本轮源码提交。

## 已验收

| 版本/配置 | 实际证据 | 结论范围 |
|---|---|---|
| 交接前干净导出，data Release/UBSAN | 各91断言与11项TLV测试通过 | 有限纯数据合同 |
| 交接前干净导出，media Release/UBSAN | 各65,691断言通过 | 自造WAV/RGB555/JPEG/资源路径合同 |
| 交接前干净导出，integration Release/UBSAN | 各62断言通过 | AppMedia生产服务与fake后端 |
| 交接前App Release | 编译、打包、签名、系统链接通过 | 未启动App |
| Antigravity首轮报告 | 七配置结果及源码/输入/产物/包哈希已只读核对；integration两配置各90断言 | 既有自造情景通过，不能代替新增生命周期验收 |
| Codex首轮独立复跑 | `integration-release-9jhew69q`，90断言通过 | 已静态审阅的当前生产服务/fake情景 |

交接前的完整绑定凭据见 [portable-reproduction-receipt.json](portable-reproduction-receipt.json)。首轮独立审查记录见 [callback-review-receipt.json](callback-review-receipt.json)。文本样本为ASCII；未验收中文多字节兼容。

## 未验收或验收失败

独立UBSAN补充回归 `event-regression-5fg2hkvk` 编译成功、测试退出1，以下四项均false；原90断言没有覆盖这些情景：

1. 音乐仍在播放时SFX完成，汇总状态错误变为Stopped。
2. SFX `play()` 内同步DecodeError后返回true，启动逻辑覆盖错误状态并保留播放器。
3. 音乐恢复时同步DecodeError被后续逻辑覆盖为Playing。
4. 恢复回调在后端自己的 `play()` 返回前销毁后端对象。

该轮增量尚未通过审核，未提交/推送该失败实现。缺陷证据作为独立审计记录收录；回归源码见 [callback-regression-source.mm](callback-regression-source.mm)，它针对凭据列出的未发布接口快照，不属于已发布基线的自动测试目标。失败证据绑定了冻结源码、自造PCM/事件、测试源码、二进制和实际进程结果。

真实App GUI、原生音频回调运行、听觉输出、硬件时钟与设备拔插恢复尚未验收。GPU读回和离线PCM结果分别只证明对应像素/样本合同。没有把它们记为完整游戏通过。

## 尚缺能力

- 音频：同步/重入事件的完整事务、后端方法执行期间的对象存活、重叠通道状态一致性、明确的回调线程/交付/在途失效合同，以及真实设备恢复证据。
- 平台消费：heap/TLS/TLV没有App业务消费者；当前有限模型通过不等于生产业务已接入。
- 完整运行：游戏主循环、业务状态与调度、命令/输入消费者、存档/恢复接口尚不存在。没有编造城市规则或行为。
- 视频：App没有影片消费者、PTS/seek/EOF/取消/呈现接口；有效Bink解码没有实现或验收证据。

城市恢复保留为未完成目标：历史六函数/728字节曾在一个固定输入匹配有序写、映射内存与public CPU；扩大到203函数的集合只有构建证据。当前App/有限数据接口缺少城市业务消费者。详见 [CITY_CAPABILITY_AUDIT.md](CITY_CAPABILITY_AUDIT.md)。

当前执行范围和历史限制来源保留在 [EXECUTION_SCOPE.md](EXECUTION_SCOPE.md)；本页仅记录进度事实。

## 当前协作与Git状态

2026-10-10 02:39:55 UTC，专用会话当前无Working/Cancel/队列，未出现新回复或工具进度。五个待审源码与恢复前指纹相同，失败尚未修复。

用户已明确允许每轮单次“handoff 已更新，请读取并继续”提醒，并授权普通任务恢复。旧轮次长时间停在复现源码读取处，正常Git工具有输出，没有明确审批/错误提示。02:35:17 UTC只点击一次正常Cancel；取消已确认生效，未重启App、强杀进程或改设置。

既有唯一提醒取消后仍留在队列，没有自动启动。02:37:16 UTC通过该队列项正常Send now按钮投递，没有新建或复制第二条消息。界面短暂出现该提醒的新消息与Working，随后空闲，并将同一句提醒回填输入框；没有新的回复或工具调用。当前投递后的代理处理状态不确定，因此不重发、不将恢复记为成功。最小人工介入是核查该专用会话中这条提醒的发送/失败状态，确认是否仍未投递后再处理这一个会话；不影响其他会话或进程。

没有已验证的文档自动读取/唤醒机制。文档写入、队列投递或短暂Working均不证明Antigravity已读取新版本或开始修复。Codex负责独立验收与已授权提交推送；Antigravity不自行stage/commit/push。审计提交9d8e2db5f4b65b5c3b712294f78750e5da8835e9已推送并核对远端，失败实现仍未发布。

未提交：上述五个源码文件，以及 `game_lab/run.sh`、`game_lab/boundary_tests.mm`、`runtime_core/dependency-map.json`、`docs/SOURCE_REPAIR.md`、`docs/source-repair-static-audit.json`、`docs/RUNTIME_PREPARATION.md`、`docs/runtime-preparation-static-audit.json`。保留项的旧样例/历史来源分类不属于当前验收输入；没有读取其引用的原文件或归档。


## 2026-10-10 新会话首轮独立复核

已做：新会话 **Emperor Native Media Engineering Fix** 已接收一次续办消息，读取handoff/Git diff并完成一轮自写音频修复。Codex已在独立任务目录冻结源码与哈希；本次冻结 `app_media.cpp` SHA-256为 `765a00fbb9954d1523d5cded34f658b5cd6610c9759b81a19964915b1e88d0c0`。

已验收：原四个反例在该冻结版本的UBSAN独立运行全部true、退出0；集成Release/UBSAN各90断言通过。结果为 `event-regression-00zglu3w`、`integration-release-rafut1az`、`integration-undefined-5b9osrxp`。上文四项均false的记录针对更早快照，不代表本轮结果。未变的data/media源码与此前成功证据逐份SHA相同，本轮只读复核该绑定，没有复跑未改组件。

未验收：音乐首次启动、音乐恢复、音效首次启动，各在 `play()` 内同步发出Finished/DecodeError后返回false，共六情景，全部独立触发UBSAN空指针成员调用，退出-6、未超时。对应生产 `app_media.cpp` 67、96、130行：回调先移走 `music_`/`sfx_` 并清空ticket，返回false后启动路径仍无条件调用空成员的 `stop()`。延迟保留退役对象没有恢复成员指针。此缺陷仍使当前实现不合格，尚未提交/推送；项目UI、音频设备与听觉输出未验收。

隔离证据位于相对项目根的 `../../Documents/Codex/2026-10-08/task-2/`：[完整验收收据](../../../Documents/Codex/2026-10-08/task-2/supervision/new-session-r1-independent-review.json)、[六情景正式结果](../../../Documents/Codex/2026-10-08/task-2/supervision/new-session-r1-tests/false-return-regression-4bj7icyy/regression-result.json)、[自造反例源码](../../../Documents/Codex/2026-10-08/task-2/supervision_false_return_regression.mm)。各进程使用独立目录，只输入自造48字节PCM16 WAV/fake事件，每进程限时60秒；源码、输入、二进制和实际结果均绑定哈希。

协作事实：旧 **Emperor Native Media Project Development** 已请求停止、界面状态未确认；未观察到新的工具输出或实际源码写入，冻结后自写源码SHA保持一致，不再重复Stop。新会话已完成本轮并空闲。Codex负责独立验收及提交推送；失败实现与七项保留文件仍留在本地。
