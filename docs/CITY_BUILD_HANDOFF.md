# 城市构建与接入的三项事实

此处本机旧任务以 `LOCAL_LEGACY_TASK` 标识，只记录相对路径；它不是构建器的查找路径或运行配置。

1. **本机已有 203 函数产物，发布仓库未收录。** 旧任务 `stage81/builds/full-separated-v5/` 中存在 game.ll、game.o、phase8-separated-OPEN.a、phase8-load-only.dylib；result.json 记载 203 函数／7346 指令／20089 原代码字节。`stage82/builds/original-v3/native-driver` 已链接，报告状态 DRIVER_BUILT_NOT_EXECUTED。本次只查旧产物路径和报告，不加载或执行，旧库的二进制哈希没有重新核验。不能将仓库缺失写成全局不存在，也不能推定这些原代码派生产物全部可发布。

2. **配置和域文件本机存在，归档缺口可部分闭合。** 旧任务 stage82/configs/authored.json、original.json 和 stage80／81／82／83 各相应 domains.c 均存在；域文件在 legacy/import-manifest.json 因 fixed_original_bindings_or_generated_source 暂留。stage24_generated.h、stage5_layout.h 也有本机文件。第三方 llasm_cpu.h 有现成本机候选与其 MIT 版权段，仍须匹配具体历史 ABI。此次仅整理来源明确的自造编译作用域与生成方法，引用已有自写源码，实际闭合 native_driver.c 的头文件编译依赖；未复制原代码、域表、游戏资产或私密旧配置。14 个暂留 import 的源文件也存在；同名 pipeline 的阶段绑定仍需明确，不能任意替换。

3. **App 接入及真实运行语义尚未完成。** game_lab/app/build.sh 未链接城市库或 runtime_core；main.mm 的 loadAndRenderMap 只是 tile ID 预览，没有真实城市更新／调度／存档消费者。旧 stage82/evidence/blockers-verified.json 列出 20 个 Win32 边界与未实现 SEH；legacy/src/stage82/tools/platform_native.c 的 s82_unimplemented 直接停止。较晚 phase7-probe-v1 报告已实现有界控制器，因此“本机没有执行器”不准确；该版本仍没有新的原执行证明、合格自然捕获到 B／C 的完整适配、真实 phase8 验收和已证明的未来回调域。

本轮只闭合自写驱动编译依赖；没有以空绑定或成功 stub 伪造链接。受阻的原城市指令恢复／差分路线没有重启，旧批准不构成本轮执行授权。

下一项具体可闭合的自写平台边界是 `legacy/src/stage79/tools/guest_memory.c` 的 `s79_memory_open`／`s79_memory_close`／`s79_guard` 提供者：它已存在，可加入同一明确构建清单，核对跨区／权限／溢出拒绝和失败清理的既有接口，再以自造区域做独立验收。这样能减少当前对象未解析的平台符号；城市入口和原绑定仍保持未解析，不能据此声称完整城市运行。不另开无关媒体样例。
