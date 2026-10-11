# 仓储与道路运输：冻结规则/API v1

2026-10-11，基线c239a7bc77756c03a51427f69068064da85f6c25。主线是实际仓库、App、UI与提交推送唯一负责人；本轮直接做自写玩法，不转向通用媒体测试，不重试此前被工具拒绝的动作。规则已实现于候选；最终验收见WAREHOUSE_ROADS.md。

## 最小规则

- 新增可选 `CityRuleset::RoadWarehouses = 4`、`BuildingType::Warehouse = 8`。默认自由城和现有三关仍为v3，普通载入v1/v3精确保持原规则与长度。仓储只在明确“仓储新城”或v3自由城明确转换时启用。
- 每仓容量600担、建设60贯、需工2、每月维护1贯。仓储新城预置(5,2)官仓，存粮400担、国库5000；此为初始配置，不收建设费。仓储地图若此格水域，使用首个行优先陆地；全水域地图拒绝并保留现城。
- 粮食实际存于各仓格子。`stats().foodSupply`必须等于全部仓库存总和，不能再有独立虚拟粮池。仓库库存非零时拒绝拆除并保留资金/粮食；空仓可按原2贯拆除。
- 只有道路格可连接道路网络，以上下左右四邻接洪泛；斜角不算。农庄、仓库均须四邻接至少一道路且共享同一个道路连通分量。直接农庄挨仓而无道路不算；建筑/水域不充当道路。
- 月序保持原模型：进入下月→旧人口/全部岗位计算产量→运输入仓→按旧人口扣粮→住房/粮荒稳定→迁入→岗位/税费/繁荣。仓库岗位也参与现有共享用工比例；不另造工资或单位移动系统。
- 仍用精确全城农产helper，产量按农庄格行优先均分（商+前余数各1），避免逐庄截断损失。农庄按行优先向所有可达仓库行优先填充。无路/无仓的产量记断路损耗，可达但满仓记容量损耗；当月即时弃置，无隐式农庄缓存。断路运输为0，接回下次月结恢复，不补发过去损耗。
- 消费从所有仓库行优先扣除，最多扣实际库存；不足按原粮荒处理。本最小版只模拟农庄到仓库，不做逐户路网配送。完整月账：`beginStored + produced = endStored + consumed + noRouteDiscard + fullDiscard`，`produced = delivered + noRouteDiscard + fullDiscard`。
- 旧v3的显式转换 `migrateToRoadWarehouses(x,y,message)` 需有效空陆格、原库存≤600、资金≥60；候选新增一仓/搬入全部旧粮/扣60/增加2岗位/稳定住房后原子提交。容量不足、原v1、已是v4及无效格均拒绝保留完整旧状态。界面不允许活动或终态挑战转换；v1请另建仓储城。没有静默旧档迁移。

## 公开API（已冻结源码）

```cpp
struct CityWarehouseRules {
  static constexpr int capacity = 600;
  static constexpr uint32_t initialX = 5, initialY = 2;
};
struct CityLogisticsMonth {
  bool valid = false;
  int beginStored=0, produced=0, delivered=0;
  int noRouteDiscard=0, fullDiscard=0;
  int consumed=0, unmetDemand=0, endStored=0;
};
struct CityLogisticsOverview {
  int warehouses=0, capacity=0, stored=0, connectedFarms=0, disconnectedFarms=0;
};
// CityStats新增totalWarehouses、grainStorageCapacity；其余API保留。
int CitySimulation::warehouseStockAt(uint32_t x,uint32_t y) const;
bool CitySimulation::farmHasWarehouseRoute(uint32_t x,uint32_t y) const;
CityLogisticsOverview CitySimulation::logisticsOverview() const;
const CityLogisticsMonth& CitySimulation::lastLogisticsMonth() const;
CityLogisticsMonth CitySimulation::nextMonthLogistics() const; // 副本真实月结，无副作用
bool CitySimulation::migrateToRoadWarehouses(uint32_t x,uint32_t y,std::string& message);
```

查询合同：`warehouseStockAt` 对非仓库格或越界坐标返回0；`farmHasWarehouseRoute` 对非农庄格、越界坐标或非v4规则返回false。仓满不改变道路连通查询结果；连通和剩余容量分别解释。`logisticsOverview` 在v4返回实际仓库/容量/总库存与农庄连通数；旧规则仓库、容量和连通数为0，stored仍返回旧粮池。`lastLogisticsMonth` 在初始、转换或非v4状态为valid=false且八项全0；`nextMonthLogistics` 仅在副本执行真实月结，不改变原状态。

v4 EMP1为131152字节：原65580字段/布局（version=4），后接16384个LE32仓粮，后接9个LE32（valid及上述8个账目）。非仓格库存必须0，仓粮0…600，所有库存和等于原foodSupply。valid仅接受0/1；以下是有效月账的存档读取上界，全部字段非负、端点包含：

| 字段 | 上界及约束 |
|---|---|
| beginStored、endStored | 9,830,400（16384×600） |
| produced、delivered、noRouteDiscard、fullDiscard | 各573,440（16384×35）；另须满足produced=delivered+noRouteDiscard+fullDiscard |
| consumed、unmetDemand | 各78,644，且两者之和≤78,644（16384×48/10向下取整+1） |

有效账还须满足beginStored+delivered−consumed=endStored，且endStored等于当前库存；这些是保守的格式边界，实际地图建筑数量/用工还会限制可达数值。valid=0时八字段必须全0。读取先在候选核验库存、账目、既有冗余统计和完整字节回写一致后替换；v1/v3不接受Warehouse枚举及扩展尾巴。

原ECG1 v1挑战仅继续承载v3，既有整档绑定和期限不改。本增量v4是自由营建，原生I/O有界扩展到可读完整v4；不声称已经提供仓储挑战。`CityHarvestRule::RoadWarehousesV1`依赖核心真实副本的入仓量/月结结果，缺失或配置冲突返回unknown，不把总产量误作可消费库存。

## 文件范围与验收

主线候选：`game_lab/city_simulation.hpp`、`city_state_io.hpp`、`city_game_state.hpp`、`app/city_goal_feedback.hpp`、`app/city_goal_feedback_adapter.hpp`、`app/main.mm`、`scripts/independent_runner.py`、新增`game_lab/warehouse_roads_tests.cpp`、本说明及受影响玩法文档。不编辑他人的`city_simulation_tests.cpp`长序列草稿或其他准备文件；现有关卡判定模块不改。候选暂存于本任务`warehouse-loop-20261011/candidate-source/`。

Release/UBSan独立造图验证无仓/无工、斜角、直邻无路、绕路/多仓/跨分量、满仓前入后出次序、断路重连、末格边界、库存守恒、非空仓拆除、v3显式转换和所有失败不污染、v1/v3精确保留、v4损坏/尾随拒绝及保存续跑。现有服务和三关1039集成合同需回归；最终App有限窗口走新增仓储/通路/断路/重连/勘查与原生保存恢复，保持空资源、静音及正常退出。

测试线程可基于这些公开API独立造图并复核；冻结源码及绝对候选路径已在API-FROZEN.json给出，月账上界与查询返回值补全未改变源码哈希。独立线程准备的参考算法手算和语法通过，不等于已运行候选实现。ang若继续，本阶段只准备`docs/WAREHOUSE_ROADS_PLAYTEST.md`的确定操作与期望，不编辑实际核心、App或操作UI/Git；主线继续实现，不等待该说明。

冻结核心SHA-256：`d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0`。完整来源及候选路径见本任务API-FROZEN.json。Release/UBSan各663仓储检查、各1039三关集成检查及4/7/9月可达回归通过。文件范围另包括scripts/city_goal_feedback_runner.py，增加当前共用核心指纹白名单，未改变旧规则选择。

## 手动库存搬迁增量

新增`bool transferWarehouseStock(uint32_t sx,uint32_t sy,uint32_t dx,uint32_t dy,int64_t amount,std::string& message)`。仅不同v4官仓、正数且不超源存粮/目标600担余量；完整候选提交，错误不改字节/事件，成功总粮/国库/日期/历史月账保持。没有道路、费用或自动分摊要求；暂存UI不进入既有v4布局。具体交互与验收见[WAREHOUSE_INVENTORY_TRANSFER.md](WAREHOUSE_INVENTORY_TRANSFER.md)。
