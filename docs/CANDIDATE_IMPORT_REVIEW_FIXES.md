# 候选导入 NoMemory 与文件名摘要修复

基线 `8298d31560cb6310b7a6de2c0b07051b665bf934`。独审报告位于 `city-validation/candidate-import-8298d31-review/README.zh-CN.md`，确认P2合法NoMemory误报为DecodeFailure、P3文件名换行可伪造摘要行。本次仅修复控制器状态映射和显示字段，不修改提供者、解析器、profile、预算或城市绑定。

## 修复

NoMemory现在显式映射到ResourceFailure，App共享格式化函数显示“资源分配失败；检查未完成”，保留当前失败诊断、不发布私有解码内容或旧范围；同文件可正常重试。既有外层bad_alloc处理仍保留，不能替代这个接口返回值分支。

显示basename现在是一个带引号的转义字段：LF/CR/tab、其它C0/DEL、C1、Unicode行/段分隔和方向控制字符均转义，引号和反斜杠也转义。普通中文/有效Unicode保持；非法UTF-8字节用十六进制显示。实际打开路径不变，不把转义用于文件操作。App只读NSTextView直接使用同一共享摘要，文件名不能再插入新的可信诊断段落。

## 实际验证与复核边界

扩展候选测试Release/UBSan各102项通过，包含原67项回归。新增测试用单次、精确36144字节分配拒绝，实际触发未修改FramedReader内部bad_alloc→合法ReadStatus::NoMemory，验证ResourceFailure、detail=no_memory、旧摘要/解码数据清空，以及取消故障后同文件恢复。这个分配hook只链接测试程序，不进入App或提供者源码。

审查方原始独立脚本和反例原样在私有目录复跑：两模式各7/0（实提供者控制器6组＋有效13字节raw帧的NoMemory接口替身1组）。这是本任务复跑审查方用例，**不是新的独立审核完成**。详细命令/源/结果在 `original-review-regressions/results.json`。

App编译、链接、打包、签名strict校验通过。实际GUI PID52033通过原生选择器选取内容为NOPE、文件名带换行和伪造成功/范围行的自造文件：名称显示为引号内字面\n字段，真实状态为Unsupported、解码否、0候选字节；摘要只有9个逻辑行，无伪造诊断行。此前成功范围已清空；重复检查和换回有效文件正常。自造文件SHA不变，默认静音保持，实例正常退出。截图和AX记录在任务 `candidate-import-review-fixes-20261013/ui-session/`；现场没有强制系统OOM，不能称全进程OOM恢复验收。

最初扩展夹具尝试创建含非法UTF-8字节的文件名，本机文件系统拒绝，两个失败目录保留；改为不可读路径的显示验证，没有放宽换行/control要求。原生选择器将换行显示为问号，首次辅助按原名匹配失败也保留；确认其显示标签后在同一面板选取，未重启App，未将辅助失败冒称生产缺陷或完成。

来源/结果/UI/提供者保护指纹见[收据](candidate-import-review-fixes-receipt.json)。不提交截图、App包、输入或原审查harness二进制。没有原EXE、GameData、真实存档、0736来源审查或被拒操作重试。完整原版兼容、身份和状态恢复仍未实现；两项修复提交后交由原审查方独立复核。
