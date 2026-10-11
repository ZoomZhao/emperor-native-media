# 两份本地数据的块解码资格

在明确授权下，以固定已审OwnedFramedReader、系统zlib1.2.12及自写只读统计工具，完成两个原始本地数据文件的有界块解码。**结果仅是指定输入SHA的container/块解码资格，不是原城市加载、存档兼容、完整游戏或原对象/异常/OS链通过。** 没有原EXE、原codec、派生游戏指令或城市反序列化执行。

| 本地输入（GameData相对路径） | 完整输入字节 | 完整压缩块 | 校验后产量 | 终止 | 未消费文件尾部 |
|---|---:|---:|---:|---|---:|
| Campaigns/UNTITLED/UNTITLEDP.map | 5224 | 15 | 471164 | 完整边界End | 0 |
| Save/zoom/C02M00_empireinfo.dat | 7147 | 14 | 430024 | 完整边界End | 0 |

完整输入SHA256分别为：

- .map：`60a66d61a51d5f4daf44d0355d9af372afc0afff767dc90899cfa1815e3068b8`
- .dat：`36a8193f09a51b5fe84e32746ac756f0d7e9b20920fafb5fc35221c73b1c040c`

每块完整marker/LE32长度/opaque/payload均按审核profile消费，host +15 stream-end与Adler检查成功；29个块的codec consumed均等于声明N，codec内unused payload均0。实际产量每块均不超过36144。各块产量及SHA、frame位置、framing consumed、codec consumed/private produced/published、opaque与terminal原因保存在[无原始字节统计](real-file-block-decode-qualification-evidence.json)。没有将opaque==实际产量加入门控；本样本的观察不推广成所有文件规则。

先对最小5224-byte .map使用input8192、output262144、blocks16。八个完整块累计262144后，第九块触发明确OutputLimit：private produced1、published0，codec consumed67/unused251；文件未消费1703字节。此为可信caller的自设资源预算生效，**不是平台/审批拒绝，也不是文件损坏**。首轮收据保持不变。随后仍使用同一工具/路径/已审核心，预先固定input8192、output1048576、blocks64；.map的15块全部通过并End，输入SHA与首轮相同。

完成首文件后才继续第二个.dat，目的是验证另一类候选的独立framing与尾部，使用同样后续固定预算；14块全部通过。两文件大小、canonical完整路径、dev/inode/mtime和完整input hash在实际读入时确认；O_RDONLY/O_NOFOLLOW/O_CLOEXEC打开、普通文件检查、exact bounded read、EOF probe和前后fstat稳定。没有写回输入。完整边界End是本次现代reader政策观察，不等于原底层close/EOF协议的全传递证明。

5个自造工具检查验证SDK SHA256与Python hashlib一致、成功输入/输出hash、坏Adler前缀不发布、未知marker尾部hash、输出预算拒绝；不使用原数据作这些fixture。真实解码字节仅在进程私有内存中用于检查和摘要，工具不输出/落盘原始或解码byte内容。仓库仅保存自写工具和hash/count/文档；详细命令/结果本地保存，没有网络/上传/push。

工具复跑命令需对应输入读取授权，输出为全新绝对目录，所有总预算必须显式给定：

```sh
python3 -B file_io/qualify_data_file.py \
  --input "$PWD/GameData/Campaigns/UNTITLED/UNTITLEDP.map" --expected-bytes 5224 \
  --max-input 8192 --max-output 1048576 --max-blocks 64 \
  --output /tmp/emperor-map-block-qualification-NEW
```

生产五文件及profile SHA在运行前核对，CommonCrypto SHA256仅用于摘要。工具hard cap为input8192/output1MiB/64blocks、单块encoded/decoded36144；不安装新软件、不尝试替代路径。协议/预算错误会输出明确统计，与进程exit0区分；实际tool/open/build拒绝则保留具体动作和stderr并停止，不规避。

剩余范围：未解析payload里的版本/对象/城市逻辑，没有证明具体原opener、liveF/root、callback/alias、seek/异常生命周期或更广格式覆盖。只认证这两个精确输入及本次固定预算；其它.map/.dat/.sav不能继承通过。字节请求API四pending保持，原runtime绑定/执行候选继续不变，ASan不可用情况不被覆盖或计通过。
