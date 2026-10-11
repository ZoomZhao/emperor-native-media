# 自写经营目标与原因反馈

当前 App 已集成 v2 模块并完成有限真实窗口验收，结果见 [GAMEPLAY_INCREMENT.md](GAMEPLAY_INCREMENT.md)。纯 C++17 头文件只读评估目标；adapter 读取公开城市快照与核心 `farmYieldForMonth()`，不推进月份，不修改模型、事件或 EMP1。关卡数值为自写配置，未与原版规则对比。

自由营建默认目标为人口50、存粮120、国库500、当前人口税减维护≥0，可通过 `CityGoalConfig` 替换。状态为Unknown/InProgress/Met/Regressed，单次转变为Reached/Lost/Restored。目标达成后可以继续经营，失守可恢复。可选限期关卡复用本反馈的当前四项数值，采用独立原期限与终态记录，见 [CITY_SCENARIOS.md](CITY_SCENARIOS.md)；没有破产终局或强制停止。

`updateCityHUD()` 按城市规则选择 `StaffedServicesV1` 或旧城的 `LaborLimitedIntegerV1`。新规则的权威下月预测来自 `nextMonthStats()` 的真实月结副本，纳入服务范围下降造成的额外住房流失；原城市与事件不变，缺失或冲突时返回Unknown。可见目标区显示状态、四项数值和差额、下月新粮/消费/迁入/税费及最主要原因，tooltip 补充其他经营原因。勘查、建造失败与存档信息保留各自信息栏。新城、成功换图和成功载入重置临时目标历史；取消或失败载入保留历史；历史不写入 EMP1。服务规则与窗口复验见 [SERVICE_LABOR.md](SERVICE_LABOR.md)。

月序为进入下月 → 收成 → 按旧人口向上取整消费 → 粮荒最多流失 4 人 → 更新住宅 → 有空房且存粮 >10 时最多迁入 6 人 → 更新岗位 → 按新人口税收减维护 → 繁荣和称号。存粮与国库使用与实际核心一致的 int 饱和边界。

农庄常规与秋收两部分分别按精确整数比例向下取整，合计为全城产出。3 农庄、21 工人/30 岗位秋收为 `10+63=73`；6 人/2 农庄常规合计为 1，不能逐庄截断后相加。adapter 读取权威 helper 并核对所选规则；helper 缺失、规则不符、旧 double ID、未知 ID 返回 unknown，不编造产量、粮荒或迁入预测。

原因包括暂停、无住宅、住房满额、容量不足、无农庄、缺工、消费超过收成、粮荒、迁入余粮门槛、维护亏损、负债和建造资金不足。全城建筑总数无法证明某座住宅的空间覆盖；对此明确 unknown，提示勘查实际周边。不把“有更楼”描述为实现了火灾/治安系统。

无效快照、配置或历史返回 Unknown 并保留旧历史；检查建筑总数、住宅容量和人口、月份/速度与非负粮食，不能替代逐格存档校验。反馈不改变城市序列化字节或事件。

服务核心 SHA-256 为 `4a21f495f3353594cc34f20806d19ead8e851e71b874ab2f2a0586834b34874d`，先前整数核心为 `9b5647534450e3a2c8c9b4818003093500bf7c6afc78cb0409126bd44658d3b4`。监督器先核对显式规则对应的核心哈希，复制明确列出的文本并为每个子进程设置 60 秒上限。新服务规则Release/UBSan各244检查、36月真实模型对照通过；服务专门测试两模式各2634检查。旧bb00e6b的223检查仅属于历史基线。

```sh
python3 scripts/city_goal_feedback_runner.py --harvest-rule integer-v1 --mode release --output /absolute/new-goal-release
python3 scripts/city_goal_feedback_runner.py --harvest-rule integer-v1 --mode undefined --output /absolute/new-goal-undefined
python3 scripts/city_goal_feedback_runner.py --harvest-rule services-v1 --mode release --output /absolute/new-goal-service-release
python3 scripts/city_goal_feedback_runner.py --harvest-rule services-v1 --mode undefined --output /absolute/new-goal-service-undefined
```

实际空资源、默认静音 App 窗口已验证目标达成、拆井失守和重建恢复，目标/原因完整可读；原生面板保存、载入、另存后完整字节一致，实例正常退出。未读取 GameData 或原游戏代码，未验证声音、原版兼容或全部窗口尺寸。
