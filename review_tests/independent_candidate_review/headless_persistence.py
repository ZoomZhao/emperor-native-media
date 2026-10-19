"""Build actual five App callbacks with Foundation mocks; never starts AppKit."""
import hashlib,json
from pathlib import Path

NAMES=['onOriginalBuildingLocate:','onOriginalManualAdopt:','onOriginalManualClear:','onOriginalManualSave:','onOriginalManualLoad:']
def extract(source,name):
    start=source.index('- (void)'+name);at=source.index('{',start)
    depth=0;quote=None;escape=False
    for end in range(at,len(source)):
        c=source[end]
        if quote:
            if escape:escape=False
            elif c=='\\':escape=True
            elif c==quote:quote=None
        elif c in ('"',"'"):quote=c
        elif c=='{':depth+=1
        elif c=='}':
            depth-=1
            if not depth:return source[start:end+1]
    raise ValueError(name)

def generate(root,out):
    source=(root/'game_lab/app/main.mm').read_text();methods=[extract(source,n)for n in NAMES]
    original=Path(__file__).with_name('negative_slot_mre.mm').read_text()
    prefix=original[:original.index('@implementation D')]
    prefix=prefix.replace('#include "game_lab/sg3_sequence.hpp"','#include "game_lab/sg3_sequence.hpp"\n#include "game_lab/app/building_candidate_config.hpp"\n#include <fstream>\n#include <stdexcept>')
    panel=r'''
static NSURL* selectedURL;static NSInteger selectedResponse=1;
static const NSInteger NSModalResponseOK=1;
@interface NSSavePanel:NSObject
@property(nonatomic,copy)NSString*nameFieldStringValue,*message;
@property(nonatomic,strong)NSURL*URL;
+(instancetype)savePanel;
-(NSInteger)runModal;
@end
@implementation NSSavePanel
+(instancetype)savePanel{NSSavePanel*p=[self new];p.URL=selectedURL;return p;}
-(NSInteger)runModal{return selectedResponse;}
@end
@interface NSOpenPanel:NSSavePanel
@property(nonatomic)BOOL canChooseFiles,canChooseDirectories,allowsMultipleSelection;
+(instancetype)openPanel;
@end
@implementation NSOpenPanel
+(instancetype)openPanel{return [self savePanel];}
@end
struct FakeMedia{
 bool missing=false;game_lab::DecodedImage pixels;size_t decodes=0;
 game_lab::SG3Archive sg3(const std::string&){game_lab::SG3Archive a;if(!missing)a.sprites.resize(1);return a;}
 game_lab::SG3DecodeResult sprite(const game_lab::SG3Archive&,size_t,const std::string&){++decodes;game_lab::SG3DecodeResult r;r.image=pixels;return r;}
};
'''
    prefix=prefix.replace('@interface D:NSObject',panel+'\n@interface D:NSObject')
    prefix=prefix.replace('C *_originalBuildingSlot,','NSURL*_candidateDirectory;FakeMedia*_media;C*_originalMapInfo;\n C *_originalBuildingSlot,')
    declarations='\n'.join(m[:m.index('{')].strip()+';'for m in methods)
    body=prefix+declarations+'\n@implementation D\n- (void)onOriginalMapPlaneChanged:(id)sender{(void)sender;}\n'+'\n'.join(methods)+'\n@end\n'+SUFFIX
    # Declarations must be inside the existing interface before its @end.
    body=body.replace('@end\n'+declarations+'\n@implementation D',declarations+'\n@end\n@implementation D')
    (out/'persistence_callbacks.mm').write_text(body)
    (out/'persistence-extraction.json').write_text(json.dumps({'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'callbacks':{n:hashlib.sha256(m.encode()).hexdigest()for n,m in zip(NAMES,methods)},'panel':'Foundation mock; accepted/cancelled response and URL injection; no AppKit or native panel validation','plane_callback':'stub; rendering tested separately','GUI_operations':0},indent=2))

SUFFIX=r'''
static size_t checks=0;
static void need(bool value,const char*message){++checks;if(!value)throw std::runtime_error(message);}
int main(int argc,char**argv){@autoreleasepool{try{
 if(argc!=2)return 2;std::filesystem::path temp(argv[1]);auto data=temp/"authored-data",repo=temp/"authored-repo",priv=temp/"private";
 std::filesystem::create_directories(data);std::filesystem::create_directories(repo/".git");std::filesystem::create_directories(priv);
 D*d=[D new];d->_originalBuildingSlot=[C new];d->_originalMapBitPicker=[C new];d->_originalMapSpatial=[C new];d->_originalMapCameraX=[C new];d->_originalMapCameraY=[C new];d->_originalManualVisible=[C new];d->_spritePackagePopup=[C new];d->_originalMapInfo=[C new];d->_candidateDirectory=[NSURL fileURLWithPath:[NSString stringWithUTF8String:data.c_str()]];
 game_lab::OriginalBuildingProfile b;game_lab::OriginalBuildingFootprint f;f.type=72;f.x=7;f.y=9;f.size=2;f.slot=1;f.recordOffset=100;b.footprints.push_back(f);auto second=f;second.slot=2;second.x=12;b.footprints.push_back(second);d->_originalSaveBuildings=b;
 d->_spriteExplicitSelection=YES;d->_spriteStaticIndex=0;d->_spriteStaticPixels.width=1;d->_spriteStaticPixels.height=1;d->_spriteStaticPixels.pixelsBGRA={0xffff0000};d->_sg3Packages={"DATA/authored.sg3"};d->_originalSourceRelative="Save/authored.sav";d->_originalSaveRevision=77;FakeMedia media;media.pixels=d->_spriteStaticPixels;d->_media=&media;
 const auto adopt=[&](){d->_originalBuildingSlot.stringValue=@"1";d->_spriteExplicitSelection=YES;d->_spriteStaticIndex=0;[d onOriginalManualAdopt:nil];need(d->_originalManualCandidate&&d->_originalManualVisible.state==1,"explicit adoption succeeds");};
 const auto cleared=[&](){need(!d->_originalManualCandidate&&d->_originalManualVisible.state==0,"bad load orclear removes old candidate");};
 adopt();[d onOriginalManualClear:nil];cleared();[d onOriginalManualClear:nil];cleared();
 adopt();d->_originalBuildingSlot.stringValue=@"2";[d onOriginalBuildingLocate:nil];cleared();need(d->_originalSelectedSlot==2,"slot switch selects new record");
 for(NSString*value in @[@"",@"-1",@"4000",@"1junk",@"999999999999999999999999999999999999"]){adopt();d->_originalBuildingSlot.stringValue=value;[d onOriginalBuildingLocate:nil];cleared();need(!d->_originalSelectedSlot,"ordinary bad slot resets selection");}
 adopt();d->_spriteExplicitSelection=NO;[d onOriginalManualAdopt:nil];cleared();adopt();d->_spriteStaticIndex.reset();[d onOriginalManualAdopt:nil];cleared();
 adopt();auto file=priv/"candidate.json";selectedURL=[NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]];[d onOriginalManualSave:nil];need(std::filesystem::exists(file),"actual save callback writes metadata");
 NSData* saved=[NSData dataWithContentsOfURL:selectedURL];NSDictionary* good=[NSJSONSerialization JSONObjectWithData:saved options:0 error:nil];need(good.count==16&&!good[@"pixels"]&&!good[@"confirmed"],"saved callback has exactly reference keys");
 [d onOriginalManualClear:nil];[d onOriginalManualLoad:nil];need(d->_originalManualCandidate&&d->_originalManualCandidate->pixels.pixelsBGRA==media.pixels.pixelsBGRA&&d->_originalManualVisible.state==1,"actual load callback redecodes original pixels");need(media.decodes==1,"load redecodes rather than loading pixel payload");
 selectedResponse=0;[d onOriginalManualLoad:nil];need(d->_originalManualCandidate.has_value(),"cancel preserves oldcandidate");selectedResponse=1;
 const auto write=[&](id object){NSData*v=[NSJSONSerialization dataWithJSONObject:object options:0 error:nil];need([v writeToURL:selectedURL atomically:YES],"own fixture written");};
 for(NSString*key in @[@"source",@"sourceRevision",@"archive",@"physicalId",@"imageRevision",@"slot",@"recordOffset",@"type",@"x",@"y",@"size",@"width",@"height",@"status"]){
  adopt();NSMutableDictionary*bad=[good mutableCopy];if([key isEqual:@"source"])bad[key]=@"Save/other.sav";else if([key isEqual:@"sourceRevision"]||[key isEqual:@"imageRevision"])bad[key]=@"0000000000000000";else if([key isEqual:@"archive"])bad[key]=@"../escape.sg3";else if([key isEqual:@"status"])bad[key]=@"confirmed";else bad[key]=@([good[key]unsignedLongLongValue]+1);write(bad);[d onOriginalManualLoad:nil];cleared();
 }
 write(good);adopt();media.missing=true;[d onOriginalManualLoad:nil];cleared();media.missing=false;
 adopt();media.pixels.pixelsBGRA[0]=0xff00ff00;[d onOriginalManualLoad:nil];cleared();media.pixels=d->_spriteStaticPixels;
 adopt();d->_originalSaveRevision=78;[d onOriginalManualLoad:nil];cleared();d->_originalSaveRevision=77;
 adopt();d->_originalSourceRelative="Save/other.sav";[d onOriginalManualLoad:nil];cleared();d->_originalSourceRelative="Save/authored.sav";
 adopt();std::filesystem::remove(file);[d onOriginalManualLoad:nil];cleared();
 adopt();{std::ofstream f(file);f<<"{";}[d onOriginalManualLoad:nil];cleared();
 adopt();{std::ofstream f(file);f.seekp(16384);f<<' ';}auto calls=media.decodes;[d onOriginalManualLoad:nil];cleared();need(media.decodes==calls,"oversize regular config rejected before assets decode");
 for(auto forbidden:{data/"blocked.json",repo/"blocked.json"}){adopt();selectedURL=[NSURL fileURLWithPath:[NSString stringWithUTF8String:forbidden.c_str()]];[d onOriginalManualSave:nil];need(!std::filesystem::exists(forbidden),"actual save refuses GameData/Git");[d onOriginalManualLoad:nil];cleared();}
 selectedURL=[NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]];write(good);[d onOriginalManualLoad:nil];need(d->_originalManualCandidate&&d->_originalManualVisible.state==1,"valid reference recovers after failures");
 std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"actual_callbacks\":5,\"native_panels\":false,\"GUI_operations\":0,\"confirmed\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}return 0;}
'''
