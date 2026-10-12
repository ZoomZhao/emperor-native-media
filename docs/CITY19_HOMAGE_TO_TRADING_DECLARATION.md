# Homage版本3的条件字段闭包：3488；继续到TradingPartners声明3542

本轮在a7f333a的有界Goal组件中显式启用source-mapped cHomageGoal支持。默认flag为false，原Housing-only CLI源码不改；新CLI只在自身设置enable_homage=true。真实默认模式的完整统计与已发布3472结果相同，默认Release/UBSan各40/0，新增扩展各29/0。原字段语义、源前提及runtime未证标记保留。

有限官方来源观察：名字859658的xref为descriptor8591e0，24字节数据给出schema0、instance-size16、factory559b80、base8590d8。其schema与流声明0一致，base指针满足expected cGoalBase的所选正常链。factory559b80的正常路径请求16字节并调用559be0；构造写vptr7b8434，随后560980只写+4/+8/+c且不写偏移0。前三槽为559c30、5592d0、560a60。getter在确切入口的MOV EAX,8591e0/RET两条native观察，以及注册调用40aa80关系保留；不证明实际模块registry选择或原实例当前vptr。

560a60存在NoProcedure元数据缺口，错误收据保留。所选官方native观察从真实槽入口开始，核对prologue→基类55feb0→独立版本记录12a5a18→mode1分支→signed版本减2/减1→版本3完整字段读序→版本设3→正常RET4。恰好48个所选native地址以内闭合，未改预算、强制创建procedure或执行原程序。初始化55a170为该记录设置3。write路径和版本2专属入口560ad7尚非完整来源，本扩展不支持它们；原reader对其他值的处理不能写成现代范围拒绝。

关键差异：class schema为0，base节现代范围为1，Homage自身节为3。版本3先读self+c，再读self+8，不能按内存字段顺序或sizeof推断。

| 实际范围 | 字节 | 来源 |
|---|---:|---|
| `[3472,3474)` | 2 | 55feb0，base节版本1 |
| `[3474,3478)` | 4 | 55fef6，self+4 |
| `[3478,3480)` | 2 | Homage独立记录12a5a18，实测3 |
| `[3480,3484)` | 4 | 560aca，self+c |
| `[3484,3488)` | 4 | 560ae0，self+8 |

16字节payload SHA256为 `a84d5370fe010602ccb2b336d646c7bfc129b4ffccd1040e2ea421a717f5b2ac`，source要求2+4+2+4+4而非sizeof推断。该项使成员2的count2正常完成，条件模型登记class5Homage/object6后next7。原factory/schema callback没有执行，字段只作宽度/顺序/摘要核验，没有城市规则或目标含义解释。

随后同流成员3在3488读版本1/count3；第一项class引用1再次读Housing20字节，止于3516。该成员ordinal1出现新class声明，schema0/nameLen20，名字摘要 `f652ebbbc12f4211e2bd57346b7ef96b4a6c99913438d6f3cc2c50043698332f` 匹配静态8596b0的cTradingPartnersGoal。声明结束3542，payload未读。已核五项、完成前三成员，active条件cache为class1Housing与object2/3/4/7，class5Homage与object6，next8。Trading class8/object9只为后续lookup/factory成立后的安排，不登记未完成类或跳过未知长度。

完整候选仍为136块/4430995解码字节，输入/解码SHA不变，余4427453字节未解释。16384为私有保留现代预算，不是解析终点或原格式上限。新增类支持需明确启用；known-class tuple的schema0检查、两个节版本现代范围、kind/非空/预算安全限制保持来源分类。原registry当前链、vptr/cache/对象存活、allocator/clear callback/别名及异常仍未证明。

新增合成覆盖Homage16字段与反向offset读序、Own版本3/legacy1/2/future4拒绝、基类版本、不同schema、每个截断、class/object及extended引用、混合Housing/Homage缓存、重复声明、unknown类停止、块边界、SHA/Adler。最初三案例期望沿用Housing20而多算4字节，共6失败；原始失败已保存，按独立源宽度修正期望，解析器未改，最终29/0各模式。默认40/0回归也通过，优化启动前拒绝。

新扩展仍待独审。没有恢复原执行/转换/探测路线，没有原字节、字段值、编译二进制或凭据上传。下一明确缺口为3542的TradingPartners lookup/schema/serialize来源；当前证据不能解析它的payload，先有限类型来源再扩字段。来源及执行回执见 [证据](city19-homage-walk-evidence.json)。
