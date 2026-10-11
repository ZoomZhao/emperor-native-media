# 原版静态路线 handoff

本批在本地5587fb54d5f0be4e68c03620c9261e04338bd312基础上补[读后与调度条件](ORIGINAL_POSTLOAD_SCHEDULER_ENTRY_CONTRACT.md)。5587fb5的八份静态文档push两次在执行前被自动审批拒绝，已向用户请求精确origin/main与载荷授权；本批不再次push或换渠道。最后获准只读远端核实为697c4b0，先前366/697成功收据保留。新本地增量未自动包含在5587fb5的审批请求内。

版本19流读入的phase/计数来源合同仍在[版本载入](ORIGINAL_VERSIONED_CITY_LOAD_CONTRACT.md)。534bf0读后直接CALL4aeeb0并内调42da10，534a30外层再调42da10；+1c8筛选为flag精确1/3/6，选定稳定基类callee仅AL0/RET。派生callee、raw cursor/对象存活、Ob4与slot关系仍未闭合。

新原源显示速度/配额byte/gate/mode明确写站点，4ac240正常reset为phase/累计/子周期0而外层周期1；但4fb530两次owner+34 guard间存在callee，不可假定稳定或所有读入都reset。所选外层loaderEAX被忽略、当次tick在载入之前；首次新状态分派须下一次quota>0和当前gate0，原hold guard仍可阻止phase消费。

七份正常局部图95节点（42da10=36,5371a0=17,4ac240=24,4f8d50=2,413a00=2,4afd40=12,436560=2）逐条匹配官方native Listing，CALL只视正常返回、5371dc真实tail JMP不当CALL。140/140/0查询/Evidence/错误，exit0/源与18对模块不变/临时空。当前新增未独立复核，5587fb5已独立37/0、两处发布状态提醒已核实并在本地增量修正；697c4b0独立48/0保持。

接下来优先一个明确原源闭包：

1. **后续覆盖到首次门控**：534f1a后的28 CALL、5371a0前四helper及JMP564b50。通过标准：phase/quota/gate/clock写入和当前读取之间的传递副作用、ABI/异常边有固定证据；未绑定实际时钟/入口475b00/live状态仍Unknown。
2. **选定派生postload方法**：+1c8、+84/+98/+1ec的一个实际候选类，核slot/Ob4/flag/root/重定位。通过标准：明确receiver/root来源及写集合，不能把基类no-op推广到派生对象或以局部无free当保活。
3. **owner34与reset时序**：4fbcc0传递callee及4fb530两个guard间写来源；补normal/exception路径。通过标准：每次guard值来源及reset触发独立证明，不能从同一字段名假设它未变。

先前文件buffered四缺边、双编号/append失败、回调Q+274/+cc/P150、OS异常/handler/退出重入仍保留。所有运行binding DENIED，不运行原或派生游戏、转换、SR/Unicorn/Wine或此前具体被拒差分路线；不读GameData/旧Swift，不操作UI/音频设备。媒体局部结果不能替代原版真实消费、完整周期或可玩证明。
