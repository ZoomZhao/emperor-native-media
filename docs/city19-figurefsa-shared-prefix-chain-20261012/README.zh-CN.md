# 同一 FigureFSA 链、cache6020 与具名条件实验

基线 `b40718a44dea33a54a11e87c90f99ccbb30800bb`。原六类组件已经在同一游标／缓存内读取 FigureFSA 外层、新类声明、新对象 payload 和回引用；本轮补充完整前缀缓存接续，并做一个明确假设类型的真实字节一致性实验。原缓存回引用 API、头文件和原测试保持原字节。

## 初始化对象对应与选择依据

首个 `511ea0` holder 的 `+30` 经 getter `5122c0` 返回，NULL 检查后才在 `51209d` 调 virtual+8。所选构造链 `510d30 → 512230 → 514500 → 512291` 支持一个名义 `+30` 候选：root `7b5bc8`、reader `4dad10`。它是外层 FigureFSA 候选，不是以下六类中的一个对象。实际 holder 同一性、对象存活、回调改写、当前 root 和 registry 内容都没有因此证明。

| 六类名义对象 | 构造 / root / reader | 结构位置与构造初值 |
|---|---|---|
| FSAState | 4f5270 / 7b3d5c / 4f5630 | 外层 FigureFSA 的 +8、+10、+c 引用，以及 +88 的 State 容器。State 自身 +14 是 Transition 引用，+18/+28 是 ActionContext 列表，+4 是 Transition 列表，+38 是名称。 |
| FSATransition | 4f5b30 / 7b3d8c / 4f6890 | State 的 Transition 引用／列表；自身 +14 是 Match 列表，+34 是 State 引用（构造置 NULL），+4 是 ActionContext 列表。+38/+3c/+40/+48 对应所选 byte、DWORD、两个 8 字节字段。 |
| FSAActionContext | 4f7970 / 7b3e74 / 4f7ae0 | State 的 +18/+28 列表及 Transition 的 +4 列表；自身 +4 Action、+8 Context 均构造置 NULL。 |
| FSAAction | 4f7670 / 7b3e58 / 4f78a0 | ActionContext +4；自身 +4 名称。 |
| FSAContext | 4f7540 / 7b3e38 / 4f75b0 | ActionContext +8；自身 +4 DWORD 构造置零。 |
| FSAMatch | 4f7b90 / 7b3ea4 / 4f7d00 | Transition +14 列表；自身 +4 名称、+14 DWORD。 |

依据是已保存工厂→构造→所选 root→slot+8→类型化读取包装的来源链，详见 [对象对应表](object-mapping.json) 与上一批 [六类合同](../city19-figurefsa-new-payloads-20261012/wire-contract.json)。表中对应的是这些 factory 被选择时的名义对象与字段，不证明原启动路径已经创建全部六类或挂在当前 holder 上。不同对象的 +14、登记节点的 +14 不能视为同一指针。

## 有界缓存接续与失败原子性

新增 `walk_city19_figurefsa_with_archive_prefix`，显式指定原前缀条目数。保留前缀的全局 ID、种类、class 关系与 schema，不重编号，不把未建模类型改称 FSA。未建模类型以 `OpaquePrefix` 携带；FSA 字段引用它们时明确停止。新类／对象追加在原编号之后，所以 cache6020 对应首个新类 ID6020、新对象 ID6021。后续记录可传入同一结果缓存继续回引用。

此前普通六类 API 的总缓存上限 1024 不变。新接续入口单独限定总缓存 ≤8192、FSA 追加缓存 ≤1024；单次记录的新对象 ≤128、嵌套深度 ≤16、引用 ≤1024、每列表 ≤256、输入视图 ≤65536 字节。所有解析状态均在本次调用内，失败不会改调用者 cache 或先前已提交输出。失败诊断只返回尝试位置和未知声明区间／schema，没有返回半成品缓存；尝试位置不作为前沿。

最终自造链验证 **Release28/0、UBSan28/0，共56/0**。在原六类22组之外，验证全局6020编号、下一记录沿用缓存、对不透明条目的拒绝、嵌套／缓存预算、晚期失败保持既有输出，以及未知声明诊断区间。原回引用 API 另复跑 **14/0 + 14/0 = 28/0**。实际编译依赖按 depfile 核对，旧 API 头字节不变。

## 具名条件样本：停止在 FigureFSAState 声明

实验名：`city19_first_holder30_root7b5bc8_six_exact_classes_cache6020`。

固定实际文件 SHA 与既有完整输入／解码 SHA 一致；现代自写 framing／zlib 路径校验136块、解码4430995字节，并按既有有界前缀组件重建 cache6020、核对首个 holder 已限定35字节前缀结束于1991913。没有运行原 reader、工厂、回调、指令或转换产物。

明确假设：`51209d` 调用时 +30 非 NULL、root7b5bc8／reader4dad10；所选正常回调不增加字节读取；只接受六个确切 schema0／自身版本1 的 payload。只给该候选65536字节视图，尝试首个记录；不尝试其它候选或未知版本，不读下一个消费者。

结果：遇目录外类声明立即停止。声明 tag 位于 **1991925**；schema0；名称位于 **1991931**，长度14，读至 **1991945**。名称 SHA `e09b7db20d233b433642535b2fcebe8db48e6b97cd2fe635d04482d5cfe57ee0` 与静态候选字符串 `FigureFSAState` 一致。此名称对应关系在条件布局下成立，不证明其原 descriptor、继承、当前工厂或 vptr。没有读取该类 payload、追加缓存项或发布部分记录。

| 项目 | 结果 |
|---|---:|
| 无条件前沿 | 1991913 |
| 条件已提交前沿 | 1991913 |
| 尝试停止位置 | 1991945 |
| 新增条件／无条件解释字节 | 0 / 0 |
| 最终提交缓存项 | 6020 |

声明诊断的32字节尝试进度不能当作完整记录通过。初次运行保留；后续同一模型仅补声明区间／指纹诊断，停止点不变。原／解码字节、字段值没有保存，输入只读且指纹不变。详见 [条件结果](conditional-experiment.json) 与 [验证回执](validation.json)。

下一步最小缺口是 **FigureFSAState 的 descriptor/schema/base 兼容性、factory→constructor/root→virtual+8 serializer，以及基类／自身版本和嵌套字段的完整来源**。本轮没有扩大六类目录、猜读其 payload 或用名称匹配证明当前 holder 身份。代码与报告仅本地提交，认证未恢复，不 push。
