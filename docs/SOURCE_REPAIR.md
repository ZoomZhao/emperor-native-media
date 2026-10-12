# 推送与源码修复记录

2026-10-10，工作目录为 `emperor-native-media`。私有远端 main 已核验为 `2f3378a91fb0987288e00c61befa0ddeacbe50be`：[提交页面](https://github.com/ZoomZhao/emperor-native-media/commit/2f3378a91fb0987288e00c61befa0ddeacbe50be)。没有 force push。以下修复在推送之后写入工作树，尚未提交或再次推送。

1. **资源解析。** WAV 从“不检查 PCM 格式、奇数 data 多写一字节、忽略 RIFF/chunk 范围”改为有界内存接口，完整验证格式、帧对齐、长度、padding、重复块，分配前拒绝无效输入。JPEG 检查尺寸和颜色空间/context，RGB555 检查每个转换步骤，转换失败不返回有效图像。文件/区间要求完整读取；SG3 count 按完整记录表限界；现有 map 预览要求完整解压输出。未知/压缩图素不猜尺寸或补零。这些是源码修复，尚无行为验收结果。
2. **App 配置与静音。** 资源只从明确绝对目录配置解析，不依赖 cwd；没有权限修改或失败 fallback。启动不创建播放器或自动播放，音乐与 SFX 共用默认静音/音量策略；静音暂停音乐并停止 SFX，取消静音不自动播放。静音时选新曲目清除旧播放器，避免随后恢复错误曲目。移除未使用的 Metal imports/link 项和主动抢占其他 App 的激活调用，标明浏览器与任意颜色 map 预览的范围。
3. **独立数据组件。** `runtime_core/` 整理自写有限 heap、私有清零 vector 与 context-local 槽/error；明确 include/src/tests 路径，不带旧引擎/快照加载器。历史 32 处 include 依赖分类为 23 处可定位共享头文件、6 处缺失生成绑定、3 处第三方 ABI。归档原样保留；没有创建绑定或激活历史链。

`game_lab/boundary_tests.mm` 和 `runtime_core/tests/boundary_tests.cpp` 使用完全自造 WAV、色值、JPEG、临时文件和分配/context 数据，已写源码，未运行。新增自造边界 runner 在将来明确允许执行时保存独立证据目录及源码/二进制/结果哈希，避免覆盖旧结果。旧 GameData runner 只接入新的解析源文件，旧记录仍是历史记录。

本轮静态检查：App/解析器/边界用例 Objective-C++ 与 C++ `-fsyntax-only`，独立 heap C 与 core 用例 C++ `-fsyntax-only`，shell `-n`，差异空白/头文件依赖/来源哈希核对。详细命令、退出码与当前源码 SHA 见 [source-repair-static-audit.json](source-repair-static-audit.json)。没有生成或链接目标程序、运行测试、打开 App、操作 UI/音频设备或读取 GameData。

静态语法成功仅证明编译器可解析这些翻译单元。分配失败、真实资源兼容、JPEG 方向/色彩管理、文件读取策略原因、路径竞态、真实听觉/设备恢复和完整游戏行为均未验证；严格格式/长度/图素拒绝可能减少原浏览器接受的输入范围，需要后续自造边界与合法授权资产验收。仍存在有效 Bink/原媒体 ABI、完整业务/执行引擎和历史链平台拦截等阻塞。

可继续：自写 graphics/audio/video 的边界/生命周期实现、纯内存 TLV 拆分、独立运行时数据合同、构建/来源证据维护。不能继续：被拦截的原二进制转换/探测及其变相恢复、旧 Swift/原游戏或衍生库执行、城市逻辑分析；本轮也不能把未获执行授权的测试、UI、设备恢复记为已完成。验收目标以 [PLATFORM_AUDIT.md](PLATFORM_AUDIT.md) 的三项具体标准为准。

后续有限数据/TLV 源码继续推进，当前差异和来源核查见 [RUNTIME_PREPARATION.md](RUNTIME_PREPARATION.md) 与 [EXECUTION_SCOPE.md](EXECUTION_SCOPE.md)。本页是初次修复记录；它不独立产生永久测试禁令。

本页是初次源码修复快照；后续已按明确授权实际运行独立自造两配置，并修复新增实测缺陷。当前结果/哈希链见 [INDEPENDENT_VALIDATION.md](INDEPENDENT_VALIDATION.md)，原执行链边界未解除。
