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

// Self-authored city building model; its rules have not been compared with the original game.
enum class BuildingType : uint8_t {
    None = 0,
    Road = 1,
    House = 2,
    Farm = 3,
    Well = 4,
    Temple = 5,
    Market = 6,
    Watchtower = 7
};

static constexpr bool isValidBuildingType(BuildingType type) {
    return type == BuildingType::Road ||
           type == BuildingType::House ||
           type == BuildingType::Farm ||
           type == BuildingType::Well ||
           type == BuildingType::Temple ||
           type == BuildingType::Market ||
           type == BuildingType::Watchtower;
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
            default: return 0;
        }
    }
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

    CitySimulation() {
        reset();
    }

    void reset() {
        stats_ = CityStats{};
        terrain_.assign(kTotalTiles, 0);
        buildings_.assign(kTotalTiles, BuildingType::None);
        houseLevels_.assign(kTotalTiles, 0);
        events_.clear();
        monthAccumulator_ = 0;
        addEvent("新朝初启，百废待兴。请修筑道路与民居，开垦沃野。");
        recalculateTotals();
    }

    bool loadFromMap(const CityMap& map) {
        if (!map.valid()) return false;
        reset();
        terrain_ = map.tiles;
        addEvent("已载入地图，开始营建城邑。");
        return true;
    }

    const CityStats& stats() const { return stats_; }
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
        if (!isValidBuildingType(type)) return false;
        if (x >= kMapSize || y >= kMapSize) return false;
        if (buildings_[y * kMapSize + x] != BuildingType::None) return false;
        if (isWaterTile(x, y)) return false; // 水域不可直接建陆上建筑
        const int cost = BuildingCost::cost(type);
        return stats_.treasury >= cost;
    }

    bool build(uint32_t x, uint32_t y, BuildingType type, std::string& message) {
        if (!isValidBuildingType(type)) {
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

    void advanceMonth() {
        stats_.month++;
        if (stats_.month > 12) {
            stats_.month = 1;
            stats_.year = boundedInt(int64_t(stats_.year) + 1);
        }

        // 1. 农田丰收周期（每月基础收成 + 秋季大收成）
        if (stats_.totalFarms > 0) {
            int harvest = stats_.totalFarms * 5;
            if (stats_.month >= 8 && stats_.month <= 10) {
                const double eff = stats_.neededWorkers > 0 ? (std::max(30, stats_.employmentPercent) / 100.0) : 1.0;
                harvest += static_cast<int>(stats_.totalFarms * 30 * eff);
            }
            stats_.foodSupply = boundedInt(int64_t(stats_.foodSupply) + harvest);
            addEvent("庄园耕作，新粮入仓 " + std::to_string(harvest) + " 担。");
        }

        // 2. 粮食消费：每 10 人每月食用 1 担粮食
        const int foodNeeded = (stats_.population + 9) / 10;
        if (stats_.foodSupply >= foodNeeded) {
            stats_.foodSupply -= foodNeeded;
        } else {
            stats_.foodSupply = 0;
            // 饥荒导致人口流失与民心下降
            const int loss = std::min(stats_.population, 4);
            stats_.population -= loss;
            stats_.favor = std::max(0, stats_.favor - 5);
            addEvent("城中粮荒！民生艰蹇，有流民迁徙出境。");
        }

        // 3. 水井滋养、集市供粮与民居升级
        updateHousing();

        // 4. 移民迁入机制
        if (stats_.population < stats_.housingCapacity && stats_.foodSupply > 10) {
            const int incoming = std::min(stats_.housingCapacity - stats_.population, 6);
            if (incoming > 0) {
                stats_.population += incoming;
                addEvent("四方流民闻风来归，新迁入户籍 " + std::to_string(incoming) + " 人。");
            }
        }

        // 5. 劳动力与岗位核算
        updateLabor();

        // 6. 财政收支：居民赋税与建筑维护
        const int taxCollected = stats_.population * 1;
        const int upkeep = stats_.totalTemples * 3 +
                           stats_.totalFarms * 1 +
                           stats_.totalWells * 1 +
                           stats_.totalMarkets * 2 +
                           stats_.totalWatchtowers * 1;
        stats_.treasury = boundedInt(int64_t(stats_.treasury) + taxCollected - upkeep);

        // 7. 宗庙庇佑与治安巡更
        if (stats_.totalTemples > 0) {
            stats_.favor = std::min(100, stats_.favor + 1);
        }
        if (stats_.population >= 40) {
            if (stats_.totalWatchtowers == 0) {
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
                    bool nearWell = hasNeighbor(x, y, BuildingType::Well, 3);
                    bool nearMarket = hasNeighbor(x, y, BuildingType::Market, 4);
                    bool nearTemple = hasNeighbor(x, y, BuildingType::Temple, 5);

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

    void updateLabor() {
        stats_.availableWorkers = stats_.population / 2;
        stats_.neededWorkers = stats_.totalFarms * 8 +
                               stats_.totalWells * 2 +
                               stats_.totalTemples * 4 +
                               stats_.totalMarkets * 4 +
                               stats_.totalWatchtowers * 2;
        if (stats_.neededWorkers == 0) {
            stats_.employmentPercent = 100;
        } else {
            const double ratio = static_cast<double>(stats_.availableWorkers) / stats_.neededWorkers;
            stats_.employmentPercent = std::clamp(static_cast<int>(ratio * 100.0), 0, 100);
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
        int roads = 0, houses = 0, farms = 0, wells = 0, temples = 0, markets = 0, watchtowers = 0;
        for (size_t i = 0; i < kTotalTiles; ++i) {
            switch (buildings_[i]) {
                case BuildingType::Road: ++roads; break;
                case BuildingType::House: ++houses; break;
                case BuildingType::Farm: ++farms; break;
                case BuildingType::Well: ++wells; break;
                case BuildingType::Temple: ++temples; break;
                case BuildingType::Market: ++markets; break;
                case BuildingType::Watchtower: ++watchtowers; break;
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
        updateHousing();
        updateLabor();
        updateCityRank();
    }

    const std::vector<std::string>& events() const { return events_; }

    void addEvent(const std::string& text) {
        if (events_.size() >= 50) events_.erase(events_.begin());
        events_.push_back(text);
    }

    std::vector<uint8_t> saveSnapshot() const {
        std::vector<uint8_t> buf;
        buf.reserve(4 + 4 + 36 + kTotalTiles * 2 + kTotalTiles * 2);
        buf.push_back('E'); buf.push_back('M'); buf.push_back('P'); buf.push_back('1');
        uint32_t ver = 1;
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
        return buf;
    }

    bool loadSnapshot(const std::vector<uint8_t>& buf) {
        constexpr size_t expectedSize = 44 + kTotalTiles * 4;
        if (buf.size() != expectedSize || buf[0] != 'E' || buf[1] != 'M' || buf[2] != 'P' || buf[3] != '1') return false;
        size_t offset = 4;
        auto word = [&]() {
            uint32_t result = 0;
            for (unsigned i = 0; i < 4; ++i) result |= uint32_t(buf[offset++]) << (8 * i);
            return result;
        };
        if (word() != 1) return false;
        auto signedWord = [&]() {
            const uint32_t bits = word();
            return static_cast<int32_t>(bits <= 0x7fffffffu ? int64_t(bits) : int64_t(bits) - 0x100000000ll);
        };
        CitySimulation candidate = *this;
        candidate.stats_ = CityStats{};
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
        candidate.recalculateTotals();
        // Redundant capacity/level data must agree with the parsed buildings.
        // Reject rather than silently normalizing malformed state.
        if (candidate.saveSnapshot() != buf) return false;
        candidate.addEvent("已读取自写城邑存档。");
        *this = std::move(candidate);
        return true;
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
    static int boundedInt(int64_t value) {
        return static_cast<int>(std::clamp(value, int64_t(std::numeric_limits<int>::min()),
                                          int64_t(std::numeric_limits<int>::max())));
    }
    CityStats stats_;
    std::vector<uint16_t> terrain_;
    std::vector<BuildingType> buildings_;
    std::vector<uint8_t> houseLevels_;
    std::vector<std::string> events_;
    int monthAccumulator_ = 0;
};

} // namespace game_lab
