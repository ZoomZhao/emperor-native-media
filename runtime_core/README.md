# 独立有限运行时数据组件

本模块仅处理自写数据模型，未接入 App、历史执行链或原游戏。最新明确授权后已构建并执行自造 Release/UBSAN 与 TLV 验收。

| 组件 | 具体能力 | 明确限制 |
|---|---|---|
| `include/heap_native.h`、`src/heap_native.c` | 零初始化导入、重复/无效元数据拒绝、8 字节对齐分配、有限 OOM、跨 heap 所有权、realloc 保留前缀、有限 heap 退役、live 元数据导出、幂等 dispose | 从自写 stage77 模型整理；固定逻辑整数区间，不对应原游戏映射。退役只使该 heap/记录失效，不回收 bump 字节，不宣称 Win32 HeapDestroy 等价 |
| `include/memory.hpp` | 新建清零的私有 vector，容量≤32 MiB；连续映射前缀；完整 read/fill/copy 范围预检；重叠 memmove；失败读取保留输出；成功写计数；heap 回调适配 | 没有 mmap、可执行权限、外部文件/快照加载或宿主指针重定位。独占对象不能复制/移动；容量终点须能由 uint32 表示 |
| `include/thread_state.hpp` | 最多 64 个明确 context，每 context 64 个槽与独立 error；明确分配/释放/重置槽；失败选择/get 保留状态/输出 | 仅 context-local 数据，select 不是 CPU/OS 线程切换；没有 opaque CPU、调度、SEH 或消息循环，也不是完整 Windows TLS ABI |
| `state_codec.py` | `dumps(value) -> bytes`、`loads(bytes, expected_sha256=None) -> value`；保留自写 S83H v2 TLV 标签；明确 bytes/节点/深度/整数预算；循环、重复 dict/set、非法 UTF-8、deque 溢出与截断拒绝 | 只接收精确内建类型，无 pickle/对象 hook、文件路径、ROOT、HostStore 或旧 manifest 加载。共享引用解码为数据副本；set 编码不承诺排序或 canonical hash；限额是编码数据预算，不是进程 RSS 硬界 |
| `tests/boundary_tests.cpp`、`tests/state_codec_tests.py` | 已写自造分配/范围/OOM/回调故障/heap 退役/context 边界和 TLV 协议锚点/格式失败/预算用例 | 沿用现有 require 与标准 unittest；两配置各91断言通过；TLV 两次各11标准测试通过 |

heap 调用者须用零初始化的、单一拥有的 `S77State`，回调指向的 Memory 必须活得更久。模型检查结构字段与范围，不能证明任意伪造 C 指针有效。任何 map/fill/copy 失败会 poison 状态，后续数据操作拒绝；只能 dispose 后重新初始化，不能把失败后的字节和元数据当事务已回滚。`export_live` 对无效/poisoned 状态返回 0 且不写输出，0 不单独证明 snapshot 有效。

当前 TLV 默认 16 MiB 编码字节、100000 个值（packed uint32 元素也计入）、64 层深度、4096 位十进制大整数；可在函数参数中缩小/有限扩大，硬上限为 64 MiB、1000000 个值、256 层和4096位。没有修改 Python 的全局整数限制、环境或系统配置。支持 None/bool、uint32/int64/有限大整数、float64、bytes/UTF-8 str、dict/list/tuple/set/deque。协议保留不代表历史任意状态兼容；本模块只接受自造/调用者明确提供的内存数据。

来源与当前源码哈希见 `provenance.json`，历史 include 分类见 `dependency-map.json`：23 处共享头文件可定位，6 处生成绑定和3处第三方 ABI 仍缺失。14 个暂留 Python import/1 个歧义 import 仍属于归档；legacy 没有修改或启用。纯 TLV 的历史 `common.ROOT`/HostStore 依赖已从当前模块移除，不改变旧档案。

`run_boundaries.sh release|undefined` 是待定向验证的精确入口，只编译当前 C/C++ 数据用例，并通过 isolated Python 执行当前 TLV unittest；没有测试发现、GameData、legacy、UI、音频、网络、安装或 push。本次未自动执行通用入口，而用任务目录控制器的明确同源编译命令落实60秒期限和输出范围。它按独立目录保存源码/构建/结果哈希，两条必需测试均成功且源码未变才接受结果；失败保留日志并非零退出。

不运行测试的来源及可定向批准的最小范围见 [EXECUTION_SCOPE.md](../docs/EXECUTION_SCOPE.md)。既有静态任务范围不解释为永久禁测；原二进制链持续限制不变。当前源版本静态证据见 `runtime-preparation-static-audit.json`（本地历史报告；未纳入本次提交）。

当前运行证据、输入留存和结果 SHA 见 [INDEPENDENT_VALIDATION.md](../docs/INDEPENDENT_VALIDATION.md)。早期 static JSON 保留历史源版本；现有实际结果以新 receipt 为准。

## 当前 portable 复现入口

`bash runtime_core/run_boundaries.sh release --output /absolute/new-output` 和 undefined 统一调用 scripts/independent_runner.py。只引用本模块/标准库及明确测试helper，冻结源码、启用实际输入留存并限每进程60秒；在干净导出数据各91断言、TLV各11测试通过。当前证据见 [portable-reproduction-receipt.json](../docs/portable-reproduction-receipt.json)。heap/Memory、Contexts、TLV仍无合理的App业务消费者，不为了接入而新增依赖；需求复核见 [BLOCKERS-AND-HANDOFF.md](../docs/BLOCKERS-AND-HANDOFF.md)。
