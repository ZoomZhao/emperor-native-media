# owner+34 两次检查与选定派生读后方法

固定 English PE SHA `8a6d2df1…6753` / profile `e3402f2d…a7b2`。本批补字段写来源和一个派生方法的正常局部合同；没有实际owner/vptr/存档、游戏运行或可玩验证，运行绑定继续DENIED。

## 两次检查不能合并为同一个条件

4fb530以entry ECX保存EBP=owner，在4fb54f先调用4fd600。该helper的4fd613、4fd681两次写owner+34=0；最后一次之后只有owner+14/+38清零与pop/RET。在有效存储、正常callee保持ESI/EDI的ABI条件下，其正常尾的+34为0；不是实际运行观察。

随后500400/5003a0/413a40、4fb8c0及成功路径其它helper仍可改变状态。4fb8c0本地无+34 store；它的ALtrue路径要求4fb9ac检查时owner+8非空，之后5032c0/62e7f0等调用仍未闭合。4fafe0安装的4fb280/4fb2e0/4fb340/4fb3a0等，经源核实是allocation/constructor factory，不能根据查询标签当作选择事件的+34写入者。非零值的实际生产源仍Unknown；“header-validation”仅是取证标签，不能因此宣称该函数解析文件头。

4fb63d/640初次读取owner+34，非零选择534a30@4fb73a；零走另一42e680路径。稍后的4fb7d5/d8重新读取owner+34，零路径才在其它正常helper之后CALL4ac240@4fb862，非零跳过。reset的既有条件结果仍是phase/累计/子周期0、外层周期1。本批未证明两次值相等，也未证明所有载入均reset。

## 之间确切发生了哪些写入

| 来源 | 直接可见作用 | 剩余条件 |
|---|---|---|
| 4fbcc0 / 4452d0 | 选定记录写+3bf8 ID、+3bf4值；字符串状态+3af3和+3af4区，非空时先清128字节再调用限0x7f复制helper。 | 480740/481f10返回的记录、源字符串长度/存活、pool raw cursor、复制和传递alias未闭合。 |
| 4fc840 | receiver+263c=arg；该站receiver130f960。 | 不能据字段偏移不同就排除与实际owner存储别名。 |
| 408de0 | 875aa4=-1，receiver+1060=-1；caller receiver1e511b8。 | 408e00的RET由caller清4字节，408e10用RET4；固定写地址并非owner34稳定性的证明。 |
| 4fc450/460/4c0与4fbb50 | owner+8空返回-1/0/-1/0；非空tail读取linked+17c/+118/+2c4/+2ec。 | 4fc4d0另tail5031f0尚未展开；linked对象存活及当前owner+8仍未绑定。 |
| 4fae60 / 4fca10 | 返回owner+10 & 1。 | 不能把这个bit条件替换成任意非零字段。 |
| 4ff650 / 4ff590 | 两项谓词都非零才分配1ebc并构造4faa60；包装器读取owner+8，flag0调用58ee60登记。 | 无直接更换owner+8；构造/容器56ff00/OS同步、传递写集合与异常未闭合。 |
| 7a941c | 源名称为PostMessageA；共同join三站发送msg7/6/18。 | 没有调用OS API；实际投递、处理与时序未知，不补造同步重入或实际+34写入。 |

选定非零载入路径在两次guard之间的17个CALL逐站保留，包括当前virtual+10、完整loader、记录更新、PostMessageA与有条件新对象登记。局部未看到直接owner+34 store，**不能排除**这些callee的传递/别名或运行时作用。未盲目展开上一批的28个后续CALL，也未把这17站的调用数量当效果闭合。

4ff650失败边尤需保留：malloc正常返回空时4ff6a9使EAX0，紧接4ff6ab读取[EAX+10]。这是原源的失败边界，不是实测崩溃，不证明malloc实际为空或OS异常结果；不能写成分配总成功或凭局部无free推导保活。

## 派生+1c8确有额外虚调用

复用4c5dc0构造证据，固定root7afbd8的+1c8字指向4f0150。同批六根取字：三个413a00基类no-op、两个4f0150、一个414b50。只展开4c5dc0这一个类，不能将基类no-op或本方法推广到全部派生对象。

Ghidra尚未为4f0150定义function，assembly/callees报NoProcedure、containing未找到；原生Listing仍能取得完整15条正常局部指令。未创建分析注释、改原文件或放宽守卫。它以ECX=O读O+7无符号byte，以及O+a/O+c/O+14有符号word，向当前virtual+100传六参数，顺序为(signed O14,signed Oa,signed Oc,unsigned O7,0,0)。本地没有直接对象store，callee的对象/全局写入仍未知。

当前虚调用4f016b的ReferenceManager无目标，仍computed unresolved。若实际vptr保持选定固定root，其+100字是4c69a0；这是下一可取证静态候选，不是当前live binding。正常callee按与六次push一致的栈清理返回后，4f0171仅MOV AL1/RET；不能说完整EAX1，42da10也没有消费该返回。原异常/对象存活、raw cursor/容器重定位尚未证明。

同root首字为4c5d90，两条原生指令返回descriptor84ca08。选定20字节描述符显示name指针84ca40、size0x928（2344字节）、schema0、factory4c5d30、parent854438；未读名称或执行factory，不给它臆造类型名/实际对象归属。

## 修正上一批遗漏与验证范围

b2b6ab1独立复核38组通过、1项文档合同遗漏：534c7f在owner为空时直接跳到速度写入，跳过4fae50。已核保存原Listing和predicate叶，真实局部条件为**owner为空，或非空owner的predicate为零**。四文档本地修正提交9beed2216f084b64c9e81966224349a59e72b255；独立报告指出的三处及allowlist同一重复表述均修正。原38/1失败收据保留；修正和本批新增待独立复核，不将author检查计为独立通过。

本批140请求/137官方Evidence/3 NoProcedure，30个procedure地址、96原native锚点、60选定数据字节，exit0/680.46秒。源身份及18对官方模块不变、临时项目空。四个完整正常局部图共65节点：派生15、描述符getter2、两个factory各24；另一factory13节点在事先75新native锚点范围处停止、最后factory未展开，两者不算闭合。18个native锚点另与已定义函数Listing逐字匹配。CALL回边只是条件正常返回，未闭合callee、异常或实际执行。

## 下一步可继续与不可宣称

1. **owner34生产源**：优先4fb8c0成功分支的5032c0/62e7f0及前置对话/选择对象。通过标准是找到确切store、receiver/alias来源和每次guard之前的覆盖，正常/异常条件明确；不能从factory或字段名补造写入。
2. **选定派生+100**：沿静态候选4c69a0补六参数ABI、确切对象/全局写、异常与容器重定位依赖。通过标准为固定根及callsite来源、callee写集合/正常出口闭合；live vptr/存活仍需合格外部证据，不能用基类结果代替。
3. **两guard之间的高收益依赖**：先4faa60/58f450/56ff00及481f10/5031f0，按已见写集合补owner34 alias及raw cursor。通过标准为明确到字段/地址、传递边与失败返回；原28后hook及pretick四helper仍按优先级另补，不称首tick门控已稳定。

可继续固定原源纯静态取证、保存来源复核、局部合同与文档修正/选择性本地提交。不能执行原/衍生游戏、转换或SR/Unicorn/Wine/历史被拒探测，不能把GameData或旧Swift用作替代源；本批不执行组件测试或操作UI/设备，不以PCM解码/GPU读回当完整游戏验收。

当前仅本地提交。5587fb5八份文档push仍待用户明确目的地与载荷授权；没有本批push/网络/替代上传，当前新HEAD不自动在原请求范围。远端仅最后核实697c4b0，不冒充本时刻已重新查询。见[筛选证据](original-owner34-derived-postload-evidence.json)与[handoff](ORIGINAL_ROUTE_HANDOFF.md)。

## 后续复核与前置/派生+100补充

本提交快照后，9beed速度修正与82d9cf5新增已独立38通过/0失败，旧b2 38/1保留，失败已闭合。描述符进制明确为0x928=2344字节。新[前置与+100合同](ORIGINAL_PRESELECTION_DERIVED100_CONTRACT.md)核owner0/10及linked170/2c4/2e0写、模态current+4循环及所选4c69a0正常RET18；仍未找到owner34非零store。选定根25c为24字节步长表读取、1e8返回O+c8而写O128；postload arg5=0在正常ABI下跳过两个26c。当前虚目标/异常/alias/首次tick继续未闭合，新批待独立复核，发布仍待人类精确授权。
