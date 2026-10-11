# 原入口、对象构造/登记与调度容器连接

本批沿真实原调用者补证，未修改或重复测试自写媒体/声明校验器。原入口7688ba在76895f调用769f80，随后才在768995进入4754f0。769f80将814000/815788交给76a086；该函数按32位指针逐项扫描[start,end)，跳过null并CALL非空值。固定814128项含42d100，官方Listing为CALL42d110后JMP42d120；42d110固定ECX8c7630尾跳42d140。因而在前序callback正常返回且表保持所见值时，这个池确由原CRT入口消费初始化，并非孤立候选数据。42d140正常路径reserve4000并填槽；reserve不等于全局size上界，null/分配失败与整初始化异常仍未证。

退出来源也补到42d120 PUSH42d130→764ae9→764a6b，42d130 Listing固定ECX8c7630/JMP42d1e0。注册helper实际写哪个registry、是否成功、退出时是否调用仍待展开，不能凭常见CRT模式默认已注册/已释放。42d100/42d130各两次NoProcedure失败保存，未强建函数；CALL/JMP由官方Listing确认，不按函数管理器失败宣称代码不存在。

载入和消费使用同一主池的源码证据：52e7c0在52e961以ECX8c7630调用42d790；52fda0九个版本分支也明示该receiver与stream ESI。52e7c0另有115d4f0池调用，不能混写。大型版本分支的完整输入/stream以及子类反序列化并未闭合。

**固定调用者给出更具体的构造关系。** 42bbd0在42bc53压literal selector=a1，42bc58固定ECX8c7630后调用42d540。用既有predicate汇编与本批选定常量表字做静态常量传播：5188b0 false、51c620 true，51c660前16个分支predicate均false，最终由51c96f调用51c9a0，分配0x150。此处不运行原函数、guest或模拟自然输入；结论只针对这个源码字面值及所选正常路径。五项稀疏表的case/目的地引用保存于机器合同。

51c9a0在子对象构造后末段把原O首word写7b65e4，返回同一O。42d540正常登记将factory返回O写saved slot S，写O+b4=index，再以同一O调用+94/三参数；正常RET20返回index。42bbd0紧接着用该returnedindex与ECX8c7634调用47f1b0，取回slot里的O。这关闭了一个具体“构造→登记→立即索引消费”的来源关系；不代表观察到了自然运行。factory null没有在42d729首槽解引用前检查；旧slot对象的virtual+4、slot原始地址跨回调存活和后续替换也不能默认安全。

本批新增九个构造根表的选定字均与先前四个候选一致：+94=428aa0、+168=413a00、+16c=416ac0、+170=416ad0、+1e8=416b50。九项是51c9a0、5da3d0、4f8e30、540770、51e5e0、4c5dc0和三个内层选定constructor416cb0/5b4000/50a570；17个外层分支不等于17个恢复类，内层还有多构造出口。完整vtable范围及实际实例仍未绑定。

登记后+94的缺口得到更具体定位：428aa0未注册procedure；所选Listing写O+4=3，正常RET12，但还含428b9a的virtual+1e8及未闭合direct callees。因此不能当纯setter，也不能默认不改vptr/全局slot。候选416b50的七字节Listing为LEAEAX,[ECX+c8]/RET，返回嵌入子对象地址，绝非boolean/no-op。phase37的44f1d0从同一8c7630池取O，426d10(O,0)检查flag bit0；所选登记值3满足该位，但O+3c、对象存活和调度时首槽仍需各自证明。随后调用者还有virtual+130等副作用前沿。

本批部分完成：85成功Evidence、4保存NoProcedure错误；89请求实际送达provider，90为harness尝试计数。最后追加resolve_native_call_targets(address42d736)在送达REA前触发自定10站点限额，会话exit1/718.770秒；这是控制脚本范围错误，非审批/平台安全拒绝。未重写为全通过、未改官方guard、未重试该请求。输入身份与18对选定模块SHA不变，退出后临时根空。原代码、转换代码未执行、原产物未上传，运行绑定与完整周期证明仍无。新语义未独立复核。

下一步继续原静态主线：[handoff](ORIGINAL_ROUTE_HANDOFF.md)列出428aa0完整Listing、virtual+130/+4、cleanup registry及版本化载入等具体入口。静态工作没有穷尽，当前不转去自写媒体回归。

## 后续静态证据增量

上文是c0bbdff时点的部分结果；后续完整428aa0 Listing及+90/子对象/同池嵌套消费已补到[注册合同](ORIGINAL_POSTREGISTRATION_CONTRACT.md)。旧预算exit1不被覆盖；transitiveeffects与实际实例仍未证。
