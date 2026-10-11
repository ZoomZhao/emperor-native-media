# OwnedFramedReader独立生命周期复核

固定提交：**08152cc48fa0229ea2e0d592a1c6f201145f45b8**。复核新增owned入口及其复用的自写parser/codec，不重复另一线程的原文件profile审核。使用独立自造三marker profile、原样/zlib payload与系统zlib，没有读取原EXE、GameData、真实存档或原代码衍生产物，也没有改主目录、生产源码、index、push、UI或设备。

结论：在公开接口和下述支持域内，**未发现新增所有权/生命周期缺陷**。Release/UBSan各 **562检查通过、0失败**；对象copy/move构造和copy/move赋值的四个compile-only负例全部因deleted操作被拒绝。562包含类型检查、owned正常/错误/析构检查和分开的borrowed前提对照，不是562种文件格式或原存档通过。ASan本批未运行，不宣称ASan通过或UBSan能证明全部内存安全。

## 直接有用的所有权结果

- **复制输入**：从const lvalue构造后，显式释放原vector storage（delete观察确认），overwrite/reallocate原vector并作为output连续读取原样与zlib块，内容及计数正确；原输入局部变量销毁后也可继续读取。
- **移动输入**：从rvalue转移输入后，修改、扩容和复用moved-from vector作output。原encoded backing在两块成功、End及错误重复调用期间保持存活，直到owned对象析构才释放。output在owner析构后仍有效。
- **对象自身转移**：四种copy/move操作显式删除，六个static_assert与运行traits检查一致，另四个真实编译拒绝探针验证调用者不能执行这些操作。用unique_ptr在首次读取前、两块之间、End后和错误后交接，pointee地址不变且状态继续正常。
- **内部引用和销毁顺序**：Buffer先完成private vector接收与setg，然后istream指向buffer，FramedReader引用istream。成员声明为buffer_/source_/reader_，按C++逆声明顺序析构reader→istream→buffer；对象不能copy/move，因此三者地址不会被公开操作重定位。运行delete观察验证owned backing没有在output swap时提前释放，并覆盖24次未读/部分读取后销毁。
- **连续与重复**：48个原样/zlib混合块含零产量块，每次复用并修改原vector作output。首次和重复End结果正确；后块截断、未知marker、坏Adler、累计输出限制和块数限制后，保留先前output，重复错误保持完整FrameResult与新的调用者output（内容/data/capacity均不变）。

空输入分别测试：默认ContainerHeader的零字节输入=Truncated；仅四字节合成container header=End；RequiredFrame的零字节输入=Truncated；RequiredFrame的完整原样零块先Frame再End。它们是既有自写reader的现代政策，本复核没有将它们认定为真实原文件终止规则。profile按值保存也已验证：构造后修改调用者profile不会更改reader当前规则。

## 支持域与禁止alias前提分开

owned支持的是合法std::vector<uint8_t>的按值传入、对象存活期间调用next、有效output vector及公开copy/move输入协议。拷贝后原vector可修改/销毁；移动后可正常操作moved-from vector。调用者仍须遵守存储转移约定，不能通过保留的原指针/迭代器越界、释放或并发改写owned backing。没有公开底层流来允许替换内部streambuf；本复核没有使用private访问宏、修改header或绕过删除的copy/move。

借用FramedReader的source/backing须覆盖全部调用并与output storage独立。这条禁止alias前提依然必要；opaque istream不保证自动检查共享底层存储。本次另做两项对照：

| 对照 | 独立观察 |
|---|---|
| borrowed有效独立source/output | 原样与zlib两块成功，backing未释放，guard未触发。 |
| borrowed streambuf借用vector X，next(X) | 首块swap后原backing释放；下一次读由guard在触碰过期storage前拦截，reader返回IoError，output保留。**actual_expired_read=false**。 |

负例是已知READER-BORROWED-ALIAS-01禁止用法的安全复现，不是新增owned缺陷，也没有把第二次实际UAF执行作为验收。负例只有确认首次释放已被观察到才尝试guarded read；观察器未满足前提则停止并使回归失败。GuardedBorrowedBuffer不暴露get-area，每次xsgetn/underflow先检查释放标志，释放后不做任何pointer arithmetic/memcpy/字节读取。移动owned输入的受监测用例在继续下一次读取前也检查backing未释放，若提前释放则停止测试。释放观察只记录地址整数，并在第一次释放后关闭匹配，避免地址复用被误判。测试的全局new/delete替换只用于测试进程，不进入生产源码或交付运行接口。

最小对照的实现位于新测试的borrowed_support_boundary；owned copy_path/move_path以相同“复用原vector作output”的模式验证修复路径。所有合成bytes由独立测试构造，zlib压缩只调用系统compress2，没有复制作者六项测试或原codec实现。

## 复跑与来源

合入新测试资料后，在包含固定08152cc组件的checkout执行：

~~~sh
python3 -B file_io/run_owned_lifetime_independent.py --output /tmp/emperor-owned-lifetime-NEW
~~~

独立任务副本使用：

~~~sh
python3 -B file_io/run_owned_lifetime_independent.py \
  --component-root /Users/zoomzhao/Documents/Codex/2026-10-11/task-2/emperor-native-media/baseline/owned-reader-08152cc/file_io \
  --output /tmp/emperor-owned-lifetime-NEW
~~~

输出必须为全新绝对目录。runner先校验三个header和两个CPP的固定SHA，不接受悄悄换版；只冻结并构建这些新自写文件、独立测试与系统zlib，使用已安装clang++/SDK，C++17 -Wall/-Wextra/-Werror/-pedantic。Release=-O2；UBSan=-O1/-g/-fsanitize=undefined/-fno-sanitize-recover=all。每个子进程限60秒，日志、源/二进制SHA、逐检查结果、四个预期编译失败源码/诊断与命令均保留。编译拒绝负例不运行，不与正常构建失败混计。

五个生产输入SHA：

| 固定自写文件 | SHA256 |
|---|---|
| include/owned_reader.hpp | c7d3f5ada30ddddd748c429f61f7bb4525504bccf0cc6d016325b8e60a13e937 |
| include/framed_reader.hpp | d3d21e07110f57a19cd37d7e1fa61a922203a4c2ccb7c4988f60cb2e543abbd9 |
| include/strict_zlib.h | 8884a8a9bc6d8222320d93023d088cd18430017e479a282ca698f102c3eaf7a4 |
| src/framed_reader.cpp | 648bb362ff03f96afa0596cc5c84e628b1b666d78325d41099a70181eab5d68b |
| src/strict_zlib.cpp | 1c13bf06ba9721a85000105f4b18763f88bfadf71626f2cae1127288c6fb2f0e |

原作者6项/构建收据按docs/reader-lifetime-contract-evidence.json指向的实际路径读取并核对SHA d0fd22a0ed6a4ec6272b1f754bd47f132d2444d944bdb0157e707f164025b223，原字节保留；独立结果另存，不改成作者报告的一部分。该路径实际在2026-10-08/task-2/reader-lifetime-contract-20261011，当前2026-10-11/task-2同名目录不存在。这是定位差异，已用提交中的准确引用解决，不是授权阻塞。

## 复核限制

未对构造前vector物化/参数复制的bad_alloc另做注入；它可以在OwnedFramedReader构造之前抛出，调用者须限制物化规模并处理异常。parser限额仍不是整个进程heap上限。没有进行多线程同步/并发修改、故意保留指针破坏owned backing、标准库全部析构路径或原文件兼容认证。本批未复跑历史ASan smoke；已发表合同记录的空main初始化失败不变，本批不把它归因于组件或计成组件内存诊断。

patch只新增独立CPP测试、runner和本复核文档。固定组件和原契约保持原SHA；无须生产修复补丁。逐结果、来源快照和复跑命令见独立交付manifest。
