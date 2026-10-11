# 可选自写关卡、期限与统一存档

2026-10-11。城市页新增三关选择和“开始挑战”，明确新建暂停的空城。HUD显示当前四项目标、已过/剩余模拟月和胜负；到期月仍有最后一次达标机会，结算后可继续营建。“新建空城”回到自由营建。App此阶段不展示、不发放徽记、资金或粮食奖励。

关卡规则和资源完全自写，不兼容原版任务或存档。接线基线为服务增量77369a1ee5b43ed144e934f548f1014fe77c95fd；服务接线时的核心SHA-256为4a21f495f3353594cc34f20806d19ead8e851e71b874ab2f2a0586834b34874d；后续共用核心新增可选仓储v4，三关仍用v3公式和原API，4/7/9月可达及完整1039合同已回归。

## 当前可达路线

| 关卡 | 人口 / 存粮 / 国库 / 人口月度收支；期限 | 服务需工规则下的自造空城操作 | 实际首次胜利 |
|---|---|---|---|
| 聚落初成，id=1 | 24 / 200 / 4000 / ≥0；8月 | 6个彼此相隔3格的住宅，容量24 | 4个月 |
| 井畔新村，id=2 | 40 / 200 / 4000 / ≥0；12月 | 住宅(1,1)/(2,2)、路(1,2)、井(2,1) | 7个月 |
| 秋粮储备，id=3 | 48 / 450 / 3500 / ≥0；14月 | 先按第二关推进7个月到40人，再建市场(3,1)、宗庙(4,1)、4农庄(10…13,10)，继续2个月 | 9个月 |

第三关获胜时人口52、容量96、存粮517、国库4952。起步一次建完四农庄会因竞争岗位卡16人，本轮保留失败对照；界面说明已经采用先增加人口的分阶段路线。三关什么都不建分别在8/12/14个月失败。原独立模块在旧服务规则下的4/7/8月路线只属于历史合同。

## 关卡模块与反馈合同

复用独立交付v2的 `CityScenarioSession`、`city_scenario_checkpoint.hpp` 和ECS1/ECA1，不重复城市、劳动力、生产或财政公式。主线只修改第三关的操作说明以匹配当前服务规则。模块读同一时刻的城市年月和HUD当前四项目标，目标配置取本次开局捕获的 `checkpoint().spec.goals`。

```cpp
CityScenarioSession challenge;
CityScenarioAwardBook receipts;
challenge.begin(spec, freshRunId, {stats.year, stats.month}, receipts);
auto feedback = evaluateCityGoals(snapshot, challenge.checkpoint().spec.goals);
auto update = challenge.observe({stats.year, stats.month}, feedback, receipts);
```

未启用时为自由营建。`begin`只用于明确开始/重试，保存唯一非零runId、配置和起始年月；期限1–1200个模拟月。暂停刷新不推进年月，明确“+1月”会消耗一个月。首次观察已超过期限时判失败，不推测此前曾达标。胜负为终态，后续退步或达标不会翻转；终态可以继续更新保存年月，elapsed冻结在结算月。

v2修复了终态跳过反馈有效性检查的问题：Running/Won/Lost都先检查反馈有效标记、人口和目标配置。无效输入拒绝且不改变会话、日期或收据；v1的失败回归记录保留，未发布该版本。正常重复观察不会重复结算。目标HUD的Reached/Restored属于暂态，不用于关卡发奖；本App没有奖励派发。

**独立API仍有范围限制：** 只传年月和独立ECS1时，无法识别同年月的另一座合法城市。主线统一容器用联合摘要检出未重建摘要的混段与损坏；这没有把该API变成城市身份认证，也不是签名或防作弊。自行重算摘要不能靠摘要本身被识别，内部字段还需各自校验。

## 统一文件与恢复

`game_lab/city_game_state.hpp` 提供 `encode/decode/save/loadCityGameState`。v1/v3自由城为原65580字节EMP1，可选仓储v4自由城为131152字节；旧v1/v3载入为自由营建；载入不静默升级规则。挑战使用单个ECG1 v1：

| 段 | 合同 |
|---|---|
| header | 52字节：magic4，4个LE32（版本1、城市长度65580、关卡长度72、收据长度），32字节SHA-256 |
| 城市 | 完整EMP1 v3 |
| ECS1 | 精确72字节，原配置、期限、runId、start/lastObserved/settled月、状态及收据标志 |
| ECA1 | 12+8×count字节，非零唯一runId，最多4096条；12…32780字节 |

摘要覆盖前20字节和完整三段内容；系统CommonCrypto提供SHA-256，无新增下载依赖。总长度最多98484字节。完整长度、版本、合法城市、关卡配置、日期、状态和收据严格检查，全部候选成功后同时替换城市、关卡和当前收据。载入调用restore，不调用begin；缺少元数据、日期冲突、混段/损坏、未知格式、符号链接或I/O失败均保留旧状态。

当前收据与载入ECA1取并集，因此另一关结算后读旧存档可能只增长收据段；原城市、起始月、期限和runId不能变。App进程中的收据跨读档保留，并随挑战存档保存；尚无独立持久全局通关档案或奖励功能，不能声称跨所有进程和旧档案的奖励去重已实现。

原生I/O只接受有界普通文件，拒绝符号链接和FIFO。保存采用同目录临时写入、fsync、关闭和原子替换；错误清理临时文件。未验证断电和并发外部改写。ECG1、ECS1、ECA1是自写格式，不是原版兼容格式。

## 开局ID

`allocateCityRunId`在本App私有 `Application Support/EmperorNativeMedia/RunCounter.ecr` 保存ECR1计数器（16字节），通过普通文件/no-follow和非阻塞文件锁保护。新ID大于现有计数器、当前关卡和当前收据中的ID下界。载入/新空城不重置计数器；损坏、忙、耗尽拒绝开局，城市保留。该文件应保留，手工删除或回滚后的跨档案唯一性未保证。

## 验收及复验

最终集成Release/UBSan各1039检查：当前服务规则19×3个整档检查点和后续状态一致，真实4/7/9月胜利、8/12/14月空城失败，原期限恢复，计数器边界，完整文件损坏/混段及失败不污染。覆盖v2 Won/Lost无效标记、负人口、不同目标的拒绝和完整checkpoint/收据保留；另用同年月且独立EMP1合法的另一城证明未重算联合摘要的混段拒绝。自由城v1/v3兼容也覆盖。

独立模块v2 Release/UBSan各941检查、独立头文件语法检查通过，原v1的899仅为历史记录。模块路线测试保留旧服务规则合同；新规则可达性以1039集成检查和实际窗口为准。

```sh
python3 scripts/independent_runner.py scenario-state --mode release --output /absolute/new-state-release
python3 scripts/independent_runner.py scenario-state --mode undefined --output /absolute/new-state-undefined
python3 scripts/city_scenario_runner.py --core-directory /absolute/own/game_lab --expected-core-sha256 d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0 --mode release --output /absolute/new-module-release
```

监督器只复制明确自写源码清单，每个子进程≤60秒，保留源码、日志和指纹。App构建、签名、系统框架链接和生产来源清单核验通过；构建记录本身不代表窗口通过。

最终默认静音、空资源App（PID95127）的实际窗口确认三关4/7/9月获胜、空城8个月到期失败，目标/期限/结果和第三关说明可读。第一关进行中读档仍为已过2月/剩6月，同runId续跑获胜；旧v1 EMP1载入为自由营建并显示旧服务规则。当前收据合并稳定后，获胜完整存档往返全部65740字节一致；失败完整往返全部65740字节一致。只损坏城市国库而保留旧摘要的文件被明确拒绝，此后另存与此前失败存档全部65740字节一致。新空城回自由营建，实例正常退出，未强制终止。

两次原生面板辅助定位失败保留日志，确认只剩主窗口、目标文件未创建后，新的独立保存操作成功；失败未计作通过。没有读取GameData或原代码，没有操作真实音频设备；截图和二进制未上传。

本地证据位于 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/scenario-loop-20261011/`：`state-v2-final-release/`、`state-v2-final-undefined/`、`module-v2-release/`、`module-v2-undefined/`、`app-v2-validation/`、`ui-final-session/`、`scenario-ui-acceptance.json`，以及保留的v1失败回归和v2补丁指纹。


## 可选第四关：官仓通路

原三关定义数组和v3规则不变，App另外提供“官仓通路”：仓储v4空城，24个模拟月内人口32、存粮650、国库4000且月度收支不亏。满仓/断路/重连是建议练习，直接连通两仓也可胜利。模块v2修复矛盾道路快照提示，实际城市的生产和运输仍来自原核心。

活动训练进度使用ECG1版本2完整保存，不用自由城EMP1代替；恢复原runId、捕获目标、起始月、期限和终态，混段或非法输入拒绝且旧城保持。原ECG1版本1精确规则保持。通过详情与窗口有限范围见[WAREHOUSE_TRAINING_APP.md](WAREHOUSE_TRAINING_APP.md)；模块作者范围见[CITY_WAREHOUSE_TRAINING.md](CITY_WAREHOUSE_TRAINING.md)。没有资源奖励或全局通关档案。
