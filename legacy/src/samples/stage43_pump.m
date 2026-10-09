/* Authored queue adapter, preserving stage42 Image ABI. */
#import <AppKit/AppKit.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct{uint32_t magic,version,pending,msg[7];uint64_t polls,events;} Image;
typedef struct{NSWindow*window;Image im;} State;
static int matches(uint32_t m,uint32_t lo,uint32_t hi){return (!lo&&!hi)||(m>=lo&&m<=hi);}
static NSEventMask mask(uint32_t lo,uint32_t hi){if(!lo&&!hi)return NSEventMaskAny;NSEventMask m=0;
 if(matches(0x200,lo,hi))m|=NSEventMaskMouseMoved|NSEventMaskLeftMouseDragged|NSEventMaskRightMouseDragged;
 if(matches(0x201,lo,hi))m|=NSEventMaskLeftMouseDown;if(matches(0x202,lo,hi))m|=NSEventMaskLeftMouseUp;
 if(matches(0x204,lo,hi))m|=NSEventMaskRightMouseDown;if(matches(0x205,lo,hi))m|=NSEventMaskRightMouseUp;
 if(lo<=0x108&&hi>=0x100)m|=NSEventMaskKeyDown|NSEventMaskKeyUp|NSEventMaskFlagsChanged;
 return m;
}
void*s43p_new(void*window){if(!window||!NSThread.isMainThread)return NULL;State*s=calloc(1,sizeof(*s));if(!s)return NULL;s->window=(__bridge NSWindow*)window;s->im.magic=0x53343250;s->im.version=1;return s;}
int s43p_peek(void*p,uint32_t remove,uint32_t lo,uint32_t hi,uint32_t*out,uint32_t*diag){State*s=p;if(!s||!out||!diag||remove>1||!NSThread.isMainThread)return -1;BOOL active=NSApp.isActive,key=s->window.isKeyWindow;diag[0]=diag[1]=0;s->im.polls++;
 if(s->im.pending&&!matches(s->im.msg[1],lo,hi))return -6;
 if(!s->im.pending){NSEventMask requested=mask(lo,hi);if(!requested)return -7;NSEvent*e=[NSApp nextEventMatchingMask:requested untilDate:NSDate.distantPast inMode:NSDefaultRunLoopMode dequeue:YES];if(active!=NSApp.isActive||key!=s->window.isKeyWindow)return -2;if(!e)return 0;diag[0]=(uint32_t)e.type;diag[1]=(uint32_t)e.subtype;
  uint32_t message=0;switch(e.type){case NSEventTypeMouseMoved:case NSEventTypeLeftMouseDragged:case NSEventTypeRightMouseDragged:message=0x200;break;case NSEventTypeLeftMouseDown:message=0x201;break;case NSEventTypeLeftMouseUp:message=0x202;break;case NSEventTypeRightMouseDown:message=0x204;break;case NSEventTypeRightMouseUp:message=0x205;break;default:break;}
  if(!message||e.window!=s->window||!matches(message,lo,hi)){[NSApp postEvent:e atStart:YES];return -5;}
  NSView*v=s->window.contentView;NSPoint q=[v convertPoint:e.locationInWindow fromView:nil];if(!v.isFlipped)q.y=v.bounds.size.height-q.y;NSPoint screen=[s->window convertPointToScreen:e.locationInWindow];CGFloat top=NSMaxY(NSScreen.screens[0].frame);uint32_t buttons=(uint32_t)NSEvent.pressedMouseButtons,wp=(buttons&1?1:0)|(buttons&2?2:0)|(buttons&4?16:0)|(e.modifierFlags&NSEventModifierFlagShift?4:0)|(e.modifierFlags&NSEventModifierFlagControl?8:0);
  if(message==0x201)wp|=1;if(message==0x202)wp&=~1u;if(message==0x204)wp|=2;if(message==0x205)wp&=~2u;
  int32_t px=(int32_t)q.x,py=(int32_t)q.y;s->im.msg[0]=0x58000001;s->im.msg[1]=message;s->im.msg[2]=wp;s->im.msg[3]=(px&65535)|((py&65535)<<16);s->im.msg[4]=(uint32_t)(e.timestamp*1000);s->im.msg[5]=(int32_t)screen.x;s->im.msg[6]=(int32_t)(top-screen.y);s->im.pending=1;s->im.events++;
 }
 memcpy(out,s->im.msg,sizeof(s->im.msg));if(remove)s->im.pending=0;return 1;
}
size_t s43p_size(void){return sizeof(Image);}
int s43p_export(void*p,void*out,size_t n){State*s=p;if(!s||!out||n!=sizeof(Image))return 0;memcpy(out,&s->im,n);return 1;}
int s43p_import(void*p,const void*in,size_t n){State*s=p;if(!s||!in||n!=sizeof(Image))return 0;const Image*i=in;if(i->magic!=0x53343250||i->version!=1||i->pending>1||(i->pending&&i->msg[0]!=0x58000001))return 0;memcpy(&s->im,i,n);return 1;}
void s43p_delete(void*p){free(p);}
