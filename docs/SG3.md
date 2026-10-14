# SG3 / 555

公开来源：[SG 格式](https://github.com/bvschaik/citybuilding-tools/wiki/SG-file-format)、[MIT sgreader](https://github.com/bvschaik/citybuilding-tools/tree/ea749c6e8bfa14ccb2ce5f1565b8f0501124a053/sgreader)；许可见根目录 THIRD_PARTY_NOTICES.md。实现是现代数据解析器，不使用原版代码。

索引起点 40680 = 80 字节头 + 600 字节索引 + 200×200 字节保留 bitmap 表。213/214 的图像记录分别为64/72字节；跳过 dummy ID0，保留其后使用中的逻辑 ID，包括镜像引用。头部容量槽位与使用中的计数分别读取，STATUS 分母用后者。

公开的 [Index 字段](https://github.com/bvschaik/citybuilding-tools/wiki/SG-file-format#index) 位于80，含300个 uint16 LE 图像ID。原生解析器现保留在 `SG3Archive::imageIndex`；`sg3LogicalIndexForGroup` 只作包内数字查找，返回排除dummy后的零基数组索引，拒绝ID0、越界slot、超过使用计数及不一致数组。容量范围不代表有效ID。真实58/58包的17,400个slot中，1,783个非零引用全部有效；Release/UBSan各53项自造检查通过。公开文档的市场slot示例来自Caesar3；Emperor的类别含义、MAP字段到slot的关系及选包规则仍未验证，不能套用。

尺寸在20/22，类型与压缩标志合读为 uint16（50）；bitmap ID在56。支持原始555、透明RLE、78×40 Emperor diamond、向前已有记录的负相对镜像，以及214 alpha。外部文件按 bitmap 名称定位并修正一字节偏移；禁止逃出所选目录。等距尺寸由78×40合同确定，避免58×30的120像素歧义。

本地58个索引中两份旧头值74480由公开兼容规则接受。9,059条mask均在颜色流后按RLE完整消费；把alpha字段当绝对文件偏移有7,918条不满足合同。保留该元数据，不猜测其原版内存意义。输出为预乘BGRA；F81F只用于旧原始type0色键，压缩字面保留（[公开问题9](https://github.com/bvschaik/citybuilding-tools/issues/9)）。

运行 `make sg3-test` 和 `EMPEROR_GAMEDATA_ROOT=/absolute/GameData make data-metrics`。未设置变量正常SKIP，真实资产只读；只输出汇总数字，不导出图像或dump。窗口动态列出全部SG3，按逻辑ID原生显示；失败清空旧图。

指标记录在 STATUS.md：解码/CGImage成功、缺依赖及当前校验拒绝分别计数。透明fallback不算成功。像素与原版显示逐图一致性未测；完整场景、动画、地图语义及游戏逻辑未由此证明。

Anyang 原版截图的离线核验现有**78处水面图块位置 /40个视觉ID**，在同一1,478个78×40 type30候选池内，78/78均有唯一5位RGB掩码匹配。它们含**374,400个逻辑RGB项**、**1,497,600个物理RGB项**；后者包括2倍缩放的四倍重复，不是更多独立内容。此前8块对应38,400逻辑项和153,600物理项。原始8位色仍有RMSE2.814–2.899、最大误差6，未修改解码颜色合同。同水域ROI的参考位置覆盖从22.94%增至53.15%；这仍只证明源图像内容出现在截图位置，不证明MAP字段、选包、变体/动画或全城绘制。相对格子间距已测量，绝对格坐标仍有14个条件候选，岸线留出验证不通过。详见 [配准和比较限制](MAP.md#offline-anyang-registration-and-sg3-visual-samples)。派生像素及坐标记录只保存在本地。

零尺寸、零载荷的逻辑记录单独返回 `EmptyRecord`，包括镜像指向同类记录；保留ID，不生成透明替代图，不计解码成功。真实数据全部427条符合此精确形状。该判断在依赖解析之前进行，避免把空记录误报成缺像素文件。此前418条未分类拒绝及9条先被缺文件遮蔽的记录已归此类；非空记录71,943条中192条缺同一个companion，其余全部通过，当前未发现有效记录的解码拒绝或未支持类型。分母与原版视觉一致性边界保持不变。

原生消费者按枚举数组的实际长度检查选择索引，不保留旧固定六包上限。真实窗口逐个打开58/58个索引，总计72,370个逻辑ID；首记录57个实际显示、1个明确缺依赖。空记录及缺依赖均清空旧画面，空记录在缺像素文件的包内也不被误报成依赖失败。
