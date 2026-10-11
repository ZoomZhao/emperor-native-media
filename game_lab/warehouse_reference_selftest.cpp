#include "warehouse_roads_reference.hpp"
#include <iostream>
using namespace warehouse_review;
static void check(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
static void add(Grid& grid,unsigned x,unsigned y,uint8_t building,int stock=0) {
    grid.buildings[tile(x,y)]=building;grid.stocks[tile(x,y)]=stock;
}
int main() {
    Grid grid;add(grid,2,2,farm);add(grid,3,2,warehouse,400);
    auto result=settle(grid,7,0);check(result.noRouteDiscard==7 && result.delivered==0,"direct adjacency needs roads");
    grid=Grid{};add(grid,3,2,farm);add(grid,3,3,road);add(grid,4,4,road);add(grid,4,5,warehouse);
    result=settle(grid,5,0);check(result.noRouteDiscard==5,"diagonal roads are disconnected");
    add(grid,4,3,road);result=settle(grid,5,0);check(result.delivered==5 && result.stocks[tile(4,5)]==5,"cardinal detour connects");
    grid=Grid{};add(grid,2,1,farm);add(grid,2,2,road);add(grid,3,2,road);add(grid,3,1,warehouse,600);
    result=settle(grid,5,10);check(result.fullDiscard==5 && result.consumed==1 && result.endStored==599,"full warehouse before consumption loses production");
    add(grid,4,2,road);add(grid,4,1,warehouse);result=settle(grid,5,10);
    check(result.fullDiscard==0 && result.stocks[tile(3,1)]==599 && result.stocks[tile(4,1)]==5,"row-major fill then consumption");
    grid=Grid{};add(grid,2,1,farm);add(grid,5,1,farm);add(grid,2,2,road);add(grid,5,2,road);add(grid,2,3,warehouse);
    result=settle(grid,1,0);check(result.delivered==1 && result.noRouteDiscard==0,"first farm gets pooled remainder");
    result=settle(grid,3,0);check(result.delivered==2 && result.noRouteDiscard==1,"cross-component farm allocation");
    grid=Grid{};add(grid,1,1,warehouse,3);add(grid,100,100,warehouse,3);
    result=settle(grid,0,50);check(result.consumed==5 && result.stocks[tile(1,1)]==0 && result.stocks[tile(100,100)]==1,"disconnected consumption still row-major");
    grid=Grid{};add(grid,125,127,farm);add(grid,126,127,road);add(grid,127,127,warehouse);
    result=settle(grid,1,0);check(result.delivered==1,"last grid cell is reachable");
    std::cout<<"PASS independent reference oracle: nine hand-computable checks; no production implementation linked or run\n";
}
