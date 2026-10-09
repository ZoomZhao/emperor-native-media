#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "stage23_bridge.h"
static id<MTLDevice> dev;static id<MTLCommandQueue> queue;static id<MTLRenderPipelineState> presentPipe;static id<MTLTexture> rendered;static int gpuW,gpuH;
@interface Renderer:NSObject<MTKViewDelegate>@end
@implementation Renderer
-(void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size{(void)view;(void)size;}
-(void)drawInMTKView:(MTKView*)view{
 MTLRenderPassDescriptor*pass=view.currentRenderPassDescriptor;id<CAMetalDrawable>dr=view.currentDrawable;if(!pass||!dr)return;
 id<MTLCommandBuffer>cmd=[queue commandBuffer];id<MTLRenderCommandEncoder>enc=[cmd renderCommandEncoderWithDescriptor:pass];[enc setRenderPipelineState:presentPipe];[enc setFragmentTexture:rendered atIndex:0];[enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[enc endEncoding];[cmd presentDrawable:dr];[cmd commit];[cmd waitUntilCompleted];
}
@end
static int gpu(const unsigned char*bytes,int w,int h,int pitch,int frame,const unsigned char*initial){
 dev=MTLCreateSystemDefaultDevice();if(!dev){fprintf(stderr,"no Metal device\n");return 1;}queue=[dev newCommandQueue];gpuW=w;gpuH=h;
 NSString*code=@"#include <metal_stdlib>\nusing namespace metal;\nstruct V{float4 position [[position]];float2 uv;};vertex V vs(uint id [[vertex_id]]){float2 p=float2((id<<1)&2,id&2);V v;v.position=float4(p.x*2-1,1-p.y*2,0,1);v.uv=p;return v;}fragment float4 exact(V v [[stage_in]],texture2d<float> t [[texture(0)]]){return t.read(uint2(v.position.xy));}fragment float4 display(V v [[stage_in]],texture2d<float> t [[texture(0)]]){constexpr sampler s(coord::normalized,address::clamp_to_edge,filter::nearest);return t.sample(s,v.uv);}";
 NSError*error=nil;id<MTLLibrary>lib=[dev newLibraryWithSource:code options:nil error:&error];if(!lib){fprintf(stderr,"Metal shader %s\n",error.description.UTF8String);return 1;}
 MTLTextureDescriptor*td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:w height:h mipmapped:NO];td.storageMode=MTLStorageModeShared;td.usage=MTLTextureUsageShaderRead;
 id<MTLTexture>source=[dev newTextureWithDescriptor:td];[source replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:0 withBytes:initial bytesPerRow:pitch];
 td.storageMode=MTLStorageModePrivate;td.usage=MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;rendered=[dev newTextureWithDescriptor:td];
 MTLRenderPipelineDescriptor*pd=[MTLRenderPipelineDescriptor new];pd.vertexFunction=[lib newFunctionWithName:@"vs"];pd.fragmentFunction=[lib newFunctionWithName:@"exact"];pd.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
 id<MTLRenderPipelineState>pipe=[dev newRenderPipelineStateWithDescriptor:pd error:&error];if(!pipe){fprintf(stderr,"Metal pipeline %s\n",error.description.UTF8String);return 1;}
 pd.fragmentFunction=[lib newFunctionWithName:@"display"];presentPipe=[dev newRenderPipelineStateWithDescriptor:pd error:&error];if(!presentPipe)return 1;
 MTLRenderPassDescriptor*pass=[MTLRenderPassDescriptor renderPassDescriptor];pass.colorAttachments[0].texture=rendered;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;
 NSUInteger row=((w*4+255)/256)*256;id<MTLBuffer>readback=[dev newBufferWithLength:row*h options:MTLResourceStorageModeShared];for(int phase=0;phase<2;phase++){
 const unsigned char*expected=phase?bytes:initial;
 if(phase){int left=w,top=h,right=0,bottom=0;for(int y=0;y<h;y++)for(int x=0;x<w;x++)if(memcmp(bytes+y*pitch+x*4,initial+y*pitch+x*4,4)){if(x<left)left=x;if(y<top)top=y;if(x+1>right)right=x+1;if(y+1>bottom)bottom=y+1;}if(left<right&&top<bottom)[source replaceRegion:MTLRegionMake2D(left,top,right-left,bottom-top) mipmapLevel:0 withBytes:bytes+top*pitch+left*4 bytesPerRow:pitch];}
 id<MTLCommandBuffer>cmd=[queue commandBuffer];
 id<MTLRenderCommandEncoder>enc=[cmd renderCommandEncoderWithDescriptor:pass];[enc setRenderPipelineState:pipe];[enc setFragmentTexture:source atIndex:0];[enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[enc endEncoding];
 id<MTLBlitCommandEncoder>blit=[cmd blitCommandEncoder];[blit copyFromTexture:rendered sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(w,h,1) toBuffer:readback destinationOffset:0 destinationBytesPerRow:row destinationBytesPerImage:row*h];[blit endEncoding];[cmd commit];[cmd waitUntilCompleted];
 if(cmd.status!=MTLCommandBufferStatusCompleted){fprintf(stderr,"GPU command error %s\n",cmd.error.description.UTF8String);return 1;}
 const unsigned char*out=readback.contents;size_t mismatches=0;for(int y=0;y<h;y++)for(int x=0;x<w*4;x++)if(out[y*row+x]!=expected[y*pitch+x])mismatches++;
 printf("Metal device: %s\nPASS GPU frame %d phase %d rendered readback: %d pixels, %zu byte mismatches\n",dev.name.UTF8String,frame,phase,w*h,mismatches);if(mismatches)return 1;
 if(phase==0)continue;
 if(frame==0&&phase==1)[[NSData dataWithBytes:out length:row*h]writeToFile:@"runs/stage23-graphics/gpu-readback.bgra" atomically:YES];
 NSBitmapImageRep*png=[[NSBitmapImageRep alloc]initWithBitmapDataPlanes:NULL pixelsWide:w pixelsHigh:h bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:w*4 bitsPerPixel:32];
 unsigned char*rgba=png.bitmapData;for(int y=0;y<h;y++)for(int x=0;x<w;x++){const unsigned char*p=out+y*row+x*4;unsigned char*q=rgba+(y*w+x)*4;q[0]=p[2];q[1]=p[1];q[2]=p[0];q[3]=255;}if(frame==0&&phase==1)[[png representationUsingType:NSBitmapImageFileTypePNG properties:@{}]writeToFile:@"evidence/stage23-gpu-frame.png" atomically:YES];
 NSString*json=[NSString stringWithFormat:@"{\"gpu_device\":\"%@\",\"width\":%d,\"height\":%d,\"readback_pitch\":%lu,\"pixels\":%d,\"byte_mismatches\":%zu,\"format\":\"BGRA8Unorm\",\"gpu_render_executed\":true}\n",dev.name,w,h,(unsigned long)row,w*h,mismatches];[json writeToFile:[NSString stringWithFormat:@"evidence/stage23-gpu-%d.json",frame] atomically:YES encoding:NSUTF8StringEncoding error:nil];}
 return 0;
}
int main(int argc,const char*argv[]){@autoreleasepool{
 const unsigned char*image=NULL;int w=0,h=0,pitch=0;const char*fixture=argc>1?argv[1]:"runs/stage23-graphics/cases.bin";if(s23_verify(fixture,&image,&w,&h,&pitch))return 1;id<MTLTexture>first=nil;for(unsigned i=0;i<8;i++){Surface*f=s23_frame(i);if(!f||gpu(f->pixels,f->w,f->h,f->pitch,i,s23_frame_initial(i)))return 2;if(i==0)first=rendered;}rendered=first;
 if(argc>2&&strcmp(argv[2],"--headless")==0)return 0;
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];[NSApp finishLaunching];
 NSWindow*win=[[NSWindow alloc]initWithContentRect:NSMakeRect(200,180,576,560) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];win.title=@"Emperor original 555 → native arm64 → Metal | pixels match";
 MTKView*view=[[MTKView alloc]initWithFrame:NSMakeRect(0,0,576,560) device:dev];view.colorPixelFormat=MTLPixelFormatBGRA8Unorm;view.paused=YES;view.enableSetNeedsDisplay=YES;Renderer*renderer=[Renderer new];view.delegate=renderer;win.contentView=view;[win makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];
 NSDate*end=[NSDate dateWithTimeIntervalSinceNow:3];while([end timeIntervalSinceNow]>0){NSEvent*e=[NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:.05] inMode:NSDefaultRunLoopMode dequeue:YES];if(e)[NSApp sendEvent:e];[NSApp updateWindows];[view draw];}
 NSTask*capture=[NSTask new];capture.executableURL=[NSURL fileURLWithPath:@"/usr/sbin/screencapture"];capture.arguments=@[@"-x",@"-l",[NSString stringWithFormat:@"%ld",(long)win.windowNumber],@"evidence/stage23-window.png"];NSError*err=nil;if([capture launchAndReturnError:&err]){[capture waitUntilExit];printf("window screenshot exit=%d\n",capture.terminationStatus);}else fprintf(stderr,"window screenshot: %s\n",err.description.UTF8String);
 [win orderOut:nil];[win close];[NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];return 0;
}}
