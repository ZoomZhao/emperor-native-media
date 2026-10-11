# Python 优化模式验收门禁修复

独立复核发现 `PROFILE-GATE-OPTIMIZE-01`：在受控副本中把两项审核许可置为 false，并强制自写 codec 返回失败后，`python -O` 会移除断言，仍误报 334/0。此前未优化模式的 334/0、独立 owned 562/0 结果有效；问题针对可绕过的验收方式。

五个自写验收/数据工具入口现在检查 `sys.flags.optimize`，在读取资格文件、输入、创建结果目录或编译前退出 2。覆盖 CLI `-O` 和环境 `PYTHONOPTIMIZE=1`。真实文件工具的源哈希、profile 哈希、返回路径/导出检查另改为显式条件。当前私有数据控制器也加同类启动检查。独立导入的 owned 生命周期 runner 原本采用显式条件，无需改动。

启动回归 6 个 unittest 方法（13 个离散情形）全通过，包括 false gate、两个优化入口、源哈希不匹配与真实工具无 assert 节点检查。正常模式重跑核心 Release/UBSan 各 1215/0，profile 集成各 334/0。真实工具正常模式仅运行自造 raw ABC，完整结束、输出 3 字节；本次未重新读取真实数据。

五个生产 reader/codec 文件、原始外层 profile 与现有真实数据统计不变。修复不构成游戏加载、原 codec 执行、听觉或 UI 验证。精确源哈希与结果哈希见 [证据](profile-gate-optimization-fix-evidence.json)。
