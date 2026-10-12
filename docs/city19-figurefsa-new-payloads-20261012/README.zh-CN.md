# FigureFSA 新类与新对象 payload

在 `46893b8fa44a78fbc5e7a920ca03a2541a42b480` 的缓存回引用组件之外，新增独立的自写有界读取器 `walk_city19_figurefsa_exact_objects`。它覆盖下面六个确切类在 schema 0、自身版本 1 下的新类声明、新对象 payload、缓存类建对象及既有对象回引用。此前仅缓存回引用的组件和测试保持原字节不变。这里仍是显式名义类型合同，不能据此确认实际 holder+30 的动态类型。

## 具体类与最小布局

最初缺少 `FSAState`、`FSATransition`、`FSAActionContext` 的 factory→constructor→root→slot+8 绑定；继续 reader 后发现 `FSAAction`、`FSAContext`、`FSAMatch` 三个嵌套依赖。六条来源现已闭合。下表宽度包含自身 version word；名称为空、列表为空、引用采用两字节 NULL 时计算最小值。内存对象大小没有用于推导流宽度。

| 确切类 | 工厂 → 构造 | 所选 root → reader | 自身版本 1 的消费顺序 | 最小字节 |
|---|---|---|---|---:|
| FSAState | 4f1cd0 → 4f5270 | 7b3d5c → 4f5630 | word、名称、两个 ActionContext 列表、Transition 引用、Transition 列表 | 20 |
| FSATransition | 4f1dd0 → 4f5b30 | 7b3d8c → 4f6890 | word、Match 列表、byte、DWORD、8 字节、8 字节、State 引用、ActionContext 列表 | 33 |
| FSAActionContext | 4f2290 → 4f7970 | 7b3e74 → 4f7ae0 | word、Action 引用、Context 引用 | 6 |
| FSAAction | 4f2090 → 4f7670 | 7b3e58 → 4f78a0 | word、名称 | 6 |
| FSAContext | 4f2190 → 4f7540 | 7b3e38 → 4f75b0 | word、DWORD | 6 |
| FSAMatch | 4f2390 → 4f7b90 | 7b3ea4 → 4f7d00 | word、名称、DWORD | 10 |

名称另加对应字节长度；对象引用为普通 tag 的 2 字节或扩展 tag 的 6 字节。所有六类的根读取器 `40cf80` 不消费字节。Transition 的布尔字段原实现消费一个 byte 后在内存归一化；组件只保存原始区间。两个 8 字节字段也只保存区间，不解释值。来源、调用点和证据指纹见 [wire-contract.json](wire-contract.json)，没有把反汇编全文或原始数据复制到仓库。

新类声明格式为 `ffff + schema word + 名称长度 word + 名称字节`，长度 1–63；组件仅接受表中确切名称和 schema 0。原 lookup 的空名称、带 NUL 别名等并未建模。短 tag 的 class 高位及 `7fff + DWORD` 扩展均已覆盖。类在类型检查前登记，对象在 payload 前登记；重复类声明获得独立 ID，已有对象和正在读取中的对象均可回引用。ID 始终是现代元数据索引，不是原指针。整个缓存和游标属于本次调用，失败不会发布部分结果或改写调用者缓存。

列表使用有符号 32 位计数，非正数不迭代。名称按 4 字节非负长度读取原始字节，宿主终止 NUL 不在流中。上限为输入 64 KiB、单名称 8 KiB、每列表 256 项、1024 引用／缓存项、128 新对象、16 层新对象、1024 名称。回引用不重复进入 payload，不占新对象深度。

外层复用所选 `4f4480` 基类版本 1/2 和 `4dad10` 自身版本 2/3/4/5 的已保存顺序；三嵌入项仍为各 `word + 4 DWORD + 外层 flag = 19` 字节。没有接入实际文件资格入口，也没有调用原 factory、registry、reader 或应用回调。

## 验证与保留的失败

最终自造验证 **Release 22/0、UBSan 22/0，共 44/0**。涵盖六类声明、六类预置缓存的短／扩展 class tag、缓存类再建对象、`State → Transition → State` 回引用环、重复类声明、全部外层版本组合、逐字节截断、错误 schema／种类／类型／索引、预算、负计数、只读守卫页及失败后的状态保持。纯缓存分支与此前组件的结果进行了对照。全部输入为本次自造字节，原指令、转换产物、GameData 和实际文件均未执行或加载。

较早的三个主类验证为 28/0，不能替代最终六类结果。首次编译被 `-Werror,-Wrange-loop-construct` 拒绝，改为引用后通过；失败日志保留在任务目录。初次静态 session 的 seed TSV 列顺序写反，工具返回 `configuration_invalid`，按既有格式修正，未改护栏。ActionContext 的初次查询返回 `No procedure exists at 0x4f7ae0`；后续依据已保存 vtable slot，在新临时数据库中显式 seed 该入口后取得完整 reader。它是来源支持的元数据声明，不是运行时发现。详见 [validation.json](validation.json) 和 [剩余条件](remaining-gaps.json)。四个正常静态 session 均以官方流程关闭，输入与选定模块指纹不变，临时目录为空。

## 仍不能推导的结论

这六个确切类的版本 1 payload 已支持；未知派生类、新名称和其它 schema 仍明确拒绝。六个源 reader 对非 1 的自身版本跳过字段后复位版本，现代组件继续采用版本 1 子集并拒绝其它值；这是接收政策差异，不能说原实现同样拒绝。

实际 holder+30 的同一性、存活、应用回调替换和 root 保持仍未闭合。presence 非零以及 class 名称／schema／自身 word 都不能证明它们。原 registry 的当前内容、factory 是否成功返回所选 root，也仅作为显式条件，现代目录不调用原逻辑。普通合法内存／分配／容器合同不替代应用回调证明。

真实无条件边界仍为 **1991913**，新增实际解释 **0 字节**。本次没有真实数据实验，不宣称原 ABI、完整加载或游戏通过。后续最小工作是补实际对象身份／生命周期及回调写集合；若遇到未支持确切类型，再补其 descriptor、factory、constructor/root/slot+8 和 serializer 的来源链，而不是按内存大小跳过 payload。

本地基线的实际待推送链见 `remaining-gaps.json`；本次选择性提交后的完整链与新 SHA 位于任务目录 `local-commit-and-pending-push.json`。此前 GitHub 登录失效，未恢复认证前不尝试 push、改远端或读取凭据。
