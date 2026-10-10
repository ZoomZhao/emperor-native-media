# 自写驱动的仅编译检查

这项检查闭合现有 `native_driver.c` 缺少 `build_scope.h` 的编译依赖。它只生成一个 C 对象；入口、返回绑定、目标绑定必须仍未解析。没有城市函数链接、驱动执行或城市行为验收。

在仓库根目录使用现有 Apple clang、nm 和 Python：

```sh
/usr/bin/python3 -I -B scripts/compile_authored_driver.py --scope configs/authored-native-driver-scope.json --output-root /tmp
```

每次在输出根目录创建新的临时子目录，冻结明确列出的自写源码和配置，生成头文件、编译对象并运行 `nm -u`。编译参数和调试编译目录均使用相对路径。每个工具进程限时 60 秒。`result.json` 记录输入／源码／头文件／对象哈希、退出码、时间、全部未解析符号及 `linked=false`、`executed=false`。原项目路径、GameData、历史库和旧运行环境均不是输入；归档 Python 只作为文本来源记录，不导入或执行。

`--scope` 必需，没有隐含默认值；文件不存在或配置不完整时，在编译和创建构建目录前失败。配置仅接受六个字段：版本、`authored_compile_only` 类型、entry、return、排序且唯一的 allowed_pcs、target_count。PC 必须位于既有自造程序的 `0xa0000000..0xa0000fff` 范围，entry 在允许集合中，return 在集合外。此自造作用域的 target_count 必须为 0；这不会生成空表或成功 stub，目标表符号仍未解析。它是编译输入契约，不是可运行程序声明。其他自写程序的作用域需要单独给出并核对其来源，不能以类型字段自称 authored 就推断来源或许可。

编译清单引用七个已归档自写 C／头文件：stage82 的 native_driver.c、runtime.h；stage80 sparse_memory.h；stage79 guest_memory.h、thread_native.h；stage78 guarded_helpers.h；stage77 heap_native.h。共享头文件目录通过明确 `-I` 提供。稀疏 checkout 若缺这些文件，检查直接报告缺少源文件，不回退到旧项目或复制现成二进制。

## 自造配置的来源

提交的是新整理的编译参数契约，不是旧执行配置。四个 allowed PC 对应 `legacy/src/stage82/tools/build_drivers.py` 自造分支 `own_phase` 中的四次 `stage72_step`，该分支明确构造自己的 load／add 7／store／return LLVM。`legacy/src/stage82/tools/fixture.py` 将自造代码放入独立 `0xa0000000` 区域，声明 authored bytes only。return 是既有自造 caller 的五字节 CALL 返回位置。这里没有原指令、原地址绑定表、CPU 快照或原城市状态。

旧自造 JSON 同时带有 EXE／Unicorn／历史运行路径，故未收录它；仅整理上述自造数值字段。已查看实际自造源码分支、地址范围和作用域字段，并核对它们与旧头文件相符；封存 manifest 的相符仅用于完整性，没有被当作许可证明。所引用 C／头文件属于已审阅归档的自写平台接口；本增量遵循本仓库的 MIT 许可。该许可不推广到原 `game.ll`、原游戏派生库、原绑定表或游戏资产。

## 结果的准确含义

检查必须保留以下未解析组，否则失败：

- 入口／完成：`s82_entry`、`s82_completion`。
- 返回绑定：`stage72_return_bindings`、`stage72_return_bindings_count`。
- 目标绑定：`stage77_structural_code_bindings`。

其他平台与系统符号也完整保存在 nm 输出中。clang 退出 0 只证明此编译单元的头文件、类型及编译依赖闭合；既有本机 203 函数产物的存在、发布归档缺口和真实运行边界见 [CITY_BUILD_HANDOFF.md](CITY_BUILD_HANDOFF.md)。本轮凭据见 [authored-driver-compile-receipt.json](authored-driver-compile-receipt.json)。
