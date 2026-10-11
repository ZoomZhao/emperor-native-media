#pragma once
#include "emperor_assets.hpp"
#include "map_geometry.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace game_lab {

// Version 2 remains reserved. Version 1 keeps its original service contract.
enum class CityRuleset : uint32_t { LegacyServices = 1, StaffedServices = 3, RoadWarehouses = 4 };

// Self-authored city building model; its rules have not been compared with the original game.
enum class BuildingType : uint8_t {
    None = 0,
    Road = 1,
    House = 2,
    Farm = 3,
    Well = 4,
    Temple = 5,
    Market = 6,
    Watchtower = 7,
    Warehouse = 8
};

static constexpr bool isValidBuildingType(BuildingType type) {
    return type == BuildingType::Road ||
           type == BuildingType::House ||
           type == BuildingType::Farm ||
           type == BuildingType::Well ||
           type == BuildingType::Temple ||
           type == BuildingType::Market ||
           type == BuildingType::Watchtower || type == BuildingType::Warehouse;
}

struct BuildingCost {
    static constexpr int cost(BuildingType type) {
        switch (type) {
            case BuildingType::Road: return 5;
            case BuildingType::House: return 15;
            case BuildingType::Farm: return 40;
            case BuildingType::Well: return 10;
            case BuildingType::Temple: return 50;
            case BuildingType::Market: return 30;
            case BuildingType::Watchtower: return 20;
            case BuildingType::Warehouse: return 60;
            case BuildingType::None: default: return 0;
        }
    }
    static constexpr const char* name(BuildingType type) {
        switch (type) {
            case BuildingType::Road: return "青石道路";
            case BuildingType::House: return "平民住宅";
            case BuildingType::Farm: return "灌溉农庄";
            case BuildingType::Well: return "饮水水井";
            case BuildingType::Temple: return "祭祖宗庙";
            case BuildingType::Market: return "市井粮肆";
            case BuildingType::Watchtower: return "巡更更楼";
            case BuildingType::Warehouse: return "官粮仓库";
            case BuildingType::None: default: return "空地";
        }
    }
};

struct BuildingLabor {
    static constexpr int workersRequired(BuildingType type) {
        switch (type) {
            case BuildingType::Farm: return 8;
            case BuildingType::Well: return 2;
            case BuildingType::Temple: return 4;
            case BuildingType::Market: return 4;
            case BuildingType::Watchtower: return 2;
            case BuildingType::Warehouse: return 2;
            default: return 0;
        }
    }
};

// Self-authored, compile-time tuning values; no original-game equivalence claim.
struct FarmProductionRules {
    static constexpr int regularYield = 5;
    static constexpr int autumnBonus = 30;
    static constexpr int firstAutumnMonth = 8;
    static constexpr int lastAutumnMonth = 10;
};

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

struct CityFarmWarehouseRoute {
    bool valid=false, hasWorkers=false;
    size_t farmTile=0;
    std::vector<size_t> roadTiles, warehouseTiles;
    int capacity=0, stored=0, freeCapacity=0;
};
struct CityStats {
    int treasury = 5000;       // 初始资金（贯/铜钱）
    int population = 0;        // 当前人口
    int housingCapacity = 0;   // 住房容量
    int foodSupply = 400;      // 存粮（担）
    int year = -1046;          // 公元前 1046 年（西周初年）
    int month = 1;             // 1-12 月
    int speed = 1;             // 0=暂停, 1=正常, 2=快速, 3=极速
    int favor = 65;            // 朝廷好感 / 繁荣度 (0-100)
    int totalRoads = 0;
    int totalHouses = 0;
    int totalFarms = 0;
    int totalWells = 0;
    int totalTemples = 0;
    int totalMarkets = 0;
    int totalWatchtowers = 0;
    int totalWarehouses = 0;
    int grainStorageCapacity = 0;

    int availableWorkers = 0;    // 适龄劳动力 (人口 / 2)
    int neededWorkers = 0;       // 营建岗位需求
    int employmentPercent = 100; // 就业率 (0-100)
    std::string rankTitle = "西周 · 拓荒村邑"; // 城市发展评定
};

class CitySimulation {
public:
    static constexpr uint32_t kMapSize = 128;
    static constexpr uint32_t kTotalTiles = kMapSize * kMapSize;
    static constexpr int kTicksPerMonth = 10;
    static constexpr size_t kLegacySnapshotBytes = 44 + kTotalTiles * 4;
    static constexpr size_t kWarehouseSnapshotBytes = kLegacySnapshotBytes + kTotalTiles * 4 + 36;

    explicit CitySimulation(CityRuleset rules = CityRuleset::LegacyServices) : ruleset_(rules) {
        reset();
    }

    CityRuleset ruleset() const { return ruleset_; }
    void reset(CityRuleset rules) {
        ruleset_ = rules;
        reset();
    }

    void reset() {
        stats_ = CityStats{};
        terrain_.assign(kTotalTiles, 0);
        buildings_.assign(kTotalTiles, BuildingType::None);
        houseLevels_.assign(kTotalTiles, 0);
        events_.clear();
        warehouseStocks_.assign(kTotalTiles, 0);
        logisticsMonth_ = {};
        if (ruleset_ == CityRuleset::RoadWarehouses) {
            const size_t i = CityWarehouseRules::initialY * kMapSize + CityWarehouseRules::initialX;
            buildings_[i] = BuildingType::Warehouse; houseLevels_[i] = 1;
            warehouseStocks_[i] = 400;
        }
        monthAccumulator_ = 0;
        addEvent("新朝初启，百废待兴。请修筑道路与民居，开垦沃野。");
        recalculateTotals();
    }

    bool loadFromMap(const CityMap& map) {
        if (!map.valid()) return false;
        if (ruleset_ == CityRuleset::RoadWarehouses) {
            size_t site = CityWarehouseRules::initialY * kMapSize + CityWarehouseRules::initialX;
            const auto dry = [&](size_t i) { return map.tiles[i] < 200 || map.tiles[i] > 240; };
            if (!dry(site)) {
                site = 0; while (site < kTotalTiles && !dry(site)) ++site;
                if (site == kTotalTiles) return false;
            }
            auto candidate = *this; candidate.reset(); candidate.terrain_ = map.tiles;
            const size_t previous = CityWarehouseRules::initialY * kMapSize + CityWarehouseRules::initialX;
            candidate.buildings_[previous] = BuildingType::None; candidate.houseLevels_[previous] = 0;
            candidate.warehouseStocks_[previous] = 0;
            candidate.buildings_[site] = BuildingType::Warehouse; candidate.houseLevels_[site] = 1;
            candidate.warehouseStocks_[site] = 400;
            candidate.recalculateTotals(); candidate.addEvent("已载入自造地图与初始官仓。");
            *this = std::move(candidate); return true;
        }
        reset();
        terrain_ = map.tiles;
        addEvent("已载入地图，开始营建城邑。");
        return true;
    }

    const CityStats& stats() const { return stats_; }
    CityStats nextMonthStats() const {
        auto candidate = *this;
        candidate.advanceMonth();
        return candidate.stats();
    }
    const CityLogisticsMonth& lastLogisticsMonth() const { return logisticsMonth_; }
    CityLogisticsMonth nextMonthLogistics() const {
        auto candidate = *this; candidate.advanceMonth(); return candidate.logisticsMonth_;
    }
    int warehouseStockAt(uint32_t x, uint32_t y) const {
        if (x >= kMapSize || y >= kMapSize || buildingAt(x,y) != BuildingType::Warehouse) return 0;
        return warehouseStocks_[y*kMapSize+x];
    }
    bool transferWarehouseStock(uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy,
                                int64_t amount, std::string& message) {
        if (ruleset_ != CityRuleset::RoadWarehouses) {
            message = "搬粮仅适用于仓储城；现有城市保持"; return false;
        }
        if (sx >= kMapSize || sy >= kMapSize || dx >= kMapSize || dy >= kMapSize ||
            (sx == dx && sy == dy) || buildingAt(sx,sy) != BuildingType::Warehouse ||
            buildingAt(dx,dy) != BuildingType::Warehouse) {
            message = "请选择两座不同的官仓；库存保持"; return false;
        }
        const size_t source = sy*kMapSize+sx, target = dy*kMapSize+dx;
        if (amount <= 0 || amount > warehouseStocks_[source] ||
            amount > CityWarehouseRules::capacity - warehouseStocks_[target]) {
            message = "数量须为正整数，且不超过源仓存粮和目标余量；库存保持"; return false;
        }
        const std::string result = "已从(" + std::to_string(sx) + "," + std::to_string(sy) +
            ")搬" + std::to_string(amount) + "担到(" + std::to_string(dx) + "," +
            std::to_string(dy) + ")；全城存粮保持" + std::to_string(stats_.foodSupply) + "担";
        auto candidate = *this;
        candidate.warehouseStocks_[source] -= int(amount);
        candidate.warehouseStocks_[target] += int(amount);
        candidate.addEvent(result);
        *this = std::move(candidate); message = result;
        return true;
    }
    CityFarmWarehouseRoute farmWarehouseRoute(uint32_t x,uint32_t y) const {
        CityFarmWarehouseRoute result;
        if (ruleset_!=CityRuleset::RoadWarehouses || x>=kMapSize || y>=kMapSize ||
            buildingAt(x,y)!=BuildingType::Farm) return result;
        result.valid=true; result.farmTile=y*kMapSize+x;
        result.hasWorkers=stats_.availableWorkers>0;
        const auto network=roadNetwork();
        const auto components=adjacentRoadComponents(result.farmTile,network.labels);
        for(size_t i=0;i<kTotalTiles;++i)
            if(network.labels[i]>=0 && std::binary_search(components.begin(),components.end(),network.labels[i]))
                result.roadTiles.push_back(i);
        result.warehouseTiles=reachableWarehouses(result.farmTile,network);
        result.capacity=int(result.warehouseTiles.size())*CityWarehouseRules::capacity;
        for(size_t i:result.warehouseTiles)result.stored+=warehouseStocks_[i];
        result.freeCapacity=result.capacity-result.stored;
        return result;
    }
    bool farmHasWarehouseRoute(uint32_t x, uint32_t y) const {
        if (ruleset_ != CityRuleset::RoadWarehouses || buildingAt(x,y) != BuildingType::Farm) return false;
        const auto network = roadNetwork();
        return !reachableWarehouses(y*kMapSize+x,network).empty();
    }
    CityLogisticsOverview logisticsOverview() const {
        CityLogisticsOverview result{stats_.totalWarehouses,stats_.grainStorageCapacity,stats_.foodSupply,0,0};
        if (ruleset_ != CityRuleset::RoadWarehouses) return result;
        const auto network = roadNetwork();
        for (size_t i=0;i<kTotalTiles;++i) if (buildings_[i] == BuildingType::Farm) {
            bool connected = false;
            for (int component : adjacentRoadComponents(i,network.labels))
                if (!network.warehouses[size_t(component)].empty()) connected = true;
            if (connected) ++result.connectedFarms; else ++result.disconnectedFarms;
        }
        return result;
    }
    bool migrateToRoadWarehouses(uint32_t x, uint32_t y, std::string& message) {
        if (ruleset_ != CityRuleset::StaffedServices) {
            message = "仅服务需工v3自由城可转换；旧规则城请另建仓储城"; return false;
        }
        if (stats_.foodSupply > CityWarehouseRules::capacity) {
            message = "现有存粮超过单仓600担，转换拒绝，粮食与城市保留"; return false;
        }
        auto candidate = *this; candidate.ruleset_ = CityRuleset::RoadWarehouses;
        if (!candidate.canBuild(x,y,BuildingType::Warehouse)) {
            message = "转换需要空陆格和60贯；现有城市保留"; return false;
        }
        candidate.warehouseStocks_.assign(kTotalTiles,0);
        candidate.warehouseStocks_[y*kMapSize+x] = stats_.foodSupply;
        candidate.logisticsMonth_ = {};
        if (!candidate.build(x,y,BuildingType::Warehouse,message)) return false;
        candidate.addEvent("已明确转换仓储规则，旧存粮完整移入新仓；仓库增加2岗位。");
        *this = std::move(candidate); message = "仓储已启用，全部旧粮入仓；农庄须连通道路，非空仓不可拆除。";
        return true;
    }
    void setSpeed(int speed) { stats_.speed = std::clamp(speed, 0, 3); }
    int monthAccumulator() const { return monthAccumulator_; }

    BuildingType buildingAt(uint32_t x, uint32_t y) const {
        if (x >= kMapSize || y >= kMapSize) return BuildingType::None;
        return buildings_[y * kMapSize + x];
    }

    bool isWaterTile(uint32_t x, uint32_t y) const {
        if (x >= kMapSize || y >= kMapSize) return false;
        // 自写原型规则：在无原二进制绑定的模型中，为提供水体障碍交互定义原型范围
        const uint16_t t = terrain_[y * kMapSize + x];
        return (t >= 200 && t <= 240);
    }

    bool canBuild(uint32_t x, uint32_t y, BuildingType type) const {
        if (!isValidBuildingType(type) || (type == BuildingType::Warehouse && ruleset_ != CityRuleset::RoadWarehouses)) return false;
        if (x >= kMapSize || y >= kMapSize) return false;
        if (buildings_[y * kMapSize + x] != BuildingType::None) return false;
        if (isWaterTile(x, y)) return false; // 水域不可直接建陆上建筑
        const int cost = BuildingCost::cost(type);
        return stats_.treasury >= cost;
    }

    bool build(uint32_t x, uint32_t y, BuildingType type, std::string& message) {
        if (!isValidBuildingType(type) || (type == BuildingType::Warehouse && ruleset_ != CityRuleset::RoadWarehouses)) {
            message = "非法或未定义的营建类型";
            return false;
        }
        if (x >= kMapSize || y >= kMapSize) {
            message = "坐标超出疆界";
            return false;
        }
        if (isWaterTile(x, y)) {
            message = "此地为水泽障碍，不可营建陆上建筑";
            return false;
        }
        const size_t idx = y * kMapSize + x;
        if (buildings_[idx] != BuildingType::None) {
            message = "该地块已有建筑，请先拆除";
            return false;
        }
        const int cost = BuildingCost::cost(type);
        if (stats_.treasury < cost) {
            message = "国库资金匮乏，不足以支应营造成本";
            return false;
        }

        stats_.treasury -= cost;
        buildings_[idx] = type;
        houseLevels_[idx] = 1;
        recalculateTotals();

        message = std::string("已营建：") + BuildingCost::name(type) + " (耗费 " + std::to_string(cost) + " 贯)";
        addEvent(message);
        return true;
    }

    bool demolish(uint32_t x, uint32_t y, std::string& message) {
        if (x >= kMapSize || y >= kMapSize) {
            message = "坐标超出疆界";
            return false;
        }
        const size_t idx = y * kMapSize + x;
        if (buildings_[idx] == BuildingType::None) {
            message = "此处为天然旷野，无须清拆";
            return false;
        }
        if (buildings_[idx] == BuildingType::Warehouse && warehouseStocks_[idx] != 0) {
            message = "仓库仍有 " + std::to_string(warehouseStocks_[idx]) + " 担存粮，请搬空或用空后再拆；库存与资金保持";
            return false;
        }
        constexpr int demolishCost = 2;
        if (stats_.treasury < demolishCost) {
            message = "国库资金不足以支付清拆工费";
            return false;
        }

        const auto oldType = buildings_[idx];
        stats_.treasury -= demolishCost;
        buildings_[idx] = BuildingType::None;
        houseLevels_[idx] = 0;
        recalculateTotals();

        message = std::string("已平整清拆：") + BuildingCost::name(oldType);
        addEvent(message);
        return true;
    }

    void tick() {
        if (stats_.speed <= 0) return;
        monthAccumulator_ += stats_.speed;
        while (monthAccumulator_ >= kTicksPerMonth) {
            monthAccumulator_ -= kTicksPerMonth;
            advanceMonth();
        }
    }

    double laborRatio() const {
        if (stats_.neededWorkers == 0) return 1.0;
        return std::clamp(static_cast<double>(stats_.availableWorkers) / stats_.neededWorkers, 0.0, 1.0);
    }

    // Pooled city yield; each component is truncated once using exact integer ratios.
    int farmYieldForMonth(int month) const {
        if (month < 1 || month > 12 || stats_.totalFarms <= 0 ||
            stats_.neededWorkers <= 0 || stats_.availableWorkers <= 0) return 0;
        const int64_t filled = std::min(stats_.availableWorkers, stats_.neededWorkers);
        const int64_t regular = int64_t(stats_.totalFarms) * FarmProductionRules::regularYield * filled / stats_.neededWorkers;
        const bool autumn = month >= FarmProductionRules::firstAutumnMonth && month <= FarmProductionRules::lastAutumnMonth;
        const int64_t bonus = autumn ? int64_t(stats_.totalFarms) * FarmProductionRules::autumnBonus * filled / stats_.neededWorkers : 0;
        return boundedInt(regular + bonus);
    }

    void advanceMonth() {
        stats_.month++;
        if (stats_.month > 12) {
            stats_.month = 1;
            stats_.year = boundedInt(int64_t(stats_.year) + 1);
        }

        // 1. 农田丰收周期（每月基础收成 + 秋季大收成，受劳动力满足率严格约束）
        updateLabor();
        if (ruleset_ == CityRuleset::RoadWarehouses) {
            transportHarvest(farmYieldForMonth(stats_.month));
        } else if (stats_.totalFarms > 0) {
            const int harvest = farmYieldForMonth(stats_.month);
            stats_.foodSupply = boundedInt(int64_t(stats_.foodSupply) + harvest);
            if (harvest == 0) {
                addEvent("农庄严重缺工，当月颗粒无收。");
            } else {
                addEvent("庄园耕作，新粮入仓 " + std::to_string(harvest) + " 担。");
            }
        }

        // 2. 粮食消费：每 10 人每月食用 1 担粮食
        const int foodNeeded = (stats_.population + 9) / 10;
        if (ruleset_ == CityRuleset::RoadWarehouses) {
            int remaining = foodNeeded;
            for (auto& stock : warehouseStocks_) {
                const int take = std::min(stock,remaining); stock -= take; remaining -= take;
            }
            logisticsMonth_.consumed = foodNeeded - remaining;
            logisticsMonth_.unmetDemand = remaining;
            stats_.foodSupply -= logisticsMonth_.consumed;
            logisticsMonth_.endStored = stats_.foodSupply;
        }
        if (ruleset_ == CityRuleset::RoadWarehouses ? logisticsMonth_.unmetDemand == 0 : stats_.foodSupply >= foodNeeded) {
            if (ruleset_ != CityRuleset::RoadWarehouses) stats_.foodSupply -= foodNeeded;
        } else {
            stats_.foodSupply = 0;
            // 饥荒导致人口流失与民心下降
            const int loss = std::min(stats_.population, 4);
            stats_.population -= loss;
            stats_.favor = std::max(0, stats_.favor - 5);
            addEvent("城中粮荒！民生艰蹇，有流民迁徙出境。");
        }

        // 3. 水井滋养、集市供粮与民居升级
        stabilizeHousingAndLabor();

        // 4. 移民迁入机制
        if (stats_.population < stats_.housingCapacity && stats_.foodSupply > 10) {
            const int incoming = std::min(stats_.housingCapacity - stats_.population, 6);
            if (incoming > 0) {
                stats_.population += incoming;
                addEvent("四方流民闻风来归，新迁入户籍 " + std::to_string(incoming) + " 人。");
            }
        }

        // 5. 劳动力与岗位核算
        stabilizeHousingAndLabor();

        // 6. 财政收支：居民赋税与建筑维护
        const int taxCollected = stats_.population * 1;
        const int upkeep = stats_.totalTemples * 3 +
                           stats_.totalFarms * 1 +
                           stats_.totalWells * 1 +
                           stats_.totalMarkets * 2 +
                           stats_.totalWatchtowers * 1 + stats_.totalWarehouses;
        stats_.treasury = boundedInt(int64_t(stats_.treasury) + taxCollected - upkeep);

        // 7. 宗庙庇佑与治安巡更
        if (stats_.totalTemples > 0 && serviceIsOperational(BuildingType::Temple)) {
            stats_.favor = std::min(100, stats_.favor + 1);
        }
        if (stats_.population >= 40) {
            if (stats_.totalWatchtowers == 0 || !serviceIsOperational(BuildingType::Watchtower)) {
                if (stats_.month % 4 == 0) {
                    stats_.favor = std::max(0, stats_.favor - 2);
                    addEvent("警更缺失！市井巡查不足，坊间偶发走水之险。");
                }
            } else {
                stats_.favor = std::min(100, stats_.favor + 1);
            }
        }

        // 8. 城市发展评定更新
        updateCityRank();
    }

    void updateHousing() {
        int capacity = 0;
        for (uint32_t y = 0; y < kMapSize; ++y) {
            for (uint32_t x = 0; x < kMapSize; ++x) {
                const size_t idx = y * kMapSize + x;
                if (buildings_[idx] == BuildingType::House) {
                    bool nearRoad = hasNeighbor(x, y, BuildingType::Road, 1);
                    bool nearWell = serviceCoverageAt(x, y, BuildingType::Well);
                    bool nearMarket = serviceCoverageAt(x, y, BuildingType::Market);
                    bool nearTemple = serviceCoverageAt(x, y, BuildingType::Temple);

                    if (nearRoad && nearWell && nearMarket && nearTemple) {
                        houseLevels_[idx] = 4; // 朱门华堂
                        capacity += 48;
                    } else if (nearRoad && nearWell && nearMarket) {
                        houseLevels_[idx] = 3; // 深宅雅舍
                        capacity += 32;
                    } else if (nearRoad && nearWell) {
                        houseLevels_[idx] = 2; // 平民木屋
                        capacity += 20;
                    } else if (nearRoad) {
                        houseLevels_[idx] = 1; // 郊野茅舍
                        capacity += 8;
                    } else {
                        houseLevels_[idx] = 1; // 闭塞小屋
                        capacity += 4;
                    }
                }
            }
        }
        stats_.housingCapacity = capacity;
        if (stats_.population > stats_.housingCapacity) {
            stats_.population = std::max(0, stats_.housingCapacity);
        }
    }

    bool hasNeighbor(uint32_t cx, uint32_t cy, BuildingType target, int radius) const {
        const int minX = std::max(0, static_cast<int>(cx) - radius);
        const int maxX = std::min(static_cast<int>(kMapSize) - 1, static_cast<int>(cx) + radius);
        const int minY = std::max(0, static_cast<int>(cy) - radius);
        const int maxY = std::min(static_cast<int>(kMapSize) - 1, static_cast<int>(cy) + radius);

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (buildings_[y * kMapSize + x] == target) return true;
            }
        }
        return false;
    }

    // Self-authored shared staffing: shortages reduce the spatial service radius.
    int serviceEffectiveRadius(BuildingType type) const {
        const int radius = type == BuildingType::Well ? 3 : type == BuildingType::Market ? 4 :
                           type == BuildingType::Temple ? 5 : 0;
        if (ruleset_ == CityRuleset::LegacyServices) return radius;
        if (stats_.neededWorkers <= 0 || stats_.availableWorkers <= 0) return 0;
        return int(int64_t(radius) * std::min(stats_.availableWorkers, stats_.neededWorkers) / stats_.neededWorkers);
    }

    bool serviceCoverageAt(uint32_t x, uint32_t y, BuildingType type) const {
        if (x >= kMapSize || y >= kMapSize) return false;
        const int radius = serviceEffectiveRadius(type);
        return radius > 0 && hasNeighbor(x, y, type, radius);
    }

    bool serviceIsOperational(BuildingType type) const {
        if (type == BuildingType::Watchtower)
            return stats_.totalWatchtowers > 0 && (ruleset_ == CityRuleset::LegacyServices ||
                   stats_.availableWorkers >= stats_.neededWorkers);
        return serviceEffectiveRadius(type) > 0;
    }

    void stabilizeHousingAndLabor() {
        // Capacity and labor can both fall after demolition or famine. Reach the
        // canonical fixed point before exposing or serializing the city.
        int previousPopulation;
        do {
            previousPopulation = stats_.population;
            updateLabor();
            updateHousing();
        } while (stats_.population < previousPopulation);
        updateLabor();
    }

    void updateLabor() {
        stats_.availableWorkers = stats_.population / 2;
        stats_.neededWorkers = stats_.totalFarms * 8 +
                               stats_.totalWells * 2 +
                               stats_.totalTemples * 4 +
                               stats_.totalMarkets * 4 +
                               stats_.totalWatchtowers * 2 + stats_.totalWarehouses * 2;
        if (stats_.neededWorkers == 0) {
            stats_.employmentPercent = 100;
        } else {
            stats_.employmentPercent = static_cast<int>(std::min<int64_t>(100, int64_t(stats_.availableWorkers) * 100 / stats_.neededWorkers));
        }
    }

    void updateCityRank() {
        if (stats_.population >= 300) {
            stats_.rankTitle = "西周 · 关中雄都";
        } else if (stats_.population >= 150) {
            stats_.rankTitle = "西周 · 繁盛郡县";
        } else if (stats_.population >= 50) {
            stats_.rankTitle = "西周 · 郁葱乡里";
        } else {
            stats_.rankTitle = "西周 · 拓荒村邑";
        }
    }

    void recalculateTotals() {
        int roads = 0, houses = 0, farms = 0, wells = 0, temples = 0, markets = 0, watchtowers = 0, warehouses = 0;
        for (size_t i = 0; i < kTotalTiles; ++i) {
            switch (buildings_[i]) {
                case BuildingType::Road: ++roads; break;
                case BuildingType::House: ++houses; break;
                case BuildingType::Farm: ++farms; break;
                case BuildingType::Well: ++wells; break;
                case BuildingType::Temple: ++temples; break;
                case BuildingType::Market: ++markets; break;
                case BuildingType::Watchtower: ++watchtowers; break;
                case BuildingType::Warehouse: ++warehouses; break;
                case BuildingType::None: default: break;
            }
        }
        stats_.totalRoads = roads;
        stats_.totalHouses = houses;
        stats_.totalFarms = farms;
        stats_.totalWells = wells;
        stats_.totalTemples = temples;
        stats_.totalMarkets = markets;
        stats_.totalWatchtowers = watchtowers;
        stats_.totalWarehouses = warehouses;
        stats_.grainStorageCapacity = warehouses * CityWarehouseRules::capacity;
        if (ruleset_ == CityRuleset::RoadWarehouses) {
            stats_.foodSupply = 0;
            for (int stock : warehouseStocks_) stats_.foodSupply += stock;
        }
        stabilizeHousingAndLabor();
        updateCityRank();
    }

    const std::vector<std::string>& events() const { return events_; }

    void addEvent(const std::string& text) {
        if (events_.size() >= 50) events_.erase(events_.begin());
        events_.push_back(text);
    }

    std::vector<uint8_t> saveSnapshot() const {
        std::vector<uint8_t> buf;
        buf.reserve(ruleset_ == CityRuleset::RoadWarehouses ? kWarehouseSnapshotBytes : kLegacySnapshotBytes);
        buf.push_back('E'); buf.push_back('M'); buf.push_back('P'); buf.push_back('1');
        const uint32_t ver = static_cast<uint32_t>(ruleset_);
        for (int i = 0; i < 4; ++i) buf.push_back(static_cast<uint8_t>((ver >> (i * 8)) & 0xff));

        auto putI32 = [&](int32_t val) {
            const uint32_t bits = static_cast<uint32_t>(val);
            for (int i = 0; i < 4; ++i) buf.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xff));
        };

        putI32(stats_.treasury);
        putI32(stats_.population);
        putI32(stats_.housingCapacity);
        putI32(stats_.foodSupply);
        putI32(stats_.year);
        putI32(stats_.month);
        putI32(stats_.speed);
        putI32(stats_.favor);
        putI32(monthAccumulator_);

        for (uint16_t t : terrain_) {
            buf.push_back(static_cast<uint8_t>(t & 0xff));
            buf.push_back(static_cast<uint8_t>((t >> 8) & 0xff));
        }
        for (auto b : buildings_) {
            buf.push_back(static_cast<uint8_t>(b));
        }
        for (uint8_t l : houseLevels_) {
            buf.push_back(l);
        }
        if (ruleset_ == CityRuleset::RoadWarehouses) {
            for (int stock : warehouseStocks_) putI32(stock);
            putI32(logisticsMonth_.valid ? 1 : 0);
            for (int value : {logisticsMonth_.beginStored,logisticsMonth_.produced,logisticsMonth_.delivered,
                             logisticsMonth_.noRouteDiscard,logisticsMonth_.fullDiscard,logisticsMonth_.consumed,
                             logisticsMonth_.unmetDemand,logisticsMonth_.endStored}) putI32(value);
        }
        return buf;
    }

    bool loadSnapshot(const std::vector<uint8_t>& buf) {
        if (buf.size() < 8 || buf[0] != 'E' || buf[1] != 'M' || buf[2] != 'P' || buf[3] != '1') return false;
        size_t offset = 4;
        auto word = [&]() {
            uint32_t result = 0;
            for (unsigned i = 0; i < 4; ++i) result |= uint32_t(buf[offset++]) << (8 * i);
            return result;
        };
        const uint32_t version = word();
        if (version != 1 && version != 3 && version != 4) return false;
        if (buf.size() != (version == 4 ? kWarehouseSnapshotBytes : kLegacySnapshotBytes)) return false;
        auto signedWord = [&]() {
            const uint32_t bits = word();
            return static_cast<int32_t>(bits <= 0x7fffffffu ? int64_t(bits) : int64_t(bits) - 0x100000000ll);
        };
        CitySimulation candidate = *this;
        candidate.ruleset_ = static_cast<CityRuleset>(version);
        candidate.stats_ = CityStats{};
        candidate.warehouseStocks_.assign(kTotalTiles,0); candidate.logisticsMonth_ = {};
        auto& stats = candidate.stats_;
        stats.treasury = signedWord();
        stats.population = signedWord();
        stats.housingCapacity = signedWord();
        stats.foodSupply = signedWord();
        stats.year = signedWord();
        stats.month = signedWord();
        stats.speed = signedWord();
        stats.favor = signedWord();
        candidate.monthAccumulator_ = signedWord();
        if (stats.population < 0 || stats.housingCapacity < 0 ||
            stats.population > int(kTotalTiles * 48) || stats.housingCapacity > int(kTotalTiles * 48) ||
            stats.foodSupply < 0 || stats.month < 1 || stats.month > 12 ||
            stats.speed < 0 || stats.speed > 3 || stats.favor < 0 || stats.favor > 100 ||
            candidate.monthAccumulator_ < 0 || candidate.monthAccumulator_ >= kTicksPerMonth) return false;
        for (auto& tile : candidate.terrain_) {
            tile = uint16_t(uint16_t(buf[offset]) | (uint16_t(buf[offset + 1]) << 8));
            offset += 2;
        }
        for (size_t i = 0; i < kTotalTiles; ++i) {
            const auto type = static_cast<BuildingType>(buf[offset++]);
            if (type != BuildingType::None && !isValidBuildingType(type)) return false;
            if (type == BuildingType::Warehouse && version != 4) return false;
            if (type != BuildingType::None && candidate.isWaterTile(i % kMapSize, i / kMapSize)) return false;
            candidate.buildings_[i] = type;
        }
        for (size_t i = 0; i < kTotalTiles; ++i) {
            const uint8_t level = buf[offset++];
            const auto type = candidate.buildings_[i];
            if ((type == BuildingType::None && level != 0) ||
                (type == BuildingType::House && (level < 1 || level > 4)) ||
                (type != BuildingType::None && type != BuildingType::House && level != 1)) return false;
            candidate.houseLevels_[i] = level;
        }
        if (version == 4) {
            for (size_t i=0;i<kTotalTiles;++i) {
                const uint32_t stock = word();
                if (stock > uint32_t(CityWarehouseRules::capacity) ||
                    (candidate.buildings_[i] != BuildingType::Warehouse && stock != 0)) return false;
                candidate.warehouseStocks_[i] = int(stock);
            }
            const uint32_t valid = word(); if (valid > 1) return false;
            auto& report = candidate.logisticsMonth_; report.valid = valid != 0;
            int* fields[] = {&report.beginStored,&report.produced,&report.delivered,&report.noRouteDiscard,
                             &report.fullDiscard,&report.consumed,&report.unmetDemand,&report.endStored};
            for (auto field : fields) { *field = signedWord(); if (*field < 0) return false; }
            if (!report.valid) { for (auto field : fields) if (*field != 0) return false; }
            else {
                constexpr int maxGrain = int(kTotalTiles)*CityWarehouseRules::capacity;
                constexpr int maxHarvest = int(kTotalTiles)*35, maxDemand = int(kTotalTiles)*48/10+1;
                if (report.beginStored > maxGrain || report.endStored > maxGrain || report.produced > maxHarvest ||
                    report.delivered > maxHarvest || report.noRouteDiscard > maxHarvest || report.fullDiscard > maxHarvest ||
                    int64_t(report.consumed)+report.unmetDemand > maxDemand ||
                    int64_t(report.produced) != int64_t(report.delivered)+report.noRouteDiscard+report.fullDiscard ||
                    int64_t(report.beginStored)+report.delivered-report.consumed != report.endStored ||
                    report.endStored != stats.foodSupply) return false;
            }
        }
        candidate.recalculateTotals();
        // Redundant capacity/level data must agree with the parsed buildings.
        // Reject rather than silently normalizing malformed state.
        if (candidate.saveSnapshot() != buf) return false;
        candidate.addEvent("已读取自写城邑存档。");
        *this = std::move(candidate);
        return true;
    }

    static int houseCapacityForLevel(int level, bool nearRoad = true) {
        if (level == 4) return 48;
        if (level == 3) return 32;
        if (level == 2) return 20;
        if (level == 1) return nearRoad ? 8 : 4;
        return 0;
    }

    int houseCapacityAt(uint32_t x, uint32_t y) const {
        if (x >= kMapSize || y >= kMapSize) return 0;
        const size_t idx = y * kMapSize + x;
        if (buildings_[idx] != BuildingType::House) return 0;
        return houseCapacityForLevel(houseLevels_[idx], hasNeighbor(x, y, BuildingType::Road, 1));
    }

    std::string inspectTile(uint32_t x, uint32_t y) const {
        if (x >= kMapSize || y >= kMapSize) {
            return "坐标超出疆界";
        }
        const size_t idx = y * kMapSize + x;
        const auto b = buildings_[idx];
        const bool water = isWaterTile(x, y);
        const std::string terrainName = water ? "河湖水域" : "陆地平原";

        if (b == BuildingType::None) {
            if (water) {
                return "🔍 勘查地块 (" + std::to_string(x) + ", " + std::to_string(y) + "): 地形=河湖水域 · 建筑=无 · 天然水泽阻隔，不可营建陆上建筑";
            }
            return "🔍 勘查地块 (" + std::to_string(x) + ", " + std::to_string(y) + "): 地形=陆地平原 · 建筑=无 · 天然旷野，可供营建";
        }

        if (b == BuildingType::Warehouse) {
            return "🔍 官粮仓库 (" + std::to_string(x) + ", " + std::to_string(y) + ") · 存粮 " +
                std::to_string(warehouseStockAt(x,y)) + "/600 担 · 需工2 · 维护1贯/月\n农庄须经四邻道路入仓；有粮时不可拆，居民统一从仓库扣粮。";
        }
        if (b == BuildingType::Farm) {
            if (ruleset_ == CityRuleset::RoadWarehouses) {
                const auto report = nextMonthLogistics();
                return "🔍 农庄 (" + std::to_string(x) + ", " + std::to_string(y) + ") · " +
                    (farmHasWarehouseRoute(x,y) ? "道路通仓" : "道路未通仓，收成损耗") +
                    " · 需工8\n下月全城生产 " + std::to_string(report.produced) + " / 入仓 " +
                    std::to_string(report.delivered) + " / 断路损耗 " + std::to_string(report.noRouteDiscard) +
                    " / 满仓损耗 " + std::to_string(report.fullDiscard) + " 担";
            }
            const int nextMonth = stats_.month == 12 ? 1 : stats_.month + 1;
            const bool autumn = nextMonth >= FarmProductionRules::firstAutumnMonth && nextMonth <= FarmProductionRules::lastAutumnMonth;
            const std::string reason = stats_.availableWorkers == 0 ? "严重缺工停产" :
                stats_.availableWorkers < stats_.neededWorkers ? "缺工折减" :
                autumn ? "满员满产，秋收丰收季" : "满员满产";
            return "🔍 农庄 (" + std::to_string(x) + ", " + std::to_string(y) +
                   ") · 需工 " + std::to_string(BuildingLabor::workersRequired(b)) +
                   " 人 · 全城岗位满足率: " + std::to_string(stats_.employmentPercent) +
                   "% (" + std::to_string(stats_.availableWorkers) + "/" + std::to_string(stats_.neededWorkers) + "工)\n" +
                   "预计" + std::to_string(nextMonth) + "月全城新粮: " + std::to_string(farmYieldForMonth(nextMonth)) +
                   " 担 · 原因: " + reason;
        }

        if (b == BuildingType::House) {
            const uint8_t lvl = houseLevels_[idx];
            const bool nearRoad = hasNeighbor(x, y, BuildingType::Road, 1);
            const int cap = houseCapacityForLevel(lvl, nearRoad);
            std::string levelName = "闭塞小屋";
            if (lvl == 4) levelName = "朱门华堂";
            else if (lvl == 3) levelName = "深宅雅舍";
            else if (lvl == 2) levelName = "平民木屋";
            else if (lvl == 1) levelName = nearRoad ? "郊野茅舍" : "闭塞小屋";

            return "🔍 民居 (" + std::to_string(x) + ", " + std::to_string(y) +
                   ") · 等级: " + std::to_string(lvl) + " (" + levelName + ") · 容量: " + std::to_string(cap) +
                   " 人 · 全城人口: " + std::to_string(stats_.population) + "/" + std::to_string(stats_.housingCapacity) +
                   "\n有效邻近：路" + (nearRoad ? "✓" : "✗") +
                   " 井" + (serviceCoverageAt(x, y, BuildingType::Well) ? "✓" : "✗") +
                   " 市" + (serviceCoverageAt(x, y, BuildingType::Market) ? "✓" : "✗") +
                   " 庙" + (serviceCoverageAt(x, y, BuildingType::Temple) ? "✓" : "✗") +
                   (ruleset_ != CityRuleset::LegacyServices ? " · 缺工会缩短服务范围" : " · 旧存档服务规则");
        }

        if (b == BuildingType::Well || b == BuildingType::Market || b == BuildingType::Temple || b == BuildingType::Watchtower) {
            const int fullRadius = b == BuildingType::Well ? 3 : b == BuildingType::Market ? 4 : b == BuildingType::Temple ? 5 : 0;
            std::string service = fullRadius > 0 ? "有效半径 " + std::to_string(serviceEffectiveRadius(b)) + "/" + std::to_string(fullRadius) + " 格" :
                "更楼需全城满岗才提供繁荣效果";
            if (ruleset_ == CityRuleset::LegacyServices) service = "旧存档规则：服务不受用工约束";
            return "🔍 " + std::string(BuildingCost::name(b)) + " (" + std::to_string(x) + ", " + std::to_string(y) +
                   ") · 需工 " + std::to_string(BuildingLabor::workersRequired(b)) + " 人 · 全城 " + std::to_string(stats_.availableWorkers) +
                   "/" + std::to_string(stats_.neededWorkers) + " 工\n" + service + " · " +
                   (serviceIsOperational(b) ? "运作中" : "缺工停用；增加人口或减少岗位");
        }

        const int workers = BuildingLabor::workersRequired(b);
        const char* name = BuildingCost::name(b);
        std::string roleDesc;
        switch (b) {
            case BuildingType::Road: roleDesc = "邻近 1 格民居可提升基础容量"; break;
            case BuildingType::Well: roleDesc = "滋养周边民居 (半径 3 格)"; break;
            case BuildingType::Temple: roleDesc = "祈福民心，每月提升朝廷好感"; break;
            case BuildingType::Market: roleDesc = "升级周边民居 (半径 4 格)，尚无供粮配送"; break;
            case BuildingType::Watchtower: roleDesc = "人口达到 40 人时，全城民心每月提升 1 点"; break;
            default: break;
        }

        std::string res = "🔍 勘查地块 (" + std::to_string(x) + ", " + std::to_string(y) + "): 地形=" + terrainName +
                          " · 建筑=" + name + " · 需工 " + std::to_string(workers) + " 人";
        if (workers > 0) {
            res += " · 全城岗位满足率: " + std::to_string(stats_.employmentPercent) + "% (" +
                   std::to_string(stats_.availableWorkers) + "/" + std::to_string(stats_.neededWorkers) + "工)";
        }
        if (!roleDesc.empty()) {
            res += " · " + roleDesc;
        }
        return res;
    }

    // 将整个 128x128 城市模拟网格渲染为 BGRA8 图像
    std::vector<uint32_t> renderGridPixels(int cursorX = -1, int cursorY = -1) const {
        std::vector<uint32_t> pixels(kTotalTiles);
        for (uint32_t y = 0; y < kMapSize; ++y) {
            for (uint32_t x = 0; x < kMapSize; ++x) {
                const size_t idx = y * kMapSize + x;
                const auto b = buildings_[idx];
                uint32_t color = 0;

                if (b == BuildingType::None) {
                    // 基础地形色彩
                    const uint16_t t = terrain_[idx];
                    if (isWaterTile(x, y)) {
                        color = 0xff2060c0; // 碧蓝色水系
                    } else if (t % 7 == 0) {
                        color = 0xff557840; // 茂盛深绿草甸
                    } else if (t % 3 == 0) {
                        color = 0xff66884a; // 浅草平原
                    } else {
                        color = 0xff709252; // 苍黄原野
                    }
                } else if (b == BuildingType::Road) {
                    color = 0xffc8c0b0; // 灰白色青石古道
                } else if (b == BuildingType::House) {
                    if (houseLevels_[idx] >= 4) {
                        color = 0xff2020e0; // 朱红金顶华堂
                    } else if (houseLevels_[idx] >= 3) {
                        color = 0xff8030b0; // 紫铜深宅
                    } else if (houseLevels_[idx] >= 2) {
                        color = 0xff4050d0; // 瓦房红顶
                    } else {
                        color = 0xff3b7cb4; // 茅舍青顶
                    }
                } else if (b == BuildingType::Farm) {
                    color = (stats_.month >= 8 && stats_.month <= 10) ? 0xff20b8e8 : 0xff388e3c; // 金黄麦浪 / 青绿禾苗
                } else if (b == BuildingType::Well) {
                    color = 0xff00d4ff; // 清润蓝白水井
                } else if (b == BuildingType::Temple) {
                    color = 0xff1020e0; // 朱红先秦金顶宗庙
                } else if (b == BuildingType::Market) {
                    color = 0xff20a0e0; // 繁盛金灿市集
                } else if (b == BuildingType::Watchtower) {
                    color = 0xff707070; // 肃穆更楼灰石
                } else if (b == BuildingType::Warehouse) {
                    color = warehouseStocks_[idx] > 0 ? 0xff30b8f0 : 0xff307898;
                }

                // 光标十字标记高亮
                if (static_cast<int>(x) == cursorX && static_cast<int>(y) == cursorY) {
                    color = 0xffffffff; // 纯白选定高亮
                }

                pixels[idx] = color;
            }
        }
        return pixels;
    }

private:
    struct RoadNetwork {
        std::vector<int> labels;
        std::vector<std::vector<size_t>> warehouses;
    };
    static std::vector<size_t> fourNeighbors(size_t i) {
        std::vector<size_t> neighbors;
        const size_t x=i%kMapSize, y=i/kMapSize;
        if (x>0) neighbors.push_back(i-1);
        if (x+1<kMapSize) neighbors.push_back(i+1);
        if (y>0) neighbors.push_back(i-kMapSize);
        if (y+1<kMapSize) neighbors.push_back(i+kMapSize);
        return neighbors;
    }
    static std::vector<int> adjacentRoadComponents(size_t i,const std::vector<int>& labels) {
        std::vector<int> ids;
        for (size_t n : fourNeighbors(i)) if (labels[n]>=0) ids.push_back(labels[n]);
        std::sort(ids.begin(),ids.end()); ids.erase(std::unique(ids.begin(),ids.end()),ids.end());
        return ids;
    }
    RoadNetwork roadNetwork() const {
        RoadNetwork net; net.labels.assign(kTotalTiles,-1);
        std::vector<size_t> queue;
        for (size_t i=0;i<kTotalTiles;++i) if (buildings_[i]==BuildingType::Road && net.labels[i]<0) {
            const int id=int(net.warehouses.size()); net.warehouses.emplace_back();
            queue.clear(); queue.push_back(i); net.labels[i]=id;
            for (size_t head=0;head<queue.size();++head)
                for (size_t n : fourNeighbors(queue[head])) if (buildings_[n]==BuildingType::Road && net.labels[n]<0) {
                    net.labels[n]=id; queue.push_back(n);
                }
        }
        for (size_t i=0;i<kTotalTiles;++i) if (buildings_[i]==BuildingType::Warehouse)
            for (int id : adjacentRoadComponents(i,net.labels)) net.warehouses[size_t(id)].push_back(i);
        return net;
    }
    static std::vector<size_t> reachableWarehouses(size_t i,const RoadNetwork& net) {
        std::vector<size_t> sites;
        for (int id : adjacentRoadComponents(i,net.labels)) {
            const auto& component = net.warehouses[size_t(id)]; sites.insert(sites.end(),component.begin(),component.end());
        }
        std::sort(sites.begin(),sites.end()); sites.erase(std::unique(sites.begin(),sites.end()),sites.end());
        return sites;
    }
    void transportHarvest(int produced) {
        logisticsMonth_ = {}; auto& report = logisticsMonth_;
        report.valid=true; report.beginStored=stats_.foodSupply; report.produced=produced;
        if (produced>0 && stats_.totalFarms>0) {
            const auto net=roadNetwork(); int ordinal=0;
            for (size_t i=0;i<kTotalTiles;++i) if (buildings_[i]==BuildingType::Farm) {
                int remaining=produced/stats_.totalFarms + (ordinal++ < produced%stats_.totalFarms ? 1 : 0);
                if (remaining==0) continue;
                const auto warehouses=reachableWarehouses(i,net);
                if (warehouses.empty()) { report.noRouteDiscard+=remaining; continue; }
                for (size_t site : warehouses) {
                    const int amount=std::min(remaining,CityWarehouseRules::capacity-warehouseStocks_[site]);
                    warehouseStocks_[site]+=amount; remaining-=amount; report.delivered+=amount;
                    if (remaining==0) break;
                }
                report.fullDiscard+=remaining;
            }
        }
        stats_.foodSupply+=report.delivered;
        addEvent("仓储月结：生产" + std::to_string(report.produced) + " / 入仓" + std::to_string(report.delivered) +
                 " / 断路损耗" + std::to_string(report.noRouteDiscard) + " / 满仓损耗" + std::to_string(report.fullDiscard) + "担。");
    }
    static int boundedInt(int64_t value) {
        return static_cast<int>(std::clamp(value, int64_t(std::numeric_limits<int>::min()),
                                          int64_t(std::numeric_limits<int>::max())));
    }
    CityRuleset ruleset_ = CityRuleset::LegacyServices;
    CityStats stats_;
    std::vector<uint16_t> terrain_;
    std::vector<BuildingType> buildings_;
    std::vector<uint8_t> houseLevels_;
    std::vector<int> warehouseStocks_;
    CityLogisticsMonth logisticsMonth_;
    std::vector<std::string> events_;
    int monthAccumulator_ = 0;
};

} // namespace game_lab
