# 官仓通路：可选自写仓储训练关

主线现已接入第四菜单和ECG1版本2完整v4挑战存档，详见[WAREHOUSE_TRAINING_APP.md](WAREHOUSE_TRAINING_APP.md)。下文保留作者在a43919c冻结基线下的模块交付与测试范围；其中“此补丁未实现”的容器缺口属于该作者补丁的历史范围，主线文件容器及真实窗口验收另记。

验证基线为已发布 `a43919c6983058d9c9a73ce8f2926e60bd3e4325`，实际核心 SHA-256 `d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0`。只新增训练头、测试、监督器和本页四个文件。原三关目录、核心、main.mm、目标反馈、ECS1/ECA1、城市/整档 I/O 均未修改；没有 UI、ang、音频设备、原代码/原资产、安装、提交或推送操作。规则和目标是可替换的自写配置，不宣称原版规则。

## v2 冲突快照修复

第二方发现v1的P2：只改真实factory快照的network计数后，全部农庄连通却预测断路损耗、全部断路却预测入仓仍返回valid=true，并给矛盾行动提示。这违反下文“冲突输入单Unknown”的合同，不是实际城市生产或观察入口虚报胜利。已在原v1冻结副本Release/UBSan复现，保留原审核输入和失败报告在 `evidence/warehouse-training-review-input` 与 `evidence/warehouse-training-v1-local-failure-*`；原1954检查通过不能覆盖这项失败。

v2只增加当前路网与**nextMonth**的必要一致性：disconnectedFarms=0时noRouteDiscard必须0；connectedFarms=0时delivered和fullDiscard必须0。冲突返回valid=false、单条Unknown。混合路网不要求断路农庄一定有损耗、连通农庄一定入仓：全城产量按整数分配，某一类可能拿到0份额。lastMonth可能来自修路前的布局，不套用当前网络关系；修路后的历史损耗提示保留。生产、分配、寻路算法均未复制或修改。

公开API、训练id/目标/期限、原三关和ECS1/ECA1格式不变。已经应用v1时用v1→v2升级补丁；尚未应用时用完整v2四文件补丁。只改原新增文件中的头、测试和本页，监督器不变；主线文件容器仍由主线独占接线。

## 玩家目标和最小操作提示

新关 `id=4, revision=1, 官仓通路`：**24 个模拟月内同时达到人口32、存粮650担、国库4000贯、当前月度收支≥0**。期限、胜负和一次徽记复用 `CityScenarioSession`；不发粮、钱或新资源奖励。新城使用明确的 `CityRuleset::RoadWarehouses` 与默认空白陆地，核心公平初始值为人口0、国库5000、官仓(5,2)存粮400、总容量600；训练模块不重置城市，也不赠送资源。主线为新的选择/重试明确创建候选新城与唯一 runId。

650担超过单仓600担容量，所以当前真实规则下必须扩充仓储，且新农产必须能经道路入仓。人口目标提供农庄/仓库的共享劳动力；目标使用现有反馈前四项，未新增隐藏完成条件。**“体验满仓”“拆路一月”“重新接路”是建议练习，不是必做步骤，也不记录步骤历史**；直接建立两仓连通路线同样允许获胜。关卡不要求某个固定农庄/仓格或全部农庄联网。

玩家短提示可按当前只读诊断显示：

- **建仓**：目标650担，初仓最多600担。另建官仓，并让农庄和新仓接入同一连续道路。
- **接路**：只认上下左右四邻的道路格；斜角、农庄直接挨仓都不算。仓库有空位但农庄断路，新粮仍会弃置。
- **满仓**：先入仓再消费，所以月末尚有空位也可能已经丢粮。看“上次月结满仓损耗”和“下次月结预演”，增仓后接路。
- **断路**：当月断路损耗即时弃置。重连只恢复下一次月结，没有缓存补发；有路也需要实际用工才能生产。

`evaluateCityWarehouseTraining` 返回中文带数量提示，分别标记上次已发生损耗、保持当前布局的下次核心预演、当前连通/容量。缺少或冲突的 v4 快照返回 `valid=false`、单条 `Unknown`，不编造原因。普通人口、国库、月度收支问题沿用其 `goals` 中已有反馈。

## 确定成功/失败路线

以下坐标仅保证在**默认空白陆地新城**可建；不是对任意载入地形的承诺。开始月为-1046年1月，表中月份是开始后经过的模拟月数。没有通过修改城市存档或额外资源提高可达性。

1. 明确开始训练，保留预置(5,2)官仓。建8座分散民居：(20,20)、(30,20)、…、(90,20)；建4农庄：(7,4)…(10,4)；建连续道路：(5,3)…(10,3)。实际建设费用310贯，剩4690贯。
2. 逐月推进。第10月实际满仓丢5担，月底仍有596担库存。尝试拆预置官仓会因有库存被拒绝，粮与资金保持。
3. 拆(6,3)道路。第11月4农庄均无仓库通路，真实产量9担、入仓0、断路弃置9担，库存降至592。
4. 重建(6,3)，新建官仓(10,2)，总容量1200。第12月重新入仓8担，只来自该月生产，无补发。
5. 第19月人口32、存粮677、国库5058，四项目标同时达成，只发一枚通关徽记。通关后拆两仓道路并继续8个月，库存实际回落到目标以下，Won保持且不重发徽记。

| 失败路线 | 真实限制 | 结果 |
|---|---|---|
| 什么都不建 | 人口0、没有农庄；仍只有600容量 | 第24月Lost，无徽记 |
| 相同8宅/4农庄/6道路，坚持只用初仓 | 有产量、道路连通，但不断出现满仓损耗，总库存≤600 | 第24月Lost，无徽记 |
| 相同8宅/4农庄并建第二仓，始终不修道路 | 总容量1200仍无法运输，实际月账记录断路损耗 | 第24月Lost，无徽记 |

真实暂停 tick/刷新不消耗期限；暂停下明确推进“+1月”仍计时。到期月包含最后达标机会，未达标才Lost；通关/失败后继续经营不会翻转本次结算。存粮或人口回落不会撤销已获得的徽记。

## 独立 API 与 App 接线

新增 `game_lab/app/city_warehouse_training.hpp`，依赖基线已发布的核心查询、目标反馈适配器、关卡/元数据头。`selfAuthoredWarehouseTraining()` 是单独定义，不改 `selfAuthoredCityScenarios()` 的三个元素。`isCityWarehouseTrainingSpec` 注册id4/revision1到v4规则家族，捕获的有效目标和期限仍可替换并随ECS1恢复；载入不重新套目录默认值。

```cpp
#include "city_warehouse_training.hpp"

// 明确选择“官仓通路”时由主线创建候选新城/会话，全部成功后替换当前对象。
game_lab::CitySimulation candidate(game_lab::CityRuleset::RoadWarehouses);
game_lab::CityScenarioSession challenge;
const auto started = game_lab::beginCityWarehouseTraining(challenge, candidate, freshRunId, profileAwards);
// started.ok 后提交新城和会话；不重置持久profileAwards或runId分配器。

// 城市实际月份变化后观察；也允许同月刷新。使用已捕获的本关目标。
const auto result = game_lab::observeCityWarehouseTraining(challenge, city, profileAwards);
const auto guide = game_lab::evaluateCityWarehouseTraining(
    game_lab::makeCityWarehouseTrainingSnapshot(city), challenge.checkpoint().spec.goals);
// 展示result.message/remainingMonths、guide.goals及guide.hints。
// 只处理result.award作为一次新的徽记事件，不用HUD的Reached/Restored发奖。
```

这些入口不调用 build/reset/migrate/tick/advanceMonth，不写城市。begin/observe/restore 规则/关卡不匹配时返回 `ok=false`，原会话及外部账本不变；终态仍传正确v4训练反馈。明确退出训练使用已有 `disable()`，保留徽记历史。原三关继续走v3及原观察入口。

新 `CityWarehouseTrainingSnapshot` 是**只读接口增量**：既有 `CityFeedbackSnapshot`、城市年月、规则标记，加现有公开 `logisticsOverview()`、`lastLogisticsMonth()` 和 `nextMonthLogistics()` 的完整结果。没有新核心字段或新模拟状态。生产、运输、消费、未来库存均来自基线真实副本月结；只校验既有格式边界/账目一致性，不重复生产或运输算法。快照须从同一次当前城市观察生成；不要拼接另一城或旧HUD缓存。

`restoreCityWarehouseTraining(session, savedCheckpoint, loadedCity, profileAwards)` 保留原runId/起始月/目标/期限/结算，先拒绝错误规则或未知训练revision，再调用原restore匹配日期。**此入口本身只校验规则和日期，不能识别同日期异城**；完整城市主体绑定和候选原子提交属于下述主线文件容器。

## 必需的主线存档小增量（此补丁未实现）

冻结基线 `a43919c` 的 `city_game_state.hpp` 活动 ECG1 v1 仅接受 `StaffedServices` 和65580字节城市；在该基线上 `encodeCityGameState(v4, activeTraining, ...)` 会明确失败。v4自由城的131152字节EMP1可存，但它不含活动关卡元数据。**不能将自由城存档称为训练进度存档，不能另写不绑定的sidecar，也不能载入时重新begin一个新期限。** 主线正在独立接线新容器，本次未读取或验收其未发布增量；以下是原接口增量建议，不宣称主线仍未实现。

建议主线保留既有四个 `encode/decode/save/loadCityGameState` 签名，最小增加 ECG1 容器版本2：

- 旧ECG1 v1继续按现有v3规则、固定65580长度读取/写入，旧文件精确回写；原三关目录与已有旧档不迁移。
- 新ECG1 v2仅允许v4城市131152字节 + 已有72字节ECS1 + ECA1，且metadata匹配注册的id4/revision1。拒绝v4配原三关、v3配训练、未知revision及长度/日期错误；不向旧版本1尾随追加字段。
- 扩大完整挑战文件读取上界；保持现有校验和覆盖头、完整城市字节、ECS1和ECA1。先校验长度/version与整体摘要绑定，再在候选中验证EMP1、ECS1、ECA1并合并当前持久徽记历史，使用restore，不调用begin；全部成功才原子替换 city/session/awards。
- 保存先观察当前v4训练到相同月份，再编码和解码预验，继续同目录临时文件+原子替换。载入/保存错误不部分改城市、会话或档案。runId分配器和徽记历史继续持久保留；载入旧保存取并集，不能覆盖已有收据。

该增量应新增实际v4训练容器往返、当前/旧通关前存档去重、断路/满仓状态续跑、同日期异城混配摘要失败、未知版本/错误规则/损坏/尾随不污染的集成测试。此模块未修改该容器，也未做原生存档面板/UI验收。

## 可执行验证与范围

在本任务独立目录运行；`--output` 必须是不存在的绝对任务目录。baseline目录未包含在补丁中；也可将 `--source-root` 指向同一发布版本的作者源码检出，不访问原版资源。

```sh
python3 scripts/city_warehouse_training_runner.py --source-root baseline/warehouse-published-a43919c --expected-core-sha256 d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0 --mode release --output /tmp/city-warehouse-training-release-NEW
python3 scripts/city_warehouse_training_runner.py --source-root baseline/warehouse-published-a43919c --expected-core-sha256 d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0 --mode undefined --output /tmp/city-warehouse-training-undefined-NEW
```

监督器复制明确的作者源码清单，用既有clang++，每个子进程上限60秒，保存源码/命令/日志/二进制指纹。测试含真实成功路线、三种到期失败、直接解法、暂停/手动月、核心预演与全部实际月账字段一致、只读状态不变、错误/边界输入unknown、错误规则/未知revision完整状态拒绝、候选配套恢复失败原子保留、原三关ECG1 v1精确往返和旧EMP1 v1/v3恢复。

训练路线的保存恢复采用**测试内候选式内存三元组**：真实v4 EMP1 + 原ECS1 + ECA1。每次成功/断路/重连/终态的配套恢复检查原runId/起始月/期限/完整字节，继续真实月结并复核旧通关前保存不再发奖；该测试辅助函数不是已实现的产品文件容器或异城绑定保证。

v2最终Release/UBSan各 **2039检查通过**，纯头文件独立 `-fsyntax-only` 通过。新增两个审核矛盾输入、无可达农庄却有fullDiscard的隔离反例，以及真实两农庄/一担与零产量的混合连通舍入、修路后旧损耗保留回归。未修改的原第二方套件两模式各 **8 PASS / 0 FAIL / 1 SCOPE**，另列6个OBSERVE；SCOPE仅指该冻结基线的产品v4容器/绑定未纳入本模块，不是容器保存通过或新增缺陷。

真实满仓/断路/通关仍分别第10/11/19月；三条失败路线第24月库存仍400/596/317，均无奖励。第二方独立替代坐标两条路线仍第19月获胜。v1第一次测试将人口目标0误当合法边界的原始失败报告，以及后来暴露的新P2审核失败均未覆盖或删除。准确两模式命令、源码和结果指纹、历史尝试见交付manifest。
