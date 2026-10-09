/* Authored actual AppKit PeekMessage source for one owned background window.
 * Empty results come from polling the actual queue; unknown events refuse.
 */
#import <AppKit/AppKit.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint32_t magic,version,pending,msg[7];uint64_t polls,events;} Image;
typedef struct {NSWindow*window;Image im;} State;
void*s42p_new(void*window){if(!window||!NSThread.isMainThread)return NULL;State*s=calloc(1,sizeof(*s));if(!s)return NULL;s->window=(__bridge NSWindow*)window;s->im.magic=0x53343250;s->im.version=1;return s;}
int s42p_peek(void*p,uint32_t remove,uint32_t*out,uint32_t*diag){State*s=p;if(!s||!out||!diag||remove>1||!NSThread.isMainThread)return -1;BOOL active=NSApp.isActive,key=s->window.isKeyWindow;diag[0]=diag[1]=0;s->im.polls++;
 if(!s->im.pending){NSEvent*event=[NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];if(active!=NSApp.isActive||key!=s->window.isKeyWindow)return -2;if(!event)return 0;diag[0]=(uint32_t)event.type;diag[1]=(uint32_t)event.subtype;
  uint32_t msg=0,wp=0;switch(event.type){case NSEventTypeMouseMoved:case NSEventTypeLeftMouseDragged:case NSEventTypeRightMouseDragged:msg=0x200;break;case NSEventTypeLeftMouseDown:msg=0x201;wp=1;break;case NSEventTypeLeftMouseUp:msg=0x202;break;case NSEventTypeRightMouseDown:msg=0x204;wp=2;break;case NSEventTypeRightMouseUp:msg=0x205;break;default:break;}
  if(!msg||event.window!=s->window){[NSApp postEvent:event atStart:YES];return -5;}
  NSPoint point=event.locationInWindow;NSRect content=[s->window contentRectForFrameRect:s->window.frame];int32_t px=(int32_t)point.x,py=(int32_t)(content.size.height-point.y);NSPoint screen=[s->window convertPointToScreen:point];CGFloat top=NSMaxY(s->window.screen.frame);s->im.msg[0]=0x58000001;s->im.msg[1]=msg;s->im.msg[2]=wp;s->im.msg[3]=(px&65535)|((py&65535)<<16);s->im.msg[4]=(uint32_t)(event.timestamp*1000);s->im.msg[5]=(int32_t)screen.x;s->im.msg[6]=(int32_t)(top-screen.y);s->im.pending=1;s->im.events++;
 }
 memcpy(out,s->im.msg,sizeof(s->im.msg));if(remove)s->im.pending=0;return 1;
}
size_t s42p_size(void){return sizeof(Image);}
int s42p_export(void*p,void*out,size_t n){State*s=p;if(!s||!out||n!=sizeof(Image))return 0;memcpy(out,&s->im,n);return 1;}
int s42p_import(void*p,const void*in,size_t n){State*s=p;if(!s||!in||n!=sizeof(Image))return 0;const Image*i=in;if(i->magic!=0x53343250||i->version!=1||i->pending>1||(i->pending&&i->msg[0]!=0x58000001))return 0;memcpy(&s->im,i,n);return 1;}
void s42p_delete(void*p){free(p);}
