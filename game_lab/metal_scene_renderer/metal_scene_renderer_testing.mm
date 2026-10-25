// SPDX-License-Identifier: MIT
// TEST-ONLY synchronous readback. Never linked into an App path by design: it
// waits on the GPU. Uses only the public Frame API.
#include "metal_scene_renderer.hpp"
#import <Metal/Metal.h>

namespace game_lab::metal_scene {
bool readFrameForTesting(const Frame& frame,DecodedImage& out,std::string& error){
    id<MTLTexture> texture=frame.texture();
    if(!texture||texture.pixelFormat!=MTLPixelFormatBGRA8Unorm){error="frame_texture_invalid";return false;}
    const size_t w=texture.width,h=texture.height,bytes=w*h*4;
    if(w!=frame.width()||h!=frame.height()){error="frame_dimension_mismatch";return false;}
    @autoreleasepool {
        id<MTLCommandQueue> queue=[texture.device newCommandQueue];
        id<MTLBuffer> buffer=[texture.device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
        id<MTLCommandBuffer> command=[queue commandBuffer];
        if(!queue||!buffer||!command){error="readback_allocation_failed";return false;}
        id<MTLBlitCommandEncoder> blit=[command blitCommandEncoder];
        if(!blit){error="readback_blit_encoder_unavailable";return false;}
        [blit copyFromTexture:texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(w,h,1)
            toBuffer:buffer destinationOffset:0 destinationBytesPerRow:w*4 destinationBytesPerImage:bytes];
        [blit endEncoding];[command commit];[command waitUntilCompleted];
        if(command.status!=MTLCommandBufferStatusCompleted){error="readback_command_failed";return false;}
        out.width=uint32_t(w);out.height=uint32_t(h);out.pixelsBGRA.resize(w*h);
        memcpy(out.pixelsBGRA.data(),buffer.contents,bytes);
    }
    return true;
}
} // namespace game_lab::metal_scene
