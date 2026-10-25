# metal_scene_renderer

持久离屏 Metal 后端（stage46）。直接消费 `game_lab::SceneDrawList` 与 `game_lab::SceneFrameTicket`，每个已接受请求产出一张已完成、不可变、城市尺寸的 `BGRA8Unorm`（非 sRGB）纹理。不判定哪个请求是当前请求，不调用 AppKit，不自动 present，正常路径不读回、不等待 GPU。App 生命周期、显示、颜色空间和 FrameKey 门由主实施负责。

## 文件

| 文件 | 作用 |
|---|---|
| `metal_scene_renderer.hpp` | 公共接口：`Renderer`、`Frame`、`Completion`、`Capabilities`、`Counters`、`Options` |
| `metal_scene_renderer.mm` | 实现：自检、缓存、两段编码、在途计数、回调投递 |
| `metal_scene_shaders.inc` | 运行时编译的 shader 源：实例化、逐项 scale、CPU 预裁剪四边形；固定功能与可编程片元分别编译 |
| `metal_scene_renderer_testing.mm` | **仅测试**：`readFrameForTesting`（blit 后同步等待） |
| `metal_scene_renderer_tests.mm` | 自造测试；`--real` 复用 stage45 的真实场景与 CPU 对照 |
| `run.py` | 构建与运行（Release/UBSan），静态检查禁用调用，真实输入哈希只读校验 |

## 公共接口要点

```cpp
namespace game_lab::metal_scene {
Renderer r(Options{/*cacheBudgetBytes=64MiB, cacheEntryLimit=2048*/});
Capabilities c=r.capabilities();   // device, blend, fixed/programmable exact, failure, BGRA8Unorm, generations, limits
SubmitResult s=r.submit(std::shared_ptr<const SceneDrawList>, SceneFrameTicket, CompletionHandler);
// s.status: Accepted | Busy | Unsupported | Closed | Failed, s.reason named
// Completion{status Ready|Failed|Cancelled, ticket (unchanged), shared_ptr<const Frame> frame, reason}
// Frame: width(), height(), pixelFormatName(), allocatedBytes(), diagnostics(), texture() [ObjC++]
r.invalidateResources();  r.shutdown();  Counters k=r.counters();
}
```

- **调用线程**：`submit`、`invalidateResources`、`shutdown`、`counters` 可在任意线程调用，都不等待 GPU。
- **回调队列**：私有串行队列 `game_lab.metal_scene.callbacks`。回调既不在主线程上执行，也不在后端锁内执行；App 只应弱捕获 owner，再切回主线程。
- **submit 立即返回**：调用线程只做轻量检查（关闭、自检状态、可用性、空列表或空回调、目标尺寸、ticket 尺寸、在途数）。完整校验（与 `validateSceneDrawList` 相同的检查和顺序，含逐像素预乘检查）、上传和编码都在私有串行 worker 队列上执行。所以非法列表的结果是 `Accepted`，随后收到 `Failed("draw_list_invalid:<code>")`，绝不会静默跳过绘制。
- **Busy 的含义**：`max_in_flight` 表示已有 2 个已接受请求尚未完成 GPU 工作；`self_test_pending` 表示启动自检尚未结束。两者都是背压，不是成功。
- **Unsupported**：没有设备、shader 或 pipeline 失败、两条混合路径都不精确。App 应保留 CPU 路径。
- **恰好一次**：每个已接受请求只得到一次 Ready、Failed 或 Cancelled（由原子 CAS 保证）；非 Accepted 一律不回调。`shutdown()` 和析构对尚未投递的请求发送 `Cancelled("backend_shutdown")`；GPU 工作随后在后台结束，资源在那之后才释放。
- **编码器判空**：每个可为 nil 的编码器工厂（第 1 段渲染、背景 blit、第 2 段渲染、自检读回 blit）都要判空。为 nil 时在 commit 之前以具名原因失败：`render_encoder_unavailable:sprite_layer`、`blit_encoder_unavailable:background`、`render_encoder_unavailable:composite`、`blit_encoder_unavailable:self_test_readback`。不 commit，不产生 Frame，不钉住资源；测试读回的 nil blit 返回 `readback_blit_encoder_unavailable`。
- **两段合成全在 GPU 上**：第 1 段按 vector 顺序把精灵画到清为 0 的中间纹理；第 2 段先把数值背景精确 blit 到输出，再把整层叠上去。中间纹理每个请求新建，不读回。
- **坐标**：裁剪条件与 CPU 参考完全相同（int64）；可见项的四边形先在 CPU 上裁到目标范围，±1,000,000 的原点不会进入光栅化。scale 1–4 按每项的 `width/图像宽` 取，同一列表可混用。
- **混合路径**：启动自检把 2 份自造列表（穷举 alpha 与混合 scale/裁剪）送进同一条生产编码路径，与 `rasterizeSceneDrawListCPU` 逐字节比较。固定功能精确时优先用它，否则用精确的可编程路径，都不行就 Unsupported。自检读回是在它自己的命令缓冲里 blit 到共享 buffer、在完成回调中比较，不属于正常路径。

## 缓存与资源

### 预算范围（全部按实际 `allocatedSize`，除非注明“逻辑”）

| 资源 | 谁控制 | 上限 / 生命周期 |
|---|---|---|
| 缓存纹理（驻留精灵） | 后端 | `retainedBytes ≤ cacheBudgetBytes`（默认 64 MiB），条目数 ≤ `cacheEntryLimit`（2048）。准入与 LRU 淘汰都用创建后读到的真实 `allocatedSize`；若真实大小超过预算，该精灵本次走未缓存路径 |
| 每个请求钉住的资源（命中或未命中的精灵、被淘汰仍在用的纹理、背景、中间层、输出、实例 buffer） | 后端 | 每个请求 ≤ `requestPinBudgetBytes`（默认 256 MiB）；超出时该请求以 `Failed("request_pin_budget_exceeded")` 结束，不绘制、不产生 Frame、不钉住任何资源。在途最多 2 个（否则 Busy），所以 `pinnedBytes ≤ 2 × requestPinBudgetBytes`。钉住的资源可能与缓存重叠，GPU 完成后释放 |
| 启动自检 | 后端，一次性 | 固定的自造列表，结束后释放；实际字节记在 `capabilities().selfTestTransientBytes`（本机 116,053,864） |
| 已交付的 Frame（输出纹理） | 调用者 | 不计入任何后端预算，`liveFrameBytes` 报告其实际字节；后端绝不为满足预算而回收、复写或复用它 |

后端持有的 GPU 内存上界为 `cacheBudgetBytes + 2 × requestPinBudgetBytes`，再加上启动自检的一次性占用；调用者持有的 Frame 另计。

- **预估与实测**：`heapTextureSizeAndAlign` 的预估只用来提前拒绝。本机实测它可能小于真实值（64×64 预估 16,512，实际 32,768），所以分配后一定用真实值复核；预估偏大最多造成一次 fail-closed 的拒绝。

- **缓存键**：`SG3DecodedSprite.identity`（进程内单调、重新解码即换新值）。命中还要求：条目的 `resourceGeneration` 等于当前值，且条目弱引用的那个**同一个**不可变 `SG3DecodeResult` 仍然存活并与列表里的完全相同。不用裸地址、archive 索引或文件名作键。同一 identity 对应新对象时不复用，计入 `staleReplacements` 并重新上传。
- **上限**：见上表。未缓存路径计入 `uncachedUploads` / `uncachedBytes`，并受每个请求的钉住预算约束。
- **不复写**：已有纹理从不被写入。淘汰只移除缓存引用，在途命令缓冲和 Frame 仍持有各自的纹理。
- **失效**：`invalidateResources()` 立即推进 `resourceGeneration`，之后不再复用任何旧缓存纹理；残余条目在 worker 上异步清除。`deviceGeneration` 等于每个 Renderer 实例的进程内唯一 `backendGeneration`；设备重置的做法是新建 Renderer，同时由 App 推进 cacheGeneration。

### 计数器生命周期

| 计数 | 生命周期 |
|---|---|
| accepted/busy/unsupported/closed/refused、ready/failed/cancelled | 每个 Renderer 单调递增 |
| uploads/hits/uncachedUploads/evictions/invalidatedEntries/staleReplacements | 单调递增 |
| uploadBytes（逻辑拷贝量，含背景）、uploadAllocatedBytes、uncachedBytes | 单调递增；后两者为实际字节 |
| retainedBytes（实际）/retainedLogicalBytes/retainedEntries；peakRetainedBytes | 当前驻留缓存；peak 为实际字节峰值 |
| inFlight | 从 Accepted 到 GPU 完成（或未编码就取消）；最多 2；Cancelled 投递后仍会计入，直到 GPU 结束 |
| pinnedTextures/pinnedBytes；peakPinnedBytes | 在途命令缓冲持有的资源（精灵、背景、中间层、输出、实例 buffer），实际字节 |
| liveFrames/liveFrameBytes | 仍被任何地方持有的 Frame |

## 运行与结果

```sh
python3 game_lab/metal_scene_renderer/run.py --mode release   --output-dir /path/outside/repo/release
python3 game_lab/metal_scene_renderer/run.py --mode undefined --output-dir /path/outside/repo/undefined
# 真实数据：只为这一次运行注入，原始文件前后做哈希校验
EMPEROR_GAMEDATA_ROOT=/verified/local/copy python3 game_lab/metal_scene_renderer/run.py --mode release --output-dir ...
```

退出码：runner 全部通过为 0，否则为 1。测试二进制：0 表示通过或 SKIP，1 表示失败，2 表示缺 Metal 能力（不算通过）。`verification.json` 记录源文件和二进制的 sha256、每次运行的 JSON 行、静态检查、输入哈希。

整合前修复后的最终结果（2026-10-24，Apple M1，macOS 26.5.2；Release 与 UBSan 各运行一次，都退出 0）：

- **能力**：固定功能与可编程自检都精确，选用 fixed_function；BGRA8Unorm；纹理上限 16384；启动自检一次性占用 116,053,864 字节（实际）。
- **自造 CPU/GPU 对照**：混合 scale 0/4,169,633 个像素不同；强制可编程 0/3,651,648；两段合成反例：GPU 等于 CPU 的 `ff3f3f3f`，压平成一段会得到 `ff404040`。
- **nil 编码器**：第 1 段渲染、背景 blit、第 2 段渲染三处各注入 3 次，9/9 都以具名原因 Failed 恰好一次，无 Ready，没有新 Frame，在途和钉住计数归零；之后 renderer 仍能正常出正确画面。“延迟 + 背景 blit 为 nil”与 shutdown 竞态 5/5，都恰好一次 Cancelled，计数归零。
- **按 allocatedSize 记账**：64×64 逻辑 16,384、实际 32,768 字节。预算 98,304 时驻留 3 项（按逻辑字节会准入 6 项），驻留实际 98,304、逻辑 49,152，淘汰 7 次。预算 32,767（大于逻辑、小于实际）时 10/10 走未缓存路径，共 327,680 字节。Frame 字节等于输出纹理的实际大小 32,768；该请求钉住 13 个资源，共 426,424 字节。
- **每请求钉住预算**：预算 262,144 时，13 个纹理的请求以 `request_pin_budget_exceeded` 失败，无 Ready、无钉住、无 Frame；2 项请求钉住 163,960 字节通过，峰值钉住 163,960 ≤ 2 × 预算。
- **原有测试**：缓存命中、失效、陈旧替换 12/12/12/1；小缓存预算下淘汰 5 次，驻留 1 项 49,152/49,152；未缓存 6 次；Frame 存活 5/5；背压 2 Accepted + 1 Busy、2 次回调；异步失败 3/3 + 2 个无回调拒绝；关闭竞态 80 个 Accepted 各恰好一次回调（Ready 10、Cancelled 70）、32 次 Closed 无回调；共 124 次完成，在主线程执行的为 0。自造检查 803/803。
- **主 renderer 结束时**：驻留实际 1,556,224 字节，对应逻辑 495,152（约 3.1 倍），峰值驻留 5,147,392，峰值钉住 40,255,232。
- **真实数据**（stage45 的 20 帧，冷、热各一）：fixed_function 与 programmable 各 0/14,310,540 个像素不同；全部输入释放后 20/20 不变；热请求命中 681/681；峰值驻留 5,292,032 字节（实际）；原始输入 140/140 未变。
- **静态检查**：正常路径文件中 10 个禁用标识符出现 0 次。

### 本次接口变化（默认调用保持兼容）

- `Options::requestPinBudgetBytes`（默认 256 MiB）；`Capabilities::requestPinBudgetBytes`、`selfTestTransientBytes`。
- `Counters`：`retainedBytes`、`peakRetainedBytes`、`pinnedBytes` 改为实际 `allocatedSize`；新增 `retainedLogicalBytes`、`uploadAllocatedBytes`、`uncachedBytes`、`peakPinnedBytes`。
- `FrameDiagnostics`：`pinnedBytes` 改为实际字节；新增 `uploadAllocatedBytes`、`uncachedBytes`。
- 新的具名失败：`request_pin_budget_exceeded`、`render_encoder_unavailable:sprite_layer|composite`、`blit_encoder_unavailable:background|self_test_readback`。
- `TestFault` 改为可叠加的位（新增 `NilSpriteEncoder`、`NilBackgroundBlit`、`NilCompositeEncoder`）；仅供测试。

计时只代表离屏执行；接受到完成的时间不含显示，也不能当作 App 帧时间。

## 限制与未覆盖

- 只测了一台设备（M1）；Intel/AMD、设备丢失、GPU 超时、内存压力都没有测，也没有设备丢失检测（App 应新建 Renderer）。
- 真实列表经捕获阶段已预先裁剪（被裁剪项为 0），远原点只由自造列表覆盖。
- 背景每次请求都上传（`DecodedImage` 没有 identity）。若要缓存背景，需要在 `SceneDrawList` 里为背景提供 identity；这只是接口提案，本目录没有修改。
- 中间纹理和输出纹理每个请求新建，没有池化。
- 故障注入只覆盖“提交前”和“完成时”两个后端路径，没有真正诱发 GPU 错误。
- 没有显示、颜色空间、Retina、aspect-fit、drawable 或 GUI 方面的任何结论。

## App 适配点（由主实施负责）

1. 启动时创建 `Renderer`。`Busy(self_test_pending)` 期间用 CPU 或稍后重试；`Unsupported` 时一直走 CPU。
2. 用 `requestSceneFrame` 拿到 ticket 后提交 `shared_ptr<const SceneDrawList>`。Accepted 之外的状态不会有回调，App 自行决定合并重试或 CPU 回退。
3. 回调里只弱捕获 owner，切到主线程后，在发布边界调用 `consumeSceneFrame(state, ticket, frame->width(), frame->height())`；Current 时再显示 `frame->texture()`（private、ShaderRead，可同尺寸 blit 或绘制到 drawable）。
4. Failed 时只有在 `mayFallbackSceneFrame` 返回真时才做 CPU 回退；Cancelled 只在 shutdown 时出现。
5. 资源根替换或解码源失效时：先推进 gate 的 Resources，再调用 `invalidateResources()`。
6. Cmd+Q 或关闭时调用 `shutdown()`（不阻塞），保留最后一帧直到视图释放。
