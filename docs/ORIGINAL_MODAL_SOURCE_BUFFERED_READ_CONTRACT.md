# 模态全局来源与缓冲短读的原静态合同

固定原English x86源SHA `8a6d2df1…6753`、profile `e3402f2d…a7b2`。**已找到模态全局的直接构造写入，并补齐缓冲读取的正常局部图；owner34非零来源、当前虚目标与异常/运行事实仍未闭合。** 原“native指令”均指原x86 Ghidra Listing/静态指令核对，不是arm64或原游戏执行。证据见[筛选索引](original-modal-source-buffered-read-evidence.json)。

## 模态receiver来源与条件性的+4匹配

1e551e4的29个准确xref逐条静态核对：28读取，唯一直接store为618670中的**618856 MOV [1e551e4],ESI**。这仅覆盖返回的确切直接引用，不证明所有指针/别名写入均已覆盖。固定初值0不是当前值。

4faca0分配**0x1a0**，非空调用6c4530(0)，后者调用618670；该base constructor写root7c1a6c并发布this到global，随后6c4530写root7c82fc。固定base根+4word为61b500，派生根+4word为**6c4930**。global发布发生在+f8/+fc/+100清零及尾helper6192f0之前，不能证明观察时已经初始化完成。此链只建立构造匹配候选；后续helper、root覆盖、实际global/对象存活仍未知。616d90只读global并写child+7c，因此旧0xdc widget根7c8344不能当此模态receiver。

6c4930、61b500的procedure查询返回NoProcedure，但原静态指令可取得；不据元数据缺口说机器码不存在。6c4930的16节点前缀含外部间接CALL6c4941，返回EAX零分支到6c4966直接写**self+fc=0**；另有CALL6c4950，ReferenceManager给EXTERNAL:b0/9f静态引用。本批没有命名或执行这两个API，也没有证明其回调闭包。61b510原循环读取+f8/+fc，这个store是所选根下可影响退出比较的来源，但不证明实际走零分支或最终正常返回。

按声明reader cap，handler正常图缺6c4956/6c4971；不把16条前缀说成完整图，不扩大到无界地址扫描。没有找到owner34非零store，也未证明两guard值相等；外部作用/回调alias、当前root、异常与存活仍需相应合格证据。没有以邻近factory或静态槽字代替live绑定。

## 447e00的正常局部图与短读条件

在既有F构造根7ac7d4+34=447e00候选下，本批复用旧90个原native节点，沿四个准确缺失后继补48节点，得到**138节点完整正常局部图**；四个出口447e5c/447f15/447f5c/447f75均RET8。三个CALL为77c2c1和两处当前virtual+60。这里CALL后继仅在callee正常返回、原ABI保留成立时有效；不是跨callee/异常或实际执行闭合。

令N为请求字节、A为F+11a88可用量。以下局部合同还要求receiver、缓冲/目标存储有效，DF0、计数非负且有界、指针/算术不越界、无重入/并发alias破坏；本批不提供这些运行事实。

| 原源分支 | 正常局部行为 |
|---|---|
| Fbyte+1e=0 | 调77c2c1。该raw helper在请求0时先返回0、不调用ReadFile；OS成功路径返回out-count，不能保证等于请求。 |
| 缓冲N<A | 复制N，前移F+20，减少A，返回N。 |
| 缓冲N>=A | A有符号正值时先复制A并累加；清A，**先调当前virtual+60，再检查remaining**。因此N==A以及N==A==0都可能触发fill，不能用raw零请求规则替代。 |
| remaining>0且新A>0 | 分批复制/前移/累加，耗尽时再fill。source循环存在，终止及量值/存储不变量未获运行证明。 |
| remaining>0但fill后A=0 | 正常路径在447f15返回已累加量，允许小于请求；不是强制完整读。最后remaining<=A分支只消费剩余量。 |

固定根+60word确认**447d30**，procedure元数据缺失；只取8条原native前缀，447d4c/447dfb仍未覆盖。故本批补齐的是447e00正常局部图，fill实际生产量/底层格式、当前root、OS短读、异常和全文件来源均未闭合。

既有archive fill78074e读取underlying current+34，可能在返回正量且仍需字节时循环；最终available<need会CALL78090e(3,0)。不能因为底层允许短返回，就断言archive消费者也静默接受，亦不能据该call猜完整异常交付。没有消费GameData或分析城市逻辑。

## 错误路径与静态工具结果

raw helper失败路径调用既有7a9280导入后进77caa3；其error arg为0时RET8，非零时调用77cbcf再77cb64。77cbcf捕获的Listing是错误码映射，非分配器；查询标签allocation不是函数语义。77cb64有0x14分配/record构造及CALL76501e，最终异常交付未闭合。不把ReadFile失败路径全部说成正常返回或全部保证throw，也未执行任何OS API。

首轮完成6项后，自写请求label的大写Listing违反原有小写校验，退出1并清理；操作名本来合法。初次继续命令被自动审批因授权上下文不足拒绝。补充用户具体REA请求原文后，**同一`python3 run_session.py`命令标准复审一次获准**；原拒绝和失败资料保留，官方检查/权限设置未改，无换路绕过。

继续会话132请求/129Evidence/3NoProcedure，exit0/688.393秒；合计138请求/135Evidence/3元数据错误，11个procedure参数地址/107native锚点/16选定data字节，在原140/30/128与1200秒总截止内。两实际会话目标身份、选定18个官方模块指纹不变，临时空、0活动会话。138正常图、16handler前缀、8fill前缀分开计，不把全图覆盖混到partial。

本批仅作者来源/文档一致性检查通过；新增语义**待独立复核**，原审阅和失败/修正记录保留。没有游戏/派生/arm64执行、转换、组件测试、安装、网络、上传或UI/设备操作；runtime schema1与所有DENIED候选不变。远端push审批仍待用户回复，不重试；旧5587fb5八文档授权不能扩成当前HEAD。

下一有限来源任务是分别补handler两个后继与fill两个后继，在新声明预算中闭合正常/异常边；仍需当前对象/root、alias与实际返回的合格来源，才能提升运行结论。静态片段、PCM/GPU或自造模型均不能填补这些输入。
