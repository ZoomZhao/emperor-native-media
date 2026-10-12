# 主池之后：首个Figure7记录与下一类边界

实际结构终点从792136推进到 **792570**，新增434字节。4e1e40段版本1、signed32计数2000；首声明匹配`Figure`/schema0，其字段段7载荷398字节完整通过。第二条出现12字节新类声明，schema0、名字SHA `f75b3a43235b9e9793f240bd88985c0eb4ad8bb2b7e0de490bbbed978fb186ba`，尚未映射，停在lookup/payload前。2000条只完成1条，不能把计数当已读完。

| 解码区间 | 本次资格 |
|---|---|
| `[792136,792142)` | 4e1e40自身段word1及4字节有符号计数2000 |
| `[792142,792154)` | Figure新类声明，schema0、6字节静态名字匹配 |
| `[792154,792552)` | 字段段7：版本2B、161项固定请求395B、可选对象存在位1B；本条无扩展载荷 |
| `[792552,792570)` | 第二条新类声明；未查registry或消费payload |
| `[792570,4430995)` | 未解释3638425字节 |

完整外框仍为165223输入字节、136块、4430995解码字节。共享archive现代条件cache从4018新增Figure类4018/对象4019到next4020；未知新类只给条件后续编号4020/4021，没有append或新对象资格。当前原registry/vptr/cache/实例与生命周期仍未观察。[机器证据](city19-first-Figure7-evidence.json)保留命令、SHA、完整前序回归与来源ID，不上传原始/解码payload、字段/GUID或Listing。

## 宽度来源及条件

原caller53066a以同一ESI=S和receiver10315d8调用4e1e40。记录10315a0经41fc10/41fbf0读word，选定read/section1后，4e1f06→42dc20向local[esp14]写signed32计数（helper返回S）；N>0经4e1190→77fd90 expected84dea8载入每项。本地未见池计数上限；循环的+f8读后调用及计数/容器副作用不在现代模型中执行或解释。

固定静态descriptor84dea8/name84dec0为Figure/schema0，sizeof412、factory4e1100→constructor4c71d0；构造根7afe60，slot0 getter4e1160返回同descriptor，slot8=4c75c0。reset4c72b0在DF0条件下保留根；slot15c=4fa3f0的正常default返回0已核。先前误标slot4的探测实际是析构槽，保留但不用于type绑定。静态构造链只支持选定根，不证明原当前vptr/registry。

reader自身记录10314a8独立读字段word，实际为7，比较7选择4c8433分支。161项固定请求395B来自完整顺序、三个2/25/3固定循环和callee宽度逐项核对；不使用sizeof412或reset范围推长度。503db0通过41fca0读取2B再把低byte存进单字节目标，不能按目标大小算1B。4c95f0/4c9600读取1B；41fca0/41fcb0读2B；4c95e0/42dc20读4B；raw20显式请求20B。字段保持opaque。

末尾503d50读取1B并把任意非零归一为true，决定是否调用4d72f0→77fd90 expected84d2f8读取可选对象。该存在位是结构控制条件，不解释为活动状态；非零时现代读取器立即停在扩展payload前，当前对象元数据标为未完整资格，不能当活缓存对象复用。本次首记录存在位为无扩展，398B完整闭合。

完整读取、有效原存储、正常callee、所选static class/root及无同S清理/读后别名仍是来源前提。原raw reader有短读可能，现代严格完整字节政策不能改写成原逐字段严格校验。

## 样本计数与验收

本样本此前主池4000条是Building结构记录数，原4000比较是默认补空阈值，未证明全文件上限，也未解释其中活动状态。当前2000是Figure列表记录数，完成1条，活动数未知；重复对象引用也不等于多个唯一或活动实例。

现代限定为：保留前缀4915584B，旧pool视图仍1MiB；Figure计数16384、每批8192、cache32768；Figure schema0/字段段7，list section1。负计数、ASCII/非null、kind及资源防护是现代子集政策。未知类与扩展不跳长度，classref建新载荷、objectref只复用完整兼容元数据。旧五个组件头、所有CLI和五个reader核心字节不改。

Release/UBSan各 **456/0** 自造验收包含398个proper payload前缀截断、class/object/扩展tag、共享Goal/Building缓存、不兼容引用、未知类、任意非零扩展存在位停车、资源checkpoint、opaque活动字节、2000条自造连续记录及SHA/Adler拒绝。自造2000条通过不表示本文件2000条完成。初fixture初始化顺序及新CLI局部变量与已有stat重名的构建失败均保留；修正后完整suite通过。

d596187独审通过：原suite各62/0、独立专项各20/0、来源审计19/0，无阻断。本轮精确纳入四文件专项，当前未改组件下各20/0；另有20项完整前序统计/输入SHA/全部旧源码一致性通过。本增量尚待独审。

```sh
python3 file_io/tests/city19_after_mainpool_tests.py --source-root "$PWD" --output /tmp/city19-Figure7-new
python3 file_io/tests/independent_city19_mainpool_component/run_properties.py --source-root "$PWD" --output /tmp/city19-mainpool-properties-new
```

输出目录须不存在。新CLI沿用外框参数，可选末尾`MAX_POOL_ITEMS MAX_FIGURE_ITEMS`，默认4096/8192；声明probe及fieldword checkpoint的32/34项自造测试与真实回执保留在任务目录，不替代最终suite。

## 下一项

从静态literal匹配第二条12B类声明的SHA，闭合其descriptor/base/factory/根与独立reader，再按准确布局继续。同一S和条件cache4020保留；不能把Figure v7基类切片套为未知派生payload。后续cGoodsControlList/cEmpire已有部分静态来源，但本样本尚未到达它们，不按预测偏移跳过2000项。原执行、活动状态、完整城市载入及更新入口仍未证明。
