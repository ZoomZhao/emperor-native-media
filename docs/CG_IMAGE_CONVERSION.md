# App 共享 CoreGraphics 转换与已保留变更验收

2026-10-12。此前九项本地变更已由用户提交 `130790d31454fc5a65e1f72aab3bf29a1d442bf6` 并推送；期间用户另提交 `5dd5050`，本任务在写入前发现并保留。0736审查交付记录核对及发布状态补充已由 `6429195` 正常发布。

本次将 `main.mm` 中现有 CoreGraphics 转换原样抽成 `cg_image_conversion.hpp`，App 的 NSImage 包装直接消费同一个 CGImage 工厂。输入合同明确为紧密行、32位小端BGRA、预乘alpha；不新增RGBA接口、不进行直alpha转换。生产转换行为未改变，也没有发现新的生产缺陷。新 `image-conversion` 入口直接验证共享工厂，不以fake presenter替代实际转换；来源清单与App包内清单包含新头文件。

| 已保留的九项内容 | 本轮核对或验证 |
|---|---|
| `docs/BLOCKERS-AND-HANDOFF.md` | 保留历史正文，增加本轮验收及允许的下一缺口 |
| `game_lab/city_simulation_tests.cpp` | 当前自写模型 Release/UBSan 各734项通过；不证明原规则一致 |
| `surface_lab/run.sh` | 原样复制后实际执行，Release/UBSan各218646项；源码/结果哈希产出成功 |
| `docs/RUNTIME_PREPARATION.md` | 保留历史源码准备说明；不转为原执行链验收 |
| `docs/SOURCE_REPAIR.md` | 保留初次静态修复快照与后续结果链接 |
| `docs/runtime-preparation-static-audit.json` | JSON解析通过；旧命令/哈希仍为历史记录 |
| `docs/source-repair-static-audit.json` | JSON解析通过；不声称所有旧来源在当前版本重新审查 |
| `game_lab/boundary_tests.mm` | 原样冻结、实际编译执行，Release/UBSan各145项通过；完全自造WAV/JPEG/色值/临时文件 |
| `runtime_core/dependency-map.json` | 32条分类计数一致：23可定位、6缺生成绑定、3缺第三方ABI；未打开对应legacy源码 |

共享工厂 Release/UBSan 各4138项通过：非正/超限尺寸和短长输入拒绝，非方形2×3图案、行顺序/步幅/BGRA通道/256个alpha值逐字节一致，调用方改写和释放输入后原CGImage不变，256轮不同尺寸图像替换/释放保持原图。这里核对的是离屏图像数据，未验证窗口方向、缩放视觉、色彩管理或drawable。重复释放不是系统无泄漏证明；没有注入CoreGraphics分配失败，UBSan不覆盖系统框架。

含共享头文件的App已实际编译、链接、打包、签名strict校验，来源清单与新代码匹配；未启动App。每个受控构建/测试子进程限60秒，结果和源码绑定见[核验收据](cg-image-conversion-receipt.json)。第一次新测试因range-loop警告失败，随后因漏链接CoreFoundation失败，三份失败目录完整保留；修正测试/构建入口后通过，不能称生产缺陷修复。

复现入口：

```sh
python3 scripts/independent_runner.py image-conversion --mode release --output /absolute/new-output
python3 scripts/independent_runner.py image-conversion --mode undefined --output /absolute/new-output
python3 scripts/independent_runner.py app --output /absolute/new-output
```

本轮只做现有变更核对和这一项App共享转换验收，不扩展媒体测试。未使用GameData、原游戏二进制/资产/反汇编、旧Swift、原执行或衍生库，不启动UI/音频设备，不重试先前平台拒绝的具体操作。

与实际加载直接相关、可独立继续的普通工程缺口：把有界文件读取和当前解码器的“文件缺失/截断/格式不支持”等原因传给App，使用完全自造坏输入验证准确反馈、失败替换与资源清理。原版存档到当前加载器的实际身份/入口绑定、原规则对应、有效Bink支持仍未闭合；用户交付0736审查本身不证明这些能力，也不解除具体受阻操作的限制。
