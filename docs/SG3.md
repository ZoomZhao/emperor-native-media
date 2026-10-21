# SG3 / 555

The [SAV19 figure consumer](SAV_FIGURES.md) binds saved image words through startup slots14/38/39/40 (`SprAmbient`/`SprMain`/`SprMain2`/`SprMain3`). Those version214 archives have no system prefix,so physical ID is low14+1,unlike the version213 surface's low14+201. Selected sprite offsets34/36 anchor figures,including mirrored payloads. Saved playback remains the default and decodes174/174 component images. The forward selector matches158/158 visible nonzero primary words (before115/158),with20/20 cited live-type branches (type1 limited to inline effect kind13) and0 remaining nonzero misses;hidden nonzero figures match6/6 separately. Cargo selection independently matches16/16 nonzero secondary words and158/158 including absent cargo;forward mode decodes174/174 components. Both selectors and their linked-figure context contain no saved image words. `0x408170`/loader`0x5CD51B..524` resolve group code through lookup physicalID−1;`0x5CFDF0` uses frame-major interleaved directions,SG+30 frame count and SG+32 direction count,clamping frame/direction before arithmetic. Direct per-type updaters can instead use literal stride8 or linear counters,as documented in SAV_FIGURES. Cargo groups use resource,cart style and amount bands from saved state. SG3 parsing/decoding is unchanged.

Zero-image load outcomes are now **28/28 resolved as invisible until an admitted update**,not28 additional placements. Postload`0x534D13→0x4D6C40(1)` uses the separate callback table`0x84E788+40*type`;both types1/76 reach`0x4D6CF0`,which clears primary/secondary words. The scheduler can run updates before first draw,so158/192 remains a saved-snapshot playback score,not measured first-frame fidelity. Kind13 is a fish/water ambient effect:selector`0x4181B1..209` uses **code1C0F,archive14/group15,`SprAmbient.sg3`,physical490,24 frames×1 direction**. Its first-tick diagnostic increments saved u8 counter,wraps at3×SGframes,and selects base+counter/3;all7/7 predictions decode without entering the scene. Pheasant action19 changes state before image selection;21/21 first-tick predictions remain explicitly blocked. Original executable hash and full call-order evidence are in SAV_FIGURES. Nonzero scene timing remains refused;this does not generalize explicit asset sequence previews to original animation.

The [N300/N500/N1000 figure extension and audio audit](SAV_FIGURES.md#n300-n500-and-n1000-with-audio-enabled-2026-10-21) extends the shared complete measured horizonN200→N1000 with explicit CRT state1:6/6 pools,203/203 outcomes,82 placed figures,90/90 decoded components,82/82 consistent primaries,53/53 building frames and21 reselections. Six original-data prey deaths and six collections are reached. Mill delivery, trader/crew removal, empty-house arrival, Watchtower return and failed-route recovery carry their cited continuations. House conversion and storage changes rerun existing building selectors and install bindings/surface cells atomically. Explicit sound enabled/disabled have identical reached simulation results; projectile allocation still consumes CRT once per successful allocation. Both inputs remain absent by default, and the App also leaves the new edit-mode input absent. Without CRT,N1000 completes3/6 pools and49/192 outcomes; refused pools install no prefix. Individual first-update208/208 components, ordered first-update207/207 components and k1 scene274/274 overlays remain covered. SG3 parsing/decoding and saved playback158/192 remain unchanged; original future-screen agreement is unmeasured.

公开来源：[SG 格式](https://github.com/bvschaik/citybuilding-tools/wiki/SG-file-format)、[MIT sgreader](https://github.com/bvschaik/citybuilding-tools/tree/ea749c6e8bfa14ccb2ce5f1565b8f0501124a053/sgreader)；许可见根目录 THIRD_PARTY_NOTICES.md。实现是现代数据解析器，不使用原版代码。

索引起点 40680 = 80 字节头 + 600 字节索引 + 200×200 字节保留 bitmap 表。213/214 的图像记录分别为64/72字节；跳过 dummy ID0，保留其后使用中的逻辑 ID，包括镜像引用。头部容量槽位与使用中的计数分别读取，STATUS 分母用后者。

公开的 [Index 字段](https://github.com/bvschaik/citybuilding-tools/wiki/SG-file-format#index) 位于80，含300个 uint16 LE 图像ID。原生解析器现保留在 `SG3Archive::imageIndex`；`sg3LogicalIndexForGroup` 只作包内数字查找，返回排除dummy后的零基数组索引，拒绝ID0、越界slot、超过使用计数及不一致数组。容量范围不代表有效ID。真实58/58包的17,400个slot中，1,783个非零引用全部有效；当前Release/UBSan各61项自造检查通过。公开文档的市场slot示例来自Caesar3；Emperor的类别含义、MAP字段到slot的关系及选包规则仍未验证，不能套用。

尺寸在20/22，类型与压缩标志合读为 uint16（50）；bitmap ID在56。此前 `originX/Y` 名称误导：公开表将34/36明确命名为**动画图像X/Y偏移**，不是静态MAP绘制origin。原生结构现改为 `animationOffsetX/Y`。本地更早证据还曾把24/26误标为origin；该字节位置错误已修正并重验，像素计数和矩阵不变。40个已匹配水面ID及4个树模板的34/36字段均为(0,0)，只保留原值，不作为静态锚点证据；24/26未在本审计中解释或用于配准。支持原始555、透明RLE、78×40 Emperor diamond、向前已有记录的负相对镜像，以及214 alpha。外部文件按 bitmap 名称定位并修正一字节偏移；禁止逃出所选目录。等距尺寸由78×40合同确定，避免58×30的120像素歧义。

78×40是单格 **base** 尺寸，并非所有单格记录的整幅画布尺寸。复用9条已匹配地面正样本，宽均为78，高46×2／49×7，均含3200字节base与额外RLE top；故严格78×40整图池遗漏9/9。当前现代解码与旧导出9/9一致，按画布底部base中心放置恢复10/10旧画面匹配；整幅画布中心放置0/10。这核验了解码几何与比较方法，不给出原版MAP静态origin。分离base/top再透明覆盖重组9/9等于整图，这9条的base/top像素无重叠且非透明alpha均255，未证多记录合成或分数alpha混合。尺寸、2×换算、窗口裁剪与正对照门禁详见 [MAP正对照](MAP.md#positive-control-gate-for-the-finite-canvas-negative)。

新增 `sg3SingleCellGeometry` 仅识别有效逻辑记录／负相对镜像源的type30、宽78、高≥40、uncompressedLength3200、tileSize0/1；返回源画布尺寸和底部base几何中心，不读取动画偏移作为静态origin。`renderSG3SingleCellCanvas` 接受调用者中心，在透明、有预算限制的视口复制整幅解码画布，使用最近邻、明确越界裁剪，不作MAP选图。原生浏览器可手动输入逻辑ID并选择单格基底对照；未证的选图和原版锚点在界面保留未知。Release/UBSan各78/78自造检查通过；实际2,983/2,983候选均生成画布及CGImage。旧正对照9/9纳入、10/10完整匹配，重新固定比较的16格仍0/16。首次原生窗口展示1/1，但屏幕5位掩码比较225/8,224像素不一致、8位最大误差1，尚未通过；不能把呈现差异报作解码失败或完整场景成功。 独审已确认实际producer buffer→CGImage provider全字节一致，差异发生在之后；一次自造显示对照的provider16,384/16,384像素精确，AppKit缓存65,536/65,536像素保留2×重复，默认／显式BGRA截图一致，四个内部色阶区域缓存→截图最大误差均1。不会修改源色或放宽阈值追求屏幕0；解码精确度和严格屏幕差异分开报告。详见[高画布限定结果](MAP.md#declared-tall-canvas-pool-and-fixed-sixteen-cell-result)。

公开多格base以行序拼接单格diamond；通用比较助手新增 `isometric_subbase_centres`，仅按宽80n−2、高≥40n推导源图中的n²个基底中心，要求调用者另证type30与n²×3200字节。它不定义MAP锚点、不选素材、不做任意像素偏移搜索。自造比较检查现46/46；真实元数据支持2..6格边长布局。本轮只解码20条数据标签land1/land2的2×2画布，四个公开子格位置对齐两处合计160次，完整画布／完整子基底均0；一个同视口无道路四邻格的新单格对照则有1个唯一完整内容匹配。详见[道路邻接对照与有界多格假设](MAP.md#same-view-clear-ground-control-and-bounded-multibase-hypothesis)；未知选图和原版放置规则保留。

本地58个索引中两份旧头值74480由公开兼容规则接受。9,059条mask均在颜色流后按RLE完整消费；把alpha字段当绝对文件偏移有7,918条不满足合同。保留该元数据，不猜测其原版内存意义。输出为预乘BGRA；F81F只用于旧原始type0色键，压缩字面保留（[公开问题9](https://github.com/bvschaik/citybuilding-tools/issues/9)）。

运行 `make sg3-test` 和 `EMPEROR_GAMEDATA_ROOT=/absolute/GameData make data-metrics`。未设置变量正常SKIP，真实资产只读；只输出汇总数字，不导出图像或dump。窗口动态列出全部SG3，按逻辑ID原生显示；失败清空旧图。

指标记录在 STATUS.md：解码/CGImage成功、缺依赖及当前校验拒绝分别计数。透明fallback不算成功。像素与原版显示逐图一致性未测；完整场景、动画、地图语义及游戏逻辑未由此证明。

Anyang 原版截图的离线核验现有**78处水面图块位置 /40个基础内容候选ID（不是已确认的原版实际选用ID）**，在同一1,478个78×40 type30候选池内，78/78均有唯一5位RGB掩码匹配。它们含**374,400个逻辑RGB项**、**1,497,600个物理RGB项**；后者包括2倍缩放的四倍重复，不是更多独立内容。此前8块对应38,400逻辑项和153,600物理项。原始8位色仍有RMSE2.814–2.899、最大误差6，未修改解码颜色合同。同水域ROI的参考位置覆盖从22.94%增至53.15%；这仍只证明源图像内容出现在截图位置，不证明MAP字段、选包、变体/动画或全城绘制。相对格子间距已测量，水域证据的14个条件候选基线及后续条件地标重放、非盲测和未证锚点范围见MAP；岸线像素一致性仍未验证。详见 [配准和比较限制](MAP.md#offline-anyang-registration-and-sg3-visual-samples)。派生像素及坐标记录只保存在本地。

零尺寸、零载荷的逻辑记录单独返回 `EmptyRecord`，包括镜像指向同类记录；保留ID，不生成透明替代图，不计解码成功。真实数据全部427条符合此精确形状。该判断在依赖解析之前进行，避免把空记录误报成缺像素文件。此前418条未分类拒绝及9条先被缺文件遮蔽的记录已归此类；非空记录71,943条中192条缺同一个companion，其余全部通过，当前未发现有效记录的解码拒绝或未支持类型。分母与原版视觉一致性边界保持不变。

## Missing ChinaMapProj resource and empty-record closure

2026-10-14对两个获准目录做只读依赖核对，目录可用性检查基线 **0/2→2/2**：用户GameData检查74,163个常规文件路径，原应用bundle在只跟随目录内符号链接的条件下检查15,471个路径。两者都没有名为 `ChinaMapProj.555` 的文件、大小写变体或同名符号链接；只存在 `ChinaMapProj.sg3`（bundle还含一份旧副本的索引）。没有在这两个目录外搜索。应用中的22个向外链接及55个坏链接未跟随；这明确限定了目录内缺失结论，不扩展为全机缺失。当前缺依赖不是已证实的大小写或相对路径解析故障，故不改查找规则、不跨根fallback、不复制或生成资源。

分别以当前GameData和原应用实际游戏根调用同一原生metrics程序，**2/2**完整汇总一致（仅耗时不同）：58/58索引、72,370逻辑记录，解码及CGImage **71,751/72,370**，空记录427、缺依赖192、无效record/payload和unsupported各0。非空分母为71,943，解码 **71,751/71,943 (99.73%)**；这是另外注明的分类分母，不替换72,370总逻辑ID分母，也不将空记录报成解码成功。新增解码进展 **0**。

独立仅索引字节审计与原生分类逐类一致：**427/427**逻辑空记录经合法负镜像解析后均为width=height=length=0，源alphaLength也均0；包含388个空源记录和39个镜像逻辑记录。它们保留ID且依赖检查之前返回 `EmptyRecord`，不是像素解码失败，不创建透明代替图。`DATA/ChinaMapProj.sg3`的201条记录单独通过原生分类复核 **2/2根**：空9、非空缺依赖192、解码0、其他拒绝0。192条都是非镜像、内部像素记录，按已支持格式依赖同名 `.555`；其原版用途或生成机制仍未测，不猜测替代像素。

当前应用的实际可选数据根是 `/Applications/皇帝龙之崛起.app/Contents/Resources/drive_c/EmperorRotMK[ZeaS]`，若用户希望改用它，必须显式传入 `--data-root`；它在此验证中也缺该文件，**不是已找到的完整替代根**，不能提升这192条覆盖。当前用户根保持 `/Users/zoomzhao/Workspace/lzjq/GameData`，所有真实验证仅对相应子测试进程设置 `EMPEROR_GAMEDATA_ROOT`，不修改全局环境、不把两份数据合并。

既有自造SG3测试Release/UBSan各 **78/78**通过，包含真实缺依赖、路径逃逸拒绝、空源及空镜像、带载荷零尺寸无效记录。metrics包装器与原生程序在未设/空根下各 **2/2 SKIP**。解码器行为无需变更；本次提交仅保存汇总和已确认缺资源阻塞，全部原文件保持只读，原版界面无操作。

原生消费者按枚举数组的实际长度检查选择索引，不保留旧固定六包上限。真实窗口逐个打开58/58个索引，总计72,370个逻辑ID；首记录57个实际显示、1个明确缺依赖。空记录及缺依赖均清空旧画面，空记录在缺像素文件的包内也不被误报成依赖失败。

## 水面时间关系与动画元数据边界

一次已批准原版实例的暂停／恢复观察已由独审复核：两组暂停重复帧完全一致，恢复再暂停后78/78固定水面位置变化，树冠对照不变，详见[原版时间观察](MAP.md#controlled-original-pauseresume-observation)。这是水面内容随运行变化的证据，不能自动等同于选用了某条SG3动画帧链。

公开 [Images 表](https://github.com/bvschaik/citybuilding-tools/wiki/SG-file-format#images) 已命名以下字段；此前“文档未定义动画字段”的概括不准确。现代MIT SGReader跳过这些动画字段，静态解码成功不等于动画语义已证。

| 记录内偏移 | 类型 | 原生保留字段 / 公开名称 |
|---|---|---|
| +30 | uint16 LE | `animationSpriteCount` / animation sprite count |
| +34 / +36 | int16 LE | `animationOffsetX/Y` / animation sprite X/Y offsets |
| +48 | uint8 | `animationReversible` / animation reversible flag |
| +58 | uint8 | `animationSpeedId` / animation speed ID |

五个值原样保留，不把标志规范化为bool、不拒绝未解释的count或speed ID，也不把动画偏移应用为静态origin。Release/UBSan分别通过**61/61**自造检查，包括213/214、LE最大count、正负偏移端点、原始byte值、dummy排除与静态解码不变。独立读取SG索引字节的真实汇总检查覆盖**58/58包、72,370/72,370逻辑记录、361,850/361,850字段**；此前完整五字段保留为0/72,370。解码/CGImage仍为71,751/72,370。未设置／空`EMPEROR_GAMEDATA_ROOT`仍成功SKIP；未导出逐记录字段或像素。

只读检查中的一个查表起点至下一更大起点区间仍只是**72条有限数值记录**；72/72均为78×40、type30、uncompressedLength3200、动画偏移(0,0)。该范围不赋予Emperor槽类别或帧链含义。共享表提供字段名，但没有闭合Emperor的帧序、count是否含base、方向分组、可反转端点重复或speed ID到实际时间的合同。特定剩余字节全零也不等于禁用动画。当前缺口应称为**有字段名、消费语义未闭合**；没有闭合原版播放规则；下文的明确ID资产预览不推断该规则。

首先冻结72长度各因数对应的**12种连续分块＋统一相位**假设及原58/20位置划分；**0/12**通过。之后另行记录三种探索性解释：前48项按奇偶交错成两个24项序列加后24项共享相位；48项与24项分组且模差为2:1；以及两组独立模差。三者在两张受控后续帧和四张此前未做内容解码的旧截图上均一致，每一比较均为58/58训练、20/20留出；四张旧图各78/78池内唯一匹配。留出集合被复用，所有截图先于新协议存在，故这是**回顾性有限重放，不是新的独立盲测**。交错解释的三类位置数为33/22/23，在已测时间状态间保持；受控后续模差为20/21，48项解释相应为40/42。三种解释未被区分，不能据此确定帧序、帧率、引擎实际ID、随机性、MAP基础选图或长期周期，更不能把候选公式接入渲染器。

另固定两种颜色重标记键：原RGB5、上述数值类别加原RGB5，目标为后续RGB5。均有训练反例。较细的类别＋颜色键，在再暂停帧有**381/513**训练键歧义，任何单值映射至少错**66,433/92,800**个训练逻辑像素；32,000个留出像素中134个来自训练单值键，其中47个不一致，31,852个键已歧义，14个未见，后两类不算通过。源图训练原始RGB和RGB5各300个键，全体各308个，观察集合中的键划分相同，因此反例并非仅由5位量化合并造成。这只排除所测“观察到的颜色／类别决定新颜色”单值映射；**隐藏调色索引、分层或位置相关着色等机制仍未排除**，也没有证明必须由换SG记录实现。设置及捕获颜色配置未变，不代表水面专属调色机制未变。

上述数值水面关系分析离线使用已有截图和只读SG3数据；后续原版四格足迹观察单独记录在MAP，未新增启动、安装或上传。字段保留验证只读索引并输出汇总。72条范围、详细内容ID、协议、候选及颜色记录留在本地；Git只保存汇总和限制。原版实例按授权继续保持暂停；MAP→SG3关联仍为0/167。


## 冻结失败格的有限 base 内容识别

本次从固定3×3中先按几何检查五个失败点：**0/5** 的完整base不受当前候选RLE覆盖，故不从其中裁掉坏像素。改用此前已经冻结、原截图中可见无遮挡的(65,33)，原物理中心(1118,336)；同视口阳性(66,43)，中心(398,776)，既有完整内容ID361。目标的完整public78×40 diamond有 **1,600逻辑/6,400物理像素**，当前候选RLE覆盖0且每像素只有一个base paint。这个当前候选支持审计不证明原版没有额外层。位置继承已测的条件性道路格对应，不再次拟合坐标。

固定窗口、ROI、UI排除和RGB5精确标准均沿用；两点所有6,400物理base像素保留原始最近邻2×重复，四个物理采样均检查。Hash仅作加速筛选，命中后逐个RGB5三元组精确复核，不允许hash碰撞成为正例，也不设最少差异接受阈值。

| 冻结域 | 记录/来源范围 | 完整base变体数 | 目标命中 | 最少不同逻辑像素（诊断） |
|---|---|---:|---:|---:|
| 既有单格缓存 | 2,983记录，11个有候选档案；原58档案枚举域 | 5,966 | 0 | 501/1,600 |
| 声明的多格base子片 | 1,131记录，29个档案 | 26,612 | 0 | 1,578/1,600 |
| 自然78×40常规RLE、精确diamond alpha | 181元数据/解码记录中7个合法完整base，2个档案 | 7 | 0 | 1,600/1,600 |

旧2,983缓存全部复用，新增解码 **0**。两个变体是：完整现代解码记录的底部40行合成base；只保留声明uncompressedLength3200流的raw base。它们共享同一1,600掩码，不将忽略的RLE全图称作完整sprite成功。阳性仅匹配 `China_Terrain.sg3` ID361的这两个base变体，**1个逻辑记录/2变体**；这只是该有限域内的base内容，不是整个原资源唯一性。

多格域仅按有效源type30、宽80n−2、高≥40n、uncompressedLength3200n²、tileSize0/n及合法后向镜像定义，n2..6；不按文件名/颜色/ID挑选。记录数分别n2:410、n3:22、n4:676、n5:16、n6:7。公开isometric行序给出 **13,306个**固定源子基底位置，每个比较raw与完整记录合成两种版本；**26,612/26,612**变体完整不透明。旧20个2×2整图缓存复用，新增完整记录解码1,111、raw版本解码1,131，失败 **0/2,242**。新域在旧11个候选档案之外增加18个；大画布在原版中的整体锚点并未由子片比较证明。

另在58档案/72,370逻辑记录的元数据中，仅自然宽78、高40的已支持常规类型产生type256:180、type257:1，共 **181/181解码**，失败0。先冻结与参考颜色无关的合法几何条件：alpha必须在固定diamond的全部1,600像素为255，外部为0。7条满足；174条不满足，其中172条含部分alpha。这172条是完整不透明base域外的组合未知量，不是已经否定的透明层。没有将它们的未覆盖像素删除以宣称匹配。

合计 **32,585/32,585**合法完整base变体在冻结目标处精确核验，目标记录 **0匹配**；三阶段阴性与各候选数均保留。阳性门禁通过，未识别目标实际图，故没有把最接近记录当实际来源，也未回推MAP字段/bitmap关系或制造选图规则。边界包括：未测编码/画布几何、原版多记录合成、源alpha消费、色彩消费及未闭合的原版锚点语义；本结果不证明目标在全部原资源中不存在。完整城市关联仍 **0/167**。

可复跑 `scripts/base_content_audit.py --manifest /absolute/local/replay-manifest.json`：真实模式只由 `EMPEROR_GAMEDATA_ROOT`配置源资产根，显式私有manifest提供已验证的原生解码缓存、冻结参考和点位。根的缓存出处必须相同；它重用像素缓存，不重新生成资产、不输出像素/hash dump。单格、固定多格masked、常规alpha三种cache schema均保留数量和匹配元数据；原生导出记录/解码失败计数独立存于本地协议。未设/空根在无site-packages时也 **2/2 SKIP**。`make base-content-audit-test` **11/11** 自造检查覆盖完整mask、重复别名、单像素错误、hash碰撞复核、alpha未知、四物理样本、透明角、几何缓存、全框坐标/禁止负索引绕回、出处和SKIP；这些不计原数据进展。

下一项最直接的数据支持是一个另行冻结的透明层假设：已存在172条自然78×40部分alpha记录，完整base测试尚未覆盖其与固定MAP候选底图的组合。若测试，保持该底图、源alpha和位置不变，限定标准premultiplied source-over，并保留阴性；即便精确一致也只形成可验证组合来源假设，不证明原版使用该合同。当前不继续扩大完整base域，不改变阈值、alpha、位置或游戏逻辑。原版同实例保持打开，本次无原版输入/新截图；所有cache、源像素、局部协议与结果留在Git外。


## 固定底图的单层原始 alpha 对照

随后只核验上一节未纳入完整不透明base的172条部分alpha图。**172/172**来自version214 `DATA/destruction.sg3`，有效type256、bitmap索引0，其原始名称为 `dust_clouds_02`；都是内部像素源、自然画布78×40、独立alpha流非空。非零alpha支持33..1600像素，全部位于固定public diamond内，外部非零alpha0；13条覆盖完整diamond，其他为子集。非零alpha原值范围8..255。它们全部记录动画offset(41,28)，该字段不作为静态origin；16条animation count11、156条0，不推断帧链或时间规则。bitmap名称和尺寸不证明地面用途。

先冻结资格 **172/172几何相容**：将自然画布在既有footprint源中心(39,20)对齐，不挪动、缩放或旋转，原始alpha/颜色保留。这个锚点是明确的诊断约定，已证原版地面用途/实际锚点 **0**。只在目标(65,33)的固定MAP预测ID273上逐一覆一层，不构造底图笛卡尔积；同屏阳性(66,43)固定ID361。第二个既有失败格(69,36)只预留给发现命中的候选做留出验证。

使用现代解码器的premultiplied BGRA，固定source-over为每通道 `src + (dst*(255-alpha)+127)//255`，不重新拟合透明度、颜色、位置或容差。全1,600逻辑/6,400物理base与既有RGB5精确/四物理样本标准不变。**0/172**目标单层组合匹配，最少不同 **1599/1600** 逻辑像素，仅作诊断。阳性的零alpha对照 **1600/1600** 保留；所有172条非零组合也没有完整匹配，最少31/1600不同，不因为近似而接受。目标无发现命中，留出点 **0/1未评估**，不报留出失败。

此结果收束这个单层条件假设，未识别目标真实来源，也没有证明原版使用该blend合同。它不否定别的源记录、别的有证锚点、多层合成或原版特殊色彩/alpha消费。未把dust bitmap当已证地面层，没有继续参数/组合扩搜，渲染器与游戏逻辑未改；完整城市关联仍 **0/167**。

并行的输入来源核对确认当前原版PID12997的正常工作目录与对应资产路径。既有可重用证据只覆盖Anyang MAP；没有把旧VM复制核验当作当前Applications实例证明。仅比较当前实验实际涉及的 `China_Terrain.sg3/.555`、`destruction.sg3/.555`：**4/4对应文件SHA256相同、4/4尺寸/mtime稳定**。完整hash和真实路径保留本地，不入Git。这排除这些对应输入的磁盘字节版本差异；不是运行时文件句柄追踪，也不扩展为全部原资源一致结论。

`base_content_audit.py`的显式 `mode: single-layer` 私有manifest可复跑该对照：重新只读验证冻结MAP u32预测，读取已有raw base和部分alpha缓存，每次只有一张固定base，保留完整几何与数量。`source_over_fixed_base`拒绝非premultiplied源、非不透明base和底图笛卡尔积；采用uint32运算避免乘法溢出，保留输入数组。五条新回归覆盖零/full alpha、整数half-alpha舍入、全部alpha级别/输入不可变、非法输入和无底图组合，合计 **16/16**，未设/空根依旧 **2/2无site-packages SKIP**；可复跑真实结果与本地初算一致。合成fixture不计原数据进展。

下一项需要直接区分“冻结地面是否稳定的多源结果”和“未覆盖的资源几何/消费”：优先用同一原版实例对固定地面与阳性做最小稳定性观察，或取得来源明确的图层/锚点格式证据，而不是再把不明用途记录任意叠加。所有本次像素/协议/结果仍仅本地；无原版新输入、截图、保存或重启。


### 私有像素缓存的范围门禁

独审发现single、regular与single-layer三处私有缓存切片缺少负offset下界：自造三幅40×78 BGRA图的offset−24960可绕回第二幅，却保留第一幅的ID标签。此P2已解除，**3/3受影响读取路径**在切片前拒绝负offset；统一验证 `offset>=0`、`size>=0`、`offset<=len(blob)`、`size<=len(blob)-offset` 与实际读取长度。BGRA存储、offset和record长度也必须保持完整4字节像素；raw-single与masked-multibase先核对完整声明长度再映射。即使层记录不参与alpha候选，也先验证其记录范围，避免过滤隐藏坏缓存。

新回归用三幅不同颜色的自造图证明负offset不能以别的ID标签取尾部图，并覆盖合法非零offset、记录末尾截断、越界、像素内偏移、存储尾部残字节、raw/multibase完整范围与未选层记录。合计 **23/23**，新回归在修复前版本会失败；未设/空根仍 **2/2 SKIP**。两份既有真实缓存协议只在子测试进程注入核验根，修复前后完整JSON **2/2一致**：有限base目标0/32,585、alpha目标0/172与阳性保持。这是缓存证据完整性修复，不增加原版匹配，不改变源像素、色彩、几何或合成规则。


## Retained cross-installation input manifest

The four-file identity record above belongs to the fixed-alpha experiment only. An earlier independent review of d5f7915 had already reported equality of58 SG3 indexes and78 existing555 files across the two installations. That result survived in tool receipts, **not a saved cross-root hash manifest**; the review's `snapshot-manifest.json` contains source/document hashes and must not be cited as an asset manifest. This distinction corrects the later assertion that only4 files had ever been compared.

A single subsequent read-only pass now retains a traceable manifest for the exact declared SG inputs: **58/58 indexes＋75/75 existing referenced555 files =133/133 paired files**, matching relative path, size and SHA256. Nonempty logical records are resolved through legal backward mirrors and internal/external companion names, with the modern reader's same-directory/`555` lookup and root confinement; empty records do not create pixel references. Both roots have the same reference graph. Both lack `DATA/ChinaMapProj.555` for192 logical records, as already documented. The3-file difference from the earlier78 count reflects this pass's **actually referenced existing-file scope**, not three new missing resources; those other files are not rehashed here.

The manifest covers **351,638,509 bytes per installation**. Relative to the alpha record's4 files/20,511,535 bytes, its retained scope adds129 file entries/331,126,974 bytes per installation; these are **not historically new comparisons**. It includes all30 archives/30 existing pixel dependencies used by the prior finite-base pool, with0 missing dependencies in that subset. Source-file metadata stayed stable during the reads; this is a per-file audit, not a simultaneous filesystem snapshot or runtime handle trace. The corresponding on-disk resource-version branch is closed; image selection, generated/composed sources and consumer semantics remain unproved. No additional full decoding, pixel export, download or resource copying is performed.

Aggregate manifest invariants pass **8/8**; unset/empty `EMPEROR_GAMEDATA_ROOT` each SKIP successfully (**2/2**, including without site-packages). Original reads use an explicit root only in each child process. The full per-file path/size/hash and reference manifests stay in the local `cross-install-input-manifest` evidence directory, outside Git. The repository records **1/1 retained manifest** plus these aggregate bounds; it does not commit original assets or hash dumps, claim new decoded images, or extend the4-file experiment's scope retroactively.

## PNG input calibration

Raw SG3555 and PNG channel values require an explicit comparison representation. The existing exact348 control contains a4,064-byte Display ICC plus cICP[12,13,0,1];three new desktop screenshots instead carry sRGB intent0. Untagged Library copies preserve9,781,248/9,781,248 RGBA pixels across3/3 originals,so their representation is linked through exact pixels;missing profiles otherwise remain unknown. Pillow/OpenCV agree22,822,912/22,822,912 pixels across seven files. Four distinct frames each retain568,504/568,504 raw2× world blocks;no interpolation is observed there.

The [bounded input calibration](MAP.md#exact-template-discovery-and-png-input-calibration) freezes new-sRGB→old-embedded-Display,perceptual intent0,flags0,LittleCMS2.17/Pillow11.3.0. Decode RGB8,apply that standard profile transform,then shift right3 for RGB5;no resampling or score-selected matrix/intent/tolerance is used. This tests representation compatibility,not an intrinsic SG3 ICC space or the engine's colour rule. Known-positive Display→sRGB→Display roundtripping changes source errors0/7,988→660/7,988,showing strict RGB5 is not preserved automatically. The unchanged2,983×6,025-position domain has0 exact source/base hits both before/after conversion;minimum base errors change6,312/6,400→1,256/6,400,but are not matches. Complete error distributions and limitations remain in MAP. Old failures stay unchanged;all ICC/pixel derivatives stay private,and cross-profile raw equality remains unproved.

### Forward source representation

Ten old exact controls support comparison RGB8=`RGB555 q << 3` at82,556/82,556 opaque pixels;native `(q << 3) | (q >> 2)` agrees0/82,556 raw pixels but retains their RGB5 values. This is an observed capture/source representation model,not a parser correction or an intrinsic ICC space for SG3. The native decoder stays unchanged.

[PNG3](https://www.w3.org/TR/png-3/#cICP-chunk) specifies Display P3 for cICP[12,13,0,1] and gives understood cICP precedence over ICC. The embedded and standard profiles have identical colourants but their fixed forward outputs differ on7/32,768 zero-filled RGB555 triplets after RGB5 quantization,max raw channel difference1. The declared primary path therefore follows cICP,not a score-selected profile:zero-fill RGB8→standard Display P3→sRGB,intent0,flags0,then2× and RGB5. Same-path old source/reference predictions agree82,556/82,556 RGB8 pixels and recover348;this proves implementation consistency only.

In the same bounded2,983×6,025 new-image domain,complete source/base hits remain0/17,972,575. At the declared794 position,changed-base agreement is4,524/4,544 with20 one-step errors;all1,856 unchanged-painted pixels still disagree despite remaining identical between frames. Whole source/overlay agree4,580/6,456 and9,092/12,480. The [disjoint old/new partitions](MAP.md#forward-source-representation-and-unchanged-pixel-partition) retain these errors,nonpainted changes and coverage limits. Prior raw/inverse negatives are representation-confounded,not global794 exclusions. Coverage/order,capture precision and absolute saved-cell anchoring remain open;no tolerance or layer fitting follows.

### ColorSync RGB8 implementation and complete fixed-source score

The20 LittleCMS changed-base residuals share one RGB555 input colour and one observed output;that colour is outside the seven ICC/cICP RGB5 differences. A fixed installed public [ColorSync API](https://developer.apple.com/documentation/colorsync/colorsynctransformconvert%28_%3A_%3A_%3A_%3A_%3A_%3A_%3A_%3A_%3A_%3A_%3A_%3A%29) probe follows the official SDK `ColorSyncProfile.h`/`ColorSyncTransform.h` contract. Both implementations receive identical serialized Display P3/standard-sRGB ICC bytes. `ColorSyncTransformCreate` uses perceptual intent0 on DeviceToPCS/PCSToDevice,BPC false and documented default Best quality,with no preferred-CMM override. `ColorSyncTransformConvert` uses RGB8 integer components,no alpha,default byte order,width×3 row stride and no conversion options. No application rounding,intent/profile choice or tolerance follows the result.

On macOS26.5.2/build25F84,ColorSync matches1/1 unique residual colour and20/20 repetitions,including a20-input batch;LittleCMS predicts blue one raw step higher. Controls pass7,988/7,988 RGB8 pixels for348 source/reference and full-batch/palette agreement,and841/841 for each P3/sRGB identity. The input set contains840 old-control colours plus the target. On the840 controls,the two implementations agree726/840 RGB8 and823/840 RGB5 outputs,max raw difference1. The serialized/reopened LittleCMS reference also differs from the retained prior in-memory-profile result at40/82,556 old-control pixels,all one-step RGB5 differences;the target's high-blue result is unchanged. These differences are retained. The point result supports an implementation hypothesis,not20 independent colours or proof of the real capture chain.

A follow-up converts **every794 source pixel**,with source RGB8=`RGB555<<3` and the same frozen ColorSync contract,without a single-colour patch. Old P3 reference crops receive that same forward conversion;new sRGB channels remain as captured. Original raw change masks,alpha,declared positions and2× placement stay fixed. Old348 still matches7,988/7,988 opaque and6,400/6,400 public pixels in RGB8. The [complete fixed-source comparison](MAP.md#colorsync-complete-fixed-source-comparison) gives:

| New sample predicate | RGB5 equal / all physical pixels | RGB8 equal / all physical pixels |
|---|---:|---:|
| Changed public base | 4,532/4,544 | 4,240/4,544 |
| Unchanged public base | 0/1,856 | 0/1,856 |
| Complete opaque source | 4,588/6,456 | 4,292/6,456 |
| Whole source-over-before canvas | 9,100/12,480 | 8,800/12,480 |

All20 prior changed-region errors disappear,while12 new RGB5 errors appear at three input colours,each with maximum raw channel error1;none is dropped. Thus4,524/4,544 was not simply promoted to4,544/4,544. A same-byte-profile LittleCMS whole-source control retains the prior RGB5 scores,so its preserved baseline is explicit. Whole ColorSync prediction still fails:3,380 errors=1,856 unchanged-painted+12 changed-painted+1,512 nonpainted. Other840-colour controls and every raw/inverse/LittleCMS result remain preserved. No candidate or position search is rerun for ColorSync,and no other parameters are tried. Captured-channel precision/model,coverage/order and absolute saved-cell anchoring remain unproved;complete scenes stay0/167. Profiles,pixels,API helper and all per-pixel differences remain private.


### Complete base/top decomposition in the separated ground discovery

The [bounded neighbour replay](MAP.md#separated-neighbour-identification-and-complete-top-coverage) directly reconstructs all585 declared nonmirrored type30 `China_land1.bmp`/`China_land2.bmp` single-cell canvases from their3,200-byte public base plus the remaining public RLE stream. Complete native base/full byte checks pass585/585 each;228/585 tops are nonempty,0/585 overlap their own public base. This content-defined split does not use animation offsets or add a source family. Frozen RGB555<<3/ColorSync conversion preserves841/841 colour controls. `scripts/colorsync_rgb8.cpp` retains the authored public API helper so the read-only manifest replay can regenerate that representation;profiles and derived pixels stay private.

Whole natural tops240/242 retain132/1,980 and148/1,888 RGB5 conflicts. Their partial coverage and the1,755-placement opacity upper bound are recorded in MAP;neither proves the original drawing order,an actual selector or a complete scene. Static RLE decomposition supplies no animation playback semantics. No runtime parser/renderer or CitySimulation/EMP1 changes follow.

The subsequent [centre-removal check](MAP.md#centre-old-top-removal-necessary-check) retains241 and325 as exact aliases on1,748 exposed pre-state base pixels. Their complete source canvases are not interchangeable:241 has a2,116-physical-pixel RLE top,325 has none. The former's original alpha reaches all1,512 non794paint and496 outside-canvas changes,but keeps52/2,116 full pre-state top conflicts. This is necessary geometric compatibility,not a post-selected source identity or permission to erase unknown background. No alpha/offset/colour edits or animation inference occur;the reveal source remains unknown.

A subsequent [rear-material check](MAP.md#rear-material-identification-behind-the-centre) uses the same585 records at three fixed positions,excluding complete centre241/325 alpha and all other possible neighbour support. The exposed2,844/2,688/1,824 physical pixels yield0/585 exact candidates each. Diagnostic244/246/240 minima retain16/20/12 RGB5 errors,all maximum raw channel difference1;none becomes an identified source. Original alpha,colour conversion and anchors stay unchanged. No independent reveal material or new conditional redraw is supplied,while all prior source/top conflicts remain recorded.

The [four conditional replacement models](MAP.md#four-conditional-old-top-replacement-predictions) allow those preselected near sources as explicit unconfirmed hypotheses,without changing `exact=false` or their48 identification errors. Three complete rear records compose over unknown;the retained pre-state has a hole defined only by the complete original241/325 top alpha. Complete794 and natural front tops follow one predeclared depth order;front246 is a separate unconfirmed branch. Rear source alpha covers2,116/2,116 old241-top pixels,so the resulting full ROI is predictable32,000/32,000 with0 unknown. The implementation preserves full denominators if future holes remain;it never copies post-state colour or alters source alpha. Full rear-record and optional-front conflicts remain in MAP alongside all previous metrics. Every one of the four complete predictions still fails;this is no SG3 selector,engine draw-order or independent-scene proof.


### Native diagnostic component composition and provenance

The [pure-source local delivery](MAP.md#pixel-provenance-and-pure-sg3-native-local-delivery) uses the existing native decoder,`renderSG3SingleCellCanvas` and shared CGImage conversion. The generic diagnostic scene API accepts only caller-declared logical IDs,full/natural-top components,centres and order. Top extraction validates the complete original record,then reuses the public native RLE reader at `offset+uncompressedLength`;it does not subtract base alpha or infer visibility. An empty top remains transparent only after original-source validation. Internal,nonmirrored,binary-alpha type30 single cells are the explicit supported scope;other cases refuse without substitution. Animation offsets and playback rules are not used.

Every output pixel has a final-producer label (0 unknown,1..N manifest layer),and all pixels start transparent. There is no screenshot/background input. The private seven-layer scene creates1/1 CGImage/PNG and covers23,968/32,000 ROI pixels;the other8,032 are transparent. Under the unchanged comparison-only RGB555<<3/ColorSync hypothesis,23,436 match and532 conflict in that full ROI;context coverage is31,156/86,400. Actual native RGBA and producer labels each match86,400/86,400 independent source-composition samples. Native raw RGB8 retains the decoder/shared DeviceRGB contract;fixed scoring conversion is a separate copy,not an engine colour correction. Candidate IDs/order remain unconfirmed and full scenes stay0/167.

The earlier edit prediction's provenance is separately10,816/32,000 SG3 plus21,184/32,000 retained pre-state pixels;its36 errors are no full native reconstruction metric. Complete target denominators,larger pure-source conflicts and all historical controls remain in MAP. Private scene metadata,PNG,producer map and all pixel derivatives stay outside Git. No new original assets are decoded into the corpus metric,and no production App/parser/CitySimulation/EMP1 behavior changes. Native scene/reference checks pass67/67 per Release/UBSan,existing SG3 checks78/78 per mode and shared CGImage checks4,138/4,138.

The [output identity preflight](MAP.md#output-identity-preflight-and-alias-protection) repairs the native scene PNG/producer alias P2 before either file is created. Parent canonicalization,existing-inode equivalence and whole-set freshness validation preserve old outputs;the renderer and original SG3 components are unchanged. Native RGBA and source labels each remain86,400/86,400 identical,and the existing Library PNG requires no replacement.


### Peripheral full-source coverage without new renderer logic

The [peripheral expansion](MAP.md#peripheral-base-coverage-with-pre-state-source-hypotheses) uses only the existing585-record domain and native diagnostic scene entry. Public base geometry gives unique owners to all8,032 baseline unknown pixels:three already top-only positions contribute7,688 when upgraded to complete sources,and six new peripheral owners account for344. This is a nine-position minimum for public-base completion,not an arbitrary top-combination minimum. Six fixed pre-state exposures each contain1,748 pixels outside the target;all3,510 candidate rows are retained. Two new near sources stay `exact=false`,the left241/325 pair remains ambiguous,243 is a separately frozen exploratory case,and two unsupported cells receive no source.

Complete natural sources and a fixed depth order increase actual SG3-generated ROI coverage23,968→31,856/32,000 under both retained243/left-alias variants. RGB5 partitions are31,588 equal＋268 conflict＋144 unknown or31,404＋452＋144;the144 unknown pixels exactly occupy the two unsupported public-base owners. All old and new whole-component conflicts remain recorded. Every604,800/604,800 native RGBA pixels and producer labels agrees with independent source composition across seven models. No parser,renderer,colour/alpha contract or production selection changes;all private scene manifests/images stay outside Git,and complete cities remain0/167.


## Explicit local asset-sequence preview

The public SG table names animation count, signed X/Y offsets, reversible flag and speed ID; it does not establish a universal Emperor frame chain. The modern MIT [SGReader ID loop](https://github.com/bvschaik/citybuilding-tools/blob/ea749c6e8bfa14ccb2ce5f1565b8f0501124a053/sgreader/sgfile.cpp#L181-L198) consumes one dummy record, then physical IDs1..count; native vector index is physicalID-1. A negative mirror reference flips an image, not playback time. No original executable, disassembly or original-game operation is used here.

Six groups were selected before visual inspection, at most three building candidates and three walker candidates. Local annotated sheets retain every requested physical ID, bitmap name, dimensions, animation count/offsets, mirror, reversible byte and speed ID, including explicit failure slots. **377/377** exported metadata rows (376 unique records because one boundary repeats) decode;**3,393/3,393** metadata fields agree with an independent raw-index read. Bitmap names alone do not identify the content: the building candidates actually show floating debris and two platform/workbench figures, so verified separate building bases remain**0**. Those three ambient sequences show continuous local motion, with no separately identified static base. Each first record is a visible animated frame rather than a proven bottom layer. Their explicit sequences exclude the visibly different group/direction at the inspected boundary.

The three walker sheets show interleaved directions, disproving adjacent-base-plus-frame playback for these samples. A second sheet per walker inspects every frame of one fixed direction and the first frame of the other directions. Only those three local fixed-direction lists are accepted; the other direction timelines are not generalized from their first images. All exact lists, contact sheets and per-record metadata remain private outside Git. No universal stride, count-includes-base rule, semantic bitmap role or speed mapping is encoded in production source.

The App now accepts**1..256 explicitly supplied physical IDs** in the current package, retaining caller order and repetitions. It decodes the entire list before publishing; an invalid, empty or failed frame clears the result without compacting IDs. All frames use one fixed transparent union canvas, either raw top-left alignment or the explicit preview convention `(x,y)=(-rawOffsetX,-rawOffsetY)`. Signed offsets, mirror, count, reverse and speed ID remain visible. This preserves original decoded pixels but does not prove an original world anchor, extra base-layer composition or shadow-color interpretation. In particular, red shadow-marker pixels in the current decoder are retained rather than guessed into an original shading rule.

Native controls provide previous/next, pause/play and a user-selected**1..30 FPS** presentation rate. Endpoint wrap is explicitly a preview loop; neither the reversible byte nor speed ID determines order/time. Package/static-image changes clear playback; tab changes and normal termination stop the timer. Preview caches are bounded to32MiB source frames plus32MiB normalized frames,256 frames and2048-square canvas extents. SG3/555 decode input budgets remain the existing decoder's bounds. No sample ID or original asset is embedded in the App, and no CitySimulation/EMP1 or city rendering path changes.

Local original-data preview advances**0/6 to6/6** supported explicit lists:3 walker and3 ambient,**122/122** decoded frames, source-pixel preservation checks and native CGImages. The common canvas changes at**116/116** adjacent pairs. The two archive/companion pairs remain byte-identical**4/4** across the metric run; Release/UBSan reports agree. Native authored tests pass**42/42** per mode, covering strict IDs, physical/vector indexing, explicit order/repetitions, signed placement, complete pixel canvases, mirror versus temporal order, empty/missing/failed-frame rejection without partial publication, cache/extent budgets, stepping and rate limits. Fixtures are authored and add no original-data coverage. Wrapper/native unset/empty checks successfully SKIP**8/8**, before manifest or asset access.

New App checks pass**36/36**: first/last/next transitions for all six lists, timed advancement/stable pause, raw/offset canvas selection, invalid/maximum preview rate, invalid lists, missing companion, archive clearing, tab pause and static-selection clearing. All**344/344** tested input files remain unchanged; the test App exits normally. Packaging retains83 source fingerprints and28 compiled units with strict ad-hoc signing and no original assets. These are asset-preview checks, not original timing, all-direction animation, complete buildings or dynamic city/gameplay coverage.

Run `python3 -B scripts/sg3_sequence.py test [--mode undefined]`. For real samples use only a local manifest: `EMPEROR_GAMEDATA_ROOT=/absolute/owned/root python3 -B scripts/sg3_sequence.py metrics --manifest /absolute/local/sequences.json`; its shape is `{"sequences":[{"archive":"DATA/YourArchive.sg3","physical_ids":[1,3,2]}]}` (1..6 explicit lists). The IDs in that schema example are arbitrary syntax, not a real animation rule. The new versioned App path is in README; all earlier deliveries are preserved.

Independent review of5817335/aaa5512 reports no P1/P2. The retained authored-only `review_tests/` suite is byte-identical to the independently executed test sources:Release/UBSan each validate960 complete scalar-oracle canvases (2,958 assertions) and33 checks using10 unchanged App callbacks with a real Foundation NSTimer loop and fake controls. This does not replace real CGImage or GUI evidence. The same suite independently closes SAV big-endian serialization48/48 and six positive controls;the old failing version was retained as the regression contrast. Re-run with `python3 -B review_tests/run_review.py --source-root /absolute/checkout --out-dir /absolute/private/output`;the bounded runner removes the real-data root from child environments. These passed flows are reused here because the production code and extracted callbacks are unchanged,not rerun for another count. ASan also fails to initialize for an empty program in the review environment;no ASan/leak pass is claimed. The current integrated package receives its own original-source GUI sweep and media/error/termination checks,with the recommended launch path in README.

The [explicit manual building candidate bridge](SAV.md#explicit-manual-sg3-candidate-preview) consumes an already-decoded static SG3 image selected inthe existing browser. Its physical ID remains1-based (vector index0 corresponds toID1),withno compaction ofempty/failed entries. It does not use animation offsets asa static city origin orassign bitmap names tosemantic building types. Manual preview/configuration acceptance andconfirmed original object-image binding are separate;confirmed binding remains0.

A bounded local visual review advances **0/8→8/8 static base canvases** in the explicitly named `China_Safety.bmp` category of `DATA/China_General.sg3` (physical IDs1538..1727;8 type30 bases among190 records). Existing local image directories were checked first;they contained no Safety-base contact sheet,so the existing decoder exported this category only. Actual pixels select **ID1559,158×132** as the preferred well-like candidate:round open stone basin/well mouth,bucket-like vessels andbamboo/wood surroundings. **ID1538,158×93** is a weaker water-facility alternative witha dragon fountain/statue andround pool;the other6 bases are roofed buildings/towers. This is visual triage,not a proven semantic selector oranimation interpretation. Both candidates are manually placed over the real saved Well record (slot39,type72,coordinate29,38,2×2) in the existing App,andboth metadata-only configurations reload successfully **2/2**. A same-record `China_Terrain.sg3` ID466 rock remains the negative control. The three explicit cases render3/3 CGImages/configuration roundtrips;confirmed bindings remain0 andthe original game is not operated. Candidate configurations,raw exports,contact sheets andApp screenshots remain private/local. Before any original comparison,freeze this source/record/image choice;check object identity first,thenfootprint registration,anchor,shadow/occlusion andanimation separately.

## Exe building animation consumer

The [SAV19 building animation trace](SAV.md#sav19-building-animation-overlays) closes a bounded Emperor consumer for all11 animation-bearing types among the27 currently bound building types. It does not generalize the earlier ambient/walker preview lists. Owned exe SHA-256 `0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`; cited file offsets=VA−0x400000.

`0x470A34` tests base SG+30;`0x418C79..0x418D37` selects positive indices1..count, and`0x429903` draws **base physical ID+frame**. Count excludes the static base. Current bases are unmirrored,nonreversible,speed4/5; full chains across enabled occurrences resolve/decode **954/954** frames (**197 unique**),with0 failures. Clay,Mill andMusic chains use supported raw type0,other chains type256; bitmap family,dimensions andabsence ofmirror/tile declarations are validated independently.

For view0 ordinary virtual+120=`0x4298C0`,placement consumes the **base** record's signed34/36 offsets andheight,not the frame's offsets: **(scanX+base.offsetX,scanY+base.offsetY−base.height+20×(side+1))**. Horizontal half-width temporaries cancel at`0x4299D7..0x429BFD`; vertical side branches are`0x429C11..0x429D70`,viewport20/40 geometry`0x469820`. The existing projected cell centre differs fromthe scan origin by(39,20). These fields remain animation-only; static MAP origins andthe explicit asset preview convention are unchanged.

The saved per-cell byte supplies low7-bit frame andhigh-bit reverse direction;SAV19 persists it atterrain+363,888. `0x418AA0` computes51 speed flags from`timeGetTime()` (`0x536B4D..53`),interval20×speedID ms;eligible draws advance one step without catch-up. Reverse playback repeats both endpoints. Native caller tick defaults0 andpreserves the SAV phase;nonzero ticks use an explicit regular-draw preview timeline withfresh clock origin,not a persisted game tick. Full gating,serialization,time bounds andthe11-type frame/offset table are in SAV. Walkers,alternate views,height occlusion,shadow fidelity andoriginal-screen agreement remain unresolved.

## Exe archive selection and Well record numbering

The later [Well static-base trace](SAV.md#exe-cited-well-static-base-image-binding) closes **1/1 semantic type and2/2 supplied saved Well occurrences** by executable evidence. It selects General slot130/code0x482 or slot131/code0x483; those headers name physical records1559 and1538 respectively. No additional pixel search or original-game operation is used. Both branches pass the independent metadata contract and native decode **2/2**; saved records choose1559 **2/2**, and the existing explicit scene placement yields2/2 native CGImages. Original-screen image, anchor, shadow/occlusion and animation fidelity are separate and unverified.

All VAs below refer to owned `Emperor.exe` SHA-256 **`0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`**. For these initialized sections, file offset is **VA−0x400000**. The tileset→filename map is **built as a41-pointer stack array**, rather than read from a contiguous static pointer table. **FUN_00475B60 @0x475B60** fills it at **0x475C3B..0x475D5A**, then passes it to **FUN_005CCDF0 at0x475D65**. Slot2 is written at **0x475C4B** with string **VA0x82B18C/file0x42B18C=`China_General`**. **FUN_005CCDF0** selects `array[currentTileset]`; **FUN_005CCF70** supplies that index and index<<9 to **FUN_005CD1F0**. The loader appends the `.sg3` extension; these are filename stems.

| Tileset | Initialized filename stem | Pointer-store VA / string VA |
|---|---|---|
|0|China_Unloaded|0x475C3B /0x82B1AC|
|1|China_Interface|0x475C43 /0x82B19C|
|2|China_General|0x475C4B /0x82B18C|
|3|China_Terrain|0x475C53 /0x82B17C|
|4|China_Mon_Temple|0x475C5B /0x82B168|
|5|China_Mon_Tumulus|0x475C63 /0x82B154|
|6|China_Mon_Grand_Canal|0x475C6B /0x82B13C|
|7|China_Mon_Splendid_Temple|0x475C73 /0x82B120|
|8|China_Mon_Earthen_Greatwall_1|0x475C7B /0x82B100|
|9|China_Mon_Earthen_Greatwall_2|0x475C83 /0x82B0E0|
|10|China_Mon_Earthen_Greatwall_3|0x475C8B /0x82B0C0|
|11..12|empty|0x475C93,0x475C97 /0x88EC4C|
|13|China_MapOfChina01|0x475C9B /0x82B0AC|
|14|SprAmbient|0x475CA3 /0x82B0A0|
|15|China_Fonts|0x475CAB /0x82B094|
|16|China_Elevation|0x475CB3 /0x82B084|
|17|Destruction|0x475CBB /0x82B078|
|18|China_Chinese|0x475CC3 /0x82B068|
|19|China_Korean|0x475CCB /0x82B058|
|20|China_Vietnamese|0x475CD3 /0x82B044|
|21|China_Khitan|0x475CDB /0x82B034|
|22|China_Mongol|0x475CE3 /0x82B024|
|23|China_Tibetan|0x475CEB /0x82B014|
|24|China_Xiongnu|0x475CF3 /0x82B004|
|25..37|empty|0x475CFB..0x475D3D /0x88EC4C|
|38|SprMain|0x475D44 /0x82AFFC|
|39|SprMain2|0x475D4F /0x82AFF0|
|40|SprMain3|0x475D5A /0x82AFE4|

This is the startup map, not a claim that every slot is always loaded or immutable: **FUN_005CCE80** can reload tileset16 as `China_Elevation_Dirt` (stringVA0x86A674). The Well API only needs the verified slot2; it does not extend unrelated ground dispatch or introduce a duplicate global archive resolver.

There are three numbering domains to keep distinct. A **code** packs tileset in bits9+ and its SG header group in low9. **T(code)** at0x408170 resolves the loader's group entry, then adds tileset×0x4000. A **native physical ID** is the on-disk record number, with dummy0 excluded and native vector index=physicalID−1. The original loader may trim its first200 system records: **FUN_005CD1F0 at0x5CD34F..0x5CD39A** tests nonzero tileset and first bitmap name `Zeus_system.bmp`, then subtracts200 from header group values at **0x5CD468..0x5CD471** before putting them into its tree. Publication at **0x5CD51B..0x5CD524** stores that normalized value−1. Thus the established `T=recordId−1+tileset×0x4000` uses the **loader-normalized record ID**, not necessarily the untrimmed physical SG record number. The native binding directly reads the physical SG header slot and does not subtract200 from its native vector index.

For the supplied General index, bitmap0 is `Zeus_system.bmp`, so group130/header1559 produces runtime low14 **1358=1559−201**, full word **0x854E**; group131/header1538 produces **1337=1538−201**, full word **0x8539**. These intermediate words are reproducible format facts, not serialized SAV image fields. Both resolve back to the physical IDs above. The consumer **FUN_005CE800 @0x5CE800** splits tileset and low14, chooses the tileset's runtime archive, and draws that record; the generic low14 consumer0x5CD8A0 retains the same namespace distinction.

| Branch | General imageIndex slot | Physical ID / native vector index | Independent SG format checks |
|---|---:|---:|---|
|predicate false,code0x482|130|1559 /1558|type30,width158,height132,tileSize2,uncompressed base12800 bytes,bitmap `China_Safety.bmp`,animation count20,speedID5|
|predicate true,code0x483|131|1538 /1537|type30,width158,height93,tileSize2,uncompressed base12800 bytes,bitmap `China_Safety.bmp`,animation count20,speedID4|

For each base, width **80×2−2=158**, height≥40×2, and uncompressed length **3200×2²=12800** agree with the independently saved/type-table2×2 footprint. Bitmap names are read through the record's bitmapId from **680+200×bitmapId**, not treated as per-image semantic names. The actual image association is supplied by the executable override; compatible dimensions/name alone would still be insufficient. Other building types in the24-byte table have not been bound by this change, since their state overrides and component rules require their own trace. Reproduce the86 authored/22 original assertions perRelease/UBSan with `scripts/building_image_binding.py`;it prints aggregates only and succeeds with SKIP for the data portion when `EMPEROR_GAMEDATA_ROOT` is unset/empty.

## Saved-building selector variants and format verification

The subsequent [complete observed-type trace](SAV.md#exe-cited-saved-building-static-bases) measures **22/27 observed live semantic types and927/1,065 live SAV record occurrences** bound through the same General tileset2. The27-type denominator includes2 entertainment-square occurrences without an ordinary footprint;26 types/1,063 footprints is a narrower placement count. This extension adds no filename inference orpixel scoring. Every type's constructor,vtable,operative selector/specialized writer,table row andsaved-field transport is cited in SAV. All executable references retain SHA-256 **`0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`** and file offset=VA−0x400000.

Packed-code group lookup and post-T record addition remain distinct. Houses useT(0x471/0x473/0x475)+(savedR&1),empty stalls useT(0x42A)+savedR%3,andHemp/Millet useT(0x431/0x42C)+saved stage+6×(savedR&1). Adding a variant to thepacked code would select a different header entry. P inWell/Inspector/Watchtower instead changes the code itself before T:0x482/483,0x486/487,0x488/489. R comes from theexe-cited serialized0xF1E780 grid,not theexisting public `mapperRandom` field;its exact transport is in SAV. No runtime PRNG call participates in these22 closed base selectors.

| Packed code(s) / variant domain | Header physical ID(s) → supported output IDs | Independent SG format contract / bitmap filename |
|---|---|---|
|0x471/0x473/0x475;+0..1|1509/1513/1517→1509..1510,1513..1514,1517..1518|type30;158×height≥80;tile2;base12,800;`China_Housing.bmp`|
|0x437|708|type30;158×95;tile2;base12,800;`China_Husbandry.bmp`|
|0x448/0x449|2789/2810|type30;158×92/140;tile2;base12,800;`China_Industry_2.bmp`|
|0x423|647|type30;398×289;tile5;base80,000;`China_StorNDist.bmp`|
|0x42A;+0..2|629→629..631|type30;158×80;tile2;base12,800;`China_StorNDist.bmp`|
|0x414/0x416|611/617|type30;158×123/114;tile2;base12,800;`China_StorNDist.bmp`|
|0x482/0x483|1559/1538|type30;158×132/93;tile2;base12,800;`China_Safety.bmp`|
|0x486/0x487|1618/1649|type30;158×193/194;tile2;base12,800;`China_Safety.bmp`|
|0x488/0x489|1680/1704|type30;158×187/198;tile2;base12,800;`China_Safety.bmp`|
|0x492|2046|type30;78×48;tile1;base3,200;`China_Government2.bmp`|
|0x4AD|825|type30;158×137;tile2;base12,800;`China_Husbandry.bmp`|
|0x43A|793|type30;238×192;tile3;base28,800;`China_Husbandry.bmp`|
|0x431;+0..11|2458→2458..2469|type30;78×height≥40;tile1;base3,200;`China_Fields.bmp`|
|0x42C;+0..11|2410→2410..2421|type30;78×height≥40;tile1;base3,200;`China_Fields.bmp`|
|0x484|1580|type30;158×136;tile2;base12,800;`China_Safety.bmp`|
|0x412|589|type30;158×137;tile2;base12,800;`China_Entertainment.bmp`|
|saved0x481|2232|type30;158×149;tile2;base12,800;`China_Religion.bmp`|

The geometry contract is width=80×footprintSide−2,height≥40×side,uncompressed base=3,200×side²; explicit nonzero tileSize must agree. The binder refuses negative mirrors,invalid archive versions/counts/header IDs,wrong caller archive,wrong bitmap family andvariants crossing bitmap families. The existing Well wrapper retains its previous geometry-only contract unchanged;its original-data tests independently check Safety metadata. All **57/57 per-type selector outputs** bind anddecode original payloads,including vacant/occupied house outputs,bothbranches for allthree P selectors andall12 stage/random combinations for eachcrop. Repeated house code outputs are counted pertype;57 is not the count ofunique artwork records. The corpus tests independently lock exact physical record distributions,not only bound counts.

Storage header records **0x424→644 (78×93)** and **0x425→645 (78×98)** are single-cell canvases;they do not meet savedWarehouse3×3/Trading4×4 geometry. Their specialized writers firststamp these images andthen overwrite8/15 component cells through0x5D5FF0. Markets,entertainment squares andwalls similarly requiretheir documented descriptor/component/neighbour rules. These five types remain **0/138 live records bound**;no footprint-sized image is invented fromone header record. Existingwriter0x4B7520's increment-per-cell semantics are distinct from0x4B72B0's same-word stamp.

The generalized preview produces **22/22 type previews andCGImages** using the existing compositor's explicitbottom-centre convention. Anchor,shadows,occlusion,animation andoriginal-screen fidelity remain unverified. Tests andprivate bite evidence are documented with theSAV selector contract;no original assets,code listings,images orderived binaries enterGit.

The later [SAV19 multipart closure](SAV.md#sav19-wall-and-multipart-building-closure) supersedes the five-type withholding above: **26/27 types and1,063/1,065 live records** now bind; **1,719/1,719 unique surface cells** place and decode. Executable SHA-256 and VA/file-offset convention are unchanged. Final storage images are one-cell components even though their saved footprints are3×3/4×4; image size is checked against each traced component, never the whole storage footprint.

| Exe-selected code / variant | General physical header / final IDs | Independent SG contract |
|---|---|---|
|Wall4B5 + neighbour/local-LCG variant0..12|370 /370..382|type30,78×height49..59,base3200,tile1,`China_Aesthetic.bmp`;13/13 outputs decode|
|Warehouse424; station425|644 /645|type30,78×93 /78×98,base3200,tile0 /1,`China_StorNDist.bmp`; origin cell only after component overrides|
|Storage occupied454 +4×(resource−1)+amountBand; empty456|1101 /1317|type30,78×height≥40,base3200,tile1,`China_Stor-bays.bmp`; all86/86 supplied storage component occurrences decode|
|Common-market pavement422 +(R&3)|632 /632..635|type30,78×40,base3200,tile1,`China_StorNDist.bmp`;24/24 new pavement cells decode; four linked2×2 stalls use their established42A/414/416 bindings|
|Entertainment4B0 +role/view table|603 /603..610 in view0|type256,tile0,`China_Entertainment.bmp`; dimensions65×95,99×76,69×95,99×76,68×75,71×75,57×44,57×44. The [SAV object-layer contract](SAV.md#sav19-entertainment-object-overlays) now places20/20 objects across2/2 records; these never become type30 surface stamps.|

For these entertainment objects, static top-left comes from the executable's role/view table atVA0x82BFE0, not SG record animationOffsetX/Y. The8/8 physical variants603..610 independently pass metadata and complete payload decoding. In view0 `0x44F86B→0x413960→0x5CD8A0` supplies scan-origin plus those signed offsets directly, with ordinary viewport clipping (`0x5297F0/0x5298A0`). Native centre-based projection converts the scan origin by(−39,−20). The compositor interleaves each type256 object with surface commands by anchor-cell depth and mode; the same source pixels and existing SG decoder are retained. The20/20 actual object occurrences decode without failures,including payload validation before offscreen culling. Original shadow treatment and complete original-screen fidelity are separate and unverified.

All new image choices cite their actual writers and serialized inputs in SAV. Market's generic48A branch is unreachable for its two common-market descriptor lists (all pavement args0). Storage uses direct per-cell overrides, wall uses0x4B72B0, and market mixes direct pavement writes with0x4B72B0 stalls. No newly supported type uses0x4B7520. Library calls remain explicit-root/archive operations and emit no assets.

## Figure frame advance

Executable SHA-256`0ca8fc07199ea319bc7e83b9f603a2f7b1225fcfd9b609e4e73c1a86cdc3c658`;code file offset=VA−0x400000. The authorized figure-logic trace closes the bounded shared primitive at`0x5CFE60`without assigning a universal time-per-figure-tick rule.

The routine resolves the supplied group code,reads signed i16 frame count at SG+30,and treats the caller's counter as **one unsigned byte**. It first resets an out-of-range counter to0 (`0x5CFE83..8D`). If speedArgument is0,it advances unconditionally;otherwise it reads u8 SG+58 and tests live flag`0x8901CC[100×SGspeed/speedArgument]` (`0x5CFE9F..B8`). An admitted advance increments and stores the byte,then resets it to0 if the new value reaches group length (`0x5CFEBA..C9`). Finally`0x5CFED5→0x5CFDF0`selects from the new counter and supplied direction. Therefore an out-of-range input resets to1 on an admitted advance for length>1,and0 on an ineligible one;byte255 increments to0 even for a group longer than255.

`originalFigureAdvanceFrame`accepts explicit51 current eligibility flags and supplied group metadata. It refuses nonpositive length,negative speed arguments andindices outside those51 flags. It never reads environment,wall clock or saved image words. `0x418AA0`refreshes the flags once before the outer simulation loop (`0x536B9E`);multiple fast-forward updates use the same flag set. Pheasant ordinary/actions31/32 use speedArgument0;action5 uses100. The exact call sites and [first-draw model](SAV_FIGURES.md#newly-verified-scheduler-callback-and-update-facts)are recorded with the per-type contract.

Authored checks distinguish reset-before-increment,reset despite ineligibility,group-end wrap,u8 overflow andscaled eligibility index8 from SGspeed4/speedArgument50. This primitive does not broaden the saved scene's tick0 contract or count as original-data update coverage on its own. The explicit [per-type update model](SAV_FIGURES.md#newly-verified-scheduler-callback-and-update-facts) now computes29/192 states (before7/192):kind13 stays7/7 using its own three-updates-per-frame rule;Pheasant action31 adds22/69 after target/link,cooldown andidle-state updates. Pheasant group4C2C or4C2B is chosen from the resulting action andterrain beforethis helper advances the frame withspeedArgument0. Updated images decode29/29 with0 failures,andindependent selector consistency is29/29;there is no original post-update oracle. SG3 parsing/decoding anddefault saved scene rendering are unchanged.
