#pragma once
#include "../city_simulation.hpp"
#include <string>

namespace game_lab {
// Transient authored UI selection: never serialized and never reserves or moves grain.
class CityWarehouseTransferSelection {
public:
    bool active() const { return active_; }
    bool ready() const { return active_ && target_; }
    uint32_t sourceX() const { return sx_; }
    uint32_t sourceY() const { return sy_; }
    uint32_t targetX() const { return dx_; }
    uint32_t targetY() const { return dy_; }
    void cancel() { *this = {}; }
    bool begin(const CitySimulation& city,uint32_t x,uint32_t y,std::string& error) {
        if (city.ruleset()!=CityRuleset::RoadWarehouses ||
            city.buildingAt(x,y)!=BuildingType::Warehouse || city.warehouseStockAt(x,y)<=0) {
            error="请先勘查一座有粮的官仓"; return false;
        }
        sx_=x;sy_=y;active_=true;target_=false;error="请选择另一座官仓作为目标";return true;
    }
    bool choose(const CitySimulation& city,uint32_t x,uint32_t y,std::string& error) {
        if (!active_ || city.ruleset()!=CityRuleset::RoadWarehouses ||
            city.buildingAt(sx_,sy_)!=BuildingType::Warehouse ||
            city.buildingAt(x,y)!=BuildingType::Warehouse || (sx_==x && sy_==y)) {
            error="目标须为另一座官仓；尚未搬粮";return false;
        }
        dx_=x;dy_=y;target_=true;error="请核对数量后点击搬粮，或取消";return true;
    }
    int maximum(const CitySimulation& city) const {
        if (!ready() || city.ruleset()!=CityRuleset::RoadWarehouses ||
            city.buildingAt(sx_,sy_)!=BuildingType::Warehouse ||
            city.buildingAt(dx_,dy_)!=BuildingType::Warehouse) return 0;
        return std::min(city.warehouseStockAt(sx_,sy_),
                        CityWarehouseRules::capacity-city.warehouseStockAt(dx_,dy_));
    }
    bool commit(CitySimulation& city,const std::string& text,std::string& message) {
        if (!ready() || text.empty() || text.size()>6) {
            message="先选目标仓，并输入1至600之间的整数";return false;
        }
        int amount=0;
        for(char digit:text) {
            if(digit<'0' || digit>'9') {message="搬粮数量只能输入整数";return false;}
            amount=amount*10+(digit-'0');
            if(amount>CityWarehouseRules::capacity) {message="搬粮数量不得超过600担";return false;}
        }
        if(!city.transferWarehouseStock(sx_,sy_,dx_,dy_,amount,message)) return false;
        cancel();return true;
    }
private:
    bool active_=false,target_=false;
    uint32_t sx_=0,sy_=0,dx_=0,dy_=0;
};
} // namespace game_lab
