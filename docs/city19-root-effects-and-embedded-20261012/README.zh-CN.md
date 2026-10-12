# reset 副作用与嵌入读取合同

真实边界保持 **1991913**，新增真实已解释字节 **0**。本轮明确了 cHero reset 的临时对象写入与动态回调来源，并找到52d5f6嵌入对象的选定构造/root/reader链。新增显式合同的1479B独立模型，自造数据 Release/UBSan 合计 **28/0**；两份已通过独审的旧测试补丁已纳入，分别重放 **20/0**。没有读取真实存档或执行原游戏。

## cHero 构造安装与当前类型

既有证据证明510d30→512230分配成功后调用514500，512291把返回对象写入+30；分配失败写null。这是选定构造路径的赋值，当前返回指针、root和对象存续仍需证明。

本轮取得4f28e0、4f8060和4f2ff0完整静态体。4f28e0给4f8060设置ECX=ESP，参数指向child容器/子对象；helper直接写入目的临时对象的offset0/1B、4/4B、8/4B、12/4B。该调用不能当作child root[0]写入，跨callee及别名副作用仍未全部闭合。

4f28e0五组virtual+0x30的receiver来自容器元素，另一项来自child+0x14指针；最后一组virtual+0x1c来自child+0x18容器元素。它们把child作为参数，仍可能经该参数写入。**child自身虚表不能绑定这些其他对象的回调。** 4f2ff0继续调用相关成员/helper，4f2e90、4f66f0、4f6700、4f6710等路径仍需核查。

因此选定cHero初始化路径的root保留尚未闭合，当前+30类型仍未绑定。结论限定已查方法及相关写入，不作“全局不存在替换”的断言。

[相关写入、callee及剩余条件](hero-root-effects-and-remaining-callees.json)

## 嵌入读取：18B，选定整段模型1479B

reset调用者定位到52c990：它对owner+0x5c4调用52c440，随后调用父reset。52c440在52c443安装root **7b6be0**；其virtual+0指向 **52c480**，对应52d5f6。

子reset相关写入已局部闭合：52c460索引0..3，52c500两条正常路径核对到RET；其内部调用52c550的索引为3*i+k，k0..2，即0..11。直接写入只覆盖this+4..+19；在有效对象与栈、正常返回及无重叠别名条件下保留root[0]。20B存储覆盖不能代替18B线宽。

父own6全部66个raw请求的目的区间最晚止于 **+0x5c0**，早于嵌入root **+0x5c4**，不直接覆盖root。原archive正常完整读取、有效目的存储和无别名仍是条件。父reset中的4307f0(owner+0x404)副作用、实际构造到达、对象存续和读取时root有效性仍未证明。

52c480读模式先版本word，再raw4（52c4cf）、raw12（52c4dc），无嵌套读取；输入版本没有参与布局分支或拒绝，读取后设置内部版本1。现代组件只接受嵌入own1，不能据此称原版拒绝其他版本。

| 选定结构 | 字节 |
|---|---:|
| 父own6 word+66请求 | 1461 |
| 嵌入word+4+12 | 18 |
| 显式名义合同模型 | **1479** |

新API `walk_city19_next_owner6_nominal_record` 要求显式选择 `NextOwner6EmbeddedContract::nominal_52c480`，并声明 `verifies_original_instance=false`。它只验收给定内存中的选定字节布局，不执行reset/callee，不接实际CLI。旧1461B前缀API与布局保持字节不变，仍pending=true、嵌入消耗0。

[构造、局部root保留、目的区间与读取绑定](embedded-selected-contract-binding.json)

## 验证与恢复

新模型Release/UBSan各14/0、合计28/0：覆盖1479B子区间、旧API停止状态、全部1479个不足前缀、缺失嵌入记录、未知版本/合同拒绝、平移、尾部未读、opaque输入不变、预算、整数极值与null视图。成功不表示实际到达52cd90、当前root已绑定或完整存档通过。

```sh
python3 file_io/tests/run_next_owner6_nominal_record_properties.py --source-root . --output /tmp/emperor-owner6-nominal-fresh
```

36bd990与9b7e975两份独审补丁逐文件匹配私有交付。针对当前未改的组件重放，各Release10/0、UBSan10/0，合计各20/0；继续验收12107B source7和1461B own6前缀。此前各三种受控变异已由独审检出，本轮未重注入变异。

Mac恢复后重新核对：三套结果、源码与依赖SHA全部匹配，未重跑测试；HEAD仍为9b7e975，索引为空，用户修改与受保护源文件保持原样。前轮静态会话exit0、临时目录为空，记录的Node/Java PID均已退出；中断仅留下未写入的中文报告，未发生提交或推送。

[新模型验证](nominal-record-validation.json) · [补丁与重放](independent-test-adoption-and-replay.json) · [静态会话摘要](static-session-receipt.json)

下一步保持原版加载主线：核查4307f0的相关写入及实际初始化来源；绑定4f28e0容器/+0x14/+0x18回调的目标root和child参数副作用；继续4f2ff0相关成员及FigureFSA base、版本、字符串和嵌入reader合同。未闭合前保持1991913，不猜读。

本轮仅使用固定输入的静态分析与自写组件的自造数据测试。未执行原游戏/衍生库/原DLL或codec，未使用SR/Unicorn/Wine，未读旧Swift或操作UI、键盘、音频设备。现有提交/推送授权持续有效，无新增审批请求。
