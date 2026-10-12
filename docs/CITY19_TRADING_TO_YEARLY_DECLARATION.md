# Trading固定字段到3554；继续到YearlyProduction声明3581

新支持仅由enable_trading=true显式启用，默认false。此前Housing-only和Homage CLI不改，重新以新header构建后，对真实样本的完整默认3472与Homage3542统计分别逐项相同。新增Trading Release、UBSan各30/0；没有重跑旧40/29整套验收。

官方有限静态观察将静态名字8596b0关联descriptor859168：schema0、size字段12、factory5596d0、base8590d8。与实际schema0声明对应，所选正常base链可满足expected cGoalBase，但未证明原进程当前registry选择。factory正常路径请求12字节，调用559730；构造写vptr7b82b8，reset560b00只写self+8/self+4。前三槽是559790、5592d0、560b90。getter的MOV EAX,859168/RET是两个有限native观察，原对象当前vptr与live cache仍未证明。

560b90有完整procedure来源：先调用完整基类55feb0，随后读取独立版本记录12a5a10，再以4字节请求读取self+8，正常尾部设记录1并RET4。55a1c0初始化记录1。**所选reader内没有计数或嵌入容器。** 名字中的Partners不是变长列表的格式证据，size字段也不决定wire长度。source明确请求2+4+2+4=12；外层560040 signed计数与十个成员循环继续单独维护。

| 实际范围 | 字节 | 本轮核验 |
|---|---:|---|
| `[3542,3544)` | 2 | base节版本1 |
| `[3544,3548)` | 4 | 55fef6，self+4，仅摘要 |
| `[3548,3550)` | 2 | Trading节版本1 |
| `[3550,3554)` | 4 | 560bd6，self+8，仅摘要 |

该payload SHA256为 `63362e01dc37cca37000c405c301d2052767603832833d48270777e6fbf924b2`。class schema0与两个节版本1分开；原reader不按取回节版本选择字段，现代1限制为限定域。source-matched registry/type、allocator正常非空、cache未预seed/不被同流别名改变、有效存活对象及回调/异常正常等前提保留，没有调用原factory/serialize方法或执行城市规则。

该项属于成员3的ordinal1，外层count3，所以3554不是新成员头，而是同成员ordinal2的对象tag。实际word0xffff，再读schema0/nameLen21和名字字节，到3581停下。名字摘要 `0c4bef2dd8625b2a7628f4112038e879b99b06ec744c5025e70409e300e1c0ef` 匹配静态859678的cYearlyProductionGoal；其descriptor/reader尚未绑定，payload未解释，不猜长度。

已核六个已匹配项，完成前三成员，active条件cache为class1Housing/object2/3/4/7，class5Homage/object6，class8Trading/object9，next10。Yearly class10/object11只是之后lookup/schema/factory成立后的安排，未登记为活跃metadata。class引用会创建新instance并消费已匹配payload；object引用需合格存活且expected-compatible元信息，不调用本项payload reader，也不新增id。现代kind guard不等于source原指针数组有kind标签，主池索引不解释。

候选完整输入/解码SHA不变，仍136块/4430995解码字节。解析终点3581，余4427414字节未解释；16384为私有保留预算，不是实测终点或原格式上限。原始/解码字节、GUID、字段值、编译产物、凭据和用户草稿不上库；全程现代自写OwnedFramedReader+系统zlib与primitive字段/meta解析，无原或衍生game执行。

新增30项每模式覆盖Trading12独立字段、节版本/不同schema、截断、重复声明、混合Housing/Trading cache、compact/extended class/object引用、错误kind、未匹配新类停止、块边界、巨大计数预算、SHA/Adler和优化启动拒绝。旧模式只做了两次有针对性的真实完整统计回归。来源会话正常exit0、身份和18份官方模块摘要不变、临时根为空，没有NoProcedure错误或连接/审批等待。本新增仍待独审，证据见 [清单](city19-trading-walk-evidence.json)。下一缺口为3581的YearlyProduction类型/serialize来源，现有证据不能解释它的payload。
