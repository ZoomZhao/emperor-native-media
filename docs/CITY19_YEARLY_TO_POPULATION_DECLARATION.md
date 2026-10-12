# Yearly独立source映射与20字节闭包3601；批量继续到Population声明3678

本轮显式enable_yearly=true，默认false。共同基类55feb0的version16/field32读序抽成受限helper，Housing与Yearly的own version16/threeDWORD模式也共享byte helper；两者的descriptor、own record与source调用独立映射，未由名字或sizeof推断。旧CLI源码不改，真实3472/3542/3581的完整旧模式统计在最终header下逐项相同，旧整套验收未重跑。新增Release/UBSan各33/0。

官方有限来源：静态859678名字xref为descriptor8591b0，schema0、size字段20、factory5599a0、base8590d8。factory请求20并调用559a00；ctor先调用已核base559120，写vptr7b839c，再经561080只写+4/+8/+c/+10，不改偏移0。前三槽为559a50、5592d0、561160。getter在确切entry观察MOV EAX,8591b0/RET；当前原进程对象、registry选择与vptr未由此证明。

561160有完整procedure来源，先调用55feb0，再用独立record12a59e8读取节版本，mode1分别以4字节读取self+8/+c/+10；55a300将该record初始化1。source此reader没有内部count/container，节版本取回值不选择字段分支。class schema0、base节1、Yearly节1分开记录，现代版本1门限是限定域。sizeof20不作wire长度证明。

| 实际范围 | 字节 | source |
|---|---:|---|
| `[3581,3583)` | 2 | base55feb0节版本1 |
| `[3583,3587)` | 4 | 55fef6，self+4 |
| `[3587,3589)` | 2 | Yearly record12a59e8，实测1 |
| `[3589,3593)` | 4 | 5611c0，self+8 |
| `[3593,3597)` | 4 | 5611cd，self+c |
| `[3597,3601)` | 4 | 5611da，self+10 |

payload SHA256 `fe1fceba52ae25d799e8649b6bf3e004cf54dc1728c3ffba5dd04a247736bb13`，20来自2+4+2+4+4+4请求。字段仅宽度/顺序/摘要，无资源ID、目标意义或城市规则解释。该项完成成员3的count3，条件cache登记class10Yearly/object11，next12。

walker随后按count继续已匹配类，成员4从3601读v1/count2，其两项均完成到3651；成员5头v1/count2到3657，第一项newclass声明schema0/nameLen15，名字摘要 `cd660ae49098a2502daa548c3d13c4ab848a48f2cc87636edbd71a6e960e4ffc` 匹配静态85970c的cPopulationGoal。声明终点3678，payload未读，descriptor/Serialize尚未绑定。已核9项、完成前5个成员（0..4）；当前member5/ordinal0。next cache14，pending Population class14/object15只是假设未来lookup/factory正常后的安排，未激活新类metadata或跳未知长度。

完整候选仍136块/4430995解码字节，输入/解码SHA一致，余4427317字节未解释。16384为现代私有保留预算，不是原容量或本次解析终点。source模型cache0null、class先登记/instance先于payload登记、object ref复用合格alive/expected-compatible元数据且不读payload、class ref创建新instance并消费匹配payload。source原untyped cache没有modern kind标签；kind/非空/negativecount/版本/名字/预算限制均继续标为现代安全范围。

新增33项每模式验证Yearly/共享pattern独立field摘要、不同class cache、重复/compact/extended引用、错误版本/schema、field各截断、unknown声明停止、SHA/Adler与item/cache预算拒绝、优化启动门。旧真实三模式回归采用完整统计，不以相同终点替代摘要/身份检查，也没有重复40/29/30旧套件。源会话正常exit0、输入身份与18官方模块摘要不变、临时为空；无NoProcedure错误或新的审批阻碍。

原runtime registry/vptr/cache/实例、allocator/lifetime/clear callback/别名/异常完整效果仍未证明；原factory/Serialize/game/衍生库未执行，原字节、decoded值、GUID、字段值、编译产物与凭据不上库。新支持仍待独审。下一只查Population实际类型与Serialize来源，再扩3678后的payload。证据见 [清单](city19-yearly-walk-evidence.json)。
