# 原版静态路线 handoff

## 最新范围：通用修复验收完成，原路线继续暂停

固定修复 `5d49c1a322437acfe21324f2997a36962fdfcc4d` 的限定独审完成，两项问题可关闭。初始cache预算独立Release6/0、UBSan6/0，涵盖prefix1/6020追加1024与1025、完整output/cache/input失败不变；五driver的两模式10个通用探针确认单／双引号、空格、`#`、`$`路径和实际依赖SHA。[修复报告](city19-budget-and-make-depfile-fix-20261012/README.zh-CN.md)

作者200/0未全套独立复跑，独审第五driver只用通用probe，没有运行其原FigureFSAState测试。该验收不覆盖0736来源、类型身份或真实条件实验。0736独审未完成；后续原来源扩展、12项条件链和真实实验继续暂停。旧作者条件前沿1992063、无条件前沿1991913保留为原有标记，本次无新增实验或前沿。

另一阻塞是GitHub认证，仍待用户恢复；不尝试认证、push或更换远端。现有本地提交及工作树改动保留。以下均为历史来源与计划，不自动授权恢复受阻路线。

## 保留的历史handoff

本地基线82d9cf50d00e887bbed8cfecb2aa8d7fee03c69a及父修正9beed221已独立38/0，旧b2 38/1收据保留、SPEED-NULL失败闭合；descriptor明确0x928=2344bytes。新[前置及派生100](ORIGINAL_PRESELECTION_DERIVED100_CONTRACT.md)待独立复核。旧[owner34/1c8](ORIGINAL_OWNER34_DERIVED_POSTLOAD_CONTRACT.md)、[读后调度](ORIGINAL_POSTLOAD_SCHEDULER_ENTRY_CONTRACT.md)、[版本载入](ORIGINAL_VERSIONED_CITY_LOAD_CONTRACT.md)前提保留。

三个selectedcaller固定408ac0=10bc348；10bc37c空xref不是无store证明。前置500400/5003a0/413a40/5032c0确切写linked170/2c4、owner0及linked2e0，4fca20清owner10；生产owner34非零的store仍未找到。5003a0后的4ff170和conditional4ff760仍构造/登记到58ee60，空分配/异常不补造正常成功。

4fb8c0→62e720→global1e551e4对象的61b510有46完整native节点：引用计数、f8/fc循环currentvirtual4、dc恢复、捕获100返回与条件virtual0析构。62e7f0的两66ce40是真JMP current8，62db60可tailcurrent44，not-call不意味着没有作用。4fae70分配dc/6c4a80 root7c8344 widget，不能把它默认当访问148的modalreceiver。没有原或UI/OS执行。

只所选4c5dc0root的100=4c69a0已取84Listing/23关键native，RET18确认六参数、AL1不是全EAX1。固定25c4f13f0四native取signedO14的24字节步长表值，bounds未知；1e8=416b50两native返回O+c8，条件下写O128=101c500低byte。Normalcallee ABI偏移账本把[ESP64]映射arg5，postload传0因而跳过两26c，不能推广到其它路径/实际vptr。具体helper的索引数组/错误字段写已保存，不解释城市模拟逻辑或声称gate稳定。

139/138/1，官方exit0/746.741秒、源与18模块相同/临时空/0活动会话。三fullnative图52节点与另84Listing仅23native分开计，80native匹配Listing；CALL条件正常返回，loop/currenttarget/异常/保活不闭合。源码合同/文档校验不等于原游戏验收，binding DENIED。

继续三个精确任务：

1. **非零owner34生产源**：global1e551e4写来源与匹配的current4 handler、选择结果的作用。通过：确切store/receiver/每次guard前覆盖及正常/异常条件，不能用空xref或无localstore证明恒零。
2. **所选100写后到门控**：按4c69a0各helper/lookup确切字段与indexedwrites追gate/quota/hold alias。通过：固定root/callsite/ABI和传递写集合，rawcursor/实际输入范围仍未知时不能开运行。
3. **登记/存活/异常**：4ff170/4ff760/58ee60构造/容器/OS边及既有rawcursor。通过：失败返回/清理/异常/alias来源有证据，不凭局部无free说保活。

两owner34guards仍独立读取，未找到实际变值store亦未证明不变；reset0/0/0/1仅条件分支。原28后hooks、pretick四helper/tail、buffered四缺边、append双编号/失败、Q274/cc/P150、OS异常/handler重入仍保留。实际载入、首次新状态tick、完整周期/可玩未证明。

不执行原/衍生游戏、转换/SR/Unicorn/Wine或历史被拒探测，不读GameData/旧Swift，不运行组件或UI/设备；没有本批网络/push/替代上传。558八份文档push两次预执行自动拒绝、旧问题待人类明确目的地/载荷回复；最后核远端697c4b0，未重新联网，新HEAD范围不自动受旧八文档审批覆盖。

## 最新有限模态来源与缓冲读取

[新静态合同](ORIGINAL_MODAL_SOURCE_BUFFERED_READ_CONTRACT.md)：1e551e4的29准确xref为28读/1store；4faca0分配1a0→6c4530→618670发布this，匹配派生root7c82fc+4=6c4930；16native前缀有条件self+fc零写，缺6c4956/6c4971。不是live绑定，owner34非零/guard相等仍未闭合。

447e00复用90+补48得138native正常局部图，四RET8；N>=A先fill后看remaining，包含buffered零请求，而raw零请求提前返0；fill后A0可返回累计短量。当前virtual60候选447d30仅8native、缺447d4c/447dfb，底层格式/OS/异常与存活Unknown。138/135/3总查询证据及元数据结果，继续官方exit0/688.393秒，源/18模块不变、临时空。原label错误/初次拒绝保留，附具体用户REA原文同命令标准复审一次获准；护栏/权限不改，无执行/转换/网络/上传/测试。新语义待独立复核，push待批不变。

## 最新fill/短读/错误传播与handler闭合

[新原静态合同](ORIGINAL_FILE_FILL_ERROR_PROPAGATION_CONTRACT.md)：fill447d30正常native75、handler6c4930正常native24闭合；首tag之外若干raw实际量及transform状态未检查，原样branch未缩available至payload短量。52Listing/10native的transform仅step1直接写outcount，失败不保证cap8d30成为有效量。raw22/mapper9完整native；bulk780533可返回短量，strict41fcb0为word2，经78074e不足走78090e/76501e请求RaiseException。异常模板e06d7363/flags1/3参数，实际catch/unwind/lifetime未证明。

handler零清selfFC、非零Translate/DispatchMessage，没有直接owner34非零store；实际WndProc/message/alias/root仍Unknown。四完整正常native图130与各Listing/锚点层级分开，codec398库存非传递闭包。138/138/0、exit0/775.37秒、源/18模块不变/tmp空/0活动会话，无目标或组件执行/转换/网络/上传/UI/设备。e95046/0、9aac44/0已二方，原失败保留、旧静态阻碍解除；本批新增待复核，仅本地，push待批不变。

## 最新选定codec原文件读取契约

[codec合同](ORIGINAL_SELECTED_CODEC_FILE_CONTRACT.md)：字面版本1.0.7、正window15、method8/头部mod31与Adler32确认zlib包装DEFLATE；既有uncompress/inflate函数名仍为语义对应，原符号/厂商实现未恢复。外层398/14、blocks1187/10+type4、codes656/10与fast306的正常Listing标签图闭合，非全native/传递证明。

最终flush117Listing/20native，两段copy均min(queued,avail_out)并更新total_out；在有效window/storage/DF0/nonalias/normalABI下受入口容量约束。codec可先写部分输出再失败，wrapper仅step1写外部count；fill忽略状态，实际短payload量及N上界也未核。有效输入规格要求完整framing/N、N<=名义36144、+15 zlib stream完成和成功actualcount；坏输入Native失败且零available是明确修复政策，不能冒称原行为。未知第三word、Huffman构造/distance/history及实际heap/文件仍Unknown。

101Evidence/0provider错误，80native地址仅79解码，保留排除not-boundary元数据并核准确guard。exit0/439.106秒、目标/18模块不变/tmp空/0活动会话，无游戏/组件/目标API/网络/上传。caller元数据遗漏既有447db4，不作穷尽。模态间接alias极限保留；新语义待二方、runtime1/候选DENIED不变，仅本地，push独立待批。
