#pragma once
#include "../city_simulation.hpp"
#include <string>
namespace game_lab {
inline std::string describeCityFarmWarehouseRoute(const CityFarmWarehouseRoute& route) {
    if (!route.valid) return "仓储城勘查农庄：黄线为可达道路，青框为官仓";
    std::string text="农庄("+std::to_string(route.farmTile%128)+","+std::to_string(route.farmTile/128)+")：道路"+
        std::to_string(route.roadTiles.size())+"格，通仓"+std::to_string(route.warehouseTiles.size())+"座，余量"+std::to_string(route.freeCapacity)+"担";
    if(route.warehouseTiles.empty())text+="；道路未通仓，须各贴同一片四邻道路";
    else if(route.freeCapacity==0)text+="；道路已通，官仓已满";
    else text+="；道路已通，入仓仍受用工和其他农庄影响";
    if(!route.hasWorkers)text+="；当前无工";
    return text;
}
} // namespace game_lab
