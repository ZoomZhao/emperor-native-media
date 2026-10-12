# 城市 v19 存档候选的完整有界现代块资格

`GameData/Save/zoom/历史战役自动保存_1.sav` 的完整只读输入为165223字节，SHA256 `e002894ce7dfa91d374715356708433afa70129bff673ce667f47b3520a12845`。外层预检在完整输入中找到136个完整包络；预检没有解码或把 opaque 字段当作解码长度。按每块最大36144字节计算总输出上限4915584字节，输入预算262144字节、块预算136，每块 encoded/decoded 上限36144。专用工具另外具有16MiB/512块绝对上限，native 子进程最多60秒。

**完整现代容器与块资格通过。** 136块均为 compressed，全部 zlib stream end 与 Adler 有效，codec consumed=N、unused=0；135块各32768字节，末块7315字节，累计4430995字节。现代完整边界End，文件尾部0。累计解码 SHA256 为 `fe4032203aed5dae2f131c975cb1e71f19bfacac4fd5b36e240607dc4491b26f`。输入以 canonical path、O_RDONLY/NOFOLLOW 打开，完整 bounded read、EOF probe 与前后 metadata 检查保持一致；输入没有写回。

解码头匹配 version19。最小原始来源为 `534a30 -> 533f20 -> 52fda0(S,0)` 的 mode1/4096 路径；`41fc10 -> 41fca0 -> 41fcb0` 在读分支取两字节，`41fbf0` 返回 AX。已保存 native 分派支持13–19，19进入 `5305d4`，随后才调用 `52fa30`。本轮复用这些已审源码，未新增原始二进制查询。首版本字匹配不证明样本在原程序中实际选择此入口，也不独立证明完整archive类型。字段解释到解码偏移2即停止；没有执行 `52fa30`、原始回调或对象反序列化。

该结果是精确输入 SHA、现代 framing/zlib 与有限预算下的资格。它不证明原始加载器、城市行为、对象/类登记、地图/地形、OS调用或完整存档兼容。原进程实时实例、vptr、异常和终止行为未补运行证明。源读序采用普通无同流别名副作用前提；metadata检查也不宣称并发修改下的原子快照。

容量增量只新增独立 `qualify_city_archive.cpp` 与 CLI；五个已审生产 reader/codec 文件、原外层profile和旧小文件工具8192字节域不变。扩容量由本次165223字节/136包络的显式任务授权而来，没有先触发工具拒绝再替换路径。源与profile哈希仍显式检查，优化模式在输入/编译前拒绝，native再次核验完整预期输入SHA。工具只报告匹配的19版本，非匹配或缺失的版本为null；CLI遇到非完整End、非19头或无效块会失败并保留统计。

仅为这次容量、SHA和跨块版本字增量做针对性自造输入检查；没有重跑已独审关闭的旧P2批次。执行来源、命令、逐块统计、原始入口绑定见 [证据](city19-real-file-block-qualification-evidence.json)。原始与解码内容只在私有内存，未存为数据文件或入库，仓库只含自写工具和摘要/计数/来源文档。

地图的独立边界保持为1463；其嵌入成员类型仍未闭合，不能借本次城市块资格宣称地图或地形已完成。现在已有完整块校验且头匹配19的真实城市样本，下一来源工作应从 `52fa30` 的消费结构开始限定核对，再决定可解释哪些后续字段。

复跑须明确给出该完整输入身份和预算，结果目录使用新的任务路径：

```sh
python3 -B file_io/qualify_city_archive.py \
  --input '/Users/zoomzhao/Workspace/emperor-native-media/GameData/Save/zoom/历史战役自动保存_1.sav' \
  --expected-bytes 165223 \
  --expected-input-sha256 e002894ce7dfa91d374715356708433afa70129bff673ce667f47b3520a12845 \
  --max-input 262144 --max-output 4915584 --max-blocks 136 \
  --output /tmp/emperor-city19-qualification-NEW
```
