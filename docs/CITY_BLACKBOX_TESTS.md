# 独立城市黑盒测试（自写规则）

本补丁只新增测试、监督器和说明，不修改生产模拟/App。测试不启动 App，不读原 EXE、GameData、旧 Swift，不操作屏幕、声音、设备或 ang，不访问网络。所有输出必须写到新的任务目录。

## 冻结核心复验（本补丁当前版本）

本次实际对象是主线冻结的五个头文件，原始只读目录为 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/gameplay-loop-20261011/final-core`。`city_simulation.hpp` 已核验 SHA-256 为 `9b5647534450e3a2c8c9b4818003093500bf7c6afc78cb0409126bd44658d3b4`，复制后再次复核一致。本补丁的 `strict-labor` 预测测试按此版本的公开 `farmYieldForMonth` 和“预计 N 月全城新粮”语义执行：每块农庄显示相同全城合计，不能把重复合计相加。

其余经济/住房/时钟/EMP1 规则保留先前真实契约；独立整数 oracle 在生产代码改用整数前已经存在，本轮没有弱化账目期望。原版游戏数值仍未使用。

新增/加强的验收：

- 两农庄/6 人和三农庄/42 人两类状态各遍历此前月份 1–12，独立判断下个月、季节、全城预测与实际新粮，覆盖 7→8、10→11、12→1；共 24 个场景。每块农庄都须显示明确下月与全城范围，数值等于独立 oracle，不能只验证生产与预测共用的函数彼此一致。
- `exact-pooled-yield-boundaries`：三农庄岗位 24 加井 2、市场 4，需求 30；人口 42 提供劳动力 21。严格期望为 `floor(15×21/30)+floor(90×21/30)=10+63=73`，8/9/10 月都相同，7/11 月为 10；实际次月存粮 `100+73-ceil(42/10)=168`。这个固定期望可以抓住旧浮点秋收 72 的误差。
- 同一回归还固定两农庄/6 人的常规新粮 `floor(10×3/16)=1`；实际消费 1，存粮保持 100。保留全部原有 99 个账目场景、十年经营、断粮恢复、1600 个操作属性及连续/存档新实例续跑。
- `farmYieldForMonth(0/13)` 按冻结函数明确契约返回 0；不是推测非法月份应如何处理。

本次输出另保存 `city-forecast-cases.tsv`、24 个预测前规范 EMP1，以及两个明确的 73/1 实际推进后 EMP1。冻结快照没有附带既有测试源文件，因此本次命令不使用 `--with-existing`；旧既有测试结果保留于此前交付，没有冒充这份新核心的执行结果。

## 版本与期望来源

初次验收提交为 `bb00e6b42ce29bf75cf0f67e11ec0ccfba862b4e`。其 `game_lab/city_simulation.hpp` SHA-256 为 `14d30ff19edffc099d2de9c1880fb6d7806eee1552365d9a58c66fd9981769c5`。同次捕获的未提交工作树 SHA-256 为 `7261c50bf56bdfcda00ee6a61f6c2e64fe1ba8b64d66abb7dd394adeeda3339d`。这两份是独立固定快照；后续主目录修改不会改变验收对象。

每项期望来自当前自写源码或 `docs/CITY_MODEL.md`，没有使用原版数值。下文行号对应固定快照，后续改版按函数名定位。源码审查用于确定契约；执行仅通过公开 API、公开 EMP1 字节格式及 App 共用文件接口。

必须显式选择规则配置：

- `baseline`：上述提交。常规收成每农庄 5；8–10 月额外收成按 `max(30, employmentPercent)` 折算。零劳动力依然产粮是尚未实现的依赖，不能伪装为已建立规则的失败。
- `strict-labor`：本次冻结核心。收成前核算劳动力；常规、秋收分别按全城 `min(availableWorkers/neededWorkers, 1)` 比例向下取整。数学期望使用此前独立整数有理数 oracle。冻结核心 `FarmProductionRules/farmYieldForMonth/advanceMonth` 明确建立此规则；预测测试要求其公开 API。
- `invariants`：先复跑共同状态/时钟/事务/存档属性，不执行旧版具体收成、住房、维护数值或勘查文本期望。仍以当前公开状态与 EMP1 接口为边界；不是新规则的完整验收。集成方提供新规则后，需审查并增加其真实期望，不能把旧数值强加给新设计。

## 测试矩阵及真实期望

| 测试 ID | 要验证的用户后果 | 真实期望来源 |
|---|---|---|
| `map-build-transaction` | 自造河流拒绝陆上建筑，非法地图/枚举/坐标/占地操作不损坏城市或扣钱 | `CITY_MODEL.md` 实现范围；两版 `loadFromMap` 120、`canBuild/build` 144–184；水域 200–240 是自写规则 |
| `pause-speed-manual-month` | 暂停保留所有状态和 tick 余数；相等模拟 tick 的 1/2/3x 得到相同状态和事件；暂停时手动推进仍有效 | `CITY_MODEL.md` 时钟；两版 `tick` 214–220；App `onSpeedChanged` 580–590 直接调用 `advanceMonth` |
| `fresh-instance-file-continuation` | 连续运行与多次文件保存、全新实例载入后执行相同操作，模型字节、网格像素、城市称号一致 | `CITY_MODEL.md` EMP1 合同及“下一步”标准；`city_state_io.hpp` 是 App 657/670 行的真实消费者；事件/UI 暂态不纳入保存等价 |
| `seeded-operation-properties` | 四个固定种子各 400 个操作，每步检查非负粮食、人口容量、劳动力、日期、公开网格计数、失败事务及新实例规范往返 | `CITY_MODEL.md` 下一步不变量；`CityStats` 范围，`updateLabor`，候选载入/重新核算；`BuildingCost` 是公开成本接口 |
| `housing-demolition-recovery` | 拆井/路立即降低容量、裁剪人口并更新劳动力；重建恢复容量，人口只能在后续月迁入 | 基线 `updateHousing` 297–339、`recalculateTotals` 373–397；工作树对应 307–349、383–407。容量 4/8/20/32/48、方形邻域半径 1/3/4/5 都来自自写源码 |
| `service-radius-boundaries` | 地图角落、恰好半径与半径外分别提供/不提供服务；方形邻域包括对角 | 两版公开 `hasNeighbor`；基线 341–355、工作树 351–355 附近；没有推断曼哈顿距离或道路网络 |
| `food-labor-monthly-ledgers` | 1/2/5 农庄、11 个人口样本、三个季节，共 99 个场景的粮食/迁入/税收维护账目符合选定契约 | 基线 `advanceMonth` 229–276；工作树 235–286；消费按月前人口，税按迁入后人口；岗位需求 8/2/4/4/2 来自 `BuildingLabor/updateLabor` |
| `shortage-and-migration-threshold` | 粮食耗尽移出四人并降低繁荣；存粮 10 阻止迁入、11 允许迁入 | 两版 `advanceMonth` 粮食、饥荒与迁入代码；饥荒 −5 繁荣，宗庙 +1；容量仍按建筑结构确定 |
| `production-demolition-shortage-recovery` | 拆最后一农庄使生产断链；粮食不再补充、耗尽导致人口损失；重建后恢复供粮和迁入 | `recalculateTotals → totalFarms/neededWorkers → advanceMonth` 的真实依赖；没有声称存在尚未实现的路网/市场物流断链 |
| `fiscal-constraints-and-debt` | 花完国库后拒绝建造和拆除，失败保持状态；维护支出可能产生负债 | `CITY_MODEL.md` 建造/拆除成本；基线 266–271、工作树 280–286 的财政公式；EMP1 允许有符号国库，不能断言 treasury ≥ 0 |
| `ten-year-sustainable-map` | 完全自造地图上靠自然迁入达到 80 人，之后 120 月无饥荒并产生可核账的粮食和财政盈余 | 当前自写默认容量和收成；4 个井服务住宅、9 道路、2 农庄、1 井。成熟后全年收成 300、消费 96，十年粮食 +2040；月税 80、维护 3，十年国库 +9240；这是此模型的场景，不是原版平衡标准 |
| `saturation-and-extreme-calendar` | int 极值不溢出，年饱和而月份正常循环，先饱和收成再消费 | `CITY_MODEL.md` 极值修复说明；两版 `boundedInt` 和 `advanceMonth` 累计顺序；UBSan 额外检查实际未定义行为 |
| `farm-inspection-forecast` | 每块农庄的明确“下月全城新粮”数值都等于独立 oracle 和实际总收成 | 冻结核心 `inspectTile` 明确采用 nextMonth 及全城语义；独立 24 个月份/季节场景；重复全城合计不相加 |
| `exact-pooled-yield-boundaries` | 固定 21/30 劳力三农庄秋收 73，以及 6 人两农庄常规总收成 1，避免浮点误差回归 | 冻结 `FarmProductionRules/farmYieldForMonth`，正文给出独立整数推导，实际粮食账目验证；用户要求的关键边界 |

属性序列并非无界模糊测试。固定种子为 `1 / 0x5eed / 0xc0ffee / 0xdeadbeef`，LCG 算法写在测试源码中。失败日志包含种子、步数、操作、坐标和类型，并保留该步前后 EMP1。99 个账目场景与 120 月经营场景有独立账本；不以断言数量代替用户后果。

## 历史诊断不一致（冻结核心已修复，证据保留）

以下描述针对首次捕获的旧工作树，本次冻结核心已改用“预计 N 月全城新粮”与共用精确整数公式。旧失败证据仍在此前 `city-validation/runs/delivery-working-*` 下；本轮适配测试按新语义重新验证，未删除原账目期望。

在捕获工作树建造住宅 `(10,10)`、道路 `(10,11)`、农庄 `(20,20)/(21,20)`。按公开 EMP1 合同自造人口 6、存粮 100、月份 2 的规范状态。总劳动力为 3，岗位为 16。两块农庄各显示预计 0 担；次月总产粮 1 担，消费 1 担，因此存粮仍为 100。

原因：勘查逐块计算 `floor(5 × 3/16)=0`；生产按全城计算 `floor(2 × 5 × 3/16)=1`。Release/UBSan 均复现，属于信息与实际账目不一致，未修改生产代码。集成方可选择统一分配取整，或明确显示全城合计；测试需要随实际产品合同更新。

## 明确未实现或未建立合同的机制

监督器独立记录 `UNIMPLEMENTED`，不把它们计作失败或通过：零工井/市场/宗庙仍可提供住宅升级；隔着完整河流、没有道路的农庄仍可生产；基线零工农庄仍产粮；维护负债没有破产处置；市场库存、水量、逐建筑劳动分配、路网运输/供给没有当前 API/合同。生产缺工在捕获工作树已经实现。宗庙/更楼的现有效果是繁荣增减，尚无真实灾害/损坏/消防模拟的验收对象。

## 运行与证据

```sh
python3 scripts/run_city_blackbox.py \
  --source-root /absolute/immutable-source \
  --output /absolute/new-task-output \
  --profile baseline --mode release --with-existing

python3 scripts/run_city_blackbox.py \
  --source-root /absolute/reviewed-strict-labor-source \
  --output /absolute/another-new-task-output \
  --profile strict-labor --mode undefined --with-existing
```

新集成版先用 `--profile invariants` 复跑共同契约，再按已确认的新规则补充准确账目。不要用配置名猜测新规则已经兼容旧数值。

每次运行只读取五个模型头文件，并保存独立源码快照后编译；`--with-existing` 额外复制/运行现有城市测试及其自造输入辅助头。源文件复制后复核哈希，发现并发变化则拒绝本次捕获。所有编译/测试子进程最多 60 秒，单次监督器的子进程总预算 150 秒（终止回收至多额外 5 秒）。只终止自己新建的进程组。

`report.json` 记录所测源码 SHA-256、编译/执行完整命令、退出码、超时、耗时、分项状态和全部输出/输入/二进制哈希。`blackbox.stdout` 保存分项结果；`.before.emp/.after.emp` 保存失败最小状态；成熟/终态 EMP1 和自造河流地图也保留。负向测试失败应返回非零退出码，不隐藏为通过。已有城市测试单独报告，不能覆盖独立失败。

纯模型/共享文件接口通过不证明 GUI、定时器调度、设备或完整原版可玩。App 消费者只做源码审查；本补丁未执行 UI。
