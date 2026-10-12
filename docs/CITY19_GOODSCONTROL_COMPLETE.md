# cGoodsControlList：200个内联记录完成

实际结构终点从1592210推进到 **1646216**，新增54006B：列表独立段1、signed32 count200、200个cGoodsControl内联段1，各270B。整段为`2+4+200*(2+268)`。同一archive条件cache保持6020，无class/object tag或编号append；整个前序Figure/主池/Goal与输入/解码SHA保持一致。余下2784779B未解释，下一消费者为cEmpire。

记录值及业务字段保持opaque，200不是活动数量或原全局上限；原对象/分配/cleanup/codec/hook不执行，原当前registry/vptr/cache/lifetime及完整城市载入未证明。来源和回执见 [机器证据](city19-goodscontrol-evidence.json)。

## 准确读序

原caller530679经getter506240取得receiver10bc910，再以同一S调用当前+8。选定constructor506250写根7b430c；slot0 getter506210→descriptor852500/name852540=cGoodsControlList/schema0，slot8=506710。reset506350→506480/506380的保存完整source支持选定路径；sizeof20及默认创建200项不推wire长度或计数上限。

506710读自身record10bc908 word，read/source1在5067a5→780533向stack[esp10]明确请求4B计数，未检查返回count；TEST/JLE按signed count遍历，先506380重建pointer vector，再413a50取得各预构造对象、依当前+8读入。它不是ReadObject协议，没有serialized class/schema/tag。负数本地未显式拒绝，现代单独拒绝；完整字节防护不等于原严格返回数检查。

506380正常以sizeof0x110分配并调用5068d0。根7b431c、slot0 getter5068a0→descriptor852518/name852530=cGoodsControl/schema0、slot8=5069b0；reset506920在DF0及有效对象条件下保留偏移0。原分配、destructor、vector生命周期/别名不执行，正常callee和无同S副作用仍是条件。

5069b0读自身record10bc8f8 word，source1有15个780533请求，宽度依序`4,2,2,1,1,36,1,1,1,1,2,72,72,36,36`，共268B，加word2得270B，不能用sizeof272替代。receiver偏移4、8、a、c、d、e、32、33、34、35、36、38、80、c8、ec与调用逐项核对；raw数组和可能的ID/指针位不解释，不赋archive编号或业务含义。其它版本原方法可能跳字段后设置1，现代只接1，不冒称原显式拒绝其它版本。

| 解码范围 | 实际资格 |
|---|---|
| `[1592210,1592216)` | list word1 + raw signed count200 |
| `[1592216,1592486)` | ordinal0/word1 +268B |
| `[1592486,1645946)` | ordinals1..198，270B各项 |
| `[1645946,1646216)` | ordinal199，270B |
| `[1646216,4430995)` | cEmpire及后续，2784779B未解释 |

## 验收与边界

新增独立Goods header/tool和15项布局表；旧八个组件头、旧CLI及五个核心不改。现代prefix4915584/count8192/items8192是资源政策，支持下一记录前checkpoint；API要求前序Figure列表完整且无pending类/扩展，同一不可变输入由CLI绑定完整SHA。

Release/UBSan各16/0属性组，包含全部270个proper record截断、6个header截断、200及201条自造记录、版本/负数/预算、cache保持、前序pending拒绝及未读Empire尾。CLI两模式各1个自造frame通过及各1个无效预算拒绝。真实200条全通过；18项前序统计/身份及旧源码连续性通过。旧整套未无关重跑，无新REA查询或后台分析器。本增量尚待独审。

```sh
python3 file_io/tests/run_goodscontrol_properties.py --source-root "$PWD" --output /tmp/city19-goodscontrol-checks-new
```

输出目录须不存在。新CLI沿用外框参数，可选末尾`MAX_POOL_ITEMS MAX_FIGURE_ITEMS ENABLE_PHEASANT MAX_GOODS_ITEMS`，默认4096/8192/1/8192。

下一实际消费者1646216：getter480740→receiverc058d0，选定根7acd58/+8 reader480800(cEmpire)。其静态三版本分支、内联对象及22次后续调用已有来源，实际word/count/payload未读，不能按sizeof32、默认200或grid stride3c00跳过。后续state/phase/gate/quota、原生命周期和完整更新仍未证明，无审批等待。
