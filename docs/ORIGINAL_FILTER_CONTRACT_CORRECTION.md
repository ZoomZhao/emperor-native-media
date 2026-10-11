# 426d10筛选合同更正

保存权威Evidence来自固定English PE8a6d2df1：426d10读byte[ECX+4]，426d13比较1，426d17比较3；任一相等到426d20 MOV EAX,1，否则426d1b XOR EAX,EAX。两个出口均RET4；压入的32位参数在本方法中未读取。

正确合同为“仅byte值1或3返回完整EAX=1，其他值返回完整EAX=0”。最小反例flag=5：5&1=1，但两次CMP均不相等，走XOR EAX,EAX/RET4，返回0。因此原bit0/所有奇数说法错误，登记值3仍通过，但必须保留后续字段没有改变的前提。

修正文档ORIGINAL_OBJECT_FACTORY_ROUTE.md和original-scheduler-contract.json，增加明确派生合同与反例；检查docs、original_bridge、runtime_core和game_lab当前源码，未发现此门槛的奇数派生实现。删除包装器的flags bit0释放规则是不同源码合同，保持原证。历史c0bbdff/c82fced提交及原Evidence不重写；二方原失败记录保留，修正是否通过需单独复核，不能称原固定提交全通过。
