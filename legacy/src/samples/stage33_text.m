/* Task-authored finite ASCII GDI adapter using installed CoreText. */
#import <Foundation/Foundation.h>
#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
static CTLineRef line(const unsigned char*bytes,size_t n,int variant,CTFontRef*font){
 if(!bytes||!n||n>1024)return NULL;for(size_t i=0;i<n;i++)if(bytes[i]<32||bytes[i]>126)return NULL;
 *font=variant?CTFontCreateWithName(CFSTR("Menlo-Regular"),13,NULL):CTFontCreateUIFontForLanguage(kCTFontUIFontSystem,13,CFSTR("en"));if(!*font)return NULL;
 CFStringRef text=CFStringCreateWithBytes(NULL,bytes,n,kCFStringEncodingASCII,false);const void*keys[]={kCTFontAttributeName,kCTForegroundColorFromContextAttributeName};const void*values[]={*font,kCFBooleanTrue};CFDictionaryRef attrs=CFDictionaryCreate(NULL,keys,values,2,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);CFAttributedStringRef str=CFAttributedStringCreate(NULL,text,attrs);CTLineRef result=CTLineCreateWithAttributedString(str);CFRelease(str);CFRelease(attrs);CFRelease(text);return result;
}
static void metrics(CTLineRef l,CTFontRef f,double*out){double ascent=0,descent=0,leading=0,advance=CTLineGetTypographicBounds(l,&ascent,&descent,&leading);CGRect ink=CTLineGetBoundsWithOptions(l,kCTLineBoundsUseGlyphPathBounds);double a[10]={ceil(advance),ceil(CTFontGetAscent(f)+CTFontGetDescent(f)),CTFontGetAscent(f),CTFontGetDescent(f),CTFontGetLeading(f),advance,ink.origin.x,ink.origin.y,ink.size.width,ink.size.height};memcpy(out,a,sizeof(a));}
int s33_measure(const unsigned char*bytes,size_t n,uint32_t variant,double*out,char*name,size_t capacity){@autoreleasepool{
 if(variant>1||!out||!name||capacity<256)return 0;CTFontRef f=NULL;CTLineRef l=line(bytes,n,variant,&f);if(!l)return 0;metrics(l,f,out);CFStringRef ps=CTFontCopyPostScriptName(f);Boolean okay=CFStringGetCString(ps,name,capacity,kCFStringEncodingUTF8);CFRelease(ps);CFRelease(l);CFRelease(f);return okay;
}}
int s33_draw(unsigned char*pixels,uint32_t w,uint32_t h,uint32_t pitch,int32_t x,int32_t y,const unsigned char*bytes,size_t n,uint32_t bk,uint32_t text,const int32_t*clip,int32_t*out){@autoreleasepool{
 if(!pixels||!w||!h||w>2048||h>2048||pitch!=w*4||!clip||!out||bk>0xffffff||text>0xffffff)return 0;CTFontRef f=NULL;CTLineRef l=line(bytes,n,0,&f);if(!l)return 0;double m[10];metrics(l,f,m);
 CGColorSpaceRef cs=CGColorSpaceCreateDeviceRGB();CGContextRef c=CGBitmapContextCreate(pixels,w,h,8,pitch,cs,kCGImageAlphaNoneSkipFirst|kCGBitmapByteOrder32Little);CGColorSpaceRelease(cs);if(!c){CFRelease(l);CFRelease(f);return 0;}
 /* Quartz bitmap coordinates are bottom-up, guest coordinates top-down.
  * Flip the graphics CTM; flip glyphs around the TA_TOP baseline separately. */
 CGContextTranslateCTM(c,0,h);CGContextScaleCTM(c,1,-1);
 CGContextClipToRect(c,CGRectMake(clip[0],clip[1],clip[2]-clip[0],clip[3]-clip[1]));CGContextSetRGBFillColor(c,(bk&255)/255.,((bk>>8)&255)/255.,((bk>>16)&255)/255.,1);CGContextFillRect(c,CGRectMake(x,y,m[0],m[1]));CGContextSetRGBFillColor(c,(text&255)/255.,((text>>8)&255)/255.,((text>>16)&255)/255.,1);CGContextSetTextMatrix(c,CGAffineTransformMakeScale(1,-1));CGContextSetTextPosition(c,x,y+m[2]);CTLineDraw(l,c);CGContextRelease(c);CFRelease(l);CFRelease(f);
 int32_t a[6]={MAX(x,clip[0]),MAX(y,clip[1]),MIN(x+(int)m[0],clip[2]),MIN(y+(int)m[1],clip[3]),(int)ceil(y+m[2]),(int)ceil(m[2])};memcpy(out,a,sizeof(a));return 1;
}}
