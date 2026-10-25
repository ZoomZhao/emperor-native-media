// SPDX-License-Identifier: MIT
#pragma once
#include "scene_draw_list.hpp"
#include "metal_scene_prototype/metal_sprite_renderer.hpp"
namespace game_lab {
// Verification adapter only. Two synchronous offscreen passes and readbacks;
// no App dependency, drawable, upload cache or asynchronous publication.
struct MetalSceneVerification {
    bool complete=false;std::string error;
    msp::GpuResult layer,composite;
};
inline MetalSceneVerification verifySceneDrawListMetal(const SceneDrawList& list,msp::MetalSpriteRenderer& renderer,msp::BlendPath path){
    MetalSceneVerification result;result.error=validateSceneDrawList(list);if(!result.error.empty())return result;
    msp::Scene layer;layer.name="original_sprite_layer";layer.width=list.width;layer.height=list.height;
    layer.initial.resize(size_t(list.width)*list.height);
    if(!list.items.empty())layer.scale=list.items.front().width/list.sprites[list.items.front().sprite].value->image.width;
    for(const auto& r:list.sprites){const auto& im=r.value->image;layer.sprites.push_back({im.width,im.height,im.pixelsBGRA});}
    for(size_t n=0;n<list.items.size();++n){
        const auto& i=list.items[n];const auto& im=list.sprites[i.sprite].value->image;
        if(i.width!=im.width*layer.scale||i.height!=im.height*layer.scale){result.error="prototype_uniform_scale_required";return result;}
        layer.items.push_back({i.sprite,i.left,i.top,n});
    }
    result.error=msp::validateScene(layer);if(!result.error.empty()){result.error="prototype_layer_"+result.error;return result;}
    result.layer=renderer.render(layer,path,msp::TargetFormat::BGRA8Unorm);
    if(!result.layer.ok){result.error="metal_layer_"+result.layer.error;return result;}
    msp::Scene composite;composite.name="original_layer_over_numerical_background";
    composite.width=list.width;composite.height=list.height;composite.initial=list.background.pixelsBGRA;
    composite.sprites.push_back({list.width,list.height,result.layer.pixels});composite.items.push_back({0,0,0,0});
    result.composite=renderer.render(composite,path,msp::TargetFormat::BGRA8Unorm);
    if(!result.composite.ok){result.error="metal_composite_"+result.composite.error;return result;}
    result.complete=true;return result;
}
} // namespace game_lab
