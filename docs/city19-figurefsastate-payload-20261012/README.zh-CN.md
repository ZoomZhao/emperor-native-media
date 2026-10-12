# FigureFSAState 源链与同一条件实验

基线 `fe09b158e410c232b6820932a6d9dc878d11a976`。根据此前条件声明继续官方纯静态来源，确认 FigureFSAState 的完整 descriptor/base/factory/constructor/root/getter/serializer 链，并只扩入该已证明类型及版本布局。没有执行原指令、转换产物、原 factory 或 callback；没有 UI、设备、网络或认证操作。

## 源链和兼容性

`FigureFSAState` 的定义字符串位于 **84d48c**，名称 data xref 指向 descriptor **84d310**。descriptor 的 schema 为 **0**，factory **4db820**，base **850230（FSAState）**，内存大小84；内存大小没有用于推导流宽度。factory 的正常成功路径调用 **4db8b0**，先调用 **4f5270** 基类构造，再安装 root **7b0034**。

该 root 的 slot0 是 **4db880**，静态完整体返回 descriptor84d310；slot+8 是 **4dbaa0**。保存的 `77bb42` 通过对象 virtual0 获取 descriptor，再调用 `77bbc0` 沿 descriptor+10 比较 base。因此现代模型只增加单向 `FigureFSAState → FSAState` 兼容关系，保存具体 class/object 类型；不会把具体类型改写为 State，不允许反向或作为 Transition 接收。

两个初次函数查询返回 `invalid_request / No procedure exists at the requested address`，地址为4db880、4dbaa0，记录保留。随后根据已保存 vtable 槽，在新临时数据库显式 seed 入口取得完整体。seed 是来源支持的元数据声明，不是运行时发现；工具和护栏未改。两会话都官方关闭，输入与选定工具指纹不变，临时目录为空。完整来源摘要见 [source-contract.json](source-contract.json)。

## 明确字节布局

serializer **4dbaa0** 先调用 **4f5630**，消费 FSAState 基类。现代模型接受基类自身版本1：名称、两个 ActionContext 列表、Transition 引用与 Transition 列表，然后读取 FigureFSAState 自身 version word。

| 自身版本 | 读取 byte 的目标顺序 | byte 数 |
|---|---|---:|
| 1 | +4a…+4f | 6 |
| 2 | +4a…+50 | 7 |
| 3 | +4a…+51 | 8 |
| 4 | +4a…+51、+49 | 9 |
| 5 | +4a…+51、+49、+48 | 10 |

顺序来自完整 Listing 和4dbce4的五项跳转表。自身部分是2字节 word 加6–10字节；基类仍有变长字符串与引用，不按固定84字节跳过。`503d50` 消费单个 byte 后在宿主内存归一化，组件只记录原始字节区间。其它自身版本在原实现跳过字段并复位5；现代仅接收已证实1–5，不能声称原实现也拒绝。类 schema0、基类 version1 和自身 version1–5 分开验证。

仅新增确切 FigureFSAState 名称/schema0，其它六类及其原子读取规则保持。未知名称、未证明兼容关系、子集外版本和不透明前缀类型引用继续停止，不猜读。

## 自造混合验证

**Release34/0、UBSan34/0，共68/0**。包括此前28组，以及具体类型保留、单向兼容与 class/object 关系检查、全部五个自身版本、与基类 State／六类混合新对象、缓存类再建派生对象、环与后续回引用、cache6020编号、逐字节截断、schema/base/own独立性、晚期失败保留此前输出与 cache。前缀与嵌套预算保持不变，Clang实际依赖路径／SHA已核对。旧回引用头与原测试保持原字节。详见 [validation.json](validation.json)。

## 同一具名真实字节条件实验

保持实验名 `city19_first_holder30_root7b5bc8_six_exact_classes_cache6020`，该历史名称未改；本轮目录修订仅加已证实 FigureFSAState。固定文件、输入／解码 SHA、前缀cache6020、holder假设、65536字节视图及预算均相同。假设51209d时+30非NULL且选择root7b5bc8/reader4dad10，正常回调不额外消费字节；没有验证这些运行时事实。

结果：首个 FigureFSA 记录在此条件下完整，停止在下一消费者之前。

| 指标 | 结果 |
|---|---:|
| 条件起点 | 1991913 |
| 条件前沿 | **1992063** |
| 新增条件解释 | **150字节** |
| 无条件前沿 | **1991913** |
| 新增无条件解释 | **0字节** |
| 条件缓存 | 6020 → 6022 |
| 新类／新对象／引用 | 1 / 1 / 4 |

新增 class ID6020、object ID6021，按源链顺序登记。没有读取下一消费者，没有保存原／解码字节或字段值，输入只读且身份不变。详见 [conditional-experiment.json](conditional-experiment.json)。匹配证明字节合同在所列假设下与样本一致，**不证明实际 holder 身份、registry、当前 vptr 或完整游戏加载**。

下一步应从条件位置1992063核对精确下一消费者及其类型／版本前提，或继续实际holder+30存活/root与回调改写证明；不能把条件前沿替换成无条件前沿，不能自动套用到其它holder。所有九个既有本地提交和用户九项改动保留。本批只本地提交，认证未恢复，不 push。
