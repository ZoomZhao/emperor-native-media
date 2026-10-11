# a43919c 冻结仓储道路独立验收

**已实际执行发布实现：Release/UBSan 各9 PASS、0 FAIL，无UBSan报错或超时。本轮未发现已发布契约失败。** GUI存档面板未验收，仍是主线明确未闭合项。

基线 `a43919c6983058d9c9a73ce8f2926e60bd3e4325`。42个选定文件直接取该Git对象，不混工作树；所有五个冻结源码哈希及API说明哈希与主线API-FROZEN.json匹配。核心 `d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0`，adapter `3f75fc6825a91bd3f9839a9b23f239087d08d224b17bd3a63daf74eb60a1c1d1`。生产源码和主目录/index/分支未修改，未操作UI/ang或推送，没有原EXE/GameData/旧Swift输入。

## 两处歧义已关闭

冻结API文档SHA-256 `d8d833ff58c88911ba3f8513e6c1bc29241ba2c181164f5f00c148ebedce395c`明确：

- 月账全部非负、端点包含。beginStored/endStored≤9830400；produced/delivered/noRouteDiscard/fullDiscard各≤573440；consumed/unmetDemand各≤78644，且二者之和≤78644。还要求两条守恒式、endStored=当前库存；无效flag时八项全零。
- 非仓格或越界warehouseStockAt返回0；非农庄/越界/非v4的farmHasWarehouseRoute返回false。仓满不改变结构性通路。旧规则overview的仓库/容量/连通数为0，stored保留旧粮；初始化/转换/非v4 lastLogisticsMonth为无效且八零。

新增两组针对这些最终定义执行。9份端点合法记录经direct/ordinary-file/unified-file三条加载入口精确接受；9份超界输入三入口拒绝且保留完整城市bytes/events及会话/收据。delivered或某损耗超界同时蕴含produced超界，endStored超越9830400同时违反物理仓容/库存匹配；这些联立约束如实记录，不声称孤立归因。格式上界测试使用规范允许的自造历史账字段，**不证明实际人口/产量可达到各格式极值**。

## 九组独立测试与真实来源

| 组 | 来源、实际覆盖及结论 |
| --- | --- |
| initial-official-warehouse-map-fallback | 最小规则的(5,2)、400粮、5000国库、不收费、2岗位、月维护1；零工连通农庄无产量；水域回退首个陆格、全水图原子拒绝，通过。 |
| four-neighbor-routes-cut-rejoin-boundary | 道路仅四邻接。直邻农庄/仓无路不通、两端各邻斜路仍不通、四邻绕路恢复；拆桥损耗、接桥无旧粮补发；127,127末格通路，通过。 |
| fair-pooled-shares-multiple-stores-components | 精确全城产量按农庄行优先商+余数；仓库行优先充填。奇数产量/后一农庄断路、多仓溢入、跨分量有空仓也不能接收别处分量作物，通过。 |
| full-before-consume-no-cache-demolition | 产粮运输在旧人口消费前。满仓当月弃置，消费腾空不补发；非空拆除拒绝全状态不变，空仓拆费2，重建费60/工2/库存0，最后一仓移除后没有虚拟粮池，通过。 |
| explicit-v3-conversion-atomic-boundaries | 仅v3且空陆格、粮≤600/资金≥60。v1/v4、水/占用/越界、59/601失败完整保留；60与0/400/600成功搬粮/扣费/加工，不动时钟/速度/tick；真实住宅阈值转换后岗位竞争导致容量稳定降级，通过。 |
| v4-malformed-v1-v3-preservation | 131152与65580长度/版本、非仓库存0、仓0…600、和及月账规范。损坏/尾随/负库存/坏flag/错误守恒均在三加载入口拒绝不污染；30份以前自造旧v1精确读档并每份后续12月一致（360月），v1/v3普通读档不迁移且拒绝仓枚举/扩展尾巴，通过。 |
| v4-save-continue-authoritative-feedback | 完整自由v4文件，80次同输入/16检查点新实例恢复，涵盖暂停、速度、tick、月、断路重连。完整131152字节一致；18次独立真实月结的逐仓库存/账目/人口与只读预测一致，反馈不把断路或满仓的总作物当可消费粮；缺失和规则冲突unknown；既有ECG1不装v4挑战，通过。 |
| final-inclusive-ledger-format-bounds | 上述新明确的保守格式上下界，九正九反，三入口和失败原子性，通过。 |
| final-query-defaults-and-ledger-lifecycle | 最终默认值，空格/其他建筑/128/UINT32_MAX边界、三规则、满仓结构性通路、初始/转换/旧规则月账生命周期，通过。 |

独立参考算法位于warehouse_roads_reference.hpp，只读公开建筑及合法仓格库存，自己洪泛道路、分配农庄余数/逐仓粮、消费及损耗，不调用生产运输/寻路算法。总产量另按基线公开FarmProductionRules和原精确共享劳动力公式核对；消费ceil(旧人口/10)来源于基线明确保留的月结合同。所有数值为自写合同，不代表原版游戏。

十八次月结前后输入完整保留 `.before.emp/.after.emp`，月账表在交付根目录 `monthly-logistics-release.tsv` / `monthly-logistics-undefined.tsv`。规范化满仓/缺粮fixture只更改声明字节布局、匹配全局粮和无效八零账，再经公开loadSnapshot接受；它们是自造测试输入。

## 回归和消费者范围

同一Git副本发布仓储663、服务2634、关卡1039检查在两模式实跑通过；三关仍4/7/9月、19×3整档检查点。主线所报数字没有直接拿来当独立结果，保留本线程真实编译/运行命令与输出。

App `onEnableWarehouses` 第767行先检查 `_cityScenario.active()` 并return，随后才调用migrate；active与Running/Won/Lost状态独立，observe不会禁用终态。失败载入也在重置目标历史/选择之前返回。此项只做冻结消费者源码审查，证据见 `consumer-static-review.json`。黑盒套件的SCOPE仅表示模型没有会话参数，不能独自证明UI守卫；独立静态证据与该范围说明并存。

本次没有窗口操作，也没有证明原生保存面板能成功完成选择/写入。headless普通文件I/O成功不能关闭主线的三次GUI面板辅助失败。未验证断电、外部并发改写、防作弊或仓储挑战。

## 可复跑

审核补丁只新增C++测试/reference、监督器和本说明，不修生产代码。输出目录必须不存在，使用自己的不可变源码副本。

```sh
python3 review/scripts/run_warehouse_independent.py --phase frozen \
  --source-root source --output runs/recheck-release --mode release \
  --legacy-saves inputs/legacy \
  --expected-core-sha256 d60e45ade98531edacfbfb4ac1e3ad3e03dd8e8ae1a08cdd811b97a6a1502ce0 \
  --expected-adapter-sha256 3f75fc6825a91bd3f9839a9b23f239087d08d224b17bd3a63daf74eb60a1c1d1
```

UBSan把mode改为undefined并用新的output。若补丁应用到仓库式副本，脚本位于scripts/run_warehouse_independent.py。监督器要求两项冻结哈希、拒绝主目录，固定源码清单并二次核对捕获哈希；每子进程≤60秒，总子进程预算150秒，留存源码、输入、二进制、命令和stdout/stderr哈希。旧prepare声明占位模式仅供准备检查，**本次交付结果全部来自frozen运行**。
