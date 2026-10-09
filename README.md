# emperor-native-media

私有的独立原生媒体组件实验与验收源码。所有图像、PCM和MOV测试输入由本仓库程序自造，默认静音。当前用途是像素表面、离线音频队列、视频时序、Metal读回、AVFoundation解码与App生命周期的可复现实验。没有原游戏执行文件、资源、存档、旧实现、二进制转换器或游戏逻辑；不代表完整游戏已通过。

## 环境与从干净 checkout 开始

需要macOS 15或以上及已安装的Apple Command Line Tools/Xcode，提供clang++、Python 3、make、xcrun、codesign、plutil。使用C++17/Objective-C++与系统AppKit、Metal、AVFoundation、CoreMedia/CoreVideo、CoreAudio等框架，无第三方依赖、下载或自动安装。当前主要验收环境为Apple Silicon；Intel、其他GPU和其他系统版本未验证。

在仓库根目录逐项运行：

```sh
make fixtures
make build
make surface
make audio
make video
make metal
make av
make stream
make integration-release
make integration-undefined
make reliability-release-metadata
make reliability-undefined-metadata
make reliability-release-faults
make reliability-undefined-faults
make reliability-release-long
make reliability-undefined-long
```

`make fixtures`从源码生成8秒色条＋48kHz双声道S16 PCM的MOV、重复为32秒素材，再生成有明确用途的截断/损坏负向样本。素材在`.generated/`，不依赖任何已验收本机文件或提交的媒体二进制。未完成生成的目录保留，失败后先归档该目录再重试，不能悄悄使用旧素材。各有界脚本整轮最多55秒，长素材实际运行约32秒。不要用一个长进程拼接全部验收；各项单独执行。

`make build`生成`media_demo/build/NativeMediaDemo.app`，只做本地ad hoc签名及静态验证。App默认待机，点击播放后使用自造8秒包内素材；PCM在内存消费并丢弃，不连接音频输出。构建与测试日志、二进制、所有素材都留在忽略目录，不进入Git。

## 模块

| 目录 | 用途 |
|---|---|
| surface_lab | RGB565LE/BGRA8表面（App显示显式转换RGBA8）、stride、裁剪、最近邻缩放、重叠复制与生命周期；独立标量参考核对 |
| audio_lab | S16队列、循环次数、暂停/恢复、事件和句柄生命周期；逐样本参考核对，无音频设备 |
| video_lab | 自造帧时间轴和有界流队列、到期帧、seek、epoch与EOF |
| metal_lab | 自造表面上传、离屏复制/缩放、GPU完成与读回；不涉及drawable/vblank |
| av_lab | 自造媒体容器与图像/PCM解码验收、系统编码器枚举 |
| stream_lab | 自造MOV生成器、有界AVAssetReader、两秒统一元数据超时/取消与呈现器 |
| media_demo | 静音App、真实单调时钟、只读CoreAudio事件监听、明确注入的恢复事件和释放验收 |

## 已验证与边界

原本机验收覆盖像素格式/裁剪/缩放/句柄释放，PCM队列/循环/格式，解码PTS和PCM逐值核对，8/32秒真实时钟，2秒延迟追赶，6类真实坏媒体，超时/取消、连续重开与资源归零。视频队列最多4＋持有1帧/62640字节，PCM最多2来源/16384字节，原生PCM包≤65536字节，Reader最多1；GPU完成后受控请求归零。相位差小于1600 PCM帧，采样到UI/读回≤100ms，8/32秒EOF上限分别8.15/32.15秒。结果与导出复验见[验收摘要](VALIDATION.md)。

真实声音、听觉质量、音频硬件时钟、物理设备切换、物理鼠标/键盘、完整人工视觉、drawable/vblank和小时级稳定性未验证。设备恢复事件是注入测试，默认设备只读取、监听，不改设置。离线PCM核对不构成听觉验证，GPU读回不构成游戏通过。历史ASAN初始化曾失败，默认脚本使用Release/UBSAN，不把ASAN列为已通过。

曾对既有包外自造MOV作有限诊断：CLI全读与AV回调通过，App在内核open等待且未返回错误；具体系统策略未确认，不能断言TCC拒绝。包外读取未修复，本仓库没有文件导入功能；包内成功不代表包外读取能力。未提交该诊断程序、日志或内存信息，也不提供提升权限或变更读取路径的办法。

## 可选自身窗口回归

已有Accessibility授权时可分别运行以下入口；没有授权则只报告缺失，不申请、不弹出权限设置。辅助程序只通过AX操作自己新建且核对PID、启动时间和包路径的App实例；不发送物理输入、不截图、不操作其他App。

```sh
make gui-helper
make gui-regression
make reliability-gui-long
make reliability-gui-reopen
make reliability-gui-loading
make clean-rebuild
```

其中私有测试可选包内32秒或坏媒体，并记录关闭时资源。加载门控是主动延迟完成通知，用于超时/取消路径，不伪称实际权限拒绝。`clean-rebuild`从两个全新输出目录编译并比较完整App包字节，只在一致后推广本仓库内的包。

## 源码与许可

未擅自添加开源许可证，本仓库保持私有，仅用于用户自己的组件开发与验收。源码为本任务自写实现、测试参考模型与生成器；检查未发现第三方源码署名/许可块、复制档案或外部依赖。Apple SDK/框架通过已安装系统工具引用，未随仓库分发。未授予对外公开、再分发或其他开源许可；如需公开，须由用户选择许可并复核。
