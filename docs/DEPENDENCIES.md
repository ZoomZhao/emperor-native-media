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
