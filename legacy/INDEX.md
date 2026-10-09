# 历史自写工程模块索引

这些文件是文本归档，保留原阶段和相对目录，便于追溯；未接入根 Makefile，也不是可直接运行的历史 checkout。占位路径、未复制的配置/生成绑定/快照会使部分历史导入或构建依赖缺失。不能把“有源文件”记为模块已移植。

| 模块 | 代表源码（相对于 `legacy/src/`） | 可复用部分与限制 |
|---|---|---|
| 最小自写状态样例 | `samples/guest_core.c` | 自写 step/save/restore 样例；不含原游戏业务引擎 |
| 基础平台/CRT 状态 | `samples/stage10_native_platform.c`、`samples/stage11_native_runtime.c` | 有界指针、有限平台 API、heap/TLS/LastError；不是通用 Win32/CRT |
| 时钟、事件与环境 | `samples/stage13_clock.c`、`samples/stage13_events.c`、`samples/stage13_directory.c` | 自写单调时钟、事件与受限目录适配；历史参考依赖另列 |
| 窗口/像素/资源接口 | `samples/stage23_bridge.c`、`samples/stage23_metal.m`、`samples/stage24_bridge.c` 及 stage26–35 文件 | 原生像素存储、Lock/Unlock/Blt、窗口和图像桥；部分桥依赖未迁移的转换函数/资源，未作为当前后端启用 |
| 音频、sample、stream | `samples/stage37_audio.c` 及 stage36–46 音频文件 | 自写 FIFO、句柄、停止/恢复与流适配；曾连接设备的代码保持归档，本轮没有运行 |
| 影片失败边界 | `samples/stage42_bink.c`、`samples/stage42_bink.py` | 有限 factory/缺文件/明确拒绝合同；没有完整有效 Bink 解码器 |
| 输入/窗口辅助接口 | stage43、stage59、stage66–67 自写文件 | 有限输入模型、消息/IPC 桥；不代表完整物理输入或消息泵 |
| 分析/构建配置工具 | stage74–76 的 `tools/`，相关 `samples/` 工具 | 自写配置、依赖/flags/闭包和构建组织源码；原提取结果、配置与第三方转换器未复制。只归档，不运行 |
| heap 与原生状态 | `stage77/tools/heap_native.c`、`heap_native.h` | 分配所有权、realloc/size、状态导入导出；仍需提炼成无历史依赖组件 |
| 有界内存/辅助操作 | `stage78/tools/guarded_helpers.c`、`stage79/tools/guest_memory.c` | 权限、区域/溢出边界、有限操作；不能泛化为完整异常语义 |
| 线程/public CPU 契约 | `stage79/tools/thread_contract.py`、`thread_native.c` | 有限线程/TLS/公开状态；与旧快照/引擎依赖未解除 |
| Runtime 与地址域 | `stage80/tools/runtime.c`、`runtime.h`、`sparse_memory.c` | 自写连接层、分离地址与状态管理；生成目标域绑定留在旧目录 |
| 整数异常边界 | `stage81/tools/idiv.c` | 自写整数语义/提交前拒绝；没有完整 SEH dispatch/unwind |
| 平台模型/冻结/比较 | `stage82/tools/platform_model.py`、`platform_native.c`、`compare.py`、`freeze_request.py` | 自写有限模型、冻结与完整状态/有序写比较；原自然输入和完整游戏行为未通过 |
| 执行门与输出审计 | `stage83/tools/write_gate.py`、各版本 control 工具 | 自写预算、输出范围和角色状态管理；旧批准/claim 不随归档成为新执行许可 |
| TLV/HostStore 与 role 管理 | `stage83/repair-v2/tools/binary_host.py`、`role_process.py`、`pipeline_final.py`、`engine_final.py` | 自写格式、惰性字段引用、角色进程边界；final 版本优先追溯，其他版本保留历史。不加载旧快照或引擎 |
| 线程/表面/回调模型 | `stage83/phase7-v1/tools/`、`phase7-v2/tools/`、`phase7-probe-v1/tools/` | 自写有限模型和夹具，部分依赖 Unicorn；原动态回调域未闭合，当前不运行 |

总计 751 份源码，3,525,452 字节。自动用途提示为：平台/运行时 127、测试/诊断 155、分析/构建 101、验证/归档控制 114、其他研究工具 254。提示是索引标签，不能用这些数字推断游戏业务代码量或通过率。

每份文件的来源相对路径、来源 SHA-256、副本 SHA-256和路径脱敏操作见 [import-manifest.json](import-manifest.json)；副本校验值见 [FILES.sha256](FILES.sha256)。未脱敏的副本与来源逐字节一致；脱敏仅发生在副本。41 份暂留源码的原因逐项记录，不声称这些内容全部属于原游戏或全部不可迁移。

原报告不整份复制，避免携带私人执行记录、原字节及失效运行命令。本轮归纳见 [平台审计](../docs/PLATFORM_AUDIT.md) 和 [GAPS](../GAPS.md)，每个报告/结果的逻辑定位与 SHA-256 见 [证据索引](../docs/audit-evidence.json)。原绝对路径只在本地忽略的 `.local/legacy-dependencies.json` 中保存。
