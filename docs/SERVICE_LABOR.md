# 自写服务用工与旧城兼容

新空城和新地图启用自写服务用工规则。劳动力仍为人口的一半，岗位为农庄8、井2、宗庙4、市场4、更楼2；共享岗位满足比例为 `min(劳动力, 岗位)/岗位`。井、市场、宗庙的有效服务半径分别为 `floor(3/4/5 × 满足比例)`，半径0停用。邻近采用原有包含对角的方形范围。

缺工现在改变真实住宅等级和容量，范围外住宅失去服务而降级；增加劳动力或减少非必要岗位会恢复范围。人口超过新容量时立即裁剪，再核算劳动力/服务范围，直到状态稳定；恢复容量不会凭空补回居民，须在有余粮和空位时按月迁入。每月迁入后再次刷新实际服务范围。

宗庙至少有一格有效服务范围才产生现有每月繁荣+1；更楼需要全城满岗，才提供现有繁荣+1及季度警更保护。建筑即便缺工仍付既有维护费。道路不需工人；农产仍按精确全城整数比例计算；没有新增火灾、市场库存、水量或路网物流系统。这些是自写玩法选择，不代表原版规则。

勘查服务设施显示实际有效半径、全城工人/岗位及停用原因；勘查住宅显示实际等级/容量和有效邻近路、井、市场、宗庙。目标区显示服务需工规则和缺工原因，粮荒导致范围缩小的住房流失计入下月预测。预测来自 `nextMonthStats()` 的真实月结副本，原城市/事件不变。

## 存档与公开接口

EMP1长度仍为65580字节，现有九个标量/地形/建筑/等级布局不变。版本1保留旧服务存在即生效的规则；严格读取、保存和续跑保持原字节及人口。版本3标识服务用工规则；版本2仍未定义，未知版本或非规范容量/等级均事务性拒绝，不污染现有城市。旧城载入后界面明确“旧存档服务规则”，新建空城才启用新规，没有静默迁移。

现有 C++ 默认构造保持旧服务合同，便于已有消费者和旧测试续跑。App在启动、新空城和成功换图时显式使用新规则。`reset()`保留当前规则，`reset(CityRuleset)`显式选规则；保存/载入恢复规则版本。

```cpp
enum class CityRuleset : uint32_t { LegacyServices = 1, StaffedServices = 3 };
CitySimulation city(CityRuleset::StaffedServices);
city.serviceEffectiveRadius(BuildingType::Well);
city.serviceCoverageAt(x, y, BuildingType::Well);
city.serviceIsOperational(BuildingType::Watchtower);
CityStats projected = city.nextMonthStats(); // only a copy advances
```

反馈接线选择 `StaffedServicesV1`，旧城选择 `LaborLimitedIntegerV1`。不匹配规则、缺失权威预测或冲突数据返回unknown。`CityStats`既有字段和类型保持；关卡模块应通过独立配置/历史/序列化接口接入，不能复用EMP1字段。

## 独立复验

```sh
python3 scripts/independent_runner.py service --mode release --output /absolute/new-service-release
python3 scripts/independent_runner.py service --mode undefined --output /absolute/new-service-undefined
python3 scripts/city_goal_feedback_runner.py --harvest-rule services-v1 --mode release --output /absolute/new-goal-service-release
python3 scripts/city_goal_feedback_runner.py --harvest-rule services-v1 --mode undefined --output /absolute/new-goal-service-undefined
```

服务两模式各2634检查通过：自然迁入改变半径0→1→2→3；每种服务零工停用、半岗精确范围与对角边界、满岗恢复；加农庄失服和拆多余农庄恢复；粮荒4人流失另触发住房降级16人流失；旧v1规则续跑；120个v3文件检查点及各后续12tick一致；损坏/未知版本不改变城市。目标反馈两模式各244检查和36个月实际新服务模型对照通过；旧服务城市既有发布测试两模式各203检查保持通过。每个编译/测试子进程≤60秒，使用明确源码清单和完全自造输入。

本地证据目录为 `/Users/zoomzhao/Documents/Codex/2026-10-08/task-2/service-loop-20261011/`。App构建、框架链接、strict签名和嵌入源码指纹校验通过。空资源、默认静音的真实窗口复验：0/6/12/18人口对应井半径0/0/1/2，远宅在半径2恢复；10月54/60达到目标；添加8个竞争农庄后24/24、井半径0，拆完恢复容量60，再推进5月恢复54人及目标。界面勘查/原因完整可读。

原生面板分别验证v3/v1载入后另存完整65580字节一致。v3 SHA-256为 `c8c7bfe83e795252a84bdfaf2c0ffcea54728c82f3f022de7c22c2908b0c3d66`，v1为 `c10f68f0af480fc7788f2887fcfed1be8b0572e4f4ffa47b40be6e213650c919`。旧城明确旧服务规则，新建恢复新规。实例PID87114正常退出，无强制终止。两次原生面板辅助定位失败保留日志；确认面板关闭后新的独立操作成功，失败未计作通过。模型测试、有限窗口复验分别记录，未验证听觉、原版规则或全部UI路径。
