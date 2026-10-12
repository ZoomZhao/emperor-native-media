# 独立数据模块与完整运行准备

私有远端 main 仍为已推送的 `2f3378a91fb0987288e00c61befa0ddeacbe50be`。后续全部修复在可审阅工作树，未自动提交/push。

上一轮实际变更为：`game_lab/emperor_assets.*`、新增 `wav_pcm.cpp`/`asset_pixels.cpp`/`asset_bytes.hpp`/`asset_limits.hpp` 修复解析与失败状态；`resource_root.*` 和 App/main 修复 cwd 路径与静音；`audio_policy.hpp` 统一音乐/SFX；边界测试/runner、Makefile/build/run 明确接入新源；README/GAPS/VALIDATION/平台审计修正证据范围。GameData 保留且忽略，未读取。

本轮在上述基础上完成以下已识别缺口：

| 实际改动 | 行为合同与自造回归 |
|---|---|
| `runtime_core/src/heap_native.c`、`include/heap_native.h` | 拒绝非零空输出/未初始化/poisoned 状态，严格 live 字段，null/poisoned export 安全；实现有限 heap 退役且不影响其他 heap；补 OOM 不变、错误回调后 poison、退役/double retire/dispose 代码用例 |
| `runtime_core/include/memory.hpp` | 拒绝 uint32 map API 无法表示的末端，失败 read/分配保留调用者输出；补范围溢出、零长度、独立重叠字节期望与无效构造代码用例 |
| `runtime_core/tests/boundary_tests.cpp` | 扩展现有 require 风格，覆盖上述实际接口；补 context 无效构造/失败输出与 error 保留；没有新测试框架 |
| `runtime_core/state_codec.py` | 从自写 TLV 数据段整理纯内存 API，保留标签并去掉 legacy/common/ROOT/HostStore/文件接口；新增 bytes/node/depth/int 预算、循环拒绝、packed 节点计数、重复容器拒绝、deque 容量不静默丢失 |
| `runtime_core/tests/state_codec_tests.py` | 标准 unittest，自造协议字节锚点、类型往返、全部截断点、巨量声明、预算、循环、重复/不可哈希、UTF-8/整数、hash、数据类型限制；不读取任何历史状态 |
| `runtime_core/run_boundaries.sh`、根 Makefile | 精确 Release/UBSAN 数据测试入口，isolated Python，独立证据目录和来源/产物/结果哈希；未执行 |
| `docs/EXECUTION_SCOPE.md`、本记录及 provenance/缺口文档 | 核对直接委托与适用 AGENTS，修正“静态任务即永久禁测”的误读；记录具体可定向验证范围和未闭合依赖 |

独立模块当前具体能力是有限 heap 所有权/退役、私有内存范围/字节操作、context-local 槽/error 和有界 TLV 内存编码；这些合同均已写源码。它没有 CPU、真实线程/异常/消息、游戏调用接线、城市业务或完整游戏引擎。

本轮 C/C++ `-fsyntax-only`、Python AST/import 文本及 shell `-n` 检查通过；未链接、运行或 import 测试/TLV，没有通过断言数。当前源码指纹和实际检查命令见 [runtime-preparation-static-audit.json](runtime-preparation-static-audit.json)。上一轮 [source-repair-static-audit.json](source-repair-static-audit.json) 是当时版本快照，其中 runtime 源已由本轮更新，不能用旧哈希声称新版本通过。

约束来源核查优先结果：当前仓库/任务目录的适用 AGENTS 均不存在；原委托的“不运行任何测试/二进制”属于文档审计直接范围，最新委托要求核清前不运行且不能自行解除真实限制。不是永久 AGENTS 禁令，也不是独立测试已经被自动审批拒绝。完整引文、查找位置和原文可访问性见 [EXECUTION_SCOPE.md](EXECUTION_SCOPE.md)。

最小待定向验证的命令为 `bash runtime_core/run_boundaries.sh release` 和 `bash runtime_core/run_boundaries.sh undefined`。精确源/依赖在 runner 的 SOURCES 与显式 compiler 行；只用标准 C/C++/Python 与当前模块、自造内存输入，排除 legacy/GameData/EXE/原库/UI/音频设备/网络/安装。当前仅准备，未执行。该组通过也只证明有限数据合同。

剩余依赖：当前纯 TLV 没有 common/ROOT；历史的14个暂留 import、1个歧义 import、6处生成绑定和3处第三方 ABI 未补造；23处自写共享头可定位但旧执行链未启用。完整游戏仍缺独立业务/执行引擎、原平台语义、有效 Bink、真实资产/App/设备和端到端验收。原二进制拦截不变，不以这些自写模块恢复原转换/探测或衍生执行。

## 后续已实际验收

最新委托明确撤销早期审计的独立禁测，有限数据/TLV Release/UBSAN 已通过，并捕获实际输入、源、二进制与结果哈希。本页先前“候选/尚未运行”属于准备时状态；当前结果及实测修复见 [INDEPENDENT_VALIDATION.md](INDEPENDENT_VALIDATION.md)。未扩权原链/UI/设备/真实资产。
