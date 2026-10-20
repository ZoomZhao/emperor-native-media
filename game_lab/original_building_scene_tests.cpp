#include "app/map_surface.hpp"
#include "original_pak.hpp"
#include "cg_image_conversion.hpp"
#include "asset_bytes.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace {
using namespace game_lab;
size_t checks=0;
void need(bool b,const char* message){++checks;if(!b)throw std::runtime_error(message);}
OriginalMapTerrain terrain(){OriginalMapTerrain t;t.activeSide=84;t.flags.assign(51984,0x80000);
    for(unsigned y=72;y<156;++y)for(unsigned x=72;x<156;++x)t.flags[y*228+x]=8;
    return t;
}
OriginalBuildingFootprint footprint(uint16_t type,uint8_t size,uint16_t x,uint16_t y,size_t slot){
    OriginalBuildingFootprint f;f.type=type;f.size=size;f.x=x;f.y=y;f.slot=slot;
    if(type==72)f.wellImageInputs=OriginalWellImageInputs{};return f;
}
OriginalBuildingProfile profile(std::vector<OriginalBuildingFootprint> f){OriginalBuildingProfile p;
    p.status=OriginalBuildingStatus::Complete;p.liveSizedRecords=f.size();p.footprints=std::move(f);return p;
}
SG3Archive archive(const std::filesystem::path& file){
    SG3Archive a;a.version=213;a.numImages=209;a.sprites.resize(a.numImages);a.bitmaps.resize(3);
    a.bitmaps[0].filename="China_Safety.bmp";a.bitmaps[1].filename="China_StorNDist.bmp";a.bitmaps[2].filename="China_Government2.bmp";
    std::vector<uint8_t> bytes;
    const auto add=[&](unsigned id,unsigned side,uint16_t color,unsigned bitmap,unsigned topRows){
        auto& s=a.sprites[id-1];s.width=int16_t(side*80-2);s.height=int16_t(side*40+topRows);
        s.type=30;s.tileSize=uint8_t(side);s.bitmapId=uint8_t(bitmap);s.uncompressedLength=side*side*3200;s.offset=uint32_t(bytes.size());
        for(unsigned i=0;i<s.uncompressedLength/2;++i){bytes.push_back(uint8_t(color));bytes.push_back(uint8_t(color>>8));}
        // Authored opaque rectangular top makes ordering visible even between
        // disjoint footprints; it is deliberately not an original asset.
        for(unsigned y=0;y<topRows;++y){unsigned left=unsigned(s.width);while(left){const unsigned count=std::min(254u,left);bytes.push_back(uint8_t(count));for(unsigned x=0;x<count;++x){bytes.push_back(uint8_t(color));bytes.push_back(uint8_t(color>>8));}left-=count;}}
        s.length=uint32_t(bytes.size())-s.offset;
    };
    add(201,2,0x03e0,0,20);add(202,5,0x7c00,1,120);add(203,1,0x001f,2,80);add(204,2,0x001f,0,120);
    for(unsigned id=205;id<=208;++id)add(id,1,0x03e0,1,0);add(209,2,0x001f,1,0);
    a.imageIndex[0x82]=201;a.imageIndex[0x83]=204;a.imageIndex[0x23]=202;a.imageIndex[0x92]=203;
    a.imageIndex[0x22]=205;a.imageIndex[0x2a]=209;
    std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    need(bool(out),"authored companion written outside repository");return a;
}
void authored(const std::filesystem::path& temp){
    auto t=terrain();const auto a=archive(temp/"own.555");
    for(unsigned side=1;side<=6;++side){auto f=footprint(72,uint8_t(side),3,4,0);
        const std::pair<uint32_t,uint32_t> expected[]={{3,4+side-1},{3,4},{3+side-1,4},{3+side-1,4+side-1}};
        for(unsigned v=0;v<4;++v)need(originalBuildingDrawCorner(f,2*v)==expected[v],"four exe view corners independently enumerated");
    }
    auto f=footprint(72,2,3,4,0);need(!originalBuildingDrawCorner(f,1)&&!originalBuildingDrawCorner(f,8),"unknown view codes refuse");
    auto p=profile({f,footprint(54,3,9,9,1)});p.liveSizedRecords=3;p.unplacedMarkers={{71,15,15,2}};
    const auto original=t.flags;auto scene=composeOriginalBuildingScene(t,p,a);
    need(scene.accepted&&scene.liveRecords==3&&scene.bindableRecords==1&&scene.placed.size()==1&&scene.placedCells==4,"only bound record stamped; live denominator retained");
    need(scene.withheldFootprints.size()==1&&scene.markers.size()==1&&t.flags==original,"withheld footprint and unknown-shape marker retained without source changes");
    const size_t origin=72;
    for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x){const size_t cell=(origin+4+y)*228+origin+3+x;
        need(scene.surface.image[cell]==(2u<<14)&&scene.surface.subTile[cell]==1&&scene.surface.draw[cell]==uint8_t(x+8*y+(x==0&&y==1?64:0)),"same image word,group side,column/row,and unique view0 corner stamped");
    }
    need(std::count_if(scene.surface.image.begin(),scene.surface.image.end(),[](auto n){return n;})==4,"no ground field renamed or unintended cells stamped");
    OriginalMapCamera c;c.width=600;c.height=500;c.centerX=3;c.centerY=4;c.outlines=false;
    auto overwritten=scene.surface;overwritten.image[(origin+5)*228+origin+3]=(2u<<14)|2u;
    auto unsafe=renderMapImageGridViewport(t,overwritten,{{2,&a,(temp/"own.555").string(),201}},c,MapImageGridOrder::View0DrawCorner);
    need(unsafe.image.valid()&&unsafe.stats.geometryUnsupported==3&&unsafe.stats.decodedCells==1,"overwritten draw corner refuses old base and preserves the newer cell accounting");
    for(unsigned scale:{1u,2u}){c.scale=scale;auto view=renderOriginalBuildingScene(t,scene,a,(temp/"own.555").string(),c);
        need(view.image.valid()&&view.bases.decodedCells==4&&view.bases.drawnBlocks==1&&view.bases.decodeFailures==0,"bound base decodes once through existing composite renderer");
        // Origin centre=(300,250); side2,height100 -> left221,top210 at1x.
        // The opaque top's first pixel is a direct anchor check, independent of
        // the compositor's formula or its own returned rectangle.
        const unsigned left=scale==1?221:142,top=scale==1?210:170;
        need(view.image.pixelsBGRA[top*c.width+left]==0xff00ff00u,"exe composite top-left differs from manual canvas-bottom-centre placement");
        need(view.placedRecords==1&&view.withheldRecords==2&&view.withheldOutlines==1&&view.unshapedMarkers==1,"view publishes explicit placed/withheld denominators");
    }
    c.scale=1;
    auto bad=renderOriginalBuildingScene(t,scene,a,(temp/"missing.555").string(),c);
    need(bad.image.valid()&&bad.bases.decodeFailures==4&&bad.bases.decodedCells==0,"failed payload never counted decoded or silently successful");
    c.centerX=83;c.centerY=83;auto culled=renderOriginalBuildingScene(t,scene,a,(temp/"own.555").string(),c);
    need(culled.bases.decodedCells==4&&culled.bases.culled==4,"offscreen payloads still validated");
    c.centerX=3;c.centerY=4;c.scale=0;need(!renderOriginalBuildingScene(t,scene,a,(temp/"own.555").string(),c).image.valid(),"invalid camera refuses whole view");c.scale=1;
    auto overlap=profile({f,footprint(72,2,4,4,1)});auto refused=composeOriginalBuildingScene(t,overlap,a);
    need(refused.accepted&&refused.placed.empty()&&refused.rejectedRecords==2&&refused.withheldFootprints.size()==2,"both overlapping records withheld without rebuild-order guess");
    auto malformed=p;malformed.status=OriginalBuildingStatus::UnsupportedTail;need(!composeOriginalBuildingScene(t,malformed,a).accepted,"partial saved table never presented as complete scene");
    // Mixed-size disjoint footprints: size5 origin depth5,draw-corner depth9;
    // size1 origin/corner depth8. The size5 red top must be painted LAST.
    auto order=composeOriginalBuildingScene(t,profile({footprint(53,5,2,3,0),footprint(126,1,1,7,1)}),a);
    c.centerX=4;c.centerY=4;
    auto v=renderOriginalBuildingScene(t,order,a,(temp/"own.555").string(),c);
    need(v.image.valid()&&v.bases.decodedCells==26&&v.bases.drawnBlocks==1,"mixed-size scene accepts all26 cells");
    need(v.image.pixelsBGRA[160*c.width+80]==0xffff0000u,"view0 diagonal draw-corner order paints later large building over earlier small top");
    std::reverse(order.placed.begin(),order.placed.end());auto reordered=renderOriginalBuildingScene(t,order,a,(temp/"own.555").string(),c);
    need(reordered.image.pixelsBGRA==v.image.pixelsBGRA,"record enumeration does not change painter order");
    // Equal-depth disjoint base rectangles meet at a seam rather than overlap:
    // left green draw(3,5), right blue draw(5,3).
    auto right=footprint(72,2,5,2,1);right.wellImageInputs->byte6f=1;
    auto ties=composeOriginalBuildingScene(t,profile({f,right}),a);
    auto tied=renderOriginalBuildingScene(t,ties,a,(temp/"own.555").string(),c);
    need(tied.image.pixelsBGRA[200*c.width+200]==0xff00ff00u&&tied.image.pixelsBGRA[200*c.width+360]==0xff0000ffu,"equal-depth left/right bases keep their independent screen placement");
    for(uint8_t orientation:{uint8_t(0),uint8_t(1)}){
        auto market=footprint(59,1,20,20,10);market.width=orientation?7:4;market.height=orientation?4:7;market.imageInputs=OriginalBuildingImageInputs{};
        market.imageInputs->marketOrientation=orientation;market.imageInputs->savedView=0;
        market.imageInputs->marketRandom.resize(28);market.imageInputs->marketDraw.assign(28,0x15);
        for(unsigned i=0;i<28;++i)market.imageInputs->marketRandom[i]=uint8_t(i%4);
        auto marketProfile=profile({market});
        for(unsigned j=0;j<4;++j){auto stall=footprint(62,2,uint16_t(20+(orientation?(j/2)*5:(j%2)*2)),uint16_t(20+(orientation?(j%2)*2:(j/2)*5)),11+j);
            stall.imageInputs=OriginalBuildingImageInputs{};stall.imageInputs->randomByte=0;
            marketProfile.footprints[0].imageInputs->marketStalls[j]=uint32_t(stall.slot);marketProfile.footprints.push_back(stall);
        }
        marketProfile.liveSizedRecords=5;const auto composite=composeOriginalBuildingScene(t,marketProfile,a);
        need(composite.accepted&&composite.placed.size()==5&&composite.placedCells==28&&composite.rejectedRecords==0,"both market orientations share only their four exact referenced stall footprints");
        for(unsigned y=2;y<=4;++y)for(unsigned x=0;x<4;++x){const size_t cell=(origin+20+(orientation?x:y))*228+origin+20+(orientation?y:x);
            need(composite.surface.image[cell]==((2u<<14)|uint32_t(4+x))&&composite.surface.draw[cell]==0x55&&composite.surface.subTile[cell]==0,"market descriptor transpose uses per-cell random and preserves nonzero low draw bits");
        }
        auto missing=marketProfile;missing.footprints.pop_back();const auto refusedMarket=composeOriginalBuildingScene(t,missing,a);
        need(refusedMarket.placed.size()==3&&refusedMarket.withheldFootprints.size()==1,"market refuses an absent referenced stall without rejecting independent remaining records");
    }
}
void corpus(){
    const char* env=std::getenv("EMPEROR_GAMEDATA_ROOT");if(!env||!*env){std::cout<<"{\"skipped\":true,\"reason\":\"EMPEROR_GAMEDATA_ROOT unset or empty\"}\n";return;}
    const auto root=ResourceRoot::fromAbsolute(env);need(root.configured(),"original root configured");
    const auto a=loadSG3(root.resolve("DATA/China_General.sg3"));const auto catalog=enumerateOriginalMapSources(root);
    need(catalog.complete,"complete original catalog");
    size_t sources=0,scenes=0,withBases=0,live=0,bindable=0,placed=0,cells=0,decoded=0,failures=0,withheld=0,outlines=0,markers=0,native=0,visible=0,focused=0;
    size_t savedDrawMatches=0,savedSideMatches=0,savedMetadataRecords=0,randomRecords=0,changedRandomInputs=0,changedSelections=0;
    size_t newUniqueCells=0,newDrawIndependent=0,newDrawPreserved=0,newSubTileMatches=0,unboundCells=0;
    std::map<uint16_t,size_t> newlyBound;
    AppMapSurface app(root);
    for(const auto& source:catalog.sources){if(source.kind!=OriginalMapSourceKind::SaveFile)continue;++sources;
        const auto snapshot=loadOriginalSave(root.resolve(source.relativePath));need(snapshot.status==OriginalSaveStatus::LoadedSnapshot&&snapshot.readStable,"original save stable complete snapshot");
        std::vector<uint8_t> input;need(detail::readFile(root.resolve(source.relativePath),4u*1024u*1024u,input),"bounded independent SAV byte input");
        const auto reread=decodeOriginalSave(input);need(reread.status==OriginalSaveStatus::LoadedSnapshot&&reread.snapshotRevision==snapshot.snapshotRevision,"independent serializer audit reads the same saved snapshot revision");
        const auto container=decodeOriginalMapContainer(input);const auto& data=container.payload;const size_t base=snapshot.terrainOffset;
        // These literal SAV19 extents come from 0x52FDA0, not MAP offsets.
        // 0x531305 reads draw; 0x531349 reads subTile; 0x53136B reads random.
        need(container.complete&&data.size()>=base+519840&&base>=155952&&detail::u16(data.data())==19,"complete SAV19 serializer profile and both relative bounds");
        std::cout<<"{\"saved_source_ordinal\":"<<sources<<",\"save_version\":19,\"terrain_offset\":"<<base<<",\"draw_offset\":"<<base-155952<<",\"subtile_offset\":"<<base+311904<<",\"random_offset\":"<<base+415872<<",\"height_offset\":"<<base+1874984<<",\"persisted_image_grid\":false}\n";
        for(const auto& f:snapshot.buildings.footprints){
            if(f.type!=2&&f.type!=3&&f.type!=5&&f.type!=7&&f.type!=62&&f.type!=194&&f.type!=196)continue;
            const size_t origin=(228-snapshot.terrain.activeSide)/2,cell=(origin+f.y)*228+origin+f.x;
            need(f.imageInputs&&f.imageInputs->randomByte&&*f.imageInputs->randomByte==data[base+415872+cell],"random-dependent building input equals SAV runtime0xF1E780 byte");++randomRecords;
            auto wrong=f;wrong.imageInputs->randomByte=data[base+467856+cell];changedRandomInputs+=wrong.imageInputs->randomByte!=f.imageInputs->randomByte;
            const auto correct=originalBuildingBaseSelection(f),previous=originalBuildingBaseSelection(wrong);
            need(correct&&previous,"both diagnostic random inputs remain in the closed selector domain");
            changedSelections+=correct->code!=previous->code||correct->variant!=previous->variant;
        }
        auto s=composeOriginalBuildingScene(snapshot.terrain,snapshot.buildings,a);
        need(s.accepted&&s.rejectedRecords==0,"original base stamps have no shape or overlap rejection");++scenes;withBases+=!s.placed.empty();
        live+=s.liveRecords;bindable+=s.bindableRecords;placed+=s.placed.size();cells+=s.placedCells;
        std::set<size_t> newlySeen;
        const size_t crop=(228-snapshot.terrain.activeSide)/2;
        for(const auto& p:s.placed)if(p.footprint.type==54||p.footprint.type==58||p.footprint.type==59||p.footprint.type==231){
            ++newlyBound[p.footprint.type];
            for(const auto& component:p.components){if(p.footprint.type==59&&component.side==2)continue;
                const size_t cell=(crop+p.footprint.y+component.y)*228+crop+p.footprint.x+component.x;
                if(!newlySeen.insert(cell).second)continue;++newUniqueCells;
                const bool draw=data[base-155952+cell]==s.surface.draw[cell],subTile=(data[base+311904+cell]&15)==s.surface.subTile[cell];
                need(draw&&subTile,"newly placed unique wall/storage/market cells agree with saved draw and subTile");
                if(component.preservedDraw){newDrawPreserved+=draw;need((s.surface.draw[cell]&0x40)!=0,"market predicts draw marker independently while retaining serialized low bits");}
                else newDrawIndependent+=draw;
                newSubTileMatches+=subTile;
            }
        }
        need(snapshot.layers.ground&&snapshot.layers.ground->valid(),"same-source ground inputs are complete before occupancy integration");
        const auto ground=composeOriginalSavedGroundScene(snapshot.terrain,snapshot.buildings,*snapshot.layers.ground,a,
            loadSG3(root.resolve("DATA/China_Terrain.sg3")),loadSG3(root.resolve("DATA/China_Elevation.sg3")));
        need(ground.accepted&&ground.groundRebuilt&&ground.unboundBuildingCells==0,"all bit8 occupancy now has a bound surface before ground; entertainment overlays share market cells");
        for(const auto& p:ground.placed)for(const auto& component:p.components){
            const size_t cell=(crop+p.footprint.y+component.y)*228+crop+p.footprint.x+component.x;
            if(component.preservedDraw)need(ground.drawWritten[cell]==0x40&&ground.subTileWritten[cell]==0x1F,"market assigns draw marker and clears subTile low5; low draw bits remain saved input");
            if(p.footprint.type==54||p.footprint.type==58)need(ground.surface.subTile[cell]==0&&ground.subTileWritten[cell]==0xFF,"storage direct overrides clear the whole subTile byte");
        }
        unboundCells+=ground.unboundBuildingCells;
        OriginalMapCamera c;c.outlines=false;c.centerX=double(snapshot.terrain.activeSide-1)/2;c.centerY=c.centerX;
        if(!s.placed.empty()){c.centerX=s.placed.front().footprint.x;c.centerY=s.placed.front().footprint.y;}
        need(app.selectSavedBuildings(snapshot.terrain,snapshot.buildings),"native App adapter accepts same current SAV source");
        auto view=app.renderSavedBuildings(c);
        need(view.image.valid()&&view.bases.decodedCells==s.placedCells&&view.bases.geometryUnsupported==0&&view.bases.unresolvedIndices==0&&view.bases.archiveUnavailable==0,"App scene decodes every stamped cell including offscreen");
        decoded+=view.bases.decodedCells;failures+=view.bases.decodeFailures;visible+=view.bases.drawnCells;
        withheld+=view.withheldRecords;outlines+=view.withheldOutlines;markers+=view.unshapedMarkers;
        auto cg=createCGImageFromPremultipliedBGRA(view.image.width,view.image.height,view.image.pixelsBGRA);need(cg,"App city scene creates native image");if(cg){++native;CGImageRelease(cg);}
        // All record occurrences must reach a visible native compositor blit,
        // not just bind or disappear outside the one default viewport.
        for(const auto& p:s.placed){c.width=800;c.height=800;c.centerX=p.footprint.x;c.centerY=p.footprint.y;
            const auto& f=p.footprint;const size_t origin=(228-snapshot.terrain.activeSide)/2;bool metadataMatches=true;
            for(size_t y=0;y<originalFootprintHeight(f);++y)for(size_t x=0;x<originalFootprintWidth(f);++x){const size_t cell=(origin+f.y+y)*228+origin+f.x+x;
                const bool draw=data[base-155952+cell]==s.surface.draw[cell],side=(data[base+311904+cell]&15)==s.surface.subTile[cell];
                savedDrawMatches+=draw;savedSideMatches+=side;metadataMatches&=draw&&side;
            }
            need(metadataMatches,"saved draw/subTile equal final footprint; market low draw bits are retained inputs");savedMetadataRecords+=metadataMatches;
            auto subset=profile({p.footprint});
            if(f.type==0x3b)for(const auto& child:snapshot.buildings.footprints)
                if(std::find(f.imageInputs->marketStalls.begin(),f.imageInputs->marketStalls.end(),child.slot)!=f.imageInputs->marketStalls.end())subset.footprints.push_back(child);
            const auto isolated=composeOriginalBuildingScene(snapshot.terrain,subset,a);
            const auto one=renderMapImageGridViewport(snapshot.terrain,isolated.surface,{{2,&a,root.resolve("DATA/China_General.555"),201}},c,MapImageGridOrder::View0DrawCorner);
            const size_t area=size_t(originalFootprintWidth(f))*originalFootprintHeight(f);
            need(one.image.valid()&&one.stats.drawnCells==area&&one.stats.decodedCells==area&&one.stats.decodeFailures==0&&std::any_of(one.image.pixelsBGRA.begin(),one.image.pixelsBGRA.end(),[](auto pixel){return pixel>>24;}),"every placed occurrence itself paints a successful focused compositor view");++focused;
        }
        app.clear();need(!app.renderSavedBuildings(c).image.valid(),"source clear drops saved scene without stale image");
    }
    need(sources==6&&scenes==6&&withBases==4&&live==1065&&bindable==1063&&placed==1063&&focused==1063,"supplied six-source placed-record denominator lock");
    need(withheld==2&&outlines==0&&markers==2&&decoded==cells&&failures==0&&native==6,"withheld shapes/markers and complete decode/native scene counts");
    need(savedDrawMatches==1751&&savedSideMatches==1751&&savedMetadataRecords==1063&&cells==1719,"persisted SAV footprint metadata agrees for every bound record and cell");
    need(randomRecords==851&&changedRandomInputs==845&&changedSelections==389,"SAV serializer correction has an explicit affected-building denominator");
    need(newlyBound==std::map<uint16_t,size_t>{{54,6},{58,2},{59,2},{231,126}}&&newUniqueCells==236&&newDrawIndependent==212&&newDrawPreserved==24&&newSubTileMatches==236&&unboundCells==0,"new record, unique-cell and independent metadata denominators");
    std::cout<<"{\"newly_bound_records\":136,\"new_unique_cells\":"<<newUniqueCells<<",\"new_full_draw_independent_matches\":"<<newDrawIndependent<<",\"new_draw_preserved_input_matches\":"<<newDrawPreserved<<",\"new_subtile_matches\":"<<newSubTileMatches<<",\"unbound_surface_occupancy\":"<<unboundCells<<"}\n";
    std::cout<<"{\"saved_draw_cells_matched\":"<<savedDrawMatches<<",\"saved_subtile_cells_matched\":"<<savedSideMatches<<",\"footprint_cells\":"<<cells<<",\"saved_metadata_records_matched\":"<<savedMetadataRecords<<",\"bound_records\":"<<placed<<",\"random_dependent_records\":"<<randomRecords<<",\"corrected_random_inputs\":"<<changedRandomInputs<<",\"corrected_image_selections\":"<<changedSelections<<",\"stored_image_word_checks_eligible\":0,\"chain_agreement_cells_eligible\":0}\n";
    std::cout<<"{\"saved_sources\":"<<sources<<",\"native_scenes\":"<<scenes<<",\"native_CGImages\":"<<native<<",\"nonempty_sources_with_bases\":"<<withBases<<",\"live_records\":"<<live<<",\"bindable_records\":"<<bindable<<",\"placed_records\":"<<placed<<",\"focused_record_views\":"<<focused<<",\"withheld_records\":"<<withheld<<",\"withheld_footprints\":"<<outlines<<",\"unshaped_markers\":"<<markers<<",\"stamped_cells\":"<<cells<<",\"decoded_cells\":"<<decoded<<",\"default_view_drawn_cells\":"<<visible<<",\"decode_failures\":"<<failures<<",\"anchor\":\"view0:(x,y+size-1)\",\"order\":\"draw-corner(x+y,x)\",\"ground\":\"same SAV numerical terrain\",\"original_screen_agreement_verified\":false}\n";
}
}
int main(int argc,char** argv){try{if(argc!=2)throw std::runtime_error("explicit temporary fixture directory required");
    authored(argv[1]);const size_t authoredChecks=checks;corpus();std::cout<<"{\"passed\":true,\"authored_checks\":"<<authoredChecks<<",\"all_checks\":"<<checks<<"}\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
