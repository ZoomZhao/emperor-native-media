#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <vector>

// Independent contract oracle. Numeric building IDs come from WAREHOUSE_ROADS_API v1
// and c239a7b, not from any unfinished production implementation.
namespace warehouse_review {
constexpr unsigned dimension=128, cells=dimension*dimension;
constexpr uint8_t road=1, farm=3, warehouse=8;
constexpr int capacity=600;
struct Grid {
    std::vector<uint8_t> buildings=std::vector<uint8_t>(cells,0);
    std::vector<int> stocks=std::vector<int>(cells,0);
};
struct Expected {
    int beginStored=0, produced=0, delivered=0, noRouteDiscard=0, fullDiscard=0;
    int consumed=0, unmetDemand=0, endStored=0;
    unsigned connectedFarms=0, disconnectedFarms=0, warehouses=0;
    std::vector<int> stocks;
};
inline unsigned tile(unsigned x,unsigned y) { return y*dimension+x; }
inline std::vector<unsigned> neighbors(unsigned i) {
    std::vector<unsigned> result;
    const auto x=i%dimension, y=i/dimension;
    if(x>0)result.push_back(i-1);
    if(x+1<dimension)result.push_back(i+1);
    if(y>0)result.push_back(i-dimension);
    if(y+1<dimension)result.push_back(i+dimension);
    return result;
}
struct Roads {
    std::vector<int> component=std::vector<int>(cells,-1);
    explicit Roads(const Grid& grid) {
        int label=0;
        for(unsigned start=0;start<cells;++start) {
            if(grid.buildings.at(start)!=road || component[start]>=0)continue;
            std::vector<unsigned> queue{start};component[start]=label;
            for(size_t cursor=0;cursor<queue.size();++cursor) {
                for(auto neighbor:neighbors(queue[cursor]))
                    if(grid.buildings.at(neighbor)==road && component[neighbor]<0) {
                        component[neighbor]=label;queue.push_back(neighbor);
                    }
            }
            ++label;
        }
    }
    std::set<int> adjacent(unsigned i) const {
        std::set<int> labels;
        for(auto neighbor:neighbors(i))if(component[neighbor]>=0)labels.insert(component[neighbor]);
        return labels;
    }
    bool connects(unsigned a,unsigned b) const {
        const auto left=adjacent(a), right=adjacent(b);
        for(auto label:left)if(right.count(label))return true;
        return false;
    }
};
inline Expected settle(const Grid& grid,int pooledProduction,int oldPopulation) {
    if(grid.buildings.size()!=cells || grid.stocks.size()!=cells || pooledProduction<0 || oldPopulation<0)
        throw std::runtime_error("Invalid independent oracle input");
    Expected out;out.produced=pooledProduction;out.stocks=grid.stocks;
    std::vector<unsigned> farms, stores;
    for(unsigned i=0;i<cells;++i) {
        if(grid.buildings[i]==farm)farms.push_back(i);
        if(grid.buildings[i]==warehouse){stores.push_back(i);out.beginStored+=grid.stocks[i];}
        if(grid.stocks[i]<0 || grid.stocks[i]>capacity || (grid.buildings[i]!=warehouse && grid.stocks[i]!=0))
            throw std::runtime_error("Invalid independent stock fixture");
    }
    if(farms.empty() && pooledProduction!=0)throw std::runtime_error("Production without farms");
    out.warehouses=static_cast<unsigned>(stores.size());
    Roads roads(grid);
    for(size_t rank=0;rank<farms.size();++rank) {
        std::vector<unsigned> reachable;
        for(auto store:stores)if(roads.connects(farms[rank],store))reachable.push_back(store);
        const int share=pooledProduction/static_cast<int>(farms.size())+
            (static_cast<int>(rank)<pooledProduction%static_cast<int>(farms.size())?1:0);
        if(reachable.empty()){++out.disconnectedFarms;out.noRouteDiscard+=share;continue;}
        ++out.connectedFarms;int remaining=share;
        for(auto store:reachable) {
            const int transfer=std::min(remaining,capacity-out.stocks[store]);
            out.stocks[store]+=transfer;out.delivered+=transfer;remaining-=transfer;
        }
        out.fullDiscard+=remaining;
    }
    // c239a7b's explicit existing consumption rule, retained by the warehouse contract.
    int demand=(oldPopulation+9)/10;
    for(auto store:stores) {
        const int take=std::min(demand,out.stocks[store]);
        out.stocks[store]-=take;out.consumed+=take;demand-=take;out.endStored+=out.stocks[store];
    }
    out.unmetDemand=demand;
    if(out.beginStored+out.produced!=out.endStored+out.consumed+out.noRouteDiscard+out.fullDiscard ||
       out.produced!=out.delivered+out.noRouteDiscard+out.fullDiscard)
        throw std::runtime_error("Independent oracle failed conservation");
    return out;
}
} // namespace warehouse_review
