# 独立自造输入验收结果

本轮已按最新委托解除“文档审计期间不运行独立测试”的临时限制，实际完成以下构建与运行。原 EXE/SR/Unicorn/Wine/衍生库、GameData、legacy、App、UI、音频设备、网络和安装均未进入本轮。

| 当前源码与配置 | 实际结果 | 覆盖范围 |
|---|---|---|
| runtime_core Release / UBSAN | 各 91 项断言通过 | 有限 heap 导入/所有权/OOM/realloc/退役/poison、私有内存范围和 context-local 状态 |
| 纯内存 TLV，两次独立执行 | 各 11 个 unittest，0 failure / 0 error | 字节锚点、类型、截断、预算、循环、重复/不可哈希项、deque 容量、UTF-8、大整数和 hash |
| 资源纯逻辑 Release / UBSAN | 各 65,691 项断言通过 | 完整 65,536 个 RGB555 字值（含高位别名）、WAV/扩展长度/格式端点、JPEG 完整/失败/尺寸、明确资源根/路径和无设备静音策略 |

扩展回归实际发现并修复两项缺陷：WAV 接受半个或缺失 body 的 WAVEFORMATEX 扩展；JPEG 缺少完整终止标记却被 ImageIO 容错恢复为有效图像。修复增加完整扩展长度/体校验，以及严格 JPEG EOI 与 ImageIO 完成状态检查，未放宽回归合同。原失败源/输入/二进制/结果保留，分别为 `media-release-pftl04g0`、`media-release-1y8f_hrd`；基础旧通过记录不作为最终版本通过。

最终接受的证据目录 ID：`data-release-2z3u6zoi`、`data-undefined-k09j957t`、`media-release-ct1qbnp8`、`media-undefined-a4vpacet`。完整证据留在当前任务目录的 `independent-validation/`，本仓库 [independent-validation-receipt.json](independent-validation-receipt.json) 保存索引与 SHA-256。每项验证了当前仓库/冻结源码、实际输入与调用参数留存、构建二进制、进程日志和结果的哈希链。

使用任务目录的 `validate_independent.py` 明确编译/执行当前白名单源码，未自动调用全量 runner/Makefile。每个构建/测试进程 60 秒期限，所有自造文件、临时文件、日志、源码快照和产物写入任务目录；最长实测单进程 4.152 秒。Release/UBSAN 使用同一输入合同，UBSAN 失败不可恢复。`game_lab/pure_boundary_tests.mm` 专门排除旧资源测试中的 SG3/map，测试不启动 App 或连接音频设备。原通用 runner 没有作为本次输出范围/期限的凭据。

边界：UBSAN 只覆盖此次编译的自写代码，未给系统 ImageIO 库加插桩；没有注入系统 malloc/CGContext 工厂失败；静音仅为纯策略，App selector/音乐与 SFX 真播放器未运行；严格 JPEG 无尾随数据、PCM 扩展合同尚未验证真实资产兼容；RGB555 字域验收不覆盖随机 stride/裁剪/缩放/lifecycle、Metal、drawable/vblank；没有真实听觉/物理恢复、有效 Bink 或完整游戏引擎/端到端验收。

所有新修复保留在工作树，未自动 commit/push，GameData 忽略不变。历史归档保持原样；共享头可定位、缺失生成绑定/第三方 ABI 的分类没有变成旧执行链授权。

## 仓库入口复现更新

原 task-only 控制器的固定机器路径不作为交接入口。当前 scripts/independent_runner.py 与三个wrapper已修复，在干净导出实际复现相同数据/TLV/媒体合同，并完成App无UI集成与只构建验收。runtime_core旧receipt中的旧wrapper指纹属于历史版本；当前runner证据使用 [portable-reproduction-receipt.json](portable-reproduction-receipt.json)。完整状态见 [BLOCKERS-AND-HANDOFF.md](BLOCKERS-AND-HANDOFF.md)。
