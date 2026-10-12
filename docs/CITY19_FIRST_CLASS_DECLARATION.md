# v19候选首个对象声明：cHousingGoal / class schema0，到3379

同一165223字节候选仍完整通过136块，解码4430995字节，输入/解码SHA与4fd605e相同。3361处首对象原始16位tag为0xffff，进入新类声明：schema0，名字长度12，随后12个名字字节。解析终点3379，剩余4427616字节未解释。名字摘要 `c4291be23ebdfef80b50eeb34ec27a9f30d63fa29e8014a7447d482e9c9ae6c7` 精确匹配原程序字面量859690的cHousingGoal；工具仅输出名字摘要及长度，报告的名字来自匹配的静态字面量。原始/解码内容、对象字段值及编译二进制不上库。

## 已核对的通用协议

77ffc8调用41fcb0读16位tag。原始word==0x7fff时，再调用42dc20读完整32位内部tag；否则将低15位保留，原bit15移到内部bit31。内部bit31清零表示对象引用，0在所选初始化slot0-null条件下表示null；非零索引须从archive缓存取得对象，并以其虚槽0所返回的descriptor做expected类型检查，不重读payload。内部bit31置位且原始word不是0xffff时，表示非零类缓存引用。**新类声明只由原始word==0xffff选择**，extended内部值0x80007fff并不等价于新类声明。

7802fe的新类声明顺序为uint16 schema、uint16 nameLen、nameLen个名字字节。源要求nameLen<64和返回字节数等于nameLen，随后在本地缓冲补NUL，按lstrcmpA搜索当前注册类链。现代限定工具只接受非空ASCII identifier、拒绝embeddedNUL；这是工具范围，不是原程序的额外名字检查。工具允许抽取未解析class的声明schema，但不把抽取成功写成registry/schema接受通过。

77fe3f的所选mode1首次cache初始化建立slot0=null、nextindex=1。新类解析、schema检查后先登记class descriptor，再做expected检查；77fd90随后调用factory、登记object，再调用object虚槽+8(S)，正常返回才恢复S+c。初始无预seed、此前无class消费者、无同流别名等前提成立且后续正常完成时，首class和object的条件缓存编号为1和2；本轮没有调用这些原方法，也未注册或重建对象，不声称原进程实际缓存状态。

协议没有在这里另读独立instance marker：新类声明或已有类引用解析出descriptor后，都由77fd90进入factory路径创建新instance；普通对象引用走缓存对象路径，不调用本次schema reader。

非versionable descriptor的masked schema不等于声明schema时，源进入错误7；versionable bit31允许记录输入schema override。类引用可能使用override或descriptor schema，不能统一当作0。expected检查通过77bbc0沿descriptor+0x10指针链比较地址，而非名字或sizeof；缓存对象路径则经77bb42从对象虚槽0取得descriptor再检查。类缓存编号、对象缓存编号与主池槽号仍分别记录。

## 本次实际声明对应的静态类型关联

名字859690的唯一所选xref为descriptor859198。有限24字节数据观察给出名字指针859690、instance-size字段20、schema字段0、factory5598a0、base8590d8。后者是expected cGoalBase。按77bbc0所选正常指针链，859198→8590d8可满足expected检查；descriptor schema0也与本次声明0匹配。**这些是流声明到静态descriptor的条件关联，不是原进程的实际registry选择、当前vptr或对象类型证明。** 当前模块注册链成员关系、重复名字、allocator/异常与回调副作用继续未证。

5598a0所选正常factory申请20字节，调用559900；该构造先调用559120，再写vptr7b8350并调用560e50。560e50只写+4/+8/+c/+10，未写偏移0。7b8350前三槽为559970、5592d0、560ef0。559970缺少procedure元数据，NoProcedure保留；官方native在确切槽入口观察MOV EAX,859198以及随后RET，共两条正常直线指令，未把失败的procedure请求当作完整Listing。559990的四条native观察将859198传入40aa80，后者转给77bb9a；这条静态注册调用关系不足以证明运行时当前模块链成员资格。

## 下一真实依赖：schema reader与独立节版本

虚槽+8的560ef0只接收一个S参数、正常RET4，首先调用基类55feb0。基类先调用已保存的无字节40cf80，再读12a5a48记录的两字节版本、读4字节到self+4，正常尾部设记录1。559f90/92/97/9c的四条native观察确认其初始化1。560ef0另读12a59f0记录的两字节版本，再分别读4字节到self+8、+c、+10，正常尾部设1；55a2b0/b2/b7/bc确认该记录也初始化1。

因此class schema0、base节版本和housing节版本是三层不同状态。源reader没有在这些方法中按返回节版本选择字段读分支；现代后续工具若限定节版本1，必须继续标为自己的范围。所选正常完整请求的payload需求是2+4+2+4+4+4=20字节，但**3379之后的实际节版本与字段尚未核验，3399仅为条件算术，未记为本轮实际终点**。对象sizeof20不决定wire payload长度；这里的相等来自具体源读序，不是sizeof推断。

## 验收与范围

限定新工具保留前3430字节于私有内存，仅解释到声明结束。整流的完整输入、解码SHA、所有块验证必须先通过才输出统计。合成Release、UBSan各36/0，覆盖compact/extended null、extended值与新类marker区别、无seed引用拒绝、schema/nameLen/名字截断、长度63/64、现代ASCII/NUL/空名字范围、标准zlib与Adler/摘要错误。引用没有合格cache entry时明确拒绝，不猜类、不读未知payload；null统计也只属于协议元数据，不证明随后非空consumer安全。

同时修入c00独审P3：两个既有工具及新工具仅把短前缀诊断改为“fixed prefix through component2 truncated”，解析和拒绝行为未变。三个工具、两种native模式、八个短长度的48项定向检查全部通过；已有17/40项闭合批次未重复扩展为新成果。新工具诊断文字改变后，保留最初probe收据，重新绑定最终源码并复验同一实际声明。Python优化启动在创建输出目录前拒绝。

本批官方有限静态会话正常清理，输入身份及18份选定官方模块摘要一致；559970的NoProcedure仍记为元数据缺口，不是平台审批拒绝。上轮1170秒/exit1 partial与其3355条件计算保持原状态；3361和本轮3379分别由后续独立执行收据支持，不追溯归入旧partial。本轮为作者验收，追加独审尚未完成。源绑定及结果见 [证据](city19-first-class-declaration-evidence.json)。
