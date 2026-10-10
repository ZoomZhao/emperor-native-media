# Game Lab：媒体资源浏览器

本模块包含资源加载器与 AppKit/NSImage 浏览器。地图页是地块编号的任意颜色预览，音乐/音效列表是候选文件名，均不代表游戏业务或资源完整性。没有完整城市逻辑、原游戏执行引擎、Metal 游戏渲染或影片播放器。

| 源码 | 支持范围 |
|---|---|
| `wav_pcm.cpp` | 内存 RIFF/WAVE，PCM16LE、1–8 声道、8000–192000Hz；校验 RIFF 范围、chunk/padding、format、byteRate/blockAlign、完整帧与重复块。奇数 data 在分配前拒绝；不重采样，不支持浮点/扩展压缩格式 |
| `asset_pixels.cpp` | 内存 RGB555LE → BGRA8，经 surface_lab 当前补位扩展合同；精确字节数、完整转换成功后才返回有效图像 |
| `emperor_assets.mm` | 有界、完整文件/区间读取；JPEG 在图像解码前检查元数据尺寸；SG3 完整表检查；现有 map 预览要求完整 128×128 解压输出；GB18030 非空文本行预览，非无损数据库导入 |
| `asset_limits.hpp`、`asset_bytes.hpp` | 总资源 128 MiB、WAV 64 MiB、文本 16 MiB；每边≤4096、像素≤4096²；这些是当前支持上限，不是全部资产兼容证明 |
| `resource_root.*` | 只接受用户显式配置的绝对目录；不搜索 cwd、不修改权限。资源路径必须相对，拒绝 parent traversal、绝对子路径、缺失/非普通文件及解析后逃出目录的 symlink |
| `app_media.*`、`audio_policy.hpp`、`app/main.mm` | 默认静音，无启动自动播放；音乐和音效共享 gain/静音；静音暂停音乐并停止音效，取消静音仅改变策略，仍需明确播放操作 |
| `boundary_tests.mm`、`run_boundaries.sh` | 完全自造 WAV/像素/JPEG/临时文件/context 策略用例；保存独立目录与源码/二进制/结果哈希。已编写，尚未执行 |
| `tests.mm`、`run.sh` | 旧 GameData 样本测试；已有 7 组/42 断言记录，没有当前修复后的新通过记录 |

图素提取仅支持 type 16/24/32 的精确原始 RGB555 方块，flags 必须为 0，uncompressedLength 须为 0 或等于预期。压缩/未知布局返回无效，不猜宽高、不补零；全部 SG3 图素兼容仍是缺口。JPEG 方向、色彩管理/视觉、分配失败注入和真实设备行为尚未验收。

后续明确允许运行时，可使用下列入口；本轮只检查源码与脚本语法，未调用这些命令：

```sh
make game-boundaries                           # 仅自造输入，默认 Release
bash game_lab/run_boundaries.sh undefined       # 自造输入 UBSAN
EMPEROR_BUILD_DIR=/absolute/fresh/build make game-app # 新目录构建，不自动启动
make run-game                                  # 显式传入仓库 GameData 绝对目录后启动
```

也可显式配置 `EmperorApp --data-root /absolute/path/to/GameData`，或环境变量 `EMPEROR_GAMEDATA_ROOT`；命令行优先。二者都未配置时会让用户明确选择资源目录，取消正常退出 0；明确配置无效仍返回 2，不弹选择器替代该配置。目录不打包、不提交、不保存为偏好；Finder 独立启动不猜测工作目录。配置成功不证明所有候选资源存在或读取权限齐全。实际选择窗口尚未运行验证，启动合同和 App 构建验收见 [APP_LAUNCH.md](../docs/APP_LAUNCH.md)。

路径过滤只是应用自身的配置合同，不是 OS sandbox；并发替换 symlink 等竞态没有专门防护验收。本轮没有绕过任何读取权限，也没有判断旧包外读取等待的系统原因。

静态语法通过不代表构建、链接、行为或听觉通过。旧 MP3 测试只验证文件存在与大小；旧图素测试只读取一例 16×16；这些不能扩展成全部音效、24/32 图素、影片或完整游戏通过。


## 当前纯逻辑实测

新增 pure_boundary_tests.mm 专用于最新范围：只自造 WAV/RGB555/JPEG、明确资源路径和纯静音状态；排除 SG3/map/文本/App/真实资产/设备。Release/UBSAN 各65,691断言通过，含65536字值域（32768颜色及高位别名），并已用失败回归修复不完整 WAV 格式扩展、截断 JPEG 假成功。JPEG 现在要求完整 EOI 和 ImageIO complete 状态；PCM 扩展要求字段/body 完整，严格合同尚未验证真实资源兼容。

输入由 self_fixture_evidence.hpp 留存在任务目录，实际记录和未覆盖范围见 [INDEPENDENT_VALIDATION.md](../docs/INDEPENDENT_VALIDATION.md)。此前“尚未执行”描述为初次准备快照；App、旧 tests.mm、通用全量 runner 和真实资源仍未运行。

## 当前 App 实际集成与仅构建验收

2026-10-10：AppMedia 已成为真实 App 与无 UI 测试共用的控制实现，补齐 App 音效到当前 WAV 校验、失败替换清理、默认静音/gain/显式播放/终止，以及 JPEG/raw555 的资源路径接线。Release/UBSAN 集成各 62 断言通过；当前 EmperorApp 编译、链接、打包和本地 codesign strict 校验通过，包内保存生产源码指纹，旧包保留。未启动 GUI、真实 AVAudioPlayer 或设备，未读取 GameData/legacy。

真实依赖清单、当前包证据和限制见 [APP_INTEGRATION.md](../docs/APP_INTEGRATION.md)。runtime_core 仍没有 App 消费者；音频队列/视频/Metal demo 未被接入 EmperorApp，完整业务引擎与状态/调度/交互/存档合同仍缺失。先前“只静态/未构建”是当时记录；当前新包与无 UI 集成已实际验证，不能推广为完整游戏或声音/GUI通过。新修复未 commit/push。

## 当前可复现入口

`bash game_lab/run_boundaries.sh release --output /absolute/new-output` 现在明确运行 pure_boundary_tests.mm，排除旧SG3/map/真实资产入口；`bash game_lab/run_app_integration.sh release --output /absolute/new-output` 运行真实AppMedia/fake后端无UI集成。二者支持 undefined，统一保存源码快照、实际输入/二进制/结果并限每进程60秒。当前六配置及只构建App已在干净导出通过。逐项阻塞与入口见 [BLOCKERS-AND-HANDOFF.md](../docs/BLOCKERS-AND-HANDOFF.md)。
