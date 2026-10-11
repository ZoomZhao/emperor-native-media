# 原文件接入前的只读资格计划

本轮授权只做已有本地原数据的元信息与有界前缀，以及自造输入的集成准备；没有读取完整真实payload、调用decoder或解析城市对象。数据位于 `/Users/zoomzhao/Workspace/emperor-native-media/GameData`，使用O_RDONLY/O_NOFOLLOW，排除隐藏项、symlink、可执行文件及代码。31个选定数据文件各读32字节，共992字节；原始prefix仅保存在本任务目录，不加入repo或上传。

**已有真实小候选匹配选定构造头，但尚无真实文件解码通过。** 22个前缀匹配文件头`aa ba dc fe`及首压缩marker`ff ef cd ab`，声明首块N在36144内、物理大小容得下该声明块、zlib两字节method/window/FCHECK形状有效。首第三word观察均为32768，仅记录opaque，不推作实际产量。没有terminal、Adler、后块、尾部或整文件验证。

| 优先级 | 本地文件（GameData相对路径） | 大小 | 首N | 首个完整声明frame到相对文件位置 |
|---|---|---:|---:|---:|
| 1 | Campaigns/UNTITLED/UNTITLEDP.map | 5224 | 719 | 735 |
| 2 | Save/zoom/C02M00_empireinfo.dat | 7147 | 842 | 858 |
| 3 | Cities/Xia.map | 79450 | 298 | 314 |
| 4 | Save/zoom/历史战役自动保存_1.sav | 165223 | 307 | 323 |

位置列是4-byte header+12-byte压缩前缀+声明N的算术候选，不是实际已消费。profile的首frame+4仍依赖完整头与已限定起点。最小的Save/ImperialRecord.imr仅284字节，头不匹配；两份48470-byte用户.dat和选定图片资源也不匹配。不匹配只说明不是这个头的候选，原raw-file passthrough是否能读这些文件仍未排除。

4ac4fc8 profile独审48/0已经核对：marker构造值可用于显式配置；不同大小4096/archive、32768/logical与window、36144/nominalcodec保持分开。匹配prefix不认证真实file entry、live root、所有payload/终止，seek请求也不等于OS成功。地图和empire-info文件的具体原consumer/opener尚未绑定；534a30的path参数与保存文件入口来源，也不能直接给某个.sav盖完整载入章。

真实文件使用之前需要：

1. 核完profile与08152cc owned独审并解决相关阻碍，再完成审核值驱动的端到端合成测试。profile48/0、owned Release/UBSan各562/0已核并绑定SHA；端到端合成Release/UBSan各334/0已完成。
2. 确认候选的具体入口，或者明确只接受通用container envelope，不解析原archive/城市对象。保持opaque，不增加产量相等门。
3. 当前32-byte前缀授权未包含完整真实body读取/解码。后续需明确授权的动作是：O_RDONLY读取选定小文件、确认identity/完整input SHA、用自写现代host-zlib组件验证framing/stream-end/Adler/count/tail；不执行任何原/衍生程序或旧codec，不消费结果为游戏状态。
4. 可信调用者先定input、output、blocks预算；未知marker、部分字段/payload或尾部不得悄然转为EOF。失败中止并保留诊断，不猜恢复位置，也不把预算不足判作原文件损坏。

可审的下一动作方案是首候选5224-byte .map：最大input8192、单块输入/输出36144、总output262144、最多16块；目标仅container/framing/codec资格。先验收完整首frame及独立codec consumed，再判断后块/尾部是否受支持；超过任一预算或未知尾部就明确停止，不能称整文件通过。**本轮不执行这个真实body方案**。若预算无法覆盖该候选，只报告所验证前缀与明确未覆盖范围。

自造集成准备复用已有独立生成器，以审核后的实际marker数值构造全合成framing；由OwnedFramedReader分别copy/move读入，覆盖stored/fixed/dynamic、raw/mixed、opaque变体、payload内部tail、字段/codec截断、输出/总量/块数预算和未知nextmarker。完整头后的零块End是现代container政策，不能把原RequiredFrame的空前缀reject预期无说明改为原EOF通过；原15pending/四项字节请求API保留。

全量只读metadata与原始prefix证据分别是 `bounded-metadata-inventory.json` / `bounded-prefix-inventory.json`，候选/计划与明确闸门在 `real-file-qualification-plan.json`。合成runner在 `synthetic-integration-preparation/`；已在两项固定独审通过并哈希绑定后执行，随后完成repo可移植版本。各构建334/0：149就绪单块case×copy/move=298，四项完整头后零块现代政策、22流政策、10外层头截断/错头检查。原RequiredFrame的两份零byte前缀不被偷换成原EOF规则。不接受真实GameData作fixture。没有扩展API、新REA、安装、UI/声音设备、网络或push。


## 可移植合成集成复跑

```sh
python3 -B file_io/run_profile_integration.py --repo "$PWD" --profile docs/original-file-outer-profile.json --gate file_io/tests/original_profile_review_gate.json --output /tmp/emperor-original-profile-NEW
python3 -B file_io/run_owned_lifetime_independent.py --output /tmp/emperor-owned-independent-NEW
```

两命令只用自造输入与固定审核过的自写核心，不消费真实GameData。预置gate校验profile和五个生产文件SHA，接受技术复核不代表真实文件/游戏授权。独立owned回归已在本任务隔离合入版本复跑各562/0，四个copy/move对象操作均按契约编译拒绝，未执行负编译probe、未运行ASan。完整结果及未满足条件见本任务 real-file-qualification-plan.json，原始真实prefix不包含在repo中。
