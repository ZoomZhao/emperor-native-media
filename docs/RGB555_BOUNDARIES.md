# RGB555 随机边界验收与可继续范围

2026-10-11。未发现 RGB555 误解码、越界或 CPU/Metal 呈现数值差异；生产转换器和 GPU 内核没有改动。本次补齐格式生成器遗漏，并实现原先缺失的有界图形验收入口。自写城市仍在默认 App 中。

## 与原验收的区别

原 media 的 65536 字值覆盖 32768 种 RGB555 颜色及 bit 15 别名；CPU/Metal 已有固定步长、裁剪、缩放、重叠及颜色穷举。这不能证明随机组合覆盖了 RGB555：CPU 随机像素/生命周期和 Metal 随机像素此前只抽取 RGB565/BGRA，GPU 生命周期只用 BGRA。

本次将上述随机格式选择扩充为三格式。增加实际离屏 GPU 的 RGB555 非紧密 stride 生命周期：目标销毁复用、丢失和提交后改写应拒绝；提交后改写源仍消费提交时快照；多个独立目标排队完成后逐字节匹配独立标量 oracle，完成回调恰好一次。保留原有 padding、裁剪、缩放及重叠比较。

未加入重复固定用例或重复 CPU 释放循环；Metal 测试文件中的 CPU Store 循环不能算 GPU 压力测试。当前 Store/Backend 无色键参数，高位别名不能算色键能力。

## 可复现入口与证据

```sh
python3 scripts/independent_runner.py surface --mode release --output /absolute/new-output
python3 scripts/independent_runner.py metal --mode undefined --output /absolute/new-output
python3 scripts/independent_runner.py verify --report /absolute/report-directory
```

surface/metal 都支持 release 和 undefined。入口冻结显式源码，在独立目录编译和生成自造输入，每个子进程限 60 秒；保存源码、输入、二进制、结果、进程记录，并检查指纹及非零失败退出。无需 Git 元数据或 GameData。复核旧报告需要与其冻结源码一致的目录；当前未提交草稿不自动成为已验收源码。

| 已独立复核的范围 | 结果与限制 |
|---|---|
| CPU surface，Release/UBSan | 随机像素与生命周期、既有值域和边界比较通过 |
| 离屏 Metal，Release/UBSan | Apple M1 上逐字节标量/CPU 对照、RGB555 ticket 生命周期通过；未验证实际设备丢失 |
| 已发布城市、media、AppMedia integration，Release/UBSan | 自造输入回归通过；城市用已发布测试版本，另一个本地长序列草稿未混入 |
| App Release | 编译、打包、签名和系统链接通过；未启动 App |

上述 11 份报告的源码/输入/二进制/结果指纹均复核一致，最长子进程约 13.24 秒。任务目录 `rgb555-followup-20261011/qualified-validation-receipt.json` 保存对应报告位置与哈希，原始 ang 草稿完整备份。

另用 App 的实际 CoreGraphics 建图阶段制作无 AppKit 消费者，自造 3×2 不透明 BGRA 图案创建图像后改写源缓冲，图像仍保留原数据和形状。仅验证这一阶段的所有权；不代表 NSImage、窗口或完整游戏视觉通过。首次消费者缺 CoreFoundation 链接是测试准备问题，不是产品故障。

## 可以继续的三个工程任务

1. **把原生图像转换纳入共用验收入口。** 当前独立 CoreGraphics 检查仍在任务目录。将实际建图代码提取为 App 和 headless 消费者共用函数；以自造透明/不透明 BGRA、奇数宽度、源缓冲改写及无效长度核对。通过标准：输出尺寸、行布局和合同内像素一致；源改写不改变已创建图像；非法输入不产生图像或污染输出；创建/释放后受控对象归零。仍不声称窗口视觉通过。
2. **补音频恢复的完整状态比较。** 仅用自造 PCM 和 fake 设备事件，覆盖队列满、0/1/2 遍循环、暂停、失效、恢复及格式拒绝。通过标准：样本、游标、循环边界与独立模型一致；暂停/失效期间不消费；恢复接原游标；失败保持状态；完成/取消恰好一次；100 轮打开/停止/释放后受控队列/来源/事件归零。离线通过不等于听觉或物理设备通过。
3. **补视频多轮重开/seek/取消的资源验收。** 用自造 MOV、图片和稀疏 PTS 注入过期完成及取消。通过标准：旧 epoch 不呈现；首尾 seek、暂停和 EOF 合同一致且 EOF 一次；元数据超时/取消释放请求；排队上限保持；100 轮重开/取消后受控 Reader/请求归零。自造 MOV 通过不代表 Bink 或原影片兼容。

## 当前不能继续的范围

不恢复、包装或绕过已受阻的原城市指令恢复/差分路线；不读取原 EXE/反汇编、旧 Swift、GameData 内容，不执行原或衍生游戏库，不启用历史执行链。当前范围不包含项目 GUI、键盘输入、真实音频设备、依赖安装或除授权私有 Git 外的上传。GameData 已放入目录本身不增加验证能力。

普通自写组件、依赖整理、无设备自造验收、App 构建和选择性 Git 交付可以继续。原版玩法/存档兼容、全部真实资源、drawable/vblank、听觉和物理断连恢复仍无证据；不得用 GPU 读回、离线 PCM 或构建成功替代这些结论。详见 [执行范围](EXECUTION_SCOPE.md)。
