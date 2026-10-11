# 读后 hook 与首次可能进入调度的条件

固定English PE `8a6d2df1…6753` / profile `e3402f2d…a7b2`，延续城市19/池1/选定Building schema0/字段段5的源条件。本批证明读后处理与下一次可能分派的**条件控制路径**，没有真实存档、live对象或实际tick。运行绑定继续DENIED。

## 对象确有读后处理，不能命名为统一开启

534a30先在534b76调用534bf0，再于534b80以W=8c7630调用42da10；534bf0内部535024还调用一次42da10。两次站点之间有其它callee，不能断言对象/槽/root未变或两次处理了同一集合。

42da10先413b40(W,1)取得slot1地址；该helper在count<=1时可能走413b70扩容，不能无条件说只读遍历。正常循环从1起、跳过slot0、每次raw EDI加4并重新读count。426d10(O,0)接受O4精确1/3，额外O4==6也进入当前virtual+1c8；不是bit0、任意非零或flag5。返回值未消费。选定稳定基类根7ab59c的+1c8=413a00，仅XOR AL,AL/RET，不写字段；派生目标、槽/根/Ob4是否被别处改写、raw cursor跨虚方法存活和重定位仍Unknown。

更早534bf0@534ec4直接CALL4aeeb0，它就是既有phase8的helper。它按记录/对象条件消费主池并重算自己的数组；不能称scheduler正常推进phase或完成周期。另有4acfc0的+84、4acc40的+98、5636b0的+1ec，实际对象/派生callee及传递作用仍未闭合。

## 写入站点、后续覆盖与条件必须分开

| 字段/来源 | 明确写入 | 后续条件与覆盖缺口 |
|---|---|---|
| speed10de0dc | 534c91从10de120写入，限owner+10 predicate为零；4fb779在共同join写70。 | 多个后续helper、字段/owner当前值未知；不能称真实速度已保持。 |
| quota阻塞byte10de0d0 | 534f1a写0。 | 后面仍有28个CALL站点，逐站保留在索引；不能称首tick时一定为0。 |
| gate88ec38 | 536b20先写10c6f60=0，42e430固定case0@42e473写gate0；4fb60c也写0。 | 42e430另外case2写gate1。写后5a6980、文件/其它hook与tick前helper未闭合，不能说helper总清gate或最终获准调度。 |
| mode10de045 | 536dff或4fb753写1。 | 载入函数返回值被忽略，mode1不证明成功载入；仍需下一次入口及门控。 |

534bf0的本地Listing没有直接CALL4ac240或直接phase归零；传递调用/别名仍可能改写。40fc20固定源仅RET，不可依据旧标签补出暂停操作。其它“gate-write”查询标签实际取得的是CMP/MOV读引用，标签不能当写入证据。

## reset是另一明确分支，外层周期为1

4ac240直接把c82ef8 phase、c82ef4累计、c82ef0子周期写0，随后4f8d50固定返回EAX1，写c82eec外层周期。4afd40仅清c82d30..54，两个REP STOSD清c82b5c..c82c27、c82c28..c82cf3，不覆盖上述计数。有效不别名平面内存、DF0、选定正常callee下，正常返回结果是**phase0/累计0/子周期0/外层周期1**，不是四个字段全零。

4ac240直接caller为42e6a0/4fb530/5d1400。选定4fb530的初次EBP+34!=0分支调用534a30；稍后的EBP+34==0分支才CALL4ac240。中间4fbcc0及其它callee存在，未证明两次guard值稳定；不能简单认定两个分支永远互斥，也不能把这个reset套到所有版本19读入。4fbcc0显示若干记录/对象字段更新，本地没有直接owner+34 store，但其传递helper/别名未闭合。

## 从正常载入返回到下一次可分派

534a30打开失败返回EAX1，正常读后路径返回EAX0。536d37调用者随后测试的AL来自40ff40，4fb73a调用者也不核载入返回。所选536b20当前循环的5371a0@536bc0发生在534a30@536d37之前；载入分支后mode1并RET536e2a。不能把本次前段tick当新载入状态的消费。

下一次入口536fe0→536b20(1)须当前mode非零且53a130配额为正，才CALL5371a0。配额还取时钟/previous115f540、速度/阈值、mode和暂停字段；若4fb779的speed70保持，index3、55+2=57ms仅是源条件，不是实测等待或必然进入。caller475b00已识别，实际入口触发仍未绑定。

5371a0先4189b0/529990/5ef190/5eff20，再读当前88ec38；零才于5371bf CALL4ac2b0。原分派器内部owner/flag/signed hold仍可提前返回，之后才消费读入phase；正常域和1..48分派合同保持。随后466e90/4e27e0可能改gate，5371d3重读；零时5371dc的真实JMP564b50是tail出口，provider的CALL_TERMINATOR不形成新call frame。tail作用/实际返回未闭合。

## 验证、交付与剩余任务

140请求/140官方Evidence/0 provider错误，24个procedure地址、107 native锚点、32选定数据字节。会话exit0/670.969秒，输入身份及18对官方模块SHA不变，临时项目空；没有活动分析器。七个选定正常局部图共95节点逐条匹配官方decoder并重建可达边；CALL只按正常返回边、5371dc只算外部tail出口，不证明callee作用、异常出口或循环终止。源/图一致性检查通过，5587fb5已独立37组通过/0新增载入矛盾、两处状态提醒在本地增量纠正；本批新增读后语义待独立复核；未执行原或派生游戏、组件测试、设备/UI或历史被拒路线。

下一批最有用：①沿534f1a之后的28个CALL补gate/quota/phase实际写集合，核tick前四helper及5371dc tail；②选一已识别派生类+1c8及+84/+98/+1ec，核槽/Ob4/flag与raw cursor覆盖；③追4fbcc0传递helper和owner+34写来源，把两次guard与reset先后关系闭合。通过标准均为固定源SHA/ID、ABI/正常与异常边、确切写入及后续覆盖；actual live实例/时钟/OS条件保留Unknown。

本批仅本地增量。5587fb5八份文档的GitHub push已被自动审批复审拒绝，等待用户精确目的地/载荷授权；本批没有再次push、网络、安装、替代通道或上传。新提交范围须另列清单，不能自动扩成已有八份的审批。见[筛选证据索引](original-postload-scheduler-entry-evidence.json)与[handoff](ORIGINAL_ROUTE_HANDOFF.md)。
