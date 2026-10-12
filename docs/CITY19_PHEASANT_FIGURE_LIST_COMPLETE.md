# cPheasantFig：具体类布局与2000项列表完成

实际4e1e40列表的2000条结构记录全部完成，终点为 **1592210**。其中1979条映射Figure/schema0/字段段7，载荷398B；21条映射cPheasantFig/schema0/自身段2在前、Figure7在后，载荷400B。两个新类声明、1998次class引用、0次object引用，无未知类/扩展跳过。相对5a9a23d的792570新增799640B，整列表从792136共800074B，剩2838785B尚未解释。

这些是序列结构记录，不是活动、唯一活实例或可运行城市证明。Building4000、Figure列表2000均为本样本计数，活动状态/数量未解释；原registry/cache/vptr/factory/生命周期及读后+f8行为未实测。原对象、codec、构造和hook不执行。完整命令、来源和SHA见 [机器证据](city19-pheasant-Figure-list-evidence.json)。

## 类型与读取顺序

第二声明的12B名字SHA在固定English PE官方字符串库存匹配cPheasantFig@85d1b8。descriptor85d1a0/schema0/sizeof412/factory5a0bc0；静态基链85d0c8(cAnimalFig)→84dea8(Figure)→7cd140满足expected Figure。该匹配与静态链不证明原当前registry唯一解析。

factory5a0bc0→constructor5a0c50，正常调用59d7d0后明确写根7bbdb0，之后只写$self40、无后续callee覆盖根。59d7d0调用Figure构造4c71d0；根slot0=5a0c20，确切返回85d1a0；slot8=5a0c70。sizeof相同仍不能推导398B。

普通官方分析没有5a0c70函数体，native首指令可解码。通过此前授权的官方单函数seed在任务副本恢复完整读取器；旧2个NoProcedure保留，caller入口声明与恢复后完整source分开。原PE/既有数据库及受核官方模块不改。

5a0c70**先**对自身record13128d0读取word，选定read/own2后在5a0cb4直接调用4c75c0。后者读取Figure自身word7、161项395B及存在位1B；cAnimalFig虽在descriptor基链上，其序列化方法不在这次调用链中。实际宽度为`2+(2+395+1)=400`，不是先Figure再自身word，也不是按基类层数加word。原其它自身版本绕过parent后正常设置2，现代仅接2，不能称原明确拒绝其它版本。class schema0、own2和Figure7是三个不同来源。

| 实际范围/计数 | 资格 |
|---|---|
| `[792136,792142)` | list1/count2000 |
| `[792142,792154)` | Figure类声明，条件类4018 |
| `[792154,792552)` | 首Figure7，398B，条件对象4019；与5a9a23d相同 |
| `[792552,792570)` | cPheasantFig类声明，条件类4020 |
| `[792570,792970)` | 首Pheasant2→Figure7，400B，条件对象4021 |
| 后续1998项至1592210 | 依class编号选择398/400具体布局；全列表1979/21条 |
| `[1592210,4430995)` | cGoodsControlList及后续未解释，2838785B |

shared archive条件cache从4018到6020，类4018/4020、2000个记录编号（保留类占号），无objectref。编号不能等同池slot或opaque字段，也不证明原活对象。可选存在位任意非零时停在4d72f0扩展payload前并保留当前对象未完整资格；本样本这些记录均无扩展。活动字段保持opaque。

## 实现与验收

新增单独variants header/tool，按缓存class ID保存具体布局选择；classref创建新载荷，objectref只复用完整兼容元数据。API默认仍是Figure-only；新CLI默认启用已核Pheasant，末尾`ENABLE_PHEASANT=0`保留旧边界。旧七个组件头、全部CLI与五个核心不改。prefix4915584/count16384/items8192/cache32768等现代范围不放宽，也不是原上限；kind/ASCII/非null/完整读与版本子集仍是现代政策。

Release/UBSan各23/0属性组，覆盖400B顺序、全部400个proper payload前缀截断、具体缓存布局、混合398/400、重复/扩展引用、旧Building不兼容、default-disabled、schema0与own2/parent7、资源边界、opaque活动位及非零扩展停车。另有CLI集成各3/0（启用、关闭、非法开关），共6/0，自造frame输入；没有原输入或原游戏。生成测试脚本时一次目录未建的失败保留，实际测试使用完整的独立fixture源码。

两次真实限定运行：开关0的**整个JSON**与5a9a23d一致；开关1完成全部2000项到1592210。前序23项回归全部通过，包括完整Goal/主池/四前置消费者、首Figure及输入/解码SHA、全部旧源码。没有无关重跑旧suite。本增量尚待独审，前序d596187独审专项已在5a9a23d纳入。

```sh
python3 file_io/tests/run_pheasant_properties.py --source-root "$PWD" --output /tmp/city19-pheasant-checks-new
```

输出目录须不存在。新CLI沿用外框参数，可选`MAX_POOL_ITEMS MAX_FIGURE_ITEMS ENABLE_PHEASANT`，最后一项仅接0/1。

## 下一实际消费者

1592210之后为530679经getter506240/receiver10bc910当前+8的cGoodsControlList，选定静态root7b430c、reader506710。已有其section1/raw-count4与cGoodsControl内联reader的静态来源，但实际word/count/记录未读取；不能按默认200或sizeof跳过。下一步先限定消费准确请求，继续保留同S/cache6020。后续cEmpire、state/phase与更新入口仍未到达，当前无分析器或审批等待。
