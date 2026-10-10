# 自造输入的原生内存验收

2026-10-10：自写 guest_memory 提供者在 Release 和 UBSan 下通过新增验收；UBSan 同时从无 Git 的五文件导出、任意工作目录复现。生产 C/H 未修改。既有 runtime_core 测试使用 vector，归档 memory_selftest 已有基础边界 smoke；本次补足原生权限、故障清理和重复生命周期证据。

## 实际覆盖

- 相邻 R/RW 区域的跨区读取、权限拒绝、跨空洞失败；失败读保留输出，失败写保留数据及计数。
- 4 KiB 客体片段落在同一 16 KiB 宿主页时，实际内核页面按 RW 并集合并；客体 guard 仍拒绝只读片段写入和空洞访问。通过当前进程的 Mach VM 查询确认 R/RW 均无 X，未使用区域为无权限。
- 32 位地址空间最后四字节、长度溢出、指针转换及一过末端拒绝。
- 定向注入 malloc、mmap、fopen、非法页大小、首次及最终 mprotect 失败，确认每个指定故障实际命中一次；真实 munmap/free/fclose 释放已获取资源，失败对象归零，可重复关闭。
- 自造短文件和非法文件偏移导致导入失败；随后完整自造文件可成功导入，内核实际权限为只读且禁止执行。
- 64 次打开、写入、关闭、再关闭、复用已关闭对象；每轮重新建立状态，关闭后不能读取旧数据。

两配置各通过 1,509 项断言。实际宿主页为 16 KiB；4 KiB/8 KiB 等其他宿主页尚未实测。故障注入证明指定错误返回路径的清理，不代表真实资源耗尽已发生。封装函数只编入测试提供者的编译单元，正常路径调用真实 libc/内核函数；没有修改生产提供者，也没有证明未插桩二进制的所有行为。

## 复现和证据

要求 64 位 macOS、系统 clang 和 Python 3。输出必须指定为绝对路径，所有编译/链接/测试子进程各限制 60 秒：

```sh
python3 scripts/guest_memory_runner.py --mode release --output /tmp/native-memory-results
python3 scripts/guest_memory_runner.py --mode undefined --output /tmp/native-memory-results
python3 scripts/guest_memory_runner.py --verify /tmp/native-memory-results/guest-memory-配置-实际目录名
```

脚本冻结且仅编译以下五个自写文件：runner、guest_memory_native_tests.c、guest_memory_test_hooks.h、guest_memory.c、guest_memory.h。两份输入文件由 runner 生成；不调用归档 memory_selftest 的可选 checkpoint 分支。报告保存源文件、实际输入、产物、进程、结果及哈希；verify 复核当前源码、冻结源码、输入、产物和报告一致性。发布证据摘要见 [native-guest-memory-receipt.json](native-guest-memory-receipt.json)。

本验收不读取 GameData、EXE、checkpoint，不运行客体代码或游戏衍生库，不启动 App、不操作设备。内存组件合同通过不能替代游戏、城市业务或 App 接入验收。

## 接下来可独立推进的三项工作

| 工程任务 | 具体通过标准 |
|---|---|
| graphics 自造像素与呈现合同 | 明确 RGB555/RGBA/BGRA、步幅、alpha；用奇数尺寸、带填充行及四角标记图验证转换、负坐标/越界裁剪和缩放；失败与迟到事件不得呈现旧图；关闭/重建后旧图资源不可继续访问。GPU 读回仅记录像素合同，真实窗口另验收。 |
| audio 自造 PCM/事件合同 | 覆盖单声道/双声道、采样率与不支持格式、队列顺序、循环次数和取消；模拟设备失效与恢复后只有有效 ticket 可重建/恢复，迟到完成不得覆盖当前状态；所有错误路径释放一次，静音默认保持。离线解码与 fake 事件通过只证明样本和状态合同，听觉及设备恢复需另外授权并实际验证。 |
| video 合成帧消费者 | 先建立 PTS、seek、EOF、取消和呈现接口；用自产帧序列验证单调 PTS、seek 后旧帧失效、EOF 一次、取消后不呈现以及资源释放；接入自写消费者后再做 App 只构建。现无真实影片消费者或有效 Bink 解码证据，不以合成帧通过冒充原格式解码。 |

可以继续自写源码、接口、既有报告和依赖的静态审查，以及获授权的独立组件实现与自造验收。不能恢复此前被平台拦截的具体原指令执行/探测动作，不能换会话绕过；历史产物存在或 GameData 已放入目录不会改变该边界。
