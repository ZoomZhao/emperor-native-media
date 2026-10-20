#pragma once
#include "original_building_view.hpp"
#include "original_building_animation.hpp"
#include "map_reference.hpp"
#include "original_save.hpp"
#include "original_figure_update.hpp"
#include <set>
#include <tuple>

namespace game_lab {
struct OriginalSceneDepth {
    uint32_t diagonal=0,column=0;uint8_t phase=0;size_t sequence=0;
    bool operator<(const OriginalSceneDepth& b)const{return std::tie(diagonal,column,phase,sequence)<std::tie(b.diagonal,b.column,b.phase,b.sequence);}
};
struct OriginalSceneOverlay {
    std::string archive="DATA/China_General.sg3";uint32_t physicalId=0,tileset=2;
    uint16_t anchorX=0,anchorY=0;int32_t offsetX=0,offsetY=0;
    OriginalSceneDepth depth;uint8_t role=0,mode=0;size_t sourceSlot=0;
};
// VA0x82BD30/file0x42BD30: twelve (x,y,role,0,0) rows per
// orientation;0x490660/680 selects T(4B0)+role/view variant and
// 0x4906A0/C0 selects signed pixel offsets at VA0x82BFE0.
inline std::optional<std::vector<OriginalSceneOverlay>> bindOriginalEntertainmentOverlays(
    const OriginalBuildingObjectSource& source,const OriginalBuildingProfile& profile,const SG3Archive& archive){
    if(source.type!=0x47||source.size!=2||!source.savedView||*source.savedView!=0||source.slot>=4000||
        source.parentSlot>=4000||source.parentSlot>=source.slot||
        (archive.version!=213&&archive.version!=214)||archive.numImages!=archive.sprites.size())return std::nullopt;
    const auto parent=std::find_if(profile.footprints.begin(),profile.footprints.end(),[&](const auto& f){return f.slot==source.parentSlot;});
    if(parent==profile.footprints.end()||parent->type!=0x3b||!parent->imageInputs||!parent->imageInputs->marketOrientation||
        *parent->imageInputs->marketOrientation>1||!parent->imageInputs->savedView||*parent->imageInputs->savedView!=0)return std::nullopt;
    const bool rotated=*parent->imageInputs->marketOrientation==1;
    // 0x544930 ->0x541F60: market descriptor virtual+64=0x4E1D00
    // returns row8, (0,2)/(2,0), not a common size2 square.
    if(source.x!=parent->x+(rotated?2:0)||source.y!=parent->y+(rotated?0:2))return std::nullopt;
    static constexpr uint8_t roles[2][12]={{1,4,4,2,11,12,12,10,0,5,5,3},{1,7,7,0,8,12,12,9,2,6,6,3}};
    static constexpr uint8_t variant[12]={0,1,2,3,5,5,4,4,7,7,6,6};
    static constexpr int32_t offset[12][2]={{-8,-53},{-8,-53},{31,-53},{-8,-33},{31,-53},{-8,-33},
        {32,-31},{-7,-53},{29,-64},{-7,-44},{31,-43},{-9,-64}};
    const size_t first=archive.imageIndex[0xb0];if(!first)return std::nullopt;
    std::vector<OriginalSceneOverlay> out;
    for(unsigned row=0;row<12;++row){
        const uint8_t role=roles[rotated][row];if(role==12)continue;
        const size_t id=first+variant[role];if(id>archive.sprites.size())return std::nullopt;
        const auto& s=archive.sprites[id-1];
        if(s.type!=256||s.mirrorOffset||s.tileSize||s.width<=0||s.height<=0||s.width>4096||s.height>4096||
            s.bitmapId>=archive.bitmaps.size()||archive.bitmaps[s.bitmapId].filename!="China_Entertainment.bmp"||
            archive.sprites[first-1].bitmapId!=s.bitmapId)return std::nullopt;
        const uint16_t x=uint16_t(source.x+(rotated?row/4:row%4)),y=uint16_t(source.y+(rotated?row%4:row/4));
        const uint8_t mode=role>=8?2:1;
        out.push_back({"DATA/China_General.sg3",uint32_t(id),2,x,y,offset[role][0],offset[role][1],
            {uint32_t(x)+y,x,uint8_t(mode==1?1:4),row},role,mode,source.slot});
    }
    return out;
}
struct OriginalBuildingPlacement {
    OriginalBuildingFootprint footprint;
    OriginalBuildingImageBinding binding;
    uint32_t drawX=0,drawY=0;
    std::vector<OriginalBuildingComponent> components;
};
struct OriginalBuildingScene {
    OriginalTerrainState surface;
    std::vector<OriginalBuildingPlacement> placed;
    std::vector<OriginalBuildingFootprint> withheldFootprints;
    std::vector<OriginalBuildingMarker> markers;
    std::vector<OriginalSceneOverlay> overlays;
    size_t overlayRecords=0,overlayObjectsExpected=0;
    size_t animationCandidates=0,animationEnabled=0,animationGated=0,animationUnknown=0,animationInvalid=0,animationSelected=0;
    size_t figureLive=0,figurePlaced=0,figureHidden=0,figureZeroImage=0,figureUnsupported=0;
    std::map<uint8_t,size_t> figureBoundTypes,figureBlockedTypes;
    size_t figureForwardMatched=0,figureForwardCompared=0,figureForwardSelected=0;
    std::map<std::string,size_t> figureForwardMismatch;
    size_t figureZeroResolved=0,figureZeroBlocked=0;
    bool figurePostLoadK1=false;
    std::optional<unsigned> timelineUpdates;
    size_t timelineFrames=0;
    size_t figureRemoved=0;
    // Explicit first-tick diagnostics, never overlay commands or saved oracles.
    std::map<size_t,OriginalFigureImageSelection> figureFirstTickImages;
    uint64_t animationTickMs=0;
    size_t liveRecords=0,bindableRecords=0,placedCells=0,rejectedRecords=0;
    bool accepted=false;
    bool groundRebuilt=false;
    size_t groundCells=0,groundUnknown=0,groundNoWrite=0,groundUnresolved=0,unboundBuildingCells=0;
    size_t groundDrawFullyAssigned=0,buildingDrawFullyAssigned=0,buildingDrawMarkerOnly=0;
    std::vector<GroundMapCell> groundSelections;
    // Metadata masks identify independently assigned bits, excluding saved input.
    std::vector<uint8_t> drawWritten,subTileWritten,buildingOccupancy;
};
// Saved image0x46BAD1 ->0x413960. Tick0 only: walking/animation updates
// are simulation-driven and cannot be inferred from elapsed milliseconds.
// Forward reconstructs primary and carried-goods secondary words from non-image
// saved state; neither saved image word is available to either selector.
enum class OriginalFigureRenderSource {Saved,Forward,PostLoadK1};
inline void installOriginalFigureOverlays(OriginalBuildingScene& scene,const OriginalMapTerrain& terrain,
    const OriginalFigureProfile& figures,const std::map<uint32_t,const SG3Archive*>& archives,uint64_t tickMs=0,
    OriginalFigureRenderSource source=OriginalFigureRenderSource::Saved,const OriginalBuildingProfile* buildings=nullptr,
    const OriginalFigureOrderedPass* ordered=nullptr){
    if(source==OriginalFigureRenderSource::PostLoadK1){
        // Only a complete ordered pass can replace the snapshot. Keep the
        // original live denominator, including the worker entering state2.
        if(tickMs||!ordered||!ordered->complete||!ordered->context||ordered->computed!=figures.live.size())return;
        auto updated=figures;updated.live.clear();updated.slots.clear();updated.cellHeads=ordered->context->cellHeads;
        for(const auto& entry:ordered->context->figures){updated.slots.push_back(entry.second);if(entry.second.alive)updated.live.push_back(entry.second);}
        installOriginalFigureOverlays(scene,terrain,updated,archives,0,OriginalFigureRenderSource::Saved,buildings);
        scene.figureRemoved=figures.live.size()-updated.live.size();scene.figureLive=figures.live.size();
        scene.figurePostLoadK1=true;return;
    }
    if(!scene.accepted||!terrain.valid()||scene.surface.terrain!=terrain.flags||
       !analyzeOriginalMap(terrain).cropValid()||figures.status!=OriginalFigureStatus::Complete||figures.live.size()>8192)return;
    scene.overlays.erase(std::remove_if(scene.overlays.begin(),scene.overlays.end(),[](const auto& o){return o.mode==4;}),scene.overlays.end());
    scene.figurePlaced=scene.figureHidden=scene.figureZeroImage=scene.figureUnsupported=0;
    scene.figureBoundTypes.clear();scene.figureBlockedTypes.clear();
    scene.figureForwardMatched=scene.figureForwardCompared=scene.figureForwardSelected=0;scene.figureForwardMismatch.clear();
    scene.figureZeroResolved=scene.figureZeroBlocked=0;scene.figureFirstTickImages.clear();
    scene.figurePostLoadK1=false;
    scene.timelineUpdates.reset();scene.timelineFrames=0;
    scene.figureRemoved=0;
    scene.figureLive=figures.live.size();const size_t origin=(228-terrain.activeSide)/2;
    const auto imageState=originalFigureImageState(figures);
    std::map<size_t,const OriginalFigure*> bySlot;
    for(const auto& f:figures.live)if(f.slot>=8192||!bySlot.emplace(f.slot,&f).second){scene.figureUnsupported=scene.figureLive;return;}
    //4EA3B0->418450->418430: kind13 is linked one diagonal cell
    // forward in view0 (table7AB3B8=229), while+28 stays the logical cell.
    const auto linked=[&](const OriginalFigure& f){
        if(figures.cellHeads.size()!=51984||originalFigureDrawCell(f)>=51984)return false;
        size_t slot=figures.cellHeads[size_t(originalFigureDrawCell(f))];std::set<size_t> visited;bool reached=false;
        while(slot){if(visited.size()>=50||!visited.insert(slot).second)return false;const auto found=bySlot.find(slot);
            if(found==bySlot.end()||originalFigureDrawCell(*found->second)!=originalFigureDrawCell(f))return false;
            reached|=slot==f.slot;slot=found->second->next;
        }return reached;
    };
    for(const auto& f:figures.live){
        if(f.hidden){++scene.figureHidden;continue;}
        if(!f.image){
            ++scene.figureZeroImage;
            const auto input=originalFigureImageInputs(f);
            if(originalFigureImageClearedAtLoad(input))++scene.figureZeroResolved;
            else {++scene.figureZeroBlocked;++scene.figureBlockedTypes[f.type];}
            scene.figureFirstTickImages.emplace(f.slot,originalFigureFirstTickImage(input,archives));
            continue;
        }
        std::optional<uint16_t> ownerType;
        if(buildings&&f.owner>=0&&size_t(f.owner)<buildings->slotTypes.size())ownerType=buildings->slotTypes[f.owner];
        const auto reconstructed=figureImage(originalFigureImageInputs(f),archives,
            f.cell<terrain.flags.size()?terrain.flags[f.cell]:0,figures.savedView,ownerType,&imageState);
        ++scene.figureForwardCompared;
        if(reconstructed.status==OriginalFigureImageStatus::Selected){++scene.figureForwardSelected;
            if(reconstructed.image==f.image)++scene.figureForwardMatched;
            else ++scene.figureForwardMismatch["selected_image_differs"];
        }else ++scene.figureForwardMismatch[originalFigureImageStatusName(reconstructed.status)];
        if(source==OriginalFigureRenderSource::Forward&&reconstructed.status!=OriginalFigureImageStatus::Selected){
            ++scene.figureUnsupported;++scene.figureBlockedTypes[f.type];continue;
        }
        const auto sub=originalFigureSubcell(f);
        const int anchorX=int(f.x)+(f.type==1),anchorY=int(f.y)+(f.type==1);
        if(tickMs||figures.savedView||!sub||f.x<0||f.y<0||anchorX>=int(terrain.activeSide)||anchorY>=int(terrain.activeSide)||
           f.cell!=(origin+f.y)*228+origin+f.x||(terrain.flags[f.cell]&0x80000u)||!linked(f)||f.drawClass||f.state85==101){
            ++scene.figureUnsupported;++scene.figureBlockedTypes[f.type];continue;
        }
        // Simple and the four cited two-image branches0x46C636..0x46C6AD.
        if(source==OriginalFigureRenderSource::Saved&&f.secondaryImage&&f.type!=19&&f.type!=20&&f.type!=21&&f.type!=22){
            ++scene.figureUnsupported;++scene.figureBlockedTypes[f.type];continue;
        }
        std::vector<OriginalSceneOverlay> selected;
        const auto depthScore=originalFigureDepthScore(f);
        if(!depthScore){++scene.figureUnsupported;++scene.figureBlockedTypes[f.type];continue;}
        const int score=*depthScore;
        const size_t sequence=size_t(score+128)*8192*2+f.slot*2;
        const auto add=[&](uint32_t word,int ox,int oy,size_t order){
            const uint32_t slot=word>>14;const auto name=originalFigureArchiveName(slot);const auto found=archives.find(slot);
            if(!name||found==archives.end()||!found->second)return false;
            // These v214 figure archives have no Zeus_system.bmp prefix:
            // loader0x5CD354..3B0 trims0, so physical ID=low14+1.
            const auto& a=*found->second;const uint32_t id=(word&0x3fff)+1;
            if(a.version!=214||a.numImages!=a.sprites.size()||a.imageIndex[0]!=1||a.bitmaps.empty()||
               a.bitmaps[0].filename=="Zeus_system.bmp"||id>a.sprites.size())return false;
            const auto& s=a.sprites[id-1];
            if((s.type!=256&&s.type!=0)||s.tileSize||s.width<=0||s.height<=0||s.width>4096||s.height>4096||s.mirrorOffset>0)return false;
            //41=half-width39+2;30=half-height20+10 (0x469820,
            //0x46BC1C..93). Original subtracts the SELECTED sprite offsets,
            // even for mirrored payloads. This is independent of image width.
            selected.push_back({std::string("DATA/")+name,id,slot,uint16_t(anchorX),uint16_t(anchorY),
                ox-s.animationOffsetX,oy-s.animationOffsetY,{uint32_t(anchorX+anchorY),uint32_t(anchorX),3,order},f.type,4,f.slot});
            return true;
        };
        int baseline=30;
        if(terrain.flags[f.cell]&0x20000u){
            if(scene.surface.roadDistance.size()!=51984){++scene.figureUnsupported;++scene.figureBlockedTypes[f.type];continue;}
            if(!scene.surface.roadDistance[f.cell])baseline=38;
            else if(scene.surface.roadDistance[f.cell]==100)baseline=34;
        }
        const uint32_t primary=source==OriginalFigureRenderSource::Forward?reconstructed.image:f.image;
        bool good=add(primary,41+sub->first,baseline+sub->second,sequence);
        const auto cargo=source==OriginalFigureRenderSource::Forward?
            figureSecondaryImage(originalFigureImageInputs(f),archives,figures.savedView,&imageState):OriginalFigureImageSelection{};
        const uint32_t secondary=source==OriginalFigureRenderSource::Forward?cargo.image:f.secondaryImage;
        if(source==OriginalFigureRenderSource::Forward&&cargo.status!=OriginalFigureImageStatus::Selected)good=false;
        if(secondary){const bool type20=f.type==20;
            const int extraX=source==OriginalFigureRenderSource::Forward?0:f.extraX;
            const int extraY=source==OriginalFigureRenderSource::Forward?0:f.extraY;
            good=good&&add(secondary,39+sub->first+(type20?2:3)+extraX,
                baseline-10+sub->second+(type20?10:11)+extraY,sequence+1);
        }
        if(!good){++scene.figureUnsupported;++scene.figureBlockedTypes[f.type];continue;}
        scene.overlays.insert(scene.overlays.end(),selected.begin(),selected.end());++scene.figurePlaced;++scene.figureBoundTypes[f.type];
    }
}
// Owned exe SHA256 0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658.
// 0x4B72B0: view codes0,2,4,6 mark (0,h-1),(0,0),(w-1,0),(w-1,h-1).
// The existing flat projection is view0 (0x4699D0), so only that view is rendered.
inline std::optional<std::pair<uint32_t,uint32_t>> originalBuildingDrawCorner(
    const OriginalBuildingFootprint& f,unsigned view=0){
    const uint32_t w=originalFootprintWidth(f),h=originalFootprintHeight(f);
    if(!w||!h||w>6||h>6||view>6||(view&1))return std::nullopt;
    return std::make_pair(uint32_t(f.x)+(view>=4?w-1:0),uint32_t(f.y)+(view==0||view==6?h-1:0));
}
// Compose a base-image working grid, also used as the pre-ground SAV seed below.
// SAV's followingU32Unknown is not an image field. No MAP is borrowed.
inline OriginalBuildingScene composeOriginalBuildingScene(const OriginalMapTerrain& terrain,
    const OriginalBuildingProfile& profile,const SG3Archive& archive,uint64_t tickMs=0,
    const OriginalBuildingEmployment* employment=nullptr){
    OriginalBuildingScene out;out.animationTickMs=tickMs;
    if(!terrain.valid()||!analyzeOriginalMap(terrain).cropValid()||profile.status!=OriginalBuildingStatus::Complete||
        (profile.savedView&&*profile.savedView!=0)||
        profile.footprints.size()>4000||profile.objectBuildings.size()>4000||profile.liveSizedRecords>4000||profile.unplacedMarkers.size()>4000)return out;
    const size_t n=OriginalTerrainState::cells;
    auto& s=out.surface;s.terrain=terrain.flags;s.image.assign(n,0);s.draw.assign(n,0);s.subTile.assign(n,0);
    s.feature.assign(n,0);s.randomByte.assign(n,0);s.gF05160.assign(n,0);s.gEAC3F0.assign(n,0);
    s.roadDistance.assign(n,0);s.fertility.assign(n,0);s.gBEBF3A.assign(n,0);s.gBF8A4A.assign(n,0);
    out.liveRecords=profile.liveSizedRecords;out.markers=profile.unplacedMarkers;
    const size_t origin=(228-terrain.activeSide)/2;
    std::vector<unsigned> owners(n,0);
    std::vector<OriginalBuildingPlacement> candidates;
    std::set<size_t> slots;
    for(const auto& f:profile.footprints){
        const auto components=bindOriginalBuildingComponents(f,"DATA/China_General.sg3",archive,&profile);
        if(!components||components->empty()){out.withheldFootprints.push_back(f);continue;}
        ++out.bindableRecords;
        const auto corner=f.type==0x3b?std::optional<std::pair<uint32_t,uint32_t>>({f.x,f.y}):originalBuildingDrawCorner(f);
        if(!validOriginalBuildingFootprint(terrain,f)||!corner||f.slot>=4000||!slots.insert(f.slot).second){++out.rejectedRecords;out.withheldFootprints.push_back(f);continue;}
        candidates.push_back({f,components->front().binding,corner->first,corner->second,*components});
        // Market's referenced stalls deliberately share its descriptor cells.
        // They remain separately counted records; only market pavement owns
        // new cells in the overlap guard.
        for(const auto& c:*components)if(f.type!=0x3b||c.side==1)
            for(size_t y=0;y<c.side;++y)for(size_t x=0;x<c.side;++x)++owners[(origin+f.y+c.y+y)*228+origin+f.x+c.x+x];
    }
    for(const auto& p:candidates){const auto& f=p.footprint;bool overlap=false;
        for(const auto& c:p.components)
            for(size_t y=0;y<c.side;++y)for(size_t x=0;x<c.side;++x){
                const unsigned count=owners[(origin+f.y+c.y+y)*228+origin+f.x+c.x+x];
                overlap|=count>1||(f.type==0x3b&&c.side==2&&count!=1);
            }
        // Rebuild order/lifecycle overrides for overlapping records are unverified.
        bool invalid=false;for(const auto& c:p.components)invalid|=c.binding.physicalId<201||c.binding.physicalId-201>0x3fff;
        if(overlap||invalid){++out.rejectedRecords;out.withheldFootprints.push_back(f);continue;}
        for(const auto& c:p.components)for(size_t y=0;y<c.side;++y)for(size_t x=0;x<c.side;++x){const size_t cell=(origin+f.y+c.y+y)*228+origin+f.x+c.x+x;
            // Writer0x4B7423..0x4B743F stamps the same surface word; the small
            // table0x81FF18 supplies column | row<<3 and one corner gets0x40.
            s.image[cell]=(c.binding.tileset<<14)|(c.binding.physicalId-201);s.subTile[cell]=uint8_t(c.side-1);s.draw[cell]=uint8_t(x+8*y);
            if(f.type==0x36||f.type==0x3a){
                // Initial square0x4B72B0 leaves column/row bytes; direct
                // overrides0x5D6095 set corner-bit and clear subTile only.
                s.draw[cell]=uint8_t(c.x+8*c.y+0x40);
            }else if(x==0&&y==c.side-1)s.draw[cell]|=0x40;
            if(c.preservedDraw)s.draw[cell]=*c.preservedDraw;
        }
        out.placed.push_back(p);
    }
    for(const auto& p:out.placed){
        // Multipart parent components are owned/animated by their own records.
        // All animation-bearing bound bases in the measured corpus are squares.
        if(p.components.size()!=1)continue;
        const auto& base=archive.sprites[p.binding.physicalId-1];if(!base.animationSpriteCount)continue;
        ++out.animationCandidates;
        const auto enabled=originalBuildingAnimationEnabled(p.footprint,employment);
        if(!enabled){++out.animationUnknown;continue;}
        if(!*enabled){++out.animationGated;continue;}
        ++out.animationEnabled;
        const auto state=originalBuildingAnimationState(base,*p.footprint.animationInputs->phase,tickMs);
        if(!state){++out.animationInvalid;continue;}
        const unsigned frame=*state&127;if(!frame)continue;
        const size_t id=p.binding.physicalId+frame;
        if(id>archive.sprites.size()){++out.animationInvalid;continue;}
        const auto offset=originalBuildingAnimationOffset(base,archive.sprites[id-1],p.footprint.size);
        if(!offset){++out.animationInvalid;continue;}
        out.overlays.push_back({"DATA/China_General.sg3",uint32_t(id),2,uint16_t(p.drawX),uint16_t(p.drawY),offset->first,offset->second,
            {p.drawX+p.drawY,p.drawX,2,p.footprint.slot},0,3,p.footprint.slot});
        ++out.animationSelected;
    }
    for(const auto& source:profile.objectBuildings){
        const auto overlays=bindOriginalEntertainmentOverlays(source,profile,archive);
        out.overlayObjectsExpected+=10;
        if(!overlays||std::none_of(out.placed.begin(),out.placed.end(),[&](const auto& p){return p.footprint.slot==source.parentSlot;}))continue;
        bool valid=true;
        for(const auto& object:*overlays){
            if(object.anchorX>=terrain.activeSide||object.anchorY>=terrain.activeSide){valid=false;break;}
            const size_t cell=(origin+object.anchorY)*228+origin+object.anchorX;
            // Ordinary 0x4EDA50 suppression is false without bit100. Do not
            // extend this static writer into its runtime-suppressed domain.
            valid&=!(terrain.flags[cell]&0x80100u)&&(terrain.flags[cell]&8u)&&s.image[cell]!=0;
        }
        if(!valid)continue;
        ++out.bindableRecords;++out.overlayRecords;
        out.overlays.insert(out.overlays.end(),overlays->begin(),overlays->end());
        out.markers.erase(std::remove_if(out.markers.begin(),out.markers.end(),[&](const auto& m){return m.slot==source.slot;}),out.markers.end());
    }
    out.placedCells=std::count_if(s.image.begin(),s.image.end(),[](auto word){return word!=0;});
    out.accepted=true;return out;
}
// SAV19 clears image at0x534CB9, runs building+100 dispatch0x4BC220 at
// 0x534E36, then ground0x53D630 at0x534E40, post-object redraw0x41F400
// at0x534E5B. Unknown building art remains occupancy, never a fake SG word.
inline OriginalBuildingScene composeOriginalSavedGroundScene(const OriginalMapTerrain& terrain,
    const OriginalBuildingProfile& profile,const OriginalSavedGround& saved,
    const SG3Archive& general,const SG3Archive& ground,const SG3Archive& elevation,uint64_t tickMs=0,
    const OriginalBuildingEmployment* employment=nullptr){
    auto out=composeOriginalBuildingScene(terrain,profile,general,tickMs,employment);
    if(!out.accepted||!saved.valid()||saved.activeSide!=terrain.activeSide||saved.state.terrain!=terrain.flags){out.accepted=false;return out;}
    auto working=saved.state;working.image=out.surface.image;
    const size_t n=OriginalTerrainState::cells;
    out.buildingOccupancy.assign(n,0);
    // Terrain bit8 is the dispatch gate at0x53ECBA; even an unbound multipart
    // writer cannot turn it into ordinary ground. Market/warehouse/station/wall
    // footprints write before ground (cited individually in docs/SAV.md).
    // Entertainment has no ordinary square contract: its parent market already
    // covers those cells. Do not manufacture a square from its common size.
    for(size_t cell=0;cell<n;++cell)if(!(terrain.flags[cell]&0x80000u)&&(terrain.flags[cell]&8u)) {
        out.buildingOccupancy[cell]=working.image[cell]?1:2;
        if(!working.image[cell]){working.image[cell]=UINT32_MAX;++out.unboundBuildingCells;}
    }
    // Exe-derived stamps seed the rebuild. Market direct writes preserve their
    // serialized low draw bits; those bits are input, not independent evidence.
    for(size_t cell=0;cell<n;++cell)if(out.surface.image[cell]) {
        working.draw[cell]=out.surface.draw[cell];working.subTile[cell]=uint8_t((working.subTile[cell]&0xE0u)|out.surface.subTile[cell]);
    }
    // Unlike 0x4B72B0, storage's final direct writes clear the entire byte.
    const size_t origin=(228-terrain.activeSide)/2;
    for(const auto& p:out.placed)if(p.footprint.type==0x36||p.footprint.type==0x3a)
        for(const auto& c:p.components)working.subTile[(origin+p.footprint.y+c.y)*228+origin+p.footprint.x+c.x]=0;
    auto rebuilt=selectGroundMapSeeded(working,terrain.activeSide,saved.material,saved.owner);
    if(rebuilt.cells.size()!=n||!rebuilt.surface.valid()){out.accepted=false;return out;}
    out.surface=std::move(rebuilt.surface);out.surface.terrain=terrain.flags;
    out.drawWritten=std::move(rebuilt.drawWritten);out.subTileWritten=std::move(rebuilt.subTileWritten);
    out.groundSelections=std::move(rebuilt.cells);
    for(size_t cell=0;cell<n;++cell) {
        if(out.buildingOccupancy[cell]) {
            out.surface.image[cell]=working.image[cell]==UINT32_MAX?0:working.image[cell];
            if(out.buildingOccupancy[cell]==1){out.drawWritten[cell]=0xFF;out.subTileWritten[cell]=0x0F;}
            continue;
        }
        out.surface.image[cell]=0;
        if(terrain.flags[cell]&0x80000u)continue;
        const auto& selection=out.groundSelections[cell].selection;
        if(!selection.known){if(selection.chainNoWrite)++out.groundNoWrite;else ++out.groundUnknown;continue;}
        const uint32_t tileset=selection.code>>9;const auto* archive=tileset==3?&ground:tileset==16?&elevation:tileset==2?&general:nullptr;
        const size_t slot=selection.code&0x1ffu;
        const size_t id=archive&&slot<archive->imageIndex.size()?size_t(archive->imageIndex[slot])+selection.variant:0;
        if(!archive||slot>=archive->imageIndex.size()||!archive->imageIndex[slot]||id<201||id>archive->sprites.size()||id-201>0x3fff){++out.groundUnresolved;continue;}
        out.surface.image[cell]=(tileset<<14)|uint32_t(id-201);++out.groundCells;
    }
    // Market pavement assigns only the marker bit; its lower draw bits came
    // from the save and cannot count as independent writer predictions.
    for(const auto& p:out.placed)for(const auto& c:p.components)if(c.preservedDraw){
        const size_t cell=(origin+p.footprint.y+c.y)*228+origin+p.footprint.x+c.x;
        out.drawWritten[cell]=0x40;out.subTileWritten[cell]=0x1F;
    }
    for(const auto& p:out.placed)if(p.footprint.type==0x36||p.footprint.type==0x3a)
        for(const auto& c:p.components)out.subTileWritten[(origin+p.footprint.y+c.y)*228+origin+p.footprint.x+c.x]=0xFF;
    for(size_t cell=0;cell<n;++cell)if(!(terrain.flags[cell]&0x80000u)){
        if(out.buildingOccupancy[cell]==1){
            out.buildingDrawFullyAssigned+=out.drawWritten[cell]==0xFF;
            out.buildingDrawMarkerOnly+=out.drawWritten[cell]==0x40;
        }else if(!out.buildingOccupancy[cell])out.groundDrawFullyAssigned+=out.drawWritten[cell]==0xFF;
    }
    out.groundRebuilt=true;return out;
}
struct OriginalBuildingSceneView {
    DecodedImage image;
    MapImageGridStats bases;
    size_t liveRecords=0,placedRecords=0,withheldRecords=0,withheldOutlines=0,unshapedMarkers=0;
    size_t groundCells=0,groundUnknown=0,groundNoWrite=0,groundUnresolved=0,unboundBuildingCells=0;
    size_t overlayObjects=0,overlayDecoded=0,overlayDrawn=0,overlayCulled=0,overlayDecodeFailures=0,overlayInvalid=0;
    size_t animationCandidates=0,animationEnabled=0,animationGated=0,animationUnknown=0,animationSelected=0,animationDecoded=0,animationInvalid=0;
    size_t figureLive=0,figurePlaced=0,figureHidden=0,figureZeroImage=0,figureUnsupported=0,figureDecoded=0;
    size_t figureForwardMatched=0,figureForwardCompared=0,figureForwardSelected=0;
    std::map<std::string,size_t> figureForwardMismatch;
    size_t figureZeroResolved=0,figureZeroBlocked=0;
    bool figurePostLoadK1=false;
    std::optional<unsigned> timelineUpdates;
    size_t timelineFrames=0;
    size_t figureRemoved=0;
    std::map<size_t,OriginalFigureImageSelection> figureFirstTickImages;
};
// The surface renderer remains the independent validation/accounting pass. Only
// scenes containing objects replay its validated full SAV stamps as commands,
// inserting objects at their cell phases. No MAP/candidate geometry is inferred.
inline bool renderOriginalSceneOverlays(const OriginalMapTerrain& terrain,const OriginalBuildingScene& scene,
    const std::vector<MapImageGridTileset>& tilesets,const OriginalMapCamera& camera,OriginalBuildingSceneView& out,DecodedImage& canvas){
    if(scene.overlays.empty())return true;
    if(scene.overlays.size()>40000)return false;
    struct Command {OriginalSceneDepth depth;uint32_t tileset=0,id=0;int left=0,top=0;bool object=false,animation=false,figure=false;};
    std::vector<Command> commands;const int scale=int(camera.scale);const size_t origin=(228-terrain.activeSide)/2;
    const auto position=[&](unsigned x,unsigned y,int ox,int oy){
        const double dx=double(x)-camera.centerX,dy=double(y)-camera.centerY;
        return std::make_pair(int(std::floor((dx-dy)*40*scale+double(camera.width)/2+.5))+ox*scale,
            int(std::floor((dx+dy)*20*scale+double(camera.height)/2+.5))+oy*scale);
    };
    const auto findTileset=[&](uint32_t slot)->const MapImageGridTileset*{for(const auto& t:tilesets)if(t.tileset==slot)return &t;return nullptr;};
    for(unsigned y=0;y<terrain.activeSide;++y)for(unsigned x=0;x<terrain.activeSide;++x){
        const size_t cell=(origin+y)*228+origin+x;const uint32_t word=scene.surface.image[cell];
        if(!word||(terrain.flags[cell]&0x80000u)||!(scene.surface.draw[cell]&0x40))continue;
        const auto* t=findTileset(word>>14);if(!t||!t->archive)return false;
        const size_t id=(word&0x3fff)+t->firstRecordId;if(!id||id>t->archive->sprites.size())return false;
        const auto& s=t->archive->sprites[id-1];const unsigned side=(scene.surface.subTile[cell]&15)+1;
        if(s.type!=30||s.mirrorOffset||side>6||y+1<side||s.width!=int(80*side-2)||s.height<int(40*side)||s.uncompressedLength!=3200*side*side)return false;
        const auto p=position(x,y-side+1,-int((side-1)*40+39),-(int(s.height)-int(side)*40+20));
        commands.push_back({{x+y,x,0,0},t->tileset,uint32_t(id),p.first,p.second,false});
    }
    out.overlayObjects=scene.overlays.size();
    for(const auto& object:scene.overlays){
        const auto* t=findTileset(object.tileset);
        const auto* name=object.mode==4?originalFigureArchiveName(object.tileset):mapImageGridArchiveName(object.tileset);
        if(object.anchorX>=terrain.activeSide||object.anchorY>=terrain.activeSide||!t||!t->archive||
            !name||object.archive!=std::string("DATA/")+name||!object.physicalId||object.physicalId>t->archive->sprites.size()||
            object.depth.diagonal!=uint32_t(object.anchorX)+object.anchorY||object.depth.column!=object.anchorX||
            object.mode<1||object.mode>4||object.depth.phase!=(object.mode==1?1:object.mode==2?4:object.mode==4?3:2)||
            object.offsetX < -4096||object.offsetX>4096||object.offsetY < -4096||object.offsetY>4096){++out.overlayInvalid;continue;}
        const auto& s=t->archive->sprites[object.physicalId-1];
        if((s.type!=256&&!((object.mode==3||object.mode==4)&&s.type==0))||(s.mirrorOffset&&object.mode!=4)||s.width<=0||s.height<=0||s.width>4096||s.height>4096){++out.overlayInvalid;continue;}
        // 0x5CE800 positions the78x40 surface at the scan's (left,top).
        // Existing projection names its centre, so the scan origin is(-39,-20).
        // 0x44F86B uses that same origin + object's signed offsets directly.
        const auto p=position(object.anchorX,object.anchorY,object.offsetX-39,object.offsetY-20);
        commands.push_back({object.depth,object.tileset,object.physicalId,p.first,p.second,true,object.mode==3,object.mode==4});
    }
    std::stable_sort(commands.begin(),commands.end(),[](const auto& a,const auto& b){return a.depth<b.depth;});
    canvas.pixelsBGRA.assign(canvas.pixelsBGRA.size(),0);
    std::map<std::pair<uint32_t,uint32_t>,DecodedImage> cache;std::set<std::pair<uint32_t,uint32_t>> failed;size_t bytes=0;
    for(const auto& command:commands){
        const auto key=std::make_pair(command.tileset,command.id);auto image=cache.find(key);
        if(image==cache.end()&&!failed.count(key)){
            const auto* t=findTileset(command.tileset);auto decoded=decodeSG3Sprite(*t->archive,command.id-1,t->internal555);
            if(!decoded.image.valid())failed.insert(key);
            else {const size_t added=decoded.image.pixelsBGRA.size()*4;if(added>64u*1024u*1024u-bytes)return false;bytes+=added;image=cache.emplace(key,std::move(decoded.image)).first;}
        }
        if(image==cache.end()){if(command.object)++out.overlayDecodeFailures;continue;}
        const auto& src=image->second;if(command.object)++out.overlayDecoded;if(command.animation)++out.animationDecoded;
        if(command.figure)++out.figureDecoded;
        if(command.left>=int(canvas.width)||command.top>=int(canvas.height)||command.left+int(src.width)*scale<=0||command.top+int(src.height)*scale<=0){if(command.object)++out.overlayCulled;continue;}
        if(command.object)++out.overlayDrawn;
        for(unsigned y=0;y<src.height;++y)for(unsigned x=0;x<src.width;++x){
            const uint32_t pixel=src.pixelsBGRA[size_t(y)*src.width+x],alpha=pixel>>24;if(!alpha)continue;
            for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx){const int xx=command.left+int(x)*scale+sx,yy=command.top+int(y)*scale+sy;
                if(xx<0||yy<0||xx>=int(canvas.width)||yy>=int(canvas.height))continue;
                auto& dst=canvas.pixelsBGRA[size_t(yy)*canvas.width+size_t(xx)];uint32_t mixed=0;
                for(unsigned shift:{0u,8u,16u,24u})mixed|=(((pixel>>shift)&255)+(((dst>>shift)&255)*(255-alpha)+127)/255)<<shift;
                dst=mixed;
            }
        }
    }
    return true;
}
inline OriginalBuildingSceneView renderOriginalBuildingScene(const OriginalMapTerrain& terrain,
    const OriginalBuildingScene& scene,const SG3Archive& archive,const std::string& companion,
    const OriginalMapCamera& camera,int bit=-1,const std::vector<MapImageGridTileset>& groundTilesets={}){
    OriginalBuildingSceneView out;if(!scene.accepted||scene.surface.terrain!=terrain.flags)return out;
    out.animationCandidates=scene.animationCandidates;out.animationEnabled=scene.animationEnabled;out.animationGated=scene.animationGated;
    out.animationUnknown=scene.animationUnknown;out.animationSelected=scene.animationSelected;out.animationInvalid=scene.animationInvalid;
    out.figureLive=scene.figureLive;out.figurePlaced=scene.figurePlaced;out.figureHidden=scene.figureHidden;
    out.figureForwardMatched=scene.figureForwardMatched;out.figureForwardCompared=scene.figureForwardCompared;
    out.figureForwardSelected=scene.figureForwardSelected;out.figureForwardMismatch=scene.figureForwardMismatch;
    out.figureZeroResolved=scene.figureZeroResolved;out.figureZeroBlocked=scene.figureZeroBlocked;
    out.figurePostLoadK1=scene.figurePostLoadK1;
    out.timelineUpdates=scene.timelineUpdates;out.timelineFrames=scene.timelineFrames;
    out.figureRemoved=scene.figureRemoved;
    out.figureFirstTickImages=scene.figureFirstTickImages;
    out.figureZeroImage=scene.figureZeroImage;out.figureUnsupported=scene.figureUnsupported;
    auto background=renderOriginalBuildingFootprints(terrain,scene.withheldFootprints,camera,bit,scene.markers);
    if(!background.image.valid())return out;
    auto tilesets=groundTilesets;tilesets.push_back({2,&archive,companion,201});
    auto bases=renderMapImageGridViewport(terrain,scene.surface,tilesets,camera,MapImageGridOrder::View0DrawCorner);
    if(!bases.image.valid())return out;
    if(!renderOriginalSceneOverlays(terrain,scene,tilesets,camera,out,bases.image))return {};
    // Existing image-grid compositor supplies the painter order and premultiplied
    // base layer. Numerical ground/withheld annotations are the diagnostic floor.
    for(size_t i=0;i<bases.image.pixelsBGRA.size();++i){const uint32_t src=bases.image.pixelsBGRA[i],a=src>>24;if(!a)continue;
        auto& dst=background.image.pixelsBGRA[i];uint32_t mixed=0;
        for(unsigned shift:{0u,8u,16u,24u})mixed|=(((src>>shift)&255)+(((dst>>shift)&255)*(255-a)+127)/255)<<shift;
        dst=mixed;
    }
    out.image=std::move(background.image);out.bases=std::move(bases.stats);out.liveRecords=scene.liveRecords;
    out.placedRecords=scene.placed.size()+scene.overlayRecords;out.withheldRecords=scene.liveRecords>=out.placedRecords?scene.liveRecords-out.placedRecords:0;
    out.withheldOutlines=background.valid;out.unshapedMarkers=scene.markers.size();
    out.groundCells=scene.groundCells;out.groundUnknown=scene.groundUnknown;out.groundNoWrite=scene.groundNoWrite;
    out.groundUnresolved=scene.groundUnresolved;out.unboundBuildingCells=scene.unboundBuildingCells;return out;
}
}
