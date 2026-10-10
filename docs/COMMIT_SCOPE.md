# 本次提交的来源与验收边界

本增量从 5b1ef5d 增加现成自写 native_driver.c 的仅编译入口、来源明确的 authored 作用域参数及头文件生成方法、独立复现凭据和构建交接说明。只引用已有七个自写 C／头文件；归档源码本体不变，归档 Python 只读作来源记录，不导入或运行。

仓库根目录与无 Git 的独立导出、不同 cwd、全新临时构建目录均编译成功，nm 明确保留入口／返回绑定／目标绑定。缺必需配置在编译前失败。结果严格为未链接、未执行的单对象检查，见 [AUTHORED_DRIVER_COMPILE.md](AUTHORED_DRIVER_COMPILE.md) 与 [authored-driver-compile-receipt.json](authored-driver-compile-receipt.json)。

本机旧任务的 203 函数产物、配置与域文件存在；不是全局缺失。它们未被本增量复制、编译或归档，来源与现存行为边界见 [CITY_BUILD_HANDOFF.md](CITY_BUILD_HANDOFF.md)。没有 game.ll、原代码派生二进制、原绑定表、GameData、私有绝对路径或秘密进入提交。

独立发布 checkout 完成提交／推送；本增量未写入原活动目录或其索引，也未操作 ang 会话。原目录中观察到的其他源码变更留在原位，未混入本次提交。没有新增 App／媒体样例、完整城市消费者或受阻的原指令执行。本次不重复既有 App 回归作为新成果。
