"""Extract current App callbacks; Foundation run loop and fake controls, no GUI."""
from pathlib import Path
import argparse, hashlib, json
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-root',type=Path)
parser.add_argument('--out-dir',type=Path)
args=parser.parse_args()
P=args.out_dir or Path(__file__).resolve().parent
P.mkdir(parents=True,exist_ok=True)
root=args.source_root or P/'source'
source=(root/'game_lab/app/main.mm').read_text()
names=['onSpritePackageChanged:', 'onSpriteSliderChanged:', 'onSpriteIdChanged:',
       'renderSpriteAtIndex:', 'stopSpriteSequence', 'onSpriteSequenceRateChanged:',
       'onSpriteSequenceApply:', 'renderSpriteSequence', 'onSpriteSequenceStep:',
       'onSpriteSequencePlay:']
methods=[]
for name in names:
    start=source.index('- (void)'+name)
    # Include one-line callbacks without swallowing the next implementation.
    at=source.index('{',start);depth=0;quote=None;escape=False
    for end in range(at,len(source)):
        ch=source[end]
        if quote:
            if escape:escape=False
            elif ch=='\\':escape=True
            elif ch==quote:quote=None
            continue
        if ch in ('"',"'"):quote=ch
        elif ch=='{':depth+=1
        elif ch=='}':
            depth-=1
            if not depth:
                end+=1
                break
    methods.append(source[start:end])
prefix=r'''
#import <Foundation/Foundation.h>
#include "game_lab/sg3_sequence.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
static const NSInteger NSControlStateValueOn=1,NSImageScaleNone=0,NSImageScaleProportionallyUpOrDown=1;
@interface FakeControl:NSObject
@property(nonatomic,copy) NSString* stringValue;
@property(nonatomic,copy) NSString* title;
@property(nonatomic,copy) NSString* accessibilityLabel;
@property(nonatomic,copy) NSString* titleOfSelectedItem;
@property(nonatomic,strong) id image;
@property(nonatomic) double doubleValue,maxValue;
@property(nonatomic) NSInteger state,indexOfSelectedItem,imageScaling;
@property(nonatomic) BOOL enabled,hidden;
@end
@implementation FakeControl
@end
static size_t presentations=0;static bool failPresent=false;
static bool presentPixels(FakeControl* view,uint32_t w,uint32_t h,const std::vector<uint32_t>& pixels){
 if(failPresent||!w||!h||pixels.size()!=size_t(w)*h)return false;
 view.image=@(++presentations);return true;
}
struct FakeMedia{
 std::string path;game_lab::SG3Archive archive;
 std::string resourcePath(const std::string&)const{return path;}
 game_lab::SG3Archive sg3(const std::string&)const{return archive;}
 game_lab::SG3DecodeResult sprite(const game_lab::SG3Archive&a,size_t i,const std::string&)const{auto p=std::filesystem::path(path);p.replace_extension(".555");return game_lab::decodeSG3Sprite(a,i,p.string());}
};
@interface EmperorAppDelegate:NSObject{@public
 FakeControl *_spriteSequenceIds,*_spriteSequenceRate,*_spriteSequencePrevious,*_spriteSequenceNext,*_spriteSequencePlay,*_spriteSequenceAlign,*_spriteInfoLabel,*_spriteImageView,*_spriteSlider,*_spriteIdField,*_spritePackagePopup,*_spriteIdApply,*_spriteBaseCanvas;
 FakeControl* _originalManualAdopt;BOOL _spriteExplicitSelection;std::optional<size_t> _spriteStaticIndex,_originalSelectedSlot;game_lab::DecodedImage _spriteStaticPixels;
 NSTimer* _spriteSequenceTimer;game_lab::SG3Sequence _spriteSequence;size_t _spriteSequenceFrame;
 game_lab::SG3Archive _currentSG3;std::vector<std::string> _sg3Packages;std::string _current555Path;FakeMedia* _media;
}
'''
declarations='\n'.join(m[:m.index('{')].strip()+';' for m in methods)
suffix=r'''
static size_t checks=0;
static void need(bool v,const char*m){++checks;if(!v)throw std::runtime_error(m);}
static void run(double seconds){NSDate* end=[NSDate dateWithTimeIntervalSinceNow:seconds];while([end timeIntervalSinceNow]>0)[NSRunLoop.mainRunLoop runMode:NSDefaultRunLoopMode beforeDate:end];}
static void apply(EmperorAppDelegate*d,NSString* ids){d->_spriteSequenceIds.stringValue=ids;[d onSpriteSequenceApply:nil];}
int main(int argc,char**argv){@autoreleasepool{try{
 if(argc!=2)return 2;
 auto path=std::filesystem::path(argv[1])/"headless.555";{std::ofstream f(path,std::ios::binary);const char bytes[]={0,0x7c,char(0xe0),3,0x1f,0};f.write(bytes,6);}
 FakeMedia media;media.path=(std::filesystem::path(argv[1])/"headless.sg3").string();media.archive.version=213;
 game_lab::SG3Sprite r;r.width=1;r.height=1;r.type=1;r.length=2;for(int i=0;i<3;++i){r.offset=i*2;media.archive.sprites.push_back(r);}
 EmperorAppDelegate*d=[EmperorAppDelegate new];
 d->_spriteSequenceIds=[FakeControl new];d->_spriteSequenceRate=[FakeControl new];d->_spriteSequencePrevious=[FakeControl new];d->_spriteSequenceNext=[FakeControl new];d->_spriteSequencePlay=[FakeControl new];d->_spriteSequenceAlign=[FakeControl new];d->_spriteInfoLabel=[FakeControl new];d->_spriteImageView=[FakeControl new];d->_spriteSlider=[FakeControl new];d->_spriteIdField=[FakeControl new];d->_spritePackagePopup=[FakeControl new];d->_spriteIdApply=[FakeControl new];d->_spriteBaseCanvas=[FakeControl new];
 d->_media=&media;d->_currentSG3=media.archive;d->_sg3Packages={"authored.sg3"};d->_spritePackagePopup.indexOfSelectedItem=0;d->_spritePackagePopup.titleOfSelectedItem=@"authored";d->_spriteSequenceRate.stringValue=@"30";
 apply(d,@"3,1,2");need(d->_spriteSequence.valid()&&d->_spriteSequenceFrame==0&&[d->_spriteIdField.stringValue isEqual:@"3"],"apply publishes first explicit physical ID");
 [d onSpriteSequenceStep:d->_spriteSequencePrevious];need(d->_spriteSequenceFrame==2&&[d->_spriteIdField.stringValue isEqual:@"2"],"previous endpoint wraps");[d onSpriteSequenceStep:d->_spriteSequenceNext];need(d->_spriteSequenceFrame==0,"next endpoint wraps");
 size_t shown=presentations;[d onSpriteSequencePlay:nil];need(d->_spriteSequenceTimer&&[d->_spriteSequencePlay.title isEqual:@"暂停"],"play creates timer");run(.15);need(presentations>=shown+2,"main runloop advances actual callback");[d onSpriteSequencePlay:nil];auto frame=d->_spriteSequenceFrame;shown=presentations;run(.10);need(!d->_spriteSequenceTimer&&d->_spriteSequenceFrame==frame&&presentations==shown,"pause invalidates and stays stable");
 for(NSString* rate in @[@"0",@"31",@"nan",@"inf",@"8x",@""]){d->_spriteSequenceRate.stringValue=rate;[d onSpriteSequencePlay:nil];need(!d->_spriteSequenceTimer,"invalid rate cannot schedule timer");}
 d->_spriteSequenceRate.stringValue=@"30";[d onSpriteSequencePlay:nil];[d onSpriteSequenceStep:d->_spriteSequenceNext];need(!d->_spriteSequenceTimer,"manual stepping pauses existing timer");frame=d->_spriteSequenceFrame;run(.08);need(d->_spriteSequenceFrame==frame,"stepped frame stable");
 [d onSpriteSequencePlay:nil];d->_spriteSequenceRate.stringValue=@"1";[d onSpriteSequenceRateChanged:nil];need(!d->_spriteSequenceTimer&&d->_spriteSequenceFrame==frame,"rate edit pauses without advancing");
 for(NSString* ids in @[@"",@"0",@"1,4",@"1,,2"]){apply(d,@"1,2");d->_spriteSequenceRate.stringValue=@"30";[d onSpriteSequencePlay:nil];apply(d,ids);need(!d->_spriteSequence.valid()&&!d->_spriteSequenceTimer&&!d->_spriteImageView.image&&!d->_spriteSequencePlay.enabled,"invalid reload clears prior timer/image/list");run(.04);need(!d->_spriteImageView.image,"failed reload cannot resurrect old timer image");}
 apply(d,@"1,2,3");[d onSpriteSequencePlay:nil];apply(d,@"2");need(!d->_spriteSequenceTimer&&d->_spriteSequenceFrame==0&&d->_spriteSequence.frames.size()==1,"shorter reload resets playhead and pauses");run(.08);need(d->_spriteSequenceFrame==0,"single reloaded frame stable");
 std::filesystem::remove(path);apply(d,@"1,2");need(!d->_spriteSequence.valid()&&!d->_spriteImageView.image,"missing companion clears old preview");{std::ofstream f(path,std::ios::binary);const char b[]={0,0x7c,char(0xe0),3,0x1f,0};f.write(b,6);}apply(d,@"1,2");need(d->_spriteSequence.valid(),"dependency restored after failed reload");
 [d onSpriteSequencePlay:nil];d->_spriteSlider.doubleValue=2;[d onSpriteSliderChanged:nil];need(!d->_spriteSequenceTimer&&!d->_spriteSequence.valid()&&[d->_spriteIdField.stringValue isEqual:@"3"],"static slider clears list and timer");
 apply(d,@"1,2");[d onSpriteSequencePlay:nil];d->_spriteIdField.stringValue=@"0";[d onSpriteIdChanged:nil];need(!d->_spriteSequenceTimer&&!d->_spriteSequence.valid()&&!d->_spriteImageView.image,"invalid static selection clears sequence");
 apply(d,@"1,2");[d onSpriteSequencePlay:nil];d->_spritePackagePopup.indexOfSelectedItem=-1;[d onSpritePackageChanged:nil];need(!d->_spriteSequenceTimer&&!d->_spriteSequence.valid()&&d->_currentSG3.sprites.empty()&&!d->_spriteImageView.image&&!d->_spriteSequenceIds.stringValue.length,"invalid package switch clears archive timer image IDs");
 d->_spritePackagePopup.indexOfSelectedItem=0;[d onSpritePackageChanged:nil];apply(d,@"1,2");need(d->_spriteSequence.valid()&&d->_spriteSequenceFrame==0,"package restored and reload succeeds");
 [d onSpriteSequencePlay:nil];failPresent=true;[d renderSpriteSequence];need(!d->_spriteSequenceTimer&&!d->_spriteImageView.image,"conversion failure pauses and clears view");failPresent=false;[d stopSpriteSequence];[d stopSpriteSequence];need(!d->_spriteSequenceTimer,"repeated stop safe");
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"extracted_callbacks\":10,\"presentations\":"<<presentations<<",\"gui_operations\":0,\"original_data\":false}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}return 0;}
'''
(P/'headless_callbacks.mm').write_text(prefix+declarations+'\n@end\n@implementation EmperorAppDelegate\n'+'\n'.join(methods)+'\n@end\n'+suffix)
(P/'callback-extraction.json').write_text(json.dumps({'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'callbacks':{n:hashlib.sha256(m.encode()).hexdigest() for n,m in zip(names,methods)},'controls':'Foundation mocks; no AppKit/GUI; actual NSTimer callbacks','scope':'10 current callbacks;manual-candidate cache ivars added to mocks; showTab/termination reviewed statically and from supplied GUI evidence'},indent=2))
