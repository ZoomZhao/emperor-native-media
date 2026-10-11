# 原对象条件生命周期与退出回调

固定English PE `8a6d2df1…6753`、REA profile `e3402f2d…a7b2`。本轮补的是原调用证据，未运行原/转换游戏、未生成绑定、未重测自写接口。

## 构造、登记与同池消费

5268d0虽未注册procedure，69条官方Listing连续覆盖232字节[5268d0,5269b8)，6个直接CALL、两个RET4，全部本地分支闭合。六个helper及共享4e07c0已核对：全局getter只读字段；临时对象的构造/计算只写本方法栈上对象；4ed150尾跳普通RET。只记录标量ABI与写集合，不复刻或命名城市规则。它与418e80其他已读helper在选定正常路径上没有取得外层slot写地址。

构造源426e60明确清零O+4..O+b7，再以O+b8调用CoCreateGuid；51c9a0的嵌入对象位于O+c8，最终根7b65e4、O14c=0。因而在正常API与内存合同下，O51构造初值为零。42d540在旧对象virtual+4与新factory之间持有rawslot S，先[S]=O、Ob4=index，再进入+94；没有retain/pin，factory null也没有在首槽解引用前检查。

对于选定根，+94的+90进入51cad0、fresh aux构造和+1fc，再经418e80按aux保存的索引从同一begin字段8c7638读slot指针Q。47f1b0只是计算地址，没有边界或保活检查。若O/root/索引/S均有效，旧对象删除与分配正常、fresh aux与O/栈/池互不别名，所选表成立，调用期间无异步或重入改写，那么正常helper写集合支持Q=O及先读构造值O51=0、后由428b6c写新值的**条件推论**。这些前提未动态验证，不能省略成“实际同一live对象已证明”。

旧对象删除现在有两个可选源合同：root7ab59c的426ce0调用426d00，只把旧root写回7ab59c并进入RET；root7b65e4的51ca00/51ca20会删除非空O14c的子对象、清空O14c、处理O+c8后进入426d00。子root7b6b3c的526850/526870/418fe0只做所见root变更和条件free。删除包装器的flags低字节bit0确实控制释放；这是另一条合同，不与426d10的flag1/3筛选混写。返回旧指针位型不表示还能访问。实际旧类、heap模式2/3、异常及后续+130仍有缺口。

## 分配失败回调

77bb08→766b04→766b16→766b42；heap模式非2/3的选定分支指向HeapAlloc，模式2/3内部allocator还需追踪。失败可能先通过7703a0调用当前1e58524，再由77bb08调用当前88a3d4；两者不能合并。固定文件88a3d4初值为77baee，调用77aeba进入既有76501e异常路径，ThrowInfo80fdf0；异常是否被捕获、回到哪一帧仍未知，不能补成返回null后继续。

真正setter为77baf6：18字节完整Listing返回旧handler、保存传入handler、RET4。一个真实调用者77d715在77d739安装780ad9，正常路径在77d76f恢复先前EBX值，期间分配0x48。因此初始值不代表全部运行时值，异常恢复与替换handler效果仍需证据。早期请求77baf0返回的是包含它的77baee函数体，标签“setter”不作为语义证据；setter已按准确指令地址核定。

## CRT初始化、登记、消费与失败

769f80先扫描[81578c,8157a8)，再扫描[814000,815788)。固定815790项为764afb，先于池initializer42d100所在的第二组。764afb的14条Listing完整47字节：分配128，设置begin1e58f54、首word0和end1e58f50=begin。分配失败会调用7689c2；其后若返回仍解引用当前begin。7689c2含日志helper及当前函数指针88aa00(status255)，实际终止路径尚未闭合。

42d120把42d130交764ae9→764a6b。764a6b申请锁号13，容量不足时以旧容量+16调用76b86e，成功重定位end，写callback并end+=4；扩容失败返回0且不追加。764ae9将成功指针转EAX0、失败零转EAX-1；42d120只POP/RET，未检查返回值。**池初始化正常返回不证明退出callback登记成功。** 分配/锁初始化/异常失败不能统一说成普通返回零。

原入口在4754f0返回后，于76899e调用769fad，传(status,0,0)给769fcf。正常cleanup分支从end-4反向遍历到begin，跳过null并CALL callback，再消费CRT终止表，最终调用ExitProcess。769fbe的(status,1,0)跳过callback数组；已经退出的guard分支指向TerminateProcess；第三参数非零的路径可解锁后返回。这些来源关闭了正常退出消费者，而非证明自然退出发生。

若42d130确已追加、逆向遍历能正常到达且数组/cursor有效，它固定ECX8c7630尾跳42d1e0。后者进入42d250(1,0)，506480遍历每个非空slot，virtual+4(flags1)后置slot0；490c80重置vector，4f8020后来释放buffer并清边界。清理中的其余reset helper仍有副作用缺口。退出consumer在锁13内调用callback，但raw ESI跨callback、begin每轮重读；回调再登记/扩容或改写数组仍可能让cursor失效，不能据锁名保证重入或生命周期安全。

## 复核与范围

独立二方对c0bbdff为34组通过/1组flag误述失败；c82fced新增29组通过，整体仍继承同一失败。本批依原Listing修正为flag==1或3，附[flag5反例](ORIGINAL_FILTER_CONTRACT_CORRECTION.md)，修复与本轮新语义尚待复核。旧Evidence/提交/失败收据不改写。

本轮会话exit0/869.791秒，140请求139Evidence/1NoProcedure（42a980），89指令锚点、28选定procedure地址；输入身份、18对选定官方模块SHA不变，官方清理后临时根空。已取+130目标42a980的PUSH EBX入口，未声称完整。后续继续该方法、handler替换/异常恢复、特殊heap模式和异常退出链，静态工作未穷尽。

## 后续证据与复核更新（2026-10-11）

3a15dc7已经独立复核44组通过、0项新增语义失败，前文“本轮修复待复核”仅描述该提交制作时状态；历史c0/c82失败收据保持原样。本次[回调合同](ORIGINAL_CALLBACK_LIFETIME_CONTRACT.md)补齐+130、5ad440存活缺口、d2局部作用、780ad9真实释放/缩减和77d715所选异常funclet。77d715的0x48分配为72字节，state0在分配返回后设置；action7a8650只释放[EBP8]，不恢复handler，实际OS异常/外层恢复仍Unknown。88aa00的固定初值769fbe只支持条件quick-exit。新增语义未独立复核，不声称UAF或完整游戏通过。
