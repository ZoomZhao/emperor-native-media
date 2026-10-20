#include "app/map_surface.hpp"
#include "original_pak.hpp"
#include "cg_image_conversion.hpp"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace game_lab;
size_t checks=0;
void need(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
OriginalTerrainState state(){
    OriginalTerrainState s;const size_t n=s.cells;
    s.terrain.assign(n,0);s.image.assign(n,0);s.feature.assign(n,0);
    for(auto* bytes:{&s.draw,&s.subTile,&s.randomByte,&s.gF05160,&s.gEAC3F0,&s.roadDistance,&s.fertility,&s.gBEBF3A,&s.gBF8A4A})bytes->assign(n,0);
    s.randomByte.assign(n,8);s.roadDistance.assign(n,255);return s;
}
void authored(){
    auto s=state();const size_t cell=114*228+114;
    std::vector<uint8_t> material(s.cells,0);std::vector<uint16_t> owner(s.cells,0);
    s.image[cell]=(2u<<14);s.draw[cell]=9;s.subTile[cell]=1;
    auto seeded=selectGroundMapSeeded(s,112,material,owner);
    need(seeded.cells.size()==s.cells&&seeded.surface.image[cell]==s.image[cell]&&!seeded.cells[cell].selection.known,"pre-ground building occupies image and suppresses plain dispatch");
    need(seeded.surface.draw[cell]==9&&seeded.surface.subTile[cell]==1&&!seeded.drawWritten[cell],"skipped building metadata is retained, not independently predicted");
    auto map=selectGroundMap(s,112);
    need(map[cell].selection.known&&map[cell].selection.code==0x603,"MAP default still clears stored occupancy");
    s.image[cell]=0;s.draw[cell]=0x13;s.subTile[cell]=0xA5;
    seeded=selectGroundMapSeeded(s,112,material,owner);
    need(seeded.surface.draw[cell]==0x53&&seeded.surface.subTile[cell]==0xA0&&seeded.drawWritten[cell]&&seeded.subTileWritten[cell],"single plain stamp predicts draw marker and subTile clearing");
    s.terrain[cell]=0x40;s.subTile[cell]=0;
    for(auto sample:{std::pair<uint8_t,const char*>{0,"0x40-road"},{255,"0x40-road"},{128,"0x40-road"},{1,"0x40-road-material-0"},{25,"0x40-road-material-0"},{26,"0x40-road-material-2"},{127,"0x40-road-material-2"}}){
        material[cell]=sample.first;seeded=selectGroundMapSeeded(s,112,material,owner);
        need(std::string(seeded.cells[cell].selection.branch)==sample.second,"signed persisted material boundaries select separate road arms");
    }
    material[cell]=0;owner[cell]=1;seeded=selectGroundMapSeeded(s,112,material,owner);
    need(seeded.cells[cell].selection.chainNoWrite&&!seeded.cells[cell].selection.known,"saved nonzero owner refuses road writer");
    material[cell]=25;seeded=selectGroundMapSeeded(s,112,material,owner);
    need(seeded.cells[cell].selection.chainNoWrite&&std::string(seeded.cells[cell].selection.branch)=="saved-owner-road-declined","owner gate also precedes unresolved positive-material table");owner[cell]=0;
    s.terrain[cell]=0x2000000;seeded=selectGroundMapSeeded(s,112,material,owner);
    need(!seeded.cells[cell].selection.known&&std::string(seeded.cells[cell].selection.branch)=="0x2000000-feature-layout","unstamped feature composite stays explicitly unknown");
    s.terrain.assign(s.cells,2);s.randomByte.assign(s.cells,0);s.draw.assign(s.cells,0);s.subTile.assign(s.cells,0);
    seeded=selectGroundMapSeeded(s,112,material,owner);
    bool composite=false;
    for(size_t i=0;i<s.cells;++i)if(seeded.cells[i].stampSide==3){const auto origin=seeded.cells[i].origin;const size_t dy=(i-origin)/228,dx=(i-origin)%228;
        need(seeded.surface.subTile[i]==2&&seeded.surface.draw[i]==uint8_t(8*dy+dx+(dx==0&&dy==2?64:0)),"3x3 constant stamp independently predicts offsets and lower-left draw corner");composite=true;break;}
    need(composite,"composite side-effect check actually exercised a stamp");
    need(selectGroundMapSeeded(s,112,{},owner).cells.empty(),"missing material grid refuses instead of assuming zero");
}
struct Agreement {
    size_t cells=0,draw=0,side=0,both=0,drawWritten=0,sideWritten=0,drawPredicted=0,sidePredicted=0,markerPredicted=0,drawAssigned=0,drawAssignedEqual=0;
    void add(size_t cell,const OriginalBuildingScene& scene,const OriginalSavedGround& saved){
        ++cells;const bool d=scene.surface.draw[cell]==saved.state.draw[cell];
        const bool s=(scene.surface.subTile[cell]&15)==(saved.state.subTile[cell]&15);
        draw+=d;side+=s;both+=d&&s;
        if(scene.drawWritten[cell]){++drawWritten;drawPredicted+=d;markerPredicted+=(scene.surface.draw[cell]&64)==(saved.state.draw[cell]&64);}
        if(scene.drawWritten[cell]==0xFF){++drawAssigned;drawAssignedEqual+=d;}
        if(scene.subTileWritten[cell]){++sideWritten;sidePredicted+=s;}
    }
    void merge(const Agreement& v){cells+=v.cells;draw+=v.draw;side+=v.side;both+=v.both;drawWritten+=v.drawWritten;sideWritten+=v.sideWritten;drawPredicted+=v.drawPredicted;sidePredicted+=v.sidePredicted;markerPredicted+=v.markerPredicted;drawAssigned+=v.drawAssigned;drawAssignedEqual+=v.drawAssignedEqual;}
    void print()const{std::cout<<"\"cells\":"<<cells<<",\"draw_equal\":"<<draw<<",\"subtile_equal\":"<<side<<",\"both_equal\":"<<both<<",\"draw_written\":"<<drawWritten<<",\"draw_output_equal\":"<<drawPredicted<<",\"draw_marker_predicted_equal\":"<<markerPredicted<<",\"draw_fully_assigned\":"<<drawAssigned<<",\"draw_assigned_equal\":"<<drawAssignedEqual<<",\"subtile_written\":"<<sideWritten<<",\"subtile_predicted_equal\":"<<sidePredicted;}
};
void corpus(){
    const char* configured=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!configured||!*configured){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    const auto root=ResourceRoot::fromAbsolute(configured);need(root.configured(),"explicit root configured");
    const auto general=loadSG3(root.resolve("DATA/China_General.sg3")),ground=loadSG3(root.resolve("DATA/China_Terrain.sg3")),elevation=loadSG3(root.resolve("DATA/China_Elevation.sg3"));
    const auto catalog=enumerateOriginalMapSources(root);need(catalog.complete,"complete source catalog");
    size_t sources=0,withGround=0,active=0,emitted=0,decoded=0,unknown=0,noWrite=0,unresolved=0,buildings=0,buildingCells=0,unbound=0,native=0;
    std::map<std::string,Agreement> branches;Agreement total,storage;AppMapSurface app(root);
    for(const auto& source:catalog.sources){if(source.kind!=OriginalMapSourceKind::SaveFile)continue;++sources;
        const auto snapshot=loadOriginalSave(root.resolve(source.relativePath));
        need(snapshot.status==OriginalSaveStatus::LoadedSnapshot&&snapshot.readStable&&snapshot.layers.ground&&snapshot.layers.ground->valid(),"stable SAV19 has every rebuild input");
        const auto& saved=*snapshot.layers.ground;
        need(saved.waterMode==0&&saved.savedView==0&&saved.activeSide==snapshot.terrain.activeSide,"saved inactive water scalar,view0 and active crop support this reconstruction");
        std::ifstream file(root.resolve(source.relativePath),std::ios::binary);std::vector<uint8_t> input{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
        const auto container=decodeOriginalMapContainer(input);const auto& raw=container.payload;const size_t base=snapshot.terrainOffset;
        need(container.complete&&base>=155952&&raw.size()>=base+2226670,"independent complete SAV19 raw byte bounds");
        bool byteAgreement=true;
        for(size_t cell=0;cell<saved.state.cells;++cell)byteAgreement&=
            saved.state.draw[cell]==raw[base-155952+cell]&&saved.owner[cell]==detail::u16(raw.data()+base-103968+2*cell)&&
            saved.state.terrain[cell]==detail::u32(raw.data()+base+4*cell)&&saved.state.feature[cell]==detail::u32(raw.data()+base+2018734+4*cell)&&
            saved.state.subTile[cell]==raw[base+311904+cell]&&saved.state.randomByte[cell]==raw[base+415872+cell]&&
            saved.material[cell]==raw[base+467856+cell]&&saved.state.roadDistance[cell]==raw[base+1510985+cell]&&
            saved.state.fertility[cell]==raw[base+1822996+cell]&&saved.state.gBEBF3A[cell]==raw[base+1874984+cell];
        need(byteAgreement,"all ten ground input arrays independently equal literal SAV19 read spans");
        need(std::all_of(saved.state.image.begin(),saved.state.image.end(),[](auto word){return word==0;}),"SAV19 starts with cleared image grid");
        const auto scene=composeOriginalSavedGroundScene(snapshot.terrain,snapshot.buildings,saved,general,ground,elevation);
        need(scene.accepted&&scene.groundRebuilt&&scene.rejectedRecords==0,"original SAV seeded ground reconstruction accepts");
        Agreement local;size_t localActive=0;
        for(size_t cell=0;cell<saved.state.cells;++cell){storage.add(cell,scene,saved);if(snapshot.terrain.flags[cell]&0x80000u)continue;++localActive;
            local.add(cell,scene,saved);
            const std::string branch=scene.buildingOccupancy[cell]?(scene.buildingOccupancy[cell]==1?"building-bound":"building-unbound"):scene.groundSelections[cell].selection.branch;
            branches[branch].add(cell,scene,saved);
            if(scene.buildingOccupancy[cell])need(!scene.groundSelections[cell].selection.known,"ground never fills occupied building cells");
        }
        need(localActive==scene.groundCells+scene.groundUnknown+scene.groundNoWrite+scene.groundUnresolved+scene.placedCells+scene.unboundBuildingCells,"active cells partition into emitted/unknown/no-write/unresolved/bound/unbound");
        OriginalMapCamera camera;camera.width=600;camera.height=400;camera.outlines=false;camera.centerX=double(snapshot.terrain.activeSide-1)/2;camera.centerY=camera.centerX;
        need(app.selectSavedBuildings(snapshot.terrain,snapshot.buildings,&saved),"App adapter selects saved arrays and same-source bases");
        const auto view=app.renderSavedBuildings(camera);
        need(view.image.valid(),"native saved ground scene renders");
        std::cout<<"{\"render_source\":"<<sources<<",\"nonzero\":"<<view.bases.nonzeroCells<<",\"decoded\":"<<view.bases.decodedCells<<",\"geometry_unsupported\":"<<view.bases.geometryUnsupported<<",\"unresolved\":"<<view.bases.unresolvedIndices<<",\"archive_unavailable\":"<<view.bases.archiveUnavailable<<",\"decode_failures\":"<<view.bases.decodeFailures<<"}\n";
        for(const auto& m:view.bases.compositeMismatches)std::cout<<"{\"geometry_reason\":\""<<m.reason<<"\",\"cells\":"<<m.cells<<",\"tileset\":"<<m.tileset<<",\"record\":"<<m.recordId<<",\"side\":"<<m.cols<<",\"group\":"<<m.groupSide<<"}\n";
        need(view.bases.decodedCells==scene.groundCells+scene.placedCells&&view.bases.decodeFailures==0&&view.bases.geometryUnsupported==0&&view.bases.unresolvedIndices==0&&view.bases.archiveUnavailable==0,"every emitted ground/building word decodes including offscreen");
        const auto cg=createCGImageFromPremultipliedBGRA(view.image.width,view.image.height,view.image.pixelsBGRA);need(cg,"native saved ground CGImage");if(cg){++native;CGImageRelease(cg);}
        const size_t localDecoded=view.bases.decodedCells-scene.placedCells;
        std::cout<<"{\"saved_source_ordinal\":"<<sources<<",\"active_cells\":"<<localActive<<",\"ground_emitted\":"<<scene.groundCells<<",\"ground_decoded\":"<<localDecoded<<",\"ground_unknown\":"<<scene.groundUnknown<<",\"chain_no_write\":"<<scene.groundNoWrite<<",\"unresolved\":"<<scene.groundUnresolved<<",\"building_records\":"<<scene.placed.size()<<",\"building_cells\":"<<scene.placedCells<<",\"building_unbound_cells\":"<<scene.unboundBuildingCells<<",\"agreement\":{";local.print();std::cout<<"}}\n";
        ++withGround;active+=localActive;emitted+=scene.groundCells;decoded+=localDecoded;unknown+=scene.groundUnknown;noWrite+=scene.groundNoWrite;unresolved+=scene.groundUnresolved;buildings+=scene.placed.size();buildingCells+=scene.placedCells;unbound+=scene.unboundBuildingCells;total.merge(local);
        app.clear();need(!app.renderSavedBuildings(camera).image.valid(),"source replacement clears reconstructed scene");
    }
    need(sources==6&&withGround==6&&native==6&&active==57684&&buildings==927&&buildingCells==1483,"six-source/active-cell/building denominators unchanged");
    need(emitted==55790&&decoded==55790&&unknown==154&&noWrite==21&&unresolved==0&&unbound==236,"all active cells partition with named unknowns and no emitted decode failures");
    need(total.drawWritten==57273&&total.drawPredicted==57235&&total.markerPredicted==57249&&total.sideWritten==57273&&total.sidePredicted==57253,"independent side-effect agreement locks explicit written denominators and elevation residuals");
    std::cout<<"{\"scope\":\"all backing cells (includes retained off-map bytes)\",";storage.print();std::cout<<"}\n";
    for(const auto& entry:branches){std::cout<<"{\"branch\":\""<<entry.first<<"\",";entry.second.print();std::cout<<"}\n";}
    std::cout<<"{\"saved_sources\":"<<sources<<",\"sources_with_SG3_ground\":"<<withGround<<",\"active_cells\":"<<active<<",\"ground_emitted\":"<<emitted<<",\"ground_decoded\":"<<decoded<<",\"ground_unknown\":"<<unknown<<",\"chain_no_write\":"<<noWrite<<",\"unresolved\":"<<unresolved<<",\"buildings_placed\":"<<buildings<<",\"building_cells\":"<<buildingCells<<",\"building_unbound_cells\":"<<unbound<<",\"native_CGImages\":"<<native<<",\"agreement\":{";total.print();std::cout<<"},\"stored_image_oracle\":false}\n";
}
}
int main(){try{authored();const auto authoredChecks=checks;corpus();std::cout<<"{\"passed\":true,\"authored_checks\":"<<authoredChecks<<",\"all_checks\":"<<checks<<"}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
