# 依赖与执行边界

| 范围 | 依赖 | 整合后的状态 |
|---|---|---|
| 当前媒体组件 | macOS 15+，已安装 Apple Command Line Tools/Xcode、clang++、Python 3、make、xcrun、codesign、plutil；AppKit/Metal/AVFoundation/CoreMedia/CoreVideo/CoreAudio/ImageIO 等系统框架 | 原根 Makefile 与入口保留；自造素材仍由本仓库生成，不从 GameData 取输入 |
| 历史 C/Objective-C 平台源码 | Apple SDK/系统框架，历史头文件/有限接口及生成绑定 | 文本归档；部分头文件、生成绑定未复制，没有新增旧构建目标 |
| 历史分析/差分驱动 | Python，历史 Unicorn 2.1.4、既有 SR/SRW/llasm/LLVM 工具，部分参考曾用 Wine | 第三方实现、安装包和运行环境留在原位；没有安装、转换、探测、导入引擎或运行历史工具 |
| 原行为输入 | 原 EXE/DLL、资源/存档、提取指令、自然输入、完整 memory/context/host 快照、生成 IR/C/绑定及衍生游戏库 | 不复制、不提交、不加载；路径引用保存在忽略的本地配置，不构成可执行依赖已齐全 |
| 验证证据 | 旧报告/结果、原日志与 claim/审批/账本 | 原件保留；公开工程文档只保存归纳、相对定位及哈希。旧批准不能作为本轮运行许可 |
| 当前本地 GameData | 用户放置的目录 | 保留且被 Git 忽略；本轮没有遍历、读取、哈希或判定其内容完整性 |

本地 `.local/legacy-dependencies.json` 记录原任务根、历史产物分类路径、暂留源码及 GameData 路径。机器绝对路径仅在该忽略文件中出现；归档中 `<LOCAL_LEGACY_TASK>`、`<LOCAL_ORIGINAL_PROJECT>`、`<LOCAL_MACHINE_PATH>` 是标记，不是已实现的运行配置。没有把旧路径自动替换为可执行的新路径。

当前媒体构建/测试的既有使用说明保存在 [MEDIA.md](MEDIA.md)。本轮没有调用这些入口；它们的历史结果与新代码边界见 [VALIDATION.md](../VALIDATION.md)。原先 AX 自身窗口入口依赖既有权限，也没有在本轮调用；音频默认静音。

历史归档中的 Python 顶层可能加载 native 库、引擎、运行探针或创建窗口，所以本次只用 AST 解析文本，未 import。C 头文件/链接依赖扫描也是文本检查。语法可解析不等于构建成功；构建成功不等于行为等价；历史有限原函数差分也不等于整游戏通过。

二进制转换/探测工作曾被平台安全拦截，整合目录不会解除该边界。本轮不恢复、不变相继续或绕过历史执行链；不读取 EXE/反汇编、不调用 SR/Unicorn/Wine、不执行游戏或衍生库、不分析城市逻辑。未来可继续的工程工作应使用完全自造数据，在独立组件中进行。


## 当前自写模块整理（2026-10-10）

`game_lab` 的 PCM 与 RGB555 内存解析分离为纯 C++ 翻译单元，App 和旧资源测试构建脚本已显式列入；resource_root 只由 App/自造边界用例使用。App 移除未使用的 Metal/MetalKit 链接项，仍使用 AppKit/AVFoundation/ImageIO/CoreGraphics。初次整理没有做链接/打包/运行验收；当前 App 已只编译/链接/签名，见下节。

`runtime_core/include`、`src`、`tests` 的依赖仅为标准 C/C++ 与明确本地头文件，静态语法检查通过。私有 vector 内存、有限 heap 与 context 数据模型不引用旧外部文件加载器、CPU ABI、引擎或 GameData。详细合同见 [runtime_core/README.md](../runtime_core/README.md)。

历史 32 处 include 解析缺口已细分：23 处能定位自写共享头文件，6 处缺失生成绑定，3 处第三方 ABI。映射及 provider 哈希见 `dependency-map.json`（本地历史报告；未纳入本次提交）。这是来源/路径分类，不改变 legacy/FILES.sha256，不创建 build_scope 等绑定，也不启用旧驱动。14 个暂留 Python 模块依赖和 1 个歧义 import 仍保留为归档问题。该次初步整理时纯 TLV 尚未抽出；后续已整理 runtime_core/state_codec.py 纯内存 API，移除 common/ROOT/HostStore/文件接口。原 HostStore 没有导入或启用。

当前有限数据/TLV 的精确自造验证入口为 runtime_core/run_boundaries.sh，只引用标准库与当前模块，尚未构建/执行。源/依赖和范围见 `RUNTIME_PREPARATION.md`（本地历史报告；未纳入本次提交）；生成绑定/第三方 ABI 与原游戏执行仍不是可用依赖。

## 当前 App 实际集成与仅构建验收

2026-10-10：AppMedia 已成为真实 App 与无 UI 测试共用的控制实现，补齐 App 音效到当前 WAV 校验、失败替换清理、默认静音/gain/显式播放/终止，以及 JPEG/raw555 的资源路径接线。Release/UBSAN 集成各 62 断言通过；当前 EmperorApp 编译、链接、打包和本地 codesign strict 校验通过，包内保存生产源码指纹，旧包保留。未启动 GUI、真实 AVAudioPlayer 或设备，未读取 GameData/legacy。

真实依赖清单、当前包证据和限制见 [APP_INTEGRATION.md](APP_INTEGRATION.md)。runtime_core 仍没有 App 消费者；音频队列/视频/Metal demo 未被接入 EmperorApp，完整业务引擎与状态/调度/交互/存档合同仍缺失。先前“只静态/未构建”是当时记录；当前新包与无 UI 集成已实际验证，不能推广为完整游戏或声音/GUI通过。新修复未 commit/push。

当前依赖需求复核与干净目录复现见 [BLOCKERS-AND-HANDOFF.md](BLOCKERS-AND-HANDOFF.md)。没有把 runtime_core 或离线队列事件硬接到 App；不存在真实逻辑地址/context/存档消费者，AV后端事件源也未定义。三个 runner 已统一为相对工程路径/明确输入/60秒期限，无历史机器路径依赖。
