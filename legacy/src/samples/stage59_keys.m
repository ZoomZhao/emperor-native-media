/* Own window/event-state experiment. No original code, global keys or text reads. */
#import <AppKit/AppKit.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define KEYMASK ((1u<<20)-1)
typedef struct {
    uint32_t magic, version, down, pending, known, epoch, reserved[2];
    uint64_t events, queries;
} Image;
_Static_assert(sizeof(Image)==48,"fixed own snapshot ABI");

@class S59Owner;
@interface S59Panel:NSPanel @end
@implementation S59Panel
- (BOOL)canBecomeKeyWindow{return NO;}
- (BOOL)canBecomeMainWindow{return NO;}
@end
@interface S59View:NSView
@property(nonatomic,weak) S59Owner *owner;
@end
@interface S59Owner:NSObject {
@public
    Image im;
    NSLock *lock;
    S59Panel *panel;
    S59View *view;
    uint64_t ignored, foreign, delivered, fixture_events;
}
- (void)observe:(NSEvent*)event;
@end

/* Apple HIToolbox Events.h hardware key codes, indexed by Windows F1..F20. */
static const uint16_t codes[20]={0x7a,0x78,0x63,0x76,0x60,0x61,0x62,0x64,0x65,0x6d,
                              0x67,0x6f,0x69,0x6b,0x71,0x6a,0x40,0x4f,0x50,0x5a};
static int key_index(uint16_t code){for(int i=0;i<20;i++)if(codes[i]==code)return i;return -1;}

@implementation S59View
- (BOOL)acceptsFirstResponder{return YES;}
- (void)keyDown:(NSEvent*)event{[self.owner observe:event];}
- (void)keyUp:(NSEvent*)event{[self.owner observe:event];}
@end
@implementation S59Owner
- (void)observe:(NSEvent*)event{
    [lock lock];
    delivered++;
    if(event.window!=panel){foreign++;[lock unlock];return;}
    int index=key_index(event.keyCode);
    if(index<0||(event.type!=NSEventTypeKeyDown&&event.type!=NSEventTypeKeyUp)){
        ignored++;[lock unlock];return;
    }
    uint32_t bit=1u<<index;
    if(event.type==NSEventTypeKeyDown){
        if(!event.isARepeat&&(!(im.known&bit)||!(im.down&bit)))im.pending|=bit;
        im.down|=bit;
    }else im.down&=~bit;
    im.known|=bit;
    im.events++;
    [lock unlock];
}
@end

static S59Owner*owner(void*p){return (__bridge S59Owner*)p;}
void*s59_new(void){
    if(!NSThread.isMainThread)return NULL;
    [NSApplication sharedApplication];
    S59Owner *s=[S59Owner new];
    s->im=(Image){.magic=0x5335394b,.version=1,.epoch=1};
    s->lock=[NSLock new];
    s->panel=[[S59Panel alloc]initWithContentRect:NSMakeRect(24,24,320,100)
                  styleMask:NSWindowStyleMaskBorderless|NSWindowStyleMaskNonactivatingPanel
                  backing:NSBackingStoreBuffered defer:NO];
    s->panel.releasedWhenClosed=NO;
    s->panel.hidesOnDeactivate=NO;
    s->panel.floatingPanel=NO;
    s->view=[[S59View alloc]initWithFrame:NSMakeRect(0,0,320,100)];
    s->view.owner=s;
    s->panel.contentView=s->view;
    if(![s->panel makeFirstResponder:s->view])return NULL;
    [s->panel orderFront:nil];
    if(NSApp.isActive||s->panel.isKeyWindow)return NULL;
    return (__bridge_retained void*)s;
}
int s59_diag(void*p,uint64_t*out){
    if(!p||!out||!NSThread.isMainThread)return -1;
    S59Owner*s=owner(p);
    [s->lock lock];
    uint64_t a[]={NSThread.isMainThread,NSApp.isActive,s->panel.isKeyWindow,s->panel.canBecomeKeyWindow,
                  s->panel.isVisible,(uint64_t)s->panel.windowNumber,getpid(),
                  (uint64_t)NSWorkspace.sharedWorkspace.frontmostApplication.processIdentifier,
                  s->delivered,s->im.events,s->ignored,s->foreign,s->fixture_events,
                  s->im.down,s->im.pending,s->im.known};
    memcpy(out,a,sizeof(a));[s->lock unlock];return 0;
}
int s59_fixture_event(void*p,uint32_t key_code,uint32_t down,uint32_t repeat,uint32_t other_window){
    if(!p||!NSThread.isMainThread||key_code>127||down>1||repeat>1||other_window>1)return -1;
    S59Owner*s=owner(p);
    if(NSApp.isActive||s->panel.isKeyWindow)return -2;
    NSPanel*other=nil;
    NSInteger target=s->panel.windowNumber;
    if(other_window){
        other=[[S59Panel alloc]initWithContentRect:NSMakeRect(0,0,8,8)
           styleMask:NSWindowStyleMaskBorderless|NSWindowStyleMaskNonactivatingPanel
           backing:NSBackingStoreBuffered defer:NO];
        other.releasedWhenClosed=NO;target=other.windowNumber;
    }
    /* Empty fixture strings only; the event handler never reads text properties. */
    NSEvent*e=[NSEvent keyEventWithType:down?NSEventTypeKeyDown:NSEventTypeKeyUp
        location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
        windowNumber:target context:nil characters:@"" charactersIgnoringModifiers:@""
        isARepeat:repeat keyCode:key_code];
    if(!e)return -3;
    s->fixture_events++;
    [s->panel sendEvent:e];
    if(other)[other close];
    return NSApp.isActive||s->panel.isKeyWindow?-4:0;
}
/* This is deliberately named for the local event domain, not Windows hardware. */
int s59_query_event_state(void*p,uint32_t vk,uint32_t*out){
    if(!p||!out)return -1;
    if(vk<0x70||vk>0x83)return -10;
    S59Owner*s=owner(p);uint32_t bit=1u<<(vk-0x70);
    [s->lock lock];
    if(!(s->im.known&bit)){[s->lock unlock];return -11;}
    *out=(s->im.down&bit?0x8000u:0)|(s->im.pending&bit?1u:0);
    s->im.pending&=~bit;s->im.queries++;
    [s->lock unlock];return 0;
}
/* Current constraints provide no global asynchronous hardware-state provider. */
int s59_query_original_scope(void*p,uint32_t vk,uint32_t*out){
    if(!p||!out||!NSThread.isMainThread)return -1;
    if(vk<0x70||vk>0x83)return -10;
    S59Owner*s=owner(p);
    if(!NSApp.isActive||!s->panel.isKeyWindow)return -20;
    return -21; /* local delivered events cannot prove system async equivalence */
}
size_t s59_size(void){return sizeof(Image);}
int s59_export(void*p,void*out,size_t n){
    if(!p||!out||n!=sizeof(Image))return -1;
    S59Owner*s=owner(p);[s->lock lock];memcpy(out,&s->im,n);[s->lock unlock];return 0;
}
int s59_import(void*p,const void*input,size_t n,uint32_t replay){
    if(!p||!input||n!=sizeof(Image)||replay>1)return -1;
    Image im;memcpy(&im,input,n);
    if(im.magic!=0x5335394b||im.version!=1||!im.epoch||im.reserved[0]||im.reserved[1]
       ||((im.down|im.pending|im.known)&~KEYMASK)||(im.down&~im.known)||(im.pending&~im.known))return -12;
    /* Live host input cannot be rewound. Unknown is not represented as key-up. */
    if(!replay){if(im.epoch==UINT32_MAX)return -13;im.epoch++;im.known=im.down=im.pending=0;}
    S59Owner*s=owner(p);[s->lock lock];s->im=im;[s->lock unlock];return 0;
}
void s59_delete(void*p){
    if(!p||!NSThread.isMainThread)return;
    S59Owner*s=(__bridge_transfer S59Owner*)p;
    [s->panel close];s->view.owner=nil;s->panel.contentView=nil;s->view=nil;s->panel=nil;
}
