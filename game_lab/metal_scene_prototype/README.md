# metal_scene_prototype

隔离的离屏 Metal 精灵绘制验证原型。只用自造纹理和场景，不读取原版资产，不接入 App，不改现有渲染管线。CPU 参考公式逐字复制自 `original_building_scene.hpp` 的对象层重放与 `map_reference.cpp` 的网格渲染，不为适配 GPU 修改。

## 运行

```sh
game_lab/metal_scene_prototype/run.sh                 # 要求固定功能与可编程混合都逐像素一致
game_lab/metal_scene_prototype/run.sh --require=fixed # 只把固定功能路径作为门
game_lab/metal_scene_prototype/run.sh --iterations=50 --verbose
```

产物在本目录 `build/`（已被 `**/build*/` 忽略），可用 `MSP_BUILD_DIR` 改位置。只需 Command Line Tools 的 `clang++` 与系统框架 Foundation/Metal/CoreGraphics；着色器在运行时用 `newLibraryWithSource` 编译（本机没有离线 `metal` 工具）。

退出码：`0` 必需检查全部通过；`1` 必需 GPU 路径有差异或不可用，或 sRGB 对照未能检出差异；`2` 无可用 Metal 设备或基础着色器编译失败（缺能力，不算通过）；`3` CPU 参考未通过独立判据；`4` 参数错误。

## 场景与判据

| 场景 | 内容 | 独立判据（不依赖 CPU 参考实现） |
|---|---|---|
| `paint_order` 48×32 | 乱序提交、同深度并列（按提交顺序）、不透明与半透明重叠 | 9 个手算像素 |
| `alpha_exhaustive` 2048×1371 | 全部 a∈[0,255]、c≤a、d∈[0,255] 的 B/G/R 组合，以及 A 通道 (a,d)，共 11,231,232 个通道 | 逐通道数学判据 `c+round_half_up(d·(255−a)/255)` |
| `scale_clip_x1..x4` 61×41 | 5×3 非对称纹理（含 1 个全透明、1 个半透明纹素）；负坐标、右/下边裁剪、恰好在外、±4096 边界 | 15/29/43/57 个手算像素：朝向、最近邻、`floor((0-left)/scale)` |
| `stress_4000` 816×620 ×2 | 4000 次绘制，32 个随机预乘精灵，alpha 0/255/部分混合，深度并列，部分超出画布 | 仅 CPU 对照 |
| sRGB 对照 | 同样场景改用 `BGRA8Unorm_sRGB` 目标 | 必须检出差异，证明比对不是空检 |

非法预乘输入（某通道大于 alpha）由 `validateScene` 拒绝：CPU 公式在这种输入下会把溢出进位到下一个通道，GPU 则会截断。适配时必须保证解码出的精灵满足预乘约束，或在入口处断言。

## 实测结果（2026-10-24，Apple M1，macOS 26.5.2 25F84）

设备：`Apple M1`，统一内存，GPU 家族 Apple1–7、Mac2、Common1–3、Metal3；基础着色器与可编程混合着色器都编译成功。

| 路径 | 场景 | 差异像素 | 差异通道 B/G/R/A | 独立判据 |
|---|---|---|---|---|
| fixed_function / BGRA8Unorm | paint_order | 0/1,536 | 0/0/0/0 | 9/9 |
| | alpha_exhaustive | 0/2,807,808 | 0/0/0/0 | 判据不符 0 |
| | scale_clip_x1/x2/x3/x4 | 各 0/2,501 | 0/0/0/0 | 15/15、29/29、43/43、57/57 |
| | stress_4000 | 0/505,920 | 0/0/0/0 | — |
| programmable / BGRA8Unorm | 同上全部场景 | 全部为 0 | 全部为 0 | 全部满分 |
| 对照 fixed / BGRA8Unorm_sRGB | paint_order | 212/1,536 | 12/200/212/0，max\|Δ\| 60 | 8/9 |
| | alpha_exhaustive | 2,803,183/2,807,808 | max\|Δ\| 73 | 判据不符 8,403,531 |

`RESULT fixed_function=EXACT programmable=EXACT srgb_control_detects=yes required=both exit=0`。连续 3 次运行结果相同，`--require=fixed` 也退出 0。

结论：在本机，固定功能混合（`one, oneMinusSourceAlpha`，目标 `BGRA8Unorm`）对所有合法预乘输入都与 CPU 整数公式逐字节一致。可编程混合（在 fragment 里读 `[[color(0)]]`，用整数公式计算）同样精确，可作为在其他设备上固定功能不精确时的替代路径；它在 Apple 家族 GPU 上可用，本原型未在 Intel/AMD Mac 上测过。

### 耗时（stress_4000，4000 次绘制，各分量分开计，单位 ms，中位数）

| 路径 | 迭代 | 一次性上传 | 目标重置 | 编码 | GPU 执行 | 提交到完成的等待 | 读回 |
|---|---|---|---|---|---|---|---|
| fixed_function | 20、50、50 | 0.36–0.47 | 0.16–0.72 | 0.63–1.62 | 5.1–6.0 | 5.9–6.5 | 1.0–2.2 |
| programmable | 20、50、50 | 0.44–1.07 | 0.72–0.96 | 1.64–2.11 | 7.2–7.5 | 7.9–8.6 | 2.2–3.3 |

范围取自三次运行。GPU 执行时间来自 `GPUEndTime−GPUStartTime`；“等待”和“读回”是测试用的同步开销，不代表上屏性能。本原型每个精灵一次 draw call，没有做 instancing，也没有纹理图集，所以这些数值只能作为离屏功能基线，不能当作 App 帧时间。

## 颜色空间（只限字节层面）

- 离屏比较只证明：BGRA8Unorm 目标的**字节值**与 CPU 参考一致。不证明窗口、CAMetalLayer 或显示器最终发出的颜色。
- `BGRA8Unorm_sRGB` 目标会在混合前把目标值线性化，字节值因此改变（见上面的对照行）。所以必须用非 sRGB 的 `BGRA8Unorm`。
- CoreGraphics 探针（离屏绘制自造的不透明色阶，1024 像素）：DeviceRGB→DeviceRGB 和 DeviceRGB→sRGB 都是 0 个字节变化；DeviceRGB→DisplayP3 有 757 个像素变化，DeviceRGB→LinearSRGB 有 1016 个；DeviceRGB→主显示器色彩空间（未命名）有 757 个，计数与 DisplayP3 相同。
- 推断（未验证）：现有 NSImage 路径在上屏时会经过 DeviceRGB（在 CG 里等同 sRGB）到显示器色彩空间的匹配。CAMetalLayer 若要外观一致，候选做法是把 `layer.colorspace` 设为 sRGB。这必须在 App 里对同一显示器截图比对才能确认；本原型没有做这一步。

## 已知限制

- 只测了一台设备（Apple M1）和一个系统版本，没有覆盖 Intel/AMD GPU。
- 没有上屏，也没有 CAMetalLayer、drawable、`presentsWithTransaction` 或 AppKit 覆盖层。
- 每个精灵单独一张纹理、单独一次 draw call，每次渲染都新建资源；没有图集、instancing、跨帧缓存、在途帧上限或 epoch 丢弃。
- 只覆盖整数 scale 1–4 和最大 4096 的尺寸，与现有渲染器的边界一致。
- 没有直接对比现有 `renderOriginalSceneOverlays` 或 `renderMapImageGridViewport` 的输出；CPU 参考是复制出来的公式和循环结构。

## 适配现有 SceneDrawList / SpriteCache / FrameKey 的最小接口

1. **绘制项**：`{SpriteKey key; int32_t left, top; uint32_t width, height;}`。左上角由 CPU 用现有的 `floor(+.5)+offset*scale` 算成整数，列表已稳定排序；GPU 只按顺序画。对应本原型的 `DrawItem` 加 `paintOrder` 结果。
2. **精灵像素契约**：小端打包 BGRA8 预乘的 `uint32_t`（即 `DecodedImage::pixelsBGRA`），内存布局等同 `MTLPixelFormatBGRA8Unorm`；解码后已镜像；入口处断言 `channel ≤ alpha`。
3. **SpriteCache → GPU 纹理**：`textureFor(SpriteKey, generation) -> id<MTLTexture>`。纹理在每帧完成回调之前要被强引用住；本原型对应每次渲染新建的 `textures` 数组。
4. **目标**：`BGRA8Unorm`，非 sRGB；先清成透明或上传底图，`loadAction` 用 Load；画布尺寸等于相机宽高，scale 只体现在四边形尺寸上。
5. **路径选择**：启动时用自造小场景（本原型的 `paint_order` 加一个 alpha 子集）做一次自检。固定功能精确就用固定功能；否则在可编程混合可用时用可编程混合；都不行就回退 CPU。
6. **FrameKey**：渲染结果只在 `{sessionEpoch, revision, sceneDigest, cameraSeq, cacheGeneration}` 等于当前期望键时提交；本原型只做同步，没有实现这一层。
7. **测试钩子**：`readback(DecodedImage&)` 只供测试，同步等待；不要用在上屏路径上。
