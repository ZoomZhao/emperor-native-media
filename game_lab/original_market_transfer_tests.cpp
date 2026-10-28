// SPDX-License-Identifier: MIT
// Authored API-domain tests. Wide bays do not establish an original capacity.
#include "original_building_scene.hpp"
#include "original_figure_update.hpp"
#include <iostream>
#include <stdexcept>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool v,const char* label){++checks;if(!v)throw std::runtime_error(label);}
struct Case {
    OriginalFigureBuildingLink market,house;
    OriginalFigureUpdateContext context;
    int count=7;
    Case(){
        market.type=59;market.alive=1;market.marketQuality=60;market.marketGoods=0;
        market.marketStorage=std::vector<OriginalStorageCompartment>(6,{0,0,10001});
        market.marketDemand=std::array<int16_t,7>{};
        house.type=3;house.alive=1;house.residents=20;house.accepting=1;house.size=1;
        house.houseGoods=std::array<int16_t,12>{};house.houseGoods->at(7)=123;
        house.foodQuality=20;
        for(unsigned col=9;col<=13;++col)context.houseMarketModels[col][1]=10;
    }
};
std::vector<int64_t> link(const OriginalFigureBuildingLink& b){
    std::vector<int64_t> out{b.type,b.alive,b.residents,b.houseModel,b.serviceCount,b.primaryFigure};
    const auto scalar=[&](const auto& value){out.push_back(value.has_value());if(value)out.push_back(*value);};
    scalar(b.marketQuality);scalar(b.marketGoods);scalar(b.foodQuality);
    out.push_back(b.marketStorage.has_value());if(b.marketStorage)for(const auto& c:*b.marketStorage){out.push_back(c.resource);out.push_back(c.amount);out.push_back(c.capacity);}
    out.push_back(b.marketDemand.has_value());if(b.marketDemand)for(auto v:*b.marketDemand)out.push_back(v);
    out.push_back(b.houseGoods.has_value());if(b.houseGoods)for(auto v:*b.houseGoods)out.push_back(v);
    return out;
}
std::vector<int64_t> snapshot(const Case& c){auto out=link(c.market);const auto h=link(c.house);out.insert(out.end(),h.begin(),h.end());out.push_back(c.count);return out;}
void rejected(Case c,const char* label){const auto before=snapshot(c);need(!originalFigureMarketHouse(c.market,c.house,c.context,c.count),label);need(snapshot(c)==before,"refusal preserves stock, quality, demand, cache, house and count");}
int64_t stock(const OriginalFigureBuildingLink& m,unsigned resource){int64_t n=0;for(const auto& c:*m.marketStorage)if(c.resource==resource)n+=c.amount;return n;}
void leaf(){
    constexpr unsigned resources[]{28,19,13,25,23,22,24},indices[]{0,1,6,2,4,5,3};
    for(unsigned k=0;k<7;++k)for(unsigned amount:{9999u,10000u,10001u}){
        Case c;c.market.marketStorage->at(0)={resources[k],amount,10001};
        if(amount>=10000){rejected(c,"all seven resource lanes reject unconsumed sentinel credits");continue;}
        const auto before=stock(c.market,resources[k]);const auto old=c.house.houseGoods->at(indices[k]);
        need(originalFigureMarketHouse(c.market,c.house,c.context,c.count),"9999 source remains usable");
        need(before-stock(c.market,resources[k])==c.house.houseGoods->at(indices[k])-old,"each accepted source debit equals its actual house credit");
        need(c.house.houseGoods->at(indices[k])==10&&c.count==8&&c.house.houseGoods->at(7)==123,"normal quantity, count and unrelated words preserved");
    }
    Case partial;partial.market.marketStorage->at(0)={28,5,10001};partial.market.marketStorage->at(1)={28,10000,10001};
    rejected(partial,"small bay exhausted before sentinel bay rolls back its accepted prefix");
    Case late;late.market.marketStorage->at(0)={28,30,10001};late.market.marketStorage->at(1)={19,10000,10001};
    rejected(late,"later material mismatch rolls back earlier food and quality");
    Case missing;missing.market.marketStorage->at(0)={28,30,10001};missing.market.marketStorage->at(1)={19,30,10001};missing.context.houseMarketModels.erase(12);
    rejected(missing,"missing late model rolls back food, material and demand prefixes");
    Case select;select.house.residents=4;select.market.marketStorage->at(0)={28,10000,10001};select.market.marketStorage->at(1)={28,4,10001};
    need(originalFigureMarketHouse(select.market,select.house,select.context,select.count),"a large untouched bay does not impose an invented capacity limit");
    need(select.market.marketStorage->at(0).amount==10000&&select.market.marketStorage->at(1).amount==2&&select.house.houseGoods->at(0)==2,"smallest eligible bay preserves the other bay identity and amount");
    Case full;full.house.houseGoods->at(0)=40;full.market.marketStorage->at(0)={28,10000,10001};
    need(originalFigureMarketHouse(full.market,full.house,full.context,full.count)&&stock(full.market,28)==10000&&full.house.houseGoods->at(0)==40,"saturated demand needs no transfer and is not rejected by a guessed bay capacity");
    Case last;last.house.residents=2;last.market.marketStorage->at(0)={28,1,800};
    need(originalFigureMarketHouse(last.market,last.house,last.context,last.count)&&*last.market.marketQuality==0&&*last.house.foodQuality==60&&last.house.houseGoods->at(0)==1,"last food clears market quality but retains entry quality for this house");
    Case wrap;for(auto& col:wrap.context.houseMarketModels)col.second[1]=0;wrap.market.marketDemand->at(1)=INT16_MIN;
    need(originalFigureMarketHouse(wrap.market,wrap.house,wrap.context,wrap.count)&&wrap.market.marketDemand->at(1)==INT16_MAX,"non-stock demand retains existing signed16 decrement wrap");
    Case credit;credit.house.residents=32767;credit.house.houseGoods->at(0)=32760;credit.market.marketStorage->at(0)={28,9999,10001};
    rejected(credit,"stock credit word wrap cannot masquerade as quantity conservation");
    Case count;count.count=INT32_MAX;count.market.marketStorage->at(0)={28,10,10001};rejected(count,"service count overflow refuses before writes");
    Case model;model.house.size=2;model.context.houseMarketModels[9][1]=INT32_MAX;model.market.marketStorage->at(0)={28,30,10001};rejected(model,"unrepresentable model multiplication rolls back food prefix");
    Case wide;wide.context.houseMarketModels[9][1]=INT32_MAX;wide.house.houseGoods->at(1)=32760;wide.market.marketStorage->at(0)={19,unsigned(INT32_MAX/6),unsigned(INT32_MAX)};
    rejected(wide,"wide intermediate credit is checked without signed overflow");
    Case alias;auto same=alias.house;same.marketStorage=alias.market.marketStorage;same.marketDemand=alias.market.marketDemand;same.marketQuality=60;
    const auto old=link(same);int n=0;need(!originalFigureMarketHouse(same,same,alias.context,n)&&link(same)==old&&n==0,"market and recipient cannot be the same mutable record");
    Case empty;empty.house.residents=0;empty.market.marketStorage.reset();const auto before=snapshot(empty);
    need(originalFigureMarketHouse(empty.market,empty.house,empty.context,empty.count)&&snapshot(empty)==before,"non-elite empty house remains a byte-pure no-op before optional-field checks");
}
struct Segment {
    OriginalFigureUpdateContext context;
    OriginalFigure figure;
    Segment(){
        Case c;c.market.marketStorage->at(0)={28,100,10001};c.market.marketGoods=100;c.market.serviceCount=299;
        context=c.context;context.buildings=std::map<size_t,OriginalFigureBuildingLink>{{1,c.market},{2,c.house},{3,c.house}};
        context.services.emplace();context.services->serviceGrid.assign(51984,0x80000000u);
        context.terrain.assign(51984,0);context.feature.assign(51984,0);context.buildingOwners.assign(51984,0);context.cropBase=0;context.activeSide=10;
        figure.type=23;figure.slot=7;figure.alive=1;figure.owner=1;figure.x=4;figure.y=4;figure.cell=916;figure.progress=19;figure.direction=2;figure.previousDirection=2;figure.roamState=1;figure.roamLimit=1000;
        context.buildingOwners[916]=2;context.buildingOwners[917]=3;
    }
};
std::vector<int64_t> snapshot(const Segment& c){
    std::vector<int64_t> out{int64_t(c.figure.slot),c.figure.type,c.figure.owner,c.figure.progress,c.figure.roamSteps,c.figure.watchCoverage};
    for(const auto& b:*c.context.buildings){out.push_back(b.first);const auto values=link(b.second);out.insert(out.end(),values.begin(),values.end());}
    for(auto v:c.context.services->serviceGrid)out.push_back(v);
    return out;
}
void segment(){
    Segment late;late.context.buildings->at(3).houseModel=99;const auto before=snapshot(late);
    need(!originalFigureEntertainmentSegment(late.figure,late.context),"a later house refusal rejects the whole market segment");
    need(snapshot(late)==before,"earlier serviced house, owner cache/count and grid all roll back");
    const auto roam=originalFigureRoam(late.figure,6,0,late.context);
    need(!roam.state&&roam.status==OriginalFigureUpdateStatus::SegmentService&&snapshot(late)==before,"movement propagates the refusal without publishing a prefix");
    Segment cell;cell.figure.cell=51984;const auto cellBefore=snapshot(cell);
    need(!originalFigureEntertainmentSegment(cell.figure,cell.context)&&snapshot(cell)==cellBefore,"final invalid grid position rolls back already-computed houses and owner count");
    Segment ok;auto* owner=&ok.context.buildings->at(1);auto* house=&ok.context.buildings->at(2);
    need(originalFigureEntertainmentSegment(ok.figure,ok.context),"multiple-house segment succeeds as one transaction");
    need(owner==&ok.context.buildings->at(1)&&house==&ok.context.buildings->at(2),"publication preserves map node references");
    need(ok.context.buildings->at(2).houseGoods->at(0)==20&&ok.context.buildings->at(3).houseGoods->at(0)==20&&stock(*owner,28)==60&&owner->serviceCount==300,"repeated radius callbacks conserve every debit and preserve count cap");
    need(ok.context.services->serviceGrid[916]==(0x80000000u|(7u<<21)),"successful segment marks only the market lane preserving other bits");
    Segment wrapped;wrapped.context.buildings->at(1).serviceCount=INT16_MAX;
    need(originalFigureEntertainmentSegment(wrapped.figure,wrapped.context)&&wrapped.context.buildings->at(1).serviceCount==originalFigureWord(INT16_MAX+4),"owner service-count signed word wrap is preserved");
    Segment huge;huge.context.buildings->at(1).marketStorage->at(0).amount=10000;const auto hugeBefore=snapshot(huge);
    need(!originalFigureEntertainmentSegment(huge.figure,huge.context)&&snapshot(huge)==hugeBefore,"sentinel mismatch also rolls back through the multi-house entry point");
}
}
int main(){try{leaf();segment();std::cout<<"{\"passed\":true,\"authored_market_transfer_checks\":"<<checks<<",\"original_data\":false,\"original_capacity_claim\":false}\n";return 0;}
catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
