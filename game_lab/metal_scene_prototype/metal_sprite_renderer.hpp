// SPDX-License-Identifier: MIT
// Isolated offscreen Metal sprite renderer for verification only. The CPU
// supplies final painter order and integer pixel origins; the GPU draws one
// quad per item, in that order, into a BGRA8 render target that is read back.
#pragma once
#include "cpu_reference.hpp"
#include <memory>
#include <string>
#include <vector>

namespace msp {
enum class BlendPath { FixedFunction, Programmable };
enum class TargetFormat { BGRA8Unorm, BGRA8UnormSRGB };
inline const char* blendPathName(BlendPath p){return p==BlendPath::FixedFunction?"fixed_function":"programmable";}
inline const char* targetFormatName(TargetFormat f){return f==TargetFormat::BGRA8Unorm?"BGRA8Unorm":"BGRA8Unorm_sRGB";}

struct GpuIteration {
    double targetResetMs=0;  // CPU: replaceRegion of the initial target bytes
    double encodeMs=0;       // CPU: command buffer + render encoder + draws
    double waitMs=0;         // wall: commit -> waitUntilCompleted (test-only sync)
    double gpuMs=0;          // GPUEndTime - GPUStartTime
    double readbackMs=0;     // CPU: getBytes from the shared target (test-only)
};
struct GpuResult {
    bool ok=false;
    std::string error;
    double uploadMs=0;       // sprite + target texture creation and upload, once
    size_t uploadBytes=0,draws=0;
    std::vector<GpuIteration> iterations;
    std::vector<uint32_t> pixels; // last iteration's readback
};
struct DeviceReport {
    bool device=false;
    std::string name,reason,baseCompile,programmableCompile;
    bool unifiedMemory=false,lowPower=false,removable=false;
    uint64_t registryId=0,recommendedWorkingSet=0;
    std::vector<std::string> families;
};
class MetalSpriteRenderer {
public:
    MetalSpriteRenderer();
    ~MetalSpriteRenderer();
    MetalSpriteRenderer(const MetalSpriteRenderer&)=delete;
    MetalSpriteRenderer& operator=(const MetalSpriteRenderer&)=delete;
    bool available() const;
    DeviceReport report() const;
    // Creates (or reports why it cannot create) the pipeline for one path/format.
    bool pathAvailable(BlendPath,TargetFormat,std::string& why);
    GpuResult render(const Scene&,BlendPath,TargetFormat,unsigned iterations=1);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace msp
