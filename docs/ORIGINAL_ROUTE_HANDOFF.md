# 完整原版路线 handoff

起点提交19c49392d9d48cb3b36bff1d39947202c8064688，前一文档dbe5e608d2556b3092e96b3fdf52f286323360b3。当前目标仍是完整原版路线；已知的声明测试不重复，自写媒体不替代原链。

新增静态连接见ORIGINAL_OBJECT_FACTORY_ROUTE.md及original-scheduler-contract.json：CRT初始化表真正消费池initializer；固定a1调用者选到51c9a0/7b65e4，登记后同caller按返回索引读取同池O；主载入的九个版本站点与phase37也明确指向8c7630。全部是静态正常/条件路径，不是运行通过。

下一步仍有价值且可在已批准固定PE静态范围继续：

1. 428aa0没有procedure，需逐条官方Listing闭合428aa0..428c04的完整控制与direct callees，重点是是否写根vptr/池slot以及+1e8嵌入对象消费。已有入口、flag写、nested CALL、RET12和416b50 LEA/RET，勿再请求已失败的procedure体或强建函数。新会话应在开始前明确这一个有限范围所需指令预算，不能把旧10站点守卫改成成功。
2. 42bbd0的42bd28 virtual+130与登记旧对象42d70e virtual+4：核对可源证的构造表/目标/清参与slot所有权；其他子类和真实vptr保持Unknown。42d540持有raw S跨旧对象回调和新构造，不能当retain/pin。
3. 764a6b：补42d130 callback进入哪份registry、失败返回与退出消费者，静态构造/登记/销毁关系分开；未证明回调实际执行。
4. 52fda0/52e7c0：按具体版本分支及stream来源连接42d790与77fd90的实际类descriptor/deserialize消费者；17个secondary factory分支的剩余内层constructor继续展开。原输入/资源内容与完整载入未验证。

尚未完成的graphics/音视频原API调用路线可继续按原caller及导入槽静态追踪；现有CPU/Metal或离线PCM不减少原DirectDraw/Miles/Bink调用合同缺口。没有结论认为所有静态工作穷尽。

阻碍必须逐项记录：

- 当前resolve_native_call_targets(42d736)未执行，原因是本任务harness自定10-key-call-site限额；85证据/4NoProcedure保留、会话exit1、临时根空。不是自动审批拒绝或原路线全面禁令。
- 42d100/42d130的procedure_assembly/pseudo共4次NoProcedure属于函数管理器输入失败，不代表目标代码不存在；以明确Listing补入口/tail，未强建函数。
- 历史原城市指令恢复/差分两次拒绝仅有possible cybersecurity risk及任务描述，当前记录缺精确命令/目标子范围，不能补造或扩展为整个原版目标永久禁止。其具体被拒执行/转换路线不包装重试；若出现新的必要动作，先提出具体目标/输入/执行方式及范围，再按标准审批评估。
- 固定无目标诊断命令 node task-2/rea-static-gate-20261011/diagnose_startup.mjs 曾在CreateProcess前遭拒，理由涉及ownership helper与ps；该同一动作后来标准复审获准。历史拒绝与获准分开保存，当前正式REA静态生命周期已多批实际运行，不能把这个旧拒绝继续列为全静态阻塞。

本轮没有新的审批拒绝或待用户审批项。原自然运行、动态实例/异常上下文、完整周期/API等价最终仍需对应真实证据；这些终点缺口不能用来提前停止尚可进行的静态来源审计。GameData/旧Swift未读取，原/派生游戏库未执行，SR/Unicorn/Wine未调用，未恢复旧被拒路线。
