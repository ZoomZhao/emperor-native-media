# 原文件读取：选定codec、长度与错误契约

**当前payload读取链可识别为zlib包装的DEFLATE；原二进制版本字面量为`1.0.7`。** 已补外层/blocks/codes状态表，核对最终输出flush的容量裁剪，以及错误状态被fill忽略的链。固定English x86 SHA `8a6d2df1…6753` / profile `e3402f2d…a7b2`；全部为原静态来源。本批未读实际文件或GameData，未运行解码器、游戏、衍生库或组件测试。[来源与契约元数据](original-selected-codec-file-evidence.json)。

## 格式与既有入口

876600的NUL结束字符串是`1.0.7`。5f89e0经5f88d0以正windowBits=15初始化；该调用的nowrap为0。外层5f8a00检查首字节低4位方法8、CINFO+8<=15、两个头字节组成的值模31为0；FDICT时读四字节字典ID并返回2。尾部按大端读四字节，与块层计算值比较。5f9ef0的两项16位和、模65521、5552字节分段与Adler-32公式吻合，错误文本含header/data check、window、dictionary和compression method。

这些来源确认当前**zlib包装DEFLATE**路径。5f8070、5f89e0/5f88d0、5f8a00、5f8880分别可语义对应uncompress、inflateInit_/inflateInit2_、inflate、inflateEnd，5f9ef0对应Adler-32；函数名和“内嵌zlib1.0.7实现”身份仍是有来源的推断，未恢复原符号或证明该厂商源码未经修改。当前caller未启用gzip自动识别或raw-DEFLATE模式；不能推广成所有原文件均使用此格式。

本机SDK的zlib.h仅作标准接口/状态命名参考。原x86 stream为0x38字节；host调用必须用host自身z_stream及初始化宏，不能把旧对象布局或0x38强塞给64位ABI。标准API是可实现候选，本批没有链接或运行验证。

## 输入、输出与写边界

fill447d30的压缩分支读取LE32声明长度N至F+11a8c，再消耗另一个四字节word，随后读N字节至F+8d58。第二word在本fill未作为产量使用，其含义保持Unknown。三次后续Read返回长度均未核对；N也未在这里限定。

输出名义区间`[F+28,F+8d58)`与输入`[F+8d58,F+11a88)`均为0x8d30=36144字节。fill把输出长度字段预置36144，再调用5f8070(destination,&length,source,N)。这些是字段布局和调用参数，实际对象分配容量、当前root及alias尚未绑定。N超名义区间存在覆盖相邻字段的条件风险；payload短读则仍把声明N当完整输入提供给codec，不是已经观测到的真实坏文件。

正常init为内部window请求`1<<15=32768`字节并设置window/end，reset令读写指针从window开始。5fb030输出flush有117条whole Listing、20个native锚点；两段拷贝各取`min(待写连续段,当前avail_out)`，减avail_out、增total_out并前移next_out。环绕第二段使用第一段剩余容量。在有效ring/存储、DF=0、无alias/重入及正常ABI等前提下，合计拷贝不超过入口容量；checksum回调来自选定5f9ef0的只读累加路径。

stored-block拷贝还取stored剩余量、avail_in、ringfree的最小值；慢code写字节前检查窗口余量，fast入口及循环要求至少258窗口余量和10输入字节。动态Huffman表构造、损坏distance/有效历史域及CRT分配/释放传递效果仍未闭合。上述输出flush边界**不等于整个旧codec对任意损坏输入都内存安全**。

## 状态为何没有变成有效长度

| 来源层 | 正常与错误传播 |
|---|---|
| init | 版本首字符/0x38尺寸、window、pointer和分配检查存在-6/-2/-4正常错误出口；字面串不等于完整版本比较。 |
| outer step | 1为terminal；2为需要字典；0/-5可非terminal或容量/输入不足；坏头/校验等进入-3，坏stream为-2。 |
| 5f8070 wrapper | init非零直接返回。step非1先cleanup，0转为-5，其余状态原样返回；**只有step=1直接写外部length=total_out**，再返回cleanup结果。 |
| 5fb030 flush | 拷贝正量可把传入-5改为0；-3等其它状态仍保持，可先输出部分内容再返回错误。 |
| fill→447e00 | fill忽略wrapper EAX，重置cursor/mode；buffered read按F+11a88 advertised available继续。失败时不能保证预置36144是有效产量。 |

checksum在block输出之后才最终比较，因此来源允许“已输出前缀但terminal校验失败”；不把内部total_out或destination部分写入误作外部有效length更新。也不借本机较新zlib的错误约定替换原wrapper的`2`、`0→-5`或length写回条件。

原wrapper未要求avail_in归零，不能凭标准格式擅加“禁止payload内尾随字节”规则；terminal零产量也会写回0，随后buffered A=0可能短返回累计量，不能自动命名为真实EOF。

## 可实现契约与损坏输入政策

这是**实现前规格，尚未落代码或运行验收**。支持域限当前压缩marker路径、有资格的F/root/存储和正常ABI。Native读取器先完整读取长度word、未知word与N个payload字节，使用检查过的长度/算术，固定buffer模式下在读取前要求N<=36144，准备独立可写36144目标。以host标准zlib的+15模式识别zlib包装，要求完整stream结束、有效Adler、无需外部字典及产量<=36144；成功后仅将actual produced发布为available。零产量和payload尾随部分按上文原契约处理，不用第三word猜产量。

短header/短payload、超长N、需要字典、codec错误等应返回明确失败，丢弃部分输出并发布零available。这是建议的Native边界政策，**明确区别于原实现对损坏输入的未检查传播**；不能宣称两者在坏文件上完全等价，也不能为“忠实”复制越界读写或假造有效产量。

最有用的三个工程验收任务（本批不执行）：

1. **有界块读取器**：自造framing覆盖每个字段及payload的任意截断、N=0/36144/36145和算术越界。通过标准：完整支持输入位置与N精确一致；任何短读/越界在codec前失败，未初始化字节不进入输入，未知word不被当产量。
2. **host codec适配器**：自造合法stored/fixed/dynamic DEFLATE的zlib流、空输出、恰36144与超容量输出、有效流后尾随字节。通过标准：支持域bytes与actual count一致；超容量明确失败，目标guard不变；用host ABI，记录stream-end和实际consumed/produced，不依赖原游戏或其衍生库。
3. **错误传播与部分输出隔离**：自造坏CMF/FCHECK/Adler、FDICT、截断流及可先产出前缀后报错的输入。通过标准：错误到调用者可见，consumer available为0、部分输出不被消费；成功才发布actual count，并明确记录与原wrapper状态/length规则的差异。

## 来源层级、审核与限制

外层398 Listing/14 cases、blocks1187/10 modes+4 types、codes656/10 modes及fast306均完成所选whole Listing正常标签图；callee正常返回为前提，不称全部native或传递闭包。101查询/101Evidence/0 provider错误，80个native查询地址中79个是解码指令；误选5fa8d9返回not-instruction-boundary元数据，保留并排除出语义锚点，随后准确核5fa8d3。官方exit0/439.106秒，固定目标/18组模块不变，tmp空，0活动分析会话。

FunctionManager的5f8070 caller列表只列583ca0，遗漏已有native447db4；447d30没有完整function metadata，因此该列表不能用作调用者穷尽或否定证据。没有扩查583ca0、所有CRT或Windows回调。模态间接alias继续作为静态极限。

f9d56f8已提交独立审核，本批尚未消费其完成报告；新增codec语义仅作者来源及文档一致性检查通过，待独立复核。runtime schema1、全部候选对象及执行DENIED保持，研究schema24→25。仅本地增量；push独立待用户，旧发布审批不覆盖新HEAD，无重试或替代上传。
