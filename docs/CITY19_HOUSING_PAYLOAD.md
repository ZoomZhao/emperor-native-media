# 首个cHousingGoal条件payload核验：3379到3399

同一完整136块、4430995解码字节的v19候选，输入/解码SHA与f14193d保持一致。首类声明schema0、名字摘要匹配静态cHousingGoal，并与859198 descriptor、base8590d8、factory5598a0、constructor559900、vptr7b8350及序列化槽560ef0建立条件关联。本轮复用已保存的完整55feb0→560ef0与有限版本初始化来源，不启动新的原始静态分析器，不执行原游戏、factory或schema callback。

已实际核验首对象20字节：base节版本1、四字节字段，再housing节版本1和三个四字节字段。payload SHA256为 `653bb4fe9d14b084bc95bde7b3f2e40e5adb9642c243f35956584d67877896ff`，解析止于3399，余4427596字节未解释。

| 流范围 | 字节 | 对应来源 |
|---|---:|---|
| `[3379,3381)` | 2 | 55feb0读12a5a48版本 |
| `[3381,3385)` | 4 | 55fef6，self+4 |
| `[3385,3387)` | 2 | 560ef0读12a59f0版本 |
| `[3387,3391)` | 4 | 560f50，self+8 |
| `[3391,3395)` | 4 | 560f5d，self+c |
| `[3395,3399)` | 4 | 560f6a，self+10 |

class schema0与两个独立节版本1分开记录。两个来源方法按mode选读写，没有按取回节版本选择字段分支；现代限定版本1是工具范围，不能写成原程序拒绝其他节版本。字段没有赋予城市规则、目标含义、资源ID或主池槽语义；只核验宽度、顺序和摘要。内存sizeof20不构成wire长度证明，本轮20来自上述2+4+2+4+4+4请求。

实际首成员计数分类为恰好1：解析器按已读四字节有符号计数取得这一结构关系，未从先前count摘要反推。因而第一个对象的字段结束后，560040正常循环结束并返回55a7a0；原数组循环进入下一stride0x18成员。**下一实际消费者位于3399，是成员1的两字节版本和四字节计数，不能把这里当首成员的第二对象tag。** 大于1的合成案例则保持同成员，下一消费者才是对象tag。

所选正常源顺序是新类读取/lookup/schema检查→登记class→expected base检查→factory→登记object→virtual+8(S)→正常恢复S+c。初始cache0=null且next1、无预seed和先前同流对象、registry/type匹配、factory正常非空、有效资源且无同流别名副作用等前提成立时，可建立条件metadata模型：class1为859198/schema0，object2引用该class，next index3。现代工具仅输出该条件模型与字段资格结果，没有构造原对象、调用原reader或证明实际原进程缓存、registry、receiver/vptr。未把archive缓存编号映射为主池槽位。

本轮限定新工具最多保留3399字节于私有内存，完整输入/解码身份与所有块成功后才输出统计；只解释首对象四个固定字段。没有原始/解码字节、GUID或对象字段值落盘，首成员只输出恰好1/大于1的结构分类。生产reader和三个此前工具保持原样。

Release、UBSan各29/0自造验收，覆盖两个独立节版本、每个字段截断、跨块未对齐布局、标准zlib、schema/class匹配拒绝、负/零计数限定域、巨大正计数不分配对象、计数1/2对应不同消费者、未知尾部、预算/摘要/Adler拒绝。优化Python在建目录前拒绝。来源关系仍有runtime registry、callback/allocator/异常等前提；这些验收不证明原游戏加载或完整archive兼容。

接下来先读下一成员头，根据其signed count决定是否继续下一成员或解析其第一个对象tag。沿用同一archive的条件class/object缓存模型；class-reference会创建新instance并读payload，object-reference复用已有object且不调用payload reader，必须先核对索引与expected基类。没有合格类型或未完成payload时停止，不跳过未知长度。证据见 [清单](city19-housing-payload-evidence.json)。
