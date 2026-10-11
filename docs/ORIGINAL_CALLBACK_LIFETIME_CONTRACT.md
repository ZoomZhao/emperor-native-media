# 原回调作用、条件生命周期及异常退出

固定 English PE SHA `8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753`，REA profile `e3402f2d7c487875edf3c6215fa95d67ee1fa133ba68c2390a9338b38656a7b2`。证据来自官方 REA/Ghidra 静态 Listing、选定表项与既有 Evidence；没有运行原或派生游戏。图中 CALL 只作为边，没有执行寄存器、内存或调用。

## +130 与前序参数消费

候选根7b65e4的+130为42a980；87条正常可达指令、9个CALL和RET16已闭合。实际调用对象、当前根和存活性仍未绑定。方法保存外层O于EBX，首先调用其当前根+c8(-3)，然后4ea050转发到4e1420；这里使用10315d8/10315dc的另一记录池，不能与主池8c7630合并。

若返回索引非零，取得的raw记录R跨后续lookup及4e98a0调用后被写入：R61=1、R13=低字节O14、R16c为所见IDIV5余数、R68=低16位Ob4。索引零跳过这些写入，但仍执行5c4500(7)。本地Listing未见直接写O根/主池slot或保活；4e1420的多分支factory/旧记录virtual+4、4e98a0的部分子调用、5c4500/IAT副作用仍未全闭合，不能推出跨调用R安全。42bbd0不消费+130的EAX，不应定义为成功布尔值。

42bbd0前序408170只RET4，另外两个已压入word仍留给下一次调用。4b72b0、5ad440、5ad940在这里分别有8/5/4个word，随后ADD ESP0x44统一清理68字节；5ad440的第五参数确为1。不能把每个方法后的邻近栈调整误当独立ABI。

## 5ad440 与 d2 回调的所见作用

5ad440先有对主池对象的virtual+cc，再在第五参数非零时从413b40取得raw slot EDI，遍历主池。筛选为426d10精确byte==1或3，再由4bdb60判selector==d2；析构flags的bit0合同属于另一处。满足筛选时，P保存在ESI中跨virtual+264和+270，EDI每轮加4，只重读长度，没有重新取得begin或pin。外层42bbd0返回后直接继续读保存的O字段及根；恢复ESI只能证明位型恢复。

这形成明确的**存活证明缺口**，没有证明callback实际释放对象、移动池或发生UAF。其它对象callback与别名关系仍未知；不能据源代码形状补运行时结论。

选定d2构造4c5dc0安装根7afbd8、置P150=0，再调用4c6c50。后者在DF清零且内存有效时写500个dword P154..P923为-1、P924=0，并不写P150。根的+264→4c7170只有两条指令，返回完整P150，RET0；不是bool，也不保活。5ad440只在其完整EAX非零时进入+270；若构造零值未改，便跳过+270。+26c表项4c6c70目前仅有SUB ESP0x10入口证据，尚不能称为setter或推断作用。

+270→4c6d30的102条正常可达指令已包含switch闭合：先排除signed ECX<0，unsigned>6跳过switch，其余0..6使用固定4c6e64的七个word；表内容和地址有效是前提。方法通过本对象+278、47f1b0主池lookup和另一个对象Q的+274进行调用，并有indexed global13789c0的word/byte写入。本地未见直接root/主slot/free操作，但Q+274实际类及作用未闭合，P本身跨这些调用。

+278→4c6e80的21条指令均RET8。4ba9e0及其4b8f70、4bc140正常直接helper体已核对：写global101c560、八个dword101c5b8..101c5d4以及10c72ac/10c72a8。失败返回EAX0；成功经两个输出地址复制全局值，最终AL=1，**完整EAX保留第二个输出地址的高位，并非EAX=1**。+270只测试AL。所选直接helper没有heap/main-slot操作，仍要求有效输入、内存、不重入全局及正常调用；不能称无副作用回调。

## 备用块 handler 与异常恢复

780ad9的35条正常Listing已闭合。781ab3/7818f8可取得缓存或通过7814f2创建状态T；实际缓存、manager、别名和当前值未知。7814f2请求0x118即280字节；78160f向IAT7a9324传flags0x40及大小，该导入符号已核定为LocalAlloc，空值会调用77aeba。781a0d仅写根7cd308、Td4/T104=-1，没有直接写T+c；任何分配零初始化推论还依赖真实API及内存合同。

T及T+c非零时，用76b61c取capacity：unsigned capacity<=requested+4会调用7656df释放备用块并置T+c=0；否则调用76b70a(pointer,capacity-requested-4)，这些加减按32位回绕。后者116条正常局部指令已闭合；模式2/3内部helper、锁和堆语义未全闭合，fallback规格化size后以flag0x10调用IAT7a9208。780ad9忽略resize返回值且不更新T+c，这一路正常返回EAX1，不能据此保证resize或下一次分配成功、保证retry终止。缺状态/备用块时会调用抛出helper；EAX0只属于它返回的条件路径。

77d715在77d739装入780ad9，随后分配0x48即**72字节**，直到分配返回后的77d74d才将EH state置0，然后在77d75e调用77f80c。正常路径置state=-1并在77d76f恢复EBX保存的旧handler。

选定静态EH元数据810220/810240将state0指向7a8650；7a865a后的实际指令是JMP764cc1，不能据provider CALL_TERMINATOR注释多造一次CALL。清理action四条指令只把[EBP8]交77bb31释放、POP/RET，未调用handler setter。因而这个action不能证明异常时恢复旧handler；分配自身失败还发生在state=-1，不能借state0清理补证明。OS实际注册帧、状态、异常、外层处理/恢复及当前global88a3d4均未知。

固定88aa00初始word为769fbe；7689c2使用当前88aa00(status255)。若它仍等于初值，就进入quick-exit、跳过此前记录的退出callback数组；这不是当前值或实际退出发生的证明。若失败handler返回，initializer仍有begin解引用，不能补成安全null返回。

## 证据质量与下一步

三批分别为100查询/99 Evidence/1 NoProcedure、139/137/2、124/123/1，均exit0，输入身份和18对选定官方模块SHA不变，临时项目为空。部分procedure未登记，已用原生Listing补闭合；没有强建函数。前批自写reader因70/18新锚点上限停下的partial结果仍保留；后批读取明确缺失指令并闭合，不修改官方防护。NoProcedure及reader上限不属于安全审批拒绝，查询成功也不是游戏测试通过。

3a15dc7的独立复核为44组通过、0项新增语义失败，已关闭该提交的flag误述；c0的34通过/1失败及c82新增29通过/继承失败历史保留。本合同新增语义仍待独立复核。详见[证据索引](original-callback-lifetime-evidence.json)、[既有生命周期与退出合同](ORIGINAL_LIFETIME_AND_EXIT_CONTRACT.md)和[后续入口](ORIGINAL_ROUTE_HANDOFF.md)。

继续静态工作有三个具体入口：4c6c70及P150的赋值→存储→读取；Q+274/先前+cc的选定候选根与所有权作用；76b70a的模式2/3及退出数组重登记/重定位。每项通过标准都是选定正常/异常边、ABI、写集合及来源具固定SHA/Evidence，并明确仍依赖的当前对象/指针/OS前提。没有证据不生成runtime绑定，不用自写接口通过替代原源证明。
