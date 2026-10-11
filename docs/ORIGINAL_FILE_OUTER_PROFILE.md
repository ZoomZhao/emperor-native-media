# 选定原文件外层 profile 的静态来源

已核定一套**新建 F、选定 root、flags0 打开、完整文件头匹配**的条件 profile。参数来自固定 English x86 SHA `8a6d2df1015cb75d797546d117da5f82b86fd08726090c2a13d853b9009d6753`，REA profile `e3402f2d7c487875edf3c6215fa95d67ee1fa133ba68c2390a9338b38656a7b2`，没有用合成 marker 充当原格式。新来源尚待独立复核，实际文件未验收。

[可读参数](original-file-outer-profile.json) · [来源索引与实际夹具统计](original-file-outer-profile-evidence.json)。本轮只增加文档参数，不修改 `file_io` 实现或字节请求 API；组件固定提交为 `16b4c8c8489b9003db8b31e1a66f9275bf52e91b`，已交独立代码/运行审核。

| 参数 | 原来源与字节 |
|---|---|
| 文件头 | 447be2 写 F+10=`fedcbaaa`，LE32 bytes `aa ba dc fe`。 |
| 压缩块 marker | 447bf7 写 F+14=`abcdefff`，LE32 bytes `ff ef cd ab`。 |
| 原样块 marker | 447bfe 写 F+18=`aaabcdef`，LE32 bytes `ef cd ab aa`。 |
| 构造根 | 447b70 经 447b9c 写 root `7ac7d4`，447ba2 调 447be0；初始化 F1c/F1d/F1e/F1f 为零。 |
| 对应虚表 | 112 选定 bytes 确认 +28=4481f0、+48=4481c0、+58=447c70、+60=447d30、+64=447f80、+68=448150。 |

这是**构造来源**，不能推广成所有 live F 的当前 marker 值。使用该 profile 仍以新建选定 F/root、有效独立存储、正常 ABI/callee、无覆盖 alias 为前提；实际文件/对象没有运行绑定。447c70 的八条正常 native 节点只写 cursor、logical end 与两个计数，不直接改三个 marker。

## 模式和三个不同的大小

534a30 在 534a8f 新建 F，534aaf 请求 `447c90(path,flags0,null)`；成功分支 534b04 创建 archive `7803c1(S,F,mode1,4096,null)`。五个选定 native 锚点确认参数与调用；mode 存 S14，buffer size 存 S1c。通用 archive 构造对 signed size 小于128的请求改为128；本调用4096保持4096。它还调用 F 当前+18等外部方法，实际副作用仍有条件。

对照写出调用 534880：文件 flags=`1001`，5349a9 的 archive mode=0、size4096、supplied buffer=null；五个 native 参数锚点分别核定。不能拿 mode0 的调用证明读模式。空 caller metadata 也不证明某个虚表方法没有调用者。

| 数值 | 角色 |
|---|---|
| 4096 | 选定 archive 的缓冲请求，通用 constructor 参数。 |
| 32768 | 447c70 设 F20=F28、F24=F8028，逻辑缓冲跨度0x8000；另外，codec 的 window 也为32768，但角色不同。 |
| 36144 | 输出 `[F28,F8d58)`、输入 `[F8d58,F11a88)` 各自的名义区域，以及 codec 的预置输出 capacity。 |

**固定安全模式**读取前要求 N<=36144、actual produced<=36144，这是对名义区域的自有安全政策，原 fill 没有 N 上限门。原 writer 的 cursor guard 是 `F28<F20<=F24`；它没有在本地证明 pending count 与 cursor 差一致，不能仅凭该 guard 给所有声明长度盖章。

**通用自有配置**是 `FrameProfile` / `emfr_decode_limits`：生产读取器没有原 magic 或36144硬编码，单块及总输入/总输出/块数由可信调用者显式给定。更大的自有 buffer 配置扩展其 owned-buffer 支持域，不能称仍符合原固定缓冲兼容范围；总预算的数字也没有原来源默认值。

## 文件头、位置与相邻块

447c90 正常成功后经+58复位，read flags0 分支在447cf5请求四字节头，**未检查实际返回数**，随后比较读取 word 与 F10。匹配置F1e=1并立即调用+60 fill；在完整正常四字节消耗且起点已限定时，首 frame 从相对物理偏移4开始。Native完整头要求是修复政策，不继承短头部未检查行为；原 open 会预填，新组件按 frame-event 请求读入，尚无原字节请求 API 等价声明。

不匹配时448280向当前+28提交 offset0/origin0。选定 root 指到4481f0：先+48处理，再调用77c346，条件需要时还会+60 fill；447c90随后置F1e0走原 raw-file passthrough。底层77c346/OS seek成功未在本轮闭合，故只称**请求定位**，不称真实文件位置已恢复。当前自写组件拒绝未知外层头，未实现该 passthrough。

447f80 的正常局部 whole Listing为80条、16 native锚点。写 flags 与 cursor有效时，F1c非零写原样块（F18、F11a88、原样payload）；默认F1c0请求613970压缩（目标F8d58、目标长度指针F11a8c、源F28、源count F11a88、预置capacity36144），忽略状态，再依次写 **F14 marker / F11a8c encoded N / F11a88 第三word / N payload**。没有扩查或重建613970/旧zlib。

第三word因此有了**选定 writer 的 pending源计数来源**；正常、不别名且计数可信时，它记录传给压缩请求的输入字节量。reader仍只是消耗四字节，不要求它等于actual produced；其它文件/路径的意义与全局关系不能推广。因此自写reader继续保留 `opaque_word`，不增产量相等门。

448150先flush/baseflush再置F1c1，提供原样写政策的虚表入口；其caller列表为空不代表未被间接调用。4481c0在写flags下条件调用+64 writer后复位；448250 close请求+48、底层77c3bf、清F1e并复位。选定局部方法没有独立终止magic写入，底层close/flush传递副作用与真实EOF没有全部证明。缓存447e00 copy-clear-fill循环及447d30每压缩块fresh codec支持相邻frame来源；payload内terminal tail仍属于声明N，不当下一frame。现代空 `Frame` 和完整边界 EOF 政策不冒充原终止协议。

## 已运行的151个独立夹具

固定组件 SHA `16b4c8c8489b9003db8b31e1a66f9275bf52e91b`，Release与UBSan各运行151个独立单块case的七种交付（完整、1/2/3/4/7/4096），每种 **1,057次、0不符合预期**。首块唯一判定为19成功、79截断拒绝、41 codec拒绝、8输入超限、4输出超限。另60直接adapter/guard、77现代流政策、21容器/API/IO检查，各构建合计1,215/0。实际结果与SHA绑定见JSON，不将132个按预期拒绝称为原文件读取成功。

不可计成完成：独立夹具的15 pending保持原样；11流案例另按明确现代政策验证，零请求两项、恰请求边界、外部near-U64累计计数四项API语义仍pending。本轮没有重跑组件或把合成输入换成真实文件，没有新增字节请求工作。

## 验证范围和剩余资格

官方60查询/60Evidence/0provider错误，8 procedure参数、48 native查询全部decoded、112选定data bytes；8完整正常Listing图184节点，内33 native锚点，只有reset8与restart5为全native。正常CALL边以callee正常返回为前提，不称全传递闭包。exit0/327.138秒，固定输入与18组选定官方模块指纹不变/tmp空/0活动会话。作者图检查的空格与地址前导零两个读取差异已修正，只规范比较，不改原证据；原失败记录保留。

下一步是独立核这套条件profile、组件代码/运行结果，再决定 Native 接入配置；不能由构造常量推出现有所有文件值，也不能给未核OS定位、alias/callback、底层footer、旧codec任意坏输入或真实存档补通过结论。没有原/衍生游戏执行、GameData读取、运行绑定、转换、安装、UI/声音设备或联网/push。待发布范围单列，本轮不重试推送。
