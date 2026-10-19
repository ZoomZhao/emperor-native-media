
#import <Foundation/Foundation.h>
#include "game_lab/original_building_candidate.hpp"
#include "game_lab/sg3_sequence.hpp"
#include <iostream>
static const NSInteger NSControlStateValueOn=1,NSControlStateValueOff=0;
@interface C:NSObject
@property(nonatomic,copy)NSString* stringValue;
@property(nonatomic)NSInteger state,indexOfSelectedItem;
- (void)selectItemAtIndex:(NSInteger)i;
@end
@implementation C
- (void)selectItemAtIndex:(NSInteger)i{self.indexOfSelectedItem=i;}
@end
@interface D:NSObject{@public
 C *_originalBuildingSlot,*_originalMapBitPicker,*_originalMapSpatial,*_originalMapCameraX,*_originalMapCameraY,*_originalManualVisible,*_spritePackagePopup;
 std::optional<game_lab::OriginalBuildingProfile>_originalSaveBuildings;std::optional<size_t>_originalSelectedSlot,_spriteStaticIndex;std::optional<game_lab::OriginalBuildingCandidate>_originalManualCandidate;
 BOOL _spriteExplicitSelection;game_lab::DecodedImage _spriteStaticPixels;game_lab::SG3Sequence _spriteSequence;std::vector<std::string>_sg3Packages;std::string _originalManualMessage,_originalSourceRelative;uint64_t _originalSaveRevision;
}
- (void)onOriginalBuildingLocate:(id)sender;
- (void)onOriginalManualAdopt:(id)sender;
- (void)onOriginalMapPlaneChanged:(id)sender;
@end
@implementation D
- (void)onOriginalMapPlaneChanged:(id)sender{(void)sender;}
- (void)onOriginalBuildingLocate:(id)sender {
    (void)sender;
    if(!_originalSaveBuildings||_originalSaveBuildings->footprints.empty())return;
    NSString* raw=[_originalBuildingSlot.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];const char* value=raw.UTF8String;char* end=nullptr;const auto slot=value?std::strtoull(value,&end,10):4000;
    auto found=std::find_if(_originalSaveBuildings->footprints.begin(),_originalSaveBuildings->footprints.end(),[&](const auto& f){return f.slot==slot;});
    if(!value||!*value||!end||*end||slot>=4000||found==_originalSaveBuildings->footprints.end()){_originalSelectedSlot.reset();_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage="建筑槽不在当前已验证占地记录中；旧人工候选已清除。";[self onOriginalMapPlaneChanged:nil];return;}
    if(!_originalSelectedSlot||*_originalSelectedSlot!=slot){_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;_originalManualMessage.clear();}_originalSelectedSlot=size_t(slot);
    const auto& f=*found;
    [_originalMapBitPicker selectItemAtIndex:0];_originalMapSpatial.state=NSControlStateValueOn;
    _originalMapCameraX.stringValue=[NSString stringWithFormat:@"%.2f",double(f.x)+(double(game_lab::originalFootprintWidth(f))-1)/2];
    _originalMapCameraY.stringValue=[NSString stringWithFormat:@"%.2f",double(f.y)+(double(game_lab::originalFootprintHeight(f))-1)/2];
    [self onOriginalMapPlaneChanged:nil];
}
- (void)onOriginalManualAdopt:(id)sender {
    (void)sender;_originalManualCandidate.reset();_originalManualVisible.state=NSControlStateValueOff;
    [self onOriginalBuildingLocate:nil];
    if(!_originalSaveBuildings||!_originalSelectedSlot||!_spriteExplicitSelection||!_spriteStaticIndex||!_spriteStaticPixels.valid()||!_spriteSequence.frames.empty()){
        _originalManualMessage="先定位已验证的建筑表槽，并在 SG3 页显式选择有效静态图像；旧候选已清除。";[self onOriginalMapPlaneChanged:nil];return;
    }
    auto target=std::find_if(_originalSaveBuildings->footprints.begin(),_originalSaveBuildings->footprints.end(),[&](const auto& f){return f.slot==*_originalSelectedSlot;});
    const NSInteger package=_spritePackagePopup.indexOfSelectedItem;if(target==_originalSaveBuildings->footprints.end()||package<0||size_t(package)>=_sg3Packages.size())return;
    game_lab::OriginalBuildingCandidate c;c.source=_originalSourceRelative;c.sourceRevision=_originalSaveRevision;c.target=*target;c.archive=_sg3Packages[size_t(package)];c.physicalId=uint32_t(*_spriteStaticIndex+1);c.pixels=_spriteStaticPixels;c.imageRevision=game_lab::originalCandidateImageRevision(c.pixels);
    if(!game_lab::validOriginalCandidate(c)){_originalManualMessage="图素候选超出画布预算或像素校验失败，未显示旧候选。";[self onOriginalMapPlaneChanged:nil];return;}
    _originalManualCandidate=std::move(c);_originalManualVisible.state=NSControlStateValueOn;_originalManualMessage="人工候选 · 锚点未验 · confirmed 0。图像底边中心对齐占地中心仅为预览约定。";[self onOriginalMapPlaneChanged:nil];
}
@end
int main(){@autoreleasepool{
 D*d=[D new];d->_originalBuildingSlot=[C new];d->_originalMapBitPicker=[C new];d->_originalMapSpatial=[C new];d->_originalMapCameraX=[C new];d->_originalMapCameraY=[C new];d->_originalManualVisible=[C new];d->_spritePackagePopup=[C new];
 game_lab::OriginalBuildingProfile b;game_lab::OriginalBuildingFootprint f;f.type=72;f.x=7;f.y=9;f.size=2;f.slot=1;f.recordOffset=100;b.footprints.push_back(f);d->_originalSaveBuildings=b;d->_originalBuildingSlot.stringValue=@"1";d->_spriteExplicitSelection=YES;d->_spriteStaticIndex=0;d->_spriteStaticPixels.width=1;d->_spriteStaticPixels.height=1;d->_spriteStaticPixels.pixelsBGRA={0xffff0000};d->_sg3Packages={"DATA/authored.sg3"};d->_originalSourceRelative="Save/authored.sav";d->_originalSaveRevision=77;
 [d onOriginalManualAdopt:nil];if(!d->_originalManualCandidate)return 2;
 d->_originalBuildingSlot.stringValue=@"-18446744073709551615";[d onOriginalBuildingLocate:nil];
 bool cleared=!d->_originalSelectedSlot&&!d->_originalManualCandidate&&d->_originalManualVisible.state==0;
 std::cout<<"{\"invalid_negative_slot\":\"-18446744073709551615\",\"selected_slot\":"<<(d->_originalSelectedSlot?long(*d->_originalSelectedSlot):-1)<<",\"candidate_still_present\":"<<(d->_originalManualCandidate?"true":"false")<<",\"visible_state\":"<<d->_originalManualVisible.state<<",\"expected_cleared\":true,\"passed\":"<<(cleared?"true":"false")<<"}\n";return cleared?0:1;
}}
