#include "original_building_scene.hpp"
#include "original_pak.hpp"
#include "app/map_surface.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <unistd.h>
using namespace game_lab;
namespace {
size_t checks=0;
void need(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
void authored(){
    // Explicit independent frame-major truth tables: 0x5CFDF0 uses metadata
    // count at+20 as direction stride, not frames-per-direction.
    const uint32_t truth[3][8]={{201,202,203,204,205,206,207,208},{209,210,211,212,213,214,215,216},{217,218,219,220,221,222,223,224}};
    for(unsigned f=0;f<3;++f)for(unsigned d=0;d<8;++d)
        need(originalFigureFrameIndex(201,3,8,f,d)==truth[f][d],"frame-major direction-interleaved index truth table");
    need(!originalFigureFrameIndex(201,3,8,3,0)&&!originalFigureFrameIndex(201,3,8,0,8)&&
        !originalFigureFrameIndex(UINT32_MAX,3,8,0,0),"index domain/overflow refusals");
    OriginalFigure f;f.type=11;f.progress=0;
    const int expected[8][2]={{2,-1},{4,0},{2,1},{0,2},{-2,1},{-4,0},{-2,-1},{0,-2}};
    for(unsigned d=0;d<8;++d){f.direction=d;
        need(originalFigureSubcell(f)==std::make_pair(expected[d][0],expected[d][1]),"eight direction progress0 position truth table");
        f.progress=9;need(originalFigureSubcell(f)==std::make_pair(10*expected[d][0],10*expected[d][1]),"last positive step");
        f.progress=10;need(originalFigureSubcell(f)==std::make_pair(-9*expected[d][0],-9*expected[d][1]),"cell transition wraps signed subcell");
        f.progress=20;need(originalFigureSubcell(f)==std::make_pair(0,0),"stationary progress20");f.progress=0;
    }
    f.direction=8;f.previousDirection=6;need(originalFigureSubcell(f)==std::make_pair(-2,-1),"stationary direction uses previous direction");
    f.progress=21;need(!originalFigureSubcell(f),"table bounds refusal");
    f.freeMovement=1;f.subX=47;f.subY=62;f.raise=3;
    need(originalFigureSubcell(f)==std::make_pair(10,6),"free movement modulo20 projected from independent (7,2) example");
    f.subX=-1;need(!originalFigureSubcell(f),"negative modulo table address refused");
    // Literal source wire offsets, decoy values in adjacent enum bytes.
    std::vector<uint8_t> bytes{1,0,1,0,0,0,255,255,0,0,6,0,'F','i','g','u','r','e',7,0};
    std::array<uint8_t,408> raw{};raw[18]=11;raw[22]=1;raw[5]=7;raw[25]=3;raw[26]=6;raw[64]=17;raw[65]=9;
    raw[8]=0x34;raw[9]=0x12;raw[28]=5;raw[30]=6;
    for(const auto& field:emperor::file_io::research::figure7_fields)
        for(size_t k=0;k<field.bytes;++k)bytes.push_back(std::string(field.helper)=="0x503db0"&&k==1?0xee:raw[field.receiver_offset+k]);
    bytes.push_back(0);
    auto parsed=parseOriginalSavedFigures(bytes,0,bytes.size(),17,0);
    need(parsed.status==OriginalFigureStatus::Complete&&parsed.parsed==1&&parsed.end==bytes.size()&&parsed.live.size()==1,"complete bounded Figure7 wire reuse");
    const auto& p=parsed.live[0];need(p.type==11&&p.frame==7&&p.action==17&&p.progress==9&&p.direction==3&&p.previousDirection==6&&p.image==0x1234&&p.x==5&&p.y==6,"field identities and enum storage width");
    auto bad=bytes;bad.back()=1;need(parseOriginalSavedFigures(bad,0,bad.size(),17,0).status==OriginalFigureStatus::Unsupported,"FSA does not guess a variable extension length");
    bad=bytes;bad.pop_back();need(parseOriginalSavedFigures(bad,0,bad.size(),17,0).status==OriginalFigureStatus::Malformed,"last byte truncation publishes no figures");
    bad=bytes;bad[18]=6;need(parseOriginalSavedFigures(bad,0,bad.size(),17,0).status==OriginalFigureStatus::Unsupported,"unknown Figure version fails closed");
    {
        auto multiple=bytes;multiple[2]=3;multiple.insert(multiple.end(),{17,128});multiple.insert(multiple.end(),bytes.begin()+18,bytes.end());multiple.insert(multiple.end(),{18,0});
        const auto read=parseOriginalSavedFigures(multiple,0,multiple.size(),17,0);
        need(read.status==OriginalFigureStatus::Complete&&read.live.size()==3&&read.live[2].slot==2&&read.end==multiple.size(),"carried archive class17, object18 and tag-only reference preserve list slots");
        auto extended=multiple;extended.erase(extended.begin()+bytes.size(),extended.begin()+bytes.size()+2);
        extended.insert(extended.begin()+bytes.size(),{255,127,17,0,0,128});
        need(parseOriginalSavedFigures(extended,0,extended.size(),17,0).status==OriginalFigureStatus::Complete,"extended class-reference tag preserves archive identity");
        auto invalidExtended=bytes;invalidExtended[2]=2;invalidExtended.insert(invalidExtended.end(),{255,127,255,255,0,0});
        need(parseOriginalSavedFigures(invalidExtended,0,invalidExtended.size(),17,0).status==OriginalFigureStatus::Malformed,"extended object65535 cannot become a class declaration");
        multiple[bytes.size()]=16;
        need(parseOriginalSavedFigures(multiple,0,multiple.size(),17,0).status==OriginalFigureStatus::Malformed,"class references cannot resolve to opaque preceding archive entries");
        auto pheasant=bytes;pheasant[10]=12;pheasant.erase(pheasant.begin()+12,pheasant.begin()+18);
        const std::string name="cPheasantFig";pheasant[10]=name.size();pheasant.insert(pheasant.begin()+12,name.begin(),name.end());
        pheasant.insert(pheasant.begin()+12+name.size(),{2,0});
        need(parseOriginalSavedFigures(pheasant,0,pheasant.size(),17,0).status==OriginalFigureStatus::Complete,"Pheasant own2 precedes Figure7");
        auto emissary=pheasant;const std::string other="cEmissaryDude";
        emissary.erase(emissary.begin()+12,emissary.begin()+12+name.size());emissary.insert(emissary.begin()+12,other.begin(),other.end());emissary[10]=other.size();
        emissary.insert(emissary.end(),{3,0,0,0,'a','b',0});
        const auto readEmissary=parseOriginalSavedFigures(emissary,0,emissary.size(),17,0);
        need(readEmissary.status==OriginalFigureStatus::Complete&&readEmissary.end==emissary.size(),"Emissary own2 consumes bounded trailing string bytes");
        emissary.pop_back();need(parseOriginalSavedFigures(emissary,0,emissary.size(),17,0).status==OriginalFigureStatus::Malformed,"Emissary variable tail truncation publishes no prefix");
    }
    OriginalSceneDepth base{5,2,0,0},object1{5,2,1,0},animation{5,2,2,0},figure{5,2,3,0},object2{5,2,4,0},next{5,3,0,0};
    need(base<object1&&object1<animation&&animation<figure&&figure<object2&&object2<next,"city cell phase order: surface, mode1, animation, figure, mode2, next cell");
    // Opaque authored rectangles make selected record, signed offsets, scale and
    // occlusion observable as pixels rather than implementation-mirroring keys.
    const auto work=std::filesystem::temp_directory_path()/("emperor-figure-pixels-"+std::to_string(getpid()));
    std::filesystem::create_directories(work);const auto file=work/"authored.555";
    std::vector<uint8_t> payload;SG3Archive archive;archive.version=214;archive.numImages=2;archive.sprites.resize(2);
    archive.imageIndex[0]=1;archive.bitmaps.push_back({"AuthoredFigures"});
    for(unsigned id=0;id<2;++id){auto& s=archive.sprites[id];s.width=30;s.height=40;s.type=256;s.offset=payload.size();s.animationOffsetX=10;s.animationOffsetY=45;
        for(unsigned y=0;y<40;++y){payload.push_back(30);for(unsigned x=0;x<30;++x){payload.push_back(id?0x1f:0);payload.push_back(id?0:0x7c);}}
        s.length=payload.size()-s.offset;
    }
    {std::ofstream output(file,std::ios::binary);output.write(reinterpret_cast<const char*>(payload.data()),payload.size());need(bool(output),"authored pixels written in temp only");}
    OriginalMapTerrain terrain;terrain.activeSide=84;terrain.flags.assign(51984,0x80000);
    for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)terrain.flags[y*228+x]=0;
    OriginalBuildingProfile buildings;buildings.status=OriginalBuildingStatus::Complete;
    auto scene=composeOriginalBuildingScene(terrain,buildings,archive);
    OriginalFigure one;one.type=11;one.alive=1;one.image=38u<<14;one.x=0;one.y=0;one.cell=72*229;one.progress=20;one.slot=4;
    OriginalFigureProfile figures;figures.status=OriginalFigureStatus::Complete;figures.live={one};
    figures.cellHeads.assign(51984,0);figures.cellHeads[one.cell]=one.slot;
    const std::map<uint32_t,const SG3Archive*> refs{{38,&archive}};
    installOriginalFigureOverlays(scene,terrain,figures,refs);
    need(scene.figurePlaced==1&&scene.overlays.size()==1&&scene.overlays[0].physicalId==1&&scene.overlays[0].offsetX==31&&scene.overlays[0].offsetY==-15,"v214 no-system-prefix physical1 and selected sprite anchor");
    installOriginalFigureOverlays(scene,terrain,figures,refs);
    need(scene.figurePlaced==1&&scene.overlays.size()==1,"reinstallation replaces figures without duplicate commands or counts");
    OriginalMapCamera camera;camera.width=200;camera.height=200;camera.centerX=0;camera.centerY=0;camera.outlines=false;
    const std::vector<MapImageGridTileset> tilesets{{38,&archive,file.string(),1}};
    for(unsigned scale:{1u,2u}){camera.scale=scale;const auto view=renderOriginalBuildingScene(terrain,scene,archive,file.string(),camera,-1,tilesets);
        const unsigned left=100-8*scale,top=100-35*scale;
        need(view.image.valid()&&view.figureDecoded==1&&view.overlayDrawn==1&&view.overlayInvalid==0&&view.image.pixelsBGRA[top*200+left]==0xffff0000u,"figure blits selected saved image at exact signed scaled position");
        need(view.image.pixelsBGRA[top*200+left-1]!=0xffff0000u,"sprite left edge is exact");
    }
    camera.scale=1;auto ordered=scene;
    ordered.overlays.push_back({"DATA/China_General.sg3",2,2,0,0,31,-15,{0,0,2,0},0,3,1});
    auto rendered=renderOriginalBuildingScene(terrain,ordered,archive,file.string(),camera,-1,tilesets);
    need(rendered.image.pixelsBGRA[65*200+92]==0xffff0000u,"figure pixel follows same-cell building animation");
    ordered.overlays.back().mode=2;ordered.overlays.back().depth.phase=4;
    need(renderOriginalBuildingScene(terrain,ordered,archive,file.string(),camera,-1,tilesets).image.pixelsBGRA[65*200+92]==0xff0000ffu,"same-cell mode2 pixel follows figure");
    {
        auto sorted=figures;auto front=one;front.slot=3;front.image=(38u<<14)|1;front.progress=0;front.direction=3;front.height=2;front.next=one.slot;
        sorted.cellHeads[one.cell]=3;sorted.live.push_back(front);
        auto composed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(composed,terrain,sorted,refs);
        const auto view=renderOriginalBuildingScene(terrain,composed,archive,file.string(),camera,-1,tilesets);
        need(view.figureDecoded==2&&view.image.pixelsBGRA[65*200+92]==0xff0000ffu,"larger subcell score draws last despite smaller slot and height cancelling pixel displacement");
        sorted.live.back().progress=20;sorted.live.back().height=0;composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,sorted,refs);
        need(renderOriginalBuildingScene(terrain,composed,archive,file.string(),camera,-1,tilesets).image.pixelsBGRA[65*200+92]==0xffff0000u,"equal subcell score breaks ties by source slot");
    }
    {
        auto pair=figures;pair.live[0].type=20;pair.live[0].secondaryImage=(38u<<14)|1;
        auto composed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(composed,terrain,pair,refs);
        need(composed.figurePlaced==1&&composed.overlays.size()==2&&composed.overlays[0].physicalId==1&&composed.overlays[1].physicalId==2,
            "two-image action keeps primary then secondary");
        const auto view=renderOriginalBuildingScene(terrain,composed,archive,file.string(),camera,-1,tilesets);
        need(view.figureDecoded==2&&view.image.pixelsBGRA[65*200+92]==0xff0000ffu,"type20 secondary overlaps primary at independently equal anchor");
        pair.live[0].extraX=7;pair.live[0].extraY=-4;composed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(composed,terrain,pair,refs);
        need(composed.overlays[1].offsetX==38&&composed.overlays[1].offsetY==-19,"saved secondary extra offset participates");
    }
    {
        auto missing=figures;missing.cellHeads[one.cell]=0;auto composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,missing,refs);need(composed.figurePlaced==0&&composed.figureUnsupported==1,"unlinked live records cannot bypass cell scan");
        auto cycle=figures;cycle.live[0].next=one.slot;composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,cycle,refs);need(composed.figurePlaced==0&&composed.figureUnsupported==1,"cycle after requested slot cannot masquerade as a valid cell list");
        auto suppressed=figures;suppressed.live[0].hidden=1;composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,suppressed,refs);need(composed.figurePlaced==0&&composed.figureHidden==1,"saved hidden gate suppresses image");
        auto unset=figures;unset.live[0].image=0;composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,unset,refs);need(composed.figurePlaced==0&&composed.figureZeroImage==1,"image zero is not an invented initialization frame");
        auto duplicate=figures;duplicate.live.push_back(one);composed=composeOriginalBuildingScene(terrain,buildings,archive);
        installOriginalFigureOverlays(composed,terrain,duplicate,refs);need(composed.figurePlaced==0&&composed.figureUnsupported==2,"duplicate source slots fail before placement");
    }
    auto elapsed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(elapsed,terrain,figures,refs,1);
    need(elapsed.figurePlaced==0&&elapsed.figureUnsupported==1,"nonzero caller tick refuses unproved simulation phase");
    for(unsigned type:{11u,13u,19u,20u,21u,22u,23u,26u,28u,29u,30u,35u,36u,39u,40u,45u,47u,76u}){
        for(unsigned d=0;d<8;++d){auto p=figures;p.live[0].type=type;p.live[0].direction=d;p.live[0].progress=0;
            auto composed=composeOriginalBuildingScene(terrain,buildings,archive);installOriginalFigureOverlays(composed,terrain,p,refs);
            need(composed.figurePlaced==1&&composed.overlays[0].physicalId==1,"all bound types/directions retain saved image at tick0");
        }
    }
    std::filesystem::remove(file);std::filesystem::remove(work);
}
void corpus(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    const auto root=ResourceRoot::fromAbsolute(env);const auto catalog=enumerateOriginalMapSources(root);need(catalog.complete,"complete source catalog");
    AppMapSurface app(root);size_t sources=0,slots=0,live=0,placed=0,hidden=0,zero=0,unsupported=0,decoded=0,failures=0;
    std::set<unsigned> types,boundTypes;std::map<unsigned,size_t> blocked;size_t focused=0,placedDecoded=0;
    for(const auto& source:catalog.sources){if(source.kind!=OriginalMapSourceKind::SaveFile)continue;
        const auto save=loadOriginalSave(root.resolve(source.relativePath));need(save.readStable&&save.layers.figures.has_value(),"same-source stable figure profile");
        const auto& figures=*save.layers.figures;
        need(figures.status==OriginalFigureStatus::Complete&&figures.parsed==figures.declared,"complete serialized figure list");
        ++sources;slots+=figures.declared;live+=figures.live.size();for(const auto& t:figures.liveTypes)types.insert(t.first);
        need(app.selectSavedBuildings(save.terrain,save.buildings,save.layers.ground?&*save.layers.ground:nullptr,0,&figures),"App installs saved figures");
        OriginalMapCamera camera;camera.width=800;camera.height=800;camera.centerX=save.terrain.activeSide/2;camera.centerY=camera.centerX;camera.outlines=false;
        const auto view=app.renderSavedBuildings(camera);need(view.image.valid()&&view.overlayInvalid==0&&view.overlayDecodeFailures==0,"original figure images decode with generic overlay layer");
        placed+=view.figurePlaced;hidden+=view.figureHidden;zero+=view.figureZeroImage;unsupported+=view.figureUnsupported;decoded+=view.figureDecoded;failures+=view.overlayDecodeFailures;
        need(view.figureLive==figures.live.size()&&view.figureLive==view.figurePlaced+view.figureHidden+view.figureZeroImage+view.figureUnsupported,"all live figure outcomes accounted");
        // Recompose explicitly for type and blockage denominators, independent of App cache.
        auto scene=composeOriginalBuildingScene(save.terrain,save.buildings,loadSG3(root.resolve("DATA/China_General.sg3")));
        std::map<uint32_t,SG3Archive> archives;std::map<uint32_t,const SG3Archive*> refs;
        for(unsigned slot:{14u,38u,39u,40u}){archives.emplace(slot,loadSG3(root.resolve(std::string("DATA/")+originalFigureArchiveName(slot))));refs[slot]=&archives.at(slot);}
        installOriginalFigureOverlays(scene,save.terrain,figures,refs);
        size_t selectedImages=0;
        for(const auto& overlay:scene.overlays)if(overlay.mode==4){
            ++selectedImages;const auto& sourceFigure=*std::find_if(figures.live.begin(),figures.live.end(),[&](const auto& f){return f.slot==overlay.sourceSlot;});
            const uint32_t selected=overlay.physicalId-1+(overlay.tileset<<14);
            need(selected==sourceFigure.image||selected==sourceFigure.secondaryImage,"independent inverse v214 mapping equals saved image word");
        }
        need(selectedImages==view.figureDecoded,"every installed figure component decodes, including offscreen");
        for(const auto& t:scene.figureBoundTypes)boundTypes.insert(t.first);
        for(const auto& t:scene.figureBlockedTypes)blocked[t.first]+=t.second;
        need(scene.figurePlaced==view.figurePlaced,"direct binder and App agree");
        std::set<size_t> focusSlots;
        for(const auto& overlay:scene.overlays)if(overlay.mode==4)focusSlots.insert(overlay.sourceSlot);
        for(const auto slot:focusSlots){
            const auto& f=*std::find_if(figures.live.begin(),figures.live.end(),[&](const auto& item){return item.slot==slot;});
            auto isolated=scene;isolated.overlays.erase(std::remove_if(isolated.overlays.begin(),isolated.overlays.end(),[&](const auto& item){return item.mode!=4||item.sourceSlot!=slot;}),isolated.overlays.end());
            camera.centerX=f.x;camera.centerY=f.y;OriginalBuildingSceneView oneView;DecodedImage canvas;
            canvas.width=800;canvas.height=800;canvas.pixelsBGRA.assign(800*800,0);
            std::vector<MapImageGridTileset> resources{{2,nullptr,"",201}};
            // Only the isolated figure draws; omit the background surface so
            // painted pixels establish that this particular occurrence blits.
            isolated.surface.image.assign(51984,0);
            for(const auto& entry:archives){auto path=std::filesystem::path(std::string("DATA/")+originalFigureArchiveName(entry.first));path.replace_extension(".555");resources.push_back({entry.first,&entry.second,root.resolve(path.string()),1});}
            need(renderOriginalSceneOverlays(save.terrain,isolated,resources,camera,oneView,canvas)&&oneView.figureDecoded==isolated.overlays.size()&&
                oneView.overlayDrawn==isolated.overlays.size()&&oneView.overlayInvalid==0&&oneView.overlayDecodeFailures==0&&
                std::any_of(canvas.pixelsBGRA.begin(),canvas.pixelsBGRA.end(),[](auto pixel){return pixel>>24;}),"each original figure itself paints a focused generic compositor view");
            ++focused;++placedDecoded;
        }
        std::cout<<"{\"figure_source\":"<<sources<<",\"slots\":"<<figures.declared<<",\"live\":"<<figures.live.size()<<",\"placed\":"<<view.figurePlaced<<",\"hidden\":"<<view.figureHidden<<",\"zero_image\":"<<view.figureZeroImage<<",\"unsupported\":"<<view.figureUnsupported<<",\"decoded_images\":"<<view.figureDecoded<<"}\n";
    }
    need(sources==6&&slots==12000&&live==192&&types.size()==20&&boundTypes.size()==18&&placed==158&&hidden==6&&zero==28&&unsupported==0&&decoded==174&&failures==0&&focused==158&&placedDecoded==158,"six-source original denominator");
    std::cout<<"{\"sources\":"<<sources<<",\"slots\":"<<slots<<",\"live_figures\":"<<live<<",\"bound_types\":"<<boundTypes.size()<<",\"live_types\":"<<types.size()<<",\"figures_placed\":"<<placed<<",\"figures_placed_and_decoded\":"<<placedDecoded<<",\"focused_figure_blits\":"<<focused<<",\"hidden\":"<<hidden<<",\"zero_image\":"<<zero<<",\"unsupported\":"<<unsupported<<",\"decoded_images\":"<<decoded<<",\"decode_failures\":"<<failures<<",\"tick\":0,\"blocked_types\":{";
    bool first=true;for(const auto& t:blocked){if(!first)std::cout<<',';first=false;std::cout<<'"'<<t.first<<"\":"<<t.second;}std::cout<<"}}\n";
}
}
int main(){try{authored();const size_t synthetic=checks;corpus();std::cout<<"{\"passed\":true,\"authored_checks\":"<<synthetic<<",\"all_checks\":"<<checks<<"}\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
